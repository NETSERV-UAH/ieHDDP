/* 
 * This file is part of the HDDP Switch distribution (https://github.com/gistnetserv-uah/eHDDP-inband).
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

#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>

#include "action_set.h"
#include "compiler.h"
#include "dp_actions.h"
#include "dp_buffers.h"
#include "dp_exp.h"
#include "dp_ports.h"
#include "datapath.h"
#include "packet.h"
#include "pipeline.h"
#include "flow_table.h"
#include "flow_entry.h"
#include "meter_table.h"
#include "oflib/ofl.h"
#include "oflib/ofl-structs.h"
#include "util.h"
#include "hash.h"
#include "oflib/oxm-match.h"
#include "vlog.h"


#define LOG_MODULE VLM_pipeline

static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(60, 60);

static void
execute_entry(struct pipeline *pl, struct flow_entry *entry,
              struct flow_table **table, struct packet **pkt);

struct pipeline *
pipeline_create(struct datapath *dp) {
    struct pipeline *pl;
    int i;
    pl = xmalloc(sizeof(struct pipeline));
    for (i=0; i<PIPELINE_TABLES; i++) {
        pl->tables[i] = flow_table_create(dp, i);
    }
    pl->dp = dp;
    nblink_initialize();
    return pl;
}

static bool
is_table_miss(struct flow_entry *entry){
    return ((entry->stats->priority) == 0 && (entry->match->length <= 4));
}

/* Sends a packet to the controller in a packet_in message */
static void
send_packet_to_controller(struct pipeline *pl, struct packet *pkt, uint8_t table_id, uint8_t reason) {

    struct ofl_msg_packet_in msg;
    struct ofl_match *m;
    msg.header.type = OFPT_PACKET_IN;
    msg.total_len   = pkt->buffer->size;
    msg.reason      = reason;
    msg.table_id    = table_id;
    msg.cookie      = 0xffffffffffffffff;
    msg.data = pkt->buffer->data;


    /* A max_len of OFPCML_NO_BUFFER means that the complete
        packet should be sent, and it should not be buffered.*/
    if (pl->dp->config.miss_send_len != OFPCML_NO_BUFFER){
        dp_buffers_save(pl->dp->buffers, pkt);
        msg.buffer_id   = pkt->buffer_id;
        msg.data_length = MIN(pl->dp->config.miss_send_len, pkt->buffer->size);
    }else {
        msg.buffer_id   = OFP_NO_BUFFER;
        msg.data_length = pkt->buffer->size;
    }

    m = &pkt->handle_std->match;
    /* In this implementation the fields in_port and in_phy_port
        always will be the same, because we are not considering logical
        ports                                 */
    msg.match = (struct ofl_match_header*)m;
    dp_send_message(pl->dp, (struct ofl_msg_header *)&msg, NULL);
}

/* Pass the packet through the flow tables.
 * This function takes ownership of the packet and will destroy it. */
void
pipeline_process_packet(struct pipeline *pl, struct packet *pkt) {
    struct flow_table *table, *next_table;

    uint8_t resent_packet_ehddp = 0;

    if (VLOG_IS_DBG_ENABLED(LOG_MODULE)) {
        char *pkt_str = packet_to_string(pkt);
        VLOG_DBG_RL(LOG_MODULE, &rl, "processing packet: %s", pkt_str);
        free(pkt_str);
    }

    if (!packet_handle_std_is_ttl_valid(pkt->handle_std)) {
        send_packet_to_controller(pl, pkt, 0/*table_id*/, OFPR_INVALID_TTL);
        packet_destroy(pkt);
        return;
    }

    /*Modificacion UAH Discovery hybrid topologies, JAH-*/
    if ((pkt->handle_std->proto->udp && eth_addr_is_multicast(pkt->handle_std->proto->eth->eth_dst)) || 
        (pkt->handle_std->proto->eth->eth_type == 56710))
    {
        packet_destroy(pkt);
        return ;
    }

    resent_packet_ehddp = ehddp_mod_local_port (pkt);
    
    /*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/

   	if ((pkt->handle_std->proto->eth->eth_type == ETH_TYPE_ARP || pkt->handle_std->proto->eth->eth_type == ETH_TYPE_ARP_INV)) 
	{
        VLOG_DBG(LOG_MODULE, "El paquete recibido es un paquete ARP.");
        if (htons(pkt->handle_std->proto->arp->ar_op) == 1 && pkt->handle_std->proto->arp->ar_tpa == ip_de_control_in_band.s_addr){
            /*Aumentamos el estadisitico de ARP Request*/
            num_pkt_arp_req++;
            pipeline_arp_path(pkt);
        }
        else if(htons(pkt->handle_std->proto->arp->ar_op) == 2 && pkt->handle_std->proto->arp->ar_spa == ip_de_control_in_band.s_addr){
            /*Aumentamos el estadisitico de ARP Replay*/
            num_pkt_arp_rep++;             
        }
    }
    
    next_table = pl->tables[0];
    while (next_table != NULL) {
        struct flow_entry *entry;

        VLOG_DBG_RL(LOG_MODULE, &rl, "trying table %u.", next_table->stats->table_id);

        pkt->table_id = next_table->stats->table_id;
        table         = next_table;
        next_table    = NULL;

        // EEDBEH: additional printout to debug table lookup
        if (VLOG_IS_DBG_ENABLED(LOG_MODULE)) {
            char *m = ofl_structs_match_to_string((struct ofl_match_header*)&(pkt->handle_std->match), pkt->dp->exp);
            VLOG_DBG_RL(LOG_MODULE, &rl, "searching table entry for packet match: %s.", m);
            free(m);
        }
        entry = flow_table_lookup(table, pkt);
        if (entry != NULL) {
	        if (VLOG_IS_DBG_ENABLED(LOG_MODULE)) {
                char *m = ofl_structs_flow_stats_to_string(entry->stats, pkt->dp->exp);
                VLOG_DBG_RL(LOG_MODULE, &rl, "found matching entry: %s.", m);
                free(m);
            }
            pkt->handle_std->table_miss = is_table_miss(entry);
            execute_entry(pl, entry, &next_table, &pkt);
            /* Packet could be destroyed by a meter instruction */
            if (!pkt)
                return;

            if (next_table == NULL) {
               /* Cookie field is set 0xffffffffffffffff
                because we cannot associate it to any
                particular flow */
                action_set_execute(pkt->action_set, pkt, 0xffffffffffffffff);
                return;
            }

        } else {
            /*Tratamos los paquetes del protocolo, empezando por el Request (Broadcast)*/
            if (select_ehddp_packets(pkt, resent_packet_ehddp) == 1)
            {
                //UAH sino es un ehddp y no tenemos reglas lo enviamos nosotros por donde sabemos
                VLOG_DBG(LOG_MODULE, "No es un paquete eHDDP asi que pasamos a la función de learning.");
                pipeline_arp_path(pkt);
            }
            /* OpenFlow 1.3 default behavior on a table miss */
			VLOG_DBG_RL(LOG_MODULE, &rl, "No matching entry found. Dropping packet.");
			packet_destroy(pkt);
			return;
        }
    }
    VLOG_WARN_RL(LOG_MODULE, &rl, "Reached outside of pipeline processing cycle.");
}

