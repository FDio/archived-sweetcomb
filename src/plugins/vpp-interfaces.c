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

#include <sysrepo.h>
#include <sysrepo/plugins.h>
#include <sysrepo/values.h>
#include <sysrepo/xpath.h>
#include <srvpp.h>

/** forward declaration */
void sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_ctx);

/**
 * @brief Plugin context structure.
 */
typedef struct plugin_ctx_s {
    srvpp_ctx_t *srvpp_ctx;                  /**< srvpp context. */
    sr_subscription_ctx_t *sr_subscription;  /**< Sysrepo subscription context. */
} plugin_ctx_t;

/**
 * @brief Helper function for converting netmask into prefix length.
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
 * @brief Helper function for converting IPv4/IPv6 address string into binary representation.
 */
static void
ip_addr_str_to_binary(const char *ip_address_str, uint8_t *ip_address_bin, bool is_ipv6)
{
    struct in6_addr addr6 = { 0, };
    struct in_addr addr4 = { 0, };

    if (is_ipv6) {
        inet_pton(AF_INET6, ip_address_str, &(addr6));
        memcpy(ip_address_bin, &addr6, sizeof(addr6));
    } else {
        inet_pton(AF_INET, ip_address_str, &(addr4));
        memcpy(ip_address_bin, &addr4, sizeof(addr4));
    }
}

/**
 * @brief Helper function for converting VPP interface speed information into actual number in bits per second.
 */
static uint64_t
get_if_link_speed(uint8_t speed)
{
#define ONE_MEGABIT (uint64_t)1000000
    switch (speed) {
        case 1:
            /* 10M */
            return 10 * ONE_MEGABIT;
            break;
        case 2:
            /* 100M */
            return 100 * ONE_MEGABIT;
            break;
        case 4:
            /* 1G */
            return 1000 * ONE_MEGABIT;
            break;
        case 8:
            /* 10G */
            return 10000 * ONE_MEGABIT;
            break;
        case 16:
            /* 40 G */
            return 40000 * ONE_MEGABIT;
            break;
        case 32:
            /* 100G */
            return 100000 * ONE_MEGABIT;
            break;
        default:
            return 0;
    }
}

/**
 * @brief Enable or disable given interface.
 */
static int
interface_enable_disable(plugin_ctx_t *plugin_ctx, const char *if_name, bool enable)
{
    vl_api_sw_interface_set_flags_t *if_set_flags_req = NULL;
    uint32_t if_index = ~0;
    int rc = 0;

    SRP_LOG_DBG("%s interface '%s'", enable ? "Enabling" : "Disabling", if_name);

    /* get interface index */
    rc = srvpp_get_if_index(plugin_ctx->srvpp_ctx, if_name, &if_index);
    if (0 != rc) {
        SRP_LOG_ERR("Invalid interface name: %s", if_name);
        return SR_ERR_INVAL_ARG;
    }

    /* process VPP API request */
    if_set_flags_req = srvpp_alloc_msg(VL_API_SW_INTERFACE_SET_FLAGS, sizeof(*if_set_flags_req));

    if_set_flags_req->sw_if_index = ntohl(if_index);
    if_set_flags_req->admin_up_down = (uint8_t)enable;

    rc = srvpp_send_request(plugin_ctx->srvpp_ctx, if_set_flags_req, NULL);

    if (0 != rc) {
        SRP_LOG_ERR("Error by processing of the sw_interface_set_flags request, rc=%d", rc);
        return SR_ERR_OPERATION_FAILED;
    } else {
        return SR_ERR_OK;
    }
}

/**
 * @brief Callback to be called by any config change of "/ietf-interfaces:interfaces/interface/enabled" leaf.
 */
