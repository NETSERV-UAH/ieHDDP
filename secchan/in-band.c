/* Copyright (c) 2008 The Board of Trustees of The Leland Stanford
 * Junior University
 *
 * We are making the OpenFlow specification and associated documentation
 * (Software) available for public use and benefit with the expectation
 * that others will use, modify and enhance the Software and contribute
 * those enhancements back to the community. However, since we would
 * like to make the Software available for broadest use, with as few
 * restrictions as possible permission is hereby granted, free of
 * charge, to any person obtaining a copy of this Software to deal in
 * the Software under the copyrights without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * The name and trademarks of copyright holder(s) may NOT be used in
 * advertising or publicity pertaining to the Software or any
 * derivatives without specific, written prior permission.
 */

#include <config.h>
#include "in-band.h"
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include "flow.h"
#include "mac-learning.h"
#include "netdev.h"
#include "ofp.h"
#include "ofpbuf.h"
#include "openflow/openflow.h"
#include "packets.h"
#include "port-watcher.h"
#include "rconn.h"
#include "secchan.h"
#include "status.h"
#include "timeval.h"
#include "vlog.h"

/**Modificaciones boby UAH**/
#include "../oflib/ofl-messages.h"
// #include "vconn.h" //Temporal
#include "vconn-stream.h" //Temporal

#define TCP_DROP false
#define UDP_DROP false
#define ARP_DROP false
#define CTRL_PRIORITY 0xfff0
#define RULE_PRIORITY 0xfff1
#define DROP_PRIORITY 0xffff
#define IDLE_TCP_RULE_TIMEOUT 5  //Segundos máximos sin utilizar la regla TCP
#define IDLE_ARP_RULE_TIMEOUT 10 //Segundos máximos sin utilizar la regla ARP

// static void install_new_localport_rules_UAH(struct rconn *local_rconn,  uint32_t new_port, struct in_addr ip);
struct in_addr local_ip = {0};
//***FIN***//

#define LOG_MODULE VLM_in_band

struct in_band_data
{
    const struct settings *s;
    struct mac_learning *ml;
    struct netdev *of_device;
    struct rconn *controller;
    int n_queued;
    //Modificaciones Boby UAH//
    struct port_watcher *pw; // Para poder buscar el número del puerto físico compartido con el puerto local
    //+++FIN+++//
};

static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(60, 60);

static void
queue_tx(struct rconn *rc, struct in_band_data *in_band, struct ofpbuf *b)
{
    rconn_send_with_limit(rc, b, &in_band->n_queued, 10);
}

static const uint8_t *
get_controller_mac(struct in_band_data *in_band)
{
    static uint32_t ip, last_nonzero_ip;
    static uint8_t mac[ETH_ADDR_LEN], last_nonzero_mac[ETH_ADDR_LEN];
    static time_t next_refresh = 0;

    uint32_t last_ip = ip;

    time_t now = time_now();

    ip = rconn_get_ip(in_band->controller);
    if (last_ip != ip || !next_refresh || now >= next_refresh)
    {
        bool have_mac;

        /* Look up MAC address. */
        memset(mac, 0, sizeof mac);
        if (ip && in_band->of_device)
        {
            int retval = netdev_arp_lookup(in_band->of_device, ip, mac);
            if (retval)
            {
                VLOG_DBG_RL(LOG_MODULE, &rl, "cannot look up controller hw address "
                                             "(" IP_FMT "): %s",
                            IP_ARGS(&ip), strerror(retval));
            }
        }
        have_mac = !eth_addr_is_zero(mac);

        /* Log changes in IP, MAC addresses. */
        if (ip && ip != last_nonzero_ip)
        {
            VLOG_DBG(LOG_MODULE, "controller IP address changed from " IP_FMT " to " IP_FMT, IP_ARGS(&last_nonzero_ip), IP_ARGS(&ip));
            last_nonzero_ip = ip;
        }
        if (have_mac && memcmp(last_nonzero_mac, mac, ETH_ADDR_LEN))
        {
            VLOG_DBG(LOG_MODULE, "controller MAC address changed from " ETH_ADDR_FMT " to " ETH_ADDR_FMT,
                     ETH_ADDR_ARGS(last_nonzero_mac), ETH_ADDR_ARGS(mac));
            memcpy(last_nonzero_mac, mac, ETH_ADDR_LEN);
        }

        /* Schedule next refresh.
         *
         * If we have an IP address but not a MAC address, then refresh
         * quickly, since we probably will get a MAC address soon (via ARP).
         * Otherwise, we can afford to wait a little while. */
        next_refresh = now + (!ip || have_mac ? 10 : 1);
    }
    return !eth_addr_is_zero(mac) ? mac : NULL;
}

