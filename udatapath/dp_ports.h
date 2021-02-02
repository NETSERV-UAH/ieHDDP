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

#ifndef DP_PORTS_H
#define DP_PORTS_H 1

#include "list.h"
#include "netdev.h"
#include "dp_exp.h"
#include "oflib/ofl.h"
#include "oflib/ofl-structs.h"
#include "oflib/ofl-messages.h"
#include "oflib-exp/ofl-exp-openflow.h"


/****************************************************************************
 * Datapath port related functions.
 ****************************************************************************/


/* FIXME:  Can declare struct of_hw_driver instead */
#if defined(OF_HW_PLAT)
#include <openflow/of_hw_api.h>
#endif


struct sender;

struct sw_queue {
    struct sw_port *port; /* reference to the parent port */
    uint16_t class_id; /* internal mapping from OF queue_id to tc class_id */
    uint64_t created;
    struct ofl_queue_stats *stats;
    struct ofl_packet_queue *props;
};


#define MAX_HW_NAME_LEN 32
enum sw_port_flags {
    SWP_USED             = 1 << 0,    /* Is port being used */
    SWP_HW_DRV_PORT      = 1 << 1,    /* Port controlled by HW driver */
};
#if defined(OF_HW_PLAT) && !defined(USE_NETDEV)
#define IS_HW_PORT(p) ((p)->flags & SWP_HW_DRV_PORT)
#else
#define IS_HW_PORT(p) 0
#endif

#define PORT_IN_USE(p) (((p) != NULL) && (p)->flags & SWP_USED)

struct sw_port {
    struct list node; /* Element in datapath.ports. */

    uint32_t flags;             /* SWP_* flags above */
    struct datapath *dp;
    struct netdev *netdev;
    struct ofl_port *conf;
    struct ofl_port_stats *stats;
    /* port queues */
    uint16_t max_queues;
    uint16_t num_queues;
    uint64_t created;
    struct sw_queue queues[NETDEV_MAX_QUEUES];
};


#if defined(OF_HW_PLAT)
struct hw_pkt_q_entry {
    struct ofpbuf *buffer;
    struct hw_pkt_q_entry *next;
    of_port_t port_no;
    int reason;
};
#endif

/*Modificacion UAH Discovery hybrid topologies, JAH-*/

struct mac_port_time{
    uint8_t  Mac[ETH_ADDR_LEN];
    uint16_t port_in;
    uint64_t valid_time_entry;
    struct mac_port_time *next;
    uint16_t type; //se usa para identificar el tipo de registro, por ejemplo tipo de sensor
};

struct mac_to_port{
    struct mac_port_time *inicio;
    struct mac_port_time *fin;
    int num_element;
};

//matriz de vecinos
struct mac_to_port bt_table;
uint16_t type_sensor;
uint8_t SENSOR_TO_SENSOR; // 0 = no se permite conexiones entre sensores; 1 = se permite conexiones entre sensores

/*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/

#define DP_MAX_PORTS 255
BUILD_ASSERT_DECL(DP_MAX_PORTS <= OFPP_MAX);



/* Adds a port to the datapath. */
int
dp_ports_add(struct datapath *dp, const char *netdev);

/* Adds a local port to the datapath. */
int
dp_ports_add_local(struct datapath *dp, const char *netdev);

/* Receives datapath packets, and runs them through the pipeline. */
void
dp_ports_run(struct datapath *dp);

/* Returns the given port. */
struct sw_port *
dp_ports_lookup(struct datapath *, uint32_t);

/* Returns the given queue of the given port. */
struct sw_queue *
dp_ports_lookup_queue(struct sw_port *, uint32_t);

/* Outputs a datapath packet on the port. */
void
dp_ports_output(struct datapath *dp, struct ofpbuf *buffer, uint32_t out_port,
              uint32_t queue_id);

/* Outputs a datapath packet on all ports except for in_port. If flood is set,
 * packet is not sent out on ports with flooding disabled. */
