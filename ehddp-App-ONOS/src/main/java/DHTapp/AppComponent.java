/*
 * Copyright 2018-present Open Networking Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package DHTapp;

//import com.sun.xml.internal.ws.api.message.Packet;
import org.apache.felix.scr.annotations.*;
import org.onlab.packet.*;
import org.onosproject.core.ApplicationId;
import org.onosproject.core.CoreService;
import org.onosproject.net.*;
import org.onosproject.net.config.*;
import org.onosproject.net.device.*;
import org.onosproject.net.flow.DefaultTrafficSelector;
import org.onosproject.net.flow.DefaultTrafficTreatment;
import org.onosproject.net.flow.TrafficSelector;
import org.onosproject.net.flow.TrafficTreatment;
import org.onosproject.net.host.HostProvider;
import org.onosproject.net.host.HostProviderService;
import org.onosproject.net.host.HostService;
import org.onosproject.net.link.*;
import org.onosproject.net.packet.*;

import org.onosproject.yang.model.NodeKey;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.lang.reflect.Array;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.function.BiConsumer;
import com.google.common.primitives.UnsignedBytes;

/** Modificar la topologia en ONOS */
import org.onosproject.net.PortNumber;

    /** Librerias para enlaces */
import org.onosproject.net.provider.ProviderId;
import org.onosproject.net.LinkKey;

    /** Librerias para devices */

/** Librerias para Host */
    import org.onosproject.net.host.HostProviderRegistry;

/** Librerias para recursos */
    import static com.google.common.base.Preconditions.checkNotNull;

/**
 * @brief Logica del protocolo de descubrimiento de topologias hibridas UAH
 * @author Joaquin Alvarez Horcajo
 *
 * Skeletal ONOS application component.
 */

@Component(immediate = true)
public class AppComponent{
    /** Tiempo de eliminacion de dispositivos */
    private final int TIME_BLOCK_IN_DEVICE = 6000; //(ms)
    /** Tiempo de refresco */
    private final long TIME_REFRESH = (long)TIME_BLOCK_IN_DEVICE, TIME_DELETE = (long)TIME_BLOCK_IN_DEVICE;
    private long Time_delete = (long)TIME_BLOCK_IN_DEVICE+500;
    /** MAC Propia del protocolo */
    private final String MAC_GENERIC = "AA:BB:CC:DD:EE:FF";
    /** Opction code del protocolo */
    private final byte OPCODE_DHT_REQUEST = 1, OPCODE_DHT_REPLY = 2, OPCODE_DHT_ACK_REQUEST = 3, OPCODE_DHT_RESENT = 4;

    /** @brieg Servicio de Log*/
    private final DHTRecopilationdata datalog = new DHTRecopilationdata();
    private final Logger log = LoggerFactory.getLogger(getClass());

    /** @brief Servicio para interactuar con el inventario de enlaces */
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    private LinkService linkService;

    /** @brief Servicio para iterceptar paquetes recibidos y emitir paquetes de salida */
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    private PacketService packetService;

    /** @brief Servicio para interactuar con el nucleo del sistema del controlador */
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    private CoreService coreService;

    /** @brief Servicio para interactuar con el inventario de dispositivos */
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    private DeviceService deviceService;

    /** @brief Servicio para interactuar con el inventario de dispositivos */
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    private HostService hostService;

    /** @brief Direccion MAC broadcast */
    private final String ETHERNET_BROADCAST_ADDRESS = "FF:FF:FF:FF:FF:FF";

    /** @brief Procesador de paquetes recibidos */
    private ReactivePacketProcessor processor = new ReactivePacketProcessor();

    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    protected NetworkConfigRegistry netCfgService;

    /** @brief Identificador de la aplicacion en Onos*/
    private ApplicationId appId;

    /** @brief Servicio de planificacion de tareas*/
    ScheduledExecutorService scheduledExecutorService = null;

    /** Topologia */
    /** register de los elementos de la red*/
    public static ProviderId PID;
    private LinkProvider linkProvider = new StubLinkProvider();
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    protected LinkProviderRegistry linkProviderRegistry;

