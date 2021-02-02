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
#ifndef OFL_PACKETS_H
#define OFL_PACKETS_H

#define ETH_TYPE_II_START      0x0600
#define ETH_TYPE_IP            0x0800
#define ETH_TYPE_ARP           0x0806
#define ETH_TYPE_VLAN          0x8100
#define ETH_TYPE_VLAN_PBB      0x88a8
#define ETH_TYPE_PBB           0x88e7
#define ETH_TYPE_MPLS          0x8847
#define ETH_TYPE_MPLS_MCAST    0x8848

#define PBB_ISID_LEN		   3
#define ETH_ADDR_LEN           6
#define IPv6_ADDR_LEN	       16


#define VLAN_VID_MASK 0x0fff
#define VLAN_VID_SHIFT 0
#define VLAN_PCP_MASK 0xe000
#define VLAN_PCP_SHIFT 13
#define VLAN_PCP_BITMASK 0x0007 /* the least 3-bit is valid */

#define VLAN_VID_MAX 4095
#define VLAN_PCP_MAX 7



#define MPLS_TTL_MASK 0x000000ff
#define MPLS_TTL_SHIFT 0
#define MPLS_S_MASK 0x00000100
#define MPLS_S_SHIFT 8
#define MPLS_TC_MASK 0x00000e00
#define MPLS_TC_SHIFT 9
#define MPLS_LABEL_MASK 0xfffff000
#define MPLS_LABEL_SHIFT 12

#define MPLS_LABEL_MAX   1048575
#define MPLS_TC_MAX            7



#define IP_ECN_MASK 0x03
#define IP_DSCP_MASK 0xfc

/*Modificacion UAH Discovery hybrid topologies, JAH-*/

#define ETH_TYPE_EHDDP 0xFFAA //JAH-UAH
#define ETH_TYPE_EHDDP_INV 0xAAFF //JAH-UAH Valores al reves para procesar el pipeline (Little Endian)

#endif /* OFL_PACKETS_H */
