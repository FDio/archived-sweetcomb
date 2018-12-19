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

#include "openconfig_interfaces.h"
#include "sys_util.h"
#include "sc_vpp_comm.h"

#include "../bapi/bapi.h"
#include "../bapi/bapi_interface.h"
#include "../bapi/bapi_ip.h"

#include <assert.h>
#include <string.h>
#include <sysrepo/xpath.h>
#include <sysrepo/values.h>

#define XPATH_SIZE 2000

static int oi_enable_interface(const char *interface_name,
                               const bool enable)
{
    ARG_CHECK(-1, interface_name);

    int rc = 0;
    sw_interface_details_query_t query = {0};
    sw_interface_details_query_set_name(&query, interface_name);

    if (false == get_interface_id(&query)) {
        return -1;
    }

    rc = bin_api_sw_interface_set_flags(query.sw_interface_details.sw_if_index,
                                        enable);
    if (VAPI_OK != rc) {
        SRP_LOG_ERR_MSG("Call bin_api_sw_interface_set_flags.");
        rc = -1;
    }

    return rc;
}

// XPATH: /openconfig-interfaces:interfaces/interface[name='%s']/config/
int openconfig_interfaces_interfaces_interface_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};
    int rc = 0;

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    // if we receive event SR_EV_APPLY - config has changed

    log_recv_event(event, "subtree_change_cb received");
    SRP_LOG_DBG("[SRP_LOG_DBG] xpath: %s", xpath);

    // apply configuration
    //
    //
    // in this type i guess we can call sr_get_item()
    // because we are subscribed to leaf we shouldn't
    // get more than one change in one callback
    // each chage represent change in a hierarchy of
    // xml document
    //
    if (event != SR_EV_APPLY) {
        return SR_ERR_OK;
    }

    // first we get event VERIFY
    // then  we get event APPLY
    //  - we can   return error from VERIFY event
    //  - we can't return error from APPLY event

    if (sr_get_changes_iter(ds, (char *)xpath, &it) != SR_ERR_OK) {
        // in example he calls even on fale
        sr_free_change_iter(it);
        return SR_ERR_OK;
    }

    while (sr_get_change_next(ds, it, &oper,
        &old_val, &new_val) == SR_ERR_OK) {

        log_recv_oper(oper, "subtree_change_cb received");

        SRP_LOG_DBG("xpath: %s", new_val->xpath);

        sr_xpath_ctx_t state = {0};

        tmp = xpath_find_first_key(new_val->xpath, "name", &state);
        if (NULL == tmp) {
            SRP_LOG_DBG_MSG("interface_name NOT found.");
            continue;
        }

        strncpy(interface_name, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        switch (oper) {
            case SR_OP_CREATED:
                if (sr_xpath_node_name_eq(new_val->xpath, "name")) {
                    //TODO: LEAF: name, type: string
                } else if(sr_xpath_node_name_eq(new_val->xpath, "type")) {
                    //TODO: LEAF: type, type: identityref
                } else if(sr_xpath_node_name_eq(new_val->xpath, "mtu")) {
                    //TODO: LEAF: mtu, type: uint16
                } else if(sr_xpath_node_name_eq(new_val->xpath,
                    "loopback-mode")) {
                    //TODO: LEAF: loopback-mode, type: boolean
                } else if(sr_xpath_node_name_eq(new_val->xpath,
                                                            "description")) {
                    //TODO: LEAF: description, type: string
                } else if(sr_xpath_node_name_eq(new_val->xpath, "enabled")) {
                    rc = oi_enable_interface(interface_name,
                                             new_val->data.bool_val);
                } else if(sr_xpath_node_name_eq(new_val->xpath,
                                                 "oc-vlan:tpid")) {
                    //TODO: LEAF: oc-vlan:tpid, type: identityref
                }
                break;

            case SR_OP_MODIFIED:
                if (sr_xpath_node_name_eq(new_val->xpath, "name")) {
                    //TODO: LEAF: name, type: string
                } else if(sr_xpath_node_name_eq(new_val->xpath, "type")) {
                    //TODO: LEAF: type, type: identityref
                } else if(sr_xpath_node_name_eq(new_val->xpath, "mtu")) {
                    //TODO: LEAF: mtu, type: uint16
                } else if(sr_xpath_node_name_eq(new_val->xpath,
                    "loopback-mode")) {
                    //TODO: LEAF: loopback-mode, type: boolean
                } else if(sr_xpath_node_name_eq(new_val->xpath,
                                                    "description")) {
                    //TODO: LEAF: description, type: string
                } else if(sr_xpath_node_name_eq(new_val->xpath, "enabled")) {
                    rc = oi_enable_interface(interface_name,
                                             new_val->data.bool_val);
                } else if(sr_xpath_node_name_eq(new_val->xpath,
                                                 "oc-vlan:tpid")) {
                    //TODO: LEAF: oc-vlan:tpid, type: identityref
                }
                break;

            case SR_OP_MOVED:
                break;

            case SR_OP_DELETED:
                if (sr_xpath_node_name_eq(old_val->xpath, "name")) {
                    //TODO: LEAF: name, type: string
                } else if(sr_xpath_node_name_eq(old_val->xpath, "type")) {
                    //TODO: LEAF: type, type: identityref
                } else if(sr_xpath_node_name_eq(old_val->xpath, "mtu")) {
                    //TODO: LEAF: mtu, type: uint16
                } else if(sr_xpath_node_name_eq(old_val->xpath,
                    "loopback-mode")) {
                    //TODO: LEAF: loopback-mode, type: boolean
                } else if(sr_xpath_node_name_eq(old_val->xpath,
                                                    "description")) {
                    //TODO: LEAF: description, type: string
                } else if(sr_xpath_node_name_eq(old_val->xpath, "enabled")) {
                    rc = oi_enable_interface(interface_name, false);
                } else if(sr_xpath_node_name_eq(old_val->xpath,
                                                 "oc-vlan:tpid")) {
                    //TODO: LEAF: oc-vlan:tpid, type: identityref
                }
                break;
        }

        if (0 != rc) {
            sr_xpath_recover(&state);

            sr_free_val(old_val);
            sr_free_val(new_val);

            sr_free_change_iter(it);

            return SR_ERR_OPERATION_FAILED;
        }

        sr_xpath_recover(&state);

        sr_free_val(old_val);
        sr_free_val(new_val);
    }

    sr_free_change_iter(it);
    return SR_ERR_OK;
}

