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
set(SONAME scvpp)
project(${SONAME})

include(GNUInstallDirs)

set(PROJECT_DESC "vpp-sysrepo integration library")
set(PROJECT_VERSION "0.0.2")

# srvpp sources
set(SCVPP_SOURCES scvpp.c)

# srvpp public headers
set(SCVPP_HEADERS scvpp.h)

set(CMAKE_C_FLAGS " -g -O0 -shared -fPIC -std=gnu99 -Wl,-rpath-link=/usr/lib")

# libraries to link with
#find_package(PkgConfig)
#pkg_check_modules(SYSREPO REQUIRED libsysrepo)
set(LINK_LIBRARIES sysrepo vlibmemoryclient vapiclient vppapiclient svm vppinfra pthread rt dl)

# build instructions
add_library(${SONAME} SHARED ${SCVPP_SOURCES})

# linker instructions
target_link_libraries(${SONAME} ${LINK_LIBRARIES})

# install rules
install(TARGETS ${SONAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${SCVPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    # generate and install pkg-config file
    configure_file("libscvpp.pc.in" "libscvpp.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libscvpp.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()
