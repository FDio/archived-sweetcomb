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

#include <string>
#include <exception>
#include <memory>

#include <vom/interface.hpp>
#include <vom/om.hpp>
#include <vom/l3_binding.hpp>
#include <vom/route.hpp>

#include "sc_plugins.h"

#include <vpp-api/client/stat_client.h>

#define MODULE_NAME "ietf-interfaces"

using VOM::interface;
using VOM::OM;
using VOM::l3_binding;
using VOM::rc_t;

/**
 * @brief Callback to be called by any config change of
 * "/ietf-interfaces:interfaces/interface/enabled" leaf.
 */
static int
ietf_interface_enable_disable_cb(sr_session_ctx_t *session, const char *xpath,
                                 sr_notif_event_t event, void *private_ctx)
{
    UNUSED(private_ctx);
    char *if_name = nullptr;
    sr_change_iter_t *iter = nullptr;
    sr_change_oper_t op = SR_OP_CREATED;
    sr_val_t *old_val = nullptr;
    sr_val_t *new_val = nullptr;
    sr_xpath_ctx_t xpath_ctx = { 0, };
    int rc = SR_ERR_OK, op_rc = SR_ERR_OK;
    std::shared_ptr<interface> intf;

    SRP_LOG_INF("In %s", __FUNCTION__);

    /* no-op for apply, we only care about SR_EV_{ENABLED,VERIFY,ABORT} */
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

        SRP_LOG_DBG("A change detected in '%s', op=%d",
                    new_val ? new_val->xpath : old_val->xpath, op);
        if_name = sr_xpath_key_value(new_val ? new_val->xpath : old_val->xpath,
                                     "interface", "name", &xpath_ctx);

        intf = interface::find(if_name);
        if (nullptr == intf) {
            SRP_LOG_ERR_MSG("Interface does not exist");
            return SR_ERR_INVAL_ARG;
        }

        switch (op) {
            case SR_OP_CREATED:
            case SR_OP_MODIFIED:
                if (new_val->data.bool_val)
                    intf->set(interface::admin_state_t::UP);
                else
                    intf->set(interface::admin_state_t::DOWN);
                break;
            case SR_OP_DELETED:
                intf->set(interface::admin_state_t::DOWN);
                break;
            default:
                break;
        }
        sr_xpath_recover(&xpath_ctx);

        sr_free_val(old_val);
        sr_free_val(new_val);

        /* Commit the changes to VOM DB and VPP */
        if ( OM::write("enable/"+intf->key(), *intf) != rc_t::OK ) {
            SRP_LOG_ERR_MSG("Fail writing changes to VPP");
            return SR_ERR_OPERATION_FAILED;
        }

    }
    sr_free_change_iter(iter);

    return SR_ERR_OK;
}

static int
ipv46_config_add_remove(const std::string &if_name,
                        const std::string &addr, uint8_t prefix,
                        bool add)
{
    std::shared_ptr<l3_binding> l3;
    std::shared_ptr<interface> intf;
    rc_t rc = rc_t::OK;

    intf = interface::find(if_name);
    if (nullptr == intf) {
        SRP_LOG_ERR_MSG("Interface does not exist");
        return SR_ERR_INVAL_ARG;
    }

    try {

        VOM::route::prefix_t pfx(addr, prefix);
        l3 = std::make_shared<l3_binding>(*intf, pfx);

        #define KEY(l3) "l3_" + l3->itf().name() + "_" + l3->prefix().to_string()
        if (add) {
            /* Commit the changes to VOM DB and VPP */
            if ( OM::write(KEY(l3), *l3) != rc_t::OK ) {
                SRP_LOG_ERR_MSG("Fail writing changes to VPP");
                return SR_ERR_OPERATION_FAILED;
            }
        } else {
            OM::remove(KEY(l3));
        }
        #undef KEY

    } catch (std::exception &exc) {  //catch boost exception from prefix_t
        SRP_LOG_ERR("Error: %s", exc.what());
        return SR_ERR_OPERATION_FAILED;
    }

    return SR_ERR_OK;
}