int openconfig_interface_mod_cb(
    __attribute__((unused)) sr_session_ctx_t *session,
    __attribute__((unused)) const char *module_name,
    __attribute__((unused)) sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    SRP_LOG_DBG("Inerafce module subscribe: %s", module_name);

    return SR_ERR_OK;
}

typedef struct
{
    bool is_subif;
    u32 subinterface_index;
    sw_interface_details_query_t sw_interface_details_query;
    sysr_values_ctx_t sysr_values_ctx;
} sys_sw_interface_dump_ctx;

#define NOT_AVAL "NA"

// XPATH: /openconfig-interfaces:interfaces/interface/state

static int sw_interface_dump_cb_inner(
    vapi_payload_sw_interface_details * reply,
    sys_sw_interface_dump_ctx * dctx)
{
    sr_val_t *vals = NULL;
    int rc = 0;
    int vc = 0;

    ARG_CHECK2(SR_ERR_INVAL_ARG, reply, dctx);

    vc = 10;

    /* convenient functions such as this can be found in sysrepo/values.h */
    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    const char* interface_name = (const char*)dctx->sw_interface_details_query.sw_interface_details.interface_name;

    sr_val_build_xpath(&vals[0], "%s/name", dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[0], SR_STRING_T, interface_name);

    sr_val_build_xpath(&vals[1], "%s/type", dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[1], SR_IDENTITYREF_T, "ianaift:ethernetCsmacd");

    sr_val_build_xpath(&vals[2], "%s/mtu", dctx->sysr_values_ctx.xpath_root);
    vals[2].type = SR_UINT16_T;
    vals[2].data.uint16_val = reply->link_mtu;

    sr_val_build_xpath(&vals[3], "%s/loopback-mode",
                       dctx->sysr_values_ctx.xpath_root);
    vals[3].type = SR_BOOL_T;
    vals[3].data.bool_val = (0 == strncmp(interface_name, "loop", 4)) ? 1 : 0;

    sr_val_build_xpath(&vals[4], "%s/description",
                       dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[4], SR_STRING_T, NOT_AVAL);

    sr_val_build_xpath(&vals[5], "%s/enabled",
                       dctx->sysr_values_ctx.xpath_root);
    vals[5].type = SR_BOOL_T;
    vals[5].data.bool_val = reply->admin_up_down;

    sr_val_build_xpath(&vals[6], "%s/ifindex",
                       dctx->sysr_values_ctx.xpath_root);
    vals[6].type = SR_UINT32_T;
    vals[6].data.uint32_val = reply->sw_if_index;

    sr_val_build_xpath(&vals[7], "%s/admin-status",
                       dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[7], SR_ENUM_T,
                        reply->admin_up_down ? "UP" : "DOWN");

    sr_val_build_xpath(&vals[8], "%s/oper-status",
                       dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[8], SR_ENUM_T,
                        reply->link_up_down ? "UP" : "DOWN");

    //TODO: Openconfig required this value
    // sr_val_build_xpath(&vals[9], "%s/last-change", dctx->sysr_values_ctx.xpath_root);
    // sr_val_set_str_data(&vals[9], YANG INPUT TYPE: oc-types:timeticks64);

    sr_val_build_xpath(&vals[9], "%s/logical",
                       dctx->sysr_values_ctx.xpath_root);
    vals[9].type = SR_BOOL_T;
    vals[9].data.bool_val = true;         //for now, we assume all are logical

    dctx->sysr_values_ctx.values = vals;
    dctx->sysr_values_ctx.values_cnt = vc;

    return SR_ERR_OK;
}


