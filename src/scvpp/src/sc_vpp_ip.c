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

DEFINE_VAPI_MSG_IDS_IP_API_JSON

VAPI_RETVAL_CB(sw_interface_add_del_address);

vapi_error_e
bin_api_sw_interface_add_del_address(u32 sw_if_index, bool is_add,
                                     const char * ip_address, u8 address_length)
{
    ARG_CHECK(VAPI_EINVAL, ip_address);

    vapi_msg_sw_interface_add_del_address *mp =
                                vapi_alloc_sw_interface_add_del_address (g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.sw_if_index = sw_if_index;
    mp->payload.is_add = is_add;
    mp->payload.is_ipv6 = 0;
    mp->payload.address_length = address_length;
    if (!sc_aton(ip_address, mp->payload.address,
        sizeof(mp->payload.address)))
        return VAPI_EINVAL;

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_add_del_address (g_vapi_ctx_instance, mp, sw_interface_add_del_address_cb, NULL));

    return rv;
}

vapi_error_e
bin_api_sw_interface_del_all_address(u32 sw_if_index)
{
    vapi_msg_sw_interface_add_del_address *mp =
                                vapi_alloc_sw_interface_add_del_address (g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.sw_if_index = sw_if_index;
    mp->payload.is_add = 0;
    mp->payload.del_all = 1;

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_add_del_address (g_vapi_ctx_instance, mp,
                                        sw_interface_add_del_address_cb, NULL));

    return rv;
}

VAPI_COPY_CB(ip_add_del_route)

vapi_error_e bin_api_ip_add_del_route(
    vapi_payload_ip_add_del_route_reply * reply,
    const char* dst_address,
    uint8_t dst_address_length,
    const char* next_hop,
    uint8_t is_add,
    uint32_t table_id,
    const char *interface_name)
{
    ARG_CHECK4(VAPI_EINVAL, reply, dst_address, next_hop, interface_name);

    SC_LOG_DBG("Interface: %s", interface_name);

    sw_interface_details_query_t query = {0};
    sw_interface_details_query_set_name(&query, interface_name);

    if (!get_interface_id(&query))
    {
        return VAPI_EINVAL;
    }

    vapi_msg_ip_add_del_route *mp = vapi_alloc_ip_add_del_route (g_vapi_ctx_instance, 1);
    assert(NULL != mp);

    //ip route add 2.2.2.2/24 via 5.5.5.5
    //show ip fib table 0 2.2.2.0/24 detail

    mp->payload.is_add = is_add;
    mp->payload.dst_address_length = dst_address_length;
    mp->payload.table_id = table_id;
    mp->payload.next_hop_sw_if_index = query.sw_interface_details.sw_if_index;
    SC_LOG_DBG("Interface: %s, index: %d", interface_name, query.sw_interface_details.sw_if_index);

    if (!sc_aton(dst_address, mp->payload.dst_address,
        sizeof(mp->payload.dst_address)))
        return VAPI_EINVAL;
    if (!sc_aton(next_hop, mp->payload.next_hop_address,
        sizeof(mp->payload.next_hop_address)))
        return VAPI_EINVAL;

    vapi_error_e rv ;
    VAPI_CALL(vapi_ip_add_del_route(g_vapi_ctx_instance, mp, ip_add_del_route_cb, reply));

    return rv;
}

static vapi_error_e
ip_fib_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                vapi_error_e rv, bool is_last,
                vapi_payload_ip_fib_details * reply)
{
    if (is_last)
    {
        assert (NULL == reply);
    }
    else
    {
        assert (NULL != reply);

        /*
        typedef struct __attribute__ ((__packed__)) {
            u32 table_id;
            u8 table_name[64];
            u8 address_length;
            u8 address[4];
            u32 count;
            u32 stats_index;
            vapi_type_fib_path path[0];
        } vapi_payload_ip_fib_details;
        */
        printf ("ip_fib_dump_cb table %u details: network %s/%u\n",
            reply->table_id, sc_ntoa(reply->address), reply->address_length);

        for (u32 i = 0; i < reply->count; ++i)
        {
            /*
            typedef struct __attribute__((__packed__)) {
                u32 sw_if_index;
                u32 table_id;
                u8 weight;
                u8 preference;
                u8 is_local;
                u8 is_drop;
                u8 is_udp_encap;
                u8 is_unreach;
                u8 is_prohibit;
                u8 is_resolve_host;
                u8 is_resolve_attached;
                u8 is_dvr;
                u8 is_source_lookup;
                u8 afi;
                u8 next_hop[16];
                u32 next_hop_id;
                u32 rpf_id;
                u32 via_label;
                u8 n_labels;
                vapi_type_fib_mpls_label label_stack[16];
            } vapi_type_fib_path;

   │493         case FIB_PATH_TYPE_ATTACHED_NEXT_HOP:                                                                                                                                                                                        │
b+>│494             s = format (s, "%U", format_ip46_address,                                                                                                                                                                                │
   │495                         &path->attached_next_hop.fp_nh,                                                                                                                                                                              │
   │496                         IP46_TYPE_ANY);

            */

           printf("\tnext hop: %s\n", sc_ntoa(reply->path[i].next_hop));
        }
    }

    return VAPI_OK;
}

vapi_error_e
bin_api_ip_fib_dump()
{
    vapi_msg_ip_fib_dump *mp = vapi_alloc_ip_fib_dump (g_vapi_ctx_instance);
    assert(NULL != mp);

    //ip route add 2.2.2.2/24 via 5.5.5.5
    //show ip fib table 0 2.2.2.0/24 detail

    vapi_error_e rv;
    VAPI_CALL(vapi_ip_fib_dump(g_vapi_ctx_instance, mp, ip_fib_dump_cb, NULL));

    return rv;
}

VAPI_RETVAL_CB(ip_table_add_del)

vapi_error_e
bin_api_table_add_del(u8 is_add, u32 table_id)
{
    vapi_msg_ip_table_add_del *mp = vapi_alloc_ip_table_add_del(g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.is_add = is_add;
    mp->payload.table_id = table_id;

    vapi_error_e rv;
    VAPI_CALL(vapi_ip_table_add_del(g_vapi_ctx_instance, mp, ip_table_add_del_cb, NULL));

    return rv;
}