    private DeviceProvider deviceProvider = new StubDeviceProvider();
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    protected DeviceProviderRegistry deviceProviderRegistry;

    private HostProvider hostProvider = new StubHostProvider();
    @Reference(cardinality = ReferenceCardinality.MANDATORY_UNARY)
    protected HostProviderRegistry hostProviderRegistry ;


    /** @brief Lista de enlaces de la topologia*/
    protected Set<String> configureNodes = new HashSet<>();
    protected Set<LinkKey> configuredLinks = new HashSet<>();
    protected Set<LinkDescription> ConfigLinksDesciption = new HashSet<>();

    /** Provider de servercios para elementos de red */
    private LinkProviderService linkProviderService;
    private DeviceProviderService deviceProviderService;
    private HostProviderService hostProviderService;

    /**Clase para manejar los enlaces */
    private DHTproviderlink DHTlink = new DHTproviderlink(PID);
    /**Clase para manejar los devices */
    private DHTproviderdevices DHTdevices = new DHTproviderdevices(PID);
    /**Clase para manejar los host */
    private DHTproviderhost DHThost = new DHTproviderhost(PID);

    private String[] TYPE_SENSORS  = new String[] {
            "TEMPERATURE", "WIND", "PRESSURE", "LIGHT", "ACCELEROMETER",
            "VIBRATION", "GIROSCOPE", "PROXIMITY", "DISTANCE", "MOVEMENT", "SMOKE",
            "MAGNETISM", "HUMIDITY"};

    /**Modo de funcionamiento */
    private short response_reply = 0;

    /**Enlaces guardados entre elementos SDN*/
    Map<String, Integer> link_sdn_nodes = new HashMap<String, Integer>();

    /** Marca de Tiempo */
    long timestamp_hddp = 0;

    /** Estadisticos */
    public Integer Num_packet_out = 0, Num_packet_in = 0, Num_packet_data = 0;
    public Integer Num_dev_Non_sdn = 0, Num_dev_sdn = 0;
    public Integer Num_packet_out_general = 0, Num_packet_in_general = 0, Num_packet_data_general = 0;
    public Integer Num_dev_Non_sdn_general = 0, Num_dev_sdn_general = 0;
    long start_process_time_general =0,  end_process_time_general = 0;
    long start_process_time = 0, end_process_time = 0;
    public int num_nodos_file = 0, num_links_file = 0, num_dev_sdn_file = 0;
    int medida_realizada = 0;
    int time_init_inband_realizada = 0;


    /** Random */
    Random randomno = new Random();

    /** @brief Funcion de activacion de la aplicacion. Punto de entrada al programa  */
    @Activate
    protected void activate() {
        try{
            appId = coreService.registerApplication("eHDDP_inband.NetServ.UAH");

            PID = new ProviderId("cfg", "eHDDP_inband.NetServ.UAH", true);

            packetService.addProcessor(processor, PacketProcessor.advisor(2)); //.director(2));

            scheduledExecutorService = Executors.newScheduledThreadPool(1);

            /**Registramos el servicio */
            linkProviderService = linkProviderRegistry.register(linkProvider);
            deviceProviderService = deviceProviderRegistry.register(deviceProvider);
            hostProviderService = hostProviderRegistry.register(hostProvider);

            /** Comprobacion de los registros, no deben ser nulos */
            ConfigProvider(deviceProviderRegistry, linkProviderRegistry, hostProviderRegistry);

            ScheduledFuture scheduledFuture =
                    scheduledExecutorService.schedule(() -> {
                        /** Cargamos la lista de enlaces*/
                        DHTlink.createLinks(netCfgService,configuredLinks);

                        /** Comenzamos el proceso de exploración*/
                        startDHTProcess();

                    },  0,  TimeUnit.SECONDS);
        }catch (Exception e){
            log.error("ERROR DHT !! -----> ALGO HA IDO MAL AL ARRANCAR: "+e.getMessage());
        }

    }

    @Deactivate
    protected void deactivate() {

        linkProviderRegistry.unregister(linkProvider);
        deviceProviderRegistry.unregister(deviceProvider);
        hostProviderRegistry.unregister(hostProvider);

        withdrawIntercepts();
        scheduledExecutorService.shutdownNow();

        log.info("Stopped");
    }

