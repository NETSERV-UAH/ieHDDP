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

#ifndef PIPELINE_H
#define PIPELINE_H 1


#include "datapath.h"
#include "packet.h"
#include "flow_table.h"
#include "oflib/ofl.h"
#include "oflib/ofl-messages.h"


struct sender;

/****************************************************************************
 * A pipeline implementation. Processes messages through flow tables,
 * including the execution of instructions.
 ****************************************************************************/

/*Modificacion UAH Discovery hybrid topologies, JAH-*/
//vecinos globales para asi poder pasar y seleccionar envios

extern struct mac_to_port bt_table, learning_table;
extern bool local_port_ok;
extern uint8_t conection_status_ofp_controller;
extern struct in_addr ip_if;
extern uint32_t old_local_port;
extern struct in_addr ip_de_control_in_band;

//tiempo de vida del controlador en la tabla de controladores debe ser mas 
//pequeño que el tiempo entre refresco del controlador
#define BT_TIME 20
#define BT_TIME_PKT 1500
#define LT_TIME 1500

/*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/

/* A pipeline structure */
struct pipeline {
    struct datapath    *dp;
    struct flow_table  *tables[PIPELINE_TABLES];
};


/* Creates a pipeline. */
struct pipeline *
pipeline_create(struct datapath *dp);

/* Processes a packet in the pipeline. */
void
pipeline_process_packet(struct pipeline *pl, struct packet *pkt);

/* Handles a flow_mod message. */
ofl_err
pipeline_handle_flow_mod(struct pipeline *pl, struct ofl_msg_flow_mod *msg,
                         const struct sender *sender);

/* Handles a table_mod message. */
ofl_err
pipeline_handle_table_mod(struct pipeline *pl,
                          struct ofl_msg_table_mod *msg,
                          const struct sender *sender);

/* Handles a flow stats request. */
ofl_err
pipeline_handle_stats_request_flow(struct pipeline *pl,
                                   struct ofl_msg_multipart_request_flow *msg,
                                   const struct sender *sender);

/* Handles a table stats request. */
ofl_err
pipeline_handle_stats_request_table(struct pipeline *pl,
                                    struct ofl_msg_multipart_request_header *msg,
                                    const struct sender *sender);

/* Handles a table feature  request. */
ofl_err
pipeline_handle_stats_request_table_features_request(struct pipeline *pl,
                                    struct ofl_msg_multipart_request_header *msg,
                                    const struct sender *sender);

/* Handles an aggregate stats request. */
ofl_err
pipeline_handle_stats_request_aggregate(struct pipeline *pl,
                                  struct ofl_msg_multipart_request_flow *msg,
                                  const struct sender *sender);


/* Commands pipeline to check if any flow in any table is timed out. */
void
pipeline_timeout(struct pipeline *pl);

/* Detroys the pipeline. */
void
pipeline_destroy(struct pipeline *pl);

/*Modificacion UAH Discovery hybrid topologies, JAH-*/
/*learning function*/
int arp_path_send_unicast(struct packet * pkt, int out_port);
void pipeline_arp_path(struct packet *pkt);

//selector de paquetes especificos del protocolo
uint8_t select_ehddp_packets(struct packet *pkt, uint8_t resent_packet_ehddp);
//manejador de paquetes request del protocolo
uint8_t handle_ehddp_request_packets(struct packet *pkt, uint8_t resent_packet_ehddp);
//manejador de paquetes replay del protocolo
uint8_t handle_ehddp_reply_packets(struct packet *pkt);

//Generador de todos los paquetes que debemos enviar
void creator_ehddp_reply_packets(struct packet *pkt);

uint8_t ehddp_mod_local_port (struct packet * pkt);
/*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/

#endif /* PIPELINE_H */
