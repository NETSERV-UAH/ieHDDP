/* 
 * This file is part of the HDDP Switch distribution (https://github.com/gistnetserv-uah/eHDDP).
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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include "datapath.h"
#include "dp_buffers.h"
#include "dp_actions.h"
#include "packet.h"
#include "packets.h"
#include "action_set.h"
#include "ofpbuf.h"
#include "oflib/ofl-structs.h"
#include "oflib/ofl-print.h"
#include "util.h"


struct packet *
packet_create(struct datapath *dp, uint32_t in_port,
    struct ofpbuf *buf, bool packet_out) {
    struct packet *pkt;

    pkt = xmalloc(sizeof(struct packet));

    pkt->dp         = dp;
    pkt->buffer     = buf;
    pkt->in_port    = in_port;
    pkt->action_set = action_set_create(dp->exp);

    pkt->packet_out       = packet_out;
    pkt->out_group        = OFPG_ANY;
    pkt->out_port         = OFPP_ANY;
    pkt->out_port_max_len = 0;
    pkt->out_queue        = 0;
    pkt->buffer_id        = NO_BUFFER;
    pkt->table_id         = 0;

    pkt->handle_std = packet_handle_std_create(pkt);
    return pkt;
}

struct packet *
packet_clone(struct packet *pkt) {
    struct packet *clone;

    clone = xmalloc(sizeof(struct packet));
    clone->dp         = pkt->dp;
    clone->buffer     = ofpbuf_clone(pkt->buffer);
    clone->in_port    = pkt->in_port;
    /* There is no case we need to keep the action-set, but if it's needed
     * we could add a parameter to the function... Jean II
     * clone->action_set = action_set_clone(pkt->action_set);
     */
    clone->action_set = action_set_create(pkt->dp->exp);


    clone->packet_out       = pkt->packet_out;
    clone->out_group        = OFPG_ANY;
    clone->out_port         = OFPP_ANY;
    clone->out_port_max_len = 0;
    clone->out_queue        = 0;
    clone->buffer_id        = NO_BUFFER; // the original is saved in buffer,
                                         // but this buffer is a copy of that,
                                         // and might be altered later
    clone->table_id         = pkt->table_id;

    clone->handle_std = packet_handle_std_clone(clone, pkt->handle_std);

    return clone;
}

void
packet_destroy(struct packet *pkt) {
    /* If packet is saved in a buffer, do not destroy it,
     * if buffer is still valid */
     
    if (pkt->buffer_id != NO_BUFFER) {
        if (dp_buffers_is_alive(pkt->dp->buffers, pkt->buffer_id)) {
            return;
        } else {
            dp_buffers_discard(pkt->dp->buffers, pkt->buffer_id, false);
        }
    }

    action_set_destroy(pkt->action_set);
    ofpbuf_delete(pkt->buffer);
    packet_handle_std_destroy(pkt->handle_std);
    free(pkt);
}

char *
packet_to_string(struct packet *pkt) {
    char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);

    fprintf(stream, "pkt{in=\"");
    ofl_port_print(stream, pkt->in_port);
    fprintf(stream, "\", actset=");
    action_set_print(stream, pkt->action_set);
    fprintf(stream, ", pktout=\"%u\", ogrp=\"", pkt->packet_out);
    ofl_group_print(stream, pkt->out_group);
    fprintf(stream, "\", oprt=\"");
    ofl_port_print(stream, pkt->out_port);
    fprintf(stream, "\", buffer=\"");
    ofl_buffer_print(stream, pkt->buffer_id);
    fprintf(stream, "\", std=");
    packet_handle_std_print(stream, pkt->handle_std);
    fprintf(stream, "}");

    fclose(stream);
    return str;
}

