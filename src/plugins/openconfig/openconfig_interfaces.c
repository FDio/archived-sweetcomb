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

#include <scvpp/comm.h>
#include <scvpp/interface.h>
#include <scvpp/ip.h>

#include "../sc_model.h"
#include "../sys_util.h"

// XPATH: /openconfig-interfaces:interfaces/interface[name='%s']/config/
static int openconfig_interfaces_interfaces_interface_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    void *private_ctx)
{
    UNUSED(private_ctx);
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_xpath_ctx_t state = {0};
    sr_val_t *old = NULL;
    sr_val_t *new = NULL;
    char *tmp = NULL;
    char interface_name[XPATH_SIZE] = {0};
    int rc = 0;

    SRP_LOG_INF("In %s", __FUNCTION__);
    SRP_LOG_INF("XPATH %s", xpath);

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    if (event == SR_EV_APPLY)
        return SR_ERR_OK;

    if (sr_get_changes_iter(ds, (char *)xpath, &it) != SR_ERR_OK) {
        sr_free_change_iter(it);
        return SR_ERR_OK;
    }

    foreach_change (ds, it, oper, old, new) {

        SRP_LOG_DBG("xpath: %s", new->xpath);

        tmp = sr_xpath_key_value(new->xpath, "interface", "name", &state);
        if (!tmp) {
            sr_set_error(ds, "XPATH interface name NOT found", new->xpath);
            return SR_ERR_INVAL_ARG;
        }
        strncpy(interface_name, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        switch (oper) {
            case SR_OP_CREATED:
            case SR_OP_MODIFIED:
                if (sr_xpath_node_name_eq(new->xpath, "name")) {
                    //TODO: LEAF: name, type: string
                } else if(sr_xpath_node_name_eq(new->xpath, "type")) {
                    //TODO: LEAF: type, type: identityref
                } else if(sr_xpath_node_name_eq(new->xpath, "mtu")) {
                    //TODO: LEAF: mtu, type: uint16
                } else if(sr_xpath_node_name_eq(new->xpath,
                    "loopback-mode")) {
                    //TODO: LEAF: loopback-mode, type: boolean
                } else if(sr_xpath_node_name_eq(new->xpath,
                                                    "description")) {
                    //TODO: LEAF: description, type: string
                } else if(sr_xpath_node_name_eq(new->xpath, "enabled")) {
                    rc = interface_enable(interface_name,
                                             new->data.bool_val);
                } else if(sr_xpath_node_name_eq(new->xpath,
                                                 "oc-vlan:tpid")) {
                    //TODO: LEAF: oc-vlan:tpid, type: identityref
                }
                break;

            case SR_OP_MOVED:
                break;

            case SR_OP_DELETED:
                if (sr_xpath_node_name_eq(old->xpath, "name")) {
                    //TODO: LEAF: name, type: string
                } else if(sr_xpath_node_name_eq(old->xpath, "type")) {
                    //TODO: LEAF: type, type: identityref
                } else if(sr_xpath_node_name_eq(old->xpath, "mtu")) {
                    //TODO: LEAF: mtu, type: uint16
                } else if(sr_xpath_node_name_eq(old->xpath,
                    "loopback-mode")) {
                    //TODO: LEAF: loopback-mode, type: boolean
                } else if(sr_xpath_node_name_eq(old->xpath,
                                                    "description")) {
                    //TODO: LEAF: description, type: string
                } else if(sr_xpath_node_name_eq(old->xpath, "enabled")) {
                    rc = interface_enable(interface_name, false);
                } else if(sr_xpath_node_name_eq(old->xpath,
                                                 "oc-vlan:tpid")) {
                    //TODO: LEAF: oc-vlan:tpid, type: identityref
                }
                break;
        }

        if (0 != rc) {
            sr_xpath_recover(&state);

            sr_free_val(old);
            sr_free_val(new);

            sr_free_change_iter(it);

            return SR_ERR_OPERATION_FAILED;
        }

        sr_xpath_recover(&state);

        sr_free_val(old);
        sr_free_val(new);
    }

    sr_free_change_iter(it);
    return SR_ERR_OK;
}

//XPATH : /openconfig-interfaces:interfaces/interface/state
static int
openconfig_interfaces_interfaces_interface_state_cb(
    const char *xpath, sr_val_t **values,
    size_t *values_cnt, uint64_t request_id, const char *original_xpath,
    void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    sw_interface_dump_t reply = {0};
    sr_val_t *vals = NULL;
    sr_xpath_ctx_t state = {0};
    char interface_name[VPP_INTFC_NAME_LEN] = {0};
    char xpath_root[XPATH_SIZE];
    int vc = 10;
    char *tmp;
    int rc;

    SRP_LOG_INF("In %s", __FUNCTION__);

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    tmp = sr_xpath_key_value((char*) xpath, "interface", "name", &state);
    if (!tmp) {
        SRP_LOG_ERR_MSG("XPATH interface name not found");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(interface_name, tmp, VPP_INTFC_NAME_LEN);
    sr_xpath_recover(&state);

    snprintf(xpath_root, XPATH_SIZE,
             "/openconfig-interfaces:interfaces/interface[name='%s']/state",
             interface_name);

    //dump interface interface_name
    rc = interface_dump_iface(&reply, interface_name);
    if (rc == -SCVPP_NOT_FOUND) {
        SRP_LOG_ERR_MSG("interface not found");
        return SR_ERR_NOT_FOUND;
    }

    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc)
        return rc;

    sr_val_build_xpath(&vals[0], "%s/name", xpath_root);
    sr_val_set_str_data(&vals[0], SR_STRING_T, (char *)reply.interface_name);

    sr_val_build_xpath(&vals[1], "%s/type", xpath_root);
    sr_val_set_str_data(&vals[1], SR_IDENTITYREF_T, "ianaift:ethernetCsmacd");

    sr_val_build_xpath(&vals[2], "%s/mtu", xpath_root);
    vals[2].type = SR_UINT16_T;
    vals[2].data.uint16_val = reply.link_mtu;

    sr_val_build_xpath(&vals[3], "%s/loopback-mode", xpath_root);
    vals[3].type = SR_BOOL_T;
    if (strncmp((char*)reply.interface_name, "loop", 4) == 0)
        vals[3].data.bool_val = 1;
    else
        vals[3].data.bool_val = 0;

    sr_val_build_xpath(&vals[4], "%s/description", xpath_root);
    sr_val_set_str_data(&vals[4], SR_STRING_T, NOT_AVAL);

    sr_val_build_xpath(&vals[5], "%s/enabled", xpath_root);
    vals[5].type = SR_BOOL_T;
    vals[5].data.bool_val = reply.admin_up_down;

    sr_val_build_xpath(&vals[6], "%s/ifindex", xpath_root);
    vals[6].type = SR_UINT32_T;
    vals[6].data.uint32_val = reply.sw_if_index;

    sr_val_build_xpath(&vals[7], "%s/admin-status", xpath_root);
    sr_val_set_str_data(&vals[7], SR_ENUM_T,
                        reply.admin_up_down ? "UP" : "DOWN");

    sr_val_build_xpath(&vals[8], "%s/oper-status", xpath_root);
    sr_val_set_str_data(&vals[8], SR_ENUM_T,
                        reply.link_up_down ? "UP" : "DOWN");

    //TODO: Openconfig required this value
    // sr_val_build_xpath(&vals[9], "%s/last-change", xpath_root);
    // sr_val_set_str_data(&vals[9], YANG INPUT TYPE: oc-types:timeticks64);

    sr_val_build_xpath(&vals[9], "%s/logical", xpath_root);
    vals[9].type = SR_BOOL_T;
    vals[9].data.bool_val = true; //for now, we assume all are logical

    *values = vals;
    *values_cnt = vc;

    return SR_ERR_OK;
}

//TODO: for some arcane reason, this doesn't work
static int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    char xpath_root[XPATH_SIZE] = {0};
    sr_xpath_ctx_t state = {0};
    sr_val_t *vals = NULL;
    char interface_name[VPP_INTFC_NAME_LEN] = {0};
    char subinterface_index[XPATH_SIZE] = {0};
    char address_ip[XPATH_SIZE] = {0};
    u8 prefix_len;
    char *tmp = NULL;
    int rc, cnt = 0;

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    SRP_LOG_INF("In %s", __FUNCTION__);

    /* Get XPATH parameters name, index and ip */

    tmp = sr_xpath_key_value((char*)xpath, "interface", "name", &state);
    if (!tmp) {
        SRP_LOG_ERR_MSG("XPATH interface name not found");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(interface_name, tmp, VPP_INTFC_NAME_LEN);
    sr_xpath_recover(&state);

    tmp = sr_xpath_key_value((char*)xpath, "subinterface", "index", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("XPATH subinterface index not found.");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(subinterface_index, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    tmp = sr_xpath_key_value((char*)xpath, "address", "ip", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("XPATH address ip not found.");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(address_ip, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    snprintf(xpath_root, XPATH_SIZE, "/openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address[ip='%s']/openconfig-if-ip:state",
             interface_name, subinterface_index, address_ip);

    rc = ipv46_address_dump(interface_name, address_ip, &prefix_len, false);
    if (!rc) {
        SRP_LOG_ERR_MSG("ipv46_address_dump failed");
        return SR_ERR_INVAL_ARG;
    }

    /* Build answer to state XPATH */

    rc = sr_new_values(3, &vals);
    if (SR_ERR_OK != rc)
        return rc;

    sr_val_build_xpath(&vals[cnt], "%s/openconfig-if-ip:ip", xpath_root);
    sr_val_set_str_data(&vals[cnt], SR_STRING_T, address_ip);
    cnt++;

    sr_val_build_xpath(&vals[cnt], "%s/openconfig-if-ip:prefix-length", xpath_root);
    vals[cnt].type = SR_UINT8_T;
    vals[cnt].data.uint8_val = prefix_len;
    cnt++;

    sr_val_build_xpath(&vals[cnt], "%s/openconfig-if-ip:origin", xpath_root);
    sr_val_set_str_data(&vals[cnt], SR_ENUM_T, "STATIC");
    cnt++;

    sr_xpath_recover(&state);
    *values = vals;
    *values_cnt = cnt;

    return SR_ERR_OK;
}

/* TODO move to common functions between all plugins */
static bool
is_subinterface(const char* subif_name, const char* base_name,
                const u32 subif_index)
{
    assert(subif_name && base_name);

    const char* dot = strchr(subif_name, '.');

    if (NULL == dot && 0 == subif_index)
        return true; //subif_index == 0 can pass as a "real" interface

    if (dot > subif_name && 0 == strncmp(subif_name, base_name,
        dot - subif_name)) {
        char * eptr = NULL;
        u32 si = strtoul(dot + 1, &eptr, 10);

        if ('\0' == *eptr && subif_index == si)
            return true;
    }

    return false;
}

// openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/state
static int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    sw_interface_dump_t reply = {0};
    char xpath_root[XPATH_SIZE];
    sr_xpath_ctx_t state = {0};
    char *tmp = NULL;
    char interface_name[VPP_INTFC_NAME_LEN] = {0};
    char subinterface_index[XPATH_SIZE] = {0};
    u32 sub_id;
    sr_val_t *vals = NULL;
    int vc = 6;
    int val_idx = 0;
    int rc;

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    SRP_LOG_INF("In %s", __FUNCTION__);

    tmp = sr_xpath_key_value((char*)xpath, "interface", "name", &state);
    if (!tmp) {
        SRP_LOG_ERR_MSG("XPATH interface name not found");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(interface_name, tmp, VPP_INTFC_NAME_LEN);
    sr_xpath_recover(&state);

    tmp = sr_xpath_key_value((char*)xpath, "subinterface", "index", &state);
    if (!tmp) {
        SRP_LOG_ERR_MSG("subinterface index not found");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(subinterface_index, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    snprintf(xpath_root, XPATH_SIZE,
             "/openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/state",
             interface_name, subinterface_index);

    rc = interface_dump_iface(&reply, interface_name);
    if (rc == -SCVPP_NOT_FOUND) {
        SRP_LOG_ERR_MSG("interface not found");
        return SR_ERR_NOT_FOUND;
    }

    /* Check that requested subinterface index matches reply subid */
    sub_id = atoi(subinterface_index);
    if (sub_id != reply.sub_id) {
        SRP_LOG_ERR_MSG("subinterface index not found.");
        return SR_ERR_NOT_FOUND;
    }

    /* Check that interface is a sub interface */
    if (!is_subinterface((char*)reply.interface_name, interface_name, sub_id)) {
        SRP_LOG_ERR("%s not a subinterface", interface_name);
        return SR_ERR_OPERATION_FAILED;
    }

    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc)
        return rc;

    sr_val_build_xpath(&vals[val_idx], "%s/index", xpath_root);
    vals[val_idx].type = SR_UINT32_T;
    vals[val_idx++].data.uint32_val = sub_id;

    sr_val_build_xpath(&vals[val_idx], "%s/description", xpath_root);
    sr_val_set_str_data(&vals[val_idx++], SR_STRING_T, NOT_AVAL);

    sr_val_build_xpath(&vals[val_idx], "%s/enabled", xpath_root);
    vals[val_idx].type = SR_BOOL_T;
    vals[val_idx++].data.bool_val = reply.admin_up_down;

    //TODO: Openconfig required this value
    // sr_val_build_xpath(&vals[val_idx], "%s/name", xpath_root);
    // sr_val_set_str_data(&vals[val_idx++], SR_STRING_T, YANG INPUT TYPE: string);

    // sr_val_build_xpath(&vals[val_idx], "%s/ifindex", interface_name, sub_id);
    // vals[val_idx].type = SR_UINT32_T;
    // vals[val_idx++].data.uint32_val = YANG INPUT TYPE: uint32;

    sr_val_build_xpath(&vals[val_idx], "%s/admin-status", xpath_root);
    sr_val_set_str_data(&vals[val_idx++], SR_ENUM_T,
                        reply.admin_up_down ? "UP" : "DOWN");

    sr_val_build_xpath(&vals[val_idx], "%s/oper-status", xpath_root);
    sr_val_set_str_data(&vals[val_idx++], SR_ENUM_T, reply.admin_up_down ? "UP" : "DOWN");

    //TODO: Openconfig required this value
    // sr_val_build_xpath(&vals[val_idx], "/openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/state/last-change", interface_name, sub_id);
    // sr_val_set_str_data(&vals[val_idx++], YANG INPUT TYPE: oc-types:timeticks64);

    sr_val_build_xpath(&vals[val_idx], "%s/logical", xpath_root);
    vals[val_idx].type = SR_BOOL_T;
    vals[val_idx++].data.bool_val = true; //for now, we assume all are logical

    sr_xpath_recover(&state);

    *values = vals;
    *values_cnt = vc;

    return SR_ERR_OK;
}

// openconfig-interfaces:interfaces/interface[name='%s']/subinterfaces/subinterface[index='%s']/oc-ip:ipv4/oc-ip:addresses/oc-ip:address[ip='%s']/oc-ip:config/
static int openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    void *private_ctx)
{
    UNUSED(private_ctx);
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_xpath_ctx_t state = {0};
    sr_val_t *old = NULL;
    sr_val_t *new = NULL;
    char *tmp = NULL;
    char interface_name[VPP_INTFC_NAME_LEN] = {0};
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

    if (event == SR_EV_APPLY)
        return SR_ERR_OK;

    if (sr_get_changes_iter(ds, (char *)xpath, &it) != SR_ERR_OK) {
        sr_free_change_iter(it);
        return SR_ERR_OK;
    }

    foreach_change (ds, it, oper, old, new) {

        SRP_LOG_DBG("xpath: %s", new->xpath);

        tmp = sr_xpath_key_value(new->xpath, "interface", "name", &state);
        if (!tmp) {
            sr_set_error(ds, "XPATH interface name NOT found", new->xpath);
            return SR_ERR_INVAL_ARG;
        }
        strncpy(interface_name, tmp, VPP_INTFC_NAME_LEN);
        sr_xpath_recover(&state);

        tmp = sr_xpath_key_value(new->xpath, "subinterface", "index", &state);
        if (!tmp) {
            sr_set_error(ds, "XPATH subinterface index NOT found", new->xpath);
            return SR_ERR_INVAL_ARG;
        }
        strncpy(subinterface_index, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        switch (oper) {
            case SR_OP_CREATED:
                if (sr_xpath_node_name_eq(new->xpath, "ip")) {
                    ip_set = true;
                    strncpy(address_ip, new->data.string_val, XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(new->xpath,
                    "prefix-length")) {
                    prefix_len_set = true;
                    prefix_len = new->data.uint8_val;
                }

                if (ip_set && prefix_len_set) //add ipv4
                    rc = ipv46_config_add_remove(interface_name, address_ip,
                                                 prefix_len, false , true);

                break;

            case SR_OP_MODIFIED:
                if (sr_xpath_node_name_eq(old->xpath, "ip")) {
                    old_ip_set = true;
                    strncpy(old_address_ip, old->data.string_val,
                            XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(old->xpath,
                    " prefix-length")) {
                    old_prefix_len_set = true;
                    old_prefix_len = old->data.uint8_val;
                }

                if (sr_xpath_node_name_eq(new->xpath, "ip")) {
                    ip_set = true;
                    strncpy(address_ip, new->data.string_val, XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(new->xpath,
                    "prefix-length")) {
                    prefix_len_set = true;
                    prefix_len = new->data.uint8_val;
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
                if (sr_xpath_node_name_eq(old->xpath, "ip")) {
                    old_ip_set = true;
                    strncpy(old_address_ip, old->data.string_val,
                            XPATH_SIZE);
                } else if (sr_xpath_node_name_eq(old->xpath,
                    "prefix-length")) {
                    old_prefix_len_set = true;
                    old_prefix_len = old->data.uint8_val;
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

            sr_free_val(old);
            sr_free_val(new);

            sr_free_change_iter(it);

            return SR_ERR_OPERATION_FAILED;
        }

        sr_xpath_recover(&state);

        sr_free_val(old);
        sr_free_val(new);
    }

    sr_free_change_iter(it);
    return SR_ERR_OK;
}

const xpath_t oc_interfaces_xpaths[OC_INTERFACES_SIZE] = {
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
