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

set(CMAKE_CXX_STANDARD 17)
# set compiler option
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_C_FLAGS "-Wall -std=c++17")
set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O2")
set(CMAKE_C_FLAGS_DEBUG   "-g -O0")
set(CMAKE_INSTALL_LIBDIR "/usr/lib")

# Cmake find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../cmake")

find_package(PUGIXML REQUIRED)
find_package(JSONCPP REQUIRED)
find_package(PkgConfig)


pkg_check_modules(SYSREPO REQUIRED libsysrepo)

add_subdirectory(proto)

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

target_link_libraries(gnmi_server proto
                        ${LIBPUGIXML_LIBRARY}
                        ${LIBJSONCPP_LIBRARY}
                        ${SYSREPO_LIBRARIES})

install(TARGETS gnmi_server RUNTIME DESTINATION bin)
