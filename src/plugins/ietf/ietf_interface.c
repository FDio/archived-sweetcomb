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

#include <vom/interface.hpp>
#include <vom/om.hpp>
#include <vom/l3_binding.hpp>
#include <vom/l3_binding_cmds.hpp>
#include <vom/route.hpp>

#include <string>
#include <exception>

#include <stdlib.h>

#include "sc_plugins.h"

using namespace VOM;

#define MODULE_NAME "ietf-interfaces"

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
    rc_t vom_rc = rc_t::OK;

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

        std::shared_ptr<interface> intf;
        intf = interface::find(if_name);
        if (nullptr == intf) {
            SRP_LOG_ERR("Interface %s does not exist", if_name);
            return SR_ERR_INVAL_ARG;
        }

        switch (op) {
            case SR_OP_CREATED:
            case SR_OP_MODIFIED:
                if (new_val->data.bool_val) {
                    intf->set(interface::admin_state_t::UP);
                } else {
                    intf->set(interface::admin_state_t::DOWN);
                }
                break;
            case SR_OP_DELETED:
                intf->set(interface::admin_state_t::DOWN);
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

        vom_rc = OM::write(MODULE_NAME, *intf);
        if (rc_t::OK != vom_rc) {
            SRP_LOG_ERR_MSG("Error write data to vpp");
        }
    }
    sr_free_change_iter(iter);

    return op_rc;
}

static int
ipv46_config_add_remove(const std::string &if_name,
                        const std::string &addr, uint8_t prefix,
                        bool add)
{
    l3_binding *l3;
    rc_t rc = rc_t::OK;

    SRP_LOG_INF("Call addr: %s, %d", addr.c_str(), prefix);

    std::shared_ptr<interface> intf = interface::find(if_name);
    if (nullptr == intf) {
        SRP_LOG_ERR_MSG("Interfaces does not exist");
        return SR_ERR_INVAL_ARG;
    }

    try {
        route::prefix_t pfx(addr, prefix);
        l3 = new l3_binding(*intf, pfx);
        HW::item<handle_t> hw_ifh(2, rc_t::OK);
        if (add) {
            HW::item<bool> hw_l3_bind(true, rc_t::OK);
            l3_binding_cmds::bind_cmd(hw_l3_bind, hw_ifh.data(), pfx);
        } else {
            SRP_LOG_INF("Unbind addr: %s, %d", addr.c_str(), prefix);
            HW::item<bool> hw_l3_bind(false, rc_t::OK);
            l3_binding_cmds::unbind_cmd(hw_l3_bind, hw_ifh.data(), pfx);
        }
    } catch (std::exception &exc) {  //catch boost exception from prefix_t
        SRP_LOG_ERR("Error: %s", exc.what());
        return SR_ERR_OPERATION_FAILED;
    }

    SRP_LOG_DBG_MSG("WRITE");
    rc = OM::write(MODULE_NAME, *l3);
    if (rc_t::OK != rc) {
        SRP_LOG_ERR_MSG("Error write data to vpp");
        return SR_ERR_OPERATION_FAILED;
    }

    return SR_ERR_OK;
}

static void
parse_interface_ipv46_address(sr_val_t *val, std::string &addr,
                              uint8_t &prefix)
{
    if (nullptr == val) {
        throw std::runtime_error("Null pointer");
    }

    if (SR_LIST_T == val->type) {
        /* create on list item - reset state vars */
//         SRP_LOG_INF("List: %s", addr.c_str());
//         addr.clear();
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

    /* no-op for apply, we only care about SR_EV_ENABLED, SR_EV_VERIFY, SR_EV_ABORT */
    if (SR_EV_APPLY == event) {
        return SR_ERR_OK;
    }
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
    }

    if (del && !old_addr.empty()) {
        op_rc = ipv46_config_add_remove(if_name, old_addr, old_prefix,
                                        false /* del */);
    }

    if (create && !new_addr.empty()) {
        op_rc = ipv46_config_add_remove(if_name, new_addr, new_prefix,
                                        true /* add */);
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

    struct stat st = {0};
    const char cmd[] = "/usr/bin/sysrepocfg --format=json -x "\
    "/tmp/sweetcomb/ietf-interfaces.json --datastore=running ietf-interfaces &";
    int rc = 0;

    SRP_LOG_INF("In %s", __FUNCTION__);

    if (!export_backup) {
        return SR_ERR_OK;
    }

    if (-1 == stat(BACKUP_DIR_PATH, &st)) {
        mkdir(BACKUP_DIR_PATH, 0700);
    }

    rc = system(cmd);
    if (0 != rc) {
        SRP_LOG_ERR("Failed restore backup for module: ietf-interfaces, errno: %s",
                    strerror(errno));
    }
    SRP_LOG_DBG_MSG("ietf-interfaces modules, backup");

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
    sr_val_t *val = nullptr;
    int vc = 5; //number of answer per interfaces
    int cnt = 0; //value counter
    int interface_num = 0;
    std::ostringstream os;
    int rc = SR_ERR_OK;

    SRP_LOG_INF("In %s", __FUNCTION__);

    if (!sr_xpath_node_name_eq(xpath, "interface"))
        goto nothing_todo; //no interface field specified

    //TODO: Not effective
    for (auto inter = interface::cbegin(); inter != interface::cend(); inter++) {
        interface_num++;
    }

    /* allocate array of values to be returned */
    SRP_LOG_DBG("number of interfaces: %d",  interface_num + 1);
    rc = sr_new_values((interface_num + 1)* vc, &val);
    if (0 != rc)
        goto nothing_todo;

    for (auto inter = interface::cbegin(); inter != interface::cend(); inter++) {
        std::shared_ptr<interface> interface = inter->second.lock();
        const char *if_name = interface->name().c_str();

        SRP_LOG_DBG("State of interface %s", if_name);
        //TODO need support for type propvirtual
        sr_val_build_xpath(&val[cnt], "%s[name='%s']/type", xpath,
                           interface->name().c_str());
        sr_val_set_str_data(&val[cnt], SR_IDENTITYREF_T, "iana-if-type:ethernetCsmacd");
        cnt++;

        interface->dump(os);

        const std::string token("oper-state:");
        std::string tmp = os.str();
        std::string state = tmp.substr(tmp.find(token));
        state = state.substr(token.length(), state.find("]") - token.length());

        //Be careful, it needs if-mib feature to work !
        sr_val_build_xpath(&val[cnt], "%s[name='%s']/admin-status", xpath,
                           if_name);
        sr_val_set_str_data(&val[cnt], SR_ENUM_T, state.c_str());
        cnt++;

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/oper-status", xpath,
                           if_name);
        sr_val_set_str_data(&val[cnt], SR_ENUM_T, state.c_str());
        cnt++;

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/phys-address", xpath,
                           if_name);
        auto l2_address = interface->l2_address();
        sr_val_build_str_data(&val[cnt], SR_STRING_T, "%s",
                              l2_address.to_string().c_str());
        cnt++;

        sr_val_build_xpath(&val[cnt], "%s[name='%s']/speed", xpath, if_name);
        val[cnt].type = SR_UINT64_T;
        //FIXME: VOM don't support print MTU??? Must find
        val[cnt].data.uint64_val = 9000;
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