static bool
is_controller_mac(const uint8_t dl_addr[ETH_ADDR_LEN],
                  struct in_band_data *in_band)
{
    const uint8_t *mac = get_controller_mac(in_band);
    return mac && eth_addr_equals(mac, dl_addr);
}

static void
in_band_learn_mac(struct in_band_data *in_band,
                  uint16_t in_port, const uint8_t src_mac[ETH_ADDR_LEN])
{
    if (mac_learning_learn(in_band->ml, src_mac, 0, in_port))
    {
        VLOG_DBG_RL(LOG_MODULE, &rl, "learned that " ETH_ADDR_FMT " is on port %" PRIu16,
                    ETH_ADDR_ARGS(src_mac), in_port);
    }
}

static bool
in_band_local_packet_cb(struct relay *r, void *in_band_)
{
    struct in_band_data *in_band = in_band_;
    struct rconn *rc = r->halves[HALF_LOCAL].rconn;

    struct ofl_msg_packet_in *oflpi;
    struct eth_header *eth;
    struct ofpbuf *payload, buf_arp, buf_amaru;
    struct ofpbuf *buf;
    uint32_t ip4_aux_1, ip4_aux_2;
    char ip4_1[INET_ADDRSTRLEN], ip4_2[INET_ADDRSTRLEN];
    struct flow flow, flow_inv = {0};
    uint32_t in_port, out_port;      /*, priority = 0xfff1; */
    uint32_t buffer_id = 0xffffffff; //NO_BUFFER, para no especificar ningún flujo almacenado por el switch

    if (!get_ofp_packet_eth_header_UAH(r, &oflpi, &eth, &in_port, &buf) || !in_band->of_device)
    {
        return false;
    }
    if (in_port == 254) //¿No es suficiente con la condicion in_port==254?)
    {
        in_port = OFPP_LOCAL;
    }

    if (oflpi != NULL)
    {
        payload = ofpbuf_new(oflpi->data_length);
        memcpy(payload->data, oflpi->data, oflpi->data_length);

        // (payload->data) = (oflpi->data);
        payload->size = oflpi->data_length;
    }
    else
    {
        return false;
    }
    // VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: In port= %u\t OFL_PACKET_IN->TOTAL LENGTH = %u", in_port, oflpi->total_len);
    flow_extract(payload, in_port, &flow);
    if (local_ip.s_addr == 0)
    {
        netdev_get_in4(in_band->of_device, &local_ip);
    }
    //Manejamos el paquete amaru que indica el neuvo puerto local
    if (eth->eth_type == htons(ETH_TYPE_AMARU))
    {
        uint32_t *new_local_port, *old_local_port;
        uint8_t *char_size, mac[ETH_ADDR_LEN];
        char *port_name, ip_char[INET_ADDRSTRLEN];
        struct in_addr *local_ip_amaru, controller_ip;
        // update_port_watcher_ports_UAH(in_band->pw); // Se intenta actualizar los puertos del port_watcher.
        buf_amaru = *payload;
        ofpbuf_try_pull(&buf_amaru, ETH_HEADER_LEN);                          //Nos deshacemos de la cabecera ethernet
        new_local_port = ofpbuf_try_pull(&buf_amaru, sizeof(uint32_t));       //Se obtiene el número del nuevo puerto local
        char_size = ofpbuf_try_pull(&buf_amaru, sizeof(uint8_t));             //Se obtiene el tamaño del nombre del puerto
        port_name = ofpbuf_try_pull(&buf_amaru, *char_size);                  //Se obtiene el nombre del nuevo puerto local
        local_ip_amaru = ofpbuf_try_pull(&buf_amaru, INET_ADDRSTRLEN);        //Se obtiene la ip del puerto local
        memcpy(mac, ofpbuf_try_pull(&buf_amaru, ETH_ADDR_LEN), ETH_ADDR_LEN); //Se obtiene la MAC del puerto local
        old_local_port = ofpbuf_try_pull(&buf_amaru, sizeof(uint32_t));       //Se obtiene el número del antiguo puerto local

        inet_ntop(AF_INET, &local_ip_amaru->s_addr, ip_char, INET_ADDRSTRLEN);
        VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: AMARU PACKET  NEW PORT = %s(%u)\t IP: %s", port_name, *new_local_port, ip_char);
        controller_ip.s_addr = rconn_get_ip(in_band->controller);
        install_new_localport_rules_UAH(r->halves[HALF_LOCAL].rconn, new_local_port, local_ip_amaru, &controller_ip, mac, old_local_port);
        // modify_socket_options_rconn_UAH(r->halves[HALF_REMOTE].rconn, port_name);

        VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: Puerto local PORT WATCHER = %u", get_pw_local_port_number_UAH(in_band->pw));

        return false; // Para que no envíe el packet in al controlador.
    }

    if (eth->eth_type == htons(ETH_TYPE_ARP))
    {
        // Se obtien la cabecera ARP
        struct arp_eth_header *arp;
        buf_arp = *payload;
        eth = ofpbuf_try_pull(&buf_arp, ETH_HEADER_LEN);
        arp = ofpbuf_try_pull(&buf_arp, ARP_ETH_HEADER_LEN);

        if (arp->ar_tpa == rconn_get_ip(in_band->controller) && !eth_addr_equals(arp->ar_sha, netdev_get_etheraddr(in_band->of_device))) //Se comprueba si la IP buscada es la del Controlador
        {

            in_band_learn_mac(in_band, in_port, eth->eth_src); //Aprende la mac del salto anterior
            // Puerto físico donde está el controlador .
            out_port = get_pw_local_port_number_UAH(in_band->pw);

            //Se configura la regla inversa
            flow_inv.dl_type = flow.dl_type;
            // flow_inv.nw_dst = flow.nw_src;
            memcpy(flow_inv.dl_dst, eth->eth_src, ETH_ADDR_LEN); // MAC switch origen como destino
            // queue_tx(rc, in_band, make_add_simple_flow(&flow_inv, ntohl(oflpi->buffer_id), in_port, IDLE_ARP_RULE_TIMEOUT, RULE_PRIORITY)); // Regla para el tráfico de vuelta
            queue_tx(rc, in_band, make_add_simple_flow(&flow_inv, buffer_id, in_port, IDLE_ARP_RULE_TIMEOUT, RULE_PRIORITY)); // Regla para el tráfico de vuelta

            /* If the switch didn't buffer the packet, we need to send a copy. */
            if (ntohl(oflpi->buffer_id) == UINT32_MAX)
            {
                VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: PACKET_OUT OUT_PORT %u", out_port);
                queue_tx(rc, in_band, make_unbuffered_packet_out(payload, in_port, out_port));
            }
            else
            {
                VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: PACKET_OUT BUFFER_ID %u \tOUT_PORT %u", oflpi->buffer_id, out_port);
                queue_tx(rc, in_band, make_buffered_packet_out(oflpi->buffer_id, in_port, out_port));
            }
        }
        else
        {
            return false;
        }
    }
    else if (eth->eth_type == htons(ETH_TYPE_IP) && flow.nw_dst == rconn_get_ip(in_band->controller) && flow.nw_src != local_ip.s_addr) //Se podría quitar esta última condición
    {
        is_controller_mac(eth->eth_src, in_band);
        out_port = get_pw_local_port_number_UAH(in_band->pw);

        ip4_aux_1 = flow.nw_dst;
        ip4_aux_2 = flow.nw_src;

        inet_ntop(AF_INET, &ip4_aux_1, ip4_1, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip4_aux_2, ip4_2, INET_ADDRSTRLEN);
        VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: Reglas SWITCH-CTRL %u==%s---%u==%s", ip4_aux_2, ip4_2, ip4_aux_1, ip4_1);

        // queue_tx(rc, in_band, make_add_simple_flow(&flow, ntohl(oflpi->buffer_id), out_port, IDLE_TCP_RULE_TIMEOUT, RULE_PRIORITY)); // Regla para el tráfico de ida
        queue_tx(rc, in_band, make_add_simple_flow(&flow, ntohl(buffer_id), out_port, IDLE_TCP_RULE_TIMEOUT, RULE_PRIORITY)); // Regla para el tráfico de ida

        //Se configura la regla inversa
        flow_inv.dl_type = flow.dl_type;
        flow_inv.nw_proto = flow.nw_proto;
        flow_inv.nw_dst = flow.nw_src;
        flow_inv.nw_src = flow.nw_dst;
        // queue_tx(rc, in_band, make_add_simple_flow(&flow_inv, ntohl(oflpi->buffer_id), in_port, IDLE_TCP_RULE_TIMEOUT, RULE_PRIORITY)); // Regla para el tráfico de vuelta
        queue_tx(rc, in_band, make_add_simple_flow(&flow_inv, ntohl(buffer_id), in_port, IDLE_TCP_RULE_TIMEOUT, RULE_PRIORITY)); // Regla para el tráfico de vuelta

        /* If the switch didn't buffer the packet, we need to send a copy. */
        if (ntohl(oflpi->buffer_id) == UINT32_MAX)
        {
            VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: PACKET_OUT OUT_PORT %u", out_port);
            queue_tx(rc, in_band, make_unbuffered_packet_out(payload, in_port, out_port));
        }
        else
        {
            VLOG_WARN(LOG_MODULE, "[IN BAND LOCAL PACKET CB]: PACKET_OUT BUFFER_ID %u \tOUT_PORT %u", oflpi->buffer_id, out_port);
            queue_tx(rc, in_band, make_buffered_packet_out(oflpi->buffer_id, in_port, out_port));
        }
    }
    else
    {
        return false;
    }

    return true;
}

