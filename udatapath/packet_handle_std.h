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

#ifndef PACKET_HANDLE_STD_H
#define PACKET_HANDLE_STD_H 1

#include <stdbool.h>
#include <stdio.h>
#include "packet.h"
#include "packets.h"
#include "match_std.h"
#include "oflib/ofl-structs.h"
#include "nbee_link/nbee_link.h"

/****************************************************************************
 * A handler processing a datapath packet for standard matches.
 ****************************************************************************/

/* The data associated with the handler */
struct packet_handle_std {
   struct packet              *pkt;
   struct protocols_std       *proto;
   struct ofl_match  match;  /* Match fields extracted from the packet
                                           are also stored in a match structure
                                           for convenience */
   bool                        valid; /* Set to true if the handler data is valid.
                                           if false, it is revalidated before
                                           executing any methods. */
   bool						   table_miss; /*Packet was matched
   											against table miss flow*/
};

/* Creates a handler */
struct packet_handle_std *
packet_handle_std_create(struct packet *pkt);

/* Destroys a handler */
void
packet_handle_std_destroy(struct packet_handle_std *handle);

/* Returns true if the TTL fields of the supported protocols are valid. */
bool
packet_handle_std_is_ttl_valid(struct packet_handle_std *handle);

/* Returns true if the packet is a fragment (IPv4). */
bool
packet_handle_std_is_fragment(struct packet_handle_std *handle);

/* Returns true if the packet matches the given standard match structure. */
bool
packet_handle_std_match(struct packet_handle_std *handle,  struct ofl_match *match);

/* Converts the packet to a string representation */
char *
packet_handle_std_to_string(struct packet_handle_std *handle);

void
packet_handle_std_print(FILE *stream, struct packet_handle_std *handle);

/* Clones the handler, and associates it with the new packet. */
struct packet_handle_std *
packet_handle_std_clone(struct packet *pkt, struct packet_handle_std *handle);

/* Revalidates the handler data */
void
packet_handle_std_validate(struct packet_handle_std *handle);


#endif /* PACKET_HANDLE_STD_H */
