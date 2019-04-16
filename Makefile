# Copyright (c) 2018 Intel and/or its affiliates.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at:
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#  Main Dependencies:
#  netopeer2 -> libyang
#            -> libnetconf2 -> libyang
#                           -> libssh (>=0.6.4)
#
#  gnmi      -> libsysrepo
#            -> protobuf (>=3)
#            -> libjsoncpp
#
#  sysrepo   -> libyang
#            -> libredblack or libavl
#            -> libev
#            -> protobuf-c

export WS_ROOT=$(CURDIR)
export BR=$(WS_ROOT)/build-root
PLATFORM?=sweetcomb

##############
#OS Detection#
##############
ifneq ($(shell uname),Darwin)
OS_ID        = $(shell grep '^ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')
OS_VERSION_ID= $(shell grep '^VERSION_ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')
endif

ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
PKG=deb
cmake=cmake
else ifeq ($(filter rhel centos fedora opensuse opensuse-leap opensuse-tumbleweed,$(OS_ID)),$(OS_ID))
PKG=rpm
cmake=cmake3
endif

#####
#DEB#
#####
#Dependencies to build
BUILD_DEB = curl build-essential autoconf automake ccache git cmake wget coreutils
#Dependencies for netopeer2
NETOPEER2_DEB = libssl-dev
#Dependencies for checkstyle
CHECKSTYLE_DEB = indent
#Dependencies for scvpp
SCVPP_DEB = libcmocka-dev
#Dependencies for sysrepo (swig required for sysrepo python, lua, java)
SYSREPO_DEB = libev-dev libavl-dev bison flex libpcre3-dev libprotobuf-c-dev protobuf-c-compiler
#Dependencies of libssh
LIBSSH_DEB = zlib1g-dev
#Sum dependencies
DEB_DEPENDS = ${BUILD_DEB} ${NETOPEER2_DEB} ${CHECKSTYLE_DEB} ${SCVPP_DEB} \
	      ${SYSREPO_DEB} ${LIBSSH-DEB}

#Dependencies for grpc
DEB_GNMI_DEPENDS = libpugixml-dev libjsoncpp-dev libtool pkg-config

#####
#RPM#
#####
#Dependencies to build
BUILD_RPM = curl autoconf automake ccache cmake3 wget gcc gcc-c++ git
#Dependencies for netopeer2
NETOPEER2_RPM = openssl-devel
#Dependencies for checkstyle
CHECKSTYLE_RPM = indent
#Dependencies for scvpp
SCVPP_RPM = libcmocka-devel
#Dependencies for sysrepo
SYSREPO_RPM = libev-devel bison flex pcre-devel protobuf-c-devel protobuf-c-compiler

RPM_DEPENDS = ${BUILD_RPM} ${NETOPEER2_RPM} ${CHECKSTYLE_RPM} ${SCVPP_RPM} \
	      ${SYSREPO_RPM}

#Dependencies for grpc
RPM_GNMI_DEPENDS = pugixml jsoncpp libtool pugixml-devel jsoncpp-devel ${GRPC_RPM}

.PHONY: help install-dep install-dep-extra install-vpp install-models uninstall-models \
    install-dep-gnmi-extra build-scvpp build-plugins build-gnmi build-package docker \
    docker-test clean distclean _clean_dl _libssh _libyang _libnetconf2 _sysrepo _netopeer2

help:
	@echo "Make Targets:"
	@echo " install-dep            - install software dependencies"
	@echo " install-dep-extra      - install software extra dependencies from source code"
	@echo " install-vpp            - install released vpp"
	@echo " install-models         - install YANG models"
	@echo " uninstall-models       - uninstall YANG models"
	@echo " install-dep-gnmi-extra - install software extra dependencips from source code for gNMI"
	@echo " build-scvpp            - build scvpp"
	@echo " test-scvpp             - unit test for scvpp"
	@echo " build-plugins          - build plugins"
	@echo " build-gnmi             - build gNMIServer"
	@echo " build-package          - build rpm or deb package"
	@echo " docker                 - build sweetcomb in docker enviroment"
	@echo " docker-test            - run test in docker enviroment"
	@echo " clean                  - clean all build"
	@echo " distclean              - remove all build directory"

install-dep:
ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
ifeq ($(OS_VERSION_ID),14.04)
	@sudo -E apt-get -y --force-yes install software-properties-common
	@sudo -E add-apt-repository ppa:openjdk-r/ppa -y
