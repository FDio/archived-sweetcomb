/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
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
#include <sys/socket.h>

#include "../sc_model.h"
#include "../sys_util.h"

#include <scvpp/interface.h>
#include <scvpp/ip.h>

/**
 * @brief Callback to be called by any config change of
 * "/ietf-interfaces:interfaces/interface/enabled" leaf.
 */
static int
ietf_interface_enable_disable_cb(sr_session_ctx_t *session, const char *xpath,
                                 sr_notif_event_t event, void *private_ctx)
{
    UNUSED(private_ctx);
    char *if_name = NULL;
    sr_change_iter_t *iter = NULL;
    sr_change_oper_t op = SR_OP_CREATED;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    sr_xpath_ctx_t xpath_ctx = { 0, };
    int rc = SR_ERR_OK, op_rc = SR_ERR_OK;

    SRP_LOG_INF("In %s", __FUNCTION__);

    /* no-op for apply, we only care about SR_EV_ENABLED, SR_EV_VERIFY, SR_EV_ABORT */
    if (SR_EV_APPLY == event)
        return SR_ERR_OK;

    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

    /* get changes iterator */
    rc = sr_get_changes_iter(session, xpath, &iter);
    if (SR_ERR_OK != rc) {
        sr_free_change_iter(iter);
        SRP_LOG_ERR("Unable to retrieve change iterator: %s", sr_strerror(rc));
        return rc;
    }

    foreach_change (session, iter, op, old_val, new_val) {

        SRP_LOG_DBG("A change detected in '%s', op=%d", new_val ? new_val->xpath : old_val->xpath, op);
        if_name = sr_xpath_key_value(new_val ? new_val->xpath : old_val->xpath, "interface", "name", &xpath_ctx);
        switch (op) {
            case SR_OP_CREATED:
            case SR_OP_MODIFIED:
                op_rc = interface_enable(if_name, new_val->data.bool_val);
                break;
            case SR_OP_DELETED:
                op_rc = interface_enable(if_name, false /* !enable */);
                break;
            default:
                break;
        }
        sr_xpath_recover(&xpath_ctx);
        if (SR_ERR_INVAL_ARG == op_rc) {
            sr_set_error(session, "Invalid interface name.", new_val ? new_val->xpath : old_val->xpath);
        }
        sr_free_val(old_val);
        sr_free_val(new_val);
    }
    sr_free_change_iter(iter);

    return op_rc;
}

/**
 * @brief Modify existing IPv4/IPv6 config on an interface.
 */
static int
interface_ipv46_config_modify(const char *if_name, sr_val_t *old_val,
                              sr_val_t *new_val, bool is_ipv6)
{
    sr_xpath_ctx_t xpath_ctx = { 0, };
    char *addr = NULL;
    uint8_t prefix = 0;
    int rc = SR_ERR_OK;

    SRP_LOG_DBG("Updating IP config on interface '%s'.", if_name);

    /* get old config to be deleted */
    if (SR_UINT8_T == old_val->type) {
        prefix = old_val->data.uint8_val;
    } else if (SR_STRING_T == old_val->type) {
        prefix = netmask_to_prefix(old_val->data.string_val);
    } else {
        return SR_ERR_INVAL_ARG;
    }
    addr = sr_xpath_key_value((char*)old_val->xpath, "address", "ip", &xpath_ctx);
    sr_xpath_recover(&xpath_ctx);

    /* delete old IP config */
    rc = ipv46_config_add_remove(if_name, addr, prefix, is_ipv6, false /* remove */);
    if (SR_ERR_OK != rc) {
        SRP_LOG_ERR("Unable to remove old IP address config, rc=%d", rc);
        return rc;
    }

    /* update the config with the new value */
    if (sr_xpath_node_name_eq(new_val->xpath, "prefix-length")) {
        prefix = new_val->data.uint8_val;
    } else if (sr_xpath_node_name_eq(new_val->xpath, "netmask")) {
        prefix = netmask_to_prefix(new_val->data.string_val);
    }

    /* set new IP config */
    rc = ipv46_config_add_remove(if_name, addr, prefix, is_ipv6, true /* add */);
    if (SR_ERR_OK != rc) {
        SRP_LOG_ERR("Unable to remove old IP address config, rc=%d", rc);
        return rc;
    }

    return rc;
}

/**
 * @brief Callback to be called by any config change in subtrees
 * "/ietf-interfaces:interfaces/interface/ietf-ip:ipv4/address"
 * or "/ietf-interfaces:interfaces/interface/ietf-ip:ipv6/address".
 */
