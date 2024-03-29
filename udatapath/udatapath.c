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

#include <config.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "command-line.h"
#include "daemon.h"
#include "datapath.h"
#include "fault.h"
#include "openflow/openflow.h"
#include "poll-loop.h"
#include "queue.h"
#include "util.h"
#include "rconn.h"
#include "timeval.h"
#include "vconn.h"
#include "dirs.h"
#include "vconn-ssl.h"
#include "vlog-socket.h"

#if defined(OF_HW_PLAT)
#include <openflow/of_hw_api.h>
#endif

#define THIS_MODULE VLM_udatapath
#include "vlog.h"

/*Modificacion UAH Discovery hybrid topologies, JAH- */
#include <time.h>
#include <sys/time.h>

#define TIME_DELETE_BT 5 //tiempo de borrado de tabla de vecinos

/* FIn Modificacion UAH Discovery hybrid topologies, JAH- */

int udatapath_cmd(int argc, char *argv[]);

static void parse_options(struct datapath *dp, int argc, char *argv[]);
static void usage(void) NO_RETURN;

static struct datapath *dp;

static char *port_list;
static char *local_port = "tap:";

static void add_ports(struct datapath *dp, char *port_list);

static bool use_multiple_connections = false;

/*Modificacion UAH Discovery hybrid topologies, JAH-*/
extern struct packet *pkt_hello;
extern struct mac_to_port bt_table, learning_table;
static char * ip_aux_init = NULL;


uint8_t old_local_port_MAC[ETH_ADDR_LEN]; //Almacena la antigua MAC del puerto que se configura como local para poder volver a asignarsela en caso de que cambie el puerto local.
bool local_port_ok = false;
struct in_addr ip_de_control_in_band;
struct in_addr ip_del_controlller;
uint64_t time_init_local_port = 0;
uint64_t time_init_cicle = 0;
uint8_t conection_status_ofp_controller = 0;
uint64_t time_connect_to_contoller = 0;
uint64_t time_exploration = 0;
uint32_t old_local_port = 0;
uint64_t time_no_move_local_port = 0;
uint64_t convergence_time = 0;
uint64_t time_start = 0;
uint8_t log_escrito = 0;
uint64_t timestamp_connecto_to_controller = 0;
uint64_t packet_time = 0;
int num_pkt_ehddp_req = 0;
int num_pkt_ehddp_rep = 0;
int num_pkt_arp_rep = 0;
int num_pkt_arp_req = 0;
uint64_t time_to_connect = 0;
bool Reply_ON = false;
uint8_t type_device_general;
int port_to_controller = 0;
bool controller_connected = false;
bool port_to_controller_configurated = false;
/*FIN Modificacion UAH Discovery hybrid topologies, JAH-*/

/* Need to treat this more generically */
#if defined(UDATAPATH_AS_LIB)
#define OFP_FATAL(_er, _str, args...) do {                \
        fprintf(stderr, _str, ## args);                   \
        return -1;                                        \
    } while (0)
#else
#define OFP_FATAL(_er, _str, args...) ofp_fatal(_er, _str, ## args)
#endif

#if !defined(UDATAPATH_AS_LIB)
int
main(int argc, char *argv[])
{
    return udatapath_cmd(argc, argv);
    
}
#endif