// XPATH: /openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/state
static int sw_subinterface_dump_cb_inner(
    vapi_payload_sw_interface_details *reply,
    sys_sw_interface_dump_ctx *dctx)
{
    sr_val_t *vals = NULL;
    int rc = 0;
    int vc = 0, val_idx = 0;

    ARG_CHECK2(SR_ERR_INVAL_ARG, reply, dctx);

    vc = 6;
    /* convenient functions such as this can be found in sysrepo/values.h */
    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    sr_val_build_xpath(&vals[val_idx], "%s/index",
                       dctx->sysr_values_ctx.xpath_root);
    vals[val_idx].type = SR_UINT32_T;
    vals[val_idx++].data.uint32_val = dctx->subinterface_index;

    sr_val_build_xpath(&vals[val_idx], "%s/description",
                       dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[val_idx++], SR_STRING_T, NOT_AVAL);

    sr_val_build_xpath(&vals[val_idx], "%s/enabled",
                       dctx->sysr_values_ctx.xpath_root);
    vals[val_idx].type = SR_BOOL_T;
    vals[val_idx++].data.bool_val = reply->admin_up_down;

    //TODO: Openconfig required this value
    // sr_val_build_xpath(&vals[val_idx], "%s/name", dctx->sysr_values_ctx.xpath_root);
    // sr_val_set_str_data(&vals[val_idx++], SR_STRING_T, YANG INPUT TYPE: string);

    // sr_val_build_xpath(&vals[val_idx], "%s/ifindex", interface_name, subinterface_index);
    // vals[val_idx].type = SR_UINT32_T;
    // vals[val_idx++].data.uint32_val = YANG INPUT TYPE: uint32;

    sr_val_build_xpath(&vals[val_idx], "%s/admin-status",
                       dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[val_idx++], SR_ENUM_T,
                        reply->admin_up_down ? "UP" : "DOWN");

    sr_val_build_xpath(&vals[val_idx], "%s/oper-status",
                       dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[val_idx++], SR_ENUM_T,
                        reply->admin_up_down ? "UP" : "DOWN");

    //TODO: Openconfig required this value
    // sr_val_build_xpath(&vals[val_idx], "/openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/state/last-change", interface_name, subinterface_index);
    // sr_val_set_str_data(&vals[val_idx++], YANG INPUT TYPE: oc-types:timeticks64);

    sr_val_build_xpath(&vals[val_idx], "%s/logical",
                       dctx->sysr_values_ctx.xpath_root);
    vals[val_idx].type = SR_BOOL_T;
    vals[val_idx++].data.bool_val = true;         //for now, we assume all are logical

    dctx->sysr_values_ctx.values = vals;
    dctx->sysr_values_ctx.values_cnt = vc;

    return SR_ERR_OK;
}

