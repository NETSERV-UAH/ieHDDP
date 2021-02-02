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

#ifndef GROUP_entry_H
#define GROUP_entry_H 1

#include <stdbool.h>
#include "hmap.h"
#include "packet.h"
#include "group_table.h"
#include "oflib/ofl-structs.h"
#include "oflib/ofl-messages.h"


/****************************************************************************
 * Implementation of a group table entry.
 ****************************************************************************/


struct packet;
struct datapath;
struct flow_entry;

struct group_entry {
    struct hmap_node             node;

    struct datapath             *dp;
    struct group_table          *table;
    struct ofl_group_desc_stats *desc;
    struct ofl_group_stats      *stats;
    uint64_t created;
    void                        *data;     /* private data for group implementation. */

    struct list                  flow_refs; /* references to flows referencing the group. */
};

struct sender;
struct group_table;

/* Executes the group entry on the packet. */
void
group_entry_execute(struct group_entry *entry,
                          struct packet *packet);

/* Creates a group entry. */
struct group_entry *
group_entry_create(struct datapath *dp, struct group_table *table, struct ofl_msg_group_mod *mod);

/* Destroys a group entry. */
void
group_entry_destroy(struct group_entry *entry);

/* Returns true if the group entry has an group action to the given group ID. */
bool
group_entry_has_out_group(struct group_entry *entry, uint32_t group_id);

/* Adds a flow reference to the group entry. */
void
group_entry_add_flow_ref(struct group_entry *entry, struct flow_entry *fe);

/* Removes a flow reference from the group entry. */
void
group_entry_del_flow_ref(struct group_entry *entry, struct flow_entry *fe);

/* Updates the time fields of the group entry statistics. Used before generating
 * group statistics messages. */
void
group_entry_update(struct group_entry *entry);

#endif /* GROUP_entry_H */