int
udatapath_cmd(int argc, char *argv[])
{
    int n_listeners;
    int error;
    int i;
    /*Modificacion UAH eHDDP, JAH-*/
    uint64_t deletetimeBT = 0; //, time_init_uah = 0;
    /*Fin Modificacion eHDDP, JAH-*/

    set_program_name(argv[0]);
    register_fault_handlers();
    time_init();
    vlog_init();

    dp = dp_new();

    parse_options(dp, argc, argv);
    signal(SIGPIPE, SIG_IGN);

    if (argc - optind < 1) {
        OFP_FATAL(0, "at least one listener argument is required; "
          "use --help for usage");
    }

    if (use_multiple_connections && (argc - optind) % 2 != 0)
        OFP_FATAL(0, "when using multiple connections, you must specify an even number of listeners");
        
    n_listeners = 0;
    for (i = optind; i < argc; i += 2) {
        const char *pvconn_name = argv[i];
        const char *pvconn_name_aux = NULL;
        struct pvconn *pvconn, *pvconn_aux = NULL;
        int retval, retval_aux;

        if (use_multiple_connections)
            pvconn_name_aux = argv[i + 1];

        retval = pvconn_open(pvconn_name, &pvconn);
        if (!retval || retval == EAGAIN) {
            // Get another listener if we are using auxiliary connections
            if (use_multiple_connections) {
                retval_aux = pvconn_open(pvconn_name_aux, &pvconn_aux);
                if (retval_aux && retval_aux != EAGAIN) {
                    ofp_error(retval_aux, "opening auxiliary %s", pvconn_name_aux);
                    pvconn_aux = NULL;
                }
            }
            dp_add_pvconn(dp, pvconn, pvconn_aux);
            n_listeners++;
        } else {
            ofp_error(retval, "opening %s", pvconn_name);
        }
    }
    if (n_listeners == 0) {
        OFP_FATAL(0, "could not listen for any connections");
    }

    if (port_list != NULL) {
        add_ports(dp, port_list);
    }
    if (local_port != NULL) {
        error = dp_ports_add_local(dp, local_port);
        if (error) {
            OFP_FATAL(error, "failed to add local port %s", local_port);
        }
    }

    error = vlog_server_listen(NULL, NULL);
    if (error) {
        OFP_FATAL(error, "could not listen for vlog connections");
    }

    die_if_already_running();
    daemonize();

    /*if (get_dp_local_port_number_UAH(dp)) 
    {
        //iniciamos el temporaizados
        time_init_uah = time_msec();
    }*/
    
    time_init_local_port = 0;

    /*Fin Modificacion UAH eHDDP, JAH-*/
    for (;;) {
        dp_run(dp);
        dp_wait(dp);
        poll_block();
        
        /*Modificacion UAH Discovery hybrid topologies, JAH-*/
        if((time_msec() - deletetimeBT >= TIME_DELETE_BT*1000) && dp->id != 1)
        {
            mac_to_port_delete_timeout(&bt_table);
            mac_to_port_delete_timeout_ehddp(&bt_table);
            //visualizar_tabla(&bt_table, dp->id);
            //visualizar_tabla(&learning_table, dp->id);
            deletetimeBT = time_msec();
        }
        /*Fin Modificacion UAH Discovery hybrid topologies, JAH-*/
    }

    return 0;
}

static void
add_ports(struct datapath *dp, char *port_list)
{
    char *port, *save_ptr;

    /* Glibc 2.7 has a bug in strtok_r when compiling with optimization that
     * can cause segfaults here:
     * http://sources.redhat.com/bugzilla/show_bug.cgi?id=5614.
     * Using ",," instead of the obvious "," works around it. */
    for (port = strtok_r(port_list, ",,", &save_ptr); port;
         port = strtok_r(NULL, ",,", &save_ptr)) {
        int error = dp_ports_add(dp, port);
        if (error) {
            ofp_fatal(error, "failed to add port %s", port);
        }
    }
}

