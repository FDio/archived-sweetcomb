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
project(scvpp)

# COMPILER & LINKER
###################

# scvpp sources
set(SCVPP_SOURCES
    src/comm.c
    src/interface.c
    src/ip.c
    src/v3po.c
    src/nat.c
)

# scvpp public headers
set(SCVPP_HEADERS
    inc/scvpp/comm.h
    inc/scvpp/interface.h
    inc/scvpp/ip.h
    inc/scvpp/v3po.h
    inc/scvpp/nat.h
)

#define CMAKE_INSTALL_LIBDIR/INCLUDEDIR
include(GNUInstallDirs)

# Generate a compile_commands.json with compile options
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

#set compiler and linker flags
set(RIGOROUS_C_FLAGS "-Wlogical-op -Wformat=2")
set(CMAKE_C_FLAGS "-Wall -Wextra -std=gnu99 ${RIGOROUS_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -std=gnu99 -g -O0 ${RIGOROUS_C_FLAGS}")
#NDEBUG to skip assert checks
set(CMAKE_C_FLAGS_RELEASE "-Wall -Wextra -std=gnu99 -DNDEBUG -O2 ${RIGOROUS_C_FLAGS}")

# libraries to link with
set(LINK_LIBRARIES vlibmemoryclient vapiclient vppapiclient svm vppinfra pthread rt dl)

# build instructions
add_library(scvpp SHARED ${SCVPP_SOURCES})
add_library(scvpp_a ${SCVPP_SOURCES})

# linker instructions
target_link_libraries(scvpp ${LINK_LIBRARIES})
target_link_libraries(scvpp_a ${LINK_LIBRARIES})

# to build shared and static library for scvpp, include project private directory inc before installing header file
target_include_directories(scvpp PRIVATE inc)
target_include_directories(scvpp_a PRIVATE inc)

# TESTING
#########

# enable testing if requested and possible
SET(ENABLE_TESTS 1 CACHE BOOL "Enable unit tests.")
if(ENABLE_TESTS)
    find_package(CMOCKA)
    if(CMOCKA_FOUND)
        MESSAGE(STATUS "CMocka found, tests are enabled.")
        enable_testing()
        add_subdirectory(tests)
    else(CMOCKA_FOUND)
        MESSAGE(WARNING "CMocka not found, tests are disabled.")
    endif(CMOCKA_FOUND)
endif(ENABLE_TESTS)


# INSTALL
#########

# install rules
install(TARGETS scvpp DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${SCVPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/scvpp)

find_package(PkgConfig QUIET) #QUIET disables message if not found
if(PKG_CONFIG_FOUND)
    # generate and install pkg-config file
    configure_file("libscvpp.pc.in" "libscvpp.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libscvpp.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()

