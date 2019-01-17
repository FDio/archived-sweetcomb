/*
 * Copyright (c) 2018 HUACHENTEL and/or its affiliates.
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

#include <stdio.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sc_vxlan.h"

i32 sc_vxlan_add_del_tunnel(u32* tunnel_if_index, u8 is_add, u8 is_ipv6, i32 instance,
        u8 src_address[VPP_IP6_ADDRESS_LEN], u8 dst_address[VPP_IP6_ADDRESS_LEN],
        u32 mcast_sw_if_index, u32 encap_vrf_id, u32 decap_next_index, u32 vni) {
    SC_INVOKE_BEGIN;
    i32 ret = -1;

    vapi_msg_vxlan_add_del_tunnel *msg = vapi_alloc_vxlan_add_del_tunnel(app_ctx);
    if (NULL == msg) {

        SC_LOG_ERR("%s:%s", __FUNCTION__, "failed to call vapi_alloc_vxlan_add_del_tunnel");
        SC_INVOKE_ENDX(-1);
        return -1;
    }

    msg->payload.is_add = is_add;
    msg->payload.is_ipv6 = is_ipv6;
    msg->payload.instance = instance;
    memcpy(msg->payload.src_address, src_address, VPP_IP6_ADDRESS_LEN);
    memcpy(msg->payload.dst_address, dst_address, VPP_IP6_ADDRESS_LEN);
    msg->payload.vni = vni;
    //	msg->payload.dst_port = 4789;

    msg->payload.mcast_sw_if_index = mcast_sw_if_index;
    msg->payload.encap_vrf_id = encap_vrf_id;
    msg->payload.decap_next_index = decap_next_index;


    vapi_msg_vxlan_add_del_tunnel_hton(msg);

    vapi_error_e rv = vapi_send(app_ctx, msg);

    vapi_msg_vxlan_add_del_tunnel_reply *resp;
    //SC_VPP_VAPI_RECV

    vapi_msg_vxlan_add_del_tunnel_reply_hton(resp);

    SC_LOG_DBG("[TIP] %s vxlan tunnel ok,and detail info follows:", (is_add ? "add" : "del"));
    SC_LOG_DBG("[TIP]    src_address: %0x", *(unsigned long*) src_address);
    SC_LOG_DBG("[TIP]    dst_address: %0x", *(unsigned long*) dst_address);
    SC_LOG_DBG("[TIP]    resp retval: %d", resp->payload.retval);
    SC_LOG_DBG("[TIP]    resp sw_if_index: %d", resp->payload.sw_if_index);

    ret = resp->payload.retval;
    if (is_add && resp->payload.retval == 0) {
        //add success : return the new tunnel intfc index
        if (tunnel_if_index) {
            *tunnel_if_index = resp->payload.sw_if_index;
        }
    }
    vapi_msg_free(app_ctx, resp);
    SC_INVOKE_END;
    return ret;
}

static int rpc_vxlan_add_del_tunnel_cb(const char* xpath, const sr_val_t *input, const size_t input_cnt,
        sr_val_t **output, size_t *output_cnt, void *private_ctx) {
    int rc = SR_ERR_OK;
    sr_val_t *value = NULL;
    sr_session_ctx_t *session = (sr_session_ctx_t *) private_ctx;

    printf(">>>rpc input:\n");
    uint8_t is_add = 1;
    uint8_t is_ipv6 = 0;
    int32_t instance = -1;
    uint8_t src_address[16] = {0};
    uint8_t dst_address[16] = {0};
    uint32_t mcast_sw_if_index = ~0;
    uint32_t encap_vrf_id = 0;
    uint32_t decap_next_index = 1;
    uint32_t vni = 100;

    int protoFamily = AF_INET;

    for (size_t i = 0; i < input_cnt; ++i) {
        //sr_print_val(input + i);
        if (sc_end_with(input[i].xpath, "/is-add")) {
            is_add = input[i].data.uint8_val;
        } else if (sc_end_with(input[i].xpath, "/is-ipv6")) {
            is_ipv6 = input[i].data.uint8_val;
            if (is_ipv6)
                protoFamily = AF_INET6;
        } else if (sc_end_with(input[i].xpath, "/instance")) {
            instance = input[i].data.int32_val;
        } else if (sc_end_with(input[i].xpath, "/src-address")) {
            inet_pton(protoFamily, input[i].data.string_val, src_address);
        } else if (sc_end_with(input[i].xpath, "/dst-address")) {
            inet_pton(protoFamily, input[i].data.string_val, dst_address);
        } else if (sc_end_with(input[i].xpath, "/mcast-sw-if-index")) {
            mcast_sw_if_index = input[i].data.uint32_val;
        } else if (sc_end_with(input[i].xpath, "/encap-vrf-id")) {
            encap_vrf_id = input[i].data.uint32_val;
        } else if (sc_end_with(input[i].xpath, "/decap-next-index")) {
            decap_next_index = input[i].data.uint32_val;
        } else if (sc_end_with(input[i].xpath, "/vni")) {
            vni = input[i].data.uint32_val;
        } else {
            SC_LOG_ERR("%s failed: %s", SC_FUNCNAME, sr_strerror(rc));
        }
    }
    //printf("\n");

    SC_LOG_DBG("After Retrieval(by):%s", "");
    SC_LOG_DBG("             is_add: [%d]", is_add);
    SC_LOG_DBG("            is_ipv6: [%d]", is_ipv6);
    SC_LOG_DBG("           instance: [%d]", instance);
    SC_LOG_DBG("        src_address: [%08X]", *(unsigned long*) src_address);
    SC_LOG_DBG("        dst_address: [%08X]", *(unsigned long*) dst_address);
    SC_LOG_DBG("  mcast_sw_if_index: [%d]", mcast_sw_if_index);
    SC_LOG_DBG("       encap_vrf_id: [%d]", encap_vrf_id);
    SC_LOG_DBG("   decap_next_index: [%d]", decap_next_index);
    SC_LOG_DBG("                vni: [%d]", vni);

    int ret = 0;
    u32 tunnel_if_index;
    ret = sc_vxlan_add_del_tunnel(&tunnel_if_index, is_add, is_ipv6, instance, src_address, dst_address,
            mcast_sw_if_index, encap_vrf_id, decap_next_index, vni);
    if (ret == 0) {
        SC_LOG_DBG("VXLAN %s tunnel success!", (is_add == 1 ? "Add" : "Delete"));
    } else {
        SC_LOG_DBG("VXLAN %s tunnel fail: %d !", (is_add == 1 ? "Add" : "Delete"), ret);
    }

    //set the output data
    rc = sr_new_values(2, output);
    if (SR_ERR_OK != rc) {
        SC_INVOKE_ENDX(rc);
        return rc;
    }
    sr_val_set_xpath(&(*output)[0], "/sc-vxlan-add-del-tunnel:sc-vxlan-add-del-tunnel/retval");
    (*output)[0].type = SR_INT32_T;
    (*output)[0].data.int32_val = ret;
    sr_val_set_xpath(&(*output)[1], "/sc-vxlan-add-del-tunnel:sc-vxlan-add-del-tunnel/sw_if_index");
    (*output)[1].type = SR_UINT32_T;
    (*output)[1].data.uint32_val = tunnel_if_index;
    *output_cnt = 2;

    for (size_t i = 0; i < *output_cnt; ++i) {
        sr_print_val(&(*output)[i]);
    }
    SC_INVOKE_END;
    return SR_ERR_OK;
}

int sc_vxlan_subscribe_tunnel_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription) {
    SC_INVOKE_BEGIN;
    int rc = SR_ERR_OK;
    rc = sr_rpc_subscribe(session, "/sc-vxlan-add-del-tunnel:sc-vxlan-add-del-tunnel",
            rpc_vxlan_add_del_tunnel_cb, (void*) session, SR_SUBSCR_DEFAULT, subscription);
    if (SR_ERR_OK != rc) {
        SC_INVOKE_ENDX(sr_strerror(rc));
        return rc;
    }
    SC_INVOKE_END;
    return rc;
}