static void
parse_interface_ipv46_address(sr_val_t *val, std::string &addr,
                              uint8_t &prefix)
{
    if (val == nullptr)
        throw std::runtime_error("Null pointer");

    if (SR_LIST_T == val->type) {
        /* create on list item - reset state vars */
        addr.clear();
    } else {
        if (sr_xpath_node_name_eq(val->xpath, "ip")) {
            addr = val->data.string_val;
        } else if (sr_xpath_node_name_eq(val->xpath, "prefix-length")) {
            prefix = val->data.uint8_val;
        } else if (sr_xpath_node_name_eq(val->xpath, "netmask")) {
            prefix = netmask_to_prefix(val->data.string_val);
        }
    }
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
    sr_change_iter_t *iter = nullptr;
    sr_change_oper_t op = SR_OP_CREATED;
    sr_val_t *old_val = nullptr;
    sr_val_t *new_val = nullptr;
    sr_xpath_ctx_t xpath_ctx = { 0, };
    std::string new_addr, old_addr;
    std::string if_name;
    uint8_t new_prefix = 0;
    uint8_t old_prefix = 0;
    int rc = SR_ERR_OK, op_rc = SR_ERR_OK;
    bool create = false;
    bool del = false;

    SRP_LOG_INF("In %s", __FUNCTION__);

    /* no-op for apply, we only care about SR_EV_{ENABLED,VERIFY,ABORT} */
    if (SR_EV_APPLY == event)
        return SR_ERR_OK;

    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

    sr_xpath_recover(&xpath_ctx);

    /* get changes iterator */
    rc = sr_get_changes_iter(session, xpath, &iter);
    if (SR_ERR_OK != rc) {
        sr_free_change_iter(iter);
        SRP_LOG_ERR("Unable to retrieve change iterator: %s", sr_strerror(rc));
        return rc;
    }

    foreach_change(session, iter, op, old_val, new_val) {

        SRP_LOG_DBG("A change detected in '%s', op=%d",
                    new_val ? new_val->xpath : old_val->xpath, op);

        if_name = sr_xpath_key_value(new_val ? new_val->xpath : old_val->xpath,
                                     "interface", "name", &xpath_ctx);
        sr_xpath_recover(&xpath_ctx);
        if ((new_val && SR_LIST_T == new_val->type) ||
            (old_val && SR_LIST_T == old_val->type)) {

            if (del && !old_addr.empty()) {
                op_rc = ipv46_config_add_remove(if_name, old_addr, old_prefix,
                                                false /* del */);
            }

            if (create && !new_addr.empty()) {
                op_rc = ipv46_config_add_remove(if_name, new_addr, new_prefix,
                                                true /* add */);
            }

            create = false;
            del = false;
            new_addr.clear();
            old_addr.clear();
            new_prefix = 0;
            old_prefix = 0;
        }

        try {
            switch (op) {
                case SR_OP_CREATED:
                    create = true;
                    parse_interface_ipv46_address(new_val, new_addr, new_prefix);
                    break;
                case SR_OP_MODIFIED:
                    create = true;
                    del = true;
                    parse_interface_ipv46_address(old_val, old_addr, old_prefix);
                    parse_interface_ipv46_address(new_val, new_addr, new_prefix);
                    break;
                case SR_OP_DELETED:
                    del = true;
                    parse_interface_ipv46_address(old_val, old_addr, old_prefix);
                    break;
                default:
                    break;
            }
        } catch (std::exception &exc) {
            SRP_LOG_ERR("Error: %s", exc.what());
        }
        sr_free_val(old_val);
        sr_free_val(new_val);

        if (del && !old_addr.empty()) {
            op_rc = ipv46_config_add_remove(if_name, old_addr, old_prefix,
                                            false /* del */);
        }

        if (create && !new_addr.empty()) {
            op_rc = ipv46_config_add_remove(if_name, new_addr, new_prefix,
                                            true /* add */);
        }

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

static std::string
yang_interface_type(const interface::type_t type)
{
    if (type == interface::type_t::BVI)
        return ""; //TODO
    else if (type == interface::type_t::VXLAN)
        return "iana-if-type:vxlan-tunnel"; //v3po type
    else if (type == interface::type_t::AFPACKET)
        return ""; //TODO
    else if (type == interface::type_t::LOCAL)
        return ""; //TODO
    else if (type == interface::type_t::TAPV2)
        return "iana-if-type:tap"; //v3po type
    else if (type == interface::type_t::VHOST)
        return "iana-if-type:vhost-user"; //v3po type
    else if (type == interface::type_t::BOND)
        return ""; //TODO
    else if (type == interface::type_t::PIPE)
        return ""; //TODO
    else if (type == interface::type_t::PIPE_END)
        return ""; //TODO
    else if (type == interface::type_t::ETHERNET)
        return "iana-if-type:ethernetCsmacd"; //IANA ifType
    else if (type == interface::type_t::LOOPBACK)
        return "iana-if-type:softwareLoopback"; //IANA ifType
    else
        return "";
}

/**
 * @brief Callback to be called by any request for state data under "/ietf-interfaces:interfaces-state/interface" path.
 * Here we reply systematically with all interfaces, it the responsability of
 * sysrepo apply a filter not to answer undesired interfaces.
 */
static int
ietf_interface_state_cb(const char *xpath, sr_val_t **values,
                        size_t *values_cnt, uint64_t request_id,
                        const char *original_xpath, void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    struct elt* stack;
    sr_val_t *val = nullptr;
    int vc = 5; //number of answer per interfaces
    int cnt = 0; //value counter
    int interface_num = 0;
    std::ostringstream os;
    int rc = SR_ERR_OK;
    std::shared_ptr<interface> interface;

    SRP_LOG_INF("In %s", __FUNCTION__);

    if (!sr_xpath_node_name_eq(xpath, "interface"))
        goto nothing_todo; //no interface field specified

    interface_num =  std::distance(interface::cbegin(), interface::cend());

    /* allocate array of values to be returned */
    SRP_LOG_DBG("number of interfaces: %d",  interface_num + 1);
    rc = sr_new_values((interface_num + 1)* vc, &val);
    if (0 != rc)
        goto nothing_todo;

    for (auto it = interface::cbegin(); it != interface::cend(); it++) {
        interface = it->second.lock();

        SRP_LOG_DBG("State of interface %s", interface->name().c_str());

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/type", xpath,
                           interface->name().c_str());
        sr_val_set_str_data(&val[cnt], SR_IDENTITYREF_T,
                            yang_interface_type(interface->type()).c_str());
        cnt++;

        /* it needs if-mib YANG feature to work !
         * admin-state: state as required by configuration */
        sr_val_build_xpath(&val[cnt], "%s[name='%s']/admin-status", xpath,
                           interface->name().c_str());
        sr_val_set_str_data(&val[cnt], SR_ENUM_T, "down"); //TODO "up" or "down"
        cnt++;

        /* oper-state: effective state. can differ from admin-state */
        sr_val_build_xpath(&val[cnt], "%s[name='%s']/oper-status", xpath,
                           interface->name().c_str());
        sr_val_set_str_data(&val[cnt], SR_ENUM_T, "down"); //TODO "up" or "down"
        cnt++;

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/phys-address", xpath,
                           interface->name().c_str());
        sr_val_build_str_data(&val[cnt], SR_STRING_T, "%s",
                              interface->l2_address().to_string().c_str());
        cnt++;
    }

    *values = val;
    *values_cnt = cnt;

    return SR_ERR_OK;

nothing_todo:
    *values = nullptr;
    *values_cnt = 0;
    return rc;
}