static void
parse_options(struct datapath *dp, int argc, char *argv[])
{
    enum {
        OPT_MFR_DESC = UCHAR_MAX + 1,
        OPT_HW_DESC,
        OPT_SW_DESC,
        OPT_DP_DESC,
        OPT_SERIAL_NUM,
        OPT_BOOTSTRAP_CA_CERT,
        OPT_NO_LOCAL_PORT,
        OPT_NO_SLICING
    };

    static struct option long_options[] = {
        {"interfaces",  required_argument, 0, 'i'},
        {"local-port",  required_argument, 0, 'L'},
        {"no-local-port", no_argument, 0, OPT_NO_LOCAL_PORT},
        {"datapath-id", required_argument, 0, 'd'},
        {"multiconn",     no_argument, 0, 'm'},
        {"verbose",     optional_argument, 0, 'v'},
        {"help",        no_argument, 0, 'h'},
        {"version",     no_argument, 0, 'V'},
        {"ip-inband",  required_argument, 0, 'I'}, //Modificación UAH
        {"type-device", required_argument, 0, 'T'}, //Modificación UAH
        {"ip-controller", required_argument, 0, 'C'}, //Modificación UAH
        {"no-slicing",  no_argument, 0, OPT_NO_SLICING},
        {"mfr-desc",    required_argument, 0, OPT_MFR_DESC},
        {"hw-desc",     required_argument, 0, OPT_HW_DESC},
        {"sw-desc",     required_argument, 0, OPT_SW_DESC},
        {"dp_desc",  required_argument, 0, OPT_DP_DESC},
        {"serial_num",  required_argument, 0, OPT_SERIAL_NUM},
        DAEMON_LONG_OPTIONS,
#ifdef HAVE_OPENSSL
        VCONN_SSL_LONG_OPTIONS
        {"bootstrap-ca-cert", required_argument, 0, OPT_BOOTSTRAP_CA_CERT},
#endif
        {0, 0, 0, 0},
    };
    char *short_options = long_options_to_short_options(long_options);

    for (;;) {
        int indexptr;
        int c;

        c = getopt_long(argc, argv, short_options, long_options, &indexptr);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'd': {
            uint64_t dpid;
            if (strlen(optarg) != 12
                || strspn(optarg, "0123456789abcdefABCDEF") != 12) {
                ofp_fatal(0, "argument to -d or --datapath-id must be "
                          "exactly 16 hex digits");
            }
            dpid = strtoll(optarg, NULL, 10);
            if (!dpid) {
                ofp_fatal(0, "argument to -d or --datapath-id must "
                          "be nonzero");
            }
            VLOG_INFO(THIS_MODULE,"DPID : %lu", dpid);
            dp_set_dpid(dp, dpid);
            break;
        }
        
        case 'm': {
            use_multiple_connections = true;
            break;
        }
        
        case 'h':
            usage();

        case 'V':
            printf("%s %s compiled "__DATE__" "__TIME__"\n",
                   program_name, VERSION BUILDNR);
            exit(EXIT_SUCCESS);

        case 'v':
            vlog_set_verbosity(optarg);
            break;

        case 'i':
            if (!port_list) {
                port_list = optarg;
            } else {
                port_list = xasprintf("%s,%s", port_list, optarg);
            }
            break;

        case 'L':
            local_port = optarg;
            port_to_controller_configurated = true;
            VLOG_INFO(THIS_MODULE,"Puerto Local: %s", local_port);
            break;

        /*Modificación UAH*/
        case 'I':
            ip_aux_init = optarg;//quitar_espacios(optarg); 
            VLOG_INFO(THIS_MODULE,"IP de control: %s", ip_aux_init);
            ip_de_control_in_band.s_addr = dp_set_ip_addr(ip_aux_init);
            inet_ntop(AF_INET, &ip_de_control_in_band, ip_aux_init, INET_ADDRSTRLEN);
            if (port_to_controller_configurated == false)
                local_port = NULL;
            break;
        
        case 'T':
            type_device_general = atoi(optarg);
            VLOG_INFO(THIS_MODULE,"Tipo de Dispostivio: %u", type_device_general);
            break;
        
        case 'C':
            ip_aux_init = optarg;//quitar_espacios(optarg); 
            VLOG_INFO(THIS_MODULE,"IP del controller: %s", ip_aux_init);
            ip_del_controlller.s_addr = dp_set_ip_addr(ip_aux_init);
            inet_ntop(AF_INET, &ip_del_controlller, ip_aux_init, INET_ADDRSTRLEN);
            if (port_to_controller_configurated == false)
                local_port = NULL;
            break;
        /*Fin modificación*/

        case OPT_NO_LOCAL_PORT:
            local_port = NULL;
            break;

        case OPT_MFR_DESC:
            dp_set_mfr_desc(dp, optarg);
            break;

        case OPT_HW_DESC:
            dp_set_hw_desc(dp, optarg);
            break;

        case OPT_SW_DESC:
            dp_set_sw_desc(dp, optarg);
            break;

        case OPT_DP_DESC:
            dp_set_dp_desc(dp, optarg);
            break;

        case OPT_SERIAL_NUM:
            dp_set_serial_num(dp, optarg);
            break;

        case OPT_NO_SLICING:
            dp_set_max_queues(dp, 0);
            break;

        DAEMON_OPTION_HANDLERS

#ifdef HAVE_OPENSSL
        VCONN_SSL_OPTION_HANDLERS

        case OPT_BOOTSTRAP_CA_CERT:
            vconn_ssl_set_ca_cert_file(optarg, true);
            break;
#endif

        case '?':
            exit(EXIT_FAILURE);

        default:
            abort();
        }
    }
    free(short_options);
}

static void
usage(void)
{
    printf("%s: userspace OpenFlow datapath\n"
           "usage: %s [OPTIONS] LISTEN...\n"
           "where LISTEN is a passive OpenFlow connection method on which\n"
       "to listen for incoming connections from the secure channel.\n",
           program_name, program_name);
    vconn_usage(false, true, false);
    printf("\nConfiguration options:\n"
           "  -i, --interfaces=NETDEV[,NETDEV]...\n"
           "                          add specified initial switch ports\n"
           "  -L, --local-port=NETDEV set network device for local port\n"
           "  --no-local-port         disable local port\n"
           "  -d, --datapath-id=ID    Use ID as the OpenFlow switch ID\n"
           "                          (ID must consist of 12 hex digits)\n"
           "  -m, --multiconn         enable multiple connections to the\n"
           "                          same controller.\n"
           "  --no-slicing            disable slicing\n"
           "\nOther options:\n"
           "  -D, --detach            run in background as daemon\n"
           "  -P, --pidfile[=FILE]    create pidfile (default: %s/ofdatapath.pid)\n"
           "  -f, --force             with -P, start even if already running\n"
           "  -v, --verbose=MODULE[:FACILITY[:LEVEL]]  set logging levels\n"
           "  -v, --verbose           set maximum verbosity level\n"
           "  -h, --help              display this help message\n"
           "  -V, --version           display version information\n"
           "  -I, --ip-inband         the ip for the in band interface with format XXX.XXX.XXX.XXX\n /*Modificacion UAH*/", 
        ofp_rundir);
    exit(EXIT_SUCCESS);
}