static
bool is_subinterface(const char* subif_name, const char* base_name,
                     const u32 subif_index)
{
    assert(subif_name && base_name);

    const char* dot = strchr(subif_name, '.');

    if (NULL == dot && 0 == subif_index)
        return true;                    //subif_index == 0 can pass as a "real" interface

    if (dot > subif_name && 0 == strncmp(subif_name, base_name,
        dot - subif_name))
    {
        char * eptr = NULL;
        u32 si = strtoul(dot + 1, &eptr, 10);

        if ('\0' == *eptr && subif_index == si)
            return true;
    }

    return false;
}

static vapi_error_e
sw_interface_dump_vapi_cb(struct vapi_ctx_s *ctx, void *callback_ctx,
                     vapi_error_e rv, bool is_last,
                     vapi_payload_sw_interface_details * reply)
{
    ARG_CHECK(VAPI_EINVAL, callback_ctx);

    if (is_last)
    {
        assert (NULL == reply);
    }
    else
    {
        assert (NULL != reply);
        sys_sw_interface_dump_ctx *dctx = callback_ctx;

        const char* const dctx_interface_name = (const char *)dctx->sw_interface_details_query.sw_interface_details.interface_name;

        SRP_LOG_DBG("interface_name: '%s', if_name: '%s'", reply->interface_name, dctx_interface_name);

        if (dctx->is_subif)
        {
            if (is_subinterface((const char*)reply->interface_name,
                dctx_interface_name, dctx->subinterface_index))
                sw_subinterface_dump_cb_inner(reply, dctx);
        }
        else
        {
            if (0 == strcmp(dctx_interface_name, (char *)reply->interface_name))
            {
                dctx->sw_interface_details_query.sw_interface_details = *reply;
                dctx->sw_interface_details_query.interface_found = true;

                sw_interface_dump_cb_inner(reply, dctx);
            }
        }
    }

    return VAPI_OK;
}

static vapi_error_e sysr_sw_interface_dump(sys_sw_interface_dump_ctx * dctx)
{
    ARG_CHECK(VAPI_EINVAL, dctx);

    vapi_msg_sw_interface_dump *dump = vapi_alloc_sw_interface_dump(g_vapi_ctx);
    vapi_error_e rv;

    dump->payload.name_filter_valid = true;
    strcpy((char*)dump->payload.name_filter, (const char *)dctx->sw_interface_details_query.sw_interface_details.interface_name);

    VAPI_CALL(vapi_sw_interface_dump(g_vapi_ctx, dump, sw_interface_dump_vapi_cb,
                                     dctx));

    if (VAPI_OK != rv) {
        SRP_LOG_DBG("vapi_sw_interface_dump error=%d", rv);
    }

    return rv;
}