/**
 * @brief Callback to be called by any request for state data under
 * "/ietf-interfaces:interfaces-state/interface/statistics" path.
 */
static int
interface_statistics_cb(const char *xpath, sr_val_t **values,
                        size_t *values_cnt, uint64_t request_id,
                        const char *original_xpath, void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    sr_val_t *val = NULL;
    int vc = 8;
    int cnt = 0; //value counter
    int rc = SR_ERR_OK;
    sr_xpath_ctx_t state = {0};
    char *tmp;
    char interface_name[VPP_INTFC_NAME_LEN] = {0};
    std::shared_ptr<interface> interface;
    interface::stats_t stats;

    SRP_LOG_INF("In %s", __FUNCTION__);

    /* Retrieve the interface asked */
    tmp = sr_xpath_key_value((char*) xpath, "interface", "name", &state);
    if (!tmp) {
        SRP_LOG_ERR_MSG("XPATH interface name not found");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(interface_name, tmp, VPP_INTFC_NAME_LEN);
    sr_xpath_recover(&state);

    /* allocate array of values to be returned */
    rc = sr_new_values(vc, &val);
    if (0 != rc)
        goto nothing_todo;

    //TODO this probably does not work
    interface = interface::find(interface_name);

    stats = interface->get_stats();

    //if/rx
    sr_val_build_xpath(&val[cnt], "%s/in-octets", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_rx.bytes;
    cnt++;

    //if/rx-unicast
    sr_val_build_xpath(&val[cnt], "%s/in-unicast-pkts", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_rx_unicast.packets;
    cnt++;

    //if/rx-broadcast
    sr_val_build_xpath(&val[cnt], "%s/in-broadcast-pkts", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_rx_broadcast.packets;
    cnt++;

    //if/rx-multicast
    sr_val_build_xpath(&val[cnt], "%s/in-multicast-pkts", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_rx_multicast.packets;
    cnt++;

    //if/tx
    sr_val_build_xpath(&val[cnt], "%s/out-octets", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_tx.bytes;
    cnt++;

    //if/tx-unicast
    sr_val_build_xpath(&val[cnt], "%s/out-unicast-pkts", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_tx_unicast.packets;
    cnt++;

    //if/tx-broadcast
    sr_val_build_xpath(&val[cnt], "%s/out-broadcast-pkts", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_tx_broadcast.packets;
    cnt++;

    //if/tx-multicast
    sr_val_build_xpath(&val[cnt], "%s/out-multicast-pkts", xpath, 5);
    val[cnt].type = SR_UINT64_T;
    val[cnt].data.uint64_val = stats.m_tx_multicast.packets;
    cnt++;

    *values = val;
    *values_cnt = cnt;

    return SR_ERR_OK;

nothing_todo:
    *values = NULL;
    *values_cnt = 0;
    return rc;
}


int
ietf_interface_init(sc_plugin_main_t *pm)
{
    int rc = SR_ERR_OK;
    SRP_LOG_DBG_MSG("Initializing ietf-interface plugin.");

    rc = sr_subtree_change_subscribe(pm->session, "/ietf-interfaces:interfaces/interface",
            ietf_interface_change_cb, nullptr, 0, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_subtree_change_subscribe(pm->session, "/ietf-interfaces:interfaces/interface/enabled",
            ietf_interface_enable_disable_cb, nullptr, 100, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_subtree_change_subscribe(pm->session, "/ietf-interfaces:interfaces/interface/ietf-ip:ipv4/address",
            ietf_interface_ipv46_address_change_cb, nullptr, 99, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_subtree_change_subscribe(pm->session, "/ietf-interfaces:interfaces/interface/ietf-ip:ipv6/address",
            ietf_interface_ipv46_address_change_cb, nullptr, 98, SR_SUBSCR_CTX_REUSE | SR_SUBSCR_EV_ENABLED, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_dp_get_items_subscribe(pm->session, "/ietf-interfaces:interfaces-state",
            ietf_interface_state_cb, nullptr, SR_SUBSCR_CTX_REUSE, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_dp_get_items_subscribe(pm->session, "/ietf-interfaces:interfaces-state/interface/statistics",
            interface_statistics_cb, NULL, SR_SUBSCR_CTX_REUSE, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    SRP_LOG_DBG_MSG("ietf-interface plugin initialized successfully.");
    return SR_ERR_OK;

error:
    SRP_LOG_ERR("Error by initialization of ietf-interface plugin. Error : %d", rc);
    return rc;
}

void
ietf_interface_exit(__attribute__((unused)) sc_plugin_main_t *pm)
{
}

SC_INIT_FUNCTION(ietf_interface_init);
SC_EXIT_FUNCTION(ietf_interface_exit);
