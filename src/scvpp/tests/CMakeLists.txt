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

include_directories ("${PROJECT_SOURCE_DIR}/src")

# check whether valgrind is installed
find_program(valgrind_FOUND valgrind)

set(TEST_SRC
    scvpp_test.h
    scvpp_test.c
    scvpp_iface_test.c
    scvpp_ip_test.c
    scvpp_nat_test.c
)

add_executable(scvpp_test ${TEST_SRC})
target_link_libraries(scvpp_test ${CMOCKA_LIBRARIES} scvpp_a)
target_include_directories(scvpp_test PRIVATE ../inc)
add_test(scvpp_test ./scvpp_test)

if(valgrind_FOUND)
    add_test(scvpp_valgrind valgrind
        --error-exitcode=1 --read-var-info=yes
        --leak-check=full --show-leak-kinds=all
        ./scvpp_test
    )
endif(valgrind_FOUND)
