/* 
 * This file is part of the HDDP Switch distribution (https://github.com/gistnetserv-uah/eHDDP-inband).
 * Copyright (c) 2020.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* The original Stanford code has been modified during the implementation of
 * the OpenFlow 1.1 userspace switch.
 *
 * Author: Zoltán Lajos Kis <zoltan.lajos.kis@ericsson.com>
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include "dp_exp.h"
#include "dp_ports.h"
#include "datapath.h"
#include "packets.h"
#include "pipeline.h"
#include "oflib/ofl.h"
#include "oflib/ofl-messages.h"
#include "oflib-exp/ofl-exp-openflow.h"
#include "oflib/ofl-log.h"
#include "util.h"

#include "vlog.h"
#define LOG_MODULE VLM_dp_ports

static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(60, 60);

#if defined(OF_HW_PLAT)
#include <openflow/of_hw_api.h>
#include <pthread.h>
#endif


#if defined(OF_HW_PLAT) && !defined(USE_NETDEV)
/* Queue to decouple receive packet thread from rconn control thread */
/* Could make mutex per-DP */
static pthread_mutex_t pkt_q_mutex = PTHREAD_MUTEX_INITIALIZER;
#define PKT_Q_LOCK pthread_mutex_lock(&pkt_q_mutex)
#define PKT_Q_UNLOCK pthread_mutex_unlock(&pkt_q_mutex)

static void
enqueue_pkt(struct datapath *dp, struct ofpbuf *buffer, of_port_t port_no,
            int reason)
{
    struct hw_pkt_q_entry *q_entry;

    if ((q_entry = xmalloc(sizeof(*q_entry))) == NULL) {
        VLOG_WARN(LOG_MODULE, "Could not alloc q entry\n");
        /* FIXME: Dealloc buffer */
        return;
    }
    q_entry->buffer = buffer;
    q_entry->next = NULL;
    q_entry->port_no = port_no;
    q_entry->reason = reason;
    pthread_mutex_lock(&pkt_q_mutex);
    if (dp->hw_pkt_list_head == NULL) {
        dp->hw_pkt_list_head = q_entry;
    } else {
        dp->hw_pkt_list_tail->next = q_entry;
    }
    dp->hw_pkt_list_tail = q_entry;
    pthread_mutex_unlock(&pkt_q_mutex);
}

/* If queue non-empty, fill out params and return 1; else return 0 */
static int
dequeue_pkt(struct datapath *dp, struct ofpbuf **buffer, of_port_t *port_no,
            int *reason)
{
    struct hw_pkt_q_entry *q_entry;
    int rv = 0;

    pthread_mutex_lock(&pkt_q_mutex);
    q_entry = dp->hw_pkt_list_head;
    if (dp->hw_pkt_list_head != NULL) {
        dp->hw_pkt_list_head = dp->hw_pkt_list_head->next;
        if (dp->hw_pkt_list_head == NULL) {
            dp->hw_pkt_list_tail = NULL;
        }
    }
    pthread_mutex_unlock(&pkt_q_mutex);

    if (q_entry != NULL) {
        rv = 1;
        *buffer = q_entry->buffer;
        *port_no = q_entry->port_no;
        *reason = q_entry->reason;
        free(q_entry);
    }

    return rv;
}
#endif


/* FIXME: Should not depend on udatapath_as_lib */
#if defined(OF_HW_PLAT) && !defined(USE_NETDEV) && defined(UDATAPATH_AS_LIB)
/*
 * Receive packet handling for hardware driver controlled ports
 *
 * FIXME:  For now, call the pkt fwding directly; eventually may
 * want to enqueue packets at this layer; at that point must
 * make sure poll event is registered or timer kicked
 */
static int
hw_packet_in(of_port_t port_no, of_packet_t *packet, int reason,
             void *cookie)
{
    struct sw_port *port;
    struct ofpbuf *buffer = NULL;
    struct datapath *dp = (struct datapath *)cookie;
    const int headroom = 128 + 2;
    const int hard_header = VLAN_ETH_HEADER_LEN;
    const int tail_room = sizeof(uint32_t);  /* For crc if needed later */

    VLOG_INFO(LOG_MODULE, "dp rcv packet on port %d, size %d\n",
              port_no, packet->length);
    if ((port_no < 1) || port_no > DP_MAX_PORTS) {
        VLOG_ERR(LOG_MODULE, "Bad receive port %d\n", port_no);
        /* TODO increment error counter */
        return -1;
    }
    port = &dp->ports[port_no];
    if (!PORT_IN_USE(port)) {
        VLOG_WARN(LOG_MODULE, "Receive port not active: %d\n", port_no);
        return -1;
    }
    if (!IS_HW_PORT(port)) {
        VLOG_ERR(LOG_MODULE, "Receive port not controlled by HW: %d\n", port_no);
        return -1;
    }
    /* Note:  We're really not counting these for port stats as they
     * should be gotten directly from the HW */
    port->rx_packets++;
    port->rx_bytes += packet->length;
    /* For now, copy data into OFP buffer; eventually may steal packet
     * from RX to avoid copy.  As per dp_run, add headroom and offset bytes.
     */
    buffer = ofpbuf_new(headroom + hard_header + packet->length + tail_room);
    if (buffer == NULL) {
        VLOG_WARN(LOG_MODULE, "Could not alloc ofpbuf on hw pkt in\n");
        fprintf(stderr, "Could not alloc ofpbuf on hw pkt in\n");
    } else {
        buffer->data = (char*)buffer->data + headroom;
        buffer->size = packet->length;
        memcpy(buffer->data, packet->data, packet->length);
        enqueue_pkt(dp, buffer, port_no, reason);
        poll_immediate_wake();
    }

    return 0;
}
#endif

#if defined(OF_HW_PLAT)
static int
dp_hw_drv_init(struct datapath *dp)
{
    dp->hw_pkt_list_head = NULL;
    dp->hw_pkt_list_tail = NULL;

    dp->hw_drv = new_of_hw_driver(dp);
    if (dp->hw_drv == NULL) {
        VLOG_ERR(LOG_MODULE, "Could not create HW driver");
        return -1;
    }
#if !defined(USE_NETDEV)
    if (dp->hw_drv->packet_receive_register(dp->hw_drv,
                                            hw_packet_in, dp) < 0) {
        VLOG_ERR(LOG_MODULE, "Could not register with HW driver to receive pkts");
    }
#endif

    return 0;
}

#endif


/* Runs a datapath packet through the pipeline, if the port is not set to down. */
static void
process_buffer(struct datapath *dp, struct sw_port *p, struct ofpbuf *buffer) {
    struct packet *pkt;

    if ((p->conf->config & (OFPPC_NO_RECV | OFPPC_PORT_DOWN)) != 0) {
        ofpbuf_delete(buffer);
        return;
    }

    // packet takes ownership of ofpbuf buffer
    pkt = packet_create(dp, p->stats->port_no, buffer, false);
    pipeline_process_packet(dp->pipeline, pkt);
}