static
int inst_compare(const void *inst1, const void *inst2){
    struct ofl_instruction_header * i1 = *(struct ofl_instruction_header **) inst1;
    struct ofl_instruction_header * i2 = *(struct ofl_instruction_header **) inst2;
    if ((i1->type == OFPIT_APPLY_ACTIONS && i2->type == OFPIT_CLEAR_ACTIONS) ||
        (i1->type == OFPIT_CLEAR_ACTIONS && i2->type == OFPIT_APPLY_ACTIONS))
        return i1->type > i2->type;

    return i1->type < i2->type;
}

ofl_err
pipeline_handle_flow_mod(struct pipeline *pl, struct ofl_msg_flow_mod *msg,
                                                const struct sender *sender) {
    /* Note: the result of using table_id = 0xff is undefined in the spec.
     *       for now it is accepted for delete commands, meaning to delete
     *       from all tables */
    ofl_err error;
    size_t i;
    bool match_kept,insts_kept;

    if(sender->remote->role == OFPCR_ROLE_SLAVE)
        return ofl_error(OFPET_BAD_REQUEST, OFPBRC_IS_SLAVE);

    match_kept = false;
    insts_kept = false;

    /*Sort by execution oder*/
    qsort(msg->instructions, msg->instructions_num,
        sizeof(struct ofl_instruction_header *), inst_compare);

    // Validate actions in flow_mod
    for (i=0; i< msg->instructions_num; i++) {
        if (msg->instructions[i]->type == OFPIT_APPLY_ACTIONS ||
            msg->instructions[i]->type == OFPIT_WRITE_ACTIONS) {
            struct ofl_instruction_actions *ia = (struct ofl_instruction_actions *)msg->instructions[i];

            error = dp_actions_validate(pl->dp, ia->actions_num, ia->actions);
            if (error) {
                return error;
            }
            error = dp_actions_check_set_field_req(msg, ia->actions_num, ia->actions);
            if (error) {
                return error;
            }
        }
	/* Reject goto in the last table. */
	if ((msg->table_id == (PIPELINE_TABLES - 1))
	    && (msg->instructions[i]->type == OFPIT_GOTO_TABLE))
	  return ofl_error(OFPET_BAD_INSTRUCTION, OFPBIC_UNSUP_INST);
    }

    if (msg->table_id == 0xff) {
        if (msg->command == OFPFC_DELETE || msg->command == OFPFC_DELETE_STRICT) {
            size_t i;

            error = 0;
            for (i=0; i < PIPELINE_TABLES; i++) {
                error = flow_table_flow_mod(pl->tables[i], msg, &match_kept, &insts_kept);
                if (error) {
                    break;
                }
            }
            if (error) {
                return error;
            } else {
                ofl_msg_free_flow_mod(msg, !match_kept, !insts_kept, pl->dp->exp);
                return 0;
            }
        } else {
            return ofl_error(OFPET_FLOW_MOD_FAILED, OFPFMFC_BAD_TABLE_ID);
        }
    } else {
        error = flow_table_flow_mod(pl->tables[msg->table_id], msg, &match_kept, &insts_kept);
        if (error) {
            return error;
        }
        if ((msg->command == OFPFC_ADD || msg->command == OFPFC_MODIFY || msg->command == OFPFC_MODIFY_STRICT) &&
                            msg->buffer_id != NO_BUFFER) {
            /* run buffered message through pipeline */
            struct packet *pkt;

            pkt = dp_buffers_retrieve(pl->dp->buffers, msg->buffer_id);
            if (pkt != NULL) {
		      pipeline_process_packet(pl, pkt);
            } else {
                VLOG_WARN_RL(LOG_MODULE, &rl, "The buffer flow_mod referred to was empty (%u).", msg->buffer_id);
            }
        }

        ofl_msg_free_flow_mod(msg, !match_kept, !insts_kept, pl->dp->exp);
        return 0;
    }

}