static void
in_band_status_cb(struct status_reply *sr, void *in_band_)
{
    struct in_band_data *in_band = in_band_;
    struct in_addr local_ip;
    uint32_t controller_ip;
    const uint8_t *controller_mac;

    if (in_band->of_device)
    {
        const uint8_t *mac = netdev_get_etheraddr(in_band->of_device);
        if (netdev_get_in4(in_band->of_device, &local_ip))
        {
            status_reply_put(sr, "local-ip=" IP_FMT, IP_ARGS(&local_ip.s_addr));
        }
        status_reply_put(sr, "local-mac=" ETH_ADDR_FMT, ETH_ADDR_ARGS(mac));

        controller_ip = rconn_get_ip(in_band->controller);
        if (controller_ip)
        {
            status_reply_put(sr, "controller-ip=" IP_FMT,
                             IP_ARGS(&controller_ip));
        }
        controller_mac = get_controller_mac(in_band);
        if (controller_mac)
        {
            status_reply_put(sr, "controller-mac=" ETH_ADDR_FMT,
                             ETH_ADDR_ARGS(controller_mac));
        }
    }
}

void get_ofp_packet_payload(struct ofp_packet_in *opi UNUSED, struct ofpbuf *payload UNUSED)
{
    //payload->data = opi->data;
    //payload->size = ntohs(opi->header.length) - offsetof(struct ofp_packet_in,
    //                                                   data);
    //**Modificaciones boby UAH**//
    // payload->data = opi->data;
    VLOG_WARN(LOG_MODULE, "HEADER LENGTH=%" PRIu16 "\tTOTAL LENGTH=%" PRIu16 "\tPACKET_IN LENGH=%zu", ntohs(opi->header.length), ntohs(opi->total_len), sizeof(*opi));
    // VLOG_WARN(LOG_MODULE, "SIZE ntohs=%u\tSIZE=%u",ntohs(opi->header.length) - offsetof(struct ofp_packet_in,data),opi->header.length - offsetof(struct ofp_packet_in,data));
    // payload->size = ntohs(opi->header.length) - offsetof(struct ofp_packet_in,data);
    ////
}