void
dp_ports_run(struct datapath *dp) {
    // static, so an unused buffer can be reused at the dp_ports_run call
    static struct ofpbuf *buffer = NULL;
    int max_mtu = 0;

    struct sw_port *p, *pn;

#if defined(OF_HW_PLAT) && !defined(USE_NETDEV)
    { /* Process packets received from callback thread */
        struct ofpbuf *buffer;
        of_port_t port_no;
        int reason;
        struct sw_port *p;

        while (dequeue_pkt(dp, &buffer, &port_no, &reason)) {
            p = dp_ports_lookup(dp, port_no);
            /* FIXME:  We're throwing away the reason that came from HW */
            process_packet(dp, p, buffer);
        }
    }
#endif

    // find largest MTU on our interfaces
    // buffer is shared among all (idle) interfaces...
    LIST_FOR_EACH_SAFE (p, pn, struct sw_port, node, &dp->port_list) {        
        const int mtu = netdev_get_mtu(p->netdev);
        if (IS_HW_PORT(p)) 
            continue;
        if (mtu > max_mtu)
            max_mtu = mtu;
    }

    LIST_FOR_EACH_SAFE (p, pn, struct sw_port, node, &dp->port_list) {
        int error;
        /* Check for interface state change */
        enum netdev_link_state link_state = netdev_link_state(p->netdev);
        if (link_state == NETDEV_LINK_UP){
            p->conf->state &= ~OFPPS_LINK_DOWN;
            dp_port_live_update(p);
        }
        else if (link_state == NETDEV_LINK_DOWN){
            p->conf->state |= OFPPS_LINK_DOWN;
            dp_port_live_update(p);
        }

        if (IS_HW_PORT(p)) {
            continue;
        }

        /*Modificaciones Boby UAH */
        if (p->conf->port_no == OFPP_LOCAL)
        {
            continue;
        }

        //+++FIN+++//

        if (buffer == NULL) {
            /* Allocate buffer with some headroom to add headers in forwarding
             * to the controller or adding a vlan tag, plus an extra 2 bytes to
             * allow IP headers to be aligned on a 4-byte boundary.  */
            const int headroom = 128 + 2;
            buffer = ofpbuf_new_with_headroom(VLAN_ETH_HEADER_LEN + max_mtu, headroom);
        }
        error = netdev_recv(p->netdev, buffer, VLAN_ETH_HEADER_LEN + max_mtu);
        if (!error) {
            p->stats->rx_packets++;
            p->stats->rx_bytes += buffer->size;
            // process_buffer takes ownership of ofpbuf buffer
            process_buffer(dp, p, buffer);
            buffer = NULL;

            /*Modificaciones UAH*/
            /*Se comprueba si se ha recibido paquetes en la interfaz configurada como puerto local 
            para poder dar por finalizada la configuración del puerto local*/
            if (dp->local_port != NULL && !strcmp(p->conf->name, dp->local_port->conf->name))
            {

                link_state = netdev_link_state(dp->local_port->netdev);
                if (link_state != NETDEV_LINK_DOWN && !local_port_ok)
                {
                    VLOG_WARN(LOG_MODULE, "[DP PORTS RUN]: El nuevo puerto local >> %s << está operativo.", dp->local_port->conf->name);

                    /*Si se ha recibido paquetes a través de la interfaz configurada como nuevo puerto local
                    se considera que ha finalizado la configuración del nuevo puerto local*/
                    local_port_ok = true; 
                }
            }
            /*+++FIN+++*/

        } else if (error != EAGAIN) {
            VLOG_ERR_RL(LOG_MODULE, &rl, "error receiving data from %s: %s",
                        netdev_get_name(p->netdev), strerror(error));
        }
    }

}

/* Returns the speed value in kbps of the highest bit set in the bitfield. */
static uint32_t port_speed(uint32_t conf) {
    if ((conf & OFPPF_1TB_FD) != 0)   return 1024 * 1024 * 1024;
    if ((conf & OFPPF_100GB_FD) != 0) return  100 * 1024 * 1024;
    if ((conf & OFPPF_40GB_FD) != 0)  return   40 * 1024 * 1024;
    if ((conf & OFPPF_10GB_FD) != 0)  return   10 * 1024 * 1024;
    if ((conf & OFPPF_1GB_FD) != 0)   return        1024 * 1024;
    if ((conf & OFPPF_1GB_HD) != 0)   return        1024 * 1024;
    if ((conf & OFPPF_100MB_FD) != 0) return         100 * 1024;
    if ((conf & OFPPF_100MB_HD) != 0) return         100 * 1024;
    if ((conf & OFPPF_10MB_FD) != 0)  return          10 * 1024;
    if ((conf & OFPPF_10MB_HD) != 0)  return          10 * 1024;

    return 0;
}

/* Creates a new port, with queues. */
static int
new_port(struct datapath *dp, struct sw_port *port, uint32_t port_no,
         const char *netdev_name, const uint8_t *new_mac, uint32_t max_queues)
{
    struct netdev *netdev;
    struct in6_addr in6;
    struct in_addr in4;
    int error;
    uint64_t now;

    now = time_msec();

    max_queues = MIN(max_queues, NETDEV_MAX_QUEUES);

    error = netdev_open(netdev_name, NETDEV_ETH_TYPE_ANY, &netdev);
    if (error) {
        return error;
    }
    /*Modificaciones UAH*/
    VLOG_WARN(LOG_MODULE, "netdev->MAC: "ETH_ADDR_FMT"", ETH_ADDR_ARGS(netdev_get_etheraddr(netdev)));
    if (!eth_addr_equals(netdev_get_etheraddr(netdev), old_local_port_MAC)){
        memcpy(old_local_port_MAC, netdev_get_etheraddr(netdev), ETH_ADDR_LEN); //Guardamos la MAC inicial del puerto que va a ser configurado como local
    }
    if (new_mac)
        VLOG_WARN(LOG_MODULE, "new_mac: "ETH_ADDR_FMT"", ETH_ADDR_ARGS(new_mac));
    /*+++FIN+++*/
    if (new_mac && !eth_addr_equals(netdev_get_etheraddr(netdev), new_mac)) {
        /* Generally the device has to be down before we change its hardware
         * address.  Don't bother to check for an error because it's really
         * the netdev_set_etheraddr() call below that we care about. */

        netdev_set_flags(netdev, 0, false);
        error = netdev_set_etheraddr(netdev, new_mac);
        if (error) {
            VLOG_WARN(LOG_MODULE, "failed to change %s Ethernet address "
                      "to "ETH_ADDR_FMT": %s",
                      netdev_name, ETH_ADDR_ARGS(new_mac), strerror(error));
        }
    }
    error = netdev_set_flags(netdev, NETDEV_UP | NETDEV_PROMISC, false);
    if (error) {
        VLOG_ERR(LOG_MODULE, "failed to set promiscuous mode on %s device", netdev_name);
        netdev_close(netdev);
        return error;
    }
    if (netdev_get_in4(netdev, &in4)) {
        VLOG_ERR(LOG_MODULE, "%s device has assigned IP address %s",
                 netdev_name, inet_ntoa(in4));
    }
    if (netdev_get_in6(netdev, &in6)) {
        char in6_name[INET6_ADDRSTRLEN + 1];
        inet_ntop(AF_INET6, &in6, in6_name, sizeof in6_name);
        VLOG_ERR(LOG_MODULE, "%s device has assigned IPv6 address %s",
                 netdev_name, in6_name);
    }

    if (max_queues > 0) {
        error = netdev_setup_slicing(netdev, max_queues);
        if (error) {
            VLOG_ERR(LOG_MODULE, "failed to configure slicing on %s device: "\
                     "check INSTALL for dependencies, or rerun "\
                     "using --no-slicing option to disable slicing",
                     netdev_name);
            netdev_close(netdev);
            return error;
        }
    }

    /* NOTE: port struct is already allocated in struct dp */
    memset(port, '\0', sizeof *port);

    port->dp = dp;

    port->conf = xmalloc(sizeof(struct ofl_port));
    port->conf->port_no    = port_no;
    memcpy(port->conf->hw_addr, netdev_get_etheraddr(netdev), ETH_ADDR_LEN);
    port->conf->name       = strcpy(xmalloc(strlen(netdev_name) + 1), netdev_name);
    port->conf->config     = 0x00000000;
    port->conf->state      = 0x00000000 | OFPPS_LIVE;
    port->conf->curr       = netdev_get_features(netdev, NETDEV_FEAT_CURRENT);
    port->conf->advertised = netdev_get_features(netdev, NETDEV_FEAT_ADVERTISED);
    port->conf->supported  = netdev_get_features(netdev, NETDEV_FEAT_SUPPORTED);
    port->conf->peer       = netdev_get_features(netdev, NETDEV_FEAT_PEER);
    port->conf->curr_speed = port_speed(port->conf->curr);
    port->conf->max_speed  = port_speed(port->conf->supported);

    //+++Modificaciones UAH+++//
    // Se configura el bit OFPPC_NO_FWD para que no se use el puerto local con OFPP_FLOOD
    if (port_no == OFPP_LOCAL)
    {
        port->conf->config |= OFPPC_NO_FWD;
    }
    //++++++//

    if (IS_HW_PORT(p)) {
#if defined(OF_HW_PLAT) && !defined(USE_NETDEV)
        of_hw_driver_t *hw_drv;

        hw_drv = p->dp->hw_drv;
        free(port->conf->name);
        port->conf->name = strcpy(xmalloc(strlen(p->hw_name) + 1), p->hw_name);
        / * Update local port state * /
        if (hw_drv->port_link_get(hw_drv, port_no)) {
            p->state &= ~OFPPS_LINK_DOWN;
        } else {
            p->state |= OFPPS_LINK_DOWN;
        }
        if (hw_drv->port_enable_get(hw_drv, port_no)) {
            p->config &= ~OFPPC_PORT_DOWN;
        } else {
            p->config |= OFPPC_PORT_DOWN;
        }
        / * FIXME:  Add current, supported and advertised features * /
#endif
    }
    dp_port_live_update(port);

    port->stats = xmalloc(sizeof(struct ofl_port_stats));
    port->stats->port_no = port_no;
    port->stats->rx_packets   = 0;
    port->stats->tx_packets   = 0;
    port->stats->rx_bytes     = 0;
    port->stats->tx_bytes     = 0;
    port->stats->rx_dropped   = 0;
    port->stats->tx_dropped   = 0;
    port->stats->rx_errors    = 0;
    port->stats->tx_errors    = 0;
    port->stats->rx_frame_err = 0;
    port->stats->rx_over_err  = 0;
    port->stats->rx_crc_err   = 0;
    port->stats->collisions   = 0;
    port->stats->duration_sec = 0;
    port->stats->duration_nsec = 0;
    port->flags |= SWP_USED;
    port->netdev = netdev;
    port->max_queues = max_queues;
    port->num_queues = 0;
    port->created = now;

    memset(port->queues, 0x00, sizeof(port->queues));

    list_push_back(&dp->port_list, &port->node);
    dp->ports_num++;

    {
    /* Notify the controllers that this port has been added */
    struct ofl_msg_port_status msg =
            {{.type = OFPT_PORT_STATUS},
             .reason = OFPPR_ADD, .desc = port->conf};

        dp_send_message(dp, (struct ofl_msg_header *)&msg, NULL/*sender*/);
    }

    return 0;
}


