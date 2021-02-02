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

#ifndef OFL_PRINT_H
#define OFL_PRINT_H 1

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>

#include "ofl.h"
#include "ofl-print.h"


/****************************************************************************
 * Functions for printing enum values
 ****************************************************************************/

char *
ofl_port_to_string(uint32_t port);

void
ofl_port_print(FILE *stream, uint32_t port);

char *
ofl_ipv6_ext_hdr_to_string(uint16_t ext_hdr);

void
ofl_ipv6_ext_hdr_print(FILE *stream, uint16_t ext_hdr);

char *
ofl_queue_to_string(uint32_t queue);

void
ofl_queue_print(FILE *stream, uint32_t queue);

char *
ofl_group_to_string(uint32_t group);

void
ofl_group_print(FILE *stream, uint32_t group);

char *
ofl_table_to_string(uint8_t table);

void
ofl_table_print(FILE *stream, uint8_t table);

char *
ofl_vlan_vid_to_string(uint32_t vid);

void
ofl_vlan_vid_print(FILE *stream, uint32_t vid);

char *
ofl_action_type_to_string(uint16_t type);

void
ofl_action_type_print(FILE *stream, uint16_t type);

char *
ofl_oxm_type_to_string(uint16_t type);

void
ofl_oxm_type_print(FILE *stream, uint32_t type);

char *
ofl_instruction_type_to_string(uint16_t type);

void
ofl_instruction_type_print(FILE *stream, uint16_t type);

char *
ofl_queue_prop_type_to_string(uint16_t type);

void
ofl_queue_prop_type_print(FILE *stream, uint16_t type);

char *
ofl_error_type_to_string(uint16_t type);

void
ofl_error_type_print(FILE *stream, uint16_t type);

char *
ofl_error_code_to_string(uint16_t type, uint16_t code);

void
ofl_error_code_print(FILE *stream, uint16_t type, uint16_t code);

char *
ofl_message_type_to_string(uint16_t type);

void
ofl_message_type_print(FILE *stream, uint16_t type);

char *
ofl_buffer_to_string(uint32_t buffer);

void
ofl_buffer_print(FILE *stream, uint32_t buffer);

char *
ofl_packet_in_reason_to_string(uint8_t reason);

void
ofl_packet_in_reason_print(FILE *stream, uint8_t reason);

char *
ofl_flow_removed_reason_to_string(uint8_t reason);

void
ofl_flow_removed_reason_print(FILE *stream, uint8_t reason);

char *
ofl_port_status_reason_to_string(uint8_t reason);

void
ofl_port_status_reason_print(FILE *stream, uint8_t reason);

char *
ofl_flow_mod_command_to_string(uint8_t command);

void
ofl_flow_mod_command_print(FILE *stream, uint8_t command);

char *
ofl_group_mod_command_to_string(uint16_t command);

void
ofl_group_mod_command_print(FILE *stream, uint16_t command);

char *
ofl_meter_mod_command_to_string(uint16_t command);

void
ofl_meter_mod_command_print(FILE *stream, uint16_t command);

char *
ofl_meter_band_type_to_string(uint16_t type); 

void
ofl_meter_band_type_print(FILE *stream, uint16_t type);

char *
ofl_group_type_to_string(uint8_t type);

void
ofl_group_type_print(FILE *stream, uint8_t type);

char *
ofl_stats_type_to_string(uint16_t type);

void
ofl_stats_type_print(FILE *stream, uint16_t type);

void 
ofl_properties_type_print(FILE *stream, uint16_t type);

void
ofl_async_packet_in(FILE *stream, uint32_t packet_in_mask);

void
ofl_async_port_status(FILE *stream, uint32_t port_status_mask);

void
ofl_async_flow_removed(FILE *stream, uint32_t flow_rem_mask);

char *
ofl_hex_to_string(uint8_t *buf, size_t buf_size);

void
ofl_hex_print(FILE *stream, uint8_t *buf, size_t buf_size);

#endif /* OFL_PRINT_H */
