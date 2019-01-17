#
# Copyright (c) 2018 Cisco and/or its affiliates.
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
#

cmake_minimum_required(VERSION 2.8)
set(SONAME sc_if)
project(${SONAME})

find_package(PkgConfig)
pkg_check_modules(SYSREPO REQUIRED libsysrepo)
pkg_check_modules(SCVPP REQUIRED libscvpp)

# get sysrepo plugins directory from pkgconfig
if (NOT SR_PLUGINS_DIR)
    if (PKG_CONFIG_FOUND)
        execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} "--variable=SR_PLUGINS_DIR" "libsysrepo" OUTPUT_VARIABLE SR_PLUGINS_DIR)
        string(STRIP ${SR_PLUGINS_DIR} SR_PLUGINS_DIR)
    endif()
endif()
if (NOT SR_PLUGINS_DIR)
    message(FATAL_ERROR "Cannot get sysrepo plugins directory due to missing pkg-config, set SR_PLUGINS_DIR manually.")
endif()

#
## srvpp sources
#set(PLUGIN_SOURCES
#	sc_interface.c
#	sc_interface_set_flags.c
#	sc_interface_add_del_address.c
#)
#
## srvpp public headers
#set(PLUGIN_HEADERS sc_interface.h)

set(CMAKE_C_FLAGS " -g -O0 -shared -fPIC -std=gnu99 -Wl,-rpath-link=/usr/lib")

aux_source_directory(. PLUGIN_SOURCES)
# build the source code into shared library
add_library(${SONAME} SHARED ${PLUGIN_SOURCES})
target_link_libraries(${SONAME} ${SYSREPO_LIBRARIES} ${SCVPP_LIBRARIES})

# install the plugin into plugins dir
install(TARGETS ${SONAME} DESTINATION ${SR_PLUGINS_DIR})