    /** Clases y Funciones auxiliares para realizar el proceso de descubrimiento */

    /** @brief Función que inicia el proceso de exploración del protocolo */
    private void startDHTProcess() {
        /* Recopilacion de datos*/
        num_nodos_file = datalog.num_elements_topolog(1);
        num_links_file = datalog.num_elements_topolog(2);
        num_dev_sdn_file = datalog.num_elements_topolog(3);
        Num_packet_out=0;

        while (true){
            /** Se Genera un array de dispositivos descubierto por SDN*/
            Iterable<Device> devices = deviceService.getAvailableDevices(Device.Type.SWITCH);

            /** Reiniciamos los Estadisticos*/
            Num_packet_out_general = 0;
            Num_packet_in_general = 0;
            Num_packet_data_general = 0;
            Num_dev_sdn_general = 0;
            Num_dev_Non_sdn_general = 0;


            /**Actualizamos marca de tiempo para todos los paquetes*/
            timestamp_hddp = System.currentTimeMillis();
            log.debug("----------------->NUEVA MARCA TEMPORA: "+timestamp_hddp);

            /** Se Recorreo ese array*/
            for(Device device : devices) {
                /** Solo se los mandamos a los OF*/
                if (device.id().toString().contains("of:")){
                    /** Activo la selección de paquetes para mi protocolo **/
                    requestIntercepts();

                    /**Aumentamos el estadistico*/
                    Num_packet_out_general++;

                    /** redescubrimos los enlaces */
                    /** Creamos el paquete inicial para enviar al device seleccionado*/
                    //log.debug("Creamos paquete DHT Request");
                    Ethernet packet = CreatePacketDHT(device.id(), OPCODE_DHT_REQUEST, 255, null,
                            randomno.nextLong(), ETHERNET_BROADCAST_ADDRESS, MAC_GENERIC);
                    //log.debug("Paquete creado correctamente");
                    /** Enviamos el paquete creado */
                    sendpacketwithDevice(device, packet);

                    //log.debug("OK->Paquete enviado correctamente!!!");
                    /** Para depurar esperamos un poco entre lanzamientos */
                    Num_dev_sdn_general++;
                    //insertamos los nodos SDN que conoce ONOS (Sin repetir)
                    if (!DHTdevices.Nodos_SDN.contains(device.id().toString())) {
                        log.info("Inserting device with id " + device.id());
                        DHTdevices.Nodos_SDN.add(device.id().toString());
                        DHTdevices.time_sdn_conection = System.currentTimeMillis();
                    }
                }
                else {
                    Num_dev_Non_sdn_general++;
                }

                /* Inicializa el numero de SDN en eHDDP */
                DHTdevices.set_num_SDN_devices(DHTdevices.Nodos_SDN.size());
                start_process_time_general = System.currentTimeMillis();
            }
            try {
                Thread.sleep(TIME_REFRESH);
                /* Num SDN encontrados por eHDDP*/
                Num_dev_sdn = DHTdevices.Nodos_SDN.size();
                int num_sensores = (deviceService.getAvailableDeviceCount() - Num_dev_sdn - Num_dev_Non_sdn);
                if (num_sensores < 0)
                    num_sensores = 0;

                if ((int)(end_process_time-start_process_time) > 0){

                    datalog.Data_generic(timestamp_hddp, (int) (end_process_time_general - start_process_time_general),
                            Num_packet_out_general, Num_packet_in_general, Num_packet_data_general, Num_dev_sdn, Num_dev_Non_sdn,
                            num_sensores, configuredLinks.size(), num_nodos_file, num_links_file, configureNodes.size(),
                            "datps_onos_inband_completo.txt");
                }
                else{
                    log.info("###############UAH->MEDIDA no válida -> num Num_dev_sdn : {} | num_dev_sdn_file: {} | diff time : {}",
                            Num_dev_sdn, num_dev_sdn_file, (int)(DHTdevices.time_sdn_conection-start_process_time));
                }


                /** Limpiamos los enlaces antigos de este dispositivo*/
                DHTlink.linkVanished(linkProviderService, ConfigLinksDesciption);
                configuredLinks.clear();
                ConfigLinksDesciption.clear();
                link_sdn_nodes.clear();
                Time_delete = System.currentTimeMillis() + TIME_DELETE;
                configureNodes.clear();
                Num_packet_out_general = 0;
                Num_packet_in_general = 0;
                Num_packet_data_general = 0;
                Num_dev_Non_sdn_general = 0;
                Num_dev_sdn_general = 0;
                start_process_time_general = 0;
                end_process_time_general = 0;


            } catch (InterruptedException e) {
                log.error("DHTAPP ERROR :Interrupted exception");
                log.error(e.getMessage());
            }
        }
    }

