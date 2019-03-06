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

export WS_ROOT=$(CURDIR)
export BR=$(WS_ROOT)/build-root
CCACHE_DIR?=$(BR)/.ccache
GDB?=gdb
PLATFORM?=sweetcomb
MACHINE=$(shell uname -m)
SUDO?=sudo

#
# OS Detection
#
ifneq ($(shell uname),Darwin)
OS_ID        = $(shell grep '^ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')
OS_VERSION_ID= $(shell grep '^VERSION_ID=' /etc/os-release | cut -f2- -d= | sed -e 's/\"//g')
CENTOS_GCC   = $(shell gcc --version|grep gcc|cut -f3 -d' '|cut -f1 -d'.')
endif

ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
PKG=deb
cmake=cmake
else ifeq ($(filter rhel centos fedora opensuse opensuse-leap opensuse-tumbleweed,$(OS_ID)),$(OS_ID))
PKG=rpm
cmake=cmake3
endif

# +libganglia1-dev if building the gmond plugin

DEB_DEPENDS  = curl build-essential autoconf automake ccache git
DEB_DEPENDS += bison flex libpcre3-dev libev-dev libavl-dev libprotobuf-c-dev protobuf-c-compiler libcmocka-dev
DEB_DEPENDS += cmake ninja-build python-pkgconfig python-dev libssl-dev indent wget zlib1g-dev

DEB_GNMI_DEPENDS = libpugixml-dev libjsoncpp-dev libtool pkg-config golang libc-ares-dev libc-ares2

RPM_DEPENDS  = curl autoconf automake ccache bison flex pcre-devel libev-devel protobuf-c-devel protobuf-c-compiler libcmocka-devel
RPM_DEPENDS += cmake ninja-build python-pkgconfig python-devel openssl-devel  graphviz wget gcc gcc-c++ indent git cmake3
RPM_GNMI_DEPENDS = pugixml jsoncpp c-ares c-ares-devel libtool pugixml-devel jsoncpp-devel devtoolset-8-gcc-c++

ifeq ($(findstring y,$(UNATTENDED)),y)
CONFIRM=-y
FORCE=--force-yes
endif

TARGETS = sweetcomb

.PHONY: help install-dep install-dep-extra install-vpp checkstyle fixstyle build-plugins build-scvpp build-package

define banner
	@echo "========================================================================"
	@echo " $(1)"
	@echo "========================================================================"
	@echo " "
endef

help:
	@echo "Make Targets:"
	@echo " install-dep            - install software dependencies"
	@echo " install-dep-extra      - install software extra dependencips from source code"
	@echo " install-vpp            - install released vpp"
	@echo " install-models       - install YANG models"
	@echo " uninstall-models     - uninstall YANG models"
	@echo " install-dep-gnmi-extra - install software extra dependencips from source code for gNMI"
	@echo " checkstyle             - check coding style"
	@echo " fixstyle               - fix coding style"
	@echo " build-scvpp            - build scvpp"
	@echo " build-plugins          - build plugins"
	@echo " build-gnmi             - build gNMIServer"
	@echo " build-package          - build rpm or deb package"
	@echo " docker                 - build sweetcomb in docker enviroment"
	@echo " docker_test            - run test in docker enviroment"
	@echo " clean                  - clean all build"
	@echo " distclean              - remove all build directory"
$(BR)/.deps.ok:
ifeq ($(findstring y,$(UNATTENDED)),y)
	make install-dep
endif
ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
	@MISSING=$$(apt-get install -y -qq -s $(DEB_DEPENDS) | grep "^Inst ") ; \
	if [ -n "$$MISSING" ] ; then \
	  echo "\nPlease install missing packages: \n$$MISSING\n" ; \
	  echo "by executing \"make install-dep\"\n" ; \
	  echo "please add the full offical ubuntu source\n" ; \
	  exit 1 ; \
	fi ; \
	exit 0
endif
	@touch $@

install-dep:
ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
ifeq ($(OS_VERSION_ID),14.04)
	@sudo -E apt-get $(CONFIRM) $(FORCE) install software-properties-common
	@sudo -E add-apt-repository ppa:openjdk-r/ppa $(CONFIRM)
endif
	@sudo -E apt-get update
	@sudo -E apt-get $(APT_ARGS) $(CONFIRM) $(FORCE) install $(DEB_DEPENDS)
