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

#define HOP_INDEX_SIZE 10 //number of digit in max 32 bit integer

static inline int
_set_route (const char *prefix, const char *nhop, bool is_add,
            const char *iface)
{
    int table = 0; //FIB table ID
    int mask;
    int rc;

    // Put prefix length in mask and prefix IP in prefix
    mask = ip_prefix_split(prefix);
    if (mask < 1) {
        SRP_LOG_ERR("Prefix length can not be %d", mask);
        return SR_ERR_INVAL_ARG;
    }

    rc = ipv46_config_add_del_route(prefix, mask, nhop, is_add, table, iface);
    if (rc != SCVPP_OK) {
        SRP_LOG_ERR("Add/delete route failed for %s %s", prefix, iface);
        return SR_ERR_INVAL_ARG;
    }

    SRP_LOG_INF("Add/delete route Success for %s %s", prefix, iface);

    return SR_ERR_OK;
}

#define STR(string) #string

#define ROOT "/openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/%s"

static inline char * _get_ds_elem(sr_session_ctx_t *sess, const char *prefix,
                                  const char *index, const char *sub)
{
    char xpath[XPATH_SIZE] = {0};
    sr_val_t *value = NULL;
    int rc;

    snprintf(xpath, XPATH_SIZE, ROOT, prefix, index, sub);

    rc = sr_get_item(sess, xpath, &value);
    if (SR_ERR_OK != rc) {
        SRP_LOG_DBG("XPATH %s not set", xpath);
        return NULL;
    }

    return value->data.string_val;
}

/* @brief add/del route to FIB table 0.
 * If you add/delete an entry to FIB, prefix is mandatory and next hop can be:
 *   - interface only
 *   - next hop IP
 *   - both interface and next hop IP
 */
static int set_route(sr_session_ctx_t *sess, const char *index,
                     const char *interface /* NULLABLE*/,
                     const char *next_hop /* NULLABLE*/,
                     const char *prefix, bool is_add)
{
    const char *iface = NULL;
    const char *nhop = NULL;

    ARG_CHECK3(SR_ERR_INVAL_ARG, sess, index, prefix);

    if (!interface && !next_hop)
        return SR_ERR_INVAL_ARG;

    if (!interface) //fetch interface in datastore, can return NULL
        iface = _get_ds_elem(sess, prefix, index, "interface-ref/config/interface");
    else //use interface provided
        iface = interface;

    if (!next_hop) //fetch next-hop in datastore , can return NULL
        nhop = _get_ds_elem(sess, prefix, index, "config/next-hop");
    else //use next hop IP provided
        nhop = next_hop;

    return _set_route(prefix, nhop, is_add, iface);
}

/* @brief Callback used to add prefix entry for XPATH:
 * /openconfig-local-routing:local-routes/static-routes/static/config
 */
static int
oc_prefix_config_cb(sr_session_ctx_t *ds, const char *xpath,
                      sr_notif_event_t event, void *private_ctx)
{
    UNUSED(ds); UNUSED(xpath); UNUSED(event); UNUSED(private_ctx);

    SRP_LOG_INF("In %s", __FUNCTION__);

    return SR_ERR_OK;
}

