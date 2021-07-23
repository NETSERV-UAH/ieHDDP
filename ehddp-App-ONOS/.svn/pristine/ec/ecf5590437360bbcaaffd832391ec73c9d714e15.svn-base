package DHTapp;

import org.onlab.packet.BasePacket;
import java.util.Arrays;
import org.onlab.packet.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.nio.ByteBuffer;
import static com.google.common.base.MoreObjects.toStringHelper;
import static org.onlab.packet.PacketUtils.checkInput;

/**
 * @brief Paquete propio para el descubrimiento de topologias hibridas
 * @author Joaquin Alvarez Horcajo
 */


public class DHTpacket extends BasePacket {

    /** Atributos de la clase */

        /** @brief Servicio de log de la aplicacion */
        private final Logger log = LoggerFactory.getLogger(getClass());

        /** @brief Campo EtherType para un paquete de confirmacion */
        static public final short DHT_ETHERNET_TYPE = (short)65450; /** 43775 **/
        /** @brief Numero máximo de elementos posibles en un paquete */
        static public final short DHT_MAX_ELEMENT = (short)31;
        /** @brief Tamaño maximo de un paquete (ojo que me lo devuelve en bit) */
        static public final short DHT_PACKET_SIZE =
                ((6*Byte.SIZE)+ (6*Byte.SIZE)+ (6*Byte.SIZE) + (2*Short.SIZE) + (Integer.SIZE) + (Long.SIZE) + (Long.SIZE) +
                        ((Short.SIZE + (2*Integer.SIZE) + Long.SIZE + Byte.SIZE) * DHT_MAX_ELEMENT))/8;

        /** @brief Campos del paquete */
        private MacAddress Mac_ant, Last_mac, Src_mac;
        private short Opcode, Num_hops, Type_devices[];
        private int outports[], inports[], Time_block;
        private byte bidirectional[];
        private long id_mac_devices[], Num_Sec, Num_Ack;

    /** Metodos de la clase */

    /** @brief Constructor por defecto */
    public DHTpacket() {

    }

    /**
     * @brief Constructor con parametros
     *
     * @param Mac_ant: Direccion fisica de la mota anterior
     * @param Last_mac: Direccion fisica de la última mota que actualiza el paquete
     * @param Src_mac: Direccion fisica de la mota que crea el paquete
     * @param Opcode: Codigo de opcion => 1 = Request; 2 = Reply
     * @param Num_hops:  Numero de dispositivos
     * @param Time_block: Tiempo de bloqueo parametrizable por el controller
     * @param Num_Sec: Numero de secuencia para identificar el momento de lanzamiento desde el controller
     * @param Type_devices: Array con el tipo de dispositivos
     * @param outports: Array con los puertos de salida
     * @param inports: Array con los puertos de entrada
     * @param id_mac_devices: Array con los ID de los dispositivos por los que pasa
     * @param bidirectional: Array indicar si la conexión con el anterior es bidireccional
     * @param Num_ack: Número aleatorio para saber que elemento se confirma
     * @return objeto de la clase DHTpacket
     */
    public DHTpacket(MacAddress Mac_ant, MacAddress Last_mac, MacAddress Src_mac, short Opcode, short Num_hops, int Time_block, long Num_Sec, long Num_ack,
                     short Type_devices[], int outports[], int inports[], long id_mac_devices[], byte bidirectional[] ) {
        this.Mac_ant = Mac_ant;
        this.Last_mac = Last_mac;
        this.Src_mac = Src_mac;
        this.Opcode = Opcode;
        this.Num_hops = Num_hops;
        this.Time_block =Time_block;
        this.Num_Sec = Num_Sec;
        this.Type_devices = Type_devices;
        this.id_mac_devices = id_mac_devices;
        this.inports = inports;
        this.outports = outports;
        this.bidirectional = bidirectional;
        this.Num_Ack = Num_ack;
    }
    /**
     * @brief Obtiene la mac anterior (esto en el controller no vale de nada XD
     *
     * @return short Mac_ant
     */
    public MacAddress getMacAnt() { return Mac_ant; }

    /**
     * @brief Obtiene la última mac que actualiza el paquete (esto en el controller no vale de nada XD
     *
     * @return short Last_mac
     */
    public MacAddress getLastMac() { return Last_mac; }

    /**
     * @brief Obtiene la mac que crea el paquete
     *
     * @return short Src_mac
     */
    public MacAddress getSrcMac() { return Src_mac; }