endif
	@sudo -E apt-get update
	@sudo -E apt-get $(APT_ARGS) -y --force-yes install $(DEB_DEPENDS)
else ifeq ($(OS_ID),centos)
	@sudo -E yum install -y $(RPM_DEPENDS) epel-release centos-release-scl devtoolset-7
	@sudo -E yum remove -y libavl libavl-devel
	@sudo -E yum install -y http://ftp.nohats.ca/libavl/libavl-0.3.5-1.fc17.x86_64.rpm http://ftp.nohats.ca/libavl/libavl-devel-0.3.5-1.fc17.x86_64.rpm
else
	$(error "This option currently works only on Ubuntu, Debian, Centos or openSUSE systems")
endif

_libssh:
	mkdir -p $(BR)/downloads/&&cd $(BR)/downloads/\
	&&wget https://git.libssh.org/projects/libssh.git/snapshot/libssh-0.7.7.tar.gz\
	&&tar xvf libssh-0.7.7.tar.gz && cd libssh-0.7.7 && mkdir build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make -j$(nproc) &&sudo make install && sudo ldconfig&&cd ../../; \

_libyang:
	@mkdir -p $(BR)/downloads/&&cd $(BR)/downloads/\
	&&wget https://github.com/CESNET/libyang/archive/v0.16-r3.tar.gz\
	&&tar xvf v0.16-r3.tar.gz && cd libyang-0.16-r3 && mkdir -p build&& cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr \
	-DGEN_LANGUAGE_BINDINGS=OFF -DGEN_CPP_BINDINGS=ON \
	-DGEN_PYTHON_BINDINGS=OFF -DBUILD_EXAMPLES=OFF \
	-DENABLE_BUILD_TESTS=OFF .. \
	&&make -j$(nproc) &&make install&&cd ../../ \
	&& mv v0.16-r3.tar.gz libyang-0.16-r3.tar.gz

_libnetconf2:
	@mkdir -p $(BR)/downloads/&&cd $(BR)/downloads/\
	&&git clone https://github.com/CESNET/libnetconf2.git&&cd libnetconf2\
	&&git checkout 7e5f7b05f10cb32a546c42355481c7d87e0409b8&& mkdir -p build&& cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF\
	-DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make -j $(nproc) &&make install&&ldconfig\
	&&cd ../../\

_sysrepo:
	@mkdir -p $(BR)/downloads/&&cd $(BR)/downloads/\
	&&wget https://github.com/sysrepo/sysrepo/archive/v0.7.7.tar.gz\
	&&tar xvf v0.7.7.tar.gz && cd sysrepo-0.7.7 && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr \
	-DGEN_LANGUAGE_BINDINGS=OFF -DGEN_CPP_BINDINGS=ON -DGEN_LUA_BINDINGS=OFF \
	-DGEN_PYTHON_BINDINGS=OFF -DGEN_JAVA_BINDINGS=OFF -DBUILD_EXAMPLES=OFF \
	-DENABLE_TESTS=OFF ..\
	&&make -j$(nproc) &&make install&&cd ../../&& mv v0.7.7.tar.gz sysrepo-0.7.7.tar.gz

_netopeer2:
	@mkdir -p $(BR)/downloads/&&cd $(BR)/downloads/\
	&&wget https://github.com/CESNET/Netopeer2/archive/v0.7-r1.tar.gz\
	&&tar xvf v0.7-r1.tar.gz\
	&& echo "Netopeer2:keystored" \
	&& cd Netopeer2-0.7-r1/keystored && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make -j$(nproc) && make install && sudo ldconfig\
	&& echo "Netopeer2:server" \
	&&cd ../../server/ && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF\
	-DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make -j$(nproc) && make install && ldconfig\
	&& echo "Netopeer2:cli" \
	&&cd ../../cli && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make -j$(nproc) && make install && sudo ldconfig\
	&&cd ../../../ && mv v0.7-r1.tar.gz Netopeer2-0.7-r1.tar.gz\

_clean_dl:
	@rm -rf $(BR)/downloads

install-dep-extra: _clean_dl _libssh _libyang _libnetconf2 _sysrepo _netopeer2
	@cd ../ && rm -rf $(BR)/downloads