ofl_err
pipeline_handle_table_mod(struct pipeline *pl,
                          struct ofl_msg_table_mod *msg,
                          const struct sender *sender) {

    if(sender->remote->role == OFPCR_ROLE_SLAVE)
        return ofl_error(OFPET_BAD_REQUEST, OFPBRC_IS_SLAVE);

    if (msg->table_id == 0xff) {
        size_t i;

        for (i=0; i<PIPELINE_TABLES; i++) {
            pl->tables[i]->features->config = msg->config;
        }
    } else {
        pl->tables[msg->table_id]->features->config = msg->config;
    }

    ofl_msg_free((struct ofl_msg_header *)msg, pl->dp->exp);
    return 0;
}

ofl_err
pipeline_handle_stats_request_flow(struct pipeline *pl,
                                   struct ofl_msg_multipart_request_flow *msg,
                                   const struct sender *sender) {

    struct ofl_flow_stats **stats = xmalloc(sizeof(struct ofl_flow_stats *));
    size_t stats_size = 1;
    size_t stats_num = 0;

    if (msg->table_id == 0xff) {
        size_t i;
        for (i=0; i<PIPELINE_TABLES; i++) {
            flow_table_stats(pl->tables[i], msg, &stats, &stats_size, &stats_num);
        }
    } else {
        flow_table_stats(pl->tables[msg->table_id], msg, &stats, &stats_size, &stats_num);
    }

    {
        struct ofl_msg_multipart_reply_flow reply =
                {{{.type = OFPT_MULTIPART_REPLY},
                  .type = OFPMP_FLOW, .flags = 0x0000},
                 .stats     = stats,
                 .stats_num = stats_num
                };

        dp_send_message(pl->dp, (struct ofl_msg_header *)&reply, sender);
    }

    free(stats);
    ofl_msg_free((struct ofl_msg_header *)msg, pl->dp->exp);
    return 0;
}

ofl_err
pipeline_handle_stats_request_table(struct pipeline *pl,
                                    struct ofl_msg_multipart_request_header *msg UNUSED,
                                    const struct sender *sender) {
    struct ofl_table_stats **stats;
    size_t i;

    stats = xmalloc(sizeof(struct ofl_table_stats *) * PIPELINE_TABLES);

    for (i=0; i<PIPELINE_TABLES; i++) {
        stats[i] = pl->tables[i]->stats;
    }

    {
        struct ofl_msg_multipart_reply_table reply =
                {{{.type = OFPT_MULTIPART_REPLY},
                  .type = OFPMP_TABLE, .flags = 0x0000},
                 .stats     = stats,
                 .stats_num = PIPELINE_TABLES};

        dp_send_message(pl->dp, (struct ofl_msg_header *)&reply, sender);
    }

    free(stats);
    ofl_msg_free((struct ofl_msg_header *)msg, pl->dp->exp);
    return 0;
}

