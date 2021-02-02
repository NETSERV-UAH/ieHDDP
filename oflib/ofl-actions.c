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

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "ofl.h"
#include "ofl-actions.h"
#include "ofl-log.h"

#define LOG_MODULE ofl_act
OFL_LOG_INIT(LOG_MODULE)

void
ofl_actions_free(struct ofl_action_header *act, struct ofl_exp *exp) {
    switch (act->type) {
        case OFPAT_SET_FIELD:{
            struct ofl_action_set_field *a = (struct ofl_action_set_field*) act;
            free(a->field->value);
            free(a->field);
            free(a);
            return;
            break;        
        }
        case OFPAT_OUTPUT:
        case OFPAT_COPY_TTL_OUT:
        case OFPAT_COPY_TTL_IN:
        case OFPAT_SET_MPLS_TTL:
        case OFPAT_DEC_MPLS_TTL:
        case OFPAT_PUSH_VLAN:
        case OFPAT_POP_VLAN:
        case OFPAT_PUSH_MPLS:
        case OFPAT_POP_MPLS:
        case OFPAT_PUSH_PBB:
        case OFPAT_POP_PBB:
        case OFPAT_SET_QUEUE:
        case OFPAT_GROUP:
        case OFPAT_SET_NW_TTL:
        case OFPAT_DEC_NW_TTL: {
            break;
        }
        case OFPAT_EXPERIMENTER: {
            if (exp == NULL || exp->act == NULL || exp->act->free == NULL) {
                OFL_LOG_WARN(LOG_MODULE, "Freeing experimenter action, but no callback is given.");
                break;
            }
            exp->act->free(act);
            return;
        }
        default: {
        }
    }
    free(act);
}

ofl_err
ofl_utils_count_ofp_actions(void *data, size_t data_len, size_t *count) {
    struct ofp_action_header *act;
    uint8_t *d;

    d = (uint8_t *)data;
    *count = 0;

    /* this is needed so that buckets are handled correctly */
    while (data_len >= sizeof(struct ofp_action_header) -4 ) {
        act = (struct ofp_action_header *)d;
        if (data_len < ntohs(act->len) || ntohs(act->len) < sizeof(struct ofp_action_header) - 4) {
            OFL_LOG_WARN(LOG_MODULE, "Received action has invalid length.");
            return ofl_error(OFPET_BAD_ACTION, OFPBAC_BAD_LEN);
        }
        data_len -= ntohs(act->len);
        d += ntohs(act->len);
        (*count)++;
    }

    return 0;
}
