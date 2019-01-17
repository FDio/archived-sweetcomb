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
endif

ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
PKG=deb
else ifeq ($(filter rhel centos fedora opensuse opensuse-leap opensuse-tumbleweed,$(OS_ID)),$(OS_ID))
PKG=rpm
endif

# +libganglia1-dev if building the gmond plugin

DEB_DEPENDS  = curl build-essential autoconf automake ccache
DEB_DEPENDS += bison flex libpcre3-dev libev-dev libavl-dev libprotobuf-c-dev protobuf-c-compiler libcmocka-dev
DEB_DEPENDS += cmake ninja-build
ifeq ($(OS_VERSION_ID),14.04)
	DEB_DEPENDS += libssl-dev
else
	DEB_DEPENDS += libssl-dev
endif

ifeq ($(findstring y,$(UNATTENDED)),y)
CONFIRM=-y
FORCE=--force-yes
endif

TARGETS = sweetcomb

.PHONY: help wipe wipe-release build build-release rebuild rebuild-release

define banner
	@echo "========================================================================"
	@echo " $(1)"
	@echo "========================================================================"
	@echo " "
endef

help:
	@echo "Make Targets:"
	@echo " install-dep         - install software dependencies"
	@echo " checkstyle          - check coding style"
	@echo " fixstyle            - fix coding style"
	@echo " scvpp               - build scvpp"
	@echo " plugins             - build plugins"
$(BR)/.deps.ok:
ifeq ($(findstring y,$(UNATTENDED)),y)
	make install-dep
endif
ifeq ($(filter ubuntu debian,$(OS_ID)),$(OS_ID))
	@MISSING=$$(apt-get install -y -qq -s $(DEB_DEPENDS) | grep "^Inst ") ; \
	if [ -n "$$MISSING" ] ; then \
	  echo "\nPlease install missing packages: \n$$MISSING\n" ; \
	  echo "by executing \"make install-dep\"\n" ; \
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

checkstyle:
	@build-root/scripts/checkstyle.sh

fixstyle:
	@build-root/scripts/checkstyle.sh --fix

scvpp:
	@build-root/scripts/scblder.sh src/scvpp

plugins:
	@build-root/scripts/scblder.sh src/plugins

