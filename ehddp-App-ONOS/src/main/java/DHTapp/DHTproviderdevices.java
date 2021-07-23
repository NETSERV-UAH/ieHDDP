package DHTapp;

import java.net.URI;
import java.util.Collections;
import java.util.List;
import java.util.ArrayList;
import java.util.Set;

import org.onosproject.net.*;
import org.onosproject.net.device.*;
import org.onosproject.net.provider.ProviderId;
import static org.onosproject.net.DeviceId.deviceId;
import static org.onosproject.net.PortNumber.portNumber;

import org.onlab.packet.*;
import org.onosproject.yang.model.NodeKey;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * @brief Provider de dispositivos para DHT
 * @author Joaquin Alvarez Horcajo
 */


public class DHTproviderdevices implements DeviceProvider {

    /** Atributes */
    /** @brieg Servicio de Log*/
    private final Logger log = LoggerFactory.getLogger(getClass());
    /** Valores typo de dispostivos validos */
    final public short TYPE_SDN = 1, TYPE_NO_SDN = 2, TYPE_SDN_NO_CONFIG = 3;
    private Integer Num_dev_sdn = 0;
    public List<String> Nodos_SDN = new ArrayList<String>();
    public long time_sdn_conection = 0;

    private enum TYPE_SENSORS {
        TEMPERATURE, WIND, PRESSURE, LIGHT, ACCELEROMETER,
                VIBRATION, GIROSCOPE, PROXIMITY, DISTANCE, MOVEMENT, SMOKE,
                MAGNETISM, HUMIDITY};

    public static ProviderId PID;

    /** @brief Construcctor de la clase */
    public DHTproviderdevices(ProviderId pid){
        PID = pid;
    }

    /** @brief Crea o actualiza los dispositivos no sensores de la red
     *  @param dps: DeviceProviderService
     *  @param hw:  numero hardware del switch
     *  @param sw: version del software
     *  @param id: identificador del switch
     *  @param manufacturer: codigo del manufacturer
     *  @param type: tipo de dispositivo
     *  @param mac: direccion mac principal del dispositivo
     *  @param serial: numero de serie del dispositivo
     *
     */
    public URI CreateorUpdateDevice(DeviceProviderService dps, DeviceProvider Dpr, String hw, String sw, String id, String manufacturer,
                             Device.Type type, String mac, String serial, Set<String> configureNodes) {
        URI uri;
        DeviceDescription desc;
        ChassisId cid = new ChassisId(mac);

        if(hw.contains("sw")){
            uri = URI.create("sw:"+str_to_id(id));
            /** Creamos el dispositivo */
            desc = new DefaultDeviceDescription(uri, type, manufacturer, hw, sw, serial, cid,
                    true, DefaultAnnotations.builder().set("Switch", "Legacy Switch")
                    .set(AnnotationKeys.PROTOCOL, "No Have").set(AnnotationKeys.USERNAME, "GateWay")
                    .set(AnnotationKeys.MANAGEMENT_ADDRESS, "1.0.0.1").set(AnnotationKeys.NAME, "GateWay")
                    .build());
        }
        else if(hw.contains("sdn")){
            uri = URI.create("of:"+str_to_id(id));
            // Creamos el dispositivo
            desc = new DefaultDeviceDescription(uri, type, "University of Alcala, Stanford University, Ericsson Research and CPqD Research",
                    "OpenFlow 1.3 Reference Userspace Switch","eHDDP", serial, cid,
                    true, DefaultAnnotations.builder().set("Switch", "SDN/ehddp Switch")
                    .set(AnnotationKeys.PROTOCOL, "OF_13").set(AnnotationKeys.USERNAME, uri.toString())
                    .set(AnnotationKeys.NAME, uri.toString())
                    //.set(AnnotationKeys.MANAGEMENT_ADDRESS, "127.0.0.1")
                    .build());
        }
        //if (hw.contains("Sensor")){
        else{
            uri = URI.create("Sensor:"+str_to_id(id));
            /** Creamos el dispositivo */
            desc = new DefaultDeviceDescription(uri, type, manufacturer, hw, sw, serial, cid,
                    true, DefaultAnnotations.builder().set(AnnotationKeys.PROTOCOL, "No Have")
                    .set(AnnotationKeys.USERNAME, hw).set(AnnotationKeys.MANAGEMENT_ADDRESS, "1.0.0.1")
                    .set(AnnotationKeys.NAME, hw)
                    .build());
        }
        configureNodes.add(desc.deviceUri().toString());

        DeviceId deviceId = deviceId(uri);
        /** unimos el puerto al device */
        dps.deviceConnected(deviceId, desc);

        /**Le cambiamos el role*/
        Dpr.roleChanged(deviceId,MastershipRole.STANDBY);
        dps.receivedRoleReply(deviceId, MastershipRole.STANDBY, MastershipRole.STANDBY);
        return uri;

    }

