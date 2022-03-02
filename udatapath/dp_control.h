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

#ifndef DP_CONTROL_H
#define DP_CONTROL_H 1

#include "datapath.h"
#include "oflib/ofl.h"
#include "oflib/ofl-messages.h"


/****************************************************************************
 * Datapath control channel related functions.
 ****************************************************************************/

/*Modificación UAH*/
extern uint8_t conection_status_ofp_controller;
extern uint64_t time_connect_to_contoller;
extern uint64_t time_exploration;
extern uint64_t convergence_time;
extern uint64_t time_start;
extern uint8_t log_escrito;
extern struct in_addr ip_if;
extern uint32_t old_local_port;
extern struct mac_to_port bt_table, learning_table;
extern bool Reply_ON;
extern int num_pkt_ehddp_req;
extern int num_pkt_ehddp_rep;
extern int num_pkt_arp_rep;
extern int num_pkt_arp_req;
extern bool controller_connected;

/*Fin modificación UAH*/

struct sender;

/* Dispatches or handles the incoming OpenFlow messages. */
ofl_err
handle_control_msg(struct datapath *dp, struct ofl_msg_header *msg,
                   const struct sender *sender);

/*modificacion UAH*/
void mod_local_port_change_connection_uah(struct datapath * dp);
void log_uah_estadisticos(struct datapath * dp);

/*Fin modificacion UAH*/


#endif /* DP_CONTROL_H */