    /**
     * @brief Obtiene el valor del timeblock.. no vale de nada
     *
     * @return short Time_block
     */
    public int getTimeblock() {
        return Time_block;
    }

    /**
     * @brief Obtiene el Numero de Secuencia del paquete
     *
     * @return short Num_Sec
     */
    public long getNum_Sec() {
        return Num_Sec;
    }

    /**
     * @brief Obtiene el Option code del paquete
     *
     * @return short Opcode
     */
    public short getOpcode() {
        return Opcode;
    }

    /**
     * @brief Obtiene El numero de dispositivos por los que ha pasado el paquete
     *
     * @return short Num_devices
     */
    public short getNumHops() { return Num_hops; }

    /**
     * @brief Obtiene un array con todos los tipos de dispositivos que lleva el paquete
     *
     * @return int [] Type_devices
     */
    public short [] getTypedevices() {
        return Type_devices;
    }

    /**
     * @brief Obtiene un array con todos los puertos de salida que lleva el paquete
     *
     * @return short [] outports
     */
    public int [] getoutports() {
        return outports;
    }

    /**
     * @brief Obtiene un array con todos los puertos de entrada que lleva el paquete
     *
     * @return short [] inports
     */
    public int [] getinports() {
        return inports;
    }

    /**
     * @brief Obtiene un array con todas las IDs de los switches que lleva el paquete
     *
     * @return long [] id_mac_devices
     */
    public long [] getidmacdevices() {
        return id_mac_devices;
    }
    /**
     * @brief Obtiene un array indicando si los enlaces son válidos en los dos sentidos
     *
     * @return long [] bidirectional
     */
    public byte [] getbidirectional() {
        return bidirectional;
    }
    /**
     * @brief obtiene el valor Num_ack del paquete
     *
     * @return long [] Num_ack
     */

    public long getNum_ack() {
        return Num_Ack;
    }


    /**
     * @brief Indica si un objeto es "igual que" este objeto, comparando todos sus elementos
     *
     * @return true si el objeto es igual, false en caso contrario
     */

