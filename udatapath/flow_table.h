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

#ifndef FLOW_TABLE_H
#define FLOW_TABLE_H 1
#include "oflib/ofl.h"
#include "oflib/ofl-messages.h"
#include "oflib/ofl-structs.h"
#include "pipeline.h"
#include "timeval.h"


#define FLOW_TABLE_MAX_ENTRIES 4096
#define TABLE_FEATURES_NUM 14

/****************************************************************************
 * Implementation of a flow table. The current implementation stores flow
 * entries in priority and then insertion order.
 ****************************************************************************/


struct flow_table {
    struct datapath           *dp;
    bool                       disabled;      /* Don't use that table. */
    struct ofl_table_features *features;      /*store table features*/
    struct ofl_table_stats    *stats;         /* structure storing table statistics. */
    
    struct list               match_entries;  /* list of entries in order. */
    struct list               hard_entries;   /* list of entries with hard timeout;
                                                ordered by their timeout times. */
    struct list               idle_entries;   /* unordered list of entries with
                                                idle timeout. */
};

extern uint32_t oxm_ids[];

extern uint32_t wildcarded[]; 

extern struct ofl_instruction_header instructions[];

extern struct ofl_action_header actions[];
/* Handles a flow mod message. */
ofl_err
flow_table_flow_mod(struct flow_table *table, struct ofl_msg_flow_mod *mod, bool *match_kept, bool *insts_kept);

/* Finds the flow entry with the highest priority, which matches the packet. */
struct flow_entry *
flow_table_lookup(struct flow_table *table, struct packet *pkt);

/* Orders the flow table to check the timeout its flows. */
void
flow_table_timeout(struct flow_table *table);

/* Creates a flow table. */
struct flow_table *
flow_table_create(struct datapath *dp, uint8_t table_id);

/* Destroys a flow table. */
void
flow_table_destroy(struct flow_table *table);

/* Collects statistics of the flow entries of the table. */
void
flow_table_stats(struct flow_table *table, struct ofl_msg_multipart_request_flow *msg,
                 struct ofl_flow_stats ***stats, size_t *stats_size, size_t *stats_num);

/* Collects aggregate statistics of the flow entries of the table. */
void
flow_table_aggregate_stats(struct flow_table *table, struct ofl_msg_multipart_request_flow *msg,
                           uint64_t *packet_count, uint64_t *byte_count, uint32_t *flow_count);

#endif /* FLOW_TABLE_H */
