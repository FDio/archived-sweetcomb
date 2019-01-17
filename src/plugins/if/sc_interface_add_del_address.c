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

#include "sc_interface.h"

static i32
sc_interface_add_del_addr(u32 sw_if_index, u8 is_add, u8 is_ipv6, u8 del_all,
        u8 address_length, u8 address[VPP_IP6_ADDRESS_LEN]);

static int
sc_interface_add_del_address_cb(sr_session_ctx_t *session, const char *module_name,
        sr_notif_event_t event, void *private_app_ctx);

int
sc_interface_add_del_address_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription) {
    SC_INVOKE_BEGIN;
    int rc = SR_ERR_OK;
    rc = sr_module_change_subscribe(session, "sc-interface-add-del-address", sc_interface_add_del_address_cb, NULL, 0,
            SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE, subscription);
    if (SR_ERR_OK != rc) {
        SC_INVOKE_ENDX(sr_strerror(rc));
    } else {
        SC_INVOKE_END;
    }
    return rc;
}
#define SC_INT_ADD_DEL_ADDR_SW_IF_INDEX   "/sc-interface-add-del-address:sc-interface-add-del-address/sw-if-index"
#define SC_INT_ADD_DEL_ADDR_IS_ADD "/sc-interface-add-del-address:sc-interface-add-del-address/is-add"
#define SC_INT_ADD_DEL_ADDR_IS_IPV6 "/sc-interface-add-del-address:sc-interface-add-del-address/is-ipv6"
#define SC_INT_ADD_DEL_ADDR_DEL_ALL "/sc-interface-add-del-address:sc-interface-add-del-address/del-all"
#define SC_INT_ADD_DEL_ADDR_ADDRESS_LENGTH "/sc-interface-add-del-address:sc-interface-add-del-address/address-length"
#define SC_INT_ADD_DEL_ADDR_ADDRESS "/sc-interface-add-del-address:sc-interface-add-del-address/address"

int
sc_interface_add_del_address_cb(sr_session_ctx_t *session, const char *module_name,
        sr_notif_event_t event, void *private_app_ctx) {
    SC_INVOKE_BEGIN;
    int rc = SR_ERR_OK;
    if (SR_EV_APPLY != event) {
        return rc;
    }

    u32 sw_if_index; //the index of a interface
    u8 is_add = 1; //range:{0,1},default=1
    u8 is_ipv6 = 0; //range:{0,1},default=0
    u8 del_all = 1; //delete all current address,default=1(when add a new address , del_all should be 0)
    u8 address_length = 24; //netmask length,default=24
    u8 address[VPP_IP6_ADDRESS_LEN]; //type inet:ip-address,new address  
    ////////////////////////////////////////////////////////////////////////////
    //retrieval items
    sr_val_t *val = NULL;
    //01
    rc = sr_get_item(session, SC_INT_ADD_DEL_ADDR_SW_IF_INDEX, &val);
    if (rc != SR_ERR_OK) {
        goto sr_error;
    }
    sw_if_index = val->data.uint32_val;
    sr_free_val(val);
    val = NULL;
    //02
    rc = sr_get_item(session, SC_INT_ADD_DEL_ADDR_IS_ADD, &val);
    if (rc == SR_ERR_OK) {
        is_add = val->data.uint8_val;
        sr_free_val(val);
        val = NULL;
    }
    //03
    rc = sr_get_item(session, SC_INT_ADD_DEL_ADDR_IS_IPV6, &val);
    if (rc == SR_ERR_OK) {
        is_ipv6 = val->data.uint8_val;
        sr_free_val(val);
        val = NULL;
    }
    //04
    rc = sr_get_item(session, SC_INT_ADD_DEL_ADDR_DEL_ALL, &val);
    if (rc == SR_ERR_OK) {
        del_all = val->data.uint8_val;
        sr_free_val(val);
        val = NULL;
    }
    //05
    rc = sr_get_item(session, SC_INT_ADD_DEL_ADDR_ADDRESS_LENGTH, &val);
    if (rc == SR_ERR_OK) {
        address_length = val->data.uint8_val;
        sr_free_val(val);
        val = NULL;
    }
    //06
    rc = sr_get_item(session, SC_INT_ADD_DEL_ADDR_ADDRESS, &val);
    if (rc == SR_ERR_OK) {
        //parse address
        if (1 == is_ipv6) {
            inet_pton(AF_INET6, val->data.string_val, address);
        } else {
            inet_pton(AF_INET, val->data.string_val, address);
        }
        sr_free_val(val);
        val = NULL;
    }

    SC_LOG_DBG("After [%s] Retrieves:", SC_FUNCNAME);
    SC_LOG_DBG("         sw_if_index: [%d]", sw_if_index);

    sc_interface_add_del_addr(sw_if_index, is_add, is_ipv6, del_all, address_length, address);

    SC_INVOKE_END;
    return SR_ERR_OK;
sr_error:
    SC_LOG_ERR("[%s] unsupported item: %s \n", SC_FUNCNAME, sr_strerror(rc));
    return rc;
}

i32 sc_interface_add_del_addr(u32 sw_if_index, u8 is_add, u8 is_ipv6, u8 del_all,
        u8 address_length, u8 address[VPP_IP6_ADDRESS_LEN]) {
    i32 ret = -1;
    vapi_msg_sw_interface_add_del_address *msg = vapi_alloc_sw_interface_add_del_address(app_ctx);
    msg->payload.sw_if_index = sw_if_index;
    msg->payload.is_add = is_add;
    msg->payload.is_ipv6 = is_ipv6;
    msg->payload.del_all = del_all;
    msg->payload.address_length = address_length;
    memcpy(msg->payload.address, address, VPP_IP6_ADDRESS_LEN);
    vapi_msg_sw_interface_add_del_address_hton(msg);

    vapi_error_e rv = vapi_send(app_ctx, msg);

    vapi_msg_sw_interface_add_del_address_reply *resp;

//    SC_VPP_VAPI_RECV;

    vapi_msg_sw_interface_add_del_address_reply_hton(resp);
    printf("[SSDSD] addDelInterfaceAddr : %d \n", resp->payload.retval);
    ret = resp->payload.retval;
    vapi_msg_free(app_ctx, resp);
    return ret;
}
