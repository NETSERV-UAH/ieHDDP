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

#include "compiler.h"
#include "dp_capabilities.h"
#include "dp_control.h"
#include "dp_actions.h"
#include "dp_buffers.h"
#include "dp_ports.h"
#include "group_table.h"
#include "meter_table.h"
#include "packets.h"
#include "pipeline.h"
#include "oflib/ofl.h"
#include "oflib/ofl-messages.h"
#include "oflib/ofl-log.h"
#include "openflow/openflow.h"

#include "vlog.h"
#define LOG_MODULE VLM_dp_ctrl

static struct vlog_rate_limit rl = VLOG_RATE_LIMIT_INIT(60, 60);

/* Handles barrier request messages. */
static ofl_err
handle_control_barrier_request(struct datapath *dp,
           struct ofl_msg_header *msg, const struct sender *sender) {

    /* Note: the implementation is single-threaded,
       so a barrier request can simply be replied. */
    struct ofl_msg_header reply =
            {.type = OFPT_BARRIER_REPLY};

    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);
    ofl_msg_free(msg, dp->exp);

    return 0;
}

/* Handles features request messages. */
static ofl_err
handle_control_features_request(struct datapath *dp,
          struct ofl_msg_header *msg, const struct sender *sender) {

    struct ofl_msg_features_reply reply =
            {{.type = OFPT_FEATURES_REPLY},
             .datapath_id  = dp->id,
             .n_buffers    = dp_buffers_size(dp->buffers),
             .n_tables     = PIPELINE_TABLES,
             .auxiliary_id = sender->conn_id,
             .capabilities = DP_SUPPORTED_CAPABILITIES,
             .reserved = 0x00000000};

    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    ofl_msg_free(msg, dp->exp);

    return 0;
}


/* Handles get config request messages. */
static ofl_err
handle_control_get_config_request(struct datapath *dp,
        struct ofl_msg_header *msg, const struct sender *sender) {

    struct ofl_msg_get_config_reply reply =
            {{.type = OFPT_GET_CONFIG_REPLY},
             .config = &dp->config};
    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    ofl_msg_free(msg, dp->exp);
    return 0;
}

/* Handles set config request messages. */
static ofl_err
handle_control_set_config(struct datapath *dp, struct ofl_msg_set_config *msg,
                                                const struct sender *sender UNUSED) {
    uint16_t flags;

    flags = msg->config->flags & OFPC_FRAG_MASK;
    if ((flags & OFPC_FRAG_MASK) != OFPC_FRAG_NORMAL
        && (flags & OFPC_FRAG_MASK) != OFPC_FRAG_DROP) {
        flags = (flags & ~OFPC_FRAG_MASK) | OFPC_FRAG_DROP;
    }

    dp->config.flags = flags;
    dp->config.miss_send_len = msg->config->miss_send_len;

    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
    return 0;
}

static inline uint64_t hton64_UAH(uint64_t n) {
#if __BYTE_ORDER == __BIG_ENDIAN
    return n;
#else
    return (((uint64_t)htonl(n)) << 32) + htonl(n >> 32);
#endif
}