    /** @brief Clase interna utilizada procesar las notificaciones de paquetes de descubrimiento y confirmacion recibidos **/
    private class ReactivePacketProcessor implements PacketProcessor {

        @Override
        public void process(PacketContext context) {

            /** Obtenemos el paquete In **/
            InboundPacket pkt = context.inPacket();
            Ethernet ethPkt = pkt.parsed();
            DHTpacket Packet_in_dht;
            short opcode=0 , num_hops=0, type_devices[];
            long id_devices[];

            /** Comprobamos si es de nuestro protocolo */
            if (ethPkt == null) {
                log.debug("Null ethernet packet");
                return;
            }

            if(ethPkt.getEtherType() == DHTpacket.DHT_ETHERNET_TYPE) {
                byte[] raw = context.inPacket().parsed().getPayload().serialize();
                try {
                    Packet_in_dht = DHTpacket.deserializer().deserialize(raw, 0, raw.length);
                } catch (DeserializationException e) {
                    log.error("Exception cached while deserializing discovery packet");
                    e.printStackTrace();
                    context.block();
                    return;
                }

                /**Solo nos quedamos con los que tienen destino DHT la MAC controller y son del lanzamiento actual*/
                if (!Packet_in_dht.getMacAnt().toString().equals(MAC_GENERIC) ||
                        Packet_in_dht.getNum_Sec() != timestamp_hddp) {
                    context.block();
                    return;
                }

                opcode = Packet_in_dht.getOpcode();
                num_hops = Packet_in_dht.getNumHops();
                type_devices = Packet_in_dht.getTypedevices();
                id_devices = Packet_in_dht.getidmacdevices();

                /** Aumentamos el Estadistico si todavia no hemos reconocido todos los SDN*/
                /* Num SDN encontrados por eHDDP*/
                if (DHTdevices.Nodos_SDN.size() < num_dev_sdn_file)
                    Num_packet_in++;
                Num_packet_in_general++;
                /** Marca temporal del ultimo paquete tratado*/
                end_process_time_general = System.currentTimeMillis();

                /** Los ack el controlador no los quiere para nada*/
                if (opcode == OPCODE_DHT_ACK_REQUEST){
                    context.block();
                    return;
                }

                /** Si llega un request y es un enlace directo entre dos SDN*/
                if (opcode == OPCODE_DHT_REQUEST){
                    if (num_hops == 1) {
                        /**Si recibimos un Request, debemos contar 1 paquete mas ya que el ultimo enlace no le contariamos*/
                        if (DHTdevices.Nodos_SDN.size() < num_dev_sdn_file)
                            Num_packet_data++;
                        Num_packet_data_general++;

                        /** Comprobamos si existe en enlace contrario en la lista */
                        Integer port_link = link_sdn_nodes.get("of:" + DHTlink.parser_idpacket_to_iddevice(id_devices[0])
                                + " " + context.inPacket().receivedFrom().deviceId().toString());
                        /** Si no existe el enlace todavia lo guardamos y a otra cosa */
                        if (port_link == null) {
                            link_sdn_nodes.put(context.inPacket().receivedFrom().deviceId().toString() + " " +
                                            "of:" + DHTlink.parser_idpacket_to_iddevice(id_devices[0]),
                                    (int) context.inPacket().receivedFrom().port().toLong());
                        } else {
                            DHTlink.linkbewteendevices(deviceService, linkProviderService, configuredLinks,
                                    context.inPacket().receivedFrom().deviceId().toString(),
                                    (int) context.inPacket().receivedFrom().port().toLong(),
                                    "of:" + DHTlink.parser_idpacket_to_iddevice(id_devices[0]),
                                    port_link, true, ConfigLinksDesciption);
                        }
                    }
                    /** Si llega un request y NO es un enlace directo entre dos SDN*/
                    else {
                        /**Aumentamos el estadistico de generales */
                        Num_packet_data_general++;
                        Num_packet_out_general++;

                        /**Sabemos que por cada uno de los enlaces va un dato */
                        Ethernet Reply_packet = CreatePacketDHT(context.inPacket().receivedFrom().deviceId(),
                                OPCODE_DHT_REPLY, (int) context.inPacket().receivedFrom().port().toLong(),
                                deviceService.getPort(context.inPacket().receivedFrom().deviceId(), PortNumber.portNumber(1)),
                                randomno.nextLong(),
                                ETHERNET_BROADCAST_ADDRESS,
                                MAC_GENERIC
                        );
                        //log.debug("Enviamos paquete reply");
                        /** Enviamos el paquete creado utilizando la id del switch */
                        sendPacketwithID(context.inPacket().receivedFrom().deviceId(),
                                context.inPacket().receivedFrom().port(),
                                Reply_packet);
                    }
                }
                /** Si llega un reply */
                else{
                    /**Sabemos que por cada device que salta un paquete es un paquete por la red*/
                    Num_packet_data_general = Num_packet_data_general + num_hops;

                    /**Enviamos ACK para que el proceso de descubrimiento siga,
                     * solo si esta activa la opcion o si viene de request*/
                    /* Desactivamos esta opcion por que estamos en cableado */
                    /*if (response_reply == 1 || num_hops == 1) {
                        send_ack_HDDP_packet(Packet_in_dht,
                                context.inPacket().receivedFrom().deviceId(),
                                (int) context.inPacket().receivedFrom().port().toLong(),
                                context.inPacket().receivedFrom().port()
                        );
                    }*/

                    /** Toca modificar la topologia con los datos obtenidos */
                    /** Si el numero de saltos es 1 Toca modificar la topologia */
                    if (num_hops == 1 && type_devices[0] == DHTdevices.TYPE_SDN){
                        /** SOLUCION PARA EL PUERTO DE SALIDA!!!*/
                        DHTlink.linkbewteendevices(deviceService, linkProviderService, configuredLinks,
                            context.inPacket().receivedFrom().deviceId().toString(),
                            (int)context.inPacket().receivedFrom().port().toLong(),
                            "of:"+ DHTlink.parser_idpacket_to_iddevice(id_devices[0]),
                            Packet_in_dht.getoutports()[0], Packet_in_dht.getbidirectional(0), ConfigLinksDesciption);
                    }else{
                        /** Existen nodos entre ellos */
                        /** Comprobamos que todos los dispositivos y sus puertos esten en la topologia
                         * sino se han de crear */
                        if (!DHTdevices.checkdevices(deviceProviderService, deviceService, deviceProvider,
                                Packet_in_dht, TYPE_SENSORS, configureNodes)){
                            log.error("ALGO FUE MAL EN LA CREACION Y COMPROBACIÖN DE DISPOSITIVOS");
                        }
                        /** Toca hacer los enlaces entre dispostivos */
                        DHTlink.linkstopology(configuredLinks, context.inPacket().receivedFrom().deviceId().toString(),
                                (int)context.inPacket().receivedFrom().port().toLong(),
                                Packet_in_dht, deviceService, linkProviderService, ConfigLinksDesciption);
                     }
                }
                /** Indicamos que el paquete ha sido manejado correctamente
                 * para que el resto de aplicaciones no lo traten */
                context.block();
            }
        }

    }