#if defined(OF_HW_PLAT) && !defined(USE_NETDEV)
int
dp_ports_add(struct datapath *dp, const char *port_name)
{
    int port_no;
    int rc = 0;
    struct sw_port *port;

    fprintf(stderr, "Adding port %s. hw_drv is %p\n", port_name, dp->hw_drv);
    if (dp->hw_drv && dp->hw_drv->port_add) {
        port_no = dp->hw_drv->port_add(dp->hw_drv, -1, port_name);
        if (port_no >= 0) {
            port = &dp->ports[port_no];
            if (port->flags & SWP_USED) {
                VLOG_ERR(LOG_MODULE, "HW port %s (%d) already created\n",
                          port_name, port_no);
                rc = -1;
            } else {
                fprintf(stderr, "Adding HW port %s as OF port number %d\n",
                       port_name, port_no);
                /* FIXME: Determine and record HW addr, etc */
                port->flags |= SWP_USED | SWP_HW_DRV_PORT;
                port->dp = dp;
                port->port_no = port_no;
                list_init(&port->queue_list);
                port->max_queues = max_queues;
                port->num_queues = 0;
                strncpy(port->hw_name, port_name, sizeof(port->hw_name));
                list_push_back(&dp->port_list, &port->node);

                struct ofl_msg_port_status msg =
                        {{.type = OFPT_PORT_STATUS},
                         .reason = OFPPR_ADD, .desc = p->conf};

                dp_send_message(dp, (struct ofl_msg_header *)&msg, NULL);

            }
        } else {
            VLOG_ERR(LOG_MODULE, "Port %s not recognized by hardware driver", port_name);
            rc = -1;
        }
    } else {
        VLOG_ERR(LOG_MODULE, "No hardware driver support; can't add ports");
        rc = -1;
    }

    return rc;
}
#else /* Not HW platform support */

int
dp_ports_add(struct datapath *dp, const char *netdev)
{
    uint32_t port_no;
    for (port_no = 1; port_no < DP_MAX_PORTS; port_no++) {
        struct sw_port *port = &dp->ports[port_no];
        if (port->netdev == NULL) {
            return new_port(dp, port, port_no, netdev, NULL, dp->max_queues);
        }
    }
    return EXFULL;
}
#endif /* OF_HW_PLAT */

int
dp_ports_add_local(struct datapath *dp, const char *netdev)
{
    if (!dp->local_port) {
        uint8_t ea[ETH_ADDR_LEN];
        struct sw_port *port;
        int error;
        //modificacion uah
        uint8_t mac[ETH_ADDR_LEN];
        /*+++FIN+++*/

        port = xcalloc(1, sizeof *port);
        eth_addr_from_uint64(dp->id, ea);
        error = new_port(dp, port, OFPP_LOCAL, netdev, ea, 0);
        VLOG_WARN(LOG_MODULE, "[dp_ports_add_local]: ERROR -> %d", error);
        if (!error) {
            dp->local_port = port;
            /*Modificación UAH*/
            time_init_local_port = time_msec();
            eth_addr_from_uint64(dp->id, mac);
            //se envia el dato arriba para ver que todo ha ido bien
            error = send_new_localport_packet_UAH(dp, port->conf->port_no, port->conf->name, mac, old_local_port, &time_init_local_port); 
            //*+++FIN+++*/
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: Mensaje enviado con codigo de ERROR -> %d", error);
        } else {
            free(port);
        }
        return error;
    } else {
        return EXFULL;
    }
}


struct sw_port *
dp_ports_lookup(struct datapath *dp, uint32_t port_no) {

    // exclude local port from ports_num
    uint32_t ports_num = dp->local_port ? dp->ports_num -1 : dp->ports_num;

    if (port_no == OFPP_LOCAL) {
        return dp->local_port;
    }
    /* Local port already checked, so dp->ports -1 */
    if (port_no < 1 || port_no > ports_num) {
        return NULL;
    }

    return &dp->ports[port_no];
}

struct sw_queue *
dp_ports_lookup_queue(struct sw_port *p, uint32_t queue_id)
{
    struct sw_queue *q;

    if (queue_id < p->max_queues) {
        q = &(p->queues[queue_id]);

        if (q->port != NULL) {
            return q;
        }
    }

    return NULL;
}