int
dp_ports_output_all(struct datapath *dp, struct ofpbuf *buffer, int in_port, bool flood);

/* Handles a port mod message. */
ofl_err
dp_ports_handle_port_mod(struct datapath *dp, struct ofl_msg_port_mod *msg,
                                               const struct sender *sender);

/* Update Live flag on a port/ */
void
dp_port_live_update(struct sw_port *port);

/* Handles a port stats request message. */
ofl_err
dp_ports_handle_stats_request_port(struct datapath *dp,
                                  struct ofl_msg_multipart_request_port *msg,
                                  const struct sender *sender);
                                  
/* Handles a port desc request message. */
ofl_err
dp_ports_handle_port_desc_request(struct datapath *dp,
                                  struct ofl_msg_multipart_request_header *msg UNUSED,
                                  const struct sender *sender UNUSED);

/* Handles a queue stats request message. */
ofl_err
dp_ports_handle_stats_request_queue(struct datapath *dp,
                                  struct ofl_msg_multipart_request_queue *msg,
                                  const struct sender *sender);

/* Handles a queue get config request message. */
ofl_err
dp_ports_handle_queue_get_config_request(struct datapath *dp,
                              struct ofl_msg_queue_get_config_request *msg,
                                                const struct sender *sender);

/* Handles a queue modify (OpenFlow experimenter) message. */
ofl_err
dp_ports_handle_queue_modify(struct datapath *dp, struct ofl_exp_openflow_msg_queue *msg,
        const struct sender *sender);

/* Handles a queue delete (OpenFlow experimenter) message. */
ofl_err
dp_ports_handle_queue_delete(struct datapath *dp, struct ofl_exp_openflow_msg_queue *msg,
        const struct sender *sender);

/*Modificacion UAH Discovery hybrid topologies, JAH-*/
/** Convert hex mac address to uint64_t  @param[in] hwaddr hex mac address
 *  @return mac address as uint64_t */
uint64_t mac2int(const uint8_t hwaddr[]);

/** Convert uint64_t mac address to hex @param[in] mac uint64_t mac address
 * @param[out] hwaddr hex mac address *//*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/
void int2mac(const uint64_t mac, uint8_t *hwaddr);

/** Convierte un littel endian de uint16 en un big endian uint16 */
uint16_t bigtolittle16(uint16_t num);
/** Convierte un littel endian de uint32 en un big endian uint32 */
uint32_t bigtolittle32(uint32_t num);
/** Convierte un littel endian de uint64 en un big endian uint64 */
uint64_t bigtolittle64(uint64_t num);

int num_port_available(struct datapath * dp);

//se crea una nueva tabla mac_to_port en cada switch
void mac_to_port_new(struct mac_to_port *mac_port);
//add generic element
int mac_to_port_add(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN], uint16_t type, uint16_t port_in, int time);
//update element (time and port) 
int mac_to_port_update(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN], uint16_t type, uint16_t port_in, int time);
//refresh time in table
int mac_to_port_time_refresh(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN], uint64_t time);
//found if is posible the out port of the mac
int mac_to_port_found_port(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN]);
//found if is posible the out port of the mac with the position on table
int mac_to_port_found_port_position(struct mac_to_port *mac_port, uint64_t position);
//found if is posible the out port of the mac with the position on table
int mac_to_port_found_mac_position(struct mac_to_port *mac_port, uint64_t position, uint8_t * Mac);
//check de timeout of the mac and port
int mac_to_port_check_timeout(struct mac_to_port *mac_port, uint8_t Mac[ETH_ADDR_LEN]);
//check de timeout of the mac and port
int mac_to_port_delete_timeout(struct mac_to_port *mac_port);
//chect port and delete of table
int mac_to_port_delete_port(struct mac_to_port *mac_port, int port);


/*Debug function */
//show new table
void log_count_request_pks(const void *Mensaje);
void visualizar_tabla(struct mac_to_port *mac_port, int64_t id_datapath);
void log_uah(const void *Mensaje, int64_t id);
/*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/

#endif /* DP_PORTS_H */