else ifeq ($(OS_ID),centos)
	@sudo -E yum install $(CONFIRM) $(RPM_DEPENDS) epel-release centos-release-scl devtoolset-7
	@sudo -E yum remove -y libavl libavl-devel
	@sudo -E yum install -y http://ftp.nohats.ca/libavl/libavl-0.3.5-1.fc17.x86_64.rpm http://ftp.nohats.ca/libavl/libavl-devel-0.3.5-1.fc17.x86_64.rpm
else
	$(error "This option currently works only on Ubuntu, Debian, Centos or openSUSE systems")
endif

define make
	@make -C $(BR) PLATFORM=$(PLATFORM) TAG=$(1) $(2)
endef

$(BR)/scripts/.version:
ifneq ("$(wildcard /etc/redhat-release)","")
	$(shell $(BR)/scripts/version rpm-string > $(BR)/scripts/.version)
else
	$(shell $(BR)/scripts/version > $(BR)/scripts/.version)
endif

#  Main Dependencies:
#  netopeer2 -> libyang
#            -> libnetconf2 -> libyang
#                           -> libssh
#  sysrepo   -> libyang
#            -> libredblack or libavl
#            -> libev
#            -> protobuf
#            -> protobuf-c

install-dep-extra:
	@rm -rf $(BR)/downloads
	@mkdir -p $(BR)/downloads/&&cd $(BR)/downloads/\
	\
	&&wget https://git.libssh.org/projects/libssh.git/snapshot/libssh-0.7.7.tar.gz\
	&&tar xvf libssh-0.7.7.tar.gz && cd libssh-0.7.7 && mkdir build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make&&sudo make install && sudo ldconfig&&cd ../../\
	\
	&&wget https://github.com/CESNET/libyang/archive/v0.16-r3.tar.gz\
	&&tar xvf v0.16-r3.tar.gz && cd libyang-0.16-r3 && mkdir -p build&& cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make &&make install&&cd ../../&& mv v0.16-r3.tar.gz libyang-0.16-r3.tar.gz\
	\
	&&wget https://github.com/swig/swig/archive/rel-3.0.12.tar.gz\
	&&tar xvf rel-3.0.12.tar.gz && cd swig-rel-3.0.12\
	&&./autogen.sh&&./configure --prefix=/usr --without-clisp --without-maximum-compile-warnings\
	&&make&&sudo make install && sudo ldconfig&&cd ../&& mv rel-3.0.12.tar.gz swig-3.0.12.tar.gz\
	\
	&&git clone https://github.com/CESNET/libnetconf2.git&&cd libnetconf2\
	&&git checkout 7e5f7b05f10cb32a546c42355481c7d87e0409b8&& mkdir -p build&& cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make&&make install&&ldconfig\
	&&cd ../../\
	\
	&&wget https://github.com/sysrepo/sysrepo/archive/v0.7.7.tar.gz\
	&&tar xvf v0.7.7.tar.gz && cd sysrepo-0.7.7 && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr -DGEN_PYTHON_VERSION=2 ..\
	&&make&&make install&&cd ../../&& mv v0.7.7.tar.gz sysrepo-0.7.7.tar.gz\
	\
	\
	&&wget https://github.com/CESNET/Netopeer2/archive/v0.7-r1.tar.gz\
	&&tar xvf v0.7-r1.tar.gz\
	\
	&& cd Netopeer2-0.7-r1/keystored && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make&&make install && sudo ldconfig\
	\
	&&cd ../../server/ && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make&& make install && ldconfig\
	\
	&&cd ../../cli && mkdir -p build && cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make&&make install && sudo ldconfig\
	&&cd ../../../ && mv v0.7-r1.tar.gz Netopeer2-0.7-r1.tar.gz\
	\
	&&cd ../ && rm -rf $(BR)/downloads

install-dep-gnmi-extra:
	@rm -rf $(BR)/downloads
	@mkdir -p $(BR)/downloads/
ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
	@sudo -E apt-get update
	@sudo -E apt-get $(APT_ARGS) $(CONFIRM) $(FORCE) install $(DEB_GNMI_DEPENDS)
	@sudo apt-get install -y software-properties-common
	@sudo add-apt-repository ppa:ubuntu-toolchain-r/test
	@sudo apt update
	@sudo apt install g++-7 -y
	@sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 \
                         --slave /usr/bin/g++ g++ /usr/bin/g++-7
	@sudo update-alternatives --config gcc

