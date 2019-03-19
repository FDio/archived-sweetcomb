/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

#include "sc_vpp_comm.h"
#include "sc_vpp_interface.h"
#include "sc_vpp_ip.h"
#include "sc_vpp_v3po.h"

//TODO Check with future function get_interface_state
static void test_enable_disable(void **state)
{
    int rc;

    rc = interface_enable("tap0", 1);
    assert_int_equal(rc, 0);

    rc = interface_enable("tap0", 0);
    assert_int_equal(rc, 0);
}

//TODO would need to make sure tap0 is index 1
//TODO delete eventually because get_interface_id will not be extern
static void test_name2index(void **state)
{
    int rc;
    const char iface_name[] = "tap0";
    sw_interface_details_query_t query = {0};
    sw_interface_details_query_set_name(&query, iface_name);

    rc = get_interface_id(&query);
    assert_int_equal(rc, 1);

    assert_string_equal(iface_name, query.sw_interface_details.interface_name);
    assert_int_equal(query.sw_interface_details.sw_if_index, 1);
}

static void test_add_ipv4(void **state)
{
    const char ip_q[VPP_IP4_ADDRESS_STRING_LEN] = "192.168.100.144";
    char ip_r[VPP_IP4_ADDRESS_STRING_LEN];
    u8 prefix_q = 24;
    u8 prefix_r;
    int rc;

    //add ipv4 on tap0
    rc = ipv46_config_add_remove("tap0", ip_q, prefix_q, false, true);
    assert_int_equal(rc, 0);

    rc = ipv46_address_dump("tap0", ip_r, &prefix_r, false);
    assert_int_equal(rc, 0);

    assert_string_equal(ip_q, ip_r);
    assert_int_equal(prefix_q, prefix_r);
}

static void test_remove_ipv4(void **state)
{
    const char ip_q[VPP_IP4_ADDRESS_STRING_LEN] = "192.168.100.144";
    char ip_r[VPP_IP4_ADDRESS_STRING_LEN];
    u8 prefix_q = 24;
    u8 prefix_r;
    int rc;

    //add ipv4 on tap0
    rc = ipv46_config_add_remove("tap0", ip_q, prefix_q, false, true);
    assert_int_equal(rc, 0);

    //remove ipv4 on tap0
    rc = ipv46_config_add_remove("tap0", ip_q, prefix_q, false, false);
    assert_int_equal(rc, 0);

    //dump ipv4
    rc = ipv46_address_dump("tap0", ip_r, &prefix_r, false);
    assert_int_equal(rc, 0);

    assert_string_equal(ip_r, "0.0.0.0");
}

static void test_sc_ntoa(void **state)
{
    u8 buf[4] = {192, 168, 100, 44};
    char *res;

    res = sc_ntoa(buf);
    assert_string_equal(res, "192.168.100.44");
}

static void test_create_tapv2(void **state)
{
    tapv2_create_t query = {0};
    int rc;

    query.id = 1;
    query.use_random_mac = 1;

    rc = create_tapv2(&query);
    assert_int_equal(rc, 0);

    //TODO dump_tav2 and compare values

    rc = delete_tapv2("tap1");
    assert_int_equal(rc, 0);
}

int main()
{
    tapv2_create_t query = {0};
    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(test_enable_disable, NULL, NULL),
            cmocka_unit_test_setup_teardown(test_name2index, NULL, NULL),
            cmocka_unit_test_setup_teardown(test_add_ipv4, NULL, NULL),
            cmocka_unit_test_setup_teardown(test_remove_ipv4, NULL, NULL),
            cmocka_unit_test_setup_teardown(test_sc_ntoa, NULL, NULL),
            cmocka_unit_test_setup_teardown(test_create_tapv2, NULL, NULL),
    };

    if (sc_connect_vpp() != 0)
        fprintf(stderr, "Error connecting to VPP\n");

    /* Create interface tap0 to test several functions */
    query.id = 0;
    query.use_random_mac = 1;
    if (create_tapv2(&query) != 0)
        fprintf(stderr, "Error creating tap0\n");

    cmocka_run_group_tests(tests, NULL, NULL);

    /* Delete tap0 */
    if (delete_tapv2("tap0") != 0)
        fprintf(stderr, "Failed deleting tap0\n");

    return sc_disconnect_vpp();
}
