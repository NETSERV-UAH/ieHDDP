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

#ifndef DP_CAPABILITIES_H
#define DP_CAPABILITIES_H 1


#include "openflow/openflow.h"


/****************************************************************************
 * Datapath capabilities.
 ****************************************************************************/

#define DP_SUPPORTED_CAPABILITIES ( OFPC_FLOW_STATS        \
                               | OFPC_TABLE_STATS          \
                               | OFPC_PORT_STATS           \
                               | OFPC_GROUP_STATS          \
                            /* | OFPC_IP_REASM       */    \
                               | OFPC_QUEUE_STATS          )
                               /*| OFPC_PORT_BLOCKED */   
    
#define DP_SUPPORTED_ACTIONS ( (1 << OFPAT_OUTPUT)          \
                             | (2 << OFPAT_COPY_TTL_OUT)    \
                             | (3 << OFPAT_COPY_TTL_IN)     \
                             | (4 << OFPAT_SET_MPLS_TTL)    \
                             | (5 << OFPAT_DEC_MPLS_TTL)    \
                             | (6 << OFPAT_PUSH_VLAN)       \
                             | (7 << OFPAT_POP_VLAN)        \
                             | (8 << OFPAT_PUSH_MPLS)       \
                             | (9 << OFPAT_POP_MPLS)        \
                             | (10 << OFPAT_SET_QUEUE)       \
                             | (11 << OFPAT_GROUP)           \
                             | (12 << OFPAT_SET_NW_TTL)      \
                             | (13 << OFPAT_DEC_NW_TTL) )
                           
#define DP_SUPPORTED_MATCH_FIELDS ( OFPXMT_OFB_IN_PORT        \
                                  | OFPXMT_OFB_IN_PHY_PORT    \
                                  | OFPXMT_OFB_METADATA       \
                                  | OFPXMT_OFB_ETH_DST        \
                                  | OFPXMT_OFB_ETH_SRC        \
                                  | OFPXMT_OFB_ETH_TYPE       \
                                  | OFPXMT_OFB_VLAN_VID       \
                                  | OFPXMT_OFB_VLAN_PCP       \
                                  | OFPXMT_OFB_IP_DSCP        \
                                  | OFPXMT_OFB_IP_ECN         \
                                  | OFPXMT_OFB_IP_PROTO       \
                                  | OFPXMT_OFB_IPV4_SRC       \
                                  | OFPXMT_OFB_IPV4_DST       \
                                  | OFPXMT_OFB_TCP_SRC        \
                                  | OFPXMT_OFB_TCP_DST        \
                                  | OFPXMT_OFB_UDP_SRC        \
                                  | OFPXMT_OFB_UDP_DST        \
                                  | OFPXMT_OFB_SCTP_SRC       \
                                  | OFPXMT_OFB_SCTP_DST       \
                                  | OFPXMT_OFB_ICMPV4_CODE    \
                                  | OFPXMT_OFB_ICMPV4_TYPE    \
                                  | OFPXMT_OFB_ARP_OP         \
                                  | OFPXMT_OFB_ARP_SHA        \
                                  | OFPXMT_OFB_ARP_SPA        \
                                  | OFPXMT_OFB_ARP_THA        \
                                  | OFPXMT_OFB_ARP_TPA        \
                                  | OFPXMT_OFB_IPV6_SRC       \
                                  | OFPXMT_OFB_IPV6_DST       \
                                  | OFPXMT_OFB_IPV6_FLABEL    \
                                  | OFPXMT_OFB_ICMPV6_CODE    \
                                  | OFPXMT_OFB_ICMPV6_TYPE    \
                                  | OFPXMT_OFB_IPV6_ND_SLL    \
                                  | OFPXMT_OFB_IPV6_ND_TARGET \
                                  | OFPXMT_OFB_IPV6_ND_TLL    \
                                  | OFPXMT_OFB_MPLS_LABEL     \
                                  | OFPXMT_OFB_MPLS_TC         )
                                
#define DP_SUPPORTED_GROUPS ( OFPGT_ALL      \
							| OFPGT_SELECT   \
							| OFPGT_INDIRECT \
							| OFPGT_FF)

#define DP_SUPPORTED_GROUP_CAPABILITIES ( OFPGFC_SELECT_WEIGHT      \
							            /*| OFPGFC_SELECT_LIVENESS    \
							            | OFPGFC_CHAINING           \
							            | OFPGFC_CHAINING_CHECKS*/)
#endif /* DP_CAPABILITIES_H */
