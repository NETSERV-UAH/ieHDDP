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

#ifndef ACTION_SET_H
#define ACTION_SET_H 1

#include <sys/types.h>
#include <stdio.h>
#include "datapath.h"
#include "packet.h"
#include "oflib/ofl.h"
#include "oflib/ofl-actions.h"
#include "oflib/ofl-structs.h"

struct action_set;
struct datapath;
struct packet;


/****************************************************************************
 * Implementation of an action set associated with a datapath packet
 ****************************************************************************/

struct action_set *
action_set_create(struct ofl_exp *exp);

/* Destroys an action set */
void
action_set_destroy(struct action_set *set);

/* Creates a clone of an action set. Used when cloning a datapath packet in
 * groups. */
struct action_set *
action_set_clone(struct action_set *set);

/* Writes the set of given actions to the set, overwriting existing types as
 * defined by the 1.1 spec. */
void
action_set_write_actions(struct action_set *set,
                         size_t actions_num,
                         struct ofl_action_header **actions);


/* Clears the actions from the set. */
void
action_set_clear_actions(struct action_set *set);

/* Executes the actions in the set on the packet. Packet is the owner of the
 * action set right now, but this might be changed in the future. */
void
action_set_execute(struct action_set *set, struct packet *pkt, uint64_t cookie);

/* Converts the action set to a string representation. */
char *
action_set_to_string(struct action_set *set);

/* Converts the action set to a string representation and appends it to the
 * given string. */
void
action_set_print(FILE *stream, struct action_set *set);

#endif /* ACTION_SET */
