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

#include <vom/interface.hpp>
#include <vom/om.hpp>

#include <vpp-oper/interface.hpp>

#include <sc_plugins.h>

using VOM::interface;
using VOM::OM;
using VOM::HW;
using VOM::rc_t;

using type_t = VOM::interface::type_t;
using admin_state_t = VOM::interface::admin_state_t;

class interface_builder {
    public:
        interface_builder() {}

        shared_ptr<VOM::interface> build() {
            if (m_name.empty() || m_type.empty())
                return nullptr;
            return make_shared<interface>(m_name, type_t::from_string(m_type),
                                          admin_state_t::from_int(m_state));
        }

        /* Getters */
        string name() {
            return m_name;
        }

        /* Setters */
        interface_builder& set_name(string n) {
            m_name = n;
            return *this;
        }

        interface_builder& set_type(string t) {
            if (t == "iana-if-type:ethernetCsmacd")
                m_type = "ETHERNET";
            return *this;
        }

        interface_builder& set_state(bool enable) {
            m_state = enable;
            return *this;
        }

        std::string to_string() {
            std::ostringstream os;
            os << m_name << "," << m_type << "," << m_state;
            return os.str();
        }

    private:
        string m_name;
        string m_type;
        bool m_state;
};


// XPATH: /openconfig-interfaces:interfaces/interface[name='%s']/config/
static int
oc_interfaces_config_cb(sr_session_ctx_t *ds, const char *xpath,
                        sr_notif_event_t event, void *private_ctx)
{
    UNUSED(private_ctx);
    interface_builder builder;
    shared_ptr<VOM::interface> intf;
    string intf_name;
    sr_change_iter_t *it = nullptr;
    sr_xpath_ctx_t state;
    sr_val_t *ol = nullptr;
    sr_val_t *ne = nullptr;
    sr_change_oper_t oper;
    bool create, remove, modify;
    int rc;

    SRP_LOG_INF("In %s", __FUNCTION__);

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    if (event != SR_EV_VERIFY)
        return SR_ERR_OK;

    if (sr_get_changes_iter(ds, (char *)xpath, &it) != SR_ERR_OK) {
        sr_free_change_iter(it);
        return SR_ERR_OK;
    }

    foreach_change (ds, it, oper, ol, ne) {

        intf_name = sr_xpath_key_value(ne->xpath, "interface", "name", &state);
        if (intf_name.empty()) {
            sr_set_error(ds, "XPATH interface name NOT found", ne->xpath);
            return SR_ERR_INVAL_ARG;
        }
        sr_xpath_recover(&state);

        switch (oper) {
            case SR_OP_CREATED:
                if (sr_xpath_node_name_eq(ne->xpath, "name")) {
                    builder.set_name(ne->data.string_val);
                    create = true;
                } else if(sr_xpath_node_name_eq(ne->xpath, "type")) {
                    builder.set_type(ne->data.string_val);
                } else if(sr_xpath_node_name_eq(ne->xpath, "enabled")) {
                    builder.set_state(ne->data.bool_val);
                }
                break;

            case SR_OP_MODIFIED:
                if (sr_xpath_node_name_eq(ne->xpath, "enabled")) {
                    string n = sr_xpath_key_value(ne->xpath, "interface",
                                                  "name", &state);
                    intf = interface::find(n);
                    if (!intf) {
                        rc = SR_ERR_OPERATION_FAILED;
                        goto nothing_todo;
                    }
                    intf->set(admin_state_t::from_int(ne->data.bool_val));
                    modify = true;
                }
                break;

            case SR_OP_DELETED:
                if (sr_xpath_node_name_eq(ol->xpath, "name")) {
                    builder.set_name(ol->data.string_val);
                    remove = true;
                }
                break;

            default:
                rc = SR_ERR_UNSUPPORTED;
                goto nothing_todo;
        }

        sr_xpath_recover(&state);

        sr_free_val(ol);
        sr_free_val(ne);
    }

    if (create) {
        SRP_LOG_INF("creating interface '%s'", builder.name().c_str());
        intf = builder.build();
        if (nullptr == intf) {
            SRP_LOG_ERR_MSG("Interface does not exist");
            rc = SR_ERR_INVAL_ARG;
            goto nothing_todo;
        }
    }

    if (create || modify) {
        if ( OM::write(intf->key(), *intf) != rc_t::OK ) {
            SRP_LOG_ERR("Fail writing changes to VPP for: %s",
                        builder.to_string().c_str());
            rc = SR_ERR_OPERATION_FAILED;
            goto nothing_todo;
        }
    } else if (remove) {
        SRP_LOG_INF("deleting interface '%s'", builder.name().c_str());
        OM::remove(builder.name());
    }

    sr_free_change_iter(it);
    return SR_ERR_OK;

nothing_todo:
    sr_free_val(ol);
    sr_free_val(ne);
    sr_free_change_iter(it);
    return rc;
}

