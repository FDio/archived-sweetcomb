/*
 * Copyright (c) 2018 PANTHEON.tech.
 *
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

#include <scvpp/comm.h>
#include <scvpp/ip.h>
#include <scvpp/interface.h>

#include <assert.h>
#include <stdio.h>

// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_IP_API_JSON

VAPI_REQUEST_CB(sw_interface_add_del_address);

static vapi_error_e
bin_api_sw_interface_add_del_address(u32 sw_if_index, bool is_add, bool is_ipv6,
                                     bool del_all, u8 address_length,
                                     const char *ip_address)
{
    vapi_msg_sw_interface_add_del_address *mp;
    vapi_error_e rv;

    ARG_CHECK(VAPI_EINVAL, ip_address);

    mp = vapi_alloc_sw_interface_add_del_address(g_vapi_ctx);
    assert(NULL != mp);

    mp->payload.sw_if_index = sw_if_index;
    mp->payload.is_add = is_add;
    mp->payload.is_ipv6 = is_ipv6;
    mp->payload.del_all = del_all;
    mp->payload.address_length = address_length;
    if (sc_aton(ip_address, mp->payload.address, VPP_IP4_ADDRESS_LEN))
        return VAPI_EINVAL;

    VAPI_CALL(vapi_sw_interface_add_del_address(g_vapi_ctx, mp,
                                        sw_interface_add_del_address_cb, NULL));

    return rv;
}

VAPI_REQUEST_CB(ip_add_del_route)

static vapi_error_e
bin_api_ip_add_del_route(vapi_payload_ip_add_del_route_reply * reply,
                         const char* dst_address, uint8_t dst_address_length,
                         const char* next_hop, uint8_t is_add,
                         uint32_t table_id, const char *next_interface)
{
    vapi_msg_ip_add_del_route *mp;
    uint32_t sw_if_index;
    vapi_error_e rv ;
    int rc;

    ARG_CHECK2(VAPI_EINVAL, reply, dst_address);

    //Require interface or next hop IP or both
    if (!next_interface && !next_hop)
        return VAPI_EINVAL;

    mp = vapi_alloc_ip_add_del_route(g_vapi_ctx, 1);
    assert(NULL != mp);

    if (next_interface) {
        rc = get_interface_id(next_interface, &sw_if_index);
        if (rc < 0)
            return VAPI_EINVAL;
    }

    mp->payload.is_add = is_add;
    mp->payload.table_id = table_id;
    mp->payload.is_ipv6 = false;
    mp->payload.is_local = false;
    sc_aton(dst_address, mp->payload.dst_address, VPP_IP4_ADDRESS_LEN);
    mp->payload.dst_address_length = dst_address_length;
    if (next_interface) //interface is not mandatory
        mp->payload.next_hop_sw_if_index = sw_if_index;
    if (next_hop) //next hop ip is not mandatory
        sc_aton(next_hop, mp->payload.next_hop_address, VPP_IP4_ADDRESS_LEN);

    VAPI_CALL(vapi_ip_add_del_route(g_vapi_ctx, mp,
                                    ip_add_del_route_cb, reply));

    return rv;
}

static vapi_error_e
ip_address_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx, vapi_error_e rv,
                    bool is_last, vapi_payload_ip_address_details *reply)
{
    UNUSED(rv);
    vapi_payload_ip_address_details *passed;

    ARG_CHECK3(VAPI_EINVAL, ctx, callback_ctx, reply);

    //copy dump reply in callback context
    if (!is_last) {
        passed = (vapi_payload_ip_address_details *) callback_ctx;
        *passed = *reply;
    }

    return VAPI_OK;
}

static vapi_error_e
bin_api_ip_address_dump(u32 sw_if_index, bool is_ipv6,
                        vapi_payload_ip_address_details *dctx)
{
    vapi_msg_ip_address_dump *mp;
    vapi_error_e rv;

    mp = vapi_alloc_ip_address_dump(g_vapi_ctx);
    assert(mp != NULL);

    mp->payload.sw_if_index = sw_if_index;
    mp->payload.is_ipv6 = is_ipv6;

    VAPI_CALL(vapi_ip_address_dump(g_vapi_ctx, mp, ip_address_dump_cb,
                                   dctx));
    if (rv != VAPI_OK)
        return rv;

    return VAPI_OK;
}

///VAPI_DUMP_LIST_CB(ip_fib); can not be used because of path flexible array

static vapi_error_e
ip_fib_all_cb(vapi_ctx_t ctx, void *caller_ctx, vapi_error_e rv, bool is_last,
              vapi_payload_ip_fib_details *reply)
{
    UNUSED(ctx); UNUSED(rv); UNUSED(is_last);
    struct elt **stackp;
    ARG_CHECK2(VAPI_EINVAL, caller_ctx, reply);

    stackp = (struct elt**) caller_ctx;
    push(stackp, reply, sizeof(*reply)+reply->count*sizeof(vapi_type_fib_path));

    return VAPI_OK;
}

struct elt* ipv4_fib_dump_all()
{
    struct elt *stack = NULL;
    vapi_msg_ip_fib_dump *mp;
    vapi_error_e rv;

    mp = vapi_alloc_ip_fib_dump(g_vapi_ctx);
    assert(mp != NULL);

    VAPI_CALL(vapi_ip_fib_dump(g_vapi_ctx, mp, ip_fib_all_cb, &stack));
    if(VAPI_OK != rv)
        return NULL;

    return stack;
}

int ipv4_fib_dump_prefix(const char *prefix_xpath, fib_dump_t **reply)
{
    struct elt *stack = NULL;
    char prefix[VPP_IP4_PREFIX_STRING_LEN];
    fib_dump_t *dump;
    int rc = -SCVPP_NOT_FOUND;

    stack = ipv4_fib_dump_all();
    if (!stack)
        return rc;

    foreach_stack_elt(stack) {
        dump = (fib_dump_t *) data;

        if (rc == -SCVPP_NOT_FOUND) {
            snprintf(prefix, VPP_IP4_PREFIX_STRING_LEN, "%s/%u",
                     sc_ntoa(dump->address), dump->address_length);
            if (!strncmp(prefix_xpath, prefix, VPP_IP4_PREFIX_STRING_LEN)) {
                *reply = dump;
                rc = SCVPP_OK;
                continue;
            }
        }

        free(dump);
    }

    return rc;
}

int ipv46_address_dump(const char *interface_name, char *ip_addr,
                       u8 *prefix_len, bool is_ipv6)
{
    vapi_payload_ip_address_details dctx = {0};
    uint32_t sw_if_index;
    vapi_error_e rv;
    int rc;

    rc = get_interface_id(interface_name, &sw_if_index);
    if (rc < 0)
        return rc;

    rv = bin_api_ip_address_dump(sw_if_index, is_ipv6, &dctx);
    if (rv != VAPI_OK)
        return -SCVPP_EINVAL;

    strcpy(ip_addr, sc_ntoa(dctx.ip)); //IP string
    *prefix_len = dctx.prefix_length; //prefix length

    return SCVPP_OK;
}

int ipv46_config_add_remove(const char *if_name, const char *addr,
                            uint8_t prefix, bool is_ipv6, bool add)
{
    vapi_error_e rv;
    uint32_t sw_if_index;
    int rc;

    ARG_CHECK2(-1, if_name, addr);

    rc = get_interface_id(if_name, &sw_if_index);
    if (rc < 0)
        return rc;

    /* add del addr */
    rv = bin_api_sw_interface_add_del_address(sw_if_index, add, is_ipv6, 0,
                                              prefix, addr);
    if (rv != VAPI_OK)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

int ipv46_config_add_del_route(const char* dst_address, u8 dst_address_length,
                               const char* next_address, u8 is_add,
                               u32 table_id, const char *interface)
{
    vapi_payload_ip_add_del_route_reply reply = {0};
    vapi_error_e rv;

    rv = bin_api_ip_add_del_route(&reply, dst_address, dst_address_length,
                                  next_address, is_add, table_id, interface);
    if (VAPI_OK != rv || reply.retval > 0)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}
