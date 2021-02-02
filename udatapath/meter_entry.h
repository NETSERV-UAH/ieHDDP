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

#ifndef METER_ENTRY_H
#define METER_ENTRY_H 1

#include <stdbool.h>
#include "hmap.h"
#include "list.h"
#include "packet.h"
#include "oflib/ofl-structs.h"
#include "oflib/ofl-messages.h"
#include "meter_table.h"



/****************************************************************************
 * Implementation of a meter entry.
 ****************************************************************************/


/* Structures from others */
struct packet;
struct datapath;
struct flow_entry;
struct sender;

/* Meter entry */
struct meter_entry {
	struct hmap_node            node;			/* Refered by the meter table */

	struct datapath				*dp;			/* The datapath */
	struct meter_table			*table;			/* The meter table */

	struct ofl_meter_stats		*stats;			/* Meter statistics */
	struct ofl_meter_config		*config;		/* Meter configuration */

    uint64_t                    created;  /* time the entry was created at. */
    	
	struct list                 flow_refs;		/* references to flows referencing the meter. */

};

/* Creates a meter entry. */
struct meter_entry *
meter_entry_create(struct datapath *dp, struct meter_table *table, struct ofl_msg_meter_mod *mod);

/*Update counters */
void
meter_entry_update(struct meter_entry *entry);

/* Destroys a meter entry. */
void
meter_entry_destroy(struct meter_entry *entry);

/* Apply the meter entry on the packet. */
void
meter_entry_apply(struct meter_entry *entry, struct packet **pkt);


/* Adds a flow reference to the meter entry. */
void
meter_entry_add_flow_ref(struct meter_entry *entry, struct flow_entry *fe);

/* Removes a flow reference from the meter entry. */
void
meter_entry_del_flow_ref(struct meter_entry *entry, struct flow_entry *fe);

void
refill_bucket(struct meter_entry *entry);

#endif /* METER_ENTRY_H */
