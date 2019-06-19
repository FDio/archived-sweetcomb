/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
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

#include <assert.h>

#include <scvpp/comm.h>
#include <scvpp/v3po.h>
#include <scvpp/interface.h>

// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_L2_API_JSON;
DEFINE_VAPI_MSG_IDS_TAPV2_API_JSON

/*
 * tap-v2 interfaces
 */

// Delete tapv2

VAPI_REQUEST_CB(tap_delete_v2);

static vapi_error_e bin_api_delete_tapv2(u32 sw_if_index)
{
    vapi_msg_tap_delete_v2 *mp;
    vapi_error_e rv;

    mp = vapi_alloc_tap_delete_v2(g_vapi_ctx);
    assert(NULL != mp);

    mp->payload.sw_if_index = sw_if_index;

    VAPI_CALL(vapi_tap_delete_v2(g_vapi_ctx, mp, tap_delete_v2_cb, NULL));
    if (rv != VAPI_OK)
        return -rv;

    return VAPI_OK;
}

int delete_tapv2(char *iface_name)
{
    uint32_t sw_if_index;
    vapi_error_e rv;
    int rc;

    rc = get_interface_id(iface_name, &sw_if_index);
    if (rc < 0)
        return rc;

    rv = bin_api_delete_tapv2(sw_if_index);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

// Create tapv2

VAPI_REQUEST_CB(tap_create_v2);

int create_tapv2(tapv2_create_t *query)
{
    vapi_msg_tap_create_v2 *mp;
    vapi_error_e rv;

    mp = vapi_alloc_tap_create_v2(g_vapi_ctx);
    assert(NULL != mp);

    memcpy(&mp->payload, query, sizeof(tapv2_create_t));

    VAPI_CALL(vapi_tap_create_v2(g_vapi_ctx, mp, tap_create_v2_cb, NULL));
    if (rv != VAPI_OK)
        return -EAGAIN;

    return 0;
}