void
dp_ports_output(struct datapath *dp, struct ofpbuf *buffer, uint32_t out_port,
              uint32_t queue_id)
{
    uint16_t class_id;
    struct sw_queue * q;
    struct sw_port *p;

    p = dp_ports_lookup(dp, out_port);

    /* FIXME:  Needs update for queuing */
    #if defined(OF_HW_PLAT) && !defined(USE_NETDEV)
    if ((p != NULL) && IS_HW_PORT(p)) {
        if (dp && dp->hw_drv) {
            if (dp->hw_drv->port_link_get(dp->hw_drv, p->port_no)) {
                of_packet_t *pkt;
                int rv;

                pkt = calloc(1, sizeof(*pkt));
                OF_PKT_INIT(pkt, buffer);
                rv = dp->hw_drv->packet_send(dp->hw_drv, out_port, pkt, 0);
                if ((rv < 0) && (rv != OF_HW_PORT_DOWN)) {
                    VLOG_ERR(LOG_MODULE, "Error %d sending pkt on HW port %d\n",
                             rv, out_port);
                    ofpbuf_delete(buffer);
                    free(pkt);
                }
            }
        }
        return;
    }

    /* Fall through to software controlled ports if not HW port */
#endif
    if (p != NULL && p->netdev != NULL) {
        if (!(p->conf->config & OFPPC_PORT_DOWN)) {
            /* avoid the queue lookup for best-effort traffic */
            if (queue_id == 0) {
                q = NULL;
                class_id = 0;
            }
            else {
                /* silently drop the packet if queue doesn't exist */
                q = dp_ports_lookup_queue(p, queue_id);
                if (q != NULL) {
                    class_id = q->class_id;
                }
                else {
                    goto error;
                }
            }

            if (!netdev_send(p->netdev, buffer, class_id)) {
                p->stats->tx_packets++;
                p->stats->tx_bytes += buffer->size;
                if (q != NULL) {
                    q->stats->tx_packets++;
                    q->stats->tx_bytes += buffer->size;
                }
            } else {
                p->stats->tx_dropped++;
            }
        }
        /* NOTE: no need to delete buffer, it is deleted along with the packet in caller. */
        return;
    }

 error:
     /* NOTE: no need to delete buffer, it is deleted along with the packet. */
    VLOG_DBG_RL(LOG_MODULE, &rl, "can't forward to bad port:queue(%d:%d)\n", out_port,
                queue_id);
}

int
dp_ports_output_all(struct datapath *dp, struct ofpbuf *buffer, int in_port, bool flood)
{
    struct sw_port *p;

    LIST_FOR_EACH (p, struct sw_port, node, &dp->port_list) {
         /*Modificación UAH*/
        // if (p->stats->port_no == in_port){
        if (p->conf->port_no == in_port) {
            continue;
        }
        // Se comparan los nombres de los puertos porque el del puerto local y otro puerto físico comparten la misma interfaz)
        if (in_port == OFPP_LOCAL && !strcmp(p->conf->name, dp->local_port->conf->name)) 
        {
            continue;
        }
        /*FIN Modificación UAH*/
        if (flood && p->conf->config & OFPPC_NO_FWD) {
            continue;
        }
        dp_ports_output(dp, buffer, p->stats->port_no, 0);
    }

    return 0;
}

ofl_err
dp_ports_handle_port_mod(struct datapath *dp, struct ofl_msg_port_mod *msg,
                                                const struct sender *sender) {

    struct sw_port *p;
    struct ofl_msg_port_status rep_msg;

    if(sender->remote->role == OFPCR_ROLE_SLAVE)
        return ofl_error(OFPET_BAD_REQUEST, OFPBRC_IS_SLAVE);

    p = dp_ports_lookup(dp, msg->port_no);

    if (p == NULL) {
        return ofl_error(OFPET_PORT_MOD_FAILED,OFPPMFC_BAD_PORT);
    }

    /* Make sure the port id hasn't changed since this was sent */
    if (memcmp(msg->hw_addr, netdev_get_etheraddr(p->netdev),
                     ETH_ADDR_LEN) != 0) {
        return ofl_error(OFPET_PORT_MOD_FAILED, OFPPMFC_BAD_HW_ADDR);
    }


    if (msg->mask) {
        p->conf->config &= ~msg->mask;
        p->conf->config |= msg->config & msg->mask;
        dp_port_live_update(p);
    }

    /*Notify all controllers that the port status has changed*/
    rep_msg.header.type = OFPT_PORT_STATUS;
    rep_msg.reason =   OFPPR_MODIFY;
    rep_msg.desc = p->conf;      
    dp_send_message(dp, (struct ofl_msg_header *)&rep_msg, NULL/*sender*/);
    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
    return 0;
}

ofl_err
dp_ports_handle_port_mod_UAH(struct datapath *dp, uint32_t port_no) {

    struct sw_port *p;
    struct ofl_msg_port_status rep_msg;


    p = dp_ports_lookup(dp, port_no);

    if (p == NULL) {
        VLOG_INFO(LOG_MODULE, "Puerto no encontrado");
        return ofl_error(OFPET_PORT_MOD_FAILED, OFPPMFC_BAD_PORT);
    }

    /*Notify all controllers that the port status has changed*/
    rep_msg.header.type = OFPT_PORT_STATUS;
    rep_msg.reason =   OFPPR_MODIFY;
    rep_msg.desc = p->conf;      
    dp_send_message(dp, (struct ofl_msg_header *)&rep_msg, NULL/*sender*/);
    return 0;
}

ofl_err send_new_localport_packet_UAH(struct datapath *dp, uint32_t new_local_port, char *port_name, uint8_t *mac, uint32_t old_local_port, 
    uint64_t * time_start_process)
{
       
    struct ofl_msg_packet_in msg;
    struct packet *pkt;

    pkt = create_ehddp_new_localport_packet_UAH(dp, new_local_port, port_name, mac, &old_local_port, time_start_process);

    // VLOG_WARN(LOG_MODULE, "Función DP ACTION OUTPUT PORT case OFPP_CONTROLLER.");
    msg.header.type = OFPT_PACKET_IN;
    msg.total_len = pkt->buffer->size;
    msg.reason = OFPR_ACTION;
    msg.table_id = pkt->table_id;
    msg.data = pkt->buffer->data;
    msg.cookie = 0xffffffffffffffff;
    msg.buffer_id = OFP_NO_BUFFER;
    msg.data_length = pkt->buffer->size;

    if (!pkt->handle_std->valid)
    {
        packet_handle_std_validate(pkt->handle_std);
    }
    /* In this implementation the fields in_port and in_phy_port
        always will be the same, because we are not considering logical
        ports*/
    msg.match = (struct ofl_match_header *)&pkt->handle_std->match;
    dp_send_message(pkt->dp, (struct ofl_msg_header *)&msg, NULL);
    return 0;
}

static void
dp_port_stats_update(struct sw_port *port) {
    port->stats->duration_sec  =  (time_msec() - port->created) / 1000;
    port->stats->duration_nsec = ((time_msec() - port->created) % 1000) * 1000000;
}

void
dp_port_live_update(struct sw_port *p) {

  if((p->conf->state & OFPPS_LINK_DOWN)
     || (p->conf->config & OFPPC_PORT_DOWN)) {
      /* Port not live */
      p->conf->state &= ~OFPPS_LIVE;
  } else {
      /* Port is live */
      p->conf->state |= OFPPS_LIVE;
  }
}

ofl_err
dp_ports_handle_stats_request_port(struct datapath *dp,
                                  struct ofl_msg_multipart_request_port *msg,
                                  const struct sender *sender UNUSED) {
    struct sw_port *port;

    struct ofl_msg_multipart_reply_port reply =
            {{{.type = OFPT_MULTIPART_REPLY},
              .type = OFPMP_PORT_STATS, .flags = 0x0000},
             .stats_num   = 0,
             .stats       = NULL};

    if (msg->port_no == OFPP_ANY) {
        size_t i = 0;

        reply.stats_num = dp->ports_num;
        reply.stats     = xmalloc(sizeof(struct ofl_port_stats *) * dp->ports_num);

        LIST_FOR_EACH(port, struct sw_port, node, &dp->port_list) {
            dp_port_stats_update(port);
            reply.stats[i] = port->stats;
            i++;
        }

    } else {
        port = dp_ports_lookup(dp, msg->port_no);

        if (port != NULL && port->netdev != NULL) {
            reply.stats_num = 1;
            reply.stats = xmalloc(sizeof(struct ofl_port_stats *));
            dp_port_stats_update(port);
            reply.stats[0] = port->stats;
        }
    }

    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    free(reply.stats);
    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);

    return 0;
}

