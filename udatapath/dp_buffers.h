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

/* The original Stanford code has been modified during the implementation of
 * the OpenFlow 1.1 userspace switch.
 *
 * Author: Zoltán Lajos Kis <zoltan.lajos.kis@ericsson.com>
 */

#ifndef DP_BUFFERS_H
#define DP_BUFFERS_H 1

#include <stdbool.h>
#include <stdint.h>
#include "ofpbuf.h"


/* Constant for representing "no buffer" */
#define NO_BUFFER 0xffffffff

/****************************************************************************
 * Datapath buffers for storing packets for packet in messages.
 ****************************************************************************/

struct datapath;
struct packet;

/* Creates a set of buffers */
struct dp_buffers *
dp_buffers_create(struct datapath *dp);

/* Returns the number of buffers */
size_t
dp_buffers_size(struct dp_buffers *dpb);

/* Saves the packet into the buffer. Returns the saved buffer ID, or NO_BUFFER
 * if saving was not possible. */
uint32_t
dp_buffers_save(struct dp_buffers *dpb, struct packet *pkt);

/* Retrieves and removes the packet from the buffer. Returns null if it was not
 * found. */
struct packet *
dp_buffers_retrieve(struct dp_buffers *dpb, uint32_t id);

/* Returns true if the buffered packet is not timed out. */
bool
dp_buffers_is_alive(struct dp_buffers *dpb, uint32_t id);

/* Discards the packet in the given buffer, and destroys the packet if destroy is set. */
void
dp_buffers_discard(struct dp_buffers *dpb, uint32_t id, bool destroy);


#endif /* DP_BUFFERS_H */
