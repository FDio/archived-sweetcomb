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

#include "sc_l2.h"

i32 sc_interface_set_l2_bridge(u32 rx_sw_if_index, u32 bd_id, u8 shg, u8 bvi, u8 enable) {
    i32 ret = -1;
    vapi_msg_sw_interface_set_l2_bridge *msg = vapi_alloc_sw_interface_set_l2_bridge(app_ctx);
    msg->payload.rx_sw_if_index = rx_sw_if_index;
    msg->payload.bd_id = bd_id;
    msg->payload.shg = shg;
    msg->payload.shg = shg;
    //  msg->payload.bvi = bvi;
    msg->payload.enable = enable;
    vapi_msg_sw_interface_set_l2_bridge_hton(msg);

    vapi_error_e rv = vapi_send(app_ctx, msg);

    vapi_msg_bridge_domain_add_del_reply *resp;

//    SC_VPP_VAPI_RECV;

    vapi_msg_bridge_domain_add_del_reply_hton(resp);

    ret = resp->payload.retval;
    vapi_msg_free(app_ctx, resp);
    return ret;
}

#define foreach_l2_interface_set_l2_bridge_item \
  _get32("rx-sw-if-index", rx_sw_if_index, 1) \
  _get32("bd-id", bd_id, 1) \
  _get8("shg", shg, 0) \
  _get8("bvi", bvi, 0) \
  _get8("enable", enable, 0) 


#define _get8(item, param, is_mandatory) \
  u8 _##param; \
  rc = sr_get_item(session, "/sc-l2-interface-set-l2-bridge:sc-l2-interface-set-l2-bridge/" item, &val); \
  if(rc == SR_ERR_OK) \
    { \
      _##param = val->data.uint8_val; \
      SC_LOG_DBG("--recv " item ":%u", _##param); \
      sr_free_val(val); \
      val = NULL; \
    } \
  else \
      { \
        if(#is_mandatory) \
          { \
            goto sr_error; \
          } \
      } 

#define _get32(item, param, is_mandatory) \
  u32 _##param; \
  rc = sr_get_item(session, "/sc-l2-interface-set-l2-bridge:sc-l2-interface-set-l2-bridge/" item, &val); \
  if(rc == SR_ERR_OK) \
    { \
      _##param = val->data.uint32_val; \
      SC_LOG_DBG("--recv " item ":%u", _##param); \
      sr_free_val(val); \
      val = NULL; \
    } \
  else \
      { \
        if(#is_mandatory) \
          { \
            goto sr_error; \
          } \
      } 

static int
netconf_l2_interface_set_l2_bridge_cb(sr_session_ctx_t *session, const char *module_name, sr_notif_event_t event, void *private_ctx) {
    SC_INVOKE_BEGIN;

    int rc = SR_ERR_OK;
    sr_val_t *val = NULL;

    if (SR_EV_APPLY != event) {
        return rc;
    }

    foreach_l2_interface_set_l2_bridge_item

            //call sc_ip_add_del_route
            int retVPP = sc_interface_set_l2_bridge(_rx_sw_if_index, _bd_id, _shg, _bvi, _enable);
    if (retVPP != 0) {
        SC_LOG_ERR("VPP l2 interface set l2 birdge error : %d \n", retVPP);
        rc = SR_ERR_OPERATION_FAILED;
    }
    goto sr_success;
    ;

sr_error:
    SC_LOG_ERR("netconf_l2_interface_set_l2_bridge_cb : %s \n", sr_strerror(rc));
sr_success:
    SC_INVOKE_END;

    return rc;
}

int sc_l2_interface_set_l2_bridge_subscribe_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription) {
    SC_INVOKE_BEGIN;
    int rc = SR_ERR_OK;

    rc = sr_module_change_subscribe(session, "sc-l2-interface-set-l2-bridge", netconf_l2_interface_set_l2_bridge_cb, NULL,
            0, SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE, subscription);

    if (SR_ERR_OK != rc) {
        SC_INVOKE_ENDX(sr_strerror(rc));
        return rc;
    }
    SC_INVOKE_END;
    return rc;
}


