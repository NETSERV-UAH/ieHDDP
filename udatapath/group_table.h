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

#ifndef GROUP_TABLE_H
#define GROUP_TABLE_H 1

#include "datapath.h"
#include "group_entry.h"
#include "oflib/ofl.h"
#include "oflib/ofl-messages.h"
#include "packet.h"


/****************************************************************************
 * Implementation of group tables.
 ****************************************************************************/


#define GROUP_TABLE_MAX_ENTRIES 4096
#define GROUP_TABLE_MAX_BUCKETS 8192

struct datapath;
struct packet;
struct sender;

struct group_table {
    struct datapath  *dp;
	struct ofl_msg_multipart_reply_group_features *features;   
	size_t            entries_num;
    struct hmap       entries;
    size_t            buckets_num;
};


/* Handles a group_mod message. */
ofl_err
group_table_handle_group_mod(struct group_table *table, struct ofl_msg_group_mod *mod, const struct sender *sender);

/* Handles a group stats request message. */
ofl_err
group_table_handle_stats_request_group(struct group_table *table,
                                  struct ofl_msg_multipart_request_group *msg,
                                  const struct sender *sender);

/* Handles a group desc stats request message */
ofl_err
group_table_handle_stats_request_group_desc(struct group_table *table,
        struct ofl_msg_multipart_request_header *msg,
        const struct sender *sender);

/* Handles a group features stats request message */
ofl_err
group_table_handle_stats_request_group_features(struct group_table *table,
                                  struct ofl_msg_multipart_request_header *msg UNUSED,
                                  const struct sender *sender);

/* Returns the group entry with the given ID. */
struct group_entry *
group_table_find(struct group_table *table, uint32_t group_id);

/* Executes the given group entry on the packet. */
void
group_table_execute(struct group_table *table, struct packet *packet, uint32_t group_id);

/* Creates a group table. */
struct group_table *
group_table_create(struct datapath *dp);

/* Destroys a group table. */
void
group_table_destroy(struct group_table *table);


#endif /* GROUP_TABLE_H */

