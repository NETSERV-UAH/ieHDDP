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

#ifndef RCONN_H
#define RCONN_H 1

#include "queue.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/* A wrapper around vconn that provides queuing and optionally reliability.
 *
 * An rconn maintains a message transmission queue of bounded length specified
 * by the caller.  The rconn does not guarantee reliable delivery of
 * queued messages: all queued messages are dropped when reconnection becomes
 * necessary.
 *
 * An rconn optionally provides reliable communication, in this sense: the
 * rconn will re-connect, with exponential backoff, when the underlying vconn
 * disconnects.
 */

struct vconn;
/* TODO Zoltan: Temporarily removed when moving to OpenFlow 1.1 */
/* struct ofpstat; */

struct rconn *rconn_new(const char *name,
                        int inactivity_probe_interval, int max_backoff);
struct rconn *rconn_new_from_vconn(const char *name, struct vconn *);
struct rconn *rconn_create(int inactivity_probe_interval, int max_backoff);
int rconn_connect(struct rconn *, const char *name);
void rconn_connect_unreliably(struct rconn *,
                              const char *name, struct vconn *vconn);
void rconn_disconnect(struct rconn *);
void rconn_destroy(struct rconn *);

void rconn_run(struct rconn *);
void rconn_run_wait(struct rconn *);
struct ofpbuf *rconn_recv(struct rconn *);
void rconn_recv_wait(struct rconn *);
int rconn_send(struct rconn *, struct ofpbuf *, int *n_queued);
int rconn_send_with_limit(struct rconn *, struct ofpbuf *,
                          int *n_queued, int queue_limit);
unsigned int rconn_packets_sent(const struct rconn *);
unsigned int rconn_packets_received(const struct rconn *);

void rconn_add_monitor(struct rconn *, struct vconn *);

const char *rconn_get_name(const struct rconn *);
bool rconn_is_alive(const struct rconn *);
bool rconn_is_connected(const struct rconn *);
int rconn_failure_duration(const struct rconn *);
bool rconn_is_connectivity_questionable(struct rconn *);

uint32_t rconn_get_ip(const struct rconn *);

const char *rconn_get_state(const struct rconn *);
unsigned int rconn_get_attempted_connections(const struct rconn *);
unsigned int rconn_get_successful_connections(const struct rconn *);
time_t rconn_get_last_connection(const struct rconn *);
time_t rconn_get_creation_time(const struct rconn *);
unsigned long int rconn_get_total_time_connected(const struct rconn *);
int rconn_get_backoff(const struct rconn *);
unsigned int rconn_get_state_elapsed(const struct rconn *);
unsigned int rconn_get_connection_seqno(const struct rconn *);
/* TODO Zoltan: Temporarily removed when moving to OpenFlow 1.1 */
/* void rconn_update_protocol_stat(struct rconn *,
                                struct ofpstat *, struct ofpstat *); */

/*Modificacion UAH*/
const uint8_t rconn_get_state_uint8_t_uah(const struct rconn *);
bool rconn_is_connected_uah(uint8_t status);
/* Fin modificacion*/

#endif /* rconn.h */
