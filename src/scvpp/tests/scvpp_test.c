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
#include <unistd.h>
#include <stdio.h>
#include <setjmp.h>

#include <scvpp/comm.h>
#include <scvpp/v3po.h>

#include "scvpp_test.h"

/* test "AAA.BBB.CCC.DDD" -> {A, B, C, D} */
static void test_sc_ntoa(void **state)
{
    UNUSED(state);
    u8 buf[4] = {192, 168, 100, 44};
    char *res;

    res = sc_ntoa(buf);
    assert_string_equal(res, "192.168.100.44");
}

/* test {A, B, C, D} -> "AAA.BBB.CCC.DDD" */
static void test_sc_aton(void **state)
{
    UNUSED(state);
    char ip[VPP_IP4_ADDRESS_STRING_LEN] = "192.168.100.44";
    uint8_t buf[4];
    int rc;

    rc = sc_aton(ip, buf, VPP_IP4_ADDRESS_LEN);
    assert_int_equal(rc, 0);

    assert_int_equal(buf[0], 192);
    assert_int_equal(buf[1], 168);
    assert_int_equal(buf[2], 100);
    assert_int_equal(buf[3], 44);
}

static int setup(void **state)
{
    UNUSED(state);
    tapv2_create_t query = {0};

    if (sc_connect_vpp() != 0) {
        fprintf(stderr, "Error connecting to VPP\n");
        return -1;
    }

    /* Create interface tap0 to test several functions */
    query.id = 0;
    query.use_random_mac = 1;
    if (create_tapv2(&query) != 0) {
        fprintf(stderr, "Error creating tap0\n");
        return -1;
    }

    return 0;
}

static int teardown(void **state)
{
    UNUSED(state);
    /* Delete tap0 */
    if (delete_tapv2("tap0") != 0) {
        fprintf(stderr, "Failed deleting tap0\n");
        return -1;
    }

    sc_disconnect_vpp();

    return 0;
}

/* return code for scvpp-test binary is the number of failed test */
int main()
{
    int rc = 0;

    const struct CMUnitTest common_tests[] = {
            cmocka_unit_test(test_sc_ntoa),
            cmocka_unit_test(test_sc_aton),
    };

    print_message("Common tests\n");
    rc |= cmocka_run_group_tests(common_tests, NULL, NULL);
    print_message("Interface tests\n");
    rc |= cmocka_run_group_tests(iface_tests, setup, teardown);
    print_message("IP tests\n");
    rc |= cmocka_run_group_tests(ip_tests, setup, teardown);
    print_message("NAT tests\n");
    rc |= cmocka_run_group_tests(nat_tests, setup, teardown);

    return rc;
}
