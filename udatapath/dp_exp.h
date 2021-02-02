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

#ifndef DP_EXP_H
#define DP_EXP_H 1

#include "packet.h"
#include "oflib/ofl-messages.h"

struct datapath;
struct sender;

/****************************************************************************
 * Datapath stubs for handling experimenter features.
 ****************************************************************************/

/* Handles experimenter actions. */
void
dp_exp_action(struct packet *pkt, struct ofl_action_experimenter *act);

/* Handles experimenter instructions. */
void
dp_exp_inst(struct packet *pkt, struct ofl_instruction_experimenter *inst);

/* Handles experimenter statistics. */
ofl_err
dp_exp_stats(struct datapath *dp, struct ofl_msg_multipart_request_experimenter *msg,
             const struct sender *sender);

/* Handles experimenter messages. */
ofl_err
dp_exp_message(struct datapath *dp,
                           struct ofl_msg_experimenter *msg,
                           const struct sender *sender);


#endif /* DP_EXP_H */
