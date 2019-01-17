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

i32 sc_bridge_domain_add_del(u32 bd_id, u8 flood, u8 uu_flood, u8 forward, u8 learn, u8 arp_term, u8 mac_age, u8 bd_tag[VPP_TAG_LEN], u8 is_add) {
    i32 ret = -1;
    vapi_msg_bridge_domain_add_del *msg = vapi_alloc_bridge_domain_add_del(app_ctx);
    msg->payload.bd_id = bd_id;
    msg->payload.flood = flood;
    msg->payload.uu_flood = uu_flood;
    msg->payload.forward = forward;
    msg->payload.learn = learn;
    msg->payload.arp_term = arp_term;
    msg->payload.mac_age = mac_age;
    memcpy(msg->payload.bd_tag, bd_tag, VPP_TAG_LEN);
    msg->payload.is_add = is_add;
    vapi_msg_bridge_domain_add_del_hton(msg);

    vapi_error_e rv = vapi_send(app_ctx, msg);

    vapi_msg_bridge_domain_add_del_reply *resp;

    //SC_VPP_VAPI_RECV;

    vapi_msg_bridge_domain_add_del_reply_hton(resp);
    printf("createBridgedomain:%d \n", resp->payload.retval);
    ret = resp->payload.retval;
    vapi_msg_free(app_ctx, resp);
    return ret;
}

#define foreach_l2_bridge_domain_add_del_item \
  _get32("bd-id", bd_id, 0) \
  _get32("flood", flood, 0)    \
  _get32("uu-flood", uu_flood, 0)    \
  _get32("forward", forward, 0)    \
  _get32("learn", learn, 0)    \
  _get32("arp-term", arp_term, 0)    \
  _get32("mac-age", mac_age, 0)    \
  _get32("is-add", is_add, 0)


#define _get8(item, param, is_mandatory) \
  u8 _##param; \
  rc = sr_get_item(session, "/sc-l2-bridge-domain-add-del:sc-l2-bridge-domain-add-del/" item, &val); \
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
  rc = sr_get_item(session, "/sc-l2-bridge-domain-add-del:sc-l2-bridge-domain-add-del/" item, &val); \
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
netconf_bridge_domain_add_del_cb(sr_session_ctx_t *session, const char *module_name, sr_notif_event_t event, void *private_ctx) {
    SC_INVOKE_BEGIN;

    int rc = SR_ERR_OK;
    sr_val_t *val = NULL;

    if (SR_EV_APPLY != event) {
        return rc;
    }

    u8 _bd_tag[VPP_TAG_LEN] = {0};

    foreach_l2_bridge_domain_add_del_item


    rc = sr_get_item(session, "/sc-l2-bridge-domain-add-del:sc-l2-bridge-domain-add-del/bd_tag", &val);
    if (rc == SR_ERR_OK) {
        strncpy(_bd_tag, val->data.string_val, VPP_TAG_LEN);
        sr_free_val(val);
        val = NULL;
    }

    //call sc_ip_add_del_route
    int retVPP = sc_bridge_domain_add_del(_bd_id, _flood, _uu_flood, _forward, _learn, _arp_term, _mac_age, _bd_tag, _is_add);
    if (retVPP != 0) {
        SC_LOG_ERR("VPP birdge domain %s error : %d \n", _is_add ? "add" : "del", retVPP);
        rc = SR_ERR_OPERATION_FAILED;
    }
    goto sr_success;
    ;

sr_error:
    SC_LOG_ERR("netconf_bridge_domain_add_del_cb : %s \n", sr_strerror(rc));
sr_success:
    SC_INVOKE_END;

    return rc;
}

int sc_l2_bridge_domain_add_del_subscribe_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription) {
    SC_INVOKE_BEGIN;
    int rc = SR_ERR_OK;

    rc = sr_module_change_subscribe(session, "sc-l2-bridge-domain-add-del", netconf_bridge_domain_add_del_cb, NULL,
            0, SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE, subscription);

    if (SR_ERR_OK != rc) {
        SC_INVOKE_ENDX(sr_strerror(rc));
        return rc;
    }
    SC_INVOKE_END;
    return rc;
}


