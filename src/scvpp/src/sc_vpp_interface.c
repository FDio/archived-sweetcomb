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

#include "sc_vpp_interface.h"

#include <assert.h>
#include <stdbool.h>
#include <vapi/l2.api.vapi.h>

#include "sc_vpp_comm.h"

DEFINE_VAPI_MSG_IDS_INTERFACE_API_JSON

void sw_interface_details_query_set_name(sw_interface_details_query_t * query,
                                         const char * interface_name)
{
    assert(query && interface_name);

    memset(query, 0, sizeof(*query));

    strncpy((char*) query->sw_interface_details.interface_name, interface_name,
            sizeof(query->sw_interface_details.interface_name));
}

static vapi_error_e
get_interface_id_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                      vapi_error_e rv, bool is_last,
                      vapi_payload_sw_interface_details * reply)
{
    sw_interface_details_query_t *dctx = callback_ctx;
    assert(dctx);

    if (!dctx->interface_found)
    {
        if (is_last)
        {
            assert(NULL == reply);
        }
        else
        {
            assert(NULL != reply);
            SC_LOG_DBG("Interface dump entry: [%u]: %s\n", reply->sw_if_index,
            reply->interface_name);

            if (0 == strcmp((const char*)dctx->sw_interface_details.interface_name,
                            (const char*)reply->interface_name))
            {
                dctx->interface_found = true;
                dctx->sw_interface_details = *reply;
            }
        }
    }

    return VAPI_OK;
}

bool get_interface_id(sw_interface_details_query_t * sw_interface_details_query)
{
    ARG_CHECK(false, sw_interface_details_query);
    sw_interface_details_query->interface_found = false;

    vapi_msg_sw_interface_dump *mp = vapi_alloc_sw_interface_dump (g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.name_filter_valid = true;
    memcpy(mp->payload.name_filter, sw_interface_details_query->sw_interface_details.interface_name,
           sizeof(mp->payload.name_filter));

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_dump(g_vapi_ctx_instance, mp, get_interface_id_cb, sw_interface_details_query));

    if (VAPI_OK != rv) {
        SC_LOG_DBG_MSG("vapi_sw_interface_dump");
        return false;
    }

    if (!sw_interface_details_query->interface_found)
        SC_LOG_ERR("interface name %s: Can't find index",
            sw_interface_details_query->sw_interface_details.interface_name);

    return sw_interface_details_query->interface_found;
}

static vapi_error_e
get_interface_name_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                      vapi_error_e rv, bool is_last,
                      vapi_payload_sw_interface_details * reply)
{
    sw_interface_details_query_t *dctx = callback_ctx;
    assert(dctx);

    if (!dctx->interface_found)
    {
        if (is_last)
        {
            assert(NULL == reply);
        }
        else
        {
            assert(reply && dctx);
            SC_LOG_DBG("Interface dump entry: [%u]: %s\n", reply->sw_if_index, reply->interface_name);

            if (dctx->sw_interface_details.sw_if_index == reply->sw_if_index)
            {
                dctx->interface_found = true;
                dctx->sw_interface_details = *reply;
            }
        }
    }

    return VAPI_OK;
}

bool get_interface_name(sw_interface_details_query_t * sw_interface_details_query)
{
    ARG_CHECK(false, sw_interface_details_query);
    sw_interface_details_query->interface_found = false;

    vapi_msg_sw_interface_dump *mp = vapi_alloc_sw_interface_dump (g_vapi_ctx_instance);
    assert(NULL != mp);

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_dump (g_vapi_ctx_instance, mp, get_interface_name_cb, sw_interface_details_query));

    if (VAPI_OK != rv) {
        SC_LOG_DBG_MSG("vapi_sw_interface_dump");
        return false;
    }

    if (!sw_interface_details_query->interface_found)
        SC_LOG_ERR("interface index %u: Can't find name",
            sw_interface_details_query->sw_interface_details.sw_if_index);

    return sw_interface_details_query->interface_found;
}

static vapi_error_e
sw_interface_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                      vapi_error_e rv, bool is_last,
                      vapi_payload_sw_interface_details * reply)
{
    if (is_last)
    {
        assert (NULL == reply);
    }
    else
    {
        assert (NULL != reply);
        SC_LOG_DBG("Interface dump entry: [%u]: %s\n", reply->sw_if_index,
        reply->interface_name);
    }

    return VAPI_OK;
}

vapi_error_e bin_api_sw_interface_dump(const char * interface_name)
{
    vapi_msg_sw_interface_dump *mp = vapi_alloc_sw_interface_dump (g_vapi_ctx_instance);
    assert(NULL != mp);

    if (NULL != interface_name)
    {
        mp->payload.name_filter_valid = true;
        strncpy((char *)mp->payload.name_filter, interface_name, sizeof(mp->payload.name_filter));
    }
    else
    {
        mp->payload.name_filter_valid = false;
        memset(mp->payload.name_filter, 0, sizeof (mp->payload.name_filter));
    }

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_dump (g_vapi_ctx_instance, mp, sw_interface_dump_cb, NULL));

    return rv;
}


VAPI_RETVAL_CB(sw_interface_set_flags);

vapi_error_e bin_api_sw_interface_set_flags(uint32_t if_index, uint8_t up)
{
    vapi_msg_sw_interface_set_flags *mp = vapi_alloc_sw_interface_set_flags (g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.sw_if_index = if_index;
    mp->payload.admin_up_down = up;

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_set_flags(g_vapi_ctx_instance, mp, sw_interface_set_flags_cb, NULL));

    return rv;
}

VAPI_RETVAL_CB(sw_interface_set_l2_bridge);

vapi_error_e bin_api_sw_interface_set_l2_bridge(u32 bd_id, u32 rx_sw_if_index,
                                                bool enable)
{
    vapi_msg_sw_interface_set_l2_bridge *mp =
                                    vapi_alloc_sw_interface_set_l2_bridge (g_vapi_ctx_instance);
    assert(NULL != mp);
    //set interface l2 bridge <interface> <bridge-domain-id> [bvi|uu-fwd] [shg]
    /*
    typedef struct __attribute__ ((__packed__)) {
        u32 rx_sw_if_index;
        u32 bd_id;
        vapi_enum_l2_port_type port_type;
        u8 shg;
        u8 enable;
    } vapi_payload_sw_interface_set_l2_bridge;
    */
    mp->payload.enable = enable;
    mp->payload.bd_id = bd_id;
    mp->payload.rx_sw_if_index = rx_sw_if_index;

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_set_l2_bridge (g_vapi_ctx_instance, mp,
                                        sw_interface_set_l2_bridge_cb, NULL));

    return rv;
}

VAPI_COPY_CB(create_loopback)

vapi_error_e bin_api_create_loopback(vapi_payload_create_loopback_reply *reply)
{
    ARG_CHECK(VAPI_EINVAL, reply);

    vapi_msg_create_loopback *mp = vapi_alloc_create_loopback (g_vapi_ctx_instance);
    assert(NULL != mp);

    //mp->payload.mac_address =

    vapi_error_e rv;
    VAPI_CALL(vapi_create_loopback (g_vapi_ctx_instance, mp, create_loopback_cb, reply));

    return rv;
}