    /**
     * @brief Funcion encarga de crear el paquete de exploración REQUEST
     *
     * @param deviceId Id del dispositivo
     * @param Opcode código de opcion para los paquetes del protocolo
     * @param port puerto (representa el puerto de salida)
     * @param mac_port class port para obtener la mac del puerto de salida
     */
    private Ethernet CreatePacketDHT(DeviceId deviceId, byte Opcode, int port, Port mac_port, long num_ack,
                                     String mac_dst, String next_hop) {
        Ethernet packet = new Ethernet();
        short Type_devices[] = new short[DHTpacket.DHT_MAX_ELEMENT];
        int  outports[] = new int[DHTpacket.DHT_MAX_ELEMENT], inports[] = new int[DHTpacket.DHT_MAX_ELEMENT];
        long id_mac_devices[] = new long[DHTpacket.DHT_MAX_ELEMENT];
        byte Previous_MAC_Length = 6, Num_devices = 1;
        BitSet Version = BitSet.valueOf(new long[] {0b0000001}), Flags = BitSet.valueOf(new long[] {0b0000000});
        byte configuration [] = new byte [DHTpacket.DHT_MAX_ELEMENT];

        /**Completamos los arrays con los datos del switch elegido */
        /*Nodo SDN*/
        Type_devices[0] = 1;
        /* Id del dispositivo */
        id_mac_devices[0] = Long.parseLong(deviceId.toString().replace("of:",""),16);
        outports[0] = port;
        inports[0] = port;

        /** Inicializamos los valores */
        configuration[0] = (byte) 0b01111111; /*iniciamos con un valor por defecto*/
        for (int pos = 1; pos < DHTpacket.DHT_MAX_ELEMENT; pos++){
            configuration[pos] = (byte)0b00000000;;
        }

        DHTpacket RequestPacket = new DHTpacket(MacAddress.valueOf(next_hop).toBytes(), MacAddress.valueOf(MAC_GENERIC).toBytes(),
                MacAddress.valueOf(MAC_GENERIC).toBytes(), Opcode, Num_devices, TIME_BLOCK_IN_DEVICE, timestamp_hddp, num_ack,
                Type_devices, outports, inports, id_mac_devices, Version, Flags, Previous_MAC_Length, configuration);

        /** Creamos paquete y completamos los datos como pay load*/
        RequestPacket.setParent(packet);

        packet.setSourceMACAddress(MAC_GENERIC)
                .setDestinationMACAddress(mac_dst)
                .setEtherType(RequestPacket.DHT_ETHERNET_TYPE)
                .setPad(true)
                .setPayload(RequestPacket);

        return packet;
    }

