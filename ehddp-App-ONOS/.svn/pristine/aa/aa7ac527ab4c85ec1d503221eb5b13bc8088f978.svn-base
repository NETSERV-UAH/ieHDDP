package DHTapp;

import org.onlab.packet.BasePacket;
import org.onlab.packet.IPacket;

import java.nio.ByteBuffer;
import java.util.Arrays;

import static com.google.common.base.MoreObjects.toStringHelper;

public class DHTHellopacket extends BasePacket {

    private short Type_sensor = 0;
    static private final short HELLO_PACKET_SIZE = (Short.SIZE)/8;

    public DHTHellopacket(short Type_sensor) {
        this.Type_sensor = Type_sensor;
    }

    /**
     * @brief Obtiene el Option code del paquete
     *
     * @return short Opcode
     */
    public short getType_sensor() {
        return Type_sensor;
    }

    @Override
    public byte[] serialize() {
        int length = HELLO_PACKET_SIZE;

        /** Creamos buffer para serializar */
        final byte[] data = new byte[length];

        /** Envolvemos el buffer para que sea mas facil serializar */
        final ByteBuffer bb = ByteBuffer.wrap(data);

        /** Serializamos campo source device id */
        bb.putShort(this.Type_sensor);

        /** Devolvemos los datos serializados */
        return data;
    }

    @Override
    public IPacket deserialize(byte[] data, int offset, int size) {
        final ByteBuffer bb = ByteBuffer.wrap(data, offset, size);

        /** sacamos el campo Opcode */
        this.Type_sensor = bb.getShort();
        return null;
    }

    @Override
    public String toString() {
        return toStringHelper(getClass())
                .add("Type_sensor", String.valueOf(this.Type_sensor))
                .toString();
    }
}