ofl_err
pipeline_handle_stats_request_table_features_request(struct pipeline *pl,
                                    struct ofl_msg_multipart_request_header *msg,
                                    const struct sender *sender) {
    struct ofl_table_features **features;
    struct ofl_msg_multipart_request_table_features *feat =
                       (struct ofl_msg_multipart_request_table_features *) msg;
    int i;           /* Feature index in feature array. Jean II */
    int table_id;
    ofl_err error = 0;

    /* Further validation of request not done in
     * ofl_structs_table_features_unpack(). Jean II */
    if(feat->table_features != NULL) {
        for(i = 0; i < feat->tables_num; i++){
	    if(feat->table_features[i]->table_id >= PIPELINE_TABLES)
	        return ofl_error(OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_TABLE);
	    /* We may want to validate things like config, max_entries,
	     * metadata... */
        }
    }

    /* Check if we already received fragments of a multipart request. */
    if(sender->remote->mp_req_msg != NULL) {
      bool nomore;

      /* We can only merge requests having the same XID. */
      if(sender->xid != sender->remote->mp_req_xid)
	{
	  VLOG_ERR(LOG_MODULE, "multipart request: wrong xid (0x%X != 0x%X)", sender->xid, sender->remote->mp_req_xid);

	  /* Technically, as our buffer can only hold one pending request,
	   * this is a buffer overflow ! Jean II */
	  /* Return error. */
	  return ofl_error(OFPET_BAD_REQUEST, OFPBRC_MULTIPART_BUFFER_OVERFLOW);
	}

      VLOG_DBG(LOG_MODULE, "multipart request: merging with previous fragments (%zu+%zu)", ((struct ofl_msg_multipart_request_table_features *) sender->remote->mp_req_msg)->tables_num, feat->tables_num);

      /* Merge the request with previous fragments. */
      nomore = ofl_msg_merge_multipart_request_table_features((struct ofl_msg_multipart_request_table_features *) sender->remote->mp_req_msg, feat);

      /* Check if incomplete. */
      if(!nomore)
	return 0;

      VLOG_DBG(LOG_MODULE, "multipart request: reassembly complete (%zu)", ((struct ofl_msg_multipart_request_table_features *) sender->remote->mp_req_msg)->tables_num);

      /* Use the complete request. */
      feat = (struct ofl_msg_multipart_request_table_features *) sender->remote->mp_req_msg;

#if 0
      {
	char *str;
	str = ofl_msg_to_string((struct ofl_msg_header *) feat, pl->dp->exp);
	VLOG_DBG(LOG_MODULE, "\nMerged request:\n%s\n\n", str);
	free(str);
      }
#endif

    } else {
      /* Check if the request is an initial fragment. */
      if(msg->flags & OFPMPF_REQ_MORE) {
	struct ofl_msg_multipart_request_table_features* saved_msg;

	VLOG_DBG(LOG_MODULE, "multipart request: create reassembly buffer (%zu)", feat->tables_num);

	/* Create a buffer the do reassembly. */
	saved_msg = (struct ofl_msg_multipart_request_table_features*) malloc(sizeof(struct ofl_msg_multipart_request_table_features));
	saved_msg->header.header.type = OFPT_MULTIPART_REQUEST;
	saved_msg->header.type = OFPMP_TABLE_FEATURES;
	saved_msg->header.flags = 0;
	saved_msg->tables_num = 0;
	saved_msg->table_features = NULL;

	/* Save the fragment for later use. */
	ofl_msg_merge_multipart_request_table_features(saved_msg, feat);
	sender->remote->mp_req_msg = (struct ofl_msg_multipart_request_header *) saved_msg;
	sender->remote->mp_req_xid = sender->xid;

	return 0;
      }

      /* Non fragmented request. Nothing to do... */
      VLOG_DBG(LOG_MODULE, "multipart request: non-fragmented request (%zu)", feat->tables_num);
    }

    /*Check to see if the body is empty.*/
    /* Should check merge->tables_num instead. Jean II */
    if(feat->table_features != NULL){
        int last_table_id = 0;

	/* Check that the table features make sense. */
        for(i = 0; i < feat->tables_num; i++){
            /* Table-IDs must be in ascending order. */
            table_id = feat->table_features[i]->table_id;
            if(table_id < last_table_id) {
                error = ofl_error(OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_TABLE);
		break;
            }
            /* Can't go over out internal max-entries. */
            if (feat->table_features[i]->max_entries > FLOW_TABLE_MAX_ENTRIES) {
                error = ofl_error(OFPET_TABLE_FEATURES_FAILED, OFPTFFC_BAD_ARGUMENT);
		break;
            }
        }

        if (error == 0) {

            /* Disable all tables, they will be selectively re-enabled. */
            for(table_id = 0; table_id < PIPELINE_TABLES; table_id++){
	        pl->tables[table_id]->disabled = true;
            }
            /* Change tables configuration
               TODO: Remove flows*/
            VLOG_DBG(LOG_MODULE, "pipeline_handle_stats_request_table_features_request: updating features");
            for(i = 0; i < feat->tables_num; i++){
                table_id = feat->table_features[i]->table_id;

                /* Replace whole table feature. */
                ofl_structs_free_table_features(pl->tables[table_id]->features, pl->dp->exp);
                pl->tables[table_id]->features = feat->table_features[i];
                feat->table_features[i] = NULL;

                /* Re-enable table. */
                pl->tables[table_id]->disabled = false;
            }
        }
    }

    /* Cleanup request. */
    if(sender->remote->mp_req_msg != NULL) {
      ofl_msg_free((struct ofl_msg_header *) sender->remote->mp_req_msg, pl->dp->exp);
      sender->remote->mp_req_msg = NULL;
      sender->remote->mp_req_xid = 0;  /* Currently not needed. Jean II. */
    }

    if (error) {
        return error;
    }

    table_id = 0;
    /* Query for table capabilities */
    loop: ;
    features = (struct ofl_table_features**) xmalloc(sizeof(struct ofl_table_features *) * 8);
    /* Return 8 tables per reply segment. */
    for (i = 0; i < 8; i++){
        /* Skip disabled tables. */
        while((table_id < PIPELINE_TABLES) && (pl->tables[table_id]->disabled == true))
	    table_id++;
	/* Stop at the last table. */
	if(table_id >= PIPELINE_TABLES)
	    break;
	/* Use that table in the reply. */
        features[i] = pl->tables[table_id]->features;
        table_id++;
    }
    VLOG_DBG(LOG_MODULE, "multipart reply: returning %d tables, next table-id %d", i, table_id);
    {
    struct ofl_msg_multipart_reply_table_features reply =
         {{{.type = OFPT_MULTIPART_REPLY},
           .type = OFPMP_TABLE_FEATURES,
           .flags = (table_id == PIPELINE_TABLES ? 0x00000000 : OFPMPF_REPLY_MORE) },
          .table_features     = features,
          .tables_num = i };
          dp_send_message(pl->dp, (struct ofl_msg_header *)&reply, sender);
    }
    if (table_id < PIPELINE_TABLES){
           goto loop;
    }
    free(features);

    return 0;
}

ofl_err
pipeline_handle_stats_request_aggregate(struct pipeline *pl,
                                  struct ofl_msg_multipart_request_flow *msg,
                                  const struct sender *sender) {
    struct ofl_msg_multipart_reply_aggregate reply =
            {{{.type = OFPT_MULTIPART_REPLY},
              .type = OFPMP_AGGREGATE, .flags = 0x0000},
              .packet_count = 0,
              .byte_count   = 0,
              .flow_count   = 0};

    if (msg->table_id == 0xff) {
        size_t i;

        for (i=0; i<PIPELINE_TABLES; i++) {
            flow_table_aggregate_stats(pl->tables[i], msg,
                                       &reply.packet_count, &reply.byte_count, &reply.flow_count);
        }

    } else {
        flow_table_aggregate_stats(pl->tables[msg->table_id], msg,
                                   &reply.packet_count, &reply.byte_count, &reply.flow_count);
    }

    dp_send_message(pl->dp, (struct ofl_msg_header *)&reply, sender);

    ofl_msg_free((struct ofl_msg_header *)msg, pl->dp->exp);
    return 0;
}


void
pipeline_destroy(struct pipeline *pl) {
    struct flow_table *table;
    int i;

    for (i=0; i<PIPELINE_TABLES; i++) {
        table = pl->tables[i];
        if (table != NULL) {
            flow_table_destroy(table);
        }
    }
    free(pl);
}


void
pipeline_timeout(struct pipeline *pl) {
    int i;

    for (i = 0; i < PIPELINE_TABLES; i++) {
        flow_table_timeout(pl->tables[i]);
    }
}


