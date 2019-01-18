
FROM ubuntu:18.04

RUN apt-get update && apt-get install -y cmake make gcc git sudo autoconf \
    libtool libpugixml-dev libjsoncpp-dev screen inetutils-ping iproute2 \
    command-not-found net-tools traceroute ethtool

RUN mkdir -p /usr/local/src

#===============================================================================
# VPP
#===============================================================================

WORKDIR /usr/local/src

RUN git clone https://gerrit.fd.io/r/vpp

WORKDIR /usr/local/src/vpp

RUN git checkout tags/v19.04-rc0

RUN yes | make install-dep

RUN make build

RUN make pkg-deb

WORKDIR /usr/local/src/vpp/build-root

RUN dpkg -i vpp-lib_*.deb vpp_*.deb vpp-dev_*.deb \
    vpp-plugins_*.deb vpp-dbg*.deb

#===============================================================================
# Protobuf
#===============================================================================

WORKDIR /usr/local/src

RUN git clone https://github.com/google/protobuf.git

WORKDIR /usr/local/src/protobuf

RUN git checkout tags/v3.6.1

RUN ./autogen.sh && ./configure --prefix=/usr && make && make install

RUN ldconfig

#===============================================================================
# Cmocka
#===============================================================================

WORKDIR /usr/local/src

RUN apt-get install -y git cmake build-essential bison flex libpcre3-dev libev-dev\
    libavl-dev valgrind python-dev lua5.2

RUN git clone git://git.cryptomilk.org/projects/cmocka.git

RUN mkdir -p ./cmocka/build

WORKDIR /usr/local/src/cmocka/build

RUN git checkout tags/cmocka-1.1.3

RUN cmake -DCMAKE_BUILD_TYPE=Debug .. && make && make install

#===============================================================================
# Libyang
#===============================================================================

WORKDIR /usr/local/src

RUN apt-get install -y libpcre3-dev

RUN git clone https://github.com/CESNET/libyang.git

RUN mkdir -p ./libyang/build

WORKDIR /usr/local/src/libyang/build

RUN git checkout tags/v0.16-r2

RUN cmake .. && make && make install

#===============================================================================
# Sysrepo
#===============================================================================

WORKDIR /usr/local/src

RUN apt-get install -y liblua5.1-0-dev protobuf-c-compiler libprotobuf-c-dev

RUN git clone https://github.com/sysrepo/sysrepo.git

RUN mkdir -p sysrepo/build

WORKDIR /usr/local/src/sysrepo/build

RUN git checkout tags/v0.7.6

RUN cmake -DCMAKE_BUILD_TYPE=Debug -DGEN_LANGUAGE_BINDINGS=OFF .. && make && make install

RUN ldconfig

#===============================================================================
# Libnetconf2
#===============================================================================

WORKDIR /usr/local/src

RUN apt-get install -y libssh-dev

RUN git clone https://github.com/CESNET/libnetconf2.git

RUN mkdir -p libnetconf2/build

WORKDIR /usr/local/src/libnetconf2/build

RUN git checkout tags/v0.12-r1

RUN cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr ../

RUN make && make install && ldconfig

#===============================================================================
# Netopeer2
#===============================================================================

WORKDIR /usr/local/src

RUN git clone https://github.com/CESNET/Netopeer2.git

WORKDIR /usr/local/src/Netopeer2

RUN git checkout tags/v0.7-r1

RUN mkdir -p ./keystored/build && mkdir -p ./cli/build && mkdir -p ./server/build

WORKDIR /usr/local/src/Netopeer2/keystored/build

RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ../

RUN make && make install && ldconfig

WORKDIR /usr/local/src/Netopeer2/server/build

RUN cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr ../

RUN make && make install && ldconfig

WORKDIR /usr/local/src/Netopeer2/cli/build

RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ../

RUN make && make install && ldconfig

#===============================================================================
# End
#===============================================================================

WORKDIR /root/src