// XPATH: /openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/config/
static int oc_next_hop_config_cb(sr_session_ctx_t *ds, const char *xpath,
                                 sr_notif_event_t event, void *private_ctx)
{
    char prefix[VPP_IP4_PREFIX_STRING_LEN] = {0};
    char next_hop[VPP_IP4_ADDRESS_STRING_LEN] = {0}; //IP of next router
    char index[HOP_INDEX_SIZE]; //next hop index
    bool index_set = false, next_hop_set = false;
    sr_change_iter_t *it;
    sr_change_oper_t oper;
    sr_val_t *old, *new, *tmp;
    sr_xpath_ctx_t state = {0};
    int rc = SR_ERR_OK;
    UNUSED(private_ctx);

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    SRP_LOG_INF("In %s", __FUNCTION__);

    if (event == SR_EV_APPLY) //SR_EV_VERIFY already applied the changes
        return SR_ERR_OK;

    rc = sr_get_changes_iter(ds, (char *)xpath, &it);
    if (rc != SR_ERR_OK) {
        sr_free_change_iter(it);
        return rc;
    }

    foreach_change(ds, it, oper, old, new) {

        rc = get_xpath_key(prefix, new->xpath, "static", "prefix",
                           VPP_IP4_PREFIX_STRING_LEN, &state);
        if (rc != 0)
            goto error;

        switch (oper) {
            case SR_OP_MODIFIED:
            case SR_OP_CREATED:
                tmp = new;
                break;
            case SR_OP_DELETED:
                tmp = old;
                break;
            default:
                SRP_LOG_WRN_MSG("Operation not supported");
                break;
        }

        if (sr_xpath_node_name_eq(tmp->xpath, "config")) {
            SRP_LOG_DBG("xpath: %s", tmp->xpath);
        } else if (sr_xpath_node_name_eq(tmp->xpath, "index")) {
            strncpy(index, tmp->data.string_val, HOP_INDEX_SIZE);
            index_set = true;
        } else if(sr_xpath_node_name_eq(tmp->xpath, "next-hop")) {
            strncpy(next_hop, tmp->data.string_val, VPP_IP4_ADDRESS_STRING_LEN);
            next_hop_set = true;
        } else if(sr_xpath_node_name_eq(tmp->xpath, "recurse")) {
            //SYSREPO BUG: Just catch it, sysrepo thinks it is mandatory...
        } else { //metric, recurse
            SRP_LOG_ERR("Unsupported field %s", tmp->xpath);
            return SR_ERR_UNSUPPORTED;
        }

        if (index_set && next_hop_set) {
            if (oper == SR_OP_CREATED) {
                rc = set_route(ds, index, NULL, next_hop, prefix, true);
            } else if (oper == SR_OP_MODIFIED) {
                rc = set_route(ds, index, NULL, next_hop, prefix, false);
                rc |= set_route(ds, index, NULL, next_hop, prefix, true);
            } else if (oper == SR_OP_DELETED) {
                rc = set_route(ds, index, NULL, next_hop, prefix, false);
            }

            if (rc != 0) {
                SRP_LOG_ERR_MSG("setting route failed");
                goto error;
            }
            index_set = false; next_hop_set = false;
        }

        sr_free_val(old);
        sr_free_val(new);
        sr_xpath_recover(&state);
    }

    sr_free_change_iter(it);
    return rc;

error:
    sr_free_val(old);
    sr_free_val(new);
    sr_xpath_recover(&state);
    sr_free_change_iter(it);
    return rc;
}

// XPATH: /openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/interface-ref/config/
static int
oc_next_hop_interface_config_cb(sr_session_ctx_t *ds, const char *xpath,
                                sr_notif_event_t event, void *private_ctx)
{
    char prefix[VPP_IP4_PREFIX_STRING_LEN] = {0};
    char interface[VPP_INTFC_NAME_LEN] = {0};
    char index[HOP_INDEX_SIZE] = {0};
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_val_t *old, *new, *tmp;
    sr_xpath_ctx_t state = {0};
    bool interface_set = false;
    int rc = SR_ERR_OK;
    UNUSED(private_ctx);

    SRP_LOG_INF("In %s", __FUNCTION__);

    if (event == SR_EV_APPLY)
        return SR_ERR_OK;

    rc = sr_get_changes_iter(ds, (char *)xpath, &it);
    if (rc != SR_ERR_OK) {
        sr_free_change_iter(it);
        return rc;
    }

    foreach_change(ds, it, oper, old, new) {

        SRP_LOG_DBG("xpath: %s", new->xpath);

        rc = get_xpath_key(prefix, new->xpath, "static", "prefix",
                           VPP_IP4_PREFIX_STRING_LEN, &state);

        rc |= get_xpath_key(index, new->xpath, "next-hop", "index",
                           HOP_INDEX_SIZE, &state);
        if (rc)
            goto error;

        switch (oper) {
            case SR_OP_MODIFIED:
            case SR_OP_CREATED:
                tmp = new;
                break;
            case SR_OP_DELETED:
                tmp = old;
                break;
            default:
                SRP_LOG_WRN_MSG("Operation not supported");
                break;
        }

        if (sr_xpath_node_name_eq(tmp->xpath, "config")) {
            SRP_LOG_DBG("xpath: %s", tmp->xpath);
        } else if (sr_xpath_node_name_eq(tmp->xpath, "interface")) {
            strncpy(interface, tmp->data.string_val, VPP_INTFC_NAME_LEN);
            interface_set = true;
        } else { //metric, recurse
            SRP_LOG_ERR("Unsupported field %s", tmp->xpath);
            return SR_ERR_UNSUPPORTED;
        }

        if (oper == SR_OP_CREATED && interface_set) {
            rc = set_route(ds, index, interface, NULL, prefix, true);
            interface_set = false;
        } else if (oper == SR_OP_MODIFIED && interface_set) {
            rc = set_route(ds, index, interface, NULL, prefix, false);
            rc |= set_route(ds, index, interface, NULL, prefix, true);
            interface_set = false;
        } else if (oper == SR_OP_DELETED && interface_set) {
            rc = set_route(ds, index, interface, NULL, prefix, false);
            interface_set = false;
        }

        if (rc != 0) {
            SRP_LOG_ERR_MSG("setting route failed");
            goto error;
        }

        sr_free_val(old);
        sr_free_val(new);
        sr_xpath_recover(&state);
    }

    sr_free_change_iter(it);
    return SR_ERR_OK;

error:
    sr_free_val(old);
    sr_free_val(new);
    sr_xpath_recover(&state);
    sr_free_change_iter(it);
    return rc;
}

