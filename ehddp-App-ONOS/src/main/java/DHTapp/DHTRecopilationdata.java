package DHTapp;
import com.sun.corba.se.spi.ior.IdentifiableFactory;
import org.onosproject.net.LinkKey;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.*;
import java.util.*;

public class DHTRecopilationdata {

    public final Logger log = LoggerFactory.getLogger(getClass());

    public int num_elements_topolog(int type_element){
        FileReader input = null;
        BufferedReader bf = null;
        int numLines=0;

        try
            {
                switch (type_element)
                {
                    case 1:
                        input = new FileReader("/home/arppath/Applications/appOnos/ehddp/RyuFileNodes.txt");
                        break;
                    case 2:
                        input = new FileReader("/home/arppath/Applications/appOnos/ehddp/RyuFileEdges.txt");
                        break;
                    case 3:
                        input = new FileReader("/home/arppath/Applications/appOnos/ehddp/RyuFileSDN_Nodes.txt");
                        break;
                    default:
                        return -1;
                }

            bf = new BufferedReader(input);

            numLines = (int)bf.lines().count();

        } catch (Exception e) {
            e.printStackTrace();
        }
        //if (type_element == 2)
        //    numLines = numLines * 2; // Son bidireccionales por eso el doble
        return numLines;
    }

    public void Data_generic(long num_sec, int conver_time, int Num_packet_out, int Num_packet_in, int Num_packet_data,
                                    int Num_SDN, int Num_Non_SDN, int Num_Sensors, int Num_enlaces, int num_total_nodos,
                                    int num_total_enlaces, int configureNodes_count, String file)
    {
        FileWriter fichero = null;
        float porcentaje_nodos = 0, porcentaje_enlaces = 0, num_nodos = 0;
        PrintWriter pw = null;

        try
        {
             log.info("##############UAH-> porcentaje_nodos = {} || porcentaje_enlaces = {} || conver_time = {}!",porcentaje_nodos,
                    porcentaje_enlaces, conver_time);

            fichero = new FileWriter("/home/arppath/data-onos-hddp/"+file, true);
            pw = new PrintWriter(fichero);
            pw.println(num_sec + "\t" + conver_time + "\t" + Num_packet_out + "\t" + Num_packet_in + "\t" + Num_packet_data +
                    "\t" + Num_SDN + "\t" + Num_Non_SDN + "\t" + Num_Sensors + "\t" + Num_enlaces);
            log.info("##############UAH-> Fichero de resultados escrito correctamente!");

        } catch (Exception e) {
            e.printStackTrace();
            log.error("###############UAH->Algo fue mal al escribir el fichero de resultados");
        } finally {
            try {
                // Nuevamente aprovechamos el finally para
                // asegurarnos que se cierra el fichero.
                if (null != fichero)
                    fichero.close();
            } catch (Exception e2) {
                e2.printStackTrace();
            }
        }
    }

    public void links_topo(Set<LinkKey> configuredLinks, long num_sec)
    {
        FileWriter fichero = null;
        PrintWriter pw = null;
        Map<String, List<String>> topology = new HashMap<String, List<String>>();
        for (LinkKey linkkey: configuredLinks){
            List<String> destinos = new ArrayList<String>();
            if (topology.get(linkkey.src().deviceId().toString()) != null)
                destinos = topology.get(linkkey.src().deviceId().toString());
            destinos.add(linkkey.dst().deviceId().toString());
            topology.put(linkkey.src().deviceId().toString(),destinos);
        }

        try
        {
            fichero = new FileWriter("/home/arppath/data-onos-hddp/topology-onos-"+String.valueOf(num_sec)+".txt", true);
            pw = new PrintWriter(fichero);
            Map<String, List<String>> sortedtopology = new TreeMap<>(topology);

            for(Map.Entry<String,List<String>> entry : sortedtopology.entrySet()) {
                String key = entry.getKey();
                pw.print(key + "\t");
                for (int i = 0; i < entry.getValue().size(); i++) {
                    pw.print(entry.getValue().get(i)+"\t");
                }
                pw.print("\n");
            }

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                // Nuevamente aprovechamos el finally para
                // asegurarnos que se cierra el fichero.
                if (null != fichero)
                    fichero.close();
            } catch (Exception e2) {
                e2.printStackTrace();
            }
        }
    }
}