else ifeq ($(OS_ID),centos)
	@sudo -E yum install $(CONFIRM) $(RPM_GNMI_DEPENDS)
else
	$(error "This option currently works only on Ubuntu, Debian, Centos or openSUSE systems")
endif

	@cd $(BR)/downloads/\
	&&wget https://github.com/Kitware/CMake/releases/download/v3.14.0-rc2/cmake-3.14.0-rc2.tar.gz \
	&&tar xvf cmake-3.14.0-rc2.tar.gz &&cd cmake-3.14.0-rc2\
	&&./configure &&make &&make install\
	\
	&&cd $(BR)/downloads/\
	&&wget https://github.com/c-ares/c-ares/releases/download/cares-1_15_0/c-ares-1.15.0.tar.gz\
	&&tar xvf c-ares-1.15.0.tar.gz\
	&&mkdir -p c-ares-1.15.0/build && cd c-ares-1.15.0/build\
	&&cmake -DCMAKE_BUILD_TYPE=Release ../ &&make install\
	\
	&&cd $(BR)/downloads/\
	&&wget https://github.com/protocolbuffers/protobuf/archive/v3.7.0rc2.tar.gz\
	&&tar xvf v3.7.0rc2.tar.gz\
	&&cd protobuf-3.7.0rc2 &&./autogen.sh &&./configure --prefix=/usr\
	&&make &&make install &&ldconfig\
	\
	&&cd $(BR)/downloads/\
	&&wget https://github.com/grpc/grpc/archive/v1.18.0.tar.gz\
	&&tar xvf v1.18.0.tar.gz &&cd grpc-1.18.0 &&mkdir -p build && cd build\
	&&cmake  -DgRPC_INSTALL:BOOL=ON -DgRPC_BUILD_TESTS:BOOL=OFF \
	-DgRPC_ZLIB_PROVIDER:STRING=package -DgRPC_CARES_PROVIDER:STRING=package \
	-DgRPC_SSL_PROVIDER:STRING=package -DgRPC_PROTOBUF_PROVIDER=package \
	-DCMAKE_BUILD_TYPE=Release  ../\
	&&make ../ &&make install

install-vpp:
	@echo "please install vpp as vpp's guide from source if failed"
ifeq ($(PKG),deb)
#	@curl -s https://packagecloud.io/install/repositories/fdio/release/script.deb.sh | sudo bash
	@sudo -E apt-get $(CONFIRM) $(FORCE) install vpp libvppinfra* vpp-plugin-* vpp-dev
else ifeq ($(PKG),rpm)
#	@curl -s https://packagecloud.io/install/repositories/fdio/release/script.rpm.sh | sudo bash
ifeq ($(OS_ID),centos)
	@sudo yum $(CONFIRM) install vpp vpp-lib vpp-plugin* vpp-devel
endif
endif

checkstyle:
	@build-root/scripts/checkstyle.sh

fixstyle:
	@build-root/scripts/checkstyle.sh --fix

build-scvpp:
	@mkdir -p $(BR)/build-scvpp/;cd $(BR)/build-scvpp;cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/scvpp/;make install;

build-plugins:
	@mkdir -p $(BR)/build-plugins/;cd $(BR)/build-plugins/;cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/plugins/;make install;

docker:
	@build-root/scripts/docker.sh

docker_test:
	@test/run_test.sh

build-gnmi:
ifeq ($(OS_ID),centos)
	@source /opt/rh/devtoolset-8/enable \
	&&mkdir -p $(BR)/build-gnmi/;cd $(BR)/build-gnmi/;cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/gnmi/;make; make install;
else
	@mkdir -p $(BR)/build-gnmi/;cd $(BR)/build-gnmi/;cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/gnmi/;make; make install;
endif

build-package:
	@mkdir -p $(BR)/build-scvpp/;cd $(BR)/build-scvpp;cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/scvpp/;make install;
	@mkdir -p $(BR)/build-package/;cd $(BR)/build-package/;$(cmake) $(WS_ROOT)/src/;make package;rm -rf $(BR)/build-package/_CPack_Packages;

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
	@cd $(BR)/build-scvpp && make clean;
	@cd $(BR)/build-plugins && make clean;
	@cd $(BR)/build-package && make clean;
	@cd $(BR)/build-gnmi && make clean;

distclean:
	@rm -rf $(BR)/build-scvpp
	@rm -rf $(BR)/build-plugins
	@rm -rf $(BR)/build-package
	@rm -rf $(BR)/build-gnmi