    /** @brief Nos dice si un device existe ya o no
     *
     * @param dsr DeviceService
     * @param deviceId Identificador del switch
     * @return true si existe, false en caso contrario
     */
    public boolean DeviceExist(DeviceService dsr, DeviceId deviceId){
        if (dsr.getDevice(deviceId) != null){
            return true;
        }
        return false;
    }

    /** @brief Comprueba que todos los dispositivos existen y si alguno no existe lo crea
     *
     * @param dpsv DeviceProviderService
     * @param dsv DeviceService
     * @param Packet_dht DHTpacket
     * @return True si todo ha ido correcto y false en caso contrario
     */
    public boolean checkdevices(DeviceProviderService dpsv, DeviceService dsv, DeviceProvider dpr,
                                DHTpacket Packet_dht, String TYPE_SENSORS[], Set<String> configureNodes){
        short type_device[] = Packet_dht.getTypedevices();
        String Mac;
        URI uri;

        for (int pos = 0; pos < Packet_dht.getNumHops(); pos++){
            Mac = Long.toHexString(Packet_dht.getidmacdevices()[pos]);
            log.info("Tipo de device leido: "+type_device[pos]);
            switch (type_device[pos]) {
                case TYPE_SDN:
                    uri = URI.create("of:"+str_to_id(Mac));
                    if (!Nodos_SDN.contains(uri.toString()))
                    {
                        log.info("Como la ID no esta en la lista, tomamos medida");
                        time_sdn_conection = System.currentTimeMillis();
                        Num_dev_sdn ++;
                        Nodos_SDN.add(uri.toString());
                    }
                    Collections.sort(Nodos_SDN);
                    log.info("ID Leida -> {} | Nodos_SDN = {}", uri.toString(), Nodos_SDN.toString());
                    log.info("Numero de Dispostivios SDN Leidos por eHDDP -> {}", Num_dev_sdn);
                    log.info("Nodos SDN detectado, no se trata ya que lo hace openflow");
                    break;
                case TYPE_NO_SDN:
                    /** Debemos actualizar los dispositivos siempre */
                    try {
                        uri = CreateorUpdateDevice(dpsv, dpr, "sw", "1.0.0",
                                Mac, "Switch Legacy", Device.Type.SWITCH, Mac, Mac, configureNodes);
                        /*para ahorrar tiempo ahora comprobamos sus puertos*/
                        checkportofdevice(dpsv, dsv, Packet_dht, uri, pos);
                    } catch (Exception e) {
                        log.error("UAH->HDDP->ERROR al crear nuevos dispositivos TYPE_NO_SDN: " + e.getMessage());
                        return false;
                    }
                    break;
                case TYPE_SDN_NO_CONFIG:
                    try {
                        uri = CreateorUpdateDevice(dpsv, dpr, "sdn", "1.0.0",
                                Mac, "Switch SDN using autoconfig process", Device.Type.SWITCH, Mac, Mac, configureNodes);
                        //para ahorrar tiempo ahora comprobamos sus puertos
                        checkportofdevice(dpsv, dsv, Packet_dht, uri, pos);
                    } catch (Exception e) {
                        log.error("UAH->HDDP->ERROR al crear nuevos dispositivos TYPE_SDN_NO_CONFIG: " + e.getMessage());
                        return false;
                    }
                    //return true; /* No hacemos nada por ahora*/
                    break;
                default: /** Si tenemos un sensor y permitimos la conexiones entre sensores */
                    /** Debemos actualizar los dispositivos siempre */
                    try {
                        uri = CreateorUpdateDevice(dpsv, dpr, TYPE_SENSORS[type_device[pos] - 4] + " Sensor", "1.0.1",
                                Mac, "Sensor", Device.Type.OTHER, Mac, Mac, configureNodes);
                        checkportofdevice(dpsv, dsv, Packet_dht, uri, pos);
                    } catch (Exception e) {
                        log.error("UAH->HDDP-> ERROR al crear nuevos sensores : " + e.getMessage());
                        return false;
                    }
                    break;
            }
        }
        return true;
    }

