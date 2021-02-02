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

#ifndef DP_ACTIONS_H
#define DP_ACTIONS_H 1

#include <sys/types.h>
#include "datapath.h"
#include "packet.h"
#include "oflib/ofl-actions.h"


/****************************************************************************
 * Datapath action implementations.
 ****************************************************************************/

/* Executes the action on the given packet. */
void
dp_execute_action(struct packet *pkt,
                  struct ofl_action_header *action);


/* Executes the list of action on the given packet. */
void
dp_execute_action_list(struct packet *pkt,
                size_t actions_num, struct ofl_action_header **actions, uint64_t cookie);

/* Outputs the packet on the given port and queue. */
void
dp_actions_output_port(struct packet *pkt, uint32_t out_port, uint32_t out_queue, uint16_t max_len, uint64_t cookie);

/* Returns true if the given list of actions has an output action to the port. */
bool
dp_actions_list_has_out_port(size_t actions_num, struct ofl_action_header **actions, uint32_t port);

/* Returns true if the given list of actions has an group action to the group. */
bool
dp_actions_list_has_out_group(size_t actions_num, struct ofl_action_header **actions, uint32_t group);

/* Validates the set of actions based on the available ports and groups. Returns an OpenFlow
 * error if the actions are invalid. */
ofl_err
dp_actions_validate(struct datapath *dp, size_t actions_num, struct ofl_action_header **actions);

/* Validates the set of set_field actions, checking if the pre requisites are present in the match. Returns and Openlow
 * error if the actions are invalid. */
ofl_err
dp_actions_check_set_field_req(struct ofl_msg_flow_mod *msg, size_t actions_num, struct ofl_action_header **actions);

/* Modificacion UAH */
size_t size_data_to_read(uint8_t config,int type_read);

/*fin modificacion UAH*/

#endif /* DP_ACTIONS_H */
