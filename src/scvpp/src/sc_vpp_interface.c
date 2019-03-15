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

#include <assert.h>
#include <stdbool.h>

#include <vapi/l2.api.vapi.h>

#include "sc_vpp_comm.h"
#include "sc_vpp_interface.h"


// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_INTERFACE_API_JSON
DEFINE_VAPI_MSG_IDS_L2_API_JSON;

void sw_interface_details_query_set_name(sw_interface_details_query_t * query,
                                         const char * interface_name)
{
    assert(query && interface_name);

    memset(query, 0, sizeof(*query));

    strncpy((char*) query->sw_interface_details.interface_name, interface_name,
            sizeof(query->sw_interface_details.interface_name));
}

static vapi_error_e
sw_interface_dump_cb(struct vapi_ctx_s *ctx, void *callback_ctx,
                     vapi_error_e rv, bool is_last,
                     vapi_payload_sw_interface_details *reply)
{
    UNUSED(rv); UNUSED(ctx); UNUSED(is_last);

    vapi_payload_sw_interface_details *passed;

    ARG_CHECK(-EINVAL, callback_ctx);

    passed = (vapi_payload_sw_interface_details *) callback_ctx;

    //Interface is found if index of query equals index of reply
    if (passed->sw_if_index != reply->sw_if_index)
        return -EINVAL;

    //copy
    *passed = *reply;

    return VAPI_OK;
}

static vapi_error_e
bin_api_sw_interface_dump(vapi_payload_sw_interface_details *details)
{
    vapi_msg_sw_interface_dump *mp;
    vapi_error_e rv;

    mp = vapi_alloc_sw_interface_dump(g_vapi_ctx_instance);

    mp->payload.name_filter_valid = 0;
    memset(mp->payload.name_filter, 0, sizeof(mp->payload.name_filter));
    assert(NULL != mp);

    VAPI_CALL(vapi_sw_interface_dump(g_vapi_ctx_instance, mp, sw_interface_dump_cb, details));
    if (!rv)
        return -EINVAL;

    return rv;
}

static vapi_error_e
interface_dump_all_cb(struct vapi_ctx_s *ctx, void *callback_ctx,
                          vapi_error_e rv, bool is_last,
                          vapi_payload_sw_interface_details * reply)
{
    UNUSED(ctx); UNUSED(rv);
    dump_all_ctx *dctx = callback_ctx;

    if (is_last)
        return VAPI_OK;

    if(dctx->capacity == 0 && dctx->intfcArray == NULL) {
        dctx->capacity = 10;
        dctx->intfcArray = (vpp_interface_t*)malloc( sizeof(vpp_interface_t)*dctx->capacity );
    }
    if(dctx->num_ifs >= dctx->capacity-1) {
        dctx->capacity += 10;
        dctx->intfcArray = (vpp_interface_t*)realloc(dctx->intfcArray, sizeof(vpp_interface_t)*dctx->capacity );
    }

    vpp_interface_t * iface = &dctx->intfcArray[dctx->num_ifs];

    iface->sw_if_index = reply->sw_if_index;
    strncpy(iface->interface_name, (char*) reply->interface_name,
            VPP_INTFC_NAME_LEN);
    iface->l2_address_length = reply->l2_address_length;
    memcpy(iface->l2_address, reply->l2_address, reply->l2_address_length );
    iface->link_speed = reply->link_speed;

    iface->link_mtu = reply->link_mtu;
    iface->admin_up_down = reply->admin_up_down;
    iface->link_up_down = reply->link_up_down;

    dctx->num_ifs += 1;

    return VAPI_OK;
}

int interface_dump_all(dump_all_ctx * dctx)
{
    vapi_msg_sw_interface_dump *dump;
    vapi_error_e rv;

    ARG_CHECK(-1, dctx);

    if(dctx == NULL)
      return -1;

    dctx->intfcArray = NULL;
    dctx->capacity = 0;
    dctx->num_ifs = 0;

    dump = vapi_alloc_sw_interface_dump(g_vapi_ctx_instance);

    dump->payload.name_filter_valid = 0;
    memset(dump->payload.name_filter, 0, sizeof(dump->payload.name_filter));
    while (VAPI_EAGAIN ==
           (rv =
            vapi_sw_interface_dump(g_vapi_ctx_instance, dump, interface_dump_all_cb,
                                   dctx)));

    return dctx->num_ifs;
}

static vapi_error_e
get_interface_id_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                      vapi_error_e rv, bool is_last,
                      vapi_payload_sw_interface_details * reply)
{
    UNUSED(ctx); UNUSED(rv);

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

// return error code instead of boolean
int get_interface_id(sw_interface_details_query_t * sw_interface_details_query)
{
    vapi_error_e rv;

    ARG_CHECK(false, sw_interface_details_query);

    sw_interface_details_query->interface_found = false;

    vapi_msg_sw_interface_dump *mp = vapi_alloc_sw_interface_dump (g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.name_filter_valid = true;
    memcpy(mp->payload.name_filter, sw_interface_details_query->sw_interface_details.interface_name,
           sizeof(mp->payload.name_filter));

    VAPI_CALL(vapi_sw_interface_dump(g_vapi_ctx_instance, mp, get_interface_id_cb, sw_interface_details_query));
    if (VAPI_OK != rv)
        return false;

    return sw_interface_details_query->interface_found;
}

int get_interface_name(sw_interface_details_query_t *query)
{
    vapi_error_e rv;

    ARG_CHECK(-EINVAL, query);

    query->interface_found = false;

    rv = bin_api_sw_interface_dump(&query->sw_interface_details);
    if (rv == VAPI_OK)
        query->interface_found = true;

    return query->interface_found;
}

VAPI_RETVAL_CB(sw_interface_set_flags);

static vapi_error_e
bin_api_sw_interface_set_flags(uint32_t if_index, uint8_t up)
{
    vapi_msg_sw_interface_set_flags *mp = vapi_alloc_sw_interface_set_flags (g_vapi_ctx_instance);
    assert(NULL != mp);

    mp->payload.sw_if_index = if_index;
    mp->payload.admin_up_down = up;

    vapi_error_e rv;
    VAPI_CALL(vapi_sw_interface_set_flags(g_vapi_ctx_instance, mp, sw_interface_set_flags_cb, NULL));

    return rv;
}

int interface_enable(const char *interface_name, const bool enable)
{
    ARG_CHECK(-1, interface_name);

    int rc = 0;
    sw_interface_details_query_t query = {0};
    sw_interface_details_query_set_name(&query, interface_name);

    rc = get_interface_id(&query);
    if (!rc)
        return -1;

    rc = bin_api_sw_interface_set_flags(query.sw_interface_details.sw_if_index,
                                        enable);
    if (VAPI_OK != rc)
        return -1;

    return 0;
}
