/*
 * Copyright (c) 2019 PANTHEON.tech.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <setjmp.h>
#include <cmocka.h>

#include "scvpp_nat_test.h"
#include "sc_vpp_comm.h"
#include "sc_vpp_nat.h"

void test_nat44_static_mapping(__attribute__((unused)) void **state)
{
    nat44_add_del_static_mapping_t map = {0};
    nat44_static_mapping_details_t dump = {0};
    u8 empty_ip[4] = {0};

    /*Configure the static mapping
      Alternative to this CLI command:
      nat44 add static mapping local 172.168.0.1 external 172.168.8.5
     */

    sc_aton("172.168.0.1", map.local_ip_address,
            sizeof(map.local_ip_address));

    sc_aton("172.168.8.5", map.external_ip_address,
            sizeof(map.external_ip_address));

    map.addr_only = 1;

    map.external_sw_if_index = ~0;

    map.is_add = 1;

    nat44_add_del_static_mapping(&map);

    nat44_static_mapping_dump(&dump);

    assert_int_equal(dump.addr_only, map.addr_only);

    assert_memory_equal(dump.local_ip_address, map.local_ip_address,
                        sizeof(dump.local_ip_address));

    assert_memory_equal(dump.external_ip_address, map.external_ip_address,
                        sizeof(dump.external_ip_address));

    /* Remove previous config*/
    map.is_add = 0;

    nat44_add_del_static_mapping(&map);

    memset(&dump, 0, sizeof(dump));

    nat44_static_mapping_dump(&dump);

    assert_int_equal(dump.addr_only, 0);

    assert_memory_equal(dump.local_ip_address, empty_ip,
                        sizeof(dump.local_ip_address));

    assert_memory_equal(dump.external_ip_address, empty_ip,
                        sizeof(dump.external_ip_address));
}

void test_nat44_static_mapping_with_ports(__attribute__((unused)) void **state)
{
    nat44_add_del_static_mapping_t map = {0};
    nat44_static_mapping_details_t dump = {0};
    nat44_add_del_address_range_t range = {0};
    u8 empty_ip[4] = {0};
    const u16 lport = 77;
    const u16 eport = 88;

    /*Configure address pool
     Alternative to this CLI:
     nat44 add address 5.4.4.4
     */
    sc_aton("5.4.4.4", range.first_ip_address,
            sizeof(range.first_ip_address));

    sc_aton("5.4.4.4", range.last_ip_address,
            sizeof(range.last_ip_address));

    range.is_add = 1;

    nat44_add_del_addr_range(&range);

    /*Configure NAT with ports
     Alternative to this CLI:
     nat44 add static mapping tcp local 172.168.0.1 77 external 5.4.4.4 88
     */

    sc_aton("172.168.0.1", map.local_ip_address,
            sizeof(map.local_ip_address));

    sc_aton("5.4.4.4", map.external_ip_address,
            sizeof(map.external_ip_address));

    map.protocol = 6;

    map.addr_only = 0;

    map.external_sw_if_index = ~0;

    map.external_port = eport;
    map.local_port = lport;

    map.is_add = 1;

    nat44_add_del_static_mapping(&map);

    nat44_static_mapping_dump(&dump);

    assert_int_equal(dump.addr_only, map.addr_only);

    assert_memory_equal(dump.local_ip_address, map.local_ip_address,
                        sizeof(dump.local_ip_address));

    assert_memory_equal(dump.external_ip_address, map.external_ip_address,
                        sizeof(dump.external_ip_address));

    assert_int_equal(dump.local_port, lport);

    assert_int_equal(dump.external_port, eport);

    /* Remove all previous config*/

    map.is_add = 0;

    nat44_add_del_static_mapping(&map);

    memset(&dump, 0, sizeof(dump));

    nat44_static_mapping_dump(&dump);

    assert_int_equal(dump.addr_only, 0);

    assert_memory_equal(dump.local_ip_address, empty_ip,
                        sizeof(dump.local_ip_address));

    assert_memory_equal(dump.external_ip_address, empty_ip,
                        sizeof(dump.external_ip_address));

    assert_int_equal(dump.local_port, 0);

    assert_int_equal(dump.external_port, 0);

    range.is_add = 0;

    nat44_add_del_addr_range(&range);
}

const struct CMUnitTest nat_tests[] = {
            cmocka_unit_test_setup_teardown(test_nat44_static_mapping, NULL,
                                            NULL),
            cmocka_unit_test_setup_teardown(test_nat44_static_mapping_with_ports,
                                            NULL, NULL),
    };