ofl_err
dp_ports_handle_port_desc_request(struct datapath *dp,
                                  struct ofl_msg_multipart_request_header *msg UNUSED,
                                  const struct sender *sender UNUSED){
    struct sw_port *port;
    size_t i = 0;


    struct ofl_msg_multipart_reply_port_desc reply =
            {{{.type = OFPT_MULTIPART_REPLY},
             .type = OFPMP_PORT_DESC, .flags = 0x0000},
             .stats_num   = 0,
             .stats       = NULL};

    reply.stats_num = dp->ports_num;
    reply.stats     = xmalloc(sizeof(struct ofl_port *) * dp->ports_num);

    LIST_FOR_EACH(port, struct sw_port, node, &dp->port_list) {
        reply.stats[i] = port->conf;
        i++;
    }

    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    free(reply.stats);
    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);

    return 0;
}

static void
dp_ports_queue_update(struct sw_queue *queue) {
    queue->stats->duration_sec  =  (time_msec() - queue->created) / 1000;
    queue->stats->duration_nsec = ((time_msec() - queue->created) % 1000) * 1000000;
}

ofl_err
dp_ports_handle_stats_request_queue(struct datapath *dp,
                                  struct ofl_msg_multipart_request_queue *msg,
                                  const struct sender *sender) {
    struct sw_port *port;

    struct ofl_msg_multipart_reply_queue reply =
            {{{.type = OFPT_MULTIPART_REPLY},
              .type = OFPMP_QUEUE, .flags = 0x0000},
             .stats_num   = 0,
             .stats       = NULL};

    if (msg->port_no == OFPP_ANY) {
        size_t i,idx = 0, num = 0;

        LIST_FOR_EACH(port, struct sw_port, node, &dp->port_list) {
            if (msg->queue_id == OFPQ_ALL) {
                num += port->num_queues;
            } else {
                if (msg->queue_id < port->max_queues) {
                    if (port->queues[msg->queue_id].port != NULL) {
                        num++;
                    }
                }
            }
        }

        reply.stats_num = num;
        reply.stats     = xmalloc(sizeof(struct ofl_port_stats *) * num);

        LIST_FOR_EACH(port, struct sw_port, node, &dp->port_list) {
            if (msg->queue_id == OFPQ_ALL) {
                for(i=0; i<port->max_queues; i++) {
                    if (port->queues[i].port != NULL) {
                        dp_ports_queue_update(&port->queues[i]);
                        reply.stats[idx] = port->queues[i].stats;
                        idx++;
                    }
                }
            } else {
                if (msg->queue_id < port->max_queues) {
                    if (port->queues[msg->queue_id].port != NULL) {
                        dp_ports_queue_update(&port->queues[msg->queue_id]);
                        reply.stats[idx] = port->queues[msg->queue_id].stats;
                        idx++;
                    }
                }
            }
        }

    } else {
        port = dp_ports_lookup(dp, msg->port_no);

        if (port != NULL && port->netdev != NULL) {
            size_t i, idx = 0;

            if (msg->queue_id == OFPQ_ALL) {
                reply.stats_num = port->num_queues;
                reply.stats = xmalloc(sizeof(struct ofl_port_stats *) * port->num_queues);

                for(i=0; i<port->max_queues; i++) {
                    if (port->queues[i].port != NULL) {
                        dp_ports_queue_update(&port->queues[i]);
                        reply.stats[idx] = port->queues[i].stats;
                        idx++;
                    }
                }
            } else {
                if (msg->queue_id < port->max_queues) {
                    if (port->queues[msg->queue_id].port != NULL) {
                        reply.stats_num = 1;
                        reply.stats = xmalloc(sizeof(struct ofl_port_stats *));
                        dp_ports_queue_update(&port->queues[msg->queue_id]);
                        reply.stats[0] = port->queues[msg->queue_id].stats;
                    }
                }
            }
        }
    }

    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    free(reply.stats);
    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);

    return 0;
}

ofl_err
dp_ports_handle_queue_get_config_request(struct datapath *dp,
                              struct ofl_msg_queue_get_config_request *msg,
                                                const struct sender *sender) {
    struct sw_port *p;

    struct ofl_msg_queue_get_config_reply reply =
            {{.type = OFPT_QUEUE_GET_CONFIG_REPLY},
             .queues = NULL};

    if (msg->port == OFPP_ANY) {
        size_t i, idx = 0, num = 0;

        LIST_FOR_EACH(p, struct sw_port, node, &dp->port_list) {
            num += p->num_queues;
        }

        reply.port       = OFPP_ANY;
        reply.queues_num = num;
        reply.queues     = xmalloc(sizeof(struct ofl_packet_queue *) * num);

        LIST_FOR_EACH(p, struct sw_port, node, &dp->port_list) {
            for (i=0; i<p->max_queues; i++) {
                if (p->queues[i].port != NULL) {
                    reply.queues[idx] = p->queues[i].props;
                    idx++;
                }
             }
         }
    } else {
        p = dp_ports_lookup(dp, msg->port);

        if (p == NULL || (p->stats->port_no != msg->port)) {
            free(reply.queues);
            ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
            return ofl_error(OFPET_QUEUE_OP_FAILED, OFPQOFC_BAD_PORT);
        } else {
            size_t i, idx = 0;

            reply.port       = msg->port;
            reply.queues_num = p->num_queues;
            reply.queues     = xmalloc(sizeof(struct ofl_packet_queue *) * p->num_queues);

            for (i=0; i<p->max_queues; i++) {
                if (p->queues[i].port != NULL) {
                    reply.queues[idx] = p->queues[i].props;
                    idx++;
                }
            }
        }
    }

    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    free(reply.queues);
    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
    return 0;
}

/*
 * Queue handling
 */

static int
new_queue(struct sw_port * port, struct sw_queue * queue,
          uint32_t queue_id, uint16_t class_id,
          struct ofl_queue_prop_min_rate * mr)
{
    uint64_t now = time_msec();

    memset(queue, '\0', sizeof *queue);
    queue->port = port;
    queue->created = now;
    queue->stats = xmalloc(sizeof(struct ofl_queue_stats));

    queue->stats->port_no = port->stats->port_no;
    queue->stats->queue_id = queue_id;
    queue->stats->tx_bytes = 0;
    queue->stats->tx_packets = 0;
    queue->stats->tx_errors = 0;
    queue->stats->duration_sec = 0;
    queue->stats->duration_nsec = 0;

    /* class_id is the internal mapping to class. It is the offset
     * in the array of queues for each port. Note that class_id is
     * local to port, so we don't have any conflict.
     * tc uses 16-bit class_id, so we cannot use the queue_id
     * field */
    queue->class_id = class_id;

    queue->props = xmalloc(sizeof(struct ofl_packet_queue));
    queue->props->queue_id = queue_id;
    queue->props->properties = xmalloc(sizeof(struct ofl_queue_prop_header *));
    queue->props->properties_num = 1;
    queue->props->properties[0] = xmalloc(sizeof(struct ofl_queue_prop_min_rate));
    ((struct ofl_queue_prop_min_rate *)(queue->props->properties[0]))->header.type = OFPQT_MIN_RATE;
    ((struct ofl_queue_prop_min_rate *)(queue->props->properties[0]))->rate = mr->rate;