//XPATH : /openconfig-interfaces:interfaces/interface/state
static int
oc_interfaces_state_cb(const char *xpath, sr_val_t **values, size_t *values_cnt,
                       uint64_t request_id, const char *original_xpath,
                       void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    vapi_payload_sw_interface_details reply;
    shared_ptr<interface_dump> dump;
    string intf_name;
    sr_val_t *vals = nullptr;
    sr_xpath_ctx_t state;
    char xpath_root[XPATH_SIZE];
    int vc = 7;
    int cnt = 0;
    int rc;

    SRP_LOG_INF("In %s", __FUNCTION__);

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    intf_name = sr_xpath_key_value((char*) xpath, "interface", "name", &state);
    if (intf_name.empty()) {
        SRP_LOG_ERR_MSG("XPATH interface name not found");
        return SR_ERR_INVAL_ARG;
    }
    sr_xpath_recover(&state);

    snprintf(xpath_root, XPATH_SIZE,
             "/openconfig-interfaces:interfaces/interface[name='%s']/state",
             intf_name.c_str());

    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc)
        return rc;

    dump = make_shared<interface_dump>(intf_name); //dump only specific intf
    HW::enqueue(dump);
    HW::write();

    reply = dump->begin()->get_payload();

    sr_val_build_xpath(&vals[cnt], "%s/name", xpath_root);
    sr_val_set_str_data(&vals[cnt], SR_STRING_T, (char *)reply.interface_name.buf);
    cnt++;

    //TODO revisit types after V3PO has been implemented
    sr_val_build_xpath(&vals[cnt], "%s/type", xpath_root);
    sr_val_set_str_data(&vals[cnt], SR_IDENTITYREF_T, "ianaift:ethernetCsmacd");
    cnt++;

    sr_val_build_xpath(&vals[cnt], "%s/mtu", xpath_root);
    vals[cnt].type = SR_UINT16_T;
    vals[cnt].data.uint16_val = reply.link_mtu;
    cnt++;

    sr_val_build_xpath(&vals[cnt], "%s/enabled", xpath_root);
    vals[cnt].type = SR_BOOL_T;
    vals[cnt].data.bool_val = reply.flags;
    cnt++;

    sr_val_build_xpath(&vals[cnt], "%s/ifindex", xpath_root);
    vals[cnt].type = SR_UINT32_T;
    vals[cnt].data.uint32_val = reply.sw_if_index;
    cnt++;

    sr_val_build_xpath(&vals[cnt], "%s/admin-status", xpath_root);
    sr_val_set_str_data(&vals[cnt], SR_ENUM_T,
                        reply.flags ? "UP" : "DOWN");
    cnt++;

    sr_val_build_xpath(&vals[cnt], "%s/oper-status", xpath_root);
    sr_val_set_str_data(&vals[cnt], SR_ENUM_T,
                        reply.link_duplex ? "UP" : "DOWN");
    cnt++;

    *values = vals;
    *values_cnt = cnt;

    return SR_ERR_OK;
}

int
openconfig_interface_init(sc_plugin_main_t *pm)
{
    int rc = SR_ERR_OK;
    SRP_LOG_DBG_MSG("Initializing openconfig-interfaces plugin.");

    rc = sr_subtree_change_subscribe(pm->session, "/openconfig-interfaces:interfaces/interface/config",
            oc_interfaces_config_cb, nullptr, 98, SR_SUBSCR_CTX_REUSE, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    rc = sr_dp_get_items_subscribe(pm->session, "/openconfig-interfaces:interfaces/interface/state",
            oc_interfaces_state_cb, nullptr, SR_SUBSCR_CTX_REUSE, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    SRP_LOG_DBG_MSG("openconfig-interfaces plugin initialized successfully.");
    return SR_ERR_OK;

error:
    SRP_LOG_ERR("Error by initialization of openconfig-interfaces plugin. Error : %d", rc);
    return rc;
}

void
openconfig_interface_exit(__attribute__((unused)) sc_plugin_main_t *pm)
{
}

SC_INIT_FUNCTION(openconfig_interface_init);
SC_EXIT_FUNCTION(openconfig_interface_exit);
