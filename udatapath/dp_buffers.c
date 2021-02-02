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

#include <stdbool.h>
#include <stdint.h>

#include "dp_buffers.h"
#include "timeval.h"
#include "packet.h"
#include "vlog.h"

#define LOG_MODULE VLM_dp_buf

static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(60, 60);


/* Buffers are identified by a 31-bit opaque ID.  We divide the ID
 * into a buffer number (low bits) and a cookie (high bits).  The buffer number
 * is an index into an array of buffers.  The cookie distinguishes between
 * different packets that have occupied a single buffer.  Thus, the more
 * buffers we have, the lower-quality the cookie... */
#define PKT_BUFFER_BITS 8
#define PKT_COOKIE_BITS (32 - PKT_BUFFER_BITS)

#define N_PKT_BUFFERS (1 << PKT_BUFFER_BITS)
#define PKT_BUFFER_MASK (N_PKT_BUFFERS - 1)


#define OVERWRITE_SECS  1

struct packet_buffer {
    struct packet *pkt;
    uint32_t       cookie;
    time_t         timeout;
};


// NOTE: The current implementation assumes that a packet is only saved once
//       to the buffers. Thus, if two entities save it, and one retrieves it,
//       the other will receive an invalid buffer response.
//       In the current implementation this should not happen.

struct dp_buffers {
    struct datapath       *dp;
    size_t                 buffer_idx;
    size_t                 buffers_num;
    struct packet_buffer   buffers[N_PKT_BUFFERS];
};


struct dp_buffers *
dp_buffers_create(struct datapath *dp) {
    struct dp_buffers *dpb = xmalloc(sizeof(struct dp_buffers));
    size_t i;

    dpb->dp          = dp;
    dpb->buffer_idx  = (size_t)-1;
    dpb->buffers_num = N_PKT_BUFFERS;

    for (i=0; i<N_PKT_BUFFERS; i++) {
        dpb->buffers[i].pkt     = NULL;
        dpb->buffers[i].cookie  = UINT32_MAX;
        dpb->buffers[i].timeout = 0;
    }

    return dpb;
}

size_t
dp_buffers_size(struct dp_buffers *dpb) {
    return dpb->buffers_num;
}

uint32_t
dp_buffers_save(struct dp_buffers *dpb, struct packet *pkt) {
    struct packet_buffer *p;
    uint32_t id;

    /* if packet is already in buffer, do not save again */
    if (pkt->buffer_id != NO_BUFFER) {
        if (dp_buffers_is_alive(dpb, pkt->buffer_id)) {
            return pkt->buffer_id;
        }
    }

    dpb->buffer_idx = (dpb->buffer_idx + 1) & PKT_BUFFER_MASK;

    p = &dpb->buffers[dpb->buffer_idx];
    if (p->pkt != NULL) {
        if (time_now() < p->timeout) {
            return NO_BUFFER;
        } else {
            p->pkt->buffer_id = NO_BUFFER;
            packet_destroy(p->pkt);
        }
    }
    /* Don't use maximum cookie value since the all-bits-1 id is
     * special. */
    if (++p->cookie >= (1u << PKT_COOKIE_BITS) - 1)
        p->cookie = 0;
    p->pkt = pkt;
    p->timeout = time_now() + OVERWRITE_SECS;
    id = dpb->buffer_idx | (p->cookie << PKT_BUFFER_BITS);

    pkt->buffer_id  = id;

    return id;
}

struct packet *
dp_buffers_retrieve(struct dp_buffers *dpb, uint32_t id) {
    struct packet *pkt = NULL;
    struct packet_buffer *p;

    p = &dpb->buffers[id & PKT_BUFFER_MASK];
    if (p->cookie == id >> PKT_BUFFER_BITS && p->pkt != NULL) {
        pkt = p->pkt;
        pkt->buffer_id = NO_BUFFER;
        pkt->packet_out = false;

        p->pkt = NULL;
    } else {
        VLOG_WARN_RL(LOG_MODULE, &rl, "cookie mismatch: %x != %x\n",
                          id >> PKT_BUFFER_BITS, p->cookie);
    }

    return pkt;
}

bool
dp_buffers_is_alive(struct dp_buffers *dpb, uint32_t id) {
    struct packet_buffer *p;

    p = &dpb->buffers[id & PKT_BUFFER_MASK];
    return ((p->cookie == id >> PKT_BUFFER_BITS) &&
            (time_now() < p->timeout));
}


void
dp_buffers_discard(struct dp_buffers *dpb, uint32_t id, bool destroy) {
    struct packet_buffer *p;

    p = &dpb->buffers[id & PKT_BUFFER_MASK];

    if (p->cookie == id >> PKT_BUFFER_BITS) {
        if (destroy) {
            p->pkt->buffer_id = NO_BUFFER;
            packet_destroy(p->pkt);
        }
        p->pkt = NULL;
    }
}
