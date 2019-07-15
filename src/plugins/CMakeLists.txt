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

cmake_minimum_required(VERSION 3.0)

# DEPENDENCIES
##############

# Cmake find modules directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")

# check for required packages
find_package(VPP) #use FindVPP.cmake
find_package(PkgConfig) #official cmake module
pkg_check_modules(SYSREPO REQUIRED libsysrepo) #PkgConfig cmake module maccro

find_package(Boost 1.40 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

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

# COMPILER & LINKER
###################

# Generate a compile_commands.json with compile options
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# set compiler and linker flags
set(RIGOROUS_CXX_FLAGS "-Wlogical-op -Wformat=2")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=gnu++11 ${RIGOROUS_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-Wall -Wextra -std=gnu++11 -g -O0 ${RIGOROUS_CXX_FLAGS}")
#NDEBUG to skip assert checks
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -std=gnu++11 -DNDEBUG -O2 ${RIGOROUS_CXX_FLAGS}")

# add plugin include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# plugins sources
set(PLUGINS_SOURCES
    sc_init.c
    sc_plugins.c
    sys_util.cpp
    vpp-oper/interface.cpp
    ietf/ietf_interface.cpp
    openconfig/openconfig_interfaces.cpp
    ietf/ietf_nat.cpp
)

set_source_files_properties(${PLUGINS_SOURCES} PROPERTIES LANGUAGE CXX)

# build the source code into shared library
add_library(sweetcomb SHARED ${PLUGINS_SOURCES})
target_link_libraries(sweetcomb ${SYSREPO_LIBRARIES} ${Boost_LIBRARIES}
                                  ${VOM_LIBRARY})

# INSTALL
#########

# install the plugin into plugins dir
install(TARGETS sweetcomb DESTINATION ${SR_PLUGINS_DIR})
