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

#include "ofl-structs.h"
#include "lib/hash.h"
#include "oxm-match.h"

void
ofl_structs_match_init(struct ofl_match *match){

    match->header.type = OFPMT_OXM;
    match->header.length = 0;
    match->match_fields = (struct hmap) HMAP_INITIALIZER(&match->match_fields);
}


void
ofl_structs_match_put8(struct ofl_match *match, uint32_t header, uint8_t value){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint8_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
}

void
ofl_structs_match_put8m(struct ofl_match *match, uint32_t header, uint8_t value, uint8_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint8_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;
}

void
ofl_structs_match_put16(struct ofl_match *match, uint32_t header, uint16_t value){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint16_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
}


void
ofl_structs_match_put16m(struct ofl_match *match, uint32_t header, uint16_t value, uint16_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint16_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;
}

void
ofl_structs_match_put32(struct ofl_match *match, uint32_t header, uint32_t value){
    struct ofl_match_tlv *m = xmalloc(sizeof (struct ofl_match_tlv));

    int len = sizeof(uint32_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put32m(struct ofl_match *match, uint32_t header, uint32_t value, uint32_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint32_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

void
ofl_structs_match_put64(struct ofl_match *match, uint32_t header, uint64_t value){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint64_t);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, &value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put64m(struct ofl_match *match, uint32_t header, uint64_t value, uint64_t mask){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint64_t);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, &value, len);
    memcpy(m->value + len, &mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

void
ofl_structs_match_put_pbb_isid(struct ofl_match *match, uint32_t header, uint8_t value[PBB_ISID_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = OXM_LENGTH(header);

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
}


void
ofl_structs_match_put_pbb_isidm(struct ofl_match *match, uint32_t header, uint8_t value[PBB_ISID_LEN], uint8_t mask[PBB_ISID_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = OXM_LENGTH(header);

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, value, len);
    memcpy(m->value + len, mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;
}

void
ofl_structs_match_put_eth(struct ofl_match *match, uint32_t header, uint8_t value[ETH_ADDR_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = ETH_ADDR_LEN;

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put_eth_m(struct ofl_match *match, uint32_t header, uint8_t value[ETH_ADDR_LEN], uint8_t mask[ETH_ADDR_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = ETH_ADDR_LEN;

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, value, len);
    memcpy(m->value + len, mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

void
ofl_structs_match_put_ipv6(struct ofl_match *match, uint32_t header, uint8_t value[IPv6_ADDR_LEN]){

    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = IPv6_ADDR_LEN;

    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, value, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

}

void
ofl_structs_match_put_ipv6m(struct ofl_match *match, uint32_t header, uint8_t value[IPv6_ADDR_LEN], uint8_t mask[IPv6_ADDR_LEN]){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = IPv6_ADDR_LEN;

    m->header = header;
    m->value = malloc(len*2);
    memcpy(m->value, value, len);
    memcpy(m->value + len, mask, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len*2 + 4;

}

/*Modificacion UAH Discovery hybrid topologies, JAH-*/

void
ofl_structs_match_put_64_as_a_mac(struct ofl_match *match, uint32_t header, uint64_t value){
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint8_t)*6;
    uint64_t aux = value << 16; //desplazamos en dos octectos para dejar los ceros a la derecha
    uint8_t *mac_result = (uint8_t *)&aux;


    m->header = header;
    m->value = malloc(len);
    memcpy(m->value, mac_result, len);
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
}

static inline uint64_t ntoh642(uint64_t n) {
    #if __BYTE_ORDER == __BIG_ENDIAN
        return n;
    #else
        return (((uint64_t)ntohl(n)) << 32) + ntohl(n >> 32);
    #endif
}

void ofl_structs_match_put_macs(struct ofl_match *match, uint32_t header, uint64_t *  value)
{
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint64_t)*EHDDP_MAX_ELEMENTS, index = 0;
    uint64_t dataaux=0;
    
    m->header = header;
    m->value = malloc(len);
    
    for (index=0;index<EHDDP_MAX_ELEMENTS;index++)
    {
        dataaux=ntoh642(value[index]);
        memcpy((m->value+(index*sizeof(uint64_t))),&dataaux,sizeof(uint64_t));
    }
    
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;
    memcpy(&dataaux,m->value,sizeof(uint64_t));
}

void ofl_structs_match_put_port(struct ofl_match *match, uint32_t header, uint32_t *  value)
{
    struct ofl_match_tlv *m = malloc(sizeof (struct ofl_match_tlv));
    int len = sizeof(uint32_t)*EHDDP_MAX_ELEMENTS, index=0;
    uint32_t dataaux=0;

    m->header = header;
    m->value = malloc(len);
    
    for (index=0;index<EHDDP_MAX_ELEMENTS;index++)
    {
        dataaux=ntohl(value[index]);
        memcpy((m->value+(index*sizeof(uint32_t))),&dataaux,sizeof(uint32_t));
    }
    
    hmap_insert(&match->match_fields,&m->hmap_node,hash_int(header, 0));
    match->header.length += len + 4;

    memcpy(&dataaux,m->value,sizeof(uint32_t));
}

/*Fin modificacion UAH MDP Hybrid version, Diego Lopez*/