    @Override
    public boolean equals(final Object obj) {
        if (this == obj) {
            return true;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (!(obj instanceof DHTpacket)) {
            return false;
        }
        final DHTpacket other = (DHTpacket) obj;
        if (this.Opcode != other.Opcode) {
            return false;
        }
        if (this.Num_hops != other.Num_hops) {
            return false;
        }
        if (this.Type_devices != other.Type_devices) {
            return false;
        }
        if (this.outports != other.outports) {
            return false;
        }
        if (this.inports != other.inports) {
            return false;
        }
        if (this.id_mac_devices != other.id_mac_devices) {
            return false;
        }
        if (this.Mac_ant != other.Mac_ant){
            return false;
        }
        if (this.Last_mac != other.Last_mac){
            return false;
        }
        if (this.Src_mac != other.Src_mac){
            return false;
        }
        if (this.Num_Sec != other.Num_Sec){
            return false;
        }
        if (this.Time_block != other.Time_block){
            return false;
        }
        if (this.Num_Ack != other.Num_Ack){
            return false;
        }
        if (this.bidirectional != other.bidirectional){
            return false;
        }
        return true;
    }

    /**
     * @brief Serializa el objeto especificado.
     *
     * @return array de bytes
     */
    @Override
    public byte[] serialize() {
        int length = DHT_PACKET_SIZE;

        /** Creamos buffer para serializar */
        final byte[] data = new byte[length];

        /** Envolvemos el buffer para que sea mas facil serializar */
        final ByteBuffer bb = ByteBuffer.wrap(data);

        /**Serializamos campo MAC anterior*/
        bb.put(this.Mac_ant.toBytes());

        /**Serializamos campo MAC anterior*/
        bb.put(this.Last_mac.toBytes());

        /**Serializamos campo MAC anterior*/
        bb.put(this.Src_mac.toBytes());

        /** Serializamos campo source device id */
        bb.putShort(this.Opcode);

        /** Serializamos campo Num Devices device id */
        bb.putShort(this.Num_hops);

        /**Serializamos Campos Time Block */
        bb.putInt(this.Time_block);

        /**Serializamos Campo Num Sec */
        bb.putLong(this.Num_Sec);

        /**serializamos el campo Num_Ack) */
        bb.putLong(this.Num_Ack);


        /** Serializamos el array de datos Tipo, Id, In_port, Out_port */
        set_array_buffer(bb, Type_devices, this.Num_hops);
        set_array_buffer(bb, id_mac_devices, this.Num_hops);
        set_array_buffer(bb, inports, this.Num_hops);
        set_array_buffer(bb, outports, this.Num_hops);
        set_array_buffer(bb, bidirectional, this.Num_hops);

        /** Devolvemos los datos serializados */
        return data;
    }

    /**
     * @brief Deserializa un paquete a partir de un array de bytes
     *
     * @param data Array de bytes recibidos
     * @param offset índice de comienzo del array de bytes
     * @param size longitud del paquete
     * @return objeto Ipacket del paquete deserializado
     */
    public IPacket deserialize(byte[] data, int offset, int size) {

        final ByteBuffer bb = ByteBuffer.wrap(data, offset, size);

        /**sacamos campo MAC_ANT*/
        this.Mac_ant = new MacAddress(get_array_buffer_byte(bb,6));
        /**sacamos campo Last Mac*/
        this.Last_mac = new MacAddress(get_array_buffer_byte(bb,6));
        /**sacamos campo Scr Mac*/
        this.Src_mac = new MacAddress(get_array_buffer_byte(bb,6));
        /** sacamos el campo Opcode */
        this.Opcode = bb.getShort();
        /** Sacamos el campo Num_devices */
        this.Num_hops = bb.getShort();
        /** Sacamos el campo time block */
        this.Time_block = bb.getInt();
        /** Sacamos el campo Num Secuence */
        this.Num_Sec = bb.getLong();
        /** Sacamos el campo Num ACK */
        this.Num_Ack = bb.getLong();
        /** Sacamos los datos pertenecientes a los diferentes arrays */
        this.Type_devices = get_array_buffer_short(bb);
        this.id_mac_devices = get_array_buffer_long(bb);
        this.inports = get_array_buffer_int(bb);
        this.outports = get_array_buffer_int(bb);
        this.bidirectional = get_array_buffer_byte(bb, DHT_MAX_ELEMENT);

        return this;
    }

    /**
     * @brief Deserializer function for Confirmation packet.
     *
     * @return deserializer function
     */

    public static Deserializer<DHTpacket> deserializer() {
        return (data, offset, length) -> {

            checkInput(data, offset, length, DHT_PACKET_SIZE);
            final ByteBuffer bb = ByteBuffer.wrap(data, offset, length);
            /** Creamos la clase para ser rellenada */
            DHTpacket packet = new DHTpacket();
            /**sacamos el campo MAC ANT */
            packet.Mac_ant = new MacAddress(get_array_buffer_byte(bb,6));
            /**sacamos el campo Last Mac */
            packet.Last_mac = new MacAddress(get_array_buffer_byte(bb,6));
            /**sacamos el campo Scr Mac */
            packet.Src_mac = new MacAddress(get_array_buffer_byte(bb,6));
            /** sacamos el campo Opcode */
            packet.Opcode = bb.getShort();
            /** Sacamos el campo Num_devices */
            packet.Num_hops = bb.getShort();
            /** sacamos Time Block */
            packet.Time_block = bb.getInt();
            /**sacamos num Sec */
            packet.Num_Sec = bb.getLong();
            /**sacamos num ACK */
            packet.Num_Ack = bb.getLong();
            /** Sacamos los datos pertenecientes a los diferentes arrays */
            packet.Type_devices = get_array_buffer_short(bb);
            packet.id_mac_devices = get_array_buffer_long(bb);
            packet.inports = get_array_buffer_int(bb);
            packet.outports = get_array_buffer_int(bb);
            packet.bidirectional = get_array_buffer_byte(bb,DHT_MAX_ELEMENT);

            return packet;
        };
    }

    @Override
    public String toString() {
        return toStringHelper(getClass())
                .add("MAC_Ant", this.Mac_ant.toString())
                .add("Lst_MAC", this.Last_mac.toString())
                .add("Scr_Mac", this.Src_mac.toString())
                .add("OpCode", String.valueOf(this.Opcode))
                .add("Num Devices",  String.valueOf(this.Num_hops))
                .add("Time Block", String.valueOf(this.Time_block))
                .add("Num Sec", String.valueOf(this.Num_Sec))
                .add("Num Ack", String.valueOf(this.Num_Ack))
                .add("Type Devices", Arrays.toString(Type_devices))
                .add("Id Mac Devices", Arrays.toString(id_mac_devices))
                .add("In Ports", Arrays.toString(inports))
                .add("Out Ports", Arrays.toString(outports))
                .add("bidirectional", Arrays.toString(bidirectional))
                .toString();
    }

    /** @brief introduce los datos en el buffer
     *
     * @param bb buffer
     * @param data datos
     * @param num_element numero de elementos a introducir en el buffer
     */
    public void set_array_buffer (ByteBuffer bb, short data [], int num_element) {
        for(int pos = 0; pos < DHT_MAX_ELEMENT; pos ++) {
            if (pos < num_element){
                /** Introducimos los datos en el paquete */
                bb.putShort(data[pos]);
            }
            else{
                /** Si ya hemos metido todos los elementos relleno con 0 */
                bb.putShort((short)0);
            }
        }
    }

    /** @brief introduce los datos en el buffer
     *
     * @param bb buffer
     * @param data datos
     * @param num_element numero de elementos a introducir en el buffer
     */
    public void set_array_buffer (ByteBuffer bb, int data [], int num_element) {
        for(int pos = 0; pos < DHT_MAX_ELEMENT; pos ++) {
            if (pos < num_element){
                /** Introducimos los datos en el paquete */
                bb.putInt(data[pos]);
            }
            else{
                /** Si ya hemos metido todos los elementos relleno con 0 */
                bb.putInt(0);
            }
        }
    }

    /** @brief introduce los datos en el buffer
     *
     * @param bb buffer
     * @param data datos
     * @param num_element numero de elementos a introducir en el buffer
     */
    public void set_array_buffer(ByteBuffer bb, long data [], int num_element) {
        for(int pos = 0; pos < DHT_MAX_ELEMENT; pos ++) {
            if (pos < num_element){
                /** Introducimos los datos en el paquete */
                bb.putLong(data[pos]);
            }
            else{
                /** Si ya hemos metido todos los elementos relleno con 0 */
                bb.putLong(0);
            }
        }
    }

    /** @brief introduce los datos en el buffer
     *
     * @param bb buffer
     * @param data datos
     * @param num_element numero de elementos a introducir en el buffer
     */
    public void set_array_buffer(ByteBuffer bb, byte data [], int num_element) {
        for(int pos = 0; pos < DHT_MAX_ELEMENT; pos ++) {
            if (pos < num_element){
                /** Introducimos los datos en el paquete */
                bb.put(data[pos]);
            }
            else{
                /** Si ya hemos metido todos los elementos relleno con 0 */
                bb.put((byte)0);
            }
        }
    }

    /** @brief obtiene los datos del buffer
     *
     * @param bb buffer
     * @return devuelve un array de datos tipo int
     */
    public static int [] get_array_buffer_int(ByteBuffer bb) {
        int data_recovery [] = new int[DHT_MAX_ELEMENT];

        for(int pos = 0; pos < DHT_MAX_ELEMENT; pos ++) {
            data_recovery[pos] = bb.getInt();
        }
        return data_recovery;
    }

    /** @brief obtiene los datos del buffer
     *
     * @param bb buffer
     * @return devuelve un array de datos tipo long
     */
    public static long [] get_array_buffer_long(ByteBuffer bb) {
        long data_recovery [] = new long[DHT_MAX_ELEMENT];

        for(int pos = 0; pos < DHT_MAX_ELEMENT; pos ++) {
            data_recovery[pos] = bb.getLong();
        }
        return data_recovery;
    }

    /** @brief obtiene los datos del buffer
     *
     * @param bb buffer
     * @return devuelve un array de datos tipo short
     */
    public static short[] get_array_buffer_short(ByteBuffer bb){
        short data_recovery [] = new short[DHT_MAX_ELEMENT];

        for(int pos = 0; pos < DHT_MAX_ELEMENT; pos ++) {
            data_recovery[pos] = bb.getShort();
        }
        return data_recovery;
    }

    /** @brief obtiene los datos del buffer
     *
     * @param bb buffer
     * @return devuelve un array de datos tipo byte
     */
    public static byte[] get_array_buffer_byte(ByteBuffer bb, int num_bytes){
        byte data_recovery [] = new byte[num_bytes];

        for(int pos = 0; pos < num_bytes; pos ++) {
            data_recovery[pos] = bb.get();
        }
        return data_recovery;
    }


    /** @brief permite generar una mac valida a partir de un long
     *
     * @param address identificador numero de la mac
     * @return string con una direccion mac valida
     */

    public String LongToMacString(long address)
    {
        int[] addressInBytes = new int[] {
                (int)((address >> 40) & 0xff),
                (int)((address >> 32) & 0xff),
                (int)((address >> 24) & 0xff),
                (int)((address >> 16) & 0xff),
                (int)((address >> 8 ) & 0xff),
                (int)((address >> 0) & 0xff)
        };

        return addressInBytes.toString();
    }
}