    port->num_queues++;
    return 0;
}

static int
port_add_queue(struct sw_port *p, uint32_t queue_id,
               struct ofl_queue_prop_min_rate * mr)
{
    if (queue_id >= p->max_queues) {
        return EXFULL;
    }

    if (p->queues[queue_id].port != NULL) {
        return EXFULL;
    }

    return new_queue(p, &(p->queues[queue_id]), queue_id, queue_id, mr);
}

static int
port_delete_queue(struct sw_port *p, struct sw_queue *q)
{
    memset(q,'\0', sizeof *q);
    p->num_queues--;
    return 0;
}

ofl_err
dp_ports_handle_queue_modify(struct datapath *dp, struct ofl_exp_openflow_msg_queue *msg,
        const struct sender *sender UNUSED) {
    // NOTE: assumes the packet queue has exactly one property, for min rate
    struct sw_port *p;
    struct sw_queue *q;

    int error = 0;

    p = dp_ports_lookup(dp, msg->port_id);
    if (PORT_IN_USE(p)) {
        q = dp_ports_lookup_queue(p, msg->queue->queue_id);
        if (q != NULL) {
            /* queue exists - modify it */
            error = netdev_change_class(p->netdev,q->class_id,
                                 ((struct ofl_queue_prop_min_rate *)msg->queue->properties[0])->rate);
             if (error) {
                 VLOG_ERR(LOG_MODULE, "Failed to update queue %d", msg->queue->queue_id);
                 return ofl_error(OFPET_QUEUE_OP_FAILED, OFPQOFC_EPERM);
             }
             else {
                 ((struct ofl_queue_prop_min_rate *)q->props->properties[0])->rate =
                         ((struct ofl_queue_prop_min_rate *)msg->queue->properties[0])->rate;
             }

        } else {
            /* create new queue */
            error = port_add_queue(p, msg->queue->queue_id,
                                       (struct ofl_queue_prop_min_rate *)msg->queue->properties[0]);
            if (error == EXFULL) {
                return ofl_error(OFPET_QUEUE_OP_FAILED, OFPQOFC_EPERM);
            }

            q = dp_ports_lookup_queue(p, msg->queue->queue_id);
                error = netdev_setup_class(p->netdev,q->class_id,
                                ((struct ofl_queue_prop_min_rate *)msg->queue->properties[0])->rate);
                if (error) {
                    VLOG_ERR(LOG_MODULE, "Failed to configure queue %d", msg->queue->queue_id);
                    return ofl_error(OFPET_QUEUE_OP_FAILED, OFPQOFC_BAD_QUEUE);
                }
        }

    } else {
        VLOG_ERR(LOG_MODULE, "Failed to create/modify queue - port %d doesn't exist", msg->port_id);
        return ofl_error(OFPET_QUEUE_OP_FAILED, OFPQOFC_BAD_PORT);
    }

    if (IS_HW_PORT(p)) {
#if defined(OF_HW_PLAT) && !defined(USE_NETDEV)
        error = dp->hw_drv->port_queue_config(dp->hw_drv, port_no,
                                              queue_id, ntohs(mr->rate));
        if (error < 0) {
            VLOG_ERR(LOG_MODULE, "Failed to update HW port %d queue %d",
                     port_no, queue_id);
        }
#endif
    }

    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
    return 0;
}


ofl_err
dp_ports_handle_queue_delete(struct datapath *dp, struct ofl_exp_openflow_msg_queue *msg,
                                                  const struct sender *sender UNUSED) {
    struct sw_port *p;
    struct sw_queue *q;

    p = dp_ports_lookup(dp, msg->port_id);
    if (p != NULL && p->netdev != NULL) {
        q = dp_ports_lookup_queue(p, msg->queue->queue_id);
        if (q != NULL) {
            netdev_delete_class(p->netdev,q->class_id);
            port_delete_queue(p, q);

            ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
            return 0;
        } else {
            return ofl_error(OFPET_QUEUE_OP_FAILED, OFPQOFC_BAD_QUEUE);
        }
    }

    return ofl_error(OFPET_QUEUE_OP_FAILED, OFPQOFC_BAD_PORT);
}

/*Modificacion UAH Discovery hybrid topologies, JAH-*/

uint16_t bigtolittle16(uint16_t num){
    return (num>>8) | (num<<8);
}

uint32_t bigtolittle32(uint32_t num){
    return  ((num>>24)&0xff) | // move byte 3 to byte 0
            ((num<<8)&0xff0000) | // move byte 1 to byte 2
            ((num>>8)&0xff00) | // move byte 2 to byte 1
            ((num<<24)&0xff000000); // byte 0 to byte 3
}

uint64_t bigtolittle64(uint64_t num){
    return  ((num >> 56) & 0x00000000000000FF) | 
            ((num >> 40) & 0x000000000000FF00) | 
            ((num >> 24) & 0x0000000000FF0000) | 
            ((num >>  8) & 0x00000000FF000000) | 
            ((num <<  8) & 0x000000FF00000000) | 
            ((num << 24) & 0x0000FF0000000000) | 
            ((num << 40) & 0x00FF000000000000) | 
            ((num << 56) & 0xFF00000000000000);

}

uint64_t mac2int(const uint8_t hwaddr[])
{
    int8_t i;
    uint64_t ret = 0;
    const uint8_t *p = hwaddr;

    for (i = 5; i >= 0; i--) {
        ret |= (uint64_t) *p++ << (CHAR_BIT * i);
    }

    return ret;
}

void int2mac(const uint64_t mac, uint8_t *hwaddr)
{
    int8_t i;
    uint8_t *p = hwaddr;

    for (i = 5; i >= 0; i--) {
        *p++ = mac >> (CHAR_BIT * i);
    }
}

void mac_to_port_new(struct mac_to_port *mac_port)
{
    mac_port->inicio = NULL;
    mac_port->fin = NULL;
    mac_port->num_element = 0;
}

int mac_to_port_add(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN], uint16_t port_in, int time, uint64_t num_sec)
{
    struct mac_port_time *nuevo_elemento = NULL;
    struct mac_port_time *actual = mac_port->fin;

    if ((nuevo_elemento = xmalloc (sizeof (struct mac_port_time))) == NULL)
        return -1;

    nuevo_elemento->port_in = port_in;
    //guardamos el momento en que la entrada deja de ser valida
    nuevo_elemento->valid_time_entry = time_msec() + (time * 0.8);
    memcpy(nuevo_elemento->Mac, Mac, ETH_ADDR_LEN);
    nuevo_elemento->nuevo_puerto = true;
    nuevo_elemento->num_sec = num_sec;
    nuevo_elemento->next = NULL;

    if(mac_port->inicio == NULL)
        mac_port->inicio = nuevo_elemento;
    else
        actual->next = nuevo_elemento;

    mac_port->fin = nuevo_elemento;
    mac_port->num_element++;
    return 0;
}

int mac_to_port_check_timeout(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN])
{
	struct mac_port_time *aux = mac_port->inicio;
	uint64_t marca_tiempo_msec = time_msec();
	while(aux != NULL)
	{
		if (memcmp(aux->Mac, Mac, ETH_ADDR_LEN) == 0)
		{
			if (marca_tiempo_msec > aux->valid_time_entry) 
				return 1; 
			else
				return 0; 
			}
		aux = aux->next;
	}
	return 2; 
}

