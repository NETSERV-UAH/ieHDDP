package org.onosproject.eHDDP;


import org.onosproject.net.*;
import org.onosproject.net.config.NetworkConfigRegistry;
import org.onosproject.net.config.basics.BasicLinkConfig;
import org.onosproject.net.device.DeviceService;
import org.onosproject.net.link.DefaultLinkDescription;
import org.onosproject.net.link.LinkDescription;
import org.onosproject.net.link.LinkProvider;
import org.onosproject.net.link.LinkProviderService;
import org.onosproject.net.provider.ProviderId;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.time.Duration;
import java.util.Arrays;
import java.util.Set;

import static org.onosproject.net.PortNumber.portNumber;

/**
 * @brief Provider de enlaces para eHDDP
 * @author Joaquin Alvarez Horcajo
 */

public class eHDDPproviderlink implements LinkProvider{

    /*!< @brief Servicio para interactuar con el inventario de dispositivos */
    //private DeviceService deviceService;
    //private LinkService linkService;

    /** Atributes */
    public static ProviderId PID;
    /** @brieg Servicio de Log*/
    private final Logger log = LoggerFactory.getLogger(getClass());
    private static final long DEF_BANDWIDTH = -1L;
    private static final double DEF_METRIC = -1;
    private static final Duration DEF_DURATION = Duration.ofNanos(-1L);

    /** Methods */

    /** @brief Creates a new element of eHDDPprovicerLink class.
     *
     */
    public eHDDPproviderlink(ProviderId pid){

        PID = pid;
    }

    public void linkVanished(LinkProviderService linkProviderService, Set<LinkDescription> ConfigLinksDesciption){
        for (LinkDescription linkkeydesc: ConfigLinksDesciption){
            linkProviderService.linkVanished(linkkeydesc);
        }

    }

    public void linkVanished(String srcname, String dstname, LinkProviderService linkProviderService) {

        BasicLinkConfig link_config;

        String srcId = srcname.split("/")[0];
        String srcport = srcname.split("/")[1];

        String dstId = dstname.split("/")[0];
        String dstport = dstname.split("/")[1];

        /** Datos sobre los extremos del enlace */
        ConnectPoint src = new ConnectPoint(DeviceId.deviceId(srcId),portNumber(srcport));
        ConnectPoint dst = new ConnectPoint(DeviceId.deviceId(dstId), portNumber(dstport));

        LinkKey linkKey1 = LinkKey.linkKey(src, dst);

        /**Si estamos en modo 1 y uno de los dos enlaces es un sensor, entonces el enlace no es durable */
        if ((srcId.contains("sw") || srcId.contains("of")) && (dstId.contains("sw") || dstId.contains("of"))){
            /** Le comunicaciones que es bidireccional y no es durable */
            link_config = new BasicLinkConfig(linkKey1).isBidirectional(true).isDurable(true);
        }
        else{
            /** Si por el contrario estamos en modo 0 o se trata de un enlace entre nodos, si podemos decir que es durable */
            link_config = new BasicLinkConfig(linkKey1).isBidirectional(true).isDurable(false);
        }

        LinkDescription ld1 = new DefaultLinkDescription(linkKey1.src(), linkKey1.dst(), Link.Type.DIRECT, insertannotation(link_config));

        linkProviderService.linkVanished(ld1);
    }

    /** @briefSignals that an infrastructure link has disappeared.
     *
     * @param linkDescription
     * @param linkProviderService
     */
    public void linkVanished(LinkDescription linkDescription, LinkProviderService linkProviderService) {
        linkProviderService.linkVanished(linkDescription);
    }

    /** @briefSignals that infrastructure links associated with the specified connect point have vanished.
     *
     * @param connectPoint
     * @param linkProviderService
     */
    public void linksVanished(ConnectPoint connectPoint, LinkProviderService linkProviderService) {
        linkProviderService.linksVanished(connectPoint);
    }

    /** @brief Signals that infrastructure links associated with the specified  device have vanished.
     *
     * @param deviceId
     * @param linkProviderService
     */
    public void linksVanished(DeviceId deviceId, LinkProviderService linkProviderService) {
        linkProviderService.linksVanished(deviceId);
    }

    /** @brief Recopila los enlaces configurados
     *
     * @param netCfgService NetworkConfigRegistry
     * @param configuredLinks conjunto de enlaces (LinkKey)
     */
    public void createLinks(NetworkConfigRegistry netCfgService, Set<LinkKey> configuredLinks) {
        netCfgService.getSubjects(LinkKey.class).forEach(LinkKey -> configuredLinks.add(LinkKey));
    }

