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

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <inttypes.h>

#include "ofl.h"
#include "ofl-print.h"
#include "ofl-structs.h"
#include "ofl-actions.h"
#include "ofl-packets.h"
#include "../oflib/oxm-match.h"
#include "openflow/openflow.h"


#define ETH_ADDR_FMT                                                    \
    "%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8
#define ETH_ADDR_ARGS(ea)                                   \
    (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]



char *
ofl_action_to_string(struct ofl_action_header *act, struct ofl_exp *exp) {
    char *str;
    size_t str_size;
    FILE *stream = open_memstream(&str, &str_size);

    ofl_action_print(stream, act, exp);
    fclose(stream);
    return str;
}

void
ofl_action_print(FILE *stream, struct ofl_action_header *act, struct ofl_exp *exp) {

    ofl_action_type_print(stream, act->type);

    switch (act->type) {
        case OFPAT_OUTPUT: {
            struct ofl_action_output *a = (struct ofl_action_output *)act;

            fprintf(stream, "{port=\"");
            ofl_port_print(stream, a->port);
            if (a->port == OFPP_CONTROLLER) {
                fprintf(stream, "\", mlen=\"%u\"}", a->max_len);
            } else {
                fprintf(stream, "\"}");
            }
            break;
        }
        case OFPAT_SET_FIELD:{
            struct ofl_action_set_field *a = (struct ofl_action_set_field *)act;
            fprintf(stream, "{field:");
            ofl_structs_oxm_tlv_print(stream, a->field);
            fprintf(stream, "}");
            break;
        }
        case OFPAT_COPY_TTL_OUT:
        case OFPAT_COPY_TTL_IN: {
            break;
        }
        case OFPAT_SET_MPLS_TTL: {
            struct ofl_action_mpls_ttl *a = (struct ofl_action_mpls_ttl *)act;

            fprintf(stream, "{ttl=\"%u\"}", a->mpls_ttl);
            break;
        }
        case OFPAT_DEC_MPLS_TTL: {
            break;
        }
        case OFPAT_PUSH_VLAN:
        case OFPAT_PUSH_MPLS:
        case OFPAT_PUSH_PBB:{
            struct ofl_action_push *a = (struct ofl_action_push *)act;

            fprintf(stream, "{eth=\"0x%04"PRIx16"\"}", a->ethertype);
            break;
        }
        case OFPAT_POP_VLAN: 
        case OFPAT_POP_PBB: {
            break;
        }
        case OFPAT_POP_MPLS: {
            struct ofl_action_pop_mpls *a = (struct ofl_action_pop_mpls *)act;

            fprintf(stream, "{eth=\"0x%04"PRIx16"\"}", a->ethertype);
            break;
        }
        case OFPAT_SET_QUEUE: {
            struct ofl_action_set_queue *a = (struct ofl_action_set_queue *)act;

            fprintf(stream, "{q=\"");
            ofl_queue_print(stream, a->queue_id);
            fprintf(stream, "\"}");
            break;
        }
        case OFPAT_GROUP: {
            struct ofl_action_group *a = (struct ofl_action_group *)act;

            fprintf(stream, "{id=\"");
            ofl_group_print(stream, a->group_id);
            fprintf(stream, "\"}");

            break;
        }
        case OFPAT_SET_NW_TTL: {
            struct ofl_action_set_nw_ttl *a = (struct ofl_action_set_nw_ttl *)act;

            fprintf(stream, "{ttl=\"%u\"}", a->nw_ttl);
            break;
        }
        case OFPAT_DEC_NW_TTL: {
            break;
        }
        case OFPAT_EXPERIMENTER: {
            if (exp == NULL || exp->act == NULL || exp->act->to_string == NULL) {
                struct ofl_action_experimenter *a = (struct ofl_action_experimenter *)act;

                fprintf(stream, "{id=\"0x%"PRIx32"\"}", a->experimenter_id);
            } else {
                char *c = exp->act->to_string(act);
                fprintf(stream, "%s", c);
                free (c);
            }
            break;
        }
    }
}