/* Executes the instructions associated with a flow entry */
static void
execute_entry(struct pipeline *pl, struct flow_entry *entry,
              struct flow_table **next_table, struct packet **pkt) {
    /* NOTE: instructions, when present, will be executed in
            the following order:
            Meter
            Apply-Actions
            Clear-Actions
            Write-Actions
            Write-Metadata
            Goto-Table
    */
    size_t i;
    struct ofl_instruction_header *inst;

    for (i=0; i < entry->stats->instructions_num; i++) {
        /*Packet was dropped by some instruction or action*/

        if(!(*pkt)){
            return;
        }

        inst = entry->stats->instructions[i];
        switch (inst->type) {
            case OFPIT_GOTO_TABLE: {
                struct ofl_instruction_goto_table *gi = (struct ofl_instruction_goto_table *)inst;

                *next_table = pl->tables[gi->table_id];
                break;
            }
            case OFPIT_WRITE_METADATA: {
                struct ofl_instruction_write_metadata *wi = (struct ofl_instruction_write_metadata *)inst;
                struct  ofl_match_tlv *f;

                /* NOTE: Hackish solution. If packet had multiple handles, metadata
                 *       should be updated in all. */
                packet_handle_std_validate((*pkt)->handle_std);
                /* Search field on the description of the packet. */
                HMAP_FOR_EACH_WITH_HASH(f, struct ofl_match_tlv,
                    hmap_node, hash_int(OXM_OF_METADATA,0), &(*pkt)->handle_std->match.match_fields){
                    uint64_t *metadata = (uint64_t*) f->value;
                    *metadata = (*metadata & ~wi->metadata_mask) | (wi->metadata & wi->metadata_mask);
                    VLOG_DBG_RL(LOG_MODULE, &rl, "Executing write metadata: 0x%"PRIx64"", *metadata);
                }
                break;
            }
            case OFPIT_WRITE_ACTIONS: {
                struct ofl_instruction_actions *wa = (struct ofl_instruction_actions *)inst;
                action_set_write_actions((*pkt)->action_set, wa->actions_num, wa->actions);
                break;
            }
            case OFPIT_APPLY_ACTIONS: {
                struct ofl_instruction_actions *ia = (struct ofl_instruction_actions *)inst;
                dp_execute_action_list((*pkt), ia->actions_num, ia->actions, entry->stats->cookie);
                break;
            }
            case OFPIT_CLEAR_ACTIONS: {
                action_set_clear_actions((*pkt)->action_set);
                break;
            }
            case OFPIT_METER: {
            	struct ofl_instruction_meter *im = (struct ofl_instruction_meter *)inst;
                meter_table_apply(pl->dp->meters, pkt , im->meter_id);
                break;
            }
            case OFPIT_EXPERIMENTER: {
                dp_exp_inst((*pkt), (struct ofl_instruction_experimenter *)inst);
                break;
            }
        }
    }
}

/*Modificacion UAH Discovery hybrid topologies, JAH-*/
static inline uint64_t hton642(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}


uint8_t select_ehddp_packets(struct packet *pkt, uint8_t resent_packet_ehddp){
    uint16_t  eth_type = pkt->handle_std->proto->eth->eth_type;

    //packet HDT (EthType = FFAA o AAFF)
    if( (eth_type== ETH_TYPE_EHDDP || eth_type == ETH_TYPE_EHDDP_INV) && pkt->dp->id > 1){
        //paquetes broadcast son paquetes request
        // VLOG_INFO(LOG_MODULE, "Paquete ehddp detectado Opcode : %d", (int)pkt->handle_std->proto->ehddp->opcode);
        if (pkt->handle_std->proto->ehddp->opcode == 1){
            VLOG_INFO(LOG_MODULE, "Paquete ehddp REQUEST detectado");
            if (time_start == 0 || pkt->handle_std->proto->ehddp->num_sec == time_start){
                time_start = bigtolittle64(pkt->handle_std->proto->ehddp->num_sec);
                time_exploration = current_timestamp(); //time_msec();
                //if(time_msec() - time_start < 0)
                if (current_timestamp() - time_start < 0)
                    convergence_time = 0; // indica que es tan rapido que no soy capaz de medir
                else
                    convergence_time = abs(current_timestamp() - time_start) ; //time_msec() - time_start); 
            }
            handle_ehddp_request_packets(pkt, resent_packet_ehddp);
        }//paquetes unicast son paquetes reply
        else if (pkt->handle_std->proto->ehddp->opcode == 2){
            VLOG_INFO(LOG_MODULE, "Paquete ehddp REPLY detectado");
            handle_ehddp_reply_packets(pkt);
        }
        else if (pkt->handle_std->proto->ehddp->opcode == 3){
            VLOG_INFO(LOG_MODULE, "Paquete ehddp ACK detectado No nos interesa para redes ethernet");
        }
        else{
            VLOG_INFO(LOG_MODULE, "Paquete ehddp DEFECTUOSO option code: >>> %d <<< !!!", pkt->handle_std->proto->ehddp->opcode);
        }
        return 0; //ya ha sido tratado por eHDDP
    }
    //el paquete no es un eHDDP se debe tratar por otro lado
    return 1;
}

