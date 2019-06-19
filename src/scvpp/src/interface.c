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

#include <scvpp/comm.h>
#include <scvpp/interface.h>

#define IFACE_SUBSTR 49

// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_INTERFACE_API_JSON

static vapi_error_e
sw_interface_dump_cb(struct vapi_ctx_s *ctx, void *callback_ctx,
                     vapi_error_e rv, bool is_last,
                     vapi_payload_sw_interface_details *reply)
{
    UNUSED(rv); UNUSED(ctx); UNUSED(is_last);

    vapi_payload_sw_interface_details *passed;

    ARG_CHECK2(VAPI_EINVAL, callback_ctx, reply);

    //copy
    passed = (vapi_payload_sw_interface_details *) callback_ctx;
    *passed = *reply;

    return VAPI_OK;
}

static vapi_error_e
bin_api_sw_interface_dump(vapi_payload_sw_interface_details *details,
                           const char *iface_name)
{
    vapi_msg_sw_interface_dump *mp;
    vapi_error_e rv;

    mp = vapi_alloc_sw_interface_dump(g_vapi_ctx);
    assert(NULL != mp);

    /* Dump a specific interfaces */
    mp->payload.name_filter_valid = true;
    strncpy((char *)mp->payload.name_filter, iface_name, IFACE_SUBSTR);

    VAPI_CALL(vapi_sw_interface_dump(g_vapi_ctx, mp,
                                     sw_interface_dump_cb, details));
    if (rv != VAPI_OK)
        return -SCVPP_EINVAL;

    return rv;
}

int get_interface_id(const char *if_name, uint32_t *sw_if_index)
{
    vapi_payload_sw_interface_details details = {0};
    vapi_error_e rv;

    ARG_CHECK2(-SCVPP_EINVAL, if_name, sw_if_index);

    rv = bin_api_sw_interface_dump(&details, if_name);
    if (rv != VAPI_OK)
        return -SCVPP_EINVAL;

    if (strncmp(if_name, (char*) details.interface_name, VPP_INTFC_NAME_LEN)
        != 0)
        return -SCVPP_NOT_FOUND;

    *sw_if_index = details.sw_if_index;

    return 0;
}

/*
 * dump only a specific interface
 */
int interface_dump_iface(sw_interface_dump_t *details, const char *iface_name)
{
    vapi_error_e rv;

    rv = bin_api_sw_interface_dump(details, iface_name);
    if (rv != VAPI_OK)
        return -SCVPP_EINVAL;

    if (strncmp(iface_name, (char*) details->interface_name, VPP_INTFC_NAME_LEN)
        != 0)
        return -SCVPP_NOT_FOUND;

    return SCVPP_OK;
}

VAPI_DUMP_LIST_CB(sw_interface)

struct elt* interface_dump_all()
{
    struct elt* stack = NULL;
    vapi_msg_sw_interface_dump *mp;
    vapi_error_e rv;

    mp = vapi_alloc_sw_interface_dump(g_vapi_ctx);

    /* Dump all */
    mp->payload.name_filter_valid = false;
    memset(mp->payload.name_filter, 0, sizeof(mp->payload.name_filter));

    VAPI_CALL(vapi_sw_interface_dump(g_vapi_ctx, mp, sw_interface_all_cb,
                                     &stack));
    if (VAPI_OK != rv)
        return NULL;

    return stack;
}

VAPI_REQUEST_CB(sw_interface_set_flags);

int interface_enable(const char *interface_name, const bool enable)
{
    vapi_msg_sw_interface_set_flags *mp;
    uint32_t sw_if_index;
    vapi_error_e rv;
    int rc;

    ARG_CHECK(-SCVPP_EINVAL, interface_name);

    rc = get_interface_id(interface_name, &sw_if_index);
    if (rc != 0)
        return -SCVPP_NOT_FOUND;

    mp = vapi_alloc_sw_interface_set_flags(g_vapi_ctx);
    assert(NULL != mp);
    mp->payload.sw_if_index = sw_if_index;
    mp->payload.admin_up_down = enable;

    VAPI_CALL(vapi_sw_interface_set_flags(g_vapi_ctx, mp,
                                          sw_interface_set_flags_cb, NULL));
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return 0;
}

int get_interface_name(char *interface_name, uint32_t sw_if_index)
{
    struct elt *stack= NULL;
    sw_interface_dump_t *dump;
    int rc = -SCVPP_NOT_FOUND;

    stack = interface_dump_all();
    if (!stack)
        return -SCVPP_NOT_FOUND;

    foreach_stack_elt(stack) {
        dump = (sw_interface_dump_t *) data;

        if (dump->sw_if_index == sw_if_index) {
            strncpy(interface_name, (char *)dump->interface_name, VPP_INTFC_NAME_LEN);
            rc = SCVPP_OK;
        }

        free(dump);
    }

    return rc;
}
