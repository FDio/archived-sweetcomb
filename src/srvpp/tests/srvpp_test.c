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

#include "srvpp.h"

#include <vpp/api/vpe_msg_enum.h>

#define vl_typedefs             /* define message structures */
#include <vpp/api/vpe_all_api_h.h>
#undef vl_typedefs

#define vl_endianfun
#include <vpp/api/vpe_all_api_h.h>
#undef vl_endianfun

/* instantiate all the print functions we know about */
#define vl_print(handle, ...)
#define vl_printfun
#include <vpp/api/vpe_all_api_h.h>
#undef vl_printfun

static int
srvpp_test_setup(void **state)
{
    srvpp_ctx_t *ctx = NULL;

    ctx = srvpp_get_ctx();
    assert_non_null(ctx);

    srvpp_setup_handler(CONTROL_PING_REPLY, control_ping_reply);
    srvpp_setup_handler(CREATE_LOOPBACK_REPLY, create_loopback_reply);
    srvpp_setup_handler(DELETE_LOOPBACK_REPLY, delete_loopback_reply);
    srvpp_setup_handler(SW_INTERFACE_DETAILS, sw_interface_details);

    *state = ctx;
    return 0;
}

static int
srvpp_test_teardown(void **state)
{
    srvpp_ctx_t *ctx = *state;

    srvpp_release_ctx(ctx);

    return 0;
}

static void
srvpp_ctx_test(void **state)
{
    srvpp_ctx_t *ctx = *state;
    assert_non_null(ctx);

    srvpp_ctx_t *ctx1 = NULL, *ctx2 = NULL;
    vl_api_control_ping_t *msg = NULL;
    int ret = 0;

    /* try to get additional contexts */
    ctx1 = srvpp_get_ctx();
    assert_non_null(ctx1);

    ctx2 = srvpp_get_ctx();
    assert_non_null(ctx2);

    assert_true(ctx == ctx1);
    assert_true(ctx1 == ctx2);

    /* send a control ping */
    msg = srvpp_alloc_msg(VL_API_CONTROL_PING, sizeof(*msg));
    assert_non_null(msg);

    ret = srvpp_send_request(ctx1, msg, NULL);
    assert_int_equal(ret, 0);

    /* release not needed contexts */
    srvpp_release_ctx(ctx1);
    srvpp_release_ctx(ctx2);
}

static void
srvpp_msg_test(void **state)
{
    srvpp_ctx_t *ctx = *state;
    assert_non_null(ctx);

    vl_api_create_loopback_t *create_loop_req = NULL;
    vl_api_create_loopback_reply_t *create_loop_reply = NULL;
    vl_api_delete_loopback_t *del_loop_req = NULL;
    vl_api_sw_interface_dump_t *if_dump_req = NULL;
    vl_api_sw_interface_details_t *if_details = NULL;
    void **details = NULL;
    size_t details_cnt = 0;
    u8 mac[6] = { 0, 10, 11, 12, 13, 14};
    int if_index = -1;
    int ret = 0;

    /* create a loopback interface */
    create_loop_req = srvpp_alloc_msg(VL_API_CREATE_LOOPBACK, sizeof(*create_loop_req));
    assert_non_null(create_loop_req);

    memcpy(create_loop_req->mac_address, mac, sizeof(mac));

    ret = srvpp_send_request(ctx, create_loop_req, (void**)&create_loop_reply);
    assert_int_equal(ret, 0);
    assert_non_null(create_loop_reply);
    if_index = ntohl(create_loop_reply->sw_if_index);

    printf("connected loopback interface, ifindex=%d\n", if_index);

    /* dump interfaces */
    if_dump_req = srvpp_alloc_msg(VL_API_SW_INTERFACE_DUMP, sizeof(*if_dump_req));
    assert_non_null(if_dump_req);

    ret = srvpp_send_dumprequest(ctx, if_dump_req, &details, &details_cnt);
    assert_int_equal(ret, 0);

    for (size_t i = 0; i < details_cnt; i++) {
        if_details = (vl_api_sw_interface_details_t *) details[i];
        printf("interface %s id=%d %s\n", if_details->interface_name, ntohl(if_details->sw_if_index), if_details->admin_up_down ? "up" : "down");
    }

    /* delete a loopback interface */
    del_loop_req = srvpp_alloc_msg(VL_API_DELETE_LOOPBACK, sizeof(*del_loop_req));
    assert_non_null(del_loop_req);

    del_loop_req->sw_if_index = htonl(if_index);

    ret = srvpp_send_request(ctx, del_loop_req, NULL);
    assert_int_equal(ret, 0);
}

int
main()
{
    const struct CMUnitTest tests[] = {
            cmocka_unit_test_setup_teardown(srvpp_ctx_test, srvpp_test_setup, srvpp_test_teardown),
            cmocka_unit_test_setup_teardown(srvpp_msg_test, srvpp_test_setup, srvpp_test_teardown),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