uint8_t ehddp_mod_local_port (struct packet * pkt){  
    
    uint8_t mac[ETH_ADDR_LEN];
    struct sw_port *p;
    int error, result;

    if((pkt->handle_std->proto->eth->eth_type== ETH_TYPE_EHDDP || pkt->handle_std->proto->eth->eth_type == ETH_TYPE_EHDDP_INV) && 
        (pkt->dp->id > 1 && pkt->dp->id < 1000)){
        //VLOG_WARN(LOG_MODULE, "UAH -> eHDDP detectado miramos si tenemos que crear local port");
        if (pkt->handle_std->proto->ehddp->opcode == 1)
        {
            //Aumentamos el estadistico de ehddp
            num_pkt_ehddp_req++;
            result = mac_to_port_found_port(&bt_table, pkt->handle_std->proto->ehddp->src_mac, pkt->handle_std->proto->ehddp->num_sec);
            if (result <= 0){
                /*le marcamos como puerto de controller*/
                port_to_controller = pkt->in_port;
                /*Guardamos la info para no liarla */
                if (result == -1)
                    mac_to_port_add(&bt_table, pkt->handle_std->proto->ehddp->src_mac, pkt->in_port, htonl(pkt->handle_std->proto->ehddp->time_block),
                        pkt->handle_std->proto->ehddp->num_sec);
                else
                    mac_to_port_update(&bt_table, pkt->handle_std->proto->ehddp->src_mac, pkt->in_port, htonl(pkt->handle_std->proto->ehddp->time_block),
                        pkt->handle_std->proto->ehddp->num_sec);
                VLOG_WARN(LOG_MODULE, "UAH -> Guardado correctamente el puerto en la tabla de eHDDP pasamos a comprobar local port");
                /*Configuramos el puerto de entrada como nuevo puerto local*/
                /*Modificaciones UAH*/
                if (type_device_general != 2){ //le metemos la condición de que existen los no sdn para que no conecte)
                    if (time_no_move_local_port == 0 || time_msec() + Time_wait_local_port < time_no_move_local_port){
                        LIST_FOR_EACH (p, struct sw_port, node, &pkt->dp->port_list) {
                            if (pkt->in_port == p->conf->port_no) // S_ACTIVE = 8 y S_IDLE = 16)
                            {
                                if (pkt->dp->local_port == NULL) //&& (conection_status_ofp_controller & (4 | 8 | 16)) == 0
                                {    
                                    VLOG_WARN(LOG_MODULE, "UAH -> Entro para modificar el LOCAL PORT");
                                    //old_local_port = 0; //Como es el primero le guardo como puerto anterior
                                    eth_addr_from_uint64(pkt->dp->id, mac);
                                    error = configure_new_local_port_ehddp_UAH(pkt->dp, mac, old_local_port, bigtolittle64(pkt->handle_std->proto->ehddp->num_sec));
                                    if (!error) {
                                        ofp_error(error, "UAH -> failed to add local port %s", p->conf->name);
                                        if (pkt->dp->local_port != NULL)
                                            free(pkt->dp->local_port);
                                        return -1;
                                    }
                                    old_local_port = error; // si el error es distinto de 0 es el primer puerto local
                                    /*Ahora hacemos que el ofprotocol se entere*/
                                    dp_ports_handle_port_mod_UAH(pkt->dp, p->conf->port_no);
                                    local_port_ok = false;
                                    VLOG_WARN(LOG_MODULE, "UAH -> Local port configurado listo para conectar");
                                }
                                else
                                {
                                    VLOG_WARN(LOG_MODULE, "UAH -> Existe un puerto local ya asi que no hacemos");
                                }
                                break;
                            }
                        }
                        time_no_move_local_port = time_msec() + Time_wait_local_port;
                    }
                }
                return 1;
            }
            else{
                //guardo el orden de llegada para poder intentar recuperar por otro lado
                mac_to_port_add(&bt_table, pkt->handle_std->proto->ehddp->src_mac, pkt->in_port, htonl(pkt->handle_std->proto->ehddp->time_block),
                    pkt->handle_std->proto->ehddp->num_sec);
            }
        }
        
    }
    return 0;
}

uint8_t handle_ehddp_request_packets(struct packet *pkt, uint8_t resent_packet_ehddp){
    int table_port = 0; //varible auxiliar para puertp
    int send_ehddp_packet = 0; //1 only request paquet; 2 only reply packet;
    int num_ports = 0; //numero de puertos disponibles
    struct packet * cpy_pkt; //forzamos el envio al controller para que mande arp

    VLOG_INFO(LOG_MODULE, "Calculamos el puerto de entrada y el numero de puertos disponible");
    num_ports = num_port_available(pkt->dp);
    //table_port = mac_to_port_found_port(&bt_table, pkt->handle_std->proto->eth->eth_src, pkt->handle_std->proto->ehddp->num_sec);
    VLOG_INFO(LOG_MODULE, "Puerto: entrada %d | Puerto en tabla: %d | Numero de puertos disponibles: %d  | Tiempo de bloqueo: %d", 
        pkt->in_port, table_port ,num_ports, htonl(pkt->handle_std->proto->ehddp->time_block));

    // Mismo puerto que el anotado en el paso anterior 
    //-> no puede existir un puerto no encontrado cuando llegue aqui(table_port == -1 ) //Puerto no encontrado
    if (resent_packet_ehddp == 1) //&& (conection_status_ofp_controller & (4 | 8 | 16)) == 0)
    {
        VLOG_INFO(LOG_MODULE, "actualizo el puerto de la entrada de tabla BT al puerto: %d", pkt->in_port);
        mac_to_port_update(&bt_table, pkt->handle_std->proto->eth->eth_src, pkt->in_port, htonl(pkt->handle_std->proto->ehddp->time_block),
            pkt->handle_std->proto->ehddp->num_sec);
        send_ehddp_packet = 1;
    }
    else if ((resent_packet_ehddp == 0 )) //&& (conection_status_ofp_controller & (4 | 8 | 16)) == 0))
    {
        /* contestamos con reply */
        send_ehddp_packet = 2;
    }
    else{
        //si somos un nodo sdn y tenemos la regla instaladas no debemos llegar aqui
        send_ehddp_packet = 0; 
    }
    
    //visualizar_tabla(&bt_table, pkt->dp->id);
    
    /* Si solo tengo un puerto contesto con reply */
    if (num_ports == 1 || send_ehddp_packet == 2){
        VLOG_INFO(LOG_MODULE, "Entramos en generar el reply: num_ports: %d", num_ports);
        //visualizar_tabla(&bt_table, pkt->dp->id);
        creator_ehddp_reply_packets(pkt);
    }
    //actualizo el numero de dispositivos por los que pasa el request, solo si tengo puertos
    //disponibles para reenviar el paquete
    else if(send_ehddp_packet == 1) 
    {
        //Mandamos al controlador para forzar el arp
        cpy_pkt = packet_clone(pkt);
        send_packet_to_controller(pkt->dp->pipeline, pkt, pkt->table_id, OFPR_NO_MATCH);
        VLOG_INFO(LOG_MODULE, "Se ha enviado un copia del eHDDP al controller para forzar el envio del ARP al controlador");
        packet_destroy(cpy_pkt);

        /*continamos nosotros haciendo el proceso*/
        update_data_msg(pkt, (uint32_t) OFPP_FLOOD, pkt->handle_std->proto->ehddp->nxt_mac);
        VLOG_INFO(LOG_MODULE,"Update number of jump: %d ", pkt->handle_std->proto->ehddp->num_devices);
        //visualizar_tabla(&bt_table, pkt->dp->id);
        dp_actions_output_port(pkt, OFPP_FLOOD, pkt->out_queue, pkt->out_port_max_len, 0xffffffffffffffff);
    }
    return 1;
}

