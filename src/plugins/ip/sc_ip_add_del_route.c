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

#include "sc_ip.h"

i32 sc_ip_add_del_route(u32 next_hop_sw_if_index, u32 table_id, u32 classify_table_index, u32 next_hop_table_id, u32 next_hop_id,
        u8 is_add, u8 is_drop, u8 is_unreach, u8 is_prohibit, u8 is_ipv6, u8 is_local, u8 is_classify, u8 is_multipath,
        u8 is_resolve_host, u8 is_resolve_attached, u8 is_dvr, u8 is_source_lookup, u8 is_udp_encap,
        u8 next_hop_weight, u8 next_hop_preference, u8 next_hop_proto,
        u8 dst_address_length, u8 dst_address[VPP_IP6_ADDRESS_LEN], u8 next_hop_address[VPP_IP6_ADDRESS_LEN],
        u8 next_hop_n_out_labels, u32 next_hop_via_label, vapi_type_fib_mpls_label* next_hop_out_label_stack) {

    SC_INVOKE_BEGIN;

    i32 ret = -1;

    vapi_msg_ip_add_del_route *msg = vapi_alloc_ip_add_del_route(app_ctx, 0);

    msg->payload.next_hop_sw_if_index = next_hop_sw_if_index > 0 ? next_hop_sw_if_index : ~0;
    msg->payload.table_id = table_id;
    msg->payload.classify_table_index = classify_table_index > 0 ? classify_table_index : ~0;
    msg->payload.next_hop_table_id = next_hop_table_id;
    msg->payload.next_hop_id = next_hop_id;
    msg->payload.is_add = is_add;
    msg->payload.is_drop = is_drop;
    msg->payload.is_unreach = is_unreach;
    msg->payload.is_prohibit = is_prohibit;
    msg->payload.is_ipv6 = is_ipv6;
    msg->payload.is_local = is_local;
    msg->payload.is_classify = is_classify;
    msg->payload.is_multipath = is_multipath;
    msg->payload.is_resolve_host = is_resolve_host;
    msg->payload.is_resolve_attached = is_resolve_attached;
    msg->payload.is_dvr = is_dvr;
    msg->payload.is_source_lookup = is_source_lookup;
    msg->payload.is_udp_encap = is_udp_encap;
    msg->payload.next_hop_weight = next_hop_weight > 0 ? next_hop_weight : 1;
    msg->payload.next_hop_preference = next_hop_preference;
    msg->payload.next_hop_proto = next_hop_proto;
    msg->payload.dst_address_length = dst_address_length > 0 ? dst_address_length : 24;
    memcpy(msg->payload.dst_address, dst_address, VPP_IP6_ADDRESS_STRING_LEN);
    memcpy(msg->payload.next_hop_address, next_hop_address, VPP_IP6_ADDRESS_STRING_LEN);
    msg->payload.next_hop_n_out_labels = next_hop_n_out_labels;
    msg->payload.next_hop_via_label = MPLS_LABEL_INVALID;
    if (next_hop_n_out_labels > 0 && next_hop_out_label_stack != NULL)
        memcpy(msg->payload.next_hop_out_label_stack, next_hop_out_label_stack, sizeof (vapi_type_fib_mpls_label) * next_hop_n_out_labels);


    vapi_msg_ip_add_del_route_hton(msg);

    vapi_error_e rv = vapi_send(app_ctx, msg);

    vapi_msg_ip_add_del_route_reply *resp;

    //SC_VPP_VAPI_RECV;

    vapi_msg_ip_add_del_route_reply_hton(resp);
    SC_LOG_DBG("[TIP] %s route ok:%u", (is_add ? "add" : "del"), resp->payload.retval);

    ret = resp->payload.retval;
    vapi_msg_free(app_ctx, resp);

    SC_INVOKE_END;

    return ret;
}

