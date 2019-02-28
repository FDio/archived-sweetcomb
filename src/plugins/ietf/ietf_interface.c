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
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ietf_interface.h"

#include "sc_vpp_interface.h"
#include "sc_vpp_ip.h"

/**
 * @brief Helper function for converting netmask (ex: 255.255.255.0)
 * to prefix length (ex: 24).
 */
static uint8_t
netmask_to_prefix(const char *netmask)
{
    in_addr_t n = 0;
    uint8_t i = 0;

    inet_pton(AF_INET, netmask, &n);

    while (n > 0) {
        n = n >> 1;
        i++;
    }

    return i;
}

/**
 * @brief Callback to be called by any config change of
 * "/ietf-interfaces:interfaces/interface/enabled" leaf.
 */
static int
ietf_interface_enable_disable_cb(sr_session_ctx_t *session, const char *xpath,
                                 sr_notif_event_t event, void *private_ctx)
{
    char *if_name = NULL;
    sr_change_iter_t *iter = NULL;
    sr_change_oper_t op = SR_OP_CREATED;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    sr_xpath_ctx_t xpath_ctx = { 0, };
    int rc = SR_ERR_OK, op_rc = SR_ERR_OK;

    SRP_LOG_INF("In %s", __FUNCTION__);
    /* no-op for apply, we only care about SR_EV_ENABLED, SR_EV_VERIFY, SR_EV_ABORT */
    if (SR_EV_APPLY == event) {
        return SR_ERR_OK;
    }
    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

    /* get changes iterator */
    rc = sr_get_changes_iter(session, xpath, &iter);
    if (SR_ERR_OK != rc) {
        SRP_LOG_ERR("Unable to retrieve change iterator: %s", sr_strerror(rc));
        return rc;
    }

    /* iterate over all changes */
    while ((SR_ERR_OK == op_rc || event == SR_EV_ABORT) &&
            (SR_ERR_OK == (rc = sr_get_change_next(session, iter, &op, &old_val, &new_val)))) {

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

static int free_sw_interface_dump_ctx(dump_all_ctx * dctx)
{
  if(dctx == NULL)
    return -1;

  if(dctx->intfcArray != NULL)
    {
      free(dctx->intfcArray);
    }

    dctx->intfcArray = NULL;
    dctx->capacity = 0;
    dctx->num_ifs = 0;

    return 0;
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
 * @brief Callback to be called by any config change in subtrees "/ietf-interfaces:interfaces/interface/ietf-ip:ipv4/address"
 * or "/ietf-interfaces:interfaces/interface/ietf-ip:ipv6/address".
 */
static int
ietf_interface_ipv46_address_change_cb(sr_session_ctx_t *session,
                                       const char *xpath,
                                       sr_notif_event_t event,
                                       void *private_ctx)
{
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
        SRP_LOG_ERR("Unable to retrieve change iterator: %s", sr_strerror(rc));
        return rc;
    }

    /* iterate over all changes */
    while ((SR_ERR_OK == op_rc || event == SR_EV_ABORT) &&
            (SR_ERR_OK == (rc = sr_get_change_next(session, iter, &op, &old_val, &new_val)))) {

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
    SRP_LOG_INF("In %s", __FUNCTION__);
    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

    return SR_ERR_OK;
}

/**
 * @brief Callback to be called by any request for state data under "/ietf-interfaces:interfaces-state/interface" path.
 */
static int
ietf_interface_state_cb(const char *xpath, sr_val_t **values,
                        size_t *values_cnt,
                        __attribute__((unused)) uint64_t request_id,
                        __attribute__((unused)) const char *original_xpath,
                        __attribute__((unused)) void *private_ctx)
{
    sr_val_t *values_arr = NULL;
    int values_arr_size = 0, values_arr_cnt = 0;
    dump_all_ctx dctx;
    int nb_iface;
    vpp_interface_t* if_details;
    int rc = 0;

    SRP_LOG_INF("In %s", __FUNCTION__);

    if (! sr_xpath_node_name_eq(xpath, "interface")) {
        /* statistics, ipv4 and ipv6 state data not supported */
      *values = NULL;
      *values_cnt = 0;
      return SR_ERR_OK;
    }

    /* dump interfaces */
    nb_iface = interface_dump_all(&dctx);
    if (nb_iface <= 0) {
        SRP_LOG_ERR_MSG("Error by processing of a interface dump request.");
        free_sw_interface_dump_ctx(&dctx);
        return SR_ERR_INTERNAL;
    }

    /* allocate array of values to be returned */
    values_arr_size = nb_iface * 5;
    rc = sr_new_values(values_arr_size, &values_arr);
    if (0 != rc) {
        free_sw_interface_dump_ctx(&dctx);
        return rc;
    }

    int i = 0;
    for (; i < nb_iface; i++) {
        if_details = dctx.intfcArray+i;

        /* currently the only supported interface types are propVirtual / ethernetCsmacd */
        sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/type", xpath, if_details->interface_name);
        sr_val_set_str_data(&values_arr[values_arr_cnt], SR_IDENTITYREF_T,
                strstr((char*)if_details->interface_name, "local0") ? "iana-if-type:propVirtual" : "iana-if-type:ethernetCsmacd");
        values_arr_cnt++;

        sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/admin-status", xpath, if_details->interface_name);
        sr_val_set_str_data(&values_arr[values_arr_cnt], SR_ENUM_T, if_details->admin_up_down ? "up" : "down");
        values_arr_cnt++;

        sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/oper-status", xpath, if_details->interface_name);
        sr_val_set_str_data(&values_arr[values_arr_cnt], SR_ENUM_T, if_details->link_up_down ? "up" : "down");
        values_arr_cnt++;

        if (if_details->l2_address_length > 0) {
            sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/phys-address", xpath, if_details->interface_name);
            sr_val_build_str_data(&values_arr[values_arr_cnt], SR_STRING_T, "%02x:%02x:%02x:%02x:%02x:%02x",
                    if_details->l2_address[0], if_details->l2_address[1], if_details->l2_address[2],
                    if_details->l2_address[3], if_details->l2_address[4], if_details->l2_address[5]);
            values_arr_cnt++;
        } else {
            sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/phys-address", xpath, if_details->interface_name);
            sr_val_build_str_data(&values_arr[values_arr_cnt], SR_STRING_T, "%02x:%02x:%02x:%02x:%02x:%02x", 0,0,0,0,0,0);
            values_arr_cnt++;
        }

        sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/speed", xpath, if_details->interface_name);
        values_arr[values_arr_cnt].type = SR_UINT64_T;
        values_arr[values_arr_cnt].data.uint64_val = if_details->link_speed;
        values_arr_cnt++;
    }

    SRP_LOG_DBG("Returning %zu state data elements for '%s'", values_arr, xpath);

    *values = values_arr;
    *values_cnt = values_arr_cnt;

    free_sw_interface_dump_ctx(&dctx);

    return SR_ERR_OK;
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