static int
ietf_interface_ipv46_address_change_cb(sr_session_ctx_t *session,
                                       const char *xpath,
                                       sr_notif_event_t event,
                                       void *private_ctx)
{
    UNUSED(private_ctx);
    sr_change_iter_t *iter = NULL;
    sr_change_oper_t op = SR_OP_CREATED;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    sr_xpath_ctx_t xpath_ctx = { 0, };
    bool is_ipv6 = false, has_addr = false, has_prefix = false;
    char addr[VPP_IP6_ADDRESS_STRING_LEN] = {0};
    uint8_t prefix = 0;
    char *node_name = NULL, *if_name = NULL;
    int rc = SR_ERR_OK, op_rc = SR_ERR_OK;

    SRP_LOG_INF("In %s", __FUNCTION__);

    /* no-op for apply, we only care about SR_EV_ENABLED, SR_EV_VERIFY, SR_EV_ABORT */
    if (SR_EV_APPLY == event) {
        return SR_ERR_OK;
    }
    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

    /* check whether we are handling ipv4 or ipv6 config */
    node_name = sr_xpath_node_idx((char*)xpath, 2, &xpath_ctx);
    if (NULL != node_name && 0 == strcmp(node_name, "ipv6")) {
        is_ipv6 = true;
    }
    sr_xpath_recover(&xpath_ctx);

    /* get changes iterator */
    rc = sr_get_changes_iter(session, xpath, &iter);
    if (SR_ERR_OK != rc) {
        sr_free_change_iter(iter);
        SRP_LOG_ERR("Unable to retrieve change iterator: %s", sr_strerror(rc));
        return rc;
    }

    foreach_change(session, iter, op, old_val, new_val) {

        SRP_LOG_DBG("A change detected in '%s', op=%d", new_val ? new_val->xpath : old_val->xpath, op);
        if_name = strdup(sr_xpath_key_value(new_val ? new_val->xpath : old_val->xpath, "interface", "name", &xpath_ctx));
        sr_xpath_recover(&xpath_ctx);

        switch (op) {
            case SR_OP_CREATED:
                if (SR_LIST_T == new_val->type) {
                    /* create on list item - reset state vars */
                    has_addr = has_prefix = false;
                } else {
                    if (sr_xpath_node_name_eq(new_val->xpath, "ip")) {
                        strncpy(addr, new_val->data.string_val, strlen(new_val->data.string_val));
                        has_addr = true;
                    } else if (sr_xpath_node_name_eq(new_val->xpath, "prefix-length")) {
                        prefix = new_val->data.uint8_val;
                        has_prefix = true;
                    } else if (sr_xpath_node_name_eq(new_val->xpath, "netmask")) {
                        prefix = netmask_to_prefix(new_val->data.string_val);
                        has_prefix = true;
                    }
                    if (has_addr && has_prefix) {
                        op_rc = ipv46_config_add_remove(if_name, addr, prefix, is_ipv6, true /* add */);
                    }
                }
                break;
            case SR_OP_MODIFIED:
                //TODO Why is it using old_val and new_val?
                op_rc = interface_ipv46_config_modify(if_name, old_val, new_val, is_ipv6);
                break;
            case SR_OP_DELETED:
                if (SR_LIST_T == old_val->type) {
                    /* delete on list item - reset state vars */
                    has_addr = has_prefix = false;
                } else {
                    if (sr_xpath_node_name_eq(old_val->xpath, "ip")) {
                        strncpy(addr, old_val->data.string_val, strlen(old_val->data.string_val));
                        has_addr = true;
                    } else if (sr_xpath_node_name_eq(old_val->xpath, "prefix-length")) {
                        prefix = old_val->data.uint8_val;
                        has_prefix = true;
                    } else if (sr_xpath_node_name_eq(old_val->xpath, "netmask")) {
                        prefix = netmask_to_prefix(old_val->data.string_val);
                        has_prefix = true;
                    }
                    if (has_addr && has_prefix) {
                        op_rc = ipv46_config_add_remove(if_name, addr, prefix, is_ipv6, false /* !add */);
                    }
                }
                break;
            default:
                break;
        }
        if (SR_ERR_INVAL_ARG == op_rc) {
            sr_set_error(session, "Invalid interface name.", new_val ? new_val->xpath : old_val->xpath);
        }
        free(if_name);
        sr_free_val(old_val);
        sr_free_val(new_val);
    }
    sr_free_change_iter(iter);

    return op_rc;
}

/**
 * @brief Callback to be called by any config change under "/ietf-interfaces:interfaces-state/interface" path.
 * Does not provide any functionality, needed just to cover not supported config leaves.
 */
