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

include_directories ("${PROJECT_SOURCE_DIR}/src")

# check whether valgrind is installed
find_program(valgrind_FOUND valgrind)

# macro for adding of an unit test
macro(ADD_UNIT_TEST TEST_NAME)
    set(TEST_SRC
        ${TEST_NAME}.c
    )
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} ${CMOCKA_LIBRARIES} srvpp_a)
    add_test(${TEST_NAME} ${TEST_NAME})
    
    if(valgrind_FOUND)
        add_test(${TEST_NAME}_valgrind valgrind
            --error-exitcode=1 --read-var-info=yes
            --leak-check=full --show-leak-kinds=all
            ./${TEST_NAME}
        )
    endif(valgrind_FOUND)
endmacro(ADD_UNIT_TEST)

# add individual unit-tests
ADD_UNIT_TEST(srvpp_test)