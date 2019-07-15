/*
 * Copyright (c) 2019 PANTHEON.tech.
 * Copyright (c) 2019 Cisco and/or its affiliates.
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

/*
 * Instances:
 * ==========
 * IETF NAT defines NAT instances, it is like databases which carry NAT
 * configurations.
 * VPP offers a single NAT database, so users should use instance with id=0.
 * Thus, enabling/disabling a NAT instance is not supported.
 *
 * NAT instance capabilities:
 * ==========================
 * We currently support:
 *  -"static-mapping"
 *
 * NAT instance policies:
 * ======================
 *
 * VOM NAT
 * ========
 * -Support NAT44/NAT66 static mapping (IP by IP) without transport protocol (NAT_IS_ADDR_ONLY)
 * -Support declaration of an interface as an INSIDE/OUTSIDE interface.
 *
 * TODO ideas of new features which can be supported
 * -Support for internal/external port in VOM and sweetcomb
 * -Support for dynamic NAT in VOM and sweetcomb
 */

#include <string>
#include <exception>
#include <memory>

#include <vom/om.hpp>
#include <vom/nat_static.hpp>

#include "sc_plugins.h"
#include "sys_util.h"

using VOM::OM;
using VOM::rc_t;

/* Just to intercept request trying to create nat instances */
static int
nat_instance_config_cb(sr_session_ctx_t *ds, const char *xpath,
                       sr_notif_event_t event, void *private_ctx)
{
    UNUSED(ds); UNUSED(xpath); UNUSED(event); UNUSED(private_ctx);

    SRP_LOG_INF("In %s", __FUNCTION__);

    return SR_ERR_OK;
}

typedef std::pair<boost::asio::ip::address, boost::asio::ip::address> nat_pair_t;
typedef std::map<uint32_t, nat_pair_t> nat_table_t;

static nat_table_t static_mapping_table;

/*
 * Valid pairs for static NAT are:
 * (internal_src address, external_src address)
 * (inernal_dest address, external_dest address)
 * Addresses must be of the same family i.e. no IP4 with IP6.
 *
 * Invalid pairs are:
 * (internal_*, internal_*) , (external_*, external_*)
 * (*_src, *_dest), (*_dest, *_src);
 */
class nat_static_builder {
public:
    /* Default constructor */
    nat_static_builder() {}

    shared_ptr<VOM::nat_static> build() {

        if (m_type != "static") //only static is supported for now
            return nullptr;

        if (! m_internal_src.empty() && ! m_external_src.empty() ) {
            m_inside = m_internal_src.address();
            m_outside = m_external_src.address();
        } else if (! m_internal_dest.empty() && ! m_external_dest.empty() ) {
            m_inside = m_internal_dest.address();
            m_outside = m_external_dest.address();
        } else
            return nullptr;

        return make_shared<VOM::nat_static>(m_inside, m_outside);
    }

    /* Getters */
    boost::asio::ip::address inside ()
    { return m_inside; }
    boost::asio::ip::address outside()
    { return m_outside; }

    /* Setters */
    nat_static_builder& set_type(std::string t)
    {
        m_type = t;
        return *this;
    }
    nat_static_builder& set_internal_src(std::string p)
    {
        m_internal_src = utils::prefix::make_prefix(p);
        return *this;
    }
    nat_static_builder& set_external_src(std::string p)
    {
        m_external_src = utils::prefix::make_prefix(p);
        return *this;
    }
    nat_static_builder& set_internal_dest(std::string p)
    {
        m_internal_dest = utils::prefix::make_prefix(p);
        return *this;
    }
    nat_static_builder& set_external_dest(std::string p)
    {
        m_external_dest = utils::prefix::make_prefix(p);
        return *this;
    }

private:
    std::string m_type;
    utils::prefix m_internal_src;
    utils::prefix m_external_src;
    utils::prefix m_internal_dest;
    utils::prefix m_external_dest;
    boost::asio::ip::address m_inside;
    boost::asio::ip::address m_outside;
};

/*
 * /ietf-nat:nat/instances/instance[id='%s']/mapping-table/mapping-entry[index='%s']/
 */
