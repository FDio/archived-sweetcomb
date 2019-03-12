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

# scvpp sources
set(SCVPP_SOURCES
    sc_vpp_comm.c
    sc_vpp_interface.c
    sc_vpp_ip.c
    sc_vpp_v3po.c
    sc_vpp_nat.c
)

# scvpp public headers
set(SCVPP_HEADERS
    sc_vpp_comm.h
    sc_vpp_interface.h
    sc_vpp_ip.h
    sc_vpp_v3po.h
    sc_vpp_nat.h
)

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

# install rules
install(TARGETS scvpp DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${SCVPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
