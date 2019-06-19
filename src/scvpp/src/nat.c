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
#include <scvpp/nat.h>

#include <assert.h>
#include <stdbool.h>


DEFINE_VAPI_MSG_IDS_NAT_API_JSON

static vapi_error_e
nat44_interface_dump_cb(struct vapi_ctx_s *ctx, void *callback_ctx,
                        vapi_error_e rv, bool is_last,
                        vapi_payload_nat44_interface_details *reply)
{
    UNUSED(ctx); UNUSED(rv);
    vapi_payload_nat44_interface_details *dctx = callback_ctx;
    assert(dctx);

    if (is_last)
    {
        assert(NULL == reply);
    }
    else
    {
        //TODO: Use LOG message for scvpp
//         SC_LOG_DBG("NAT Interface dump entry: [%u]: %u\n", reply->sw_if_index,
//                     reply->is_inside);
        *dctx = *reply;
    }

    return VAPI_OK;
}

static vapi_error_e
bin_api_nat44_interface_dump(vapi_payload_nat44_interface_details *reply)
{
    vapi_error_e rv;
    vapi_msg_nat44_interface_dump *mp;

    ARG_CHECK(VAPI_EINVAL, reply);

    mp = vapi_alloc_nat44_interface_dump(g_vapi_ctx);
    assert(NULL != mp);

    VAPI_CALL(vapi_nat44_interface_dump(g_vapi_ctx, mp,
                                        nat44_interface_dump_cb, reply));

    return rv;
}

VAPI_REQUEST_CB(nat44_add_del_interface_addr);

static vapi_error_e
bin_api_nat44_add_del_interface_addr(
    const vapi_payload_nat44_add_del_interface_addr *msg)
{
    vapi_error_e rv;
    vapi_msg_nat44_add_del_interface_addr *mp;

    ARG_CHECK(VAPI_EINVAL, msg);

    mp = vapi_alloc_nat44_add_del_interface_addr(g_vapi_ctx);
    assert(NULL != mp);

    mp->payload = *msg;

    VAPI_CALL(vapi_nat44_add_del_interface_addr(g_vapi_ctx, mp,
                                                nat44_add_del_interface_addr_cb,
                                                NULL));

    return rv;
}

VAPI_REQUEST_CB(nat44_add_del_address_range);

static vapi_error_e
bin_api_nat44_add_del_addr_range(
    const vapi_payload_nat44_add_del_address_range *range)
{
    vapi_error_e rv;
    vapi_msg_nat44_add_del_address_range *mp;

    ARG_CHECK(VAPI_EINVAL, range);

    mp = vapi_alloc_nat44_add_del_address_range(g_vapi_ctx);

    assert(NULL != mp);

    mp->payload = *range;

    VAPI_CALL(vapi_nat44_add_del_address_range(g_vapi_ctx, mp,
                                               nat44_add_del_address_range_cb,
                                               NULL));

    return rv;
}

VAPI_REQUEST_CB(nat44_add_del_static_mapping);

static vapi_error_e
bin_api_nat44_add_del_static_mapping(
    const vapi_payload_nat44_add_del_static_mapping *msg)
{
    vapi_error_e rv;
    vapi_msg_nat44_add_del_static_mapping *mp;

    ARG_CHECK(VAPI_EINVAL, msg);

    mp = vapi_alloc_nat44_add_del_static_mapping(g_vapi_ctx);
    assert(NULL != mp);

    mp->payload = *msg;

    VAPI_CALL(vapi_nat44_add_del_static_mapping(g_vapi_ctx, mp,
                                                nat44_add_del_static_mapping_cb,
                                                NULL));

    return rv;
}

static vapi_error_e nat44_static_mapping_dump_cb(
    struct vapi_ctx_s *ctx, void *callback_ctx, vapi_error_e rv,
    bool is_last,vapi_payload_nat44_static_mapping_details *reply)
{
    UNUSED(rv); UNUSED(ctx);
    vapi_payload_nat44_static_mapping_details *dctx = callback_ctx;
    assert(dctx);

    if (is_last)
    {
        assert(NULL == reply);
    }
    else
    {
        *dctx = *reply;
    }

    return VAPI_OK;
}

static vapi_error_e
bin_api_nat44_static_mapping_dump(
    vapi_payload_nat44_static_mapping_details *reply)
{
    vapi_error_e rv;
    vapi_msg_nat44_static_mapping_dump *msg;

    ARG_CHECK(VAPI_EINVAL, reply);

    msg = vapi_alloc_nat44_static_mapping_dump(g_vapi_ctx);
    assert(NULL != msg);

    VAPI_CALL(vapi_nat44_static_mapping_dump(g_vapi_ctx, msg,
                                             nat44_static_mapping_dump_cb,
                                             reply));

    return rv;
}

VAPI_REQUEST_CB(nat44_forwarding_enable_disable);

static vapi_error_e bin_api_nat44_forwarding_enable_disable(
    const vapi_payload_nat44_forwarding_enable_disable *msg)
{
    vapi_error_e rv;
    vapi_msg_nat44_forwarding_enable_disable *mp;

    ARG_CHECK(VAPI_EINVAL, msg);

    mp = vapi_alloc_nat44_forwarding_enable_disable(g_vapi_ctx);
    assert(NULL != mp);

    mp->payload = *msg;

    VAPI_CALL(vapi_nat44_forwarding_enable_disable(
        g_vapi_ctx, mp, nat44_forwarding_enable_disable_cb, NULL));

    return rv;
}

VAPI_REQUEST_CB(nat_set_workers);

static vapi_error_e
bin_api_nat_set_workers(const vapi_payload_nat_set_workers *msg)
{
    vapi_error_e rv;
    vapi_msg_nat_set_workers *mp;

    ARG_CHECK(VAPI_EINVAL, msg);

    mp = vapi_alloc_nat_set_workers(g_vapi_ctx);
    assert(NULL != mp);

    mp->payload = *msg;

    VAPI_CALL(vapi_nat_set_workers(g_vapi_ctx, mp, nat_set_workers_cb, NULL));

    return rv;
}

int nat44_interface_dump(nat44_interface_details_t *reply)
{
    vapi_error_e rv;

    rv = bin_api_nat44_interface_dump(reply);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

int nat44_add_del_interface_addr(const nat44_add_del_interface_addr_t *msg)
{
    vapi_error_e rv;

    rv = bin_api_nat44_add_del_interface_addr(msg);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

int nat44_add_del_addr_range(const nat44_add_del_address_range_t *range)
{
    vapi_error_e rv;

    rv = bin_api_nat44_add_del_addr_range(range);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

int nat44_add_del_static_mapping(const nat44_add_del_static_mapping_t *msg)
{
    vapi_error_e rv;

    rv = bin_api_nat44_add_del_static_mapping(msg);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

int nat44_static_mapping_dump(nat44_static_mapping_details_t *reply)
{
    vapi_error_e rv;

    rv = bin_api_nat44_static_mapping_dump(reply);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

int
nat44_forwarding_enable_disable(const nat44_forwarding_enable_disable_t *msg)
{
    vapi_error_e rv;

    rv = bin_api_nat44_forwarding_enable_disable(msg);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return 0;
}

int nat_set_workers(const nat_set_workers_t *msg)
{
    vapi_error_e rv;

    rv = bin_api_nat_set_workers(msg);
    if (VAPI_OK != rv)
        return -SCVPP_EINVAL;

    return SCVPP_OK;
}