//update element
int mac_to_port_update(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN], uint16_t port_in, int time, uint64_t num_sec) 
{
    struct mac_port_time *aux = mac_port->inicio;
    
    if (mac_port->num_element > 0){
        while(aux != NULL)
        {
            if(memcmp(aux->Mac, Mac, ETH_ADDR_LEN) == 0)
            {
                /*Indicamos si hay cambio de puerto*/
                if (aux->port_in != port_in)
                    aux->nuevo_puerto = true; 
                else
                    aux->nuevo_puerto = false; 
                aux->port_in = port_in;
                aux->num_sec = num_sec;
                //miramos cual si el tiempo guardado + la actualizacion
                if (time_msec() + (time * 0.8) >= aux->valid_time_entry)
                    // le metemos el tiempo correspondiente
                    aux->valid_time_entry = time_msec() + (time * 0.8);
                //todo correcto
                return 0; 
            }
            aux = aux->next; //pasamos al siguiente elemento de la lista
        }
    }
    return 1;//no se encontro la mac
}

int mac_to_port_time_refresh(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN], uint64_t time, uint64_t num_sec) //update element
{
    struct mac_port_time *aux = mac_port->inicio;
   
    if (mac_port->num_element > 0){
        while(aux != NULL)
        {
            if(memcmp(aux->Mac, Mac, ETH_ADDR_LEN) == 0)
            {
                //miramos cual si el tiempo guardado + la actualizacion
                aux->nuevo_puerto = false;
                aux->num_sec = num_sec;
                if (time_msec() + (time * 0.8) > aux->valid_time_entry)
                    aux->valid_time_entry = time_msec() + (time * 0.8); // le metemos el tiempo correspondiente
                return 0; //todo correcto
            }
            aux = aux->next; //pasamos al siguiente elemento de la lista
        }
    }
    return -1;//no se encontro la mac
}

int mac_to_port_found_port(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN], uint64_t num_sec)
//chequemos si existe una mac y devolvemos un puerto
{
    struct mac_port_time *aux = mac_port->inicio;
    uint64_t marca_tiempo_msec = time_msec();
    int port_select = -1;
   
    if (mac_port->num_element > 0){
        while(aux != NULL)
        {
            if( (memcmp(aux->Mac, Mac, ETH_ADDR_LEN) == 0))
            {
                if (marca_tiempo_msec <= aux->valid_time_entry && aux->num_sec == num_sec)
                    //todo correcto
                    port_select = aux->port_in; 
                else 
                    port_select = 0; //puerto 0 -> puerto encontrado pero caducado
                break;
            }
            //actual pasa a ser el siguiente
            aux = aux->next;
        }
    }

    return port_select; //si no existe tal puerto
}

int mac_to_port_found_mac_position(struct mac_to_port *mac_port, uint64_t position, uint8_t * Mac){
    struct mac_port_time *aux = mac_port->inicio;
    uint64_t marca_tiempo_msec = time_msec();
    int pos = 0;

    if (mac_port->num_element > 0){
        while(aux != NULL)
        {
            if (pos == position-1) {
                if (marca_tiempo_msec <= aux->valid_time_entry){
                    VLOG_INFO(LOG_MODULE, "MAC encontrada en posicion localizada devolvemos 1");
                    memcpy(Mac, aux->Mac, ETH_ADDR_LEN);
                    return pos;
                }
                else 
                {
                    VLOG_INFO(LOG_MODULE, "MAC encontrada pero caducada devolvemos 0");
                    return 0; //puerto 0 -> puerto encontrado pero caducado
                }
            }
            //actual pasa a ser el siguiente
            aux = aux->next;
            pos++;
        }
    }
    VLOG_INFO(LOG_MODULE, "MAC NO encontrada devolvemos -1");
    return -1; //si no existe tal puerto
}

int mac_to_port_delete_timeout(struct mac_to_port *mac_port)
{
    struct mac_port_time *anterior = mac_port->inicio;
    struct mac_port_time *actual = mac_port->inicio;
    uint64_t marca_tiempo_msec = time_msec();
    bool inicial = false;
    
    //Buscamos si tenemos elementos que buscar   
    while (actual != NULL && mac_port->num_element > 0)
    {
        if (marca_tiempo_msec >= actual->valid_time_entry )
        {
            if(actual == mac_port->inicio && actual == mac_port->fin){
                mac_port->inicio = NULL;
                mac_port->fin = NULL;
                mac_port->num_element = 0;
            }
            else if(actual == mac_port->inicio)
            {
                mac_port->inicio = actual->next;
                anterior = mac_port->inicio;
                inicial = true;
            }
            else if(actual == mac_port->fin)
            {
                anterior->next = NULL;
                mac_port->fin = anterior;
            }
            else{
                anterior->next = actual->next;
            }
            if (actual != NULL){
                free(actual);
            }
            if (mac_port->num_element > 0 ){
                mac_port->num_element--;
            }  
        }
        else
        {
            anterior = actual;
        }
        if (anterior != NULL)
        {
            if (inicial == true)
            {
                actual = mac_port->inicio;
            }
            else{
                actual = anterior->next;
            }
            inicial = false;
        }
        else
            actual = NULL;
        
    }
    return 0;
}

/*Debemos guardar todos los elementos de un mismo controlador por tanto cuando el controlador
este caducado borramos todo*/
int mac_to_port_delete_timeout_ehddp(struct mac_to_port *mac_port)
{
    struct mac_port_time *anterior = mac_port->inicio;
    struct mac_port_time *actual = mac_port->inicio;
    uint64_t marca_tiempo_msec = time_msec();
    uint8_t limpiar_tabla = 0;
    bool inicial = false;
    
    //Buscamos si tenemos elementos que buscar
    
    //comprobamos si el controlador ha caducado y en tal caso borramos todo
    if (mac_port->num_element > 0){
        if (marca_tiempo_msec > actual->valid_time_entry )
        {
            limpiar_tabla = 1;
        }
    }

    while (actual != NULL && mac_port->num_element > 0)
    {
        if (limpiar_tabla == 1)
        {
            if(actual == mac_port->inicio && actual == mac_port->fin){
                mac_port->inicio = NULL;
                mac_port->fin = NULL;
                mac_port->num_element = 0;
            }
            else if(actual == mac_port->inicio)
            {
                mac_port->inicio = actual->next;
                anterior = mac_port->inicio;
                inicial = true;
            }
            else if(actual == mac_port->fin)
            {
                anterior->next = NULL;
                mac_port->fin = anterior;
            }
            else{
                anterior->next = actual->next;
            }
            if (actual != NULL){
                free(actual);
            }
            if (mac_port->num_element > 0 ){
                mac_port->num_element--;
            }  
        }
        else
        {
            anterior = anterior->next;
        }
        if (anterior != NULL)
        {
            if (inicial == true)
            {
                actual = anterior;
            }
            else{
                actual = anterior->next;
            }
            inicial = false;
        }
        else
            actual = NULL;
        
    }
    return 0;
}

int mac_to_port_delete_position(struct mac_to_port *mac_port, int position)
{
    struct mac_port_time *anterior = mac_port->inicio;
    struct mac_port_time *actual = mac_port->inicio;
    int pos_act = 1;
    
    //Buscamos si tenemos elementos que buscar
    while (actual != NULL && mac_port->num_element > 0)
    {
        if (pos_act == position)
        {
            if(actual == mac_port->inicio && actual == mac_port->fin){
                mac_port->inicio = NULL;
                mac_port->fin = NULL;
                mac_port->num_element = 0;
            }
            else if(actual == mac_port->inicio)
            {
                mac_port->inicio = actual->next;
                anterior = mac_port->inicio;
            }
            else if(actual == mac_port->fin)
            {
                anterior->next = NULL;
                mac_port->fin = anterior;
            }
            else{
                anterior->next = actual->next;
            }
            if (actual != NULL){
                free(actual);
            }
            if (mac_port->num_element > 0 ){
                mac_port->num_element--;
            }  
        }
        else
        {
            pos_act++;
            anterior = anterior->next;
        }
        if (anterior != NULL)
            actual = anterior->next;
        else
            actual = NULL;
    }
    return 0;
}

