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

#ifndef SECCHAN_H
#define SECCHAN_H 1

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include "list.h"
#include "packets.h"

struct secchan;

/* Maximum number of management connection listeners. */
#define MAX_MGMT 8

#define MAX_CONTROLLERS 3

/* Settings that may be configured by the user. */
struct settings {
    /* Overall mode of operation. */
    bool discovery;           /* Discover the controller automatically? */
    bool in_band;             /* Connect to controller in-band? */

    /* Related vconns and network devices. */
    const char *dp_name;        /* Local datapath. */
    int num_controllers;        /* Number of configured controllers. */
    const char *controller_names[MAX_CONTROLLERS]; /* Controllers (if not discovery mode). */
    const char *listener_names[MAX_MGMT]; /* Listen for mgmt connections. */
    size_t n_listeners;          /* Number of mgmt connection listeners. */
    const char *monitor_name;   /* Listen for traffic monitor connections. */

    /* Failure behavior. */
    int max_idle;             /* Idle time for flows in fail-open mode. */
    int probe_interval;       /* # seconds idle before sending echo request. */
    int max_backoff;          /* Max # seconds between connection attempts. */

    /* Packet-in rate-limiting. */
    int rate_limit;           /* Tokens added to bucket per second. */
    int burst_limit;          /* Maximum number token bucket size. */

    /* Discovery behavior. */
    regex_t accept_controller_regex;  /* Controller vconns to accept. */
    const char *accept_controller_re; /* String version of regex. */
    bool update_resolv_conf;          /* Update /etc/resolv.conf? */

    /* Spanning tree protocol. */
    bool enable_stp;
};

struct half {
    struct rconn *rconn;
    struct ofpbuf *rxbuf;
    int n_txq;                  /* No. of packets queued for tx on 'rconn'. */
};

struct relay {
    struct list node;

#define HALF_LOCAL 0
#define HALF_REMOTE 1
    struct half halves[2];

    /* The secchan has a primary connection (relay) to an OpenFlow controller.
     * This primary connection actually makes two connections to the datapath:
     * one for OpenFlow requests and responses, and one that is only used for
     * receiving asynchronous events such as 'ofp_packet_in' events.  This
     * design keeps replies to OpenFlow requests from being dropped by the
     * kernel due to a flooded network device.
     *
     * The secchan may also have any number of secondary "management"
     * connections (relays).  These connections do not receive asychronous
     * events and thus have a null 'async_rconn'. */
    bool is_mgmt_conn;          /* Is this a management connection? */
    struct rconn *async_rconn;  /* For receiving asynchronous events. */
};

struct hook_class {
    bool (*local_packet_cb)(struct relay *, void *aux);
    bool (*remote_packet_cb)(struct relay *, void *aux);
    void (*periodic_cb)(void *aux);
    void (*wait_cb)(void *aux);
    void (*closing_cb)(struct relay *, void *aux);
};

void add_hook(struct secchan *, const struct hook_class *, void *);

struct ofp_packet_in *get_ofp_packet_in(struct relay *);
bool get_ofp_packet_eth_header(struct relay *, struct ofp_packet_in **,struct eth_header **);

//**Modificaciones UAH**//
struct ofl_msg_packet_in *get_ofl_packet_in_UAH(struct relay *, uint32_t *, struct ofpbuf **);
bool get_ofp_packet_eth_header_UAH(struct relay *, struct ofl_msg_packet_in **, struct eth_header **, uint32_t *, struct ofpbuf **);
uint16_t get_of_port_UAH(const char *);
//++++FIN+++++//

#endif /* secchan.h */
