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

#ifndef OXM_MATCH_H
#define OXM_MATCH_H 1

#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "ofpbuf.h"
#include "hmap.h"
#include "packets.h"
#include "openflow/openflow.h" 
#include "../oflib/ofl-structs.h"


#define OXM_VENDOR(HEADER) ((HEADER) >> 16)
#define OXM_FIELD(HEADER) (((HEADER) >> 9) & 0x7f)
#define OXM_TYPE(HEADER) (((HEADER) >> 9) & 0x7fffff)
#define OXM_HASMASK(HEADER) (((HEADER) >> 8) & 1)
#define OXM_LENGTH(HEADER) ((HEADER) & 0xff)
#define VENDOR_FROM_TYPE(TYPE) ((TYPE) >> 7)
#define FIELD_FROM_TYPE(TYPE)  ((TYPE) & 0x7f)

enum oxm_field_index {
#define DEFINE_FIELD(HEADER,DL_TYPES, NW_PROTO, MASKABLE) \
        OFI_OXM_##HEADER,
#include "oxm-match.def"
    NUM_OXM_FIELDS = 71 //56 JAH-UAH PARA AÑADIR EL MATCH DE LOS NUEVOS PAQUETES
};

struct oxm_field {
    struct hmap_node hmap_node;
    enum oxm_field_index index;       /* OFI_* value. */
    uint32_t header;                  /* OXM_* value. */
    uint16_t dl_type[N_OXM_DL_TYPES]; /* dl_type prerequisites. */
    uint8_t nw_proto;                 /* nw_proto prerequisite, if nonzero. */
    bool maskable;                    /* Writable with OXAST_REG_{MOVE,LOAD}? */
};

/* All the known fields. */
extern struct oxm_field all_fields[NUM_OXM_FIELDS];

bool 
check_bad_wildcard(uint8_t value, uint8_t mask);

bool 
check_bad_wildcard16(uint16_t value, uint16_t mask);

bool 
check_bad_wildcard32(uint32_t value, uint32_t mask);

bool 
check_bad_wildcard48(uint8_t *value, uint8_t *mask);

bool 
check_bad_wildcard64(uint64_t value, uint64_t mask);

bool 
check_bad_wildcard128(uint8_t *value, uint8_t *mask);

struct oxm_field *
oxm_field_lookup(uint32_t header);

bool
oxm_prereqs_ok(const struct oxm_field *field, const struct ofl_match *rule);

int
oxm_pull_match(struct ofpbuf * buf, struct ofl_match *match_dst, int match_len);

int oxm_put_match(struct ofpbuf *buf, struct ofl_match *omt);

struct ofl_match_tlv *
oxm_match_lookup(uint32_t header, const struct ofl_match *omt);

uint32_t oxm_entry_ok(const void *, unsigned int );

int
oxm_field_bytes(uint32_t header);

int
oxm_field_bits(uint32_t header);



#endif /* oxm-match.h */