int num_port_available(struct datapath * dp){
    int num_port_inactivos = 0, i = 0;

    for (i = 1; i<(int)dp->ports_num; i++){
        if (dp->ports[i].conf->state & OFPPS_LINK_DOWN)
            num_port_inactivos++;
        //else
        //    VLOG_INFO(LOG_MODULE, "El puerto %d esta activo(OFPPS_LINK_DOWN == %lu): %lu", 
        //        i,(long unsigned int)OFPPS_LINK_DOWN, (long unsigned int)dp->ports[i].conf->state);
    } 
    /** Le restamos uno ya que el puerto del controller tampoco cuenta con habilitado */
    return ((int)dp->ports_num - num_port_inactivos - 1);

}

void visualizar_tabla(struct mac_to_port *mac_port, int64_t id_datapath)
{
	char mac_tabla[5000];
	struct mac_port_time *aux = mac_port->inicio;
	int i=0,j=0;

	sprintf(mac_tabla, "DpID: %d \npos|      Mac        |Puerto IN|Time\n", (int)id_datapath);
	sprintf(mac_tabla + strlen(mac_tabla),"----------------------------------------------\n");
	while(aux != NULL)
	{
		sprintf(mac_tabla + strlen(mac_tabla)," %d |",i+1);
		//pasamos mac_port->fila[i]->Mac a algo legible
		if(aux->Mac != NULL)
		{
			//pasamos mac a que sea legible
			sprintf(mac_tabla + strlen(mac_tabla), "%x:", aux->Mac[0]);
			for(j=1; j<6;j++)
			{
				if(aux->Mac[j])
					sprintf(mac_tabla + strlen(mac_tabla),"%x",aux->Mac[j]);
				if(j!=5)
					sprintf(mac_tabla + strlen(mac_tabla),":");
			}
			//pasamos puerto para ser legible
			sprintf(mac_tabla + strlen(mac_tabla), "| %d |",aux->port_in);

			//pasamos tiempo para ser legible
			//if (aux->valid_time_entry > time_msec())
			sprintf(mac_tabla + strlen(mac_tabla),"%.3f \n",((float)(aux->valid_time_entry - time_msec()))/1000);
		}
		i++;
		aux = aux->next;
	}
	sprintf(mac_tabla + strlen(mac_tabla),"\n");
	VLOG_INFO(LOG_MODULE, "%s\n", mac_tabla);
}

void log_uah(const void *Mensaje, int64_t id)
{

	FILE * file;
	char nombre[90], nombre2[90];

	VLOG_DBG_RL(LOG_MODULE, &rl, "Traza UAH -> Entro a Crear Log");
	sprintf(nombre,"/tmp/DHT_switch_%d.log",(int)id);
	
	file=fopen(nombre,"a");
	if(file != NULL)
	{
		fseek(file, 0L, SEEK_END);
		if(ftell(file) > 16000)
		{
			fclose(file);
			sprintf(nombre2,"/tmp/ehddp_switch_%d-%lu.log",(int)id,(long)time_msec());
			rename(nombre,nombre2);
		}
		file = fopen( nombre , "a" );
		fputs(Mensaje, file);
		fclose(file);
	}
	else
		VLOG_DBG_RL(LOG_MODULE, &rl, "Traza UAH -> Archivo no abierto");
}

struct in_addr remove_local_port_UAH(struct datapath *dp)
{
    int error;
    struct in_addr ip_0 = {INADDR_ANY}, ip_if;                                //Para poner a 0 la ip de la interfaz a eliminar
    
    netdev_get_in4(dp->local_port->netdev, &ip_if);                           //Se obtiene la ip de la interfaz  
    netdev_set_in4(dp->local_port->netdev, ip_0, ip_0);                       //Se configura la ip a 0
    error = netdev_set_etheraddr(dp->local_port->netdev, old_local_port_MAC); //Se asigna la antigua MAC
    if (error)
    {
        VLOG_WARN(LOG_MODULE, "failed to change %s Ethernet address to " ETH_ADDR_FMT ": %s",
                  dp->local_port->conf->name, ETH_ADDR_ARGS(old_local_port_MAC), strerror(error));
    }

    list_pop_back(&dp->port_list); //Se elimina el último puerto (puerto_local) de la lista

    free(dp->local_port->conf);
    free(dp->local_port->stats);
    free(dp->local_port); //Se libera la memoria del peurto local

    dp->ports_num--; //Se decrementa el número de puertos
    dp->local_port = NULL;

    if (ip_if.s_addr == INADDR_ANY)                                           //Si la IP es 0.0.0.0 es decir no hay
        ip_if.s_addr = ip_de_control_in_band.s_addr;                                     //le asignamos la definida en la configuración incial
        
    return ip_if;
}

int configure_new_local_port_ehddp_UAH(struct datapath *dp, uint8_t *mac, uint32_t old_local_port, uint64_t time_start_process)
{
    int error;
    struct sw_port *p;
    struct mac_port_time *aux = bt_table.inicio;
    char ip_aux[INET_ADDRSTRLEN];
    struct in_addr mask, local_ip = ip_de_control_in_band;
    if  (aux != NULL)
    {
        p = dp_ports_lookup(dp, aux->port_in);
        if (p == NULL)
        {
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: Software port NO ENCONTRADO %d!!", aux->port_in);
            return 0;
        }
        error = dp_ports_add_local(dp, p->conf->name);
        if (!error){           
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: Antiguo puerto local (%u) | Nuevo puerto local %s (%u)", old_local_port, 
                p->conf->name, p->conf->port_no);

            //inet_ntop(AF_INET, &local_ip.s_addr, ip_aux, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &local_ip.s_addr, ip_aux, INET_ADDRSTRLEN);
            inet_pton(AF_INET, "255.255.255.0", &(mask.s_addr));
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: IP de la interfaz %s (mask: %s)", ip_aux, "255.255.255.0");
            netdev_set_in4(dp->local_port->netdev, local_ip, mask); //Se configura la ip del nuevo puerto local
            if (!netdev_get_in4(dp->local_port->netdev, &local_ip)){
                VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: No se ha configurado correctamente el nuevo puero local!!");
                inet_ntop(AF_INET, &local_ip.s_addr, ip_aux, INET_ADDRSTRLEN);
                VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: IP de la interfaz %s", ip_aux);
                return 0;
            }
            
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: Enviamos informacion al OFPROTOCOL para que cambie el puerto local!!");
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: Port = %d | name = %s | time_start_process = %lu", p->conf->port_no, p->conf->name, time_start_process);
            error = send_new_localport_packet_UAH(dp, p->conf->port_no, p->conf->name, mac, old_local_port, &time_start_process); 
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: Mensaje enviado con codigo de ERROR -> %d", error);
        }
        else
        {
            VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: ADD local port FAIL!!!");
            return 0;
        }
        return p->conf->port_no;
    }
    else
    {
        VLOG_WARN(LOG_MODULE, "[CONFIGURE NEW LOCAL PORT]: No se puede configurar ningun puerto local ya que no conocemos como llegar al controller!!!");
        return -1;
    }
    
}

uint32_t get_matching_if_port_number_UAH(struct datapath *dp, char *netdev_name)
{
    struct sw_port *p;
    LIST_FOR_EACH(p, struct sw_port, node, &dp->port_list)
    {
        if (!strcmp(p->conf->name, netdev_name))
        {
            return p->conf->port_no;
        }
    }
    return 0;
}

/*Fin Modificacion UAH eHDDP, JAH-*/
