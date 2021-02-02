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

#ifndef METER_TABLE_H
#define METER_TABLE_H 1

#include <stdbool.h>
#include "hmap.h"
#include "list.h"
#include "packet.h"
#include "oflib/ofl-structs.h"
#include "oflib/ofl-messages.h"
#include "meter_entry.h"

#define DEFAULT_MAX_METER 256
#define DEFAULT_MAX_BAND_PER_METER 16
#define DEFAULT_MAX_METER_COLOR 8
#define METER_TABLE_MAX_BANDS 1024


/****************************************************************************
 * Implementation of meter table.
 ****************************************************************************/

/* Meter table */
struct meter_table {
  struct datapath		*dp;				/* The datapath */
	struct ofl_meter_features *features;	
	size_t				 entries_num;		/* The number of meters */
  struct hmap			meter_entries;	    /* Meter entries */
	size_t              bands_num;

};


/* Creates a meter table. */
struct meter_table *
meter_table_create(struct datapath *dp);

/* Destroys a meter table. */
void
meter_table_destroy(struct meter_table *table);

/* Returns the meter with the given ID. */
struct meter_entry *
meter_table_find(struct meter_table *table, uint32_t meter_id);

/* Apply the given meter on the packet. */
void
meter_table_apply(struct meter_table *table, struct packet **packet, uint32_t meter_id);

/* Handles a meter_mod message. */
ofl_err
meter_table_handle_meter_mod(struct meter_table *table, struct ofl_msg_meter_mod  *mod, const struct sender *sender);


/* Handles a meter stats request message. */
ofl_err
meter_table_handle_stats_request_meter(struct meter_table *table,
                                  struct ofl_msg_multipart_meter_request *msg,
                                  const struct sender *sender UNUSED);

/* Handles a meter config request message. */
ofl_err
meter_table_handle_stats_request_meter_conf(struct meter_table *table,
                                  struct ofl_msg_multipart_meter_request *msg UNUSED,
                                  const struct sender *sender);

ofl_err
meter_table_handle_features_request(struct meter_table *table,
                                   struct ofl_msg_multipart_request_header *msg UNUSED,
                                  const struct sender *sender); 

void 
meter_table_add_tokens(struct meter_table *table);


#endif /* METER_TABLE_H */
