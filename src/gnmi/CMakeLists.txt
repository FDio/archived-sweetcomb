#
# Copyright (c) 2019 PANTHEON.tech.
#
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
project(gNMI-server)

#C++11 is required by gNMI
set(CMAKE_CXX_STANDARD 11)

# DEPENDENCIES
##############

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")

find_package(PUGIXML REQUIRED) #use FindPUGIXML.cmake
find_package(JSONCPP REQUIRED) #use FindJSONCPP.cmake
find_package(PkgConfig) #official cmake module
pkg_check_modules(SYSREPO REQUIRED libsysrepo) #PkgConfig cmake module maccro

if(LIBPUGIXML_FOUND)
    message ("PUGIXML found")
else()
    message (FATAL_ERROR "Cannot find PUGIXML")
endif()

if(LIBJSONCPP_FOUND)
    message ("LIBJSONCPP found")
else()
    message (FATAL_ERROR "Cannot find LIBJSONCPP")
endif()

# DEPENDENCIES & COMPILATION OF GNMI PROTO
##########################################

# set compiler and linker flags
set(RIGOROUS_C_FLAGS "-Wlogical-op -Wold-style-cast -Wuseless-cast")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -std=c++11 ${RIGOROUS_C_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-Wall -Wextra -std=c++11 -DNDEBUG -O2 ${RIGOROUS_C_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG   "-Wall -Wextra -std=c++11 -g -O0 ${RIGOROUS_C_FLAGS}")

add_subdirectory(proto)

# COMPILER & LINKER
###################

# Generate a compile_commands.json with compile options
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(GNMI_SRC main.cpp
    log.cpp
    gnmiserver.cpp
    sysrepoapi.cpp
    sysrepoapipool.cpp
    xml2json.cpp
    gnmidata.cpp)

add_executable(gnmi_server ${GNMI_SRC})

target_include_directories(gnmi_server PRIVATE ${PROTOBUF_INCLUDE_DIR}
                             ${CMAKE_CURRENT_BINARY_DIR}/proto
                             ${LIBPUGIXML_INCLUDE_DIR}
                             ${LIBJSONCPP_INCLUDE_DIR}
                             ${SYSREPO_INCLUDE_DIRS})

# link gnmi_server executable with protobuf, grpc, pugixml, sysrepo libraries
target_link_libraries(gnmi_server gnmi
                        ${LIBPUGIXML_LIBRARY}
                        ${LIBJSONCPP_LIBRARY}
                        ${SYSREPO_LIBRARIES})

# INSTALL
#########

install(TARGETS gnmi_server RUNTIME DESTINATION bin)