#define foreach_ip_add_del_route_item \
  _get32("next-hop-sw-if-index", next_hop_sw_if_index, 0) \
  _get32("table-id", table_id, 0) \
  _get32("classify-table-index", classify_table_index, 0) \
  _get32("next-hop-table-id", next_hop_table_id, 0) \
  _get32("next-hop-id", next_hop_id, 0) \
  _get8("is-add", is_add, 0) \
  _get8("is-drop", is_drop, 0) \
  _get8("is-unreach", is_unreach, 0) \
  _get8("is-prohibit", is_prohibit, 0) \
  _get8("is-ipv6", is_ipv6, 0) \
  _get8("is-local", is_local, 0) \
  _get8("is-classify", is_classify, 0) \
  _get8("is-multipath", is_multipath, 0) \
  _get8("is-resolve-host", is_resolve_host, 0) \
  _get8("is-resolve-attached", is_resolve_attached, 0) \
  _get8("is-dvr", is_dvr, 0) \
  _get8("is-source-lookup", is_source_lookup, 0) \
  _get8("is-udp-encap", is_udp_encap, 0) \
  _get8("next-hop-weight", next_hop_weight, 0) \
  _get8("next-hop-preference", next_hop_preference, 0) \
  _get8("next-hop-proto", next_hop_proto, 0) \
  _get8("dst-address-length", dst_address_length, 0) \
  _get8("next-hop-n-out-labels", next_hop_n_out_labels, 0) \
  _get32("next-hop-via-label", next_hop_via_label, 0)

#define _get8(item, param, is_mandatory) \
  u8 _##param; \
  rc = sr_get_item(session, "/sc-ip-add-del-route:sc-ip-add-del-route/" item, &val); \
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
  rc = sr_get_item(session, "/sc-ip-add-del-route:sc-ip-add-del-route/" item, &val); \
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
netconf_ip_add_del_route_cb(sr_session_ctx_t *session, const char *module_name, sr_notif_event_t event, void *private_ctx) {
    SC_INVOKE_BEGIN;

    int rc = SR_ERR_OK;
    sr_val_t *val = NULL;

    if (SR_EV_APPLY != event) {
        return rc;
    }

    u8 _dst_address[16];
    u8 _next_hop_address[16];
    vapi_type_fib_mpls_label *_next_hop_out_label_stack;

    foreach_ip_add_del_route_item

    _dst_address_length = _dst_address_length > 0 ? _dst_address_length : 24;

    rc = sr_get_item(session, "/sc-ip-add-del-route:sc-ip-add-del-route/dst-address", &val);
    if (rc == SR_ERR_OK) {
        inet_pton(AF_INET, val->data.string_val, _dst_address);
        sr_free_val(val);
        val = NULL;
    } else {
        SC_LOG_ERR("**have no dst_address\n", "");
        goto sr_error;
    }

    rc = sr_get_item(session, "/sc-ip-add-del-route:sc-ip-add-del-route/next-hop-address", &val);
    if (rc == SR_ERR_OK) {
        inet_pton(AF_INET, val->data.string_val, _next_hop_address);
        sr_free_val(val);
        val = NULL;
    } else {
        SC_LOG_ERR("**have no next_hop_address\n", "");
        goto sr_error;
    }


    //call sc_ip_add_del_route
    SC_LOG_DBG("_dst_address:%08X", *(unsigned long*) _dst_address);
    SC_LOG_DBG("_next_hop_address:%08X", *(unsigned long*) _next_hop_address);

    sc_ip_add_del_route(_next_hop_sw_if_index, _table_id, _classify_table_index, _next_hop_table_id, _next_hop_id,
            _is_add, _is_drop, _is_unreach, _is_prohibit, _is_ipv6, _is_local, _is_classify, _is_multipath,
            _is_resolve_host, _is_resolve_attached, _is_dvr, _is_source_lookup, _is_udp_encap,
            _next_hop_weight, _next_hop_preference, _next_hop_proto,
            _dst_address_length, _dst_address, _next_hop_address,
            _next_hop_n_out_labels, _next_hop_via_label, NULL);
    goto sr_success;


sr_error:
    SC_LOG_ERR("netconf_ip_add_del_route_cb : %s \n", sr_strerror(rc));
sr_success:


    SC_INVOKE_END;

    return rc;
}

int sc_ip_subscribe_route_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription) {
    SC_INVOKE_BEGIN;
    int rc = SR_ERR_OK;

    rc = sr_module_change_subscribe(session, "sc-ip-add-del-route", netconf_ip_add_del_route_cb, NULL,
            0, SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE, subscription);

    if (SR_ERR_OK != rc) {
        SC_INVOKE_ENDX(sr_strerror(rc));
        return rc;
    }
    SC_INVOKE_END;
    return rc;
}