uint8_t handle_ehddp_reply_packets(struct packet *pkt){

    int out_port = 0;
    uint16_t num_elementos = 0;
    uint8_t nxt_mac[ETH_ADDR_LEN] = {0};

    out_port = mac_to_port_found_port(&bt_table, pkt->handle_std->proto->ehddp->nxt_mac,
        pkt->handle_std->proto->ehddp->num_sec);

    if (out_port < 1){
        VLOG_INFO(LOG_MODULE,"ERROR!!!! DON'T found any out_port!!!!");
        return 0;
    }
    else
    {

        VLOG_INFO(LOG_MODULE, "Update the eHDDP reply packet: in-port: %d | out-port %d | type_device: %d",
            pkt->in_port, out_port, NODO_SDN_CONFIG);

        if (mac_to_port_found_mac_position(&bt_table, 1, nxt_mac) < 0) //obtenemos la mac del controller
        {
            VLOG_INFO(LOG_MODULE, "No tenemos la mac del controller todavia asi que no podemos hacer nada");
            return -1;
        }

        num_elementos = update_data_msg(pkt, out_port, nxt_mac);
        VLOG_INFO(LOG_MODULE, "Reply packet: %s", packet_to_string(pkt));

        if(num_elementos == 0){ // Indica que tenemos hueco en el paquete para enviar 
            //visualizar_tabla(mac_port, pkt->dp->id);
            dp_actions_output_port(pkt, out_port, pkt->out_queue, pkt->out_port_max_len, 0xffffffffffffffff);
            /*Aumentamos el estadistico de ehddp reply*/
            num_pkt_ehddp_rep++;
        }
        else
            VLOG_INFO(LOG_MODULE, "Se ha sobrepasado el numero de elementos maximos en el paquete: %d",num_elementos);
                
        return 1;
    }    
}


void creator_ehddp_reply_packets(struct packet *pkt){
    struct packet *pkt_reply = NULL;
    uint8_t num_devices = 0x01;
    uint16_t type_device;

    if (type_device_general == 1 || type_device_general == 3)
        type_device = NODO_SDN_CONFIG;
    else
        type_device = NODO_NO_SDN;

    /*Siempre que actualize/cree un ehddp son un nodo NODO_SDN_CONFIG*/
    pkt_reply = create_ehddp_reply_packet(pkt->dp, pkt->handle_std->proto->eth->eth_src,pkt->in_port,
        pkt->in_port, type_device, num_devices,
        pkt->handle_std->proto->ehddp->num_sec, pkt->handle_std->proto->ehddp->num_ack, pkt->handle_std->proto->ehddp->time_block);
    VLOG_INFO(LOG_MODULE, "create_ehddp_reply_packet OK");
    //envio el paquete por el puerto de entrada
    dp_actions_output_port(pkt_reply, pkt->in_port, pkt->out_queue, pkt->out_port_max_len, 0xffffffffffffffff);
    VLOG_INFO(LOG_MODULE, "dp_actions_output_port OK OUT PORT : %d", pkt->in_port);
    /*Aumentamos el estadistico de ehddp reply*/
    num_pkt_ehddp_rep++;

    //destruyo el paquete para limpiar la memoria
    if (pkt_reply){
        packet_destroy(pkt_reply);
        VLOG_INFO(LOG_MODULE, "pkt_reply delete!! OK");
    }
}