/* Handles packet out messages. */
static ofl_err
handle_control_packet_out(struct datapath *dp, struct ofl_msg_packet_out *msg,
                                                const struct sender *sender) {
    struct packet *pkt;
    int error;
    if(sender->remote->role == OFPCR_ROLE_SLAVE){
        return ofl_error(OFPET_BAD_REQUEST, OFPBRC_IS_SLAVE);
    }
    
    /*modificación UAH*/
    if ((uint8_t)msg->data_length == 1){ //si la longitud del mensaje es de 1 solo caracter es nuestro fijo 
        conection_status_ofp_controller = (uint8_t)msg->data[0];
        ofl_msg_free_packet_out(msg, false, dp->exp);   
        //VLOG_WARN(LOG_MODULE, "conection_status_ofp_controller >> %d <<", conection_status_ofp_controller);
        //mod_local_port_change_connection_uah(dp);
        //VLOG_WARN(LOG_MODULE, "handle_control_packet_out ->  mod_local_port_change_connection_uah -> OK ");
        return 0;
    }
    if (msg->data_length == sizeof(uint64_t)) //si tenemos un mensaje del tamaño de un uint64_t es nuestra marca temporal
    {
        memcpy(&time_connect_to_contoller, msg->data, sizeof(uint64_t));
        controller_connected = true;
        ofl_msg_free_packet_out(msg, false, dp->exp);
        VLOG_WARN(LOG_MODULE, "Instante de Conexion al controlador >> %lu <<", time_connect_to_contoller);
        if (dp->id > 1)
            log_uah_estadisticos(dp);
        //if (Reply_ON)
        //    send_reply_to_controller(dp);
        return 0;
    }
    /*fin modificación UAH*/

    error = dp_actions_validate(dp, msg->actions_num, msg->actions);
    if (error) {
        return error;
    }

    if (msg->buffer_id == NO_BUFFER) {
        struct ofpbuf *buf;
        /* If there is no packet in the message, send error message */ 
        if (!msg->data_length){
             return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_PACKET);
        }
        /* NOTE: the created packet will take the ownership of data in msg. */
        buf = ofpbuf_new(0);
        ofpbuf_use(buf, msg->data, msg->data_length);
        ofpbuf_put_uninit(buf, msg->data_length);        
        pkt = packet_create(dp, msg->in_port, buf, true);        
    } else {
        /* NOTE: in this case packet should not have data */
        pkt = dp_buffers_retrieve(dp->buffers, msg->buffer_id);
    }

    if (pkt == NULL) {
        /* This might be a wrong req., or a timed out buffer */
        return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BUFFER_EMPTY);
    }

    //Si detectamos que es un eHDDP packet (el primero marcamos el instante de llegada)
    if (log_escrito == 0 && dp->id == 1 && 
        (pkt->handle_std->proto->eth->eth_type == ETH_TYPE_EHDDP || pkt->handle_std->proto->eth->eth_type == ETH_TYPE_EHDDP_INV)){
        //time_start = time_msec();
        time_start = current_timestamp();
        log_uah_estadisticos(pkt->dp);
        VLOG_WARN(LOG_MODULE, "He escrito el fichero del dp->id = 1 >> %lu <<", time_start);
        log_escrito = 1;
    }

    dp_execute_action_list(pkt, msg->actions_num, msg->actions, 0xffffffffffffffff);

    packet_destroy(pkt);
    ofl_msg_free_packet_out(msg, false, dp->exp);    
    return 0;
}


/* Handles desc stats request messages. */
static ofl_err
handle_control_stats_request_desc(struct datapath *dp,
                                  struct ofl_msg_multipart_request_header *msg,
                                  const struct sender *sender) {
    struct ofl_msg_reply_desc reply =
            {{{.type = OFPT_MULTIPART_REPLY},
              .type = OFPMP_DESC, .flags = 0x0000},
              .mfr_desc   = dp->mfr_desc,
              .hw_desc    = dp->hw_desc,
              .sw_desc    = dp->sw_desc,
              .serial_num = dp->serial_num,
              .dp_desc    = dp->dp_desc};
    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
    return 0;
}

/* Dispatches statistic request messages to the appropriate handler functions. */
static ofl_err
handle_control_stats_request(struct datapath *dp,
                                  struct ofl_msg_multipart_request_header *msg,
                                                const struct sender *sender) {
    switch (msg->type) {
        case (OFPMP_DESC): {
            return handle_control_stats_request_desc(dp, msg, sender);
        }
        case (OFPMP_FLOW): {
            return pipeline_handle_stats_request_flow(dp->pipeline, (struct ofl_msg_multipart_request_flow *)msg, sender);
        }
        case (OFPMP_AGGREGATE): {
            return pipeline_handle_stats_request_aggregate(dp->pipeline, (struct ofl_msg_multipart_request_flow *)msg, sender);
        }
        case (OFPMP_TABLE): {
            return pipeline_handle_stats_request_table(dp->pipeline, msg, sender);
        }
        case (OFPMP_TABLE_FEATURES):{
            return pipeline_handle_stats_request_table_features_request(dp->pipeline, msg, sender);
        }
        case (OFPMP_PORT_STATS): {
            return dp_ports_handle_stats_request_port(dp, (struct ofl_msg_multipart_request_port *)msg, sender);
        }
        case (OFPMP_QUEUE): {
            return dp_ports_handle_stats_request_queue(dp, (struct ofl_msg_multipart_request_queue *)msg, sender);
        }
        case (OFPMP_GROUP): {
            return group_table_handle_stats_request_group(dp->groups, (struct ofl_msg_multipart_request_group *)msg, sender);
        }
        case (OFPMP_GROUP_DESC): {
            return group_table_handle_stats_request_group_desc(dp->groups, msg, sender);
        }
		case (OFPMP_GROUP_FEATURES):{
            return group_table_handle_stats_request_group_features(dp->groups, msg, sender);			
		}		
        case (OFPMP_METER):{
        	return meter_table_handle_stats_request_meter(dp->meters,(struct ofl_msg_multipart_meter_request*)msg, sender);
        }
        case (OFPMP_METER_CONFIG):{
            return meter_table_handle_stats_request_meter_conf(dp->meters,(struct ofl_msg_multipart_meter_request*)msg, sender);        
        }
        case OFPMP_METER_FEATURES:{
            return meter_table_handle_features_request(dp->meters, msg, sender);
        }
        case OFPMP_PORT_DESC:{
            return dp_ports_handle_port_desc_request(dp, msg, sender);        
        }
        case (OFPMP_EXPERIMENTER): {
            return dp_exp_stats(dp, (struct ofl_msg_multipart_request_experimenter *)msg, sender);
        }
        default: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_MULTIPART);
        }
    }
}