    public String str_to_id(String data){
        int len = data.length();
        String aux = "";
        for (int i= 0; i < (16 - len); i++){
            aux += "0";
        }
        aux+=data;
        return aux;
    }

    /** Comprueba que todos los puertos existen y si alguno no existe lo crea
     *
     * @param dpsv DeviceProviderService
     * @param dsv DeviceService
     * @param Packet_dht DHTpacket
     * @return true Si se han creado y asignado correctamente los puerto a los dispositivos; false en caso contrario
     */
    public boolean checkportperdevices(DeviceProviderService dpsv, DeviceService dsv, DHTpacket Packet_dht){
        short type_device[] = Packet_dht.getTypedevices();
        int ports_in[] = Packet_dht.getinports(), ports_out[] = Packet_dht.getoutports();
        String Nom_URI = "";
        /** Matriz de puertos a insertar, máximo 2 puertos por switch y paquete recibido */

        for (int pos = 0; pos < Packet_dht.getNumHops(); pos++){
            switch(type_device[pos])
            {
                case TYPE_SDN: /* SDN case */
                case TYPE_SDN_NO_CONFIG:
                    Nom_URI = "of:"+parser_idpacket_to_iddevice(Packet_dht.getidmacdevices()[pos]);
                    break;
                case TYPE_NO_SDN:
                    Nom_URI = "sw:"+Long.toHexString(Packet_dht.getidmacdevices()[pos]);
                    break;
                default: /* Sensor case, only when we can connect two sensor between them*/
                    Nom_URI = Long.toHexString(Packet_dht.getidmacdevices()[pos]);
                    break;
            }
            /** Si me llega un 0 como puerto lo elimino */
            if (ports_out[pos] == 0 || ports_in[pos] == 0)
                return false;
             /** creamos el uri del device */
            checkportofdevice(dpsv, dsv, Packet_dht, URI.create(Nom_URI), pos);
        }
        /** Todo ha ido correctamente */
        return true;
    }

    /** Comprueba que todos los puertos existen y si alguno no existe lo crea
     *
     * @param dpsv DeviceProviderService
     * @param dsv DeviceService
     * @param Packet_dht DHTpacket
     * @return true Si se han creado y asignado correctamente los puerto a los dispositivos; false en caso contrario
     */
    private void checkportofdevice(DeviceProviderService dpsv, DeviceService dsv, DHTpacket Packet_dht,
                                     URI uri, int pos){
        int num_port = 0;
        int port_in = Packet_dht.getinports()[pos], port_out = Packet_dht.getoutports()[pos];
        /** Matriz de puertos a insertar, máximo 2 puertos por switch y paquete recibido */
        long num_ports[] = new long[2];


        /** Si me llega un 0 como puerto lo elimino */
        if (port_out == 0 || port_in == 0 || port_out == 255 || port_in == 255)
            return;

        /** Reiniciamos la acumulacion */
        num_ports[0] = num_ports[1] = 0;
        /** Comprobamos los puerto de entrada de los dispositivos */
        if (!PortIsOnDevice (dsv, deviceId(uri), port_in)){
            num_ports[num_port] = (long)port_in;
            num_port++;
        }
        if (port_out != port_in){
            /** Comprobamos los puerto de salida de los dispositivos */
            if (!PortIsOnDevice (dsv, deviceId(uri), port_out)){
                num_ports[num_port] = (long)port_out;
                num_port++;
            }
        }
        /** Si existe algun puerto para crear */
        if (num_port > 0){
            try{
                if (deviceId(uri) != null) {
                    LinkPortToDevice(dpsv, dsv, deviceId(uri), num_port, num_ports);
                }
            }catch (Exception e){
                log.error("Algo fue mal con los puertos: "+e.getMessage());
                /** Algo ha ido con ERROR*/
                return;
            }
        }

        /** Todo ha ido correctamente */
        return;
    }