// //**FIN**//
static void
in_band_local_port_cb(const struct ofp_port *port, void *in_band_)
{
    struct in_band_data *in_band = in_band_;
    if (port)
    {
        char name[sizeof port->name + 1];
        get_port_name(port, name, sizeof name);

        if (!in_band->of_device || strcmp(netdev_get_name(in_band->of_device), name))
        {
            int error;
            netdev_close(in_band->of_device);
            error = netdev_open(name, NETDEV_ETH_TYPE_NONE,
                                &in_band->of_device);
            if (error)
            {
                VLOG_ERR(LOG_MODULE, "failed to open in-band control network device "
                                     "\"%s\": %s",
                         name, strerror(errno));
            }
        }
    }
    else
    {
        netdev_close(in_band->of_device);
        in_band->of_device = NULL;
    }
}

static void
in_band_periodic_cb(void *in_band_)
{
    struct in_band_data *in_band = in_band_;
    mac_learning_run(in_band->ml, NULL);
}

static void
in_band_wait_cb(void *in_band_)
{
    struct in_band_data *in_band = in_band_;
    mac_learning_wait(in_band->ml);
}

static struct hook_class in_band_hook_class = {
    in_band_local_packet_cb, /* local_packet_cb */
    NULL,                    /* remote_packet_cb */
    in_band_periodic_cb,     /* periodic_cb */
    in_band_wait_cb,         /* wait_cb */
    NULL,                    /* closing_cb */
};