    private void send_ack_HDDP_packet(DHTpacket Packet_in_dht, DeviceId deviceId, int port, PortNumber port_number) {
        byte op_code;
        /**Antes de seguir enviamos un ack para qeu el proceso de descubrimiento continue*/
        op_code = OPCODE_DHT_ACK_REQUEST;

        Ethernet ACK_packet = CreatePacketDHT(deviceId, op_code, port,
                deviceService.getPort(deviceId, port_number), Packet_in_dht.getNum_ack(),
                Packet_in_dht.getLastMac().toString(),
                Packet_in_dht.getLastMac().toString()
        );
        log.debug("Enviamos paquete ACK");
        /** no vamos a aumentar este numero ya que en cableado no debe salir */
        /**Aumentamos el estadistico de packet out */
        Num_packet_out_general++;
        /** Enviamos el paquete creado utilizando la id del switch */
        sendPacketwithID(deviceId, port_number, ACK_packet);
    }

    /**
     * @brief Imprime informacion del contexto del packet-in recibido.
     *
     * @param context Contexto del packet-in recibido por el controlador
     * @param Packet_in_dht paquete propio del protocolo
     */
    private void printPacketContextInfo(PacketContext context, DHTpacket Packet_in_dht) {
        Ethernet inEthPacket = context.inPacket().parsed();
        if(inEthPacket.getEtherType() != DHTpacket.DHT_ETHERNET_TYPE)
        {
            log.debug("Unknown");
            return;
        }

        log.debug("DHT packet received. Device: " + context.inPacket().receivedFrom().deviceId()
                + " rcv port: " + context.inPacket().receivedFrom().port()
                + " src MAC: " + inEthPacket.getSourceMAC()
                + " dst MAC: " + inEthPacket.getDestinationMAC()
                + " Packet: " + Packet_in_dht.toString());
    }