int openconfig_interfaces_interfaces_interface_state_cb(
    const char *xpath, sr_val_t **values,
    size_t *values_cnt, uint64_t request_id,
    __attribute__((unused)) void *private_ctx)
{
    sr_xpath_ctx_t state = {0};
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    tmp = xpath_find_first_key(xpath, "name", &state);
    if (NULL == tmp) {
        SRP_LOG_DBG_MSG("interface_name not found.");
        return SR_ERR_INVAL_ARG;
    }

    strncpy(interface_name, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    sys_sw_interface_dump_ctx dctx = {
        .is_subif = false
    };
    sw_interface_details_query_set_name(&dctx.sw_interface_details_query,
                                        interface_name);

    snprintf(dctx.sysr_values_ctx.xpath_root, XPATH_SIZE,
             "/openconfig-interfaces:interfaces/interface[name='%s']/state",
             interface_name);

    sysr_sw_interface_dump(&dctx);

    if (!dctx.sw_interface_details_query.interface_found) {
        SRP_LOG_DBG_MSG("interface not found");
        return SR_ERR_NOT_FOUND;
    }

    sr_xpath_recover(&state);
    *values = dctx.sysr_values_ctx.values;
    *values_cnt = dctx.sysr_values_ctx.values_cnt;

    return SR_ERR_OK;
}

// XPATH: /openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address/openconfig-if-ip:state
int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_state_vapi_cb(
    vapi_payload_ip_address_details *reply,
    sysr_values_ctx_t *sysr_values_ctx)
{
    sr_val_t *vals = NULL;
    int rc = 0;
    int vc = 0;

    ARG_CHECK2(SR_ERR_INVAL_ARG, reply, sysr_values_ctx);

    vc = 3;
    /* convenient functions such as this can be found in sysrepo/values.h */
    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    char * address_ip = bapi_ntoa(reply->ip);

    sr_val_build_xpath(&vals[0], "%s/openconfig-if-ip:ip",
                       sysr_values_ctx->xpath_root);
    sr_val_set_str_data(&vals[0], SR_STRING_T, address_ip);

    sr_val_build_xpath(&vals[1], "%s/openconfig-if-ip:prefix-length",
                       sysr_values_ctx->xpath_root);
    vals[1].type = SR_UINT8_T;
    vals[1].data.uint8_val = reply->prefix_length;

    sr_val_build_xpath(&vals[2], "%s/openconfig-if-ip:origin",
                       sysr_values_ctx->xpath_root);
    sr_val_set_str_data(&vals[2], SR_ENUM_T, "STATIC");

    sysr_values_ctx->values = vals;
    sysr_values_ctx->values_cnt = vc;

    return SR_ERR_OK;
}

vapi_error_e
ip_address_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                    vapi_error_e rv, bool is_last,
                    vapi_payload_ip_address_details * reply)
{
    ARG_CHECK2(VAPI_EINVAL, ctx, callback_ctx);

    sysr_values_ctx_t *dctx = callback_ctx;

    if (is_last)
    {
        assert (NULL == reply);
    }
    else
    {
        assert (NULL != reply);

        printf ("ip address dump entry:"
                    "\tsw_if_index[%u]"
                    "\tip[%s/%u]\n"
                    , reply->sw_if_index, bapi_ntoa(reply->ip),
                reply->prefix_length);

        openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_state_vapi_cb(reply, dctx);
    }

    return VAPI_OK;
}

