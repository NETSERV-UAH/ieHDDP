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
    $ sudo udatapath/ofdatapath -d=<dpid> -i=<if-list> punix:<socket> --no-slicing -I <Ip-inband>
    ```

2. Start the secure channel, which will connect the datapath to the controller:

    ```
    $ secchan/ofprotocol tcp:<switch-host>:<switch-port> tcp:<ctrl-host>:<ctrl-port>
    ```

# License
HDDP Switch is released under the GPLv3 license. 

This file is part of the HDDP Switch distribution (https://github.com/gistnetserv-uah/eHDDP-inband).
Copyright (c) 2020.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by  the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

