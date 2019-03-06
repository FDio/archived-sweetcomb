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

#include <unistd.h>
#include <setjmp.h>
#include <stdarg.h>
#include <cmocka.h>

#include "scvpp_test.h"

#include <scvpp/interface.h>
#include <scvpp/v3po.h>

static void test_enable_disable(void **state)
{
    UNUSED(state);
    sw_interface_dump_t dump = {0};
    int rc;

    rc = interface_enable("tap0", 1);
    assert_int_equal(rc, SCVPP_OK);

    rc = interface_dump_iface(&dump, "tap0");
    assert_int_equal(rc, SCVPP_OK);

    assert_int_equal(dump.admin_up_down, true);

    rc = interface_enable("tap0", 0);
    assert_int_equal(rc, SCVPP_OK);
}

static void test_create_tapv2(void **state)
{
    UNUSED(state);
    tapv2_create_t query = {0};
    sw_interface_dump_t dump = {0};
    int rc;

    query.id = 1;
    query.use_random_mac = 1;

    rc = create_tapv2(&query);
    assert_int_equal(rc, SCVPP_OK);

    rc = interface_dump_iface(&dump, "tap1");
    assert_int_equal(rc, SCVPP_OK);
}

static int teardown_tapv2(void **state)
{
    UNUSED(state);
    return delete_tapv2("tap1");
}

static void test_dump_iface_all(void **state)
{
    UNUSED(state);
    struct elt *stack = NULL;
    sw_interface_dump_t *dump;
    bool exist = false;

    stack = interface_dump_all();
    assert_non_null(stack);
    foreach_stack_elt(stack) {
        dump = (sw_interface_dump_t *) data;
        if (!strncmp((char*) dump->interface_name, "tap0", VPP_INTFC_NAME_LEN))
            exist = true;
        free(dump);
    }
    assert_true(exist);
}

static void test_dump_iface_exist(void **state)
{
    UNUSED(state);
    vapi_payload_sw_interface_details details = {0};
    int rc;

    rc = interface_dump_iface(&details, "local0");
    assert_int_equal(rc, SCVPP_OK);

    assert_string_equal(details.interface_name, "local0");
}

static void test_dump_iface_unexist(void **state)
{
    UNUSED(state);
    vapi_payload_sw_interface_details details = {0};
    int rc;

    rc = interface_dump_iface(&details, "unexisting");
    assert_int_equal(rc, -SCVPP_NOT_FOUND);
}

static void test_get_interface_name(void **state)
{
    UNUSED(state);
    char interface_name[VPP_INTFC_NAME_LEN];
    uint32_t tap0_if_index;
    int rc;

    rc = get_interface_id("tap0", &tap0_if_index);
    assert_int_equal(rc, SCVPP_OK);

    rc = get_interface_name(interface_name, tap0_if_index);
    assert_int_equal(rc, SCVPP_OK);

    assert_string_equal(interface_name, "tap0");
}

const struct CMUnitTest iface_tests[IFACE_TEST_SIZE] = {
    cmocka_unit_test_teardown(test_create_tapv2, teardown_tapv2),
    cmocka_unit_test(test_enable_disable),
    cmocka_unit_test(test_dump_iface_all),
    cmocka_unit_test(test_dump_iface_exist),
    cmocka_unit_test(test_dump_iface_unexist),
    cmocka_unit_test(test_get_interface_name),
};