/*Modificacion UAH Discovery hybrid topologies, JAH-*/
struct packet * create_ehddp_reply_packet(struct datapath *dp, uint8_t * mac_dst,
    uint32_t in_port, uint32_t out_port, uint16_t type_device, uint64_t mac_device_64, uint8_t num_devices,
    u_int64_t num_sec, u_int64_t num_ack, u_int32_t time_block)
{
    /*                                 Estructura del paquete
     * ----------------------------------------------------------------------------------
     * | ETH HEADER | FLAGS AND VERSION 1B | OPTION CODE 1B | NUM DEVICE 1B | NUM SECUENCE 64B |
     * | PREVIOUS MAC LEN 1B | PREVIOUS MAC (SIZE PREVIOS MAC)B | ACK NUMBER 64B | 
     * | MAC_ORIGEN (SIZE PREVIOS MAC)B | LAST_MODIFIED (SIZE PREVIOS MAC)B | time_block 32B |
     * | struct = [CONFIGURATION [X|K|M] 1B | TYPE DEVICE XB | MAC DEVICE KB | IN_PORT MB | OUT_PORT MB] | 
     * ---------------------------------------------------------------------------------- 
    */

    struct packet *pkt = NULL;
    struct ofpbuf *buffer2 = NULL;
    
    uint8_t opcode = 0x02, flag_and_ersion = 0x01, num_device = num_devices, previous_size_mac = 0x06, configurations = 0b01111111;
    uint16_t etherType = bigtolittle16(ETH_TYPE_EHDDP), type_devices = htons(type_device);
    uint8_t * device_mac = NULL;
    uint32_t out_ports = htonl(out_port), in_ports = htonl(in_port);
 
    device_mac = malloc(sizeof(uint8_t)*ETH_ADDR_LEN);
    int2mac(mac_device_64, device_mac);
    mac_device_64 = bigtolittle64(mac_device_64);
    time_block = time_block;

    //Now, create the packet and add the Ethernet header
    buffer2= ofpbuf_new( sizeof(struct eth_header) + sizeof(struct ehddp_header));
    ofpbuf_put(buffer2, mac_dst, ETH_ADDR_LEN); 
    ofpbuf_put(buffer2, dp->ports[1].conf->hw_addr, ETH_ADDR_LEN);
    ofpbuf_put(buffer2, &etherType, sizeof(uint16_t));
    
    //Now, create the HDP header
    ofpbuf_put(buffer2,&flag_and_ersion, sizeof(uint8_t));
    ofpbuf_put(buffer2,&opcode, sizeof(uint8_t));
    ofpbuf_put(buffer2,&num_device, sizeof(uint8_t));
    ofpbuf_put(buffer2,&num_sec, sizeof(uint64_t));
    ofpbuf_put(buffer2,&previous_size_mac, sizeof(uint8_t));
    ofpbuf_put(buffer2,mac_dst, sizeof(uint8_t)*ETH_ADDR_LEN);
    ofpbuf_put(buffer2,&num_ack, sizeof(uint64_t));
    ofpbuf_put(buffer2,device_mac, sizeof(uint8_t)*ETH_ADDR_LEN);
    ofpbuf_put(buffer2,device_mac, sizeof(uint8_t)*ETH_ADDR_LEN);
    ofpbuf_put(buffer2,&time_block, sizeof(uint32_t));

    ofpbuf_put(buffer2,&configurations, sizeof(uint8_t));
    ofpbuf_put(buffer2,&type_devices, sizeof(uint16_t));
    ofpbuf_put(buffer2,&mac_device_64, sizeof(uint64_t));
    ofpbuf_put(buffer2,&in_ports, sizeof(uint32_t));
    ofpbuf_put(buffer2,&out_ports, sizeof(uint32_t));

    //Creamos la estructura del paquete
    pkt = packet_create(dp, in_port, buffer2, false);

    //Creamos la cabecera ethernet
    pkt->handle_std->proto->eth = xmalloc(sizeof(struct eth_header));
    memcpy(pkt->handle_std->proto->eth->eth_dst, mac_dst, ETH_ADDR_LEN);
    memcpy(pkt->handle_std->proto->eth->eth_src, dp->ports[1].conf->hw_addr, ETH_ADDR_LEN);
    //Insertamos el eth al reves por tema de little endian
    pkt->handle_std->proto->eth->eth_type=ETH_TYPE_EHDDP_INV;

    //creamos la cabecera de nuestro protocolo
    pkt->handle_std->proto->ehddp=xmalloc(sizeof(struct ehddp_header));
    pkt->handle_std->proto->ehddp->flags = flag_and_ersion;
    pkt->handle_std->proto->ehddp->opcode = opcode;
    pkt->handle_std->proto->ehddp->num_devices = num_device;
    pkt->handle_std->proto->ehddp->previous_size_mac = previous_size_mac;
    memcpy(&pkt->handle_std->proto->ehddp->nxt_mac, mac_dst, sizeof(uint8_t)*ETH_ADDR_LEN);
    pkt->handle_std->proto->ehddp->num_sec = num_sec;
    pkt->handle_std->proto->ehddp->num_ack = num_ack;
    memcpy(&pkt->handle_std->proto->ehddp->last_mac, device_mac, sizeof(uint8_t)*ETH_ADDR_LEN);
    memcpy(&pkt->handle_std->proto->ehddp->src_mac, device_mac, sizeof(uint8_t)*ETH_ADDR_LEN);
    pkt->handle_std->proto->ehddp->time_block = time_block;

    pkt->handle_std->proto->ehddp->configurations[0] = configurations;
    pkt->handle_std->proto->ehddp->type_devices[0] = type_devices;
    pkt->handle_std->proto->ehddp->ids[0] = mac_device_64;
    pkt->handle_std->proto->ehddp->in_ports[0] = in_ports;
    pkt->handle_std->proto->ehddp->out_ports[0] = out_ports;
   
    //validamos el paquete
    packet_handle_std_validate(pkt->handle_std);
    return pkt;
}

