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

#ifndef PACKET_H
#define PACKET_H 1

#include <stdbool.h>
#include "action_set.h"
#include "datapath.h"
#include "packet_handle_std.h"
#include "ofpbuf.h"
#include "oflib/ofl-structs.h"
#include "packets.h"


/****************************************************************************
 * Represents a packet received on the datapath, and its associated processing
 * state.
 ****************************************************************************/


struct packet {
    struct datapath    *dp;
    struct ofpbuf      *buffer;    /* buffer containing the packet */
    uint32_t            in_port;
    struct action_set  *action_set; /* action set associated with the packet */
    bool                packet_out; /* true if the packet arrived in a packet out msg */

    uint32_t            out_group; /* OFPG_ANY = no out group */
    uint32_t            out_port;  /* OFPP_ANY = no out port */
    uint16_t            out_port_max_len;  /* max length to send, if out_port is OFPP_CONTROLLER */
    uint32_t            out_queue;
    uint8_t             table_id; /* table in which is processed */
    uint32_t            buffer_id; /* if packet is stored in buffer, buffer_id;
                                      otherwise 0xffffffff */

    struct packet_handle_std  *handle_std; /* handler for standard match structure */
};

/* Creates a packet. */
struct packet *
packet_create(struct datapath *dp, uint32_t in_port, struct ofpbuf *buf, bool packet_out);

/* Converts the packet to a string representation. */
char *
packet_to_string(struct packet *pkt);

/* Destroys a packet along with all its associated structures */
void
packet_destroy(struct packet *pkt);

/* Clones a packet deeply, i.e. all associated structures are also cloned. */
struct packet *
packet_clone(struct packet *pkt);

/*Modificacion UAH Discovery hybrid topologies, JAH-*/

//Tipos de dispositivos 
#define NODO_SDN 1
#define NODO_NO_SDN 2
#define NODO_SENSOR 3
extern uint16_t type_sensor;

//funciones para crear paquete de Discovery hybrid topologies
struct packet * create_ehddp_reply_packet(struct datapath *dp, uint8_t * mac_dst,
    uint32_t in_port, uint32_t out_port, uint16_t type_device, uint64_t mac_device_64, uint8_t num_devices,
    u_int64_t num_sec, u_int64_t num_ack, u_int32_t time_block);

//funcion para actualizar los paquetes
uint16_t update_data_msg(struct packet * pkt, uint32_t out_port,  uint8_t * nxt_mac);
/*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/

#endif /* PACKET_H */
