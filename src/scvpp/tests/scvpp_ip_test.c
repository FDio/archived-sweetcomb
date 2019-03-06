/*
 * Copyright (c) 2019 Cisco
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

#include <unistd.h>
#include <setjmp.h>
#include <stdarg.h>
#include <cmocka.h>

#include "scvpp_test.h"

#include <scvpp/ip.h>
#include <scvpp/interface.h>
#include <scvpp/comm.h>

static void test_add_remove_ipv4(void **state)
{
    UNUSED(state);
    const char ip_q[VPP_IP4_ADDRESS_STRING_LEN] = "192.168.100.144";
    char ip_r[VPP_IP4_ADDRESS_STRING_LEN];
    u8 prefix_q = 24;
    u8 prefix_r;
    int rc;

    //add ipv4 on tap0
    rc = ipv46_config_add_remove("tap0", ip_q, prefix_q, false, true);
    assert_int_equal(rc, SCVPP_OK);

    //dump ipv4 on tap0 and check if it mach
    rc = ipv46_address_dump("tap0", ip_r, &prefix_r, false);
    assert_int_equal(rc, SCVPP_OK);
    assert_string_equal(ip_q, ip_r);
    assert_int_equal(prefix_q, prefix_r);

    //remove ipv4 on tap0
    rc = ipv46_config_add_remove("tap0", ip_q, prefix_q, false, false);
    assert_int_equal(rc, SCVPP_OK);

    //dump ipv4 after removal and check if equals 0.0.0.0
    rc = ipv46_address_dump("tap0", ip_r, &prefix_r, false);
    assert_int_equal(rc, SCVPP_OK);
    assert_string_equal(ip_r, "0.0.0.0");
}

static void test_ipv4_add_del_route_no_iface(void **state)
{
    UNUSED(state);
    int rc;
    char dst_address[VPP_IP4_ADDRESS_STRING_LEN] = "192.168.100.0";
    uint8_t prefix_len = 24;
    char next_hop[VPP_IP4_ADDRESS_STRING_LEN] = "192.168.100.100";
    uint32_t table_id = 0;
    fib_dump_t *dump;
    struct elt *stack;
    bool route_found = false;

    /* Must fail, can not have both interface and next hop IP null */
    rc = ipv46_config_add_del_route(dst_address, prefix_len, NULL, true,
                                    table_id, NULL);
    assert_int_equal(rc, -SCVPP_EINVAL);

    rc = ipv46_config_add_del_route(dst_address, prefix_len, next_hop, true,
                                    table_id, NULL);
    assert_int_equal(rc, SCVPP_OK);

    /* Dump all FIB routes and check if we find ours */
    stack = ipv4_fib_dump_all();
    assert_non_null(stack);
    foreach_stack_elt(stack) {
        dump = (fib_dump_t *) data;

        if (strncmp(sc_ntoa(dump->address), dst_address,
                    VPP_IP4_ADDRESS_STRING_LEN) == 0 &&
            dump->address_length == prefix_len) {
            route_found = true;
            assert_int_equal(dump->table_id, table_id);
            assert_int_equal(dump->count, 1);
            assert_string_equal(sc_ntoa(dump->path[0].next_hop), next_hop);
        }
        free(dump);
    }
    assert_true(route_found);

    /* Delete previously set route */
    rc = ipv46_config_add_del_route(dst_address, prefix_len, next_hop, false,
                                    table_id, NULL);
    assert_int_equal(rc, SCVPP_OK);

    /* Check our route has been deleted */
    route_found = false;
    stack = ipv4_fib_dump_all();
    assert_non_null(stack);
    foreach_stack_elt(stack) {
        dump = (fib_dump_t *) data;
        if (strncmp(sc_ntoa(dump->address), dst_address,
                    VPP_IP4_ADDRESS_STRING_LEN) == 0 &&
            dump->address_length == prefix_len) {
            route_found = true;
        }
        free(dump);
    }
    assert_false(route_found);
}

static void test_ipv4_add_del_route_no_next_hop_ip(void **state)
{
    UNUSED(state);
    int rc;
    char dst_address[VPP_IP4_ADDRESS_STRING_LEN] = "192.168.100.0";
    uint8_t prefix_len = 24;
    char interface[VPP_IP4_ADDRESS_STRING_LEN] = "tap0";
    uint32_t table_id = 0;
    uint32_t sw_if_index;
    fib_dump_t *dump;
    struct elt *stack;
    bool route_found = false;

    //Add a new route
    rc = ipv46_config_add_del_route(dst_address, prefix_len, NULL, true,
                                    table_id, interface);
    assert_int_equal(rc, SCVPP_OK);

    //Dump all FIB routes and check we find ours
    stack = ipv4_fib_dump_all();
    assert_non_null(stack);

    rc = get_interface_id(interface, &sw_if_index);
    assert_int_equal(rc, SCVPP_OK);

    foreach_stack_elt(stack) {
        dump = (fib_dump_t *) data;

        if (strncmp(sc_ntoa(dump->address), dst_address,
                    VPP_IP4_ADDRESS_STRING_LEN) == 0 &&
            dump->address_length == prefix_len) {
            route_found = true;
            assert_int_equal(dump->table_id, table_id);
            assert_int_equal(dump->count, 1);
            assert_int_equal(dump->path[0].sw_if_index, sw_if_index);
        }
        free(dump);
    }
    assert_true(route_found);

    //Delete route
    rc = ipv46_config_add_del_route(dst_address, prefix_len, NULL, false,
                                    table_id, interface);
    assert_int_equal(rc, SCVPP_OK);

    //Check our route has been deleted
    route_found = false;
    stack = ipv4_fib_dump_all();
    assert_non_null(stack);

    foreach_stack_elt(stack) {
        dump = (fib_dump_t *) data;
        if (strncmp(sc_ntoa(dump->address), dst_address,
                    VPP_IP4_ADDRESS_STRING_LEN) == 0 &&
            dump->address_length == prefix_len) {
            route_found = true;
        }
        free(dump);
    }
    assert_false(route_found);
}


const struct CMUnitTest ip_tests[IP_TEST_SIZE] = {
    cmocka_unit_test(test_add_remove_ipv4),
    cmocka_unit_test(test_ipv4_add_del_route_no_next_hop_ip),
    cmocka_unit_test(test_ipv4_add_del_route_no_iface),
};