static int
interface_enable_disable_cb(sr_session_ctx_t *session, const char *xpath, sr_notif_event_t event, void *private_ctx)
{
    char *if_name = NULL;
    sr_change_iter_t *iter = NULL;
    sr_change_oper_t op = SR_OP_CREATED;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    sr_xpath_ctx_t xpath_ctx = { 0, };
    int rc = SR_ERR_OK, op_rc = SR_ERR_OK;

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
                op_rc = interface_enable_disable(private_ctx, if_name, new_val->data.bool_val);
                break;
            case SR_OP_DELETED:
                op_rc = interface_enable_disable(private_ctx, if_name, false /* !enable */);
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
 * @brief Add or remove IPv4/IPv6 address to/from an interface.
 */
static int
interface_ipv46_config_add_remove(plugin_ctx_t *plugin_ctx, const char *if_name, uint8_t *addr, uint8_t prefix,
        bool is_ipv6, bool add)
{
    vl_api_sw_interface_add_del_address_t *add_del_req = NULL;
    uint32_t if_index = ~0;
    int rc = 0;

    SRP_LOG_DBG("%s IP config on interface '%s'.", add ? "Adding" : "Removing", if_name);

    /* get interface index */
    rc = srvpp_get_if_index(plugin_ctx->srvpp_ctx, if_name, &if_index);
    if (0 != rc) {
        SRP_LOG_ERR("Invalid interface name: %s", if_name);
        return SR_ERR_INVAL_ARG;
    }

    /* process VPP API request */
    add_del_req = srvpp_alloc_msg(VL_API_SW_INTERFACE_ADD_DEL_ADDRESS, sizeof(*add_del_req));

    memcpy(&add_del_req->address, addr, is_ipv6 ? 16 : 4);
    add_del_req->is_ipv6 = (uint32_t)is_ipv6;
    add_del_req->address_length = prefix;

    add_del_req->sw_if_index = ntohl(if_index);
    add_del_req->is_add = (uint32_t)add;

    rc = srvpp_send_request(plugin_ctx->srvpp_ctx, add_del_req, NULL);

    if (0 != rc) {
        SRP_LOG_ERR("Error by processing of the sw_interface_set_flags request, rc=%d", rc);
        return SR_ERR_OPERATION_FAILED;
    } else {
        return SR_ERR_OK;
    }
}

/**
 * @brief Modify existing IPv4/IPv6 config on an interface.
 */
static int
interface_ipv46_config_modify(plugin_ctx_t *plugin_ctx, sr_session_ctx_t *session, const char *if_name,
        sr_val_t *old_val, sr_val_t *new_val, bool is_ipv6)
{
    sr_xpath_ctx_t xpath_ctx = { 0, };
    char *addr_str = NULL;
    uint8_t addr[16] = { 0, };
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
    addr_str = sr_xpath_key_value((char*)old_val->xpath, "address", "ip", &xpath_ctx);
    ip_addr_str_to_binary(addr_str, addr, is_ipv6);
    sr_xpath_recover(&xpath_ctx);

    /* delete old IP config */
    rc = interface_ipv46_config_add_remove(plugin_ctx, if_name, addr, prefix, is_ipv6, false /* remove */);
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
    rc = interface_ipv46_config_add_remove(plugin_ctx, if_name, addr, prefix, is_ipv6, true /* add */);
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
interface_ipv46_address_change_cb(sr_session_ctx_t *session, const char *xpath, sr_notif_event_t event, void *private_ctx)
{
    sr_change_iter_t *iter = NULL;
    sr_change_oper_t op = SR_OP_CREATED;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    sr_xpath_ctx_t xpath_ctx = { 0, };
    bool is_ipv6 = false, has_addr = false, has_prefix = false;
    uint8_t addr[16] = { 0, };
    uint8_t prefix = 0;
    char *node_name = NULL, *if_name = NULL;
    int rc = SR_ERR_OK, op_rc = SR_ERR_OK;

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
                        ip_addr_str_to_binary(new_val->data.string_val, addr, is_ipv6);
                        has_addr = true;
                    } else if (sr_xpath_node_name_eq(new_val->xpath, "prefix-length")) {
                        prefix = new_val->data.uint8_val;
                        has_prefix = true;
                    } else if (sr_xpath_node_name_eq(new_val->xpath, "netmask")) {
                        prefix = netmask_to_prefix(new_val->data.string_val);
                        has_prefix = true;
                    }
                    if (has_addr && has_prefix) {
                        op_rc = interface_ipv46_config_add_remove(private_ctx, if_name, addr, prefix, is_ipv6, true /* add */);
                    }
                }
                break;
            case SR_OP_MODIFIED:
                op_rc = interface_ipv46_config_modify(private_ctx, session, if_name, old_val, new_val, is_ipv6);
                break;
            case SR_OP_DELETED:
                if (SR_LIST_T == old_val->type) {
                    /* delete on list item - reset state vars */
                    has_addr = has_prefix = false;
                } else {
                    if (sr_xpath_node_name_eq(old_val->xpath, "ip")) {
                        ip_addr_str_to_binary(old_val->data.string_val, addr, is_ipv6);
                        has_addr = true;
                    } else if (sr_xpath_node_name_eq(old_val->xpath, "prefix-length")) {
                        prefix = old_val->data.uint8_val;
                        has_prefix = true;
                    } else if (sr_xpath_node_name_eq(old_val->xpath, "netmask")) {
                        prefix = netmask_to_prefix(old_val->data.string_val);
                        has_prefix = true;
                    }
                    if (has_addr && has_prefix) {
                        op_rc = interface_ipv46_config_add_remove(private_ctx, if_name, addr, prefix, is_ipv6, false /* !add */);
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
interface_change_cb(sr_session_ctx_t *session, const char *xpath, sr_notif_event_t event, void *private_ctx)
{
    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

    return SR_ERR_OK;
}

/**
 * @brief Callback to be called by any request for state data under "/ietf-interfaces:interfaces-state/interface" path.
 */
static int
interface_state_cb(const char *xpath, sr_val_t **values, size_t *values_cnt, void *private_ctx)
{
    plugin_ctx_t *plugin_ctx = private_ctx;
    vl_api_sw_interface_dump_t *if_dump_req = NULL;
    vl_api_sw_interface_details_t *if_details = NULL;
    void **details = NULL;
    size_t details_cnt = 0;
    sr_val_t *values_arr = NULL;
    size_t values_arr_size = 0, values_arr_cnt = 0;
    int rc = 0;

    SRP_LOG_DBG("Requesting state data for '%s'", xpath);

    if (! sr_xpath_node_name_eq(xpath, "interface")) {
        /* statistics, ipv4 and ipv6 state data not supported */
        *values_cnt = 0;
        return SR_ERR_OK;
    }

    /* dump interfaces */
    if_dump_req = srvpp_alloc_msg(VL_API_SW_INTERFACE_DUMP, sizeof(*if_dump_req));
    rc = srvpp_send_dumprequest(plugin_ctx->srvpp_ctx, if_dump_req, &details, &details_cnt);
    if (0 != rc) {
        SRP_LOG_ERR_MSG("Error by processing of a interface dump request.");
        return SR_ERR_INTERNAL;
    }

    /* allocate array of values to be returned */
    values_arr_size = details_cnt * 5;
    rc = sr_new_values(values_arr_size, &values_arr);
    if (0 != rc) {
        return rc;
    }

    for (size_t i = 0; i < details_cnt; i++) {
        if_details = (vl_api_sw_interface_details_t *) details[i];

        /* currently the only supported interface types are propVirtual / ethernetCsmacd */
        sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/type", xpath, if_details->interface_name);
        sr_val_set_str_data(&values_arr[values_arr_cnt], SR_IDENTITYREF_T,
                strstr((char*)if_details->interface_name, "local") ? "iana-if-type:propVirtual" : "iana-if-type:ethernetCsmacd");
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
        }

        sr_val_build_xpath(&values_arr[values_arr_cnt], "%s[name='%s']/speed", xpath, if_details->interface_name);
        values_arr[values_arr_cnt].type = SR_UINT64_T;
        values_arr[values_arr_cnt].data.uint64_val = get_if_link_speed(if_details->link_speed);
        values_arr_cnt++;
    }

    SRP_LOG_DBG("Returning %zu state data elements for '%s'", values_arr, xpath);

    *values = values_arr;
    *values_cnt = values_arr_cnt;
    return SR_ERR_OK;
}

/**
 * @brief Callback to be called by plugin daemon upon plugin load.
 */
int
sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx)
{
    plugin_ctx_t *ctx = NULL;
    int rc = SR_ERR_OK;

    SRP_LOG_DBG_MSG("Initializing vpp-interfaces plugin.");

    /* allocate the plugin context */
    ctx = calloc(1, sizeof(*ctx));
    if (NULL == ctx) {
        return SR_ERR_NOMEM;
    }

    /* get srvpp context */
    ctx->srvpp_ctx = srvpp_get_ctx();
    if (NULL == ctx->srvpp_ctx) {
        return SR_ERR_INIT_FAILED;
    }

    /* setup handlers for required VPP API messages */
    srvpp_setup_handler(SW_INTERFACE_SET_FLAGS_REPLY, sw_interface_set_flags_reply);
    srvpp_setup_handler(SW_INTERFACE_ADD_DEL_ADDRESS_REPLY, sw_interface_add_del_address_reply);

    rc = sr_subtree_change_subscribe(session, "/ietf-interfaces:interfaces/interface",
            interface_change_cb, ctx, 0, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &ctx->sr_subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_subtree_change_subscribe(session, "/ietf-interfaces:interfaces/interface/enabled",
            interface_enable_disable_cb, ctx, 100, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &ctx->sr_subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_subtree_change_subscribe(session, "/ietf-interfaces:interfaces/interface/ietf-ip:ipv4/address",
            interface_ipv46_address_change_cb, ctx, 99, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &ctx->sr_subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_subtree_change_subscribe(session, "/ietf-interfaces:interfaces/interface/ietf-ip:ipv6/address",
            interface_ipv46_address_change_cb, ctx, 98, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &ctx->sr_subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_dp_get_items_subscribe(session, "/ietf-interfaces:interfaces-state",
            interface_state_cb, ctx, SR_SUBSCR_CTX_REUSE, &ctx->sr_subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    *private_ctx = ctx;

    SRP_LOG_INF_MSG("vpp-interfaces plugin initialized successfully.");

    return SR_ERR_OK;

error:
    SRP_LOG_ERR_MSG("Error by initialization of the vpp-interfaces plugin.");
    sr_plugin_cleanup_cb(session, ctx);
    return rc;
}

/**
 * @brief Callback to be called by plugin daemon upon plugin unload.
 */
void
sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_ctx)
{
    plugin_ctx_t *ctx = (plugin_ctx_t *) private_ctx;

    SRP_LOG_DBG_MSG("Cleanup of vpp-interfaces plugin requested.");

    sr_unsubscribe(session, ctx->sr_subscription);

    srvpp_release_ctx(ctx->srvpp_ctx);
    free(ctx);
}

/**
 * @brief Callback to be called by plugin daemon periodically, to check whether the plugin and managed app is healthy.
 */
/*
int
sr_plugin_health_check_cb(sr_session_ctx_t *session, void *private_ctx)
{
    plugin_ctx_t *ctx = (plugin_ctx_t *) private_ctx;
    vl_api_control_ping_t *ping = NULL;
    int rc = 0;

    ping = srvpp_alloc_msg(VL_API_CONTROL_PING, sizeof(*ping));

    rc = srvpp_send_request(ctx->srvpp_ctx, ping, NULL);

    if (0 != rc) {
        return SR_ERR_DISCONNECT;
    } else {
        return SR_ERR_OK;
    }
}
*/
