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

#ifndef IN_BAND_H
#define IN_BAND_H 1
/*Modificaciones UAH*/
#include "stdint.h"

/*+++FIN+++*/

struct port_watcher;
struct rconn;
struct secchan;
struct settings;
struct switch_status;
struct in_band_data;
struct in_addr;

void in_band_start(struct secchan *, const struct settings *,
                   struct switch_status *, struct port_watcher *,
                   struct rconn *remote);

//Modificaciones UAH//

void install_in_band_rules_UAH(struct rconn *local_rconn, struct in_band_data *in_band, uint16_t of_port);
void install_new_localport_rules_UAH(struct rconn *local_rconn, uint32_t *new_local_port, struct in_addr *local_ip, struct in_addr *controller_ip, uint8_t *mac, uint32_t *old_local_port);

//++++++//

#endif /* in-band.h */