    /**
     * @brief Envia paquete de descubrimiento
     *
     * @param device Nodo que envia el paquete
     * @param packet trama Ethernet que encapsula el paquete de descubrimiento
     */
    private void sendpacketwithDevice(Device device, Ethernet packet) {

        TrafficTreatment treatment = DefaultTrafficTreatment.builder()
                .setOutput(PortNumber.FLOOD)
                .build();

        byte[] buffer = packet.serialize();
        OutboundPacket outboundPacket = new DefaultOutboundPacket(device.id(),
                treatment, ByteBuffer.wrap(buffer));

        packetService.emit(outboundPacket);
    }

    /**
     * @brief Enviar Paquete usando el puerto de salida y la id del dispisitivo
     *
     * @param sourceDeviceId Nodo que envia el paquete
     * @param outPort Puerto por donde se reenvia el paquete
     * @param packet trama Ethernet que encapsula el paquete de confirmacion
     */
    private void sendPacketwithID(DeviceId sourceDeviceId, PortNumber outPort, Ethernet packet) {

        TrafficTreatment treatment = DefaultTrafficTreatment.builder()
                .setOutput(outPort)
                .build();

        byte[] buffer = packet.serialize();
        OutboundPacket outboundPacket = new DefaultOutboundPacket(sourceDeviceId,
                treatment, ByteBuffer.wrap(buffer));

        packetService.emit(outboundPacket);
    }


    /**
     * @brief Activa la notificacion de paquetes de descubrimiento y confirmacion recibidos
     */
    private void requestIntercepts() {
        TrafficSelector.Builder selector = DefaultTrafficSelector.builder();

        selector.matchEthType(DHTpacket.DHT_ETHERNET_TYPE);
        packetService.requestPackets(selector.build(), PacketPriority.REACTIVE, appId);
    }

    /**
     * @brief Desactiva la notificacion de paquetes de descubrimiento y confirmacion recibidos
     */
    private void withdrawIntercepts() {
        packetService.removeProcessor(processor);
    }

    /**
     * Creates a new configuration provider.
     *
     * @param deviceProviderRegistry device provider registry
     * @param linkProviderRegistry   link provider registry
     * @param hostProviderRegistry   host provider registry
     */
    private void ConfigProvider(
                   DeviceProviderRegistry deviceProviderRegistry,
                   LinkProviderRegistry linkProviderRegistry,
                   HostProviderRegistry hostProviderRegistry) {
        this.deviceProviderRegistry = checkNotNull(deviceProviderRegistry, "Device provider registry cannot be null");
        this.linkProviderRegistry = checkNotNull(linkProviderRegistry, "Link provider registry cannot be null");
        this.hostProviderRegistry = checkNotNull(hostProviderRegistry, "Host provider registry cannot be null");
    }


    private static boolean isremoveable(String unixTime, long Time_delete_device) {
        String time ;

        time = unixTime.replace("connected ","");
        time = time.replace("disconnected ","");
        time = time.replace("ago","");

        // si ya dias u horas o minutos se elimina directamente
        if ( time.contains("d") || unixTime.contains("h") || unixTime.contains("m")){
            return true;
        }
        else if (time.contains("2")){
            if (Long.getLong(time.split("2")[0]) > Time_delete_device)
                return true;
        }
        return false;
    }

    // Stub provider used to get LinkProviderService
    private static final class StubLinkProvider implements LinkProvider {
        @Override
        public ProviderId id() {
            return PID;
        }
    }

    private static final class StubDeviceProvider implements DeviceProvider {
        @Override
        public ProviderId id() {
            return PID;
        }

        @Override
        public void triggerProbe(DeviceId deviceId) {

        }

        @Override
        public void roleChanged(DeviceId deviceId, MastershipRole mastershipRole) {

        }

        @Override
        public boolean isReachable(DeviceId deviceId) {
            return false;
        }

        @Override
        public void changePortState(DeviceId deviceId, PortNumber portNumber, boolean b) {

        }
    }

    private static final class StubHostProvider implements HostProvider{

        @Override
        public void triggerProbe(Host host) {

        }

        @Override
        public ProviderId id() {
            return PID;
        }
    }
}
