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
else ifeq ($(filter rhel centos fedora opensuse opensuse-leap opensuse-tumbleweed,$(OS_ID)),$(OS_ID))
PKG=rpm
endif

# +libganglia1-dev if building the gmond plugin

DEB_DEPENDS  = curl build-essential autoconf automake ccache
DEB_DEPENDS += bison flex libpcre3-dev libev-dev libavl-dev libprotobuf-c-dev protobuf-c-compiler libcmocka-dev
DEB_DEPENDS += cmake ninja-build python-pkgconfig python-dev libssl-dev indent wget

RPM_DEPENDS = curl autoconf automake ccache bison flex pcre-devel libev-devel protobuf-c-devel protobuf-c-compiler libcmocka-devel
RPM_DEPENDS = cmake ninja-build python-pkgconfig python-devel openssl-devel  graphviz wget gcc gcc-c++ indent

ifeq ($(findstring y,$(UNATTENDED)),y)
CONFIRM=-y
FORCE=--force-yes
endif

TARGETS = sweetcomb

.PHONY: help install-dep install-dep-extra install-vpp checkstyle fixstyle build build-scvpp

define banner
	@echo "========================================================================"
	@echo " $(1)"
	@echo "========================================================================"
	@echo " "
endef

help:
	@echo "Make Targets:"
	@echo " install-dep          - install software dependencies"
	@echo " install-dep-extra    - install software extra dependencips from source code"
	@echo " install-vpp          - install released vpp"
	@echo " checkstyle           - check coding style"
	@echo " fixstyle             - fix coding style"
	@echo " build-scvpp          - build scvpp"
	@echo " build                - build plugin"
	@echo " clean               - clean all build"
	@echo " distclean           - remove all build directory"
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
	@sudo -E yum install $(COMFIRM) http://ftp.nohats.ca/libavl/libavl-0.3.5-1.fc17.x86_64.rpm http://ftp.nohats.ca/libavl/libavl-devel-0.3.5-1.fc17.x86_64.rpm
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
ifeq ($(OS_ID),centos)
ifeq ($(CENTOS_GCC),4)
	@echo "please use gcc version 7 or higher(# scl enable devtoolset-7 bash\n)"
	@echo "exit devtoolset-7 after this step finished"
	exit 1
endif
endif
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
	&&wget https://github.com/CESNET/libnetconf2/archive/v0.12-r1.tar.gz\
	&&tar xvf v0.12-r1.tar.gz && cd libnetconf2-0.12-r1 && mkdir -p build&& cd build\
	&&cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX:PATH=/usr ..\
	&&make&&make install&&ldconfig\
	&&cd ../../ && mv v0.12-r1.tar.gz libnetconf2-0.12-r1.tar.gz\
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

install-vpp:
	@echo "please install vpp as vpp's guide from source if failed"
ifeq ($(PKG),deb)
	@curl -s https://packagecloud.io/install/repositories/fdio/release/script.deb.sh | sudo bash\
	&&sudo -E apt-get $(CONFIRM) $(FORCE) install vpp vpp-lib vpp-plugins vpp-devel vpp-api-python vpp-api-lua vpp-api-java
else ifeq ($(PKG),rpm)
	@curl -s https://packagecloud.io/install/repositories/fdio/release/script.rpm.sh | sudo bash
ifeq ($(OS_ID),centos)
	@sudo yum $(CONFIRM) $(FORCE) install vpp vpp-lib vpp-plugins vpp-devel vpp-api-python vpp-api-lua vpp-api-java
endif
endif

checkstyle:
	@build-root/scripts/checkstyle.sh

fixstyle:
	@build-root/scripts/checkstyle.sh --fix

build-scvpp:
	@mkdir -p $(BR)/build-scvpp/;cd $(BR)/build-scvpp;cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/scvpp/;make install;
build:
	@mkdir -p $(BR)/build-plugins/;cd $(BR)/build-plugins/;cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr $(WS_ROOT)/src/plugins/;make install;

clean:
	@cd $(BR)/build-scvpp && make clean;
	@cd $(BR)/build-plugins && make clean;

distclean:
	@rm -rf $(BR)/build-scvpp
	@rm -rf $(BR)/build-plugins
