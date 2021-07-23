package DHTapp;

import org.onlab.graph.BellmanFordGraphSearch;
import org.onlab.packet.BasePacket;
import java.util.Arrays;
import java.util.BitSet;
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
        static private short DHT_PACKET_SIZE =
                (Byte.SIZE + Byte.SIZE + Byte.SIZE + Long.SIZE + Byte.SIZE + 6*Byte.SIZE + Long.SIZE +
                        6*Byte.SIZE + 6*Byte.SIZE + Integer.SIZE + //parte debug
                        ((Byte.SIZE + Short.SIZE + Long.SIZE + Integer.SIZE + Integer.SIZE) * DHT_MAX_ELEMENT))/8;
                /*((6*Byte.SIZE)+ (6*Byte.SIZE)+ (6*Byte.SIZE) + (2*Short.SIZE) + (Integer.SIZE) + (Long.SIZE) + (Long.SIZE) +
                        ((Short.SIZE + (2*Integer.SIZE) + Long.SIZE + Byte.SIZE) * DHT_MAX_ELEMENT))/8;

        /** @brief Campos del paquete */
        private byte Mac_ant[], Last_mac[], Src_mac[];
        private short Type_devices[];
        private int outports[], inports[], Time_block;
        private BitSet Flags = new BitSet(), Version = new BitSet();
        private byte configuration[];
        private byte Opcode, Num_hops, Previous_MAC_Length;
        private long id_mac_devices[], Num_Sec, Num_Ack;

    /** Metodos de la clase */
    private static short set_Packet_Size (short num_devices){
        return (short)((Byte.SIZE + Byte.SIZE + Byte.SIZE + Long.SIZE + Byte.SIZE + 6*Byte.SIZE + Long.SIZE +
                        6*Byte.SIZE + 6*Byte.SIZE + Integer.SIZE + //parte debug
                        ((Byte.SIZE + Short.SIZE + Long.SIZE + Integer.SIZE + Integer.SIZE) * num_devices))/8);
    }

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
     * @param configuration: Array para indicar las configuraciones de cada tupla (incluido la bidireccionalidad)
     * @param Num_ack: Número aleatorio para saber que elemento se confirma
     * @return objeto de la clase DHTpacket
     */
    public DHTpacket(byte[] Mac_ant, byte[] Last_mac, byte[] Src_mac, byte Opcode, byte Num_hops, int Time_block, long Num_Sec, long Num_ack,
                     short Type_devices[], int outports[], int inports[], long id_mac_devices[], BitSet Version,
                     BitSet Flags, byte Previous_MAC_Length, byte configuration [] ) {
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
        this.configuration = configuration;
        this.Num_Ack = Num_ack;
        /** New version of frame */
        this.Version = Version;
        this.Flags = Flags;
        this.Previous_MAC_Length = Previous_MAC_Length;
    }
    /**
     * @brief Obtiene la mac anterior (esto en el controller no vale de nada XD
     *
     * @return byte[] Mac_ant
     */
    public MacAddress getMacAnt() {
        return MacAddress.valueOf(Mac_ant);
    }

    /**
     * @brief Obtiene la última mac que actualiza el paquete (esto en el controller no vale de nada XD
     *
     * @return byte[] Last_mac
     */
    public MacAddress getLastMac() {
        return MacAddress.valueOf(Last_mac);
    }

    /**
     * @brief Obtiene la mac que crea el paquete
     *
     * @return byte[] Src_mac
     */
    public MacAddress getSrcMac() {
        return MacAddress.valueOf(Src_mac);
    }

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
     * @return byte Opcode
     */
    public byte getOpcode() {
        return Opcode;
    }

    /**
     * @brief Obtiene El numero de dispositivos por los que ha pasado el paquete
     *
     * @return short Num_devices
     */
    public byte getNumHops() { return Num_hops; }

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
     * @brief obtiene el valor Num_ack del paquete
     *
     * @return long [] Num_ack
     */

    public long getNum_ack() {
        return Num_Ack;
    }
    /**
     * @brief obtiene el valor Flags del paquete
     *
     * @return BitSet Flags
     */
    public BitSet getFlags() { return Flags;}
    /**
     * @brief obtiene el valor Version del paquete
     *
     * @return BitSet Version
     */
    public BitSet getVersion() { return Version;}
    /**
     * @brief obtiene el valor configuration del paquete
     *
     * @return BitSet [] configuration
     */
    public byte [] getconfiguration() { return configuration;}

    /**
     * @brief obtiene el valor Device_type_len del paquete
     *
     * @return BitSet [] Device_type_len
     */
    public int getDevice_type_len(int pos) {
        return ((configuration[pos] & 0b11000000) << 6) +1;
    }

    /**
     * @brief obtiene el valor Device_id_len del paquete
     *
     * @return int Device_id_len
     */
    public int getDevice_id_len(int pos) {
        return ((configuration[pos] & 0b00111000) << 3) +1;
    }
    /**
     * @brief obtiene el valor port_len del paquete
     *
     * @return int port_len
     */
    public int getport_len(int pos) {
        return ((configuration[pos] & 0b00000110) << 1) +1;
    }

    /**
     * @brief obtiene el valor port_len del paquete
     *
     * @return boolean bidirectional field
     */
    public boolean getbidirectional(int pos) {
        if ((configuration[pos] & 0b00000001) == 1)
            return true;
        else
            return false;
    }

    /**
     * @brief obtiene el valor Previous_MAC_Length del paquete
     *
     * @return byte Previous_MAC_Length
     */
    public byte getPrevious_MAC_Length(){return Previous_MAC_Length;}

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
        if (this.configuration != other.configuration){
            return false;
        }
        if ( this.Version != other.Version){
            return false;
        }
        if ( this.Flags != other.Flags){
            return false;
        }
        if ( this.Previous_MAC_Length != other.Previous_MAC_Length){
            return false;
        }

        return true;
    }

    private byte BitSet_To_Byte(BitSet vector_1, BitSet vector_2) {
        BitSet vector_1_clone = (BitSet)vector_1.clone();

        int n = vector_1_clone.length();//_desired length of the first (leading) vector
        int index = -1;
        while (index < (vector_2.length() - 1)) {
            index = vector_2.nextSetBit((index + 1));
            vector_1_clone.set((index + n));
        }
        return vector_1_clone.toByteArray()[0];
    }

    private byte BitSet_To_Byte(BitSet vector_1, BitSet vector_2, BitSet vector_3, BitSet vector_4) {
        BitSet vector_1_aux = (BitSet)vector_1.clone();

        int n = vector_1_aux.length();//_desired length of the first (leading) vector
        int index = -1;
        while (index < (vector_2.length() - 1)) {
            index = vector_2.nextSetBit((index + 1));
            vector_1_aux.set((index + n));
        }
        while (index < (vector_3.length() - 1)) {
            index = vector_3.nextSetBit((index + 1));
            vector_1_aux.set((index + n));
        }
        while (index < (vector_4.length() - 1)) {
            index = vector_4.nextSetBit((index + 1));
            vector_1_aux.set((index + n));
        }
        return vector_1_aux.toByteArray()[0];
    }

    /**
     * @brief Serializa el objeto especificado.
     *
     * @return array de bytes
     */
    @Override
    public byte[] serialize() {
        /** Creamos buffer para serializar */
        final byte[] data = new byte[set_Packet_Size (this.Num_hops)];

        /** Envolvemos el buffer para que sea mas facil serializar */
        final ByteBuffer bb = ByteBuffer.wrap(data);

        /** Serializamos Flags and version */
        bb.put(BitSet_To_Byte(this.Flags, this.Version));

        /** Serializamos campo source device id */
        bb.put(this.Opcode);

        /** Serializamos campo Num Devices device id */
        bb.put(this.Num_hops);

        /**Serializamos Campo Num Sec */
        bb.putLong(this.Num_Sec);

        /** Serializamos campo Previous_MAC_Length device id */
        bb.put(this.Previous_MAC_Length);

        /**Serializamos campo MAC anterior*/
        bb.put(this.Mac_ant);

        /**serializamos el campo Num_Ack) */
        bb.putLong(this.Num_Ack);

        /**Serializamos campo MAC anterior*/
        bb.put(this.Last_mac); //debug field

        /**Serializamos campo MAC anterior*/
        bb.put(this.Src_mac); //debug field

        /**Serializamos Campos Time Block */
        bb.putInt(this.Time_block); //debug field

        /** Serializamos el array de datos Tipo, Id, In_port, Out_port */
        set_array_dev_buffer(bb, this.configuration, this.Type_devices, this.id_mac_devices, this.inports,
                this.outports, this.Num_hops);

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

        /** Sacamos el primer byte (Flags and Version) */
        BitSet aux_BitSet = BitSet.valueOf(new byte[] { bb.get() });
        this.Flags = BitSet_split_vars(aux_BitSet, 7, 4);
        this.Version = BitSet_split_vars(aux_BitSet, 3, 0);

        /** sacamos el campo Opcode */
        this.Opcode = bb.get();

        /** Sacamos el campo Num_devices */
        this.Num_hops = bb.get();

        /** Sacamos el campo Num Secuence */
        this.Num_Sec = bb.getLong();

        /** Serializamos campo Previous_MAC_Length device id */
        this.Previous_MAC_Length = bb.get();

        /**Serializamos campo MAC anterior*/
        this.Mac_ant = get_array_buffer_byte(bb,this.Previous_MAC_Length);

        /**serializamos el campo Num_Ack) */
        this.Num_Ack = bb.getLong();

        /**Serializamos campo MAC anterior*/
        this.Last_mac = get_array_buffer_byte(bb,this.Previous_MAC_Length); //debug field

        /**Serializamos campo MAC anterior*/
        this.Src_mac= get_array_buffer_byte(bb,this.Previous_MAC_Length); //debug field

        /**Serializamos Campos Time Block */
        this.Time_block = bb.getInt(); //debug field

        this.configuration = new byte[this.Num_hops];
        this.Type_devices = new short[this.Num_hops];
        this.id_mac_devices = new long[this.Num_hops];
        this.inports = new int[this.Num_hops];
        this.outports = new int[this.Num_hops];

        for (int pos = 0; pos < this.Num_hops; pos++){
            /** Sacamos los datos pertenecientes a los diferentes arrays */
            this.configuration[pos] = bb.get();
            this.Type_devices[pos] = get_Type_device_frame(bb, this.getDevice_type_len(pos));
            this.id_mac_devices[pos] = get_id_mac_device_frame(bb, this.getDevice_id_len(pos));
            this.inports[pos] = get_port_frame(bb, this.getport_len(pos));
            this.outports[pos] = get_port_frame(bb, this.getport_len(pos));
        }

        return this;
    }

    /**
     * @brief Deserializer function for Confirmation packet.
     *
     * @return deserializer function
     */

    public static Deserializer<DHTpacket> deserializer() {
        return (data, offset, length) -> {

            checkInput(data, offset, length, 61);
            final ByteBuffer bb = ByteBuffer.wrap(data, offset, length);
            /** Creamos la clase para ser rellenada */
            DHTpacket packet = new DHTpacket();
            /** Sacamos el primer byte (Flags and Version) */
            BitSet aux_BitSet = BitSet.valueOf(new byte[] { bb.get() });
            packet.Flags = packet.BitSet_split_vars(aux_BitSet, 7, 4);
            packet.Version = packet.BitSet_split_vars(aux_BitSet, 3, 0);

            /** sacamos el campo Opcode */
            packet.Opcode = bb.get();

            /** Sacamos el campo Num_devices */
            packet.Num_hops = bb.get();

            /** Sacamos el campo Num Secuence */
            packet.Num_Sec = bb.getLong();

            /** Serializamos campo Previous_MAC_Length device id */
            packet.Previous_MAC_Length = bb.get();

            /**Serializamos campo MAC anterior*/
            packet.Mac_ant = get_array_buffer_byte(bb,packet.Previous_MAC_Length);

            /**serializamos el campo Num_Ack) */
            packet.Num_Ack = bb.getLong();

            /**Serializamos campo MAC anterior*/
            packet.Last_mac = get_array_buffer_byte(bb,packet.Previous_MAC_Length); //debug field

            /**Serializamos campo MAC anterior*/
            packet.Src_mac= get_array_buffer_byte(bb,packet.Previous_MAC_Length); //debug field

            /**Serializamos Campos Time Block */
            packet.Time_block = bb.getInt(); //debug field

            packet.configuration = new byte[packet.Num_hops];
            packet.Type_devices = new short[packet.Num_hops];
            packet.id_mac_devices = new long[packet.Num_hops];
            packet.inports = new int[packet.Num_hops];
            packet.outports = new int[packet.Num_hops];


            for (int pos = 0; pos < packet.Num_hops; pos++){
                /** Sacamos los datos pertenecientes a los diferentes arrays */
                packet.configuration[pos] = bb.get();
                packet.Type_devices[pos] = get_Type_device_frame(bb, packet.getDevice_type_len(pos));
                packet.id_mac_devices[pos] = get_id_mac_device_frame(bb, packet.getDevice_id_len(pos));
                packet.inports[pos] = get_port_frame(bb, packet.getport_len(pos));
                packet.outports[pos] = get_port_frame(bb, packet.getport_len(pos));
            }

            return packet;
        };
    }

    public BitSet BitSet_split_vars(BitSet data, int pos_init, int pos_end){
        BitSet result = new BitSet ();
        int pos = 0; /* Variable para posicionar los valores en el resultado de forma correcta*/

        for (int i = pos_init; i < pos_end; i ++){
            if (data.get(i) == true)
                result.set(pos,true);
            else
                result.set(pos,false);
            pos ++;
        }

        return result;

    }

    @Override
    public String toString() {
        return toStringHelper(getClass())
                .add("Version", this.Version.toString())
                .add("Flags", this.Flags.toString())
                .add("OpCode", String.valueOf(this.Opcode))
                .add("Num Devices",  String.valueOf(this.Num_hops))
                .add("Num Sec", String.valueOf(this.Num_Sec))
                .add("Previous_MAC_Length",String.valueOf(this.Previous_MAC_Length))
                .add("MAC_Ant", this.Mac_ant.toString())
                .add("Num Ack", String.valueOf(this.Num_Ack))
                .add("Lst_MAC", this.Last_mac.toString())
                .add("Scr_Mac", this.Src_mac.toString())
                .add("Time Block", String.valueOf(this.Time_block))
                .add("Configuration", Arrays.toString(this.configuration))
                .add("Type Devices", Arrays.toString(Type_devices))
                .add("Id Mac Devices", Arrays.toString(id_mac_devices))
                .add("In Ports", Arrays.toString(inports))
                .add("Out Ports", Arrays.toString(outports))
                .toString();
    }

    public static short get_Type_device_frame (ByteBuffer bb, int len){
        short type_device = 0;
        switch (len){
            case 0:
                type_device = 0;
                break;
            case 1:
                type_device = (short) bb.get();
                break;
            default:
                type_device = (short) bb.getShort();
                break;
        }

        return type_device;
    }

    public static int get_port_frame (ByteBuffer bb, int len) {
        int port = 0;
        switch (len) {
            /* Son casos teoricamente imposibles... pero por si acaso*/
            case 0:
            case 3:
                port = 0;
                break;
            case 1:
                port = (int) bb.get();
                break;
            case 2:
                port = (int) bb.getShort();
                break;
            default:
                port = (int) bb.getInt();
                break;
        }
        return port;
    }

    public static long get_id_mac_device_frame (ByteBuffer bb, int len){
        long id_mac_device = 0;
        switch (len){
            /* Son casos teoricamente imposibles... pero por si acaso*/
            case 0:
            case 3:
                id_mac_device = 0;
                break;
            case 1:
                id_mac_device = (long) bb.get();
                break;
            case 2:
                id_mac_device = (long) bb.getShort();
                break;
            case 4:
                id_mac_device = (long) bb.getInt();
                break;
            default:
                id_mac_device = (long) bb.getLong();
                break;
        }

        return id_mac_device;
    }

    /** @brief introduce los datos en el buffer
     *
     * @param bb buffer
     * @param configuration datos
     * @param Type_devices datos
     * @param id_mac_devices datos
     * @param inport datos
     * @param outport datos
     * @param num_element numero de elementos a introducir en el buffer
     */
    public void set_array_dev_buffer (ByteBuffer bb, byte configuration[], short Type_devices [],
                                      long id_mac_devices [], int inport [], int outport [], int num_element) {
        for(int pos = 0; pos < num_element; pos ++) {
            bb.put(configuration[pos]);
            bb.putShort(Type_devices[pos]);
            bb.putLong(id_mac_devices[pos]);
            bb.putInt(inport[pos]);
            bb.putInt(outport[pos]);
        }

    }
    public static long bytesToLong(byte[] bytes) {
        ByteBuffer buffer = ByteBuffer.allocate(Long.BYTES);
        buffer.put(bytes);
        buffer.flip();//need flip
        return buffer.getLong();
    }

    public static int bytesToInt(byte[] bytes) {
        ByteBuffer buffer = ByteBuffer.allocate(Integer.BYTES);
        buffer.put(bytes);
        buffer.flip();//need flip
        return buffer.getInt();
    }

    public static short bytesToShort(byte[] bytes) {
        ByteBuffer buffer = ByteBuffer.allocate(Short.BYTES);
        buffer.put(bytes);
        buffer.flip();//need flip
        return buffer.getShort();
    }

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