#Protobuf must not be compiled with multiple core, it requires too much memory
#gRPC can be compiled with cmake or autotools. autotools has been choosen
#if cmake is choosen use SSL, ZLIB distribution package and compile protobuf separately
install-dep-gnmi-extra:
ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
	@sudo -E apt-get $(APT_ARGS) -y --force-yes install $(DEB_GNMI_DEPENDS)
else ifeq ($(OS_ID),centos)
	@sudo -E yum install -y $(RPM_GNMI_DEPENDS)
else
	$(error "This option currently works only on Ubuntu, Debian, Centos or openSUSE systems")
endif
	@rm -rf $(BR)/downloads
	@mkdir -p $(BR)/downloads/&&cd $(BR)/downloads/\
	&& git clone --depth=1 -b v1.16.0 https://github.com/grpc/grpc \
	&& cd grpc && git submodule update --init \
	&& make && make install \
	&& cd third_party/protobuf \
	&& ./autogen.sh && ./configure && make install \
	&&cd ../../.. && rm -rf $(BR)/downloads

install-vpp:
	@echo "please install vpp as vpp's guide from source if failed"
ifeq ($(PKG),deb)
#	@curl -s https://packagecloud.io/install/repositories/fdio/release/script.deb.sh | sudo bash
	@sudo -E apt-get -y --force-yes install vpp libvppinfra* vpp-plugin-* vpp-dev
else ifeq ($(PKG),rpm)
#	@curl -s https://packagecloud.io/install/repositories/fdio/release/script.rpm.sh | sudo bash
ifeq ($(OS_ID),centos)
	@sudo yum -y install vpp vpp-lib vpp-plugin* vpp-devel
endif
endif

build-scvpp:
	@mkdir -p $(BR)/build-scvpp/; cd $(BR)/build-scvpp; \
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/scvpp/;\
	make install

test-scvpp: build-scvpp
	@cd $(BR)/build-scvpp; make test

build-plugins:
	@mkdir -p $(BR)/build-plugins/; cd $(BR)/build-plugins/; \
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/plugins/; \
	make install

build-gnmi:
	@mkdir -p $(BR)/build-gnmi/; cd $(BR)/build-gnmi/; \
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/gnmi/;make; \
	make install;

build-package:
	@mkdir -p $(BR)/build-package/; cd $(BR)/build-package/;\
	$(cmake) -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr \
	-DENABLE_TESTS=OFF $(WS_ROOT)/src/;make package;\
	rm -rf $(BR)/build-package/_CPack_Packages;

install-models:
	@cd src/plugins/yang/ietf \
	&& sysrepoctl --install --yang=ietf-ip@2014-06-16.yang \
	&& sysrepoctl --install --yang=ietf-nat@2017-11-16.yang \
	&& sysrepoctl --install --yang=iana-if-type@2017-01-19.yang \
	&& sysrepoctl -e if-mib -m ietf-interfaces;
	@cd src/plugins/yang/openconfig \
	&& sysrepoctl -S --install --yang=openconfig-local-routing@2017-05-15.yang \
	&& sysrepoctl -S --install --yang=openconfig-interfaces@2018-08-07.yang \
	&& sysrepoctl -S --install --yang=openconfig-if-ip@2018-01-05.yang;

uninstall-models:
	@sysrepoctl -u -m openconfig-if-ip \
	&& sysrepoctl -u -m openconfig-if-aggregate \
	&& sysrepoctl -u -m openconfig-local-routing \
	&& sysrepoctl -u -m openconfig-interfaces \
	&& sysrepoctl -u -m openconfig-vlan-types \
	&& sysrepoctl -u -m ietf-ip \
	&& sysrepoctl -u -m ietf-nat \
	&& sysrepoctl -u -m iana-if-type;

clean:
	@if [ -d $(BR)/build-scvpp ] ;   then cd $(BR)/build-scvpp   && make clean; fi
	@if [ -d $(BR)/build-plugins ] ; then cd $(BR)/build-plugins && make clean; fi
	@if [ -d $(BR)/build-package ] ; then cd $(BR)/build-package && make clean; fi
	@if [ -d $(BR)/build-gnmi ] ;    then cd $(BR)/build-gnmi    && make clean; fi

distclean:
	@rm -rf $(BR)/build-scvpp
	@rm -rf $(BR)/build-plugins
	@rm -rf $(BR)/build-package
	@rm -rf $(BR)/build-gnmi

docker: distclean
	@build-root/scripts/docker.sh

docker-test:
	@test/run_test.sh