static int
nat_mapping_table_config_cb(sr_session_ctx_t *ds, const char *xpath,
                            sr_notif_event_t event, void *private_ctx)
{
    UNUSED(private_ctx);
    nat_static_builder builder;
    std::shared_ptr<VOM::nat_static> ns = nullptr;
    sr_val_t *ol = nullptr;
    sr_val_t *ne = nullptr;
    sr_change_iter_t *it;
    sr_change_oper_t oper;
    uint32_t xindex; // mapping entry index from xpath
    bool create, remove;
    int rc;

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    if (event != SR_EV_VERIFY)
        return SR_ERR_OK;

    SRP_LOG_INF("In %s", __FUNCTION__);

    rc = sr_get_changes_iter(ds, (char *)xpath, &it);
    if (rc != SR_ERR_OK)
        goto error;

    foreach_change(ds, it, oper, ol, ne) {

        switch (oper) {
        case SR_OP_CREATED:
            if (sr_xpath_node_name_eq(ne->xpath, "index")) {
                xindex = ne->data.uint32_val;
                create = true;
            } else if (sr_xpath_node_name_eq(ne->xpath, "type")) {
                /* For configuration only "static" can be supported */
                builder.set_type(string(ne->data.string_val));
            } else if (sr_xpath_node_name_eq(ne->xpath, "internal-src-address")) {
                /* source IP on NAT internal network src address */
                builder.set_internal_src(string(ne->data.string_val));
            } else if (sr_xpath_node_name_eq(ne->xpath, "external-src-address")) {
                /* source IP on NAT external network src address */
                builder.set_external_src(string(ne->data.string_val));
            } else if (sr_xpath_node_name_eq(ne->xpath, "internal-dst-address")) {
                /* destination IP on NAT internal network src address */
                builder.set_internal_dest(string(ne->data.string_val));
            } else if (sr_xpath_node_name_eq(ne->xpath, "external-dst-address")) {
                /* destination IP on NAT internal network src address */
                builder.set_external_dest(string(ne->data.string_val));
            }
            break;

        case SR_OP_DELETED:
            if (sr_xpath_node_name_eq(ol->xpath, "index")) {
                xindex = ol->data.uint32_val;
                remove = true;
            }
            break;

        default:
            SRP_LOG_WRN_MSG("Operation not supported");
            rc = SR_ERR_UNSUPPORTED;
            goto error;
        }

        sr_free_val(ne);
        sr_free_val(ol);
    }

    sr_free_change_iter(it);

    if (create) {
        ns = builder.build();
        if (ns == nullptr) {
            SRP_LOG_ERR("Fail building nat %s %s",
                        builder.inside().to_string().c_str(),
                        builder.outside().to_string().c_str());
            rc = SR_ERR_INVAL_ARG;
            goto error;
        }
    }

    if (create) {
        #define KEY(xindex) to_string(xindex)
        if (OM::write(KEY(xindex), *ns) != rc_t::OK) {
            SRP_LOG_ERR("Fail writing changes for nat: %s",
                        ns->to_string().c_str());
            rc = SR_ERR_OPERATION_FAILED;
            goto error;
        }
        static_mapping_table[xindex] =  nat_pair_t(builder.inside(),
                                                   builder.outside());
        SRP_LOG_INF_MSG("Sucess creating nat static entry");
    } else if (remove) {
        OM::remove(KEY(xindex));
        static_mapping_table.erase(xindex);
        #undef KEY
    }

    return SR_ERR_OK;

error:
    sr_free_val(ol);
    sr_free_val(ne);
    sr_free_change_iter(it);
    return rc;
}

/*
 * /ietf-nat:nat/instances/instance/capabilities
 */
static int
nat_cap_state_cb(const char *xpath, sr_val_t **values, size_t *values_cnt,
                 uint64_t request_id, const char *original_xpath,
                 void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    sr_val_t *val = nullptr;
    int vc = 1; //expected number of answer
    int cnt = 0; //value counter
    int rc;

    SRP_LOG_INF("In %s", __FUNCTION__);

    rc = sr_new_values(vc, &val);
    if (0 != rc) {
        rc = SR_ERR_NOMEM;
        goto nothing_todo;
    }

    sr_val_build_xpath(&val[cnt], "%s/static-mapping", xpath);
    val[cnt].type = SR_BOOL_T;
    val[cnt].data.bool_val = true;
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
ietf_nat_init(sc_plugin_main_t *pm)
{
    int rc = SR_ERR_OK;
    SRP_LOG_DBG_MSG("Initializing ietf-nat plugin.");

    rc = sr_subtree_change_subscribe(pm->session, "/ietf-nat:nat/instances/instance",
            nat_instance_config_cb, NULL, 1, SR_SUBSCR_CTX_REUSE, &pm->subscription);
    if (rc != SR_ERR_OK) {
        goto error;
    }

    rc = sr_subtree_change_subscribe(pm->session, "/ietf-nat:nat/instances/instance/mapping-table/mapping-entry",
            nat_mapping_table_config_cb, NULL, 10, SR_SUBSCR_CTX_REUSE, &pm->subscription);
    if (rc != SR_ERR_OK) {
        goto error;
    }

    rc = sr_dp_get_items_subscribe(pm->session, "/ietf-nat:nat/instances/instance/capabilities",
                                   nat_cap_state_cb, NULL, SR_SUBSCR_CTX_REUSE, &pm->subscription);
    if (SR_ERR_OK != rc) {
        goto error;
    }

    SRP_LOG_DBG_MSG("ietf-nat plugin initialized successfully.");
    return SR_ERR_OK;

error:
    SRP_LOG_ERR("Error by initialization of ietf-nat plugin. Error : %d", rc);
    return rc;
}

void
ietf_nat_exit(__attribute__((unused)) sc_plugin_main_t *pm)
{
}

SC_INIT_FUNCTION(ietf_nat_init);
SC_EXIT_FUNCTION(ietf_nat_exit);