//TODO: for some arcane reason, this doesn't work
int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    __attribute__((unused)) uint64_t request_id,
    __attribute__((unused)) void *private_ctx)
{
    sr_xpath_ctx_t state = {0};
    vapi_error_e rv;
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};
    char subinterface_index[XPATH_SIZE] = {0};
    char address_ip[XPATH_SIZE] = {0};

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    tmp = xpath_find_first_key(xpath, "name", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("interface_name not found.");
        return SR_ERR_INVAL_ARG;
    }

    strncpy(interface_name, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    tmp = xpath_find_first_key(xpath, "index", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("subinterface_index not found.");
        return SR_ERR_INVAL_ARG;
    }

    strncpy(subinterface_index, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    tmp = xpath_find_first_key(xpath, "ip", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("address_ip not found.");
        return SR_ERR_INVAL_ARG;
    }

    strncpy(address_ip, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    sysr_values_ctx_t dctx = {0};
    snprintf(dctx.xpath_root, XPATH_SIZE, "/openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address[ip='%s']/openconfig-if-ip:state",
             interface_name, subinterface_index, address_ip);

    sw_interface_details_query_t query = {0};
    sw_interface_details_query_set_name(&query, interface_name);

    if (!get_interface_id(&query))
    {
        return SR_ERR_INVAL_ARG;
    }

    vapi_msg_ip_address_dump *mp = vapi_alloc_ip_address_dump (g_vapi_ctx);
    mp->payload.sw_if_index = query.sw_interface_details.sw_if_index;
    mp->payload.is_ipv6 = 0;

    rv = vapi_ip_address_dump(g_vapi_ctx, mp, ip_address_dump_cb, &dctx);
    if (VAPI_OK != rv)
    {
        SRP_LOG_ERR_MSG("VAPI call failed");
        return SR_ERR_INVAL_ARG;
    }

    sr_xpath_recover(&state);
    *values = dctx.values;
    *values_cnt = dctx.values_cnt;

    return SR_ERR_OK;
}

int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    __attribute__((unused)) uint64_t request_id,
    __attribute__((unused)) void *private_ctx)
{
    sr_xpath_ctx_t state = {0};
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};
    char subinterface_index[XPATH_SIZE] = {0};

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    tmp = xpath_find_first_key(xpath, "name", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("interface_name not found.");
        return SR_ERR_INVAL_ARG;
    }

    strncpy(interface_name, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    tmp = xpath_find_first_key(xpath, "index", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("subinterface_index not found.");
        return SR_ERR_INVAL_ARG;
    }

    strncpy(subinterface_index, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    sys_sw_interface_dump_ctx dctx =
    {
        .is_subif = true,
        .subinterface_index = atoi(subinterface_index)
    };
    sw_interface_details_query_set_name(&dctx.sw_interface_details_query,
                                        interface_name);

    snprintf(dctx.sysr_values_ctx.xpath_root, XPATH_SIZE, "/openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/state",
             interface_name, subinterface_index);

    sysr_sw_interface_dump(&dctx);

    if (!dctx.sw_interface_details_query.interface_found) {
        SRP_LOG_DBG_MSG("interface not found");
        return SR_ERR_NOT_FOUND;
    }

    sr_xpath_recover(&state);
    *values = dctx.sysr_values_ctx.values;
    *values_cnt = dctx.sysr_values_ctx.values_cnt;

    return SR_ERR_OK;
}

VAPI_RETVAL_CB(sw_interface_add_del_address);

static int oi_int_ipv4_conf(const char *interface_name,
                            const char *address_ip, u8 prefix_length,
                            bool is_add)
{
    ARG_CHECK2(-1, interface_name, address_ip);

    sw_interface_details_query_t query = {0};
    sw_interface_details_query_set_name(&query, interface_name);
    if (!get_interface_id(&query)) {
        return 0;
    }

    if (VAPI_OK != bin_api_sw_interface_add_del_address(query.sw_interface_details.sw_if_index,
        is_add, address_ip, prefix_length))
    {
        SRP_LOG_ERR_MSG("Call vapi_sw_interface_add_del_address.");
        return -1;
    }

    return 0;
}

// XPATH: /openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/oc-ip:ipv4/oc-ip:addresses/oc-ip:address[ip='%s']/oc-ip:config/
int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};
    char subinterface_index[XPATH_SIZE] = {0};
    char address_ip[XPATH_SIZE] = {0};
    char old_address_ip[XPATH_SIZE] = {0};
    u8 prefix_len = 0;
    u8 old_prefix_len = 0;
    int rc = 0;
    bool ip_set = false, prefix_len_set = false;
    bool old_ip_set = false, old_prefix_len_set = false;

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    // if we receive event SR_EV_APPLY - config has changed

    log_recv_event(event, "subtree_change_cb received");
    SRP_LOG_DBG("[SRP_LOG_DBG] xpath: %s", xpath);

    if (event != SR_EV_APPLY) {
        return SR_ERR_OK;
    }

    if (sr_get_changes_iter(ds, (char *)xpath, &it) != SR_ERR_OK) {
        // in example he calls even on fale
        sr_free_change_iter(it);
        return SR_ERR_OK;
    }

    while (sr_get_change_next(ds, it, &oper,
        &old_val, &new_val) == SR_ERR_OK) {

        log_recv_oper(oper, "subtree_change_cb received");

        SRP_LOG_DBG("xpath: %s", new_val->xpath);

        sr_xpath_ctx_t state = {0};

        tmp = xpath_find_first_key(new_val->xpath, "name", &state);
        if (NULL == tmp) {
            SRP_LOG_DBG_MSG("interface_name NOT found.");
            continue;
        }

        strncpy(interface_name, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        tmp = xpath_find_first_key(new_val->xpath, "index", &state);
        if (NULL == tmp) {
            SRP_LOG_DBG_MSG("subinterface_index NOT found.");
            continue;
        }

        strncpy(subinterface_index, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        switch (oper) {
            case SR_OP_CREATED:
                if (sr_xpath_node_name_eq(new_val->xpath, "ip")) {
                    ip_set = true;
                    strncpy(address_ip, new_val->data.string_val, XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(new_val->xpath,
                    "prefix-length")) {
                    prefix_len_set = true;
                    prefix_len = new_val->data.uint8_val;
                }

                if (ip_set && prefix_len_set) {
                    rc = oi_int_ipv4_conf(interface_name, address_ip,
                                          prefix_len, true);
                }

                break;

            case SR_OP_MODIFIED:
                if (sr_xpath_node_name_eq(old_val->xpath, "ip")) {
                    old_ip_set = true;
                    strncpy(old_address_ip, old_val->data.string_val,
                            XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(old_val->xpath,
                    " prefix-length")) {
                    old_prefix_len_set = true;
                    old_prefix_len = old_val->data.uint8_val;
                }

                if (sr_xpath_node_name_eq(new_val->xpath, "ip")) {
                    ip_set = true;
                    strncpy(address_ip, new_val->data.string_val, XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(new_val->xpath,
                    "prefix-length")) {
                    prefix_len_set = true;
                    prefix_len = new_val->data.uint8_val;
                }

                if (old_ip_set && old_prefix_len_set) {
                    rc = oi_int_ipv4_conf(interface_name, old_address_ip,
                                          old_prefix_len, false);
                    if (0 != rc) {
                        break;
                    }
                }

                if (ip_set && prefix_len_set) {
                    rc = oi_int_ipv4_conf(interface_name, address_ip,
                                          prefix_len, true);
                }
                break;

            case SR_OP_MOVED:
                break;

            case SR_OP_DELETED:
                if (sr_xpath_node_name_eq(old_val->xpath, "ip")) {
                    old_ip_set = true;
                    strncpy(old_address_ip, old_val->data.string_val,
                            XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(old_val->xpath,
                    "prefix-length")) {
                    old_prefix_len_set = true;
                    old_prefix_len = old_val->data.uint8_val;
                }

                if (old_ip_set && old_prefix_len_set) {
                    rc = oi_int_ipv4_conf(interface_name, old_address_ip,
                                         old_prefix_len, false);
                }
                break;
        }

        if (0 != rc) {
            sr_xpath_recover(&state);

            sr_free_val(old_val);
            sr_free_val(new_val);

            sr_free_change_iter(it);

            return SR_ERR_OPERATION_FAILED;
        }

        sr_xpath_recover(&state);

        sr_free_val(old_val);
        sr_free_val(new_val);
    }

    sr_free_change_iter(it);
    return SR_ERR_OK;
}