static int
ietf_interface_change_cb(sr_session_ctx_t *session, const char *xpath,
                         sr_notif_event_t event, void *private_ctx)
{
    UNUSED(session); UNUSED(xpath); UNUSED(event); UNUSED(private_ctx);

    SRP_LOG_INF("In %s", __FUNCTION__);

    return SR_ERR_OK;
}

/**
 * @brief Callback to be called by any request for state data under "/ietf-interfaces:interfaces-state/interface" path.
 */
static int
ietf_interface_state_cb(const char *xpath, sr_val_t **values,
                        size_t *values_cnt, uint64_t request_id,
                        const char *original_xpath, void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    struct elt* stack;
    sw_interface_dump_t *dump;
    sr_val_t *val = NULL;
    int vc = 5; //number of answer per interfaces
    int cnt = 0; //value counter
    int rc = SR_ERR_OK;

    SRP_LOG_INF("In %s", __FUNCTION__);

    if (!sr_xpath_node_name_eq(xpath, "interface"))
        goto nothing_todo; //no interface field specified

    /* dump interfaces */
    stack = interface_dump_all();
    if (!stack)
        goto nothing_todo; //no element returned

    /* allocate array of values to be returned */
    SRP_LOG_DBG("number of interfaces: %d", stack->id+1);
    rc = sr_new_values((stack->id + 1)* vc, &val);
    if (0 != rc)
        goto nothing_todo;

    foreach_stack_elt(stack) {
        dump = (sw_interface_dump_t *) data;

        SRP_LOG_DBG("State of interface %s", dump->interface_name);
        //TODO need support for type propvirtual
        sr_val_build_xpath(&val[cnt], "%s[name='%s']/type", xpath, dump->interface_name);
        sr_val_set_str_data(&val[cnt], SR_IDENTITYREF_T, "iana-if-type:ethernetCsmacd");
        cnt++;

        //Be careful, it needs if-mib feature to work !
        sr_val_build_xpath(&val[cnt], "%s[name='%s']/admin-status", xpath, dump->interface_name);
        sr_val_set_str_data(&val[cnt], SR_ENUM_T, dump->link_up_down ? "up" : "down");
        cnt++;

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/oper-status", xpath, dump->interface_name);
        sr_val_set_str_data(&val[cnt], SR_ENUM_T, dump->link_up_down ? "up" : "down");
        cnt++;

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/phys-address", xpath, dump->interface_name);
        if (dump->l2_address_length > 0) {
            sr_val_build_str_data(&val[cnt], SR_STRING_T,
                                  "%02x:%02x:%02x:%02x:%02x:%02x",
                                  dump->l2_address[0], dump->l2_address[1],
                                  dump->l2_address[2], dump->l2_address[3],
                                  dump->l2_address[4], dump->l2_address[5]);
        } else {
            sr_val_build_str_data(&val[cnt], SR_STRING_T, "%02x:%02x:%02x:%02x:%02x:%02x", 0,0,0,0,0,0);
        }
        cnt++;

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/speed", xpath, dump->interface_name);
        val[cnt].type = SR_UINT64_T;
        val[cnt].data.uint64_val = dump->link_speed;
        cnt++;

        free(dump);
    }

    *values = val;
    *values_cnt = cnt;

    return SR_ERR_OK;

nothing_todo:
    *values = NULL;
    *values_cnt = 0;
    return rc;
}

const xpath_t ietf_interfaces_xpaths[IETF_INTERFACES_SIZE] = {
    {
        .xpath = "/ietf-interfaces:interfaces/interface",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = ietf_interface_change_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED
    },
    {
        .xpath = "/ietf-interfaces:interfaces/interface/enabled",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = ietf_interface_enable_disable_cb,
        .private_ctx = NULL,
        .priority = 100,
        .opts = SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED
    },
    {
        .xpath = "/ietf-interfaces:interfaces/interface/ietf-ip:ipv4/address",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = ietf_interface_ipv46_address_change_cb,
        .private_ctx = NULL,
        .priority = 99,
        .opts = SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED
    },
    {
        .xpath = "/ietf-interfaces:interfaces/interface/ietf-ip:ipv6/address",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = ietf_interface_ipv46_address_change_cb,
        .private_ctx = NULL,
        .priority = 98,
        .opts = SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED
    },
    {
        .xpath = "/ietf-interfaces:interfaces-state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = ietf_interface_state_cb,
        .private_ctx = NULL,
        .priority = 98,
        //.opts = SR_SUBSCR_DEFAULT,
        .opts = SR_SUBSCR_CTX_REUSE
    }
};