    /** @brief Crea un enlace entre dos dispositivos SDN
     *
     * @param configuredLinks conjunto de enlaces (LinkKey)
     * @param srcDpId identificador string del switch origen del enlace
     * @param srcport identificador int del puerto origen del enlace
     * @param dstDpId identificador string del switch destino del enlace
     * @param dstport identificador int del puerto destino del enlace
     * @param linkProviderService LinkProviderService
     * @return true Si todo es correcto, false en caso contrario
     */
    public boolean linkbewteendevices( LinkProviderService linkProviderService,
                                      Set<LinkKey> configuredLinks, String srcDpId , int srcport, String dstDpId,
                                      int dstport, boolean bidirectional, Set<LinkDescription> ConfigLinksDesciption) {

        BasicLinkConfig link_config_scr_to_dst= null, link_config_dst_to_scr = null;

        /*Los enlaces con los host no se hacen asi, se les indica como localización*/
        if (srcDpId.contains("sn") || dstDpId.contains("sn"))
            return true;

        /** Datos sobre el origen del enlace */
        log.info("UAH->linktopology->Creamos DeviceId de: {}", srcDpId);
        DeviceId srcDeviceId = DeviceId.deviceId(srcDpId);
        PortNumber srcPortNumber = portNumber(srcport);
        ConnectPoint src = new ConnectPoint(srcDeviceId, srcPortNumber);

        /** Datos sobre el destino del enlace */
        log.info("UAH->linktopology->Creamos DeviceId de: {}", dstDpId);
        DeviceId dstDeviceId = DeviceId.deviceId(dstDpId);
        PortNumber dstPortNumber = portNumber(dstport);
        ConnectPoint dst = new ConnectPoint(dstDeviceId, dstPortNumber);

        /** Enlaces en un sentido y el otro */
        LinkKey linkKey1 = LinkKey.linkKey(src, dst);
        LinkKey linkKey2 = LinkKey.linkKey(dst, src);

        /*if (configuredLinks.contains(linkKey1) && configuredLinks.contains(linkKey2)) {
            log.debug("----- UAH -> Detectado ya creado ");
            return true;
        }*/

        /**Si uno de los dos enlaces es un sensor, entonces el enlace no es durable */
        //if ( !(srcDpId.contains("sw") || srcDpId.contains("of")) || !(dstDpId.contains("sw") || dstDpId.contains("of"))){
        if ((srcDpId.contains("sw") || srcDpId.contains("of")) && (dstDpId.contains("sw") || dstDpId.contains("of"))){
            /** Si se trata de un enlace entre nodos, si podemos decir que es durable */
            link_config_scr_to_dst = new BasicLinkConfig(linkKey1).isBidirectional(true).isDurable(true);
            link_config_dst_to_scr = new BasicLinkConfig(linkKey2).isBidirectional(true).isDurable(true);
        }
        else{
            /** Le comunicaciones que es bidireccional y no es durable */
            if (configuredLinks.contains(linkKey1))
                configuredLinks.remove(linkKey1);
            if (configuredLinks.contains(linkKey2))
                configuredLinks.remove(linkKey2);

            link_config_scr_to_dst = new BasicLinkConfig(linkKey1).isDurable(false);
            link_config_dst_to_scr = new BasicLinkConfig(linkKey2).isDurable(false);

            if (bidirectional == true) {
                link_config_scr_to_dst.isBidirectional(true);
                link_config_dst_to_scr.isBidirectional(true);
            }
            else
            {
                link_config_scr_to_dst.isBidirectional(false);
                link_config_dst_to_scr.isBidirectional(false);
            }
        }

        /** Genero el linkkey entre ambos equipos */
        try{
            if (!configuredLinks.contains(linkKey1)) {
                insert_linkkey(linkProviderService, ConfigLinksDesciption, configuredLinks, link_config_scr_to_dst, linkKey1);
                if (bidirectional == true || (srcDpId.contains("of") && dstDpId.contains("of"))){
                    insert_linkkey(linkProviderService, ConfigLinksDesciption, configuredLinks, link_config_dst_to_scr, linkKey2);
                    log.info("UAH->linktopology->Enlace creado entre: {} y {}", linkKey1, linkKey2);
                }
                else
                    log.info("UAH->linktopology->Enlace creado entre: {}", linkKey1);
            }
            else
                log.debug("UAH->linktopology->Enlace NO creado entre: {} debido a que ya existe", linkKey1);
        }catch (Exception e){
            log.error("ERROR: ALGO NO VA BIEN CON LA CREACION DE LOS ENLACES!!!");
            log.error("toString(): "  + e.toString());
            log.error("getMessage(): " + e.getMessage());
        }
        return true;
    }

    private void insert_linkkey(LinkProviderService linkProviderService, Set<LinkDescription> ConfigLinksDesciption,
                                Set<LinkKey> configuredLinks, BasicLinkConfig link_config, LinkKey linkKey) {
        SparseAnnotations sa = insertannotation(link_config);
        LinkDescription ld = new DefaultLinkDescription(linkKey.src(), linkKey.dst(), Link.Type.DIRECT, sa);
        if (!ConfigLinksDesciption.contains(ld)) {
            linkProviderService.linkDetected(ld);
            configuredLinks.add(linkKey);
            ConfigLinksDesciption.add(ld);
        }
    }