void pipeline_arp_path(struct packet *pkt)
{
    int puerto_mac = 0;
    struct packet * cpy_pkt;
    
    if (VLOG_IS_DBG_ENABLED(LOG_MODULE)) {
        char *pkt_str = packet_to_string(pkt);
        VLOG_DBG_RL(LOG_MODULE, &rl, "processing packet: %s", pkt_str);
        free(pkt_str);
    }

    if (!pkt)
    {
        VLOG_INFO(LOG_MODULE, "Es un paquete no es válido");
        return;
    }

    if (!pkt->dp)
    {
        VLOG_INFO(LOG_MODULE, "El dp no esta creado correctamente");
        return;
    }

    if (pkt->dp->local_port){
        if (memcmp(pkt->handle_std->proto->eth->eth_src, netdev_get_etheraddr(pkt->dp->local_port->netdev),ETH_ADDR_LEN) ==0){
            VLOG_INFO(LOG_MODULE, "Es un paquete ARP que he creado yo mismo");
            return;
        }
    }

	if (pkt->handle_std->proto->eth->eth_type == ETH_TYPE_ARP || pkt->handle_std->proto->eth->eth_type == ETH_TYPE_ARP_INV) 
	{
        puerto_mac = mac_to_port_found_port(&learning_table, pkt->handle_std->proto->eth->eth_src, 0);
        VLOG_INFO(LOG_MODULE, "Puerto_mac = %d | pkt->in_port = %d", puerto_mac, pkt->in_port);
        if (eth_addr_is_broadcast(pkt->handle_std->proto->eth->eth_dst) || eth_addr_is_multicast(pkt->handle_std->proto->eth->eth_dst))
        {
            VLOG_INFO(LOG_MODULE, "Es un paquete ARP-Request del tipo broadcast");
			if (puerto_mac == -1 || puerto_mac == 0){
                VLOG_INFO(LOG_MODULE, "No conozco Puerto anterior");
				mac_to_port_add(&learning_table, pkt->handle_std->proto->eth->eth_src, pkt->in_port, BT_TIME_PKT, 0);
            }
			else if (puerto_mac == pkt->in_port){
                //VLOG_INFO(LOG_MODULE, "SI conozco Puerto anterior y es el mismo que el que tengo");
				mac_to_port_time_refresh(&learning_table, pkt->handle_std->proto->eth->eth_src, BT_TIME_PKT,0);
            }
			else
			{
				return;
			}
            /*Se lo mandamos al controlador tambien para que tenga notificación de ello, solo si somos un SDN*/
            if (type_device_general != 2) { //} && controller_connected == true){
                cpy_pkt = packet_clone(pkt);
                send_packet_to_controller(pkt->dp->pipeline, pkt, pkt->table_id, OFPR_NO_MATCH);
                VLOG_INFO(LOG_MODULE, "Se ha enviado un copia del ARP al controller para su aprendizaje");
                packet_destroy(cpy_pkt);
            }
            VLOG_INFO(LOG_MODULE, "Se guarda correctamente todo pasamos a hacer el broadcast del ARP");

            VLOG_INFO(LOG_MODULE, "ar_tpa = %u | ar_tpa=%u | s_addr = %u", pkt->handle_std->proto->arp->ar_tpa, 
                htonl(pkt->handle_std->proto->arp->ar_tpa), ip_del_controlller.s_addr);
            //dependiendo del arp puedo sacarlo por el puerto del controlador
            if (pkt->handle_std->proto->arp->ar_tpa != ip_del_controlller.s_addr)
            {
                VLOG_INFO(LOG_MODULE, "La IP NO es la del CONTROLLER se envia por FLOOD");
			    dp_actions_output_port(pkt, OFPP_FLOOD, pkt->out_queue, pkt->out_port_max_len, 0xffffffffffffffff);
            }
            else{
                VLOG_INFO(LOG_MODULE, "La IP SI es la del CONTROLLER se envia por %d", port_to_controller);
                //dp_actions_output_port(pkt, port_to_controller, pkt->out_queue, pkt->out_port_max_len, 0xffffffffffffffff);
                VLOG_INFO(LOG_MODULE, "PERO SE LO DEJAMOS A SDN QUE YA TIENE LA REGLA");
            }
            return;
        }
        else 
        {
            VLOG_INFO(LOG_MODULE, "El paquete no es broadcast");
            if (htons(pkt->handle_std->proto->arp->ar_op) == 2){
                if (puerto_mac == -1){
                    VLOG_INFO(LOG_MODULE, "Es un ARP-Reply pero NO conocemos un puerto anterior"); 
                    mac_to_port_add(&learning_table, pkt->handle_std->proto->eth->eth_src, pkt->in_port, LT_TIME, 0);
                }
                else
                {
                    VLOG_INFO(LOG_MODULE, "Es un ARP-Reply pero conocemos un puerto anterior"); 
                    if(mac_to_port_check_timeout(&learning_table, pkt->handle_std->proto->eth->eth_src) == 1)
                        mac_to_port_update(&learning_table, pkt->handle_std->proto->eth->eth_src, pkt->in_port, LT_TIME, 0);
                    else
                        mac_to_port_time_refresh(&learning_table, pkt->handle_std->proto->eth->eth_src,LT_TIME, 0); 
                } 
            }
            VLOG_INFO(LOG_MODULE, "Todo actualizado de forma correcta");         
       }
    }
    VLOG_INFO(LOG_MODULE, "Buscamos puerto de salida");
    puerto_mac = mac_to_port_found_port(&learning_table, pkt->handle_std->proto->eth->eth_dst, 0);
    VLOG_INFO(LOG_MODULE, "sacamos paquete unicast por puerto = %d", puerto_mac);
    arp_path_send_unicast(pkt, puerto_mac);

    return;
}

int arp_path_send_unicast(struct packet * pkt, int out_port)
{
	if (out_port > 0)
	{
		if(pkt->dp->ports[out_port].conf->state == OFPPS_LIVE)
		{
			mac_to_port_time_refresh(&learning_table, pkt->handle_std->proto->eth->eth_dst, LT_TIME, 0);
			dp_actions_output_port(pkt, out_port,pkt->out_queue, pkt->out_port_max_len, 0xffffffffffffffff);
		}
	}
	return 0;
}


/*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/