    /** Enlaza una lista de puertos a un dispositivo
     *
     * @param dps DeviceProviderService
     * @param dsv DeviceService
     * @param deviceId id del dispositivo (DeviceId)
     * @param num_port numero total de puertos a asignar al dispositivo
     * @param num_ports lista de puertos (solo el identificador numerico del puerto)
     */
    public void LinkPortToDevice(DeviceProviderService dps, DeviceService dsv, DeviceId deviceId, int num_port, long num_ports[]){
        List<PortDescription> portdes = new ArrayList<>();
        List<Port>ports = dsv.getPorts(deviceId);

        /* A este proceso solo llegan los puertos que realmente no estan asignados al dispositivo*/

        /** Configuramos los nuevos puertos */
        for (int pos = 0; pos < num_port; pos++){
            if (num_ports[pos] != 0) /*El cero no me vale como numero de puerto*/
                portdes.add(CreatePortDescription(Port.Type.COPPER, num_ports[pos], 1000));
        }
        /** debemos meter los antiguos  */
        for(Port port : ports) {
            portdes.add(CreatePortDescription(Port.Type.COPPER, port.number().toLong(), 1000));
        }

        /** Actualizamos los puertos del dispositivo */
        try{
            dps.updatePorts(deviceId, portdes);
        }catch (Exception e) {
            log.error("ERROR: dps.updatePorts(deviceId, portsdescription)-> " + deviceId.toString()+"->"+portdes);
        }

    }

    /** Nos comprueba si un dispositivo tiene un puerto o no
     *
     * @param dsr DeviceService
     * @param deviceId id del dispositivo (DeviceId)
     * @param num_port identificador del puerto a comprobar
     * @return true si el puerto esta en el dispositivo y false en caso contrario
     */
    public boolean PortIsOnDevice (DeviceService dsr, DeviceId deviceId, int num_port){

        Device device = dsr.getDevice(deviceId);
        if (device != null){
            if (dsr.getPort(deviceId, portNumber(num_port)) != null)
                return true;
            else
                return false;
        }
        return false;
    }

    /** @brieg Crea una puerto para un device
     *
     * @param Type typo de dispositivo
     * @param Port identificador del puerto
     * @param speed velocidad del enlace
     * @return devuelve la descripcion del puerto en una variable tipo PortDescription
     */
    public PortDescription CreatePortDescription(Port.Type Type, long Port, long speed) {
        return DefaultPortDescription.builder().withPortNumer(portNumber(Port)).isEnabled(true).
                portSpeed(speed).type(Type).build();

    }

    public boolean removedevice(DeviceProviderService deviceProviderService, DeviceService deviceService, DeviceId deviceid){
        try{
            /** Eliminamos los puertos del dispositivo */
            for (Port port: deviceService.getPorts(deviceid)){
                deviceProviderService.deletePort(deviceid,
                        CreatePortDescription(port.type(), port.number().toLong(), port.portSpeed()));
            }
            /** Eliminamos El dispositivo */
            deviceProviderService.deviceDisconnected(deviceid);
        }catch (Exception e){
            log.error("DHTAPP ERROR :AL BORRAR LOS DISPOSITIVOS");
            log.error(e.getMessage());
            return false;
        }
        return true;
    }

    /**
     * @brief Parsea la id recibida en el paquete a una valida para el sistema *
     * @param dpid Id del dispositivo (long)
     */
    private String parser_idpacket_to_iddevice(long dpid) {
        String DpString = Long.toHexString(dpid);
        int len = DpString.length();

        for(int aux = len; aux<16; aux++){
            DpString="0"+ DpString;
        }
        return DpString;
    }

    public void set_num_SDN_devices (Integer num){
        Num_dev_sdn = num;
    }

    public Integer get_num_SDN_devices (){
        return Num_dev_sdn;
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

    @Override
    public ProviderId id() {
        return PID;
    }
}