    /** @brief Genera los enlaces definidos por el protocolo en la topologia de onos
     *
     * @param configuredLinks conjunto de enlaces (LinkKey)
     * @param srcDpIdpacketin string del switch origen del enlace
     * @param srcportpacketin identificador int del puerto origen del enlace
     * @param Packet_in_eHDDP class eHDDPpacket
     * @param deviceService DeviceService
     * @param linkProviderService LinkProviderService
     * @return true si todo ok, false en caso contrario
     */
    public boolean linkstopology(Set<LinkKey> configuredLinks, String srcDpIdpacketin , int srcportpacketin,
                                 eHDDPpacket Packet_in_eHDDP, DeviceService deviceService,
                                 LinkProviderService linkProviderService, Set<LinkDescription> ConfigLinksDesciption)
    {
        String dstDpid[] = new String[2];
        short types_divices[] = Packet_in_eHDDP.getTypedevices();
        int in_ports[]= Packet_in_eHDDP.getinports();
        int out_ports[]= Packet_in_eHDDP.getoutports();
        long Id_Devices[]= Packet_in_eHDDP.getidmacdevices();
        boolean bidirectional = false;
        /* ampliamos el espacio en uno para los puertos */
        in_ports = Arrays.copyOf(in_ports, in_ports.length + 1);
        out_ports = Arrays.copyOf(out_ports, out_ports.length + 1);

        /** El primer elemento es especial ya que se conecta con el SDN */
        for (int num = 0; num < Packet_in_eHDDP.getNumHops(); num ++){
            /* Obtenemos el valor bidireccional del enlace */
            bidirectional = Packet_in_eHDDP.getbidirectional(num);
            /** Estamos en el caso de conexión con el Switch SDN */
            switch(types_divices[num])
            {
                case 1: /* SDN case */
                case 3: /* Configure SDN Proces case */
                    dstDpid[0] = "of:"+parser_idpacket_to_iddevice(Id_Devices[num]);
                    bidirectional = false;
                    break;
                case 2: /* NO SDN Case */
                    dstDpid[0] = "sw:"+parser_idpacket_to_iddevice(Id_Devices[num]);
                    break;
                default: /* Sensor case */
                    dstDpid[0] = parser_idpacket_to_iddevice(Id_Devices[num]);
                    break;
            }

            /** Si estamos en el ultimo elemento lo unimos con el sdn device que ha enviado el packet in */
            if (num + 1 == Packet_in_eHDDP.getNumHops()){
                dstDpid[1] = srcDpIdpacketin;
                in_ports[num+1] = srcportpacketin;
            }
            else {
                switch (types_divices[num + 1]) {
                    case 1: /* NO SDN case */
                    case 3: /* Configure SDN Proces case */
                        dstDpid[1] = "of:" + parser_idpacket_to_iddevice(Id_Devices[num + 1]);
                        break;
                    case 2: /* NO SDN Case */
                        dstDpid[1] = "sw:" + parser_idpacket_to_iddevice(Id_Devices[num + 1]);
                        break;
                    default: /* Sensor case */
                        dstDpid[1] = parser_idpacket_to_iddevice(Id_Devices[num + 1]);
                        break;
                }
            }

            try{
                if (out_ports[num] != 0 && out_ports[num] != 255  && in_ports[num + 1] != 0 && in_ports[num + 1] != 255
                        && dstDpid[0] != dstDpid[1]) {
                    linkbewteendevices(linkProviderService, configuredLinks, dstDpid[0],
                            out_ports[num], dstDpid[1], in_ports[num + 1], bidirectional,
                            ConfigLinksDesciption);
                }
            } catch (Exception e) {
                log.error("eHDDPapp ERROR al crear nuevos Enlaces entre devices genericos : " + e.getMessage());
                return false;
            }

        }
        return true;
    }

    /**
     * @brief Parsea la id recibida en el paquete a una valida para el sistema *
     * @param dpid Id del dispositivo (long)
     */
    public String parser_idpacket_to_iddevice(long dpid) {
        String DpidString = Long.toHexString(dpid);
        int len = DpidString.length();

        for(int aux = len; aux<16; aux++){
            DpidString="0"+ DpidString;
        }
        return DpidString;
    }


    /**
     * Generates an annotation from an existing annotation and LinkConfig.
     *
     * @param cfg the link config entity from network config
     * @return annotation combining both sources
     */

    public static DefaultAnnotations insertannotation(BasicLinkConfig cfg) {
        DefaultAnnotations.Builder annotations = DefaultAnnotations.builder();

        if (cfg.metric() != DEF_METRIC) {
            annotations.set(AnnotationKeys.METRIC, String.valueOf(cfg.metric()));
        }
        if (cfg.latency() != DEF_DURATION) {
            annotations.set(AnnotationKeys.LATENCY, cfg.latency().toString());
        }
        if (cfg.bandwidth() != DEF_BANDWIDTH) {
            annotations.set(AnnotationKeys.BANDWIDTH, String.valueOf(cfg.bandwidth()));
        }
        if (cfg.isDurable() != false) {
            annotations.set(AnnotationKeys.DURABLE, String.valueOf(cfg.isDurable()));
        }
        if (cfg.isBidirectional() != false){
            annotations.set("Bidirectional", String.valueOf(cfg.isBidirectional()));
        }
        return annotations.build();
    }

    @Override
    public ProviderId id() {
        return PID;
    }
}