/* Handles echo reply messages. */
static ofl_err
handle_control_echo_reply(struct datapath *dp UNUSED,
                                struct ofl_msg_echo *msg,
                                  const struct sender *sender UNUSED) {

    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
    return 0;
}

/* Handles echo request messages. */
static ofl_err
handle_control_echo_request(struct datapath *dp,
                                          struct ofl_msg_echo *msg,
                                                const struct sender *sender) {
    struct ofl_msg_echo reply =
            {{.type = OFPT_ECHO_REPLY},
             .data_length = msg->data_length,
             .data        = msg->data};
    dp_send_message(dp, (struct ofl_msg_header *)&reply, sender);

    ofl_msg_free((struct ofl_msg_header *)msg, dp->exp);
    return 0;
}

/* Dispatches control messages to appropriate handler functions. */
ofl_err
handle_control_msg(struct datapath *dp, struct ofl_msg_header *msg,
                   const struct sender *sender) {

    if (VLOG_IS_DBG_ENABLED(LOG_MODULE)) {
        char *msg_str = ofl_msg_to_string(msg, dp->exp);
        VLOG_DBG_RL(LOG_MODULE, &rl, "received control msg: %.400s", msg_str);
        free(msg_str);
    }

    /* NOTE: It is assumed that if a handler returns with error, it did not use
             any part of the control message, thus it can be freed up.
             If no error is returned however, the message must be freed inside
             the handler (because the handler might keep parts of the message) */
    switch (msg->type) {
        case OFPT_HELLO: {
            ofl_msg_free(msg, dp->exp);
            return 0;
        }
        case OFPT_ERROR: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_BARRIER_REQUEST: {
            return handle_control_barrier_request(dp, msg, sender);
        }
        case OFPT_BARRIER_REPLY: {
            ofl_msg_free(msg, dp->exp);
            return 0;
        }
        case OFPT_FEATURES_REQUEST: {
            return handle_control_features_request(dp, msg, sender);
        }
        case OFPT_FEATURES_REPLY: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_GET_CONFIG_REQUEST: {
            return handle_control_get_config_request(dp, msg, sender);
        }
        case OFPT_GET_CONFIG_REPLY: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_SET_CONFIG: {
            return handle_control_set_config(dp, (struct ofl_msg_set_config *)msg, sender);
        }
        case OFPT_PACKET_IN: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_PACKET_OUT: {
            return handle_control_packet_out(dp, (struct ofl_msg_packet_out *)msg, sender);
            break;
        }
        case OFPT_FLOW_REMOVED: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_PORT_STATUS: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_FLOW_MOD: {
            return pipeline_handle_flow_mod(dp->pipeline, (struct ofl_msg_flow_mod *)msg, sender);
        }
        case OFPT_GROUP_MOD: {
            return group_table_handle_group_mod(dp->groups, (struct ofl_msg_group_mod *)msg, sender);
        }
        case OFPT_PORT_MOD: {
            return dp_ports_handle_port_mod(dp, (struct ofl_msg_port_mod *)msg, sender);
        }
        case OFPT_TABLE_MOD: {
            return pipeline_handle_table_mod(dp->pipeline, (struct ofl_msg_table_mod *)msg, sender);
        }
        case OFPT_MULTIPART_REQUEST: {
            return handle_control_stats_request(dp, (struct ofl_msg_multipart_request_header *)msg, sender);
        }
        case OFPT_MULTIPART_REPLY: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_ECHO_REQUEST: {
            return handle_control_echo_request(dp, (struct ofl_msg_echo *)msg, sender);
        }
        case OFPT_ECHO_REPLY: {
            return handle_control_echo_reply(dp, (struct ofl_msg_echo *)msg, sender);
        }
        case OFPT_QUEUE_GET_CONFIG_REQUEST: {
            return dp_ports_handle_queue_get_config_request(dp, (struct ofl_msg_queue_get_config_request *)msg, sender);
        }
        case OFPT_ROLE_REQUEST: {
            return dp_handle_role_request(dp, (struct ofl_msg_role_request*)msg, sender);
        }
        case OFPT_ROLE_REPLY:{
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_QUEUE_GET_CONFIG_REPLY: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_METER_MOD:{
			return meter_table_handle_meter_mod(dp->meters, (struct ofl_msg_meter_mod *)msg, sender);
		}
        case OFPT_EXPERIMENTER: {
            return dp_exp_message(dp, (struct ofl_msg_experimenter *)msg, sender);
        }
        case OFPT_GET_ASYNC_REPLY:{
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
        case OFPT_GET_ASYNC_REQUEST:
        case OFPT_SET_ASYNC:{
            return dp_handle_async_request(dp, (struct ofl_msg_async_config*)msg, sender);
        }
        default: {
            return ofl_error(OFPET_BAD_REQUEST, OFPBRC_BAD_TYPE);
        }
    }
}

void mod_local_port_change_connection_uah(struct datapath * dp){
    uint8_t mac[ETH_ADDR_LEN];

    VLOG_WARN(LOG_MODULE, "mod_local_port_change_connection_uah >> %d <<", bt_table.num_element);
    if (dp != NULL){
        if (dp->local_port != NULL && (conection_status_ofp_controller & (4 | 8 | 16)) == 0){
            //borramos el puerto local actual y la posición en la tabla
            if (bt_table.num_element > 0){
                mac_to_port_delete_position(&bt_table, 1);
            }
            if (bt_table.num_element == 0){ //no tenemos mas intentos pues borramos y esperamos
                //Si existia un puerto local anterior le utilizamos para hacer el cambio              
                if (dp->local_port)
                    free(dp->local_port);
                local_port_ok = false;
                old_local_port = 0;
                VLOG_WARN(LOG_MODULE, "Puerto Local Borrado");
            }
            else{
                //Si existia un puerto local anterior le utilizamos para hacer el cambio
                VLOG_WARN(LOG_MODULE, "Si existia un puerto local anterior le utilizamos para hacer el cambio");
                memcpy(mac, netdev_get_etheraddr(dp->local_port->netdev), ETH_ADDR_LEN);
                if (configure_new_local_port_ehddp_UAH(dp, mac, old_local_port, 0) == 1){
                    local_port_ok = false;
                    old_local_port = dp->local_port->conf->port_no; //guardamos el puerto anterior
                }
                else
                {
                    free(dp->local_port);
                    local_port_ok = false;
                    old_local_port = 0;
                    //VLOG_WARN(LOG_MODULE, "Algo no ha ido bien con la configuración del nuevo puerto local, se borra para evitar problemas");
                }

            }
            
            //Ahora hacemos que el ofprotocol se entere
            //dp_ports_handle_port_mod_UAH(pkt->dp, p->conf->port_no);         
        }

    }
}

void log_uah_estadisticos(struct datapath * dp){
    FILE * file;
	char nombre[100], Mensaje[1000];

	VLOG_DBG_RL(LOG_MODULE, &rl, "Traza UAH -> Entro a Crear estadisticos");
	sprintf(nombre,"/tmp/Estadisticos_switch.log");
    file=fopen(nombre,"a");

    sprintf(Mensaje,"%d\t%lu\t%lu\t%d\t%d\t%d\t%d\t%lu\t%lu\n ",(int)dp->id, time_connect_to_contoller, convergence_time, 
        num_pkt_ehddp_req, num_pkt_ehddp_rep, num_pkt_arp_req, num_pkt_arp_rep, time_exploration, time_start);

	if(file != NULL)
	{
        fseek(file, 0L, SEEK_END);
        file = fopen( nombre , "a" );
		fputs(Mensaje, file);
		fclose(file);
    }
}
