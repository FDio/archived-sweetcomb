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
#include <string.h>

#include "openconfig_interfaces.h"
#include "sys_util.h"

#include "sc_vpp_comm.h"
#include "sc_vpp_interface.h"
#include "sc_vpp_ip.h"

#define XPATH_SIZE 2000

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

    SRP_LOG_INF_MSG("In openconfig_interfaces_interfaces_interface_config_cb");

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
                    rc = interface_enable(interface_name,
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
                    rc = interface_enable(interface_name,
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
                    rc = interface_enable(interface_name, false);
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

// openconfig-interfaces
int openconfig_interface_mod_cb(
    __attribute__((unused)) sr_session_ctx_t *session,
    __attribute__((unused)) const char *module_name,
    __attribute__((unused)) sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    SRP_LOG_INF_MSG("In openconfig_interface_mod_cb");

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

static bool
is_subinterface(const char* subif_name, const char* base_name,
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
    vapi_msg_sw_interface_dump *dump;
    vapi_error_e rv;

    ARG_CHECK(VAPI_EINVAL, dctx);

    dump = vapi_alloc_sw_interface_dump(g_vapi_ctx_instance);

    dump->payload.name_filter_valid = true;
    strcpy((char*)dump->payload.name_filter, (const char *)dctx->sw_interface_details_query.sw_interface_details.interface_name);

    VAPI_CALL(vapi_sw_interface_dump(g_vapi_ctx_instance, dump, sw_interface_dump_vapi_cb,
                                     dctx));

    if (VAPI_OK != rv) {
        SRP_LOG_DBG("vapi_sw_interface_dump error=%d", rv);
    }

    return rv;
}

int openconfig_interfaces_interfaces_interface_state_cb(
    const char *xpath, sr_val_t **values,
    size_t *values_cnt,
    __attribute__((unused)) uint64_t request_id,
    __attribute__((unused)) const char *original_xpath,
    __attribute__((unused)) void *private_ctx)
{
    sr_xpath_ctx_t state = {0};
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};

    SRP_LOG_INF_MSG("In openconfig_interfaces_interfaces_interface_state_cb");

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    tmp = xpath_find_first_key(xpath, "name", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("Interface name not found in sysrepo database");
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
        SRP_LOG_ERR_MSG("interface not found");
        return SR_ERR_NOT_FOUND;
    }

    sr_xpath_recover(&state);
    *values = dctx.sysr_values_ctx.values;
    *values_cnt = dctx.sysr_values_ctx.values_cnt;

    return SR_ERR_OK;
}

int oc_dump_ip_helper(char *address_ip, u8 prefix_len,
                      sysr_values_ctx_t *sysr_values_ctx)
{
    sr_val_t *vals = NULL;
    int rc = 0;
    int vc = 0;

    ARG_CHECK2(SR_ERR_INVAL_ARG, address_ip, sysr_values_ctx);

    vc = 3;
    /* convenient functions such as this can be found in sysrepo/values.h */
    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    sr_val_build_xpath(&vals[0], "%s/openconfig-if-ip:ip",
                       sysr_values_ctx->xpath_root);
    sr_val_set_str_data(&vals[0], SR_STRING_T, address_ip);

    sr_val_build_xpath(&vals[1], "%s/openconfig-if-ip:prefix-length",
                       sysr_values_ctx->xpath_root);
    vals[1].type = SR_UINT8_T;
    vals[1].data.uint8_val = prefix_len;

    sr_val_build_xpath(&vals[2], "%s/openconfig-if-ip:origin",
                       sysr_values_ctx->xpath_root);
    sr_val_set_str_data(&vals[2], SR_ENUM_T, "STATIC");

    sysr_values_ctx->values = vals;
    sysr_values_ctx->values_cnt = vc;

    return SR_ERR_OK;
}

//TODO: for some arcane reason, this doesn't work
int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    __attribute__((unused)) uint64_t request_id,
    __attribute__((unused)) const char *original_xpath,
    __attribute__((unused)) void *private_ctx)
{
    sr_xpath_ctx_t state = {0};
    int rc;
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};
    char subinterface_index[XPATH_SIZE] = {0};
    char address_ip[XPATH_SIZE] = {0};
    u8 prefix_len;

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    SRP_LOG_INF_MSG("In oc-interfaces oc-ip");

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

    rc = ipv46_address_dump(interface_name, address_ip, &prefix_len, false);
    if (!rc) {
        SRP_LOG_ERR_MSG("ipv46_address_dump failed");
        return SR_ERR_INVAL_ARG;
    }

    rc = oc_dump_ip_helper(address_ip, prefix_len, &dctx);
    if (!rc) {
        SRP_LOG_ERR_MSG("oc_dump_ip_helper failed");
        return rc;
    }

    sr_xpath_recover(&state);
    *values = dctx.values;
    *values_cnt = dctx.values_cnt;

    return SR_ERR_OK;
}

// openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/state
int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    __attribute__((unused)) uint64_t request_id,
    __attribute__((unused)) const char *original_xpath,
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

// openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/oc-ip:ipv4/oc-ip:addresses/oc-ip:address[ip='%s']/oc-ip:config/
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

    while (sr_get_change_next(ds, it, &oper, &old_val, &new_val) == SR_ERR_OK) {

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
                    //add ipv4
                    rc = ipv46_config_add_remove(interface_name, address_ip,
                                                 prefix_len, false , true);
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
                    //remove ipv4
                    rc = ipv46_config_add_remove(interface_name, old_address_ip,
                                                old_prefix_len, false, false);
                    if (0 != rc) {
                        break;
                    }
                }

                if (ip_set && prefix_len_set) {
                    //add ipv4
                    rc = ipv46_config_add_remove(interface_name, address_ip,
                                                 prefix_len, false, true);
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
                    //remove ipv4
                    rc = ipv46_config_add_remove(interface_name, old_address_ip,
                                                 old_prefix_len, false, false);
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

const xpath_t oc_interfaces_xpaths[OC_INTERFACES_SIZE] = {
    {
        .xpath = "openconfig-interfaces",
        .method = MODULE,
        .datastore = SR_DS_RUNNING,
        .cb.mcb  = openconfig_interface_mod_cb,
        .private_ctx = NULL,
        .priority = 0,
        //.opts = SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY
        .opts = SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/config",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = openconfig_interfaces_interfaces_interface_config_cb,
        .private_ctx = NULL,
        .priority = 98,
        //.opts = SR_SUBSCR_DEFAULT
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = openconfig_interfaces_interfaces_interface_state_cb,
        .private_ctx = NULL,
        .priority = 98,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_state_cb,
        .private_ctx = NULL,
        .priority = 99,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address/openconfig-if-ip:config",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_config_cb,
        .private_ctx = NULL,
        .priority = 100,
        //.opts = SR_SUBSCR_DEFAULT
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address/openconfig-if-ip:state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_state_cb,
        .private_ctx = NULL,
        .priority = 100,
        .opts = SR_SUBSCR_CTX_REUSE
    }
};
