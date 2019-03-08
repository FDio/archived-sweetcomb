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

#include <assert.h>

#include "sc_vpp_comm.h"
#include "sc_vpp_v3po.h"
#include "sc_vpp_interface.h"

/*
 * tap-v2 interfaces
 */

DEFINE_VAPI_MSG_IDS_TAPV2_API_JSON

// Dump tapv2

//typedef struct __attribute__ ((__packed__)) {
//    u32 sw_if_index;
//    u32 id;
//    u8 dev_name[64];
//    u16 tx_ring_sz;
//    u16 rx_ring_sz;
//    u8 host_mac_addr[6];
//    u8 host_if_name[64];
//    u8 host_namespace[64];
//    u8 host_bridge[64];
//    u8 host_ip4_addr[4];
//    u8 host_ip4_prefix_len;
//    u8 host_ip6_addr[16];
//    u8 host_ip6_prefix_len;
//    u32 tap_flags;
//} vapi_payload_sw_interface_tap_v2_details;

// Delete tapv2

VAPI_RETVAL_CB(tap_delete_v2);

static vapi_error_e bin_api_delete_tapv2(u32 sw_if_index)
{
    vapi_msg_tap_delete_v2 *mp;
    vapi_error_e rv;

    mp = vapi_alloc_tap_delete_v2(g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.sw_if_index = sw_if_index;

    VAPI_CALL(vapi_tap_delete_v2(g_vapi_ctx_instance, mp, tap_delete_v2_cb,
                                 NULL));
    if (rv != VAPI_OK)
        return -EAGAIN;

    return rv;
}

int delete_tapv2(char *iface_name)
{
    int rc;
    sw_interface_details_query_t query = {0};

    sw_interface_details_query_set_name(&query, iface_name);

    rc = get_interface_id(&query);
    if (!rc)
        return -1;

    rc = bin_api_delete_tapv2(query.sw_interface_details.sw_if_index);
    if (VAPI_OK != rc)
        return -1;

    return 0;
}

// Create tapv2

VAPI_RETVAL_CB(tap_create_v2);

int create_tapv2(tapv2_create_t *query)
{
    vapi_msg_tap_create_v2 *mp;
    vapi_error_e rv;

    mp = vapi_alloc_tap_create_v2(g_vapi_ctx_instance);
    assert(NULL != mp);

    memcpy(&mp->payload, query, sizeof(tapv2_create_t));

    VAPI_CALL(vapi_tap_create_v2(g_vapi_ctx_instance, mp, tap_create_v2_cb,
                                 NULL));
    if (rv != VAPI_OK)
        return -EAGAIN;

    return 0;
}