// XPATH: /openconfig-local-routing:local-routes/static-routes/static/state
static int oc_prefix_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    char prefix[VPP_IP4_PREFIX_STRING_LEN] = {0};
    sr_xpath_ctx_t state = {0};
    sr_val_t *vals = NULL;
    fib_dump_t *reply = NULL;
    int vc = 1;
    int rc = 0;

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    SRP_LOG_INF("In %s", __FUNCTION__);

    rc = get_xpath_key(prefix, (char *)xpath, "static", "prefix",
                       VPP_IP4_PREFIX_STRING_LEN, &state);
    if (rc != 0)
        return SR_ERR_INVAL_ARG;

    rc = ipv4_fib_dump_prefix(prefix, &reply);
    if (rc == -SCVPP_NOT_FOUND) {
        SRP_LOG_ERR("Prefix %s not found in VPP FIB", prefix);
        return SR_ERR_NOT_FOUND;
    }

    /* Allocation for number of elements dumped by ipv4_fib_dump_all */
    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc)
        return rc;

    sr_val_build_xpath(&vals[0], "%s/prefix", xpath);
    sr_val_set_str_data(&vals[0], SR_STRING_T, prefix);

    sr_xpath_recover(&state);

    free(reply);
    *values = vals;
    *values_cnt = vc;

    return SR_ERR_OK;
}

// XPATH /openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/state
static int
oc_next_hop_state_cb(const char *xpath, sr_val_t **values, size_t *values_cnt,
                     uint64_t request_id, const char *original_xpath,
                     void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    char prefix[VPP_IP4_PREFIX_STRING_LEN] = {0};
    char index[HOP_INDEX_SIZE];
    char next_hop[INET_ADDRSTRLEN] = {0};
    fib_dump_t *reply = NULL;
    sr_xpath_ctx_t state = {0};
    sr_val_t *vals = NULL;
    int vc = 3;
    int rc = 0;

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    SRP_LOG_INF("In %s", __FUNCTION__);

    /* Get prefix and index key from XPATH */
    rc = get_xpath_key(prefix, (char *)xpath, "static", "prefix",
                       VPP_IP4_PREFIX_STRING_LEN, &state);

    rc |= get_xpath_key(index, (char*)xpath, "next-hop", "index",
                       VPP_IP4_PREFIX_STRING_LEN, &state);

    if (rc != 0)
        return SR_ERR_INVAL_ARG;

    rc = ipv4_fib_dump_prefix(prefix, &reply);
    if (rc != SCVPP_OK) {
        SRP_LOG_ERR("Fail to dump prefix %s", prefix);
        return SR_ERR_INVAL_ARG;
    }

    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc)
        return rc;

    // check next hop index equals next hop id
    if (strtoul(index, NULL, 0) != reply->path[0].next_hop_id) {
        SRP_LOG_ERR("next hop index is %d for prefix %s",
                     reply->path[0].next_hop_id, prefix);
        SRP_LOG_ERR("before %s, stroul is %d", index,  strtoul(index, NULL, 0));
        return SR_ERR_INVAL_ARG;
    }

    sr_val_build_xpath(&vals[0], "%s/index", xpath);
    sr_val_set_str_data(&vals[0], SR_STRING_T, index);

    strncpy(next_hop, sc_ntoa(reply->path[0].next_hop), VPP_IP4_ADDRESS_LEN);
    sr_val_build_xpath(&vals[1], "%s/next-hop", xpath);
    sr_val_set_str_data(&vals[1], SR_STRING_T, (char *)reply->path[0].next_hop);

    sr_val_build_xpath(&vals[2], "%s/metric", xpath);
    vals[2].type = SR_UINT32_T;
    vals[2].data.uint32_val = reply->path[0].weight;

    free(reply);
    sr_xpath_recover(&state);
    *values = vals;
    *values_cnt = vc;

    return SR_ERR_OK;
}