uint16_t update_data_msg(struct packet * pkt, uint32_t out_port,  uint8_t * nxt_mac){
    uint8_t configuration = 0b01111111;
    uint16_t type_device = 0, num_elements=pkt->handle_std->proto->ehddp->num_devices + 1;
    uint32_t in_port = htonl(pkt->in_port), out_port_update=0;
    uint64_t mac_64 = bigtolittle64(mac2int(pkt->dp->ports[1].conf->hw_addr));

    if (pkt->dp->id >= 0x1000 ){ /** Soy un sensor indico que sensor soy */
        type_device = htons(type_sensor);
    }
    else /** Como no soy un sensor, soy un NO SDN */
    {
        type_device = htons(NODO_NO_SDN); 
    } 

    if (num_elements > EHDDP_MAX_ELEMENTS)
        return num_elements;
    if (out_port > 255)
        out_port = 255;

    out_port_update = htonl(out_port);

    //Modificamos El buffer primero y luego mi trabla
        /*Introducimos el valor del campo en el paquete */
    ofpbuf_put(pkt->buffer,&configuration, sizeof(uint8_t));
    ofpbuf_put(pkt->buffer,&type_device, sizeof(uint16_t));
    ofpbuf_put(pkt->buffer,&mac_64, sizeof(uint64_t));
    ofpbuf_put(pkt->buffer,&in_port, sizeof(uint32_t));
    ofpbuf_put(pkt->buffer,&out_port_update, sizeof(uint32_t));

    //Puerto de entrada sentido SRC->DST
    if (pkt->handle_std->proto->ehddp->opcode == 2)
        memcpy(&pkt->handle_std->proto->eth->eth_dst, nxt_mac,sizeof(uint8_t)*ETH_ADDR_LEN);

    pkt->handle_std->proto->ehddp->num_devices = num_elements;
    pkt->handle_std->proto->ehddp->previous_size_mac = 0x06; //estamos en un switch ethernet
    memcpy(&pkt->handle_std->proto->ehddp->last_mac, 
        pkt->dp->ports[1].conf->hw_addr,sizeof(uint8_t)*ETH_ADDR_LEN); //decimos que hemos sido nosotros los ultimos en modificar
    memcpy(&pkt->handle_std->proto->ehddp->nxt_mac, nxt_mac,sizeof(uint8_t)*ETH_ADDR_LEN); 

    /*pkt->handle_std->proto->ehddp->configurations[num_elements-1]=configuration;
    pkt->handle_std->proto->ehddp->type_devices[num_elements-1]=htons(type_device);
    pkt->handle_std->proto->ehddp->ids[num_elements-1]=mac2int(pkt->dp->ports[1].conf->hw_addr);
    pkt->handle_std->proto->ehddp->in_ports[num_elements-1]=pkt->in_port;
    pkt->handle_std->proto->ehddp->out_ports[num_elements-1]=out_port;*/

    pkt->packet_out=false;
    pkt->handle_std->valid = false;

    packet_handle_std_validate(pkt->handle_std);
    return 0;
}

/*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/
