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

cmake_minimum_required(VERSION 2.8)
project(srvpp)

# add subdirectories
add_subdirectory(src)

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

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    # generate and install pkg-config file
    configure_file("libsrvpp.pc.in" "libsrvpp.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libsrvpp.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif()