// XPATH /openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/interface-ref/state
static int
oc_next_hop_interface_state_cb(const char *xpath, sr_val_t **values,
                               size_t *values_cnt, uint64_t request_id,
                               const char *original_xpath, void *private_ctx)
{
    UNUSED(request_id); UNUSED(original_xpath); UNUSED(private_ctx);
    sr_xpath_ctx_t state = {0};
    char prefix[VPP_IP4_PREFIX_STRING_LEN] = {0};
    char interface[VPP_INTFC_NAME_LEN] = {0};
    char index[HOP_INDEX_SIZE]; //next hop index
    fib_dump_t *reply = NULL;
    sr_val_t *vals = NULL;
    int vc = 1;
    int rc = 0;

    //ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    SRP_LOG_INF("In %s", __FUNCTION__);

    ///* Get prefix key from XPATH */
    rc = get_xpath_key(prefix, (char*) xpath, "static", "prefix",
                       VPP_IP4_PREFIX_STRING_LEN, &state);

    rc |= get_xpath_key(index, (char *)xpath, "next-hop", "index",
                        HOP_INDEX_SIZE, &state);

    if (rc != 0)
        return SR_ERR_INVAL_ARG;

    rc = ipv4_fib_dump_prefix(prefix, &reply);
    if (rc != SCVPP_OK) {
        SRP_LOG_ERR("Fail to dump prefix %s", prefix);
        return SR_ERR_INVAL_ARG;
    }
    //TODO: check nhop?

    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc)
        return rc;

    // check next hop index equals next hop id
    if (strtoul(index, NULL, 0) != reply->path[0].next_hop_id) {
        SRP_LOG_ERR("next hop index is %d for prefix %s",
                     reply->path[0].next_hop_id, prefix);
        SRP_LOG_ERR("before %s, stroul is %d", index,  strtoul(index, NULL, 0));
        return SR_ERR_INVAL_ARG;
    }

    rc = get_interface_name(interface, reply->path[0].sw_if_index);
    if (rc != SCVPP_OK) {
        SRP_LOG_ERR("No interface name for id %d", reply->path[0].sw_if_index);
        return SR_ERR_INVAL_ARG;
    }

    sr_val_build_xpath(&vals[0], "%s/interface", xpath);
    sr_val_set_str_data(&vals[0], SR_STRING_T, interface);

    free(reply);
    sr_xpath_recover(&state);
    *values = vals;
    *values_cnt = vc;

    return SR_ERR_OK;
}

const xpath_t oc_local_routing_xpaths[OC_LROUTING_SIZE] = {
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/config",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = oc_prefix_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        //.opts = SR_SUBSCR_DEFAULT
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/config",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = oc_next_hop_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        //.opts = SR_SUBSCR_DEFAULT
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/interface-ref/config",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = oc_next_hop_interface_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        //.opts = SR_SUBSCR_DEFAULT
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = oc_prefix_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = oc_next_hop_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/interface-ref/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = oc_next_hop_interface_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
};
