# eHDDP Switch In-Band

Copyright (C) 2020 Isaias Martinez-Yelmo(1), Joaquín Álvarez-Horcajo(1);

                 (1) GIST, University of Alcala, Spain.


## Before building
The switch makes use of the NetBee library to parse packets, so we need to install it first.

1. Install the following packages:

    ```
    $ sudo apt-get install cmake libpcap-dev libxerces-c3.1 libxerces-c-dev libpcre3 libpcre3-dev flex bison pkg-config autoconf libtool libboost-dev
    ```

2. Clone and build netbee

    ```
    $ git clone https://github.com/netgroup-polito/netbee.git
    $ cd netbee/src
    $ cmake .
    $ make
    ```

3. Add the shared libraries built in `/nbeesrc/bin/` to your `/usr/local/lib` directory

    ```
    $ sudo cp ../bin/libn*.so /usr/local/lib
    ```

4. Run `ldconfig`

    ```
    $ sudo ldconfig
    ```

5. Put the contens of folder `nbeesrc/include` in the `/usr/include`

    ```
    $ sudo cp -R ../include/* /usr/include/
    ```

## Building
Run the following commands in the `ofsoftswitch13` directory to build and install everything:

    $ ./boot.sh
    $ ./configure
    $ make
    $ sudo make install

## Running
1. Start the datapath:

    ```
    $ sudo udatapath/ofdatapath --datapath-id=<dpid> --interfaces=<if-list> ptcp:<port>
    ```

    This will start the datapath, with the given datapath ID, using the interaces listed. It will open a passive TCP connection on the given port. For a complete list of options, use the `--help` argument.

2. Start the secure channel, which will connect the datapath to the controller:

    ```
    $ secchan/ofprotocol tcp:<switch-host>:<switch-port> tcp:<ctrl-host>:<ctrl-port>
    ```

    This will open TCP connections to both the switch and the controller, relaying OpenFlow protocol messages between them. For a complete list of options, use the `--help` argument.

## Configuring
You can send requests to the switch using the `dpctl` utility.

* Check the flow statistics for table 0.

    ```
    $ utilities/dpctl tcp:<switch-host>:<switch-port> stats-flow table=0
    ```

* Install a flow to match IPv6 packets with extension headers hop by hop and destination and coming from port 1.

    ```
    $ utilities/dpctl tcp:<switch-host>:<switch-port> flow-mod table=0,cmd=add in_port=1,eth_type=0x86dd,ext_hdr=hop+dest apply:output=2
    ```

* Add a meter:

    ```
    $ utilities/dpctl tcp:<switch-host>:<switch-port> meter-mod cmd=add,meter=1 drop:rate=50
    ```

* Send flow to meter table

    ```
    $ utilities/dpctl tcp:<switch-host>:<switch-port> flow-mod table=0,cmd=add in_port=1 meter:1
    ```

For a complete list of commands and arguments, use the `--help` argument. Also, check the wiki for [Flow Mod examples](https://github.com/CPqD/ofsoftswitch13/wiki/Dpctl-Flow-Mod-Cases)

# Contribute
Please submit your bug reports, fixes and suggestions as pull requests on
GitHub, or by contacting us directly.

# License
HDDP Switch is released under the GPLv3 license. 

This file is part of the HDDP Switch distribution (https://github.com/gistnetserv-uah/eHDDP-inband).
Copyright (c) 2020.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by  the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

