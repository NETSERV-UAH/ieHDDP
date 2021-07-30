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

#ifndef PORT_WATCHER_H
#define PORT_WATCHER_H 1

#include <stdint.h>
#include "compiler.h"
#include "secchan.h"

struct ofp_port;
struct port_watcher;
struct secchan;

void port_watcher_start(struct secchan *,
                        struct rconn *local, struct rconn *remote,
                        struct port_watcher **);
bool port_watcher_is_ready(const struct port_watcher *);
uint32_t port_watcher_get_config(const struct port_watcher *,
                                 uint32_t port_no);
const char *port_watcher_get_name(const struct port_watcher *,
                                  uint32_t port_no) UNUSED;
const uint8_t *port_watcher_get_hwaddr(const struct port_watcher *,
                                       uint32_t port_no);
void port_watcher_set_flags(struct port_watcher *, uint32_t port_no,
                            uint32_t config, uint32_t c_mask,
                            uint32_t state, uint32_t s_mask);

typedef void port_changed_cb_func(uint32_t port_no,
                                  const struct ofp_port *old,
                                  const struct ofp_port *new,
                                  void *aux);

void port_watcher_register_callback(struct port_watcher *,
                                    port_changed_cb_func *port_changed,
                                    void *aux);

typedef void local_port_changed_cb_func(const struct ofp_port *new,
                                        void *aux);

void port_watcher_register_local_port_callback(struct port_watcher *pw,
                                               local_port_changed_cb_func *cb,
                                               void *aux);

void get_port_name(const struct ofp_port *, char *name, size_t name_size);

//Modificaciones UAH//
uint32_t get_pw_local_port_number_UAH(struct port_watcher *pw);
bool got_features_reply_state_UAH(struct port_watcher *pw);
void update_port_watcher_ports_UAH(struct port_watcher *pw);
void get_pw_name(struct port_watcher *pw, char * name);
struct ofp_port * get_ofp_port_local_UAH(struct port_watcher *pw, uint32_t port_no);
uint64_t get_datapath_id (struct port_watcher *pw);
struct rconn *get_remote_rconn(struct port_watcher *pw);
struct rconn *get_local_rconn(struct port_watcher *pw);
//+++FIN+++//

#endif /* port-watcher.h */
