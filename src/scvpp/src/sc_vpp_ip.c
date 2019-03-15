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

#include "sc_vpp_comm.h"
#include "sc_vpp_ip.h"

#include "sc_vpp_interface.h"

#include <assert.h>

// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_IP_API_JSON

VAPI_RETVAL_CB(sw_interface_add_del_address);

static vapi_error_e
bin_api_sw_interface_add_del_address(u32 sw_if_index, bool is_add, bool is_ipv6,
                                     bool del_all, u8 address_length,
                                     const char *ip_address)
{
    vapi_msg_sw_interface_add_del_address *mp;
    vapi_error_e rv;

    ARG_CHECK(VAPI_EINVAL, ip_address);

    mp = vapi_alloc_sw_interface_add_del_address(g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.sw_if_index = sw_if_index;
    mp->payload.is_add = is_add;
    mp->payload.is_ipv6 = is_ipv6;
    mp->payload.del_all = del_all;
    mp->payload.address_length = address_length;
    if (sc_aton(ip_address, mp->payload.address, sizeof(mp->payload.address)))
        return VAPI_EINVAL;

    VAPI_CALL(vapi_sw_interface_add_del_address(g_vapi_ctx_instance, mp,
                                        sw_interface_add_del_address_cb, NULL));

    return rv;
}

VAPI_COPY_CB(ip_add_del_route)

static vapi_error_e
bin_api_ip_add_del_route(vapi_payload_ip_add_del_route_reply * reply,
                         const char* dst_address, uint8_t dst_address_length,
                         const char* next_hop, uint8_t is_add,
                         uint32_t table_id, const char *interface_name)
{
    sw_interface_details_query_t query = {0};
    vapi_msg_ip_add_del_route *mp;

    ARG_CHECK4(VAPI_EINVAL, reply, dst_address, next_hop, interface_name);

    sw_interface_details_query_set_name(&query, interface_name);

    if (!get_interface_id(&query))
        return VAPI_EINVAL;

    mp = vapi_alloc_ip_add_del_route (g_vapi_ctx_instance, 1);
    assert(NULL != mp);

    //ip route add 2.2.2.2/24 via 5.5.5.5
    //show ip fib table 0 2.2.2.0/24 detail

    mp->payload.is_add = is_add;
    mp->payload.dst_address_length = dst_address_length;
    mp->payload.table_id = table_id;
    mp->payload.next_hop_sw_if_index = query.sw_interface_details.sw_if_index;

    if (sc_aton(dst_address, mp->payload.dst_address,
        sizeof(mp->payload.dst_address)))
        return VAPI_EINVAL;
    if (sc_aton(next_hop, mp->payload.next_hop_address,
        sizeof(mp->payload.next_hop_address)))
        return VAPI_EINVAL;

    vapi_error_e rv ;
    VAPI_CALL(vapi_ip_add_del_route(g_vapi_ctx_instance, mp, ip_add_del_route_cb, reply));

    return rv;
}

static vapi_error_e
ip_address_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx, vapi_error_e rv,
                    bool is_last, vapi_payload_ip_address_details *reply)
{
    UNUSED(rv);
    ARG_CHECK3(VAPI_EINVAL, ctx, callback_ctx, reply);

    //copy dump reply in callback context
    if (!is_last && callback_ctx) {
        vapi_payload_ip_address_details *passed =
            (vapi_payload_ip_address_details *) callback_ctx;
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

    mp = vapi_alloc_ip_address_dump(g_vapi_ctx_instance);
    mp->payload.sw_if_index = sw_if_index;
    mp->payload.is_ipv6 = is_ipv6;

    VAPI_CALL(vapi_ip_address_dump(g_vapi_ctx_instance, mp, ip_address_dump_cb,
                                   dctx));
    if (rv != VAPI_OK)
        return rv;

    return VAPI_OK;
}

/*
 * @brief Dump IPv4/IPv6 address from an interface.
 * @param interface_name Name of the interface to dump.
 * @param ip_addr pointer where dump will store IP.
 * @param prefix_len pointer where dump will store prefix
 */
int ipv46_address_dump(const char *interface_name, char *ip_addr,
                       u8 *prefix_len, bool is_ipv6)
{
    vapi_payload_ip_address_details dctx = {0};
    sw_interface_details_query_t query = {0};
    vapi_error_e rv;

    sw_interface_details_query_set_name(&query, interface_name);

    if (!get_interface_id(&query))
        return -EINVAL;

    rv = bin_api_ip_address_dump(query.sw_interface_details.sw_if_index, is_ipv6, &dctx);
    if (rv != VAPI_OK)
        return -EAGAIN;

    strcpy(ip_addr, sc_ntoa(dctx.ip)); //IP string
    *prefix_len = dctx.prefix_length; //prefix length

    return 0;
}

/**
 * @brief Add or remove IPv4/IPv6 address to/from an interface.
 */
int ipv46_config_add_remove(const char *if_name, const char *addr,
                            uint8_t prefix, bool is_ipv6, bool add)
{
    vapi_error_e rv;
    int rc;

    ARG_CHECK2(-1, if_name, addr);

    /* get interface index */
    sw_interface_details_query_t query = {0};
    sw_interface_details_query_set_name(&query, if_name);
    rc = get_interface_id(&query);
    if (!rc)
        return -EINVAL;

    /* add del addr */
    rv = bin_api_sw_interface_add_del_address(query.sw_interface_details.sw_if_index,
                                              add, is_ipv6, 0, prefix, addr);
    if (rv != VAPI_OK)
        return -EINVAL;

    return 0;
}

/*
 * @brief Add or remove an IP route
 */
int ipv46_config_add_del_route(const char* dst_address, u8 dst_address_length,
                               const char* next_address, u8 is_add,
                               u32 table_id, const char *interface)
{
    vapi_payload_ip_add_del_route_reply reply = {0};
    vapi_error_e rv;

    rv = bin_api_ip_add_del_route(&reply, dst_address, dst_address_length,
                                  next_address, is_add, table_id, interface);
    if (VAPI_OK != rv || reply.retval > 0) {
        return -EINVAL;
    }

    return 0;
}