//Modificaciones Boby UAH//
void install_in_band_rules_UAH(struct rconn *local_rconn, struct in_band_data *in_band)
{
    struct in_addr local_ip;
    struct flow arp_flow = {0}, tcp_flow = {0}, local_mac_flow = {0};
    uint32_t local_port_no;
    uint32_t buffer_id = 0xffffffff; //NO_BUFFER, para no especificar ningún flujo almacenado por el switch
    // uint16_t priority = 0xfff0;
    // uint16_t priority_drop = 0xffff;
    arp_flow.dl_type = htons(ETH_TYPE_ARP);
    tcp_flow.dl_type = htons(ETH_TYPE_IP);
    tcp_flow.nw_proto = IP_TYPE_TCP;

    /*¡¡¡¡Estas reglas no son necesarias para el simple_switch_13 de RYU. Tampoco son necesarias con ONOS si se utiliza RECTIVE FORWARDING.!!!!*/
    rconn_send(local_rconn, make_add_simple_flow(&arp_flow, buffer_id, OFPP_CONTROLLER, OFP_FLOW_PERMANENT, CTRL_PRIORITY), NULL); // Regla para procesar los paquetes ARP de otros switches.
    rconn_send(local_rconn, make_add_simple_flow(&tcp_flow, buffer_id, OFPP_CONTROLLER, OFP_FLOW_PERMANENT, CTRL_PRIORITY), NULL); // Regla para procesar los paquetes TCP de otros switches.
    // rconn_send(local_rconn, make_add_simple_flow(&tcp_flow, buffer_id, OFPP_CONTROLLER, OFP_FLOW_PERMANENT, 6), NULL); // Regla para procesar los paquetes TCP de otros switches.

    //Se configuran los drop para evitar bucles con el propio tráfico
    local_port_no = get_pw_local_port_number_UAH(in_band->pw); // Se obtiene el número del puerto físico que comparte interfaz con el puerto local.
    VLOG_WARN(LOG_MODULE, "[INSTALL IN BAND RULES UAH]: Puerto físico del controlador = %u", local_port_no);
    netdev_get_in4(in_band->of_device, &local_ip);

    //!!Los drops configurados en el puerto local no harían falta al haber desactivado el procesamiento de paquetes en el puerto local!!
    // arp_flow.in_port = htonl(OFPP_LOCAL);
    // arp_flow.nw_dst = 0; // Drop sin tener en cuenta la IP destino
    // rconn_send(local_rconn, make_add_simple_flow(&arp_flow, buffer_id, 0, OFP_FLOW_PERMANENT, priority_drop), NULL);

    // // Drop TCP entrante en el puerto local
    // tcp_flow.in_port = htonl(OFPP_LOCAL);
    // rconn_send(local_rconn, make_add_simple_flow(&tcp_flow, buffer_id, 0, OFP_FLOW_PERMANENT, priority_drop), NULL);

    /*Los DROP utilizando la dirección IP de la interfaz del puerto local pueden sustituirse por los dos siguientes DROP que utilizan la dirección MAC*/
    /*Así se aumenta la compatibilidad ya que funcionaría con ONOS y RYU sin hacer más cambios*/
    // Drop TCP con destino el Switch e in_port = puerto físico del controlador
    // tcp_flow.nw_dst = local_ip.s_addr;
    // tcp_flow.in_port = htonl(local_port_no);
    // rconn_send(local_rconn, make_add_simple_flow(&tcp_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);

    // // Drop TCP con origen el Switch
    // tcp_flow.nw_src = local_ip.s_addr;
    // tcp_flow.nw_dst = 0;
    // tcp_flow.in_port = htonl(local_port_no);
    // rconn_send(local_rconn, make_add_simple_flow(&tcp_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);

    //Se instalan dos reglas para no procesar en la interfaz del puerto local los paquetes con MAC origen/destino la de la interfaz del puerto local
    //MAC ORIGEN LA DE LA INTERFAZ DEL PUERTO LOCAL
    local_mac_flow.in_port = htonl(local_port_no);
    memcpy(local_mac_flow.dl_src, netdev_get_etheraddr(in_band->of_device), ETH_ADDR_LEN);
    rconn_send(local_rconn, make_add_simple_flow(&local_mac_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);

    //MAC DESTINO LA DE LA INTERFAZ DEL PUERTO LOCAL
    memset(local_mac_flow.dl_src, 0, ETH_ADDR_LEN);
    memcpy(local_mac_flow.dl_dst, netdev_get_etheraddr(in_band->of_device), ETH_ADDR_LEN);
    rconn_send(local_rconn, make_add_simple_flow(&local_mac_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);
}

void install_new_localport_rules_UAH(struct rconn *local_rconn, uint32_t *new_local_port, struct in_addr *local_ip UNUSED, struct in_addr *controller_ip, uint8_t *mac, uint32_t *old_local_port)
{
    uint32_t buffer_id = 0xffffffff;
    // uint16_t priority_drop = 0xffff;
    struct flow /*tcp_flow = {0},*/ tcp_mod_flow = {0}, del_drop = {0}, drop_mod_flow = {0};
    // char ip_char[INET_ADDRSTRLEN];

    // Se eliminan los flujos DROP para el antiguo puerto local (Basados en la MAC de la interfaz)
    memcpy(del_drop.dl_src, mac, ETH_ADDR_LEN);
    del_drop.in_port = htonl(*old_local_port);
    rconn_send(local_rconn, make_del_flow(&del_drop, 0x00), NULL);

    memset(del_drop.dl_src, 0, ETH_ADDR_LEN);
    memcpy(del_drop.dl_dst, mac, ETH_ADDR_LEN);
    rconn_send(local_rconn, make_del_flow(&del_drop, 0x00), NULL);

    // DROP con origen/destino la MAC de la interfaz del puerto local teniendo en cuenta el nuevo puerto local
    drop_mod_flow.in_port = htonl(*new_local_port);
    memcpy(drop_mod_flow.dl_src, mac, ETH_ADDR_LEN);
    rconn_send(local_rconn, make_add_simple_flow(&drop_mod_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);

    //MAC DESTINO LA DE LA INTERFAZ DEL PUERTO LOCAL
    memset(drop_mod_flow.dl_src, 0, ETH_ADDR_LEN);
    memcpy(drop_mod_flow.dl_dst, mac, ETH_ADDR_LEN);
    rconn_send(local_rconn, make_add_simple_flow(&drop_mod_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);

    // Drop TCP con origen/destino el Switch teniendo en cuenta el nuevo puerto
    // inet_ntop(AF_INET, &local_ip->s_addr, ip_char, INET_ADDRSTRLEN);
    // VLOG_WARN(LOG_MODULE, "[INSTALL NEW LOCALPORT RULES]: IP: %s\t Puerto: %u", ip_char, *new_local_port);
    // tcp_flow.dl_type = htons(ETH_TYPE_IP);
    // tcp_flow.nw_proto = IP_TYPE_TCP;

    // tcp_flow.nw_dst = local_ip->s_addr;
    // tcp_flow.in_port = htonl(*new_local_port);
    // rconn_send(local_rconn, make_add_simple_flow(&tcp_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);

    // tcp_flow.nw_src = local_ip->s_addr;
    // tcp_flow.nw_dst = 0;
    // tcp_flow.in_port = htonl(*new_local_port);
    // rconn_send(local_rconn, make_add_simple_flow(&tcp_flow, buffer_id, 0, OFP_FLOW_PERMANENT, DROP_PRIORITY), NULL);

    /*Se modifican los flujos TCP con destino el controlador para sacar los paquetes por el nuevo puerto de control*/
    // inet_pton(AF_INET, "10.0.0.104", &del_ip.s_addr);
    tcp_mod_flow.dl_type = htons(ETH_TYPE_IP);
    tcp_mod_flow.nw_proto = IP_TYPE_TCP;
    tcp_mod_flow.nw_dst = controller_ip->s_addr;
    // rconn_send(local_rconn, make_del_flow(&tcp_del_flow,0x00),NULL);
    // rconn_send(local_rconn, modify_local_port_in_band_rules_UAH(&tcp_mod_flow, buffer_id, 0x00, RULE_PRIORITY, *new_local_port), NULL);
    rconn_send(local_rconn, make_del_flow(&tcp_mod_flow, 0x00), NULL);

    // Se eliminan los flujos DROP para el antiguo puerto local (basados en la IP de la interfaz)
    // tcp_del_flow.dl_type = htons(ETH_TYPE_IP);
    // tcp_del_flow.nw_proto = IP_TYPE_TCP;
    // tcp_del_flow.nw_dst = local_ip->s_addr;
    // tcp_del_flow.in_port = htonl(*old_local_port);
    // rconn_send(local_rconn, make_del_flow(&tcp_del_flow, 0x00), NULL);

    // tcp_del_flow.nw_src = local_ip->s_addr;
    // tcp_del_flow.nw_dst = 0;
    // rconn_send(local_rconn, make_del_flow(&tcp_del_flow, 0x00), NULL);
}
/*+++FIN+++*/

void in_band_start(struct secchan *secchan,
                   const struct settings *s, struct switch_status *ss,
                   struct port_watcher *pw, struct rconn *remote)
{
    struct in_band_data *in_band;

    in_band = xcalloc(1, sizeof *in_band);
    in_band->s = s;
    in_band->ml = mac_learning_create();
    in_band->of_device = NULL;
    in_band->controller = remote;
    //Modificaciones Boby UAH//
    in_band->pw = pw;
    //+++FIN+++//
    switch_status_register_category(ss, "in-band", in_band_status_cb, in_band);
    port_watcher_register_local_port_callback(pw, in_band_local_port_cb,
                                              in_band);
    add_hook(secchan, &in_band_hook_class, in_band);
}
