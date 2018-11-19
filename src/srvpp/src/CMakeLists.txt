#
# Copyright (c) 2016 Cisco and/or its affiliates.
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

include(GNUInstallDirs)

# srvpp sources
set(SRVPP_SOURCES
    srvpp_logger.c
    srvpp.c
)

# srvpp public headers
set(SRVPP_HEADERS
    srvpp.h
)

set(CMAKE_C_FLAGS " -g -O0 -fpic -fPIC -std=gnu99 -Wl,-rpath-link=/usr/lib")

# libraries to link with
set(LINK_LIBRARIES sysrepo vlibmemoryclient vapiclient vppapiclient svm vppinfra pthread rt dl)

# build instructions
add_library(srvpp SHARED ${SRVPP_SOURCES})
add_library(srvpp_a ${SRVPP_SOURCES})

# linker instructions
target_link_libraries(srvpp ${LINK_LIBRARIES})
target_link_libraries(srvpp_a ${LINK_LIBRARIES})

# install rules
install(TARGETS srvpp DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${SRVPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
