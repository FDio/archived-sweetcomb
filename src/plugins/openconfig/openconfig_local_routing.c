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

#include "openconfig_local_routing.h"

#include "../sys_util.h"

#include "sc_vpp_comm.h"
#include "sc_vpp_interface.h"
#include "sc_vpp_ip.h"

#define XPATH_SIZE 2000

int openconfig_local_routing_mod_cb(
    __attribute__((unused)) sr_session_ctx_t* session,
    __attribute__((unused)) const char* module_name,
    __attribute__((unused)) sr_notif_event_t event,
    __attribute__((unused)) void* private_ctx)
{
    SRP_LOG_DBG("Local routing module subscribe: %s", module_name);

    return SR_ERR_OK;
}

static int set_route(sr_session_ctx_t *sess, const char *index,
                     const char *n_interface /*NULLABLE*/,
                     const char *n_next_hop /*NULLABLE*/,
                     const char *prefix, bool is_add)
{
    int rc = SR_ERR_OK;
    sr_val_t *value = NULL;
    char xpath[XPATH_SIZE] = {0};
    const char *interface = NULL;
    const char *next_hop = NULL;

    ARG_CHECK3(SR_ERR_INVAL_ARG, sess, index, prefix);

    if (NULL == n_interface) {
        snprintf(xpath, XPATH_SIZE,
        "/openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/interface-ref/config/interface", prefix, index);

        rc = sr_get_item(sess, xpath, &value);
        if (SR_ERR_OK != rc) {
            SRP_LOG_DBG_MSG("Interface not set");
            return 0;
        }

        interface = value->data.string_val;
    } else {
        interface = n_interface;
    }

    if (NULL == n_next_hop) {
        snprintf(xpath, XPATH_SIZE,
        "/openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/config/next-hop", prefix, index);

        rc = sr_get_item(sess, xpath, &value);
        if (SR_ERR_OK != rc) {
            SRP_LOG_DBG_MSG("Next-hop not set");
            return 0;
        }

        next_hop = value->data.string_val;
    } else {
        next_hop = n_next_hop;
    }

    int mask = ip_prefix_split(prefix);
    if (mask < 1) {
        return SR_ERR_INVAL_ARG;
    }

    rc = ipv46_config_add_del_route(prefix, mask, next_hop, is_add, 0,
                                    interface);
    if (!rc) {
        return SR_ERR_INVAL_ARG;
    }

    return SR_ERR_OK;
}

// XPATH: /openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/config/
int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    int rc = SR_ERR_OK;
    char *tmp = NULL;
    char static_prefix[XPATH_SIZE] = {0};
    char next_hop_index[XPATH_SIZE] = {0};
    char next_hop[XPATH_SIZE] = {0};
    char old_next_hop_index[XPATH_SIZE] = {0};
    char old_next_hop[XPATH_SIZE] = {0};
    bool index_set = false, next_hop_set = false;
    bool old_index_set = false, old_next_hop_set = false;

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

    while (sr_get_change_next(ds, it, &oper,
        &old_val, &new_val) == SR_ERR_OK) {

        log_recv_oper(oper, "subtree_change_cb received");

        SRP_LOG_DBG("xpath: %s", new_val->xpath);

        sr_xpath_ctx_t state = {0};

        tmp = xpath_find_first_key(new_val->xpath, "prefix", &state);
        if (NULL == tmp) {
            SRP_LOG_DBG_MSG("static_prefix NOT found.");
            continue;
        }

        strncpy(static_prefix, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        switch (oper) {
            case SR_OP_CREATED:
                if (sr_xpath_node_name_eq(new_val->xpath, "index")) {
                    strncpy(next_hop_index, new_val->data.string_val,
                            XPATH_SIZE);
                    index_set = true;
                } else if(sr_xpath_node_name_eq(new_val->xpath, "next-hop")) {
                    next_hop_set = true;
                    strncpy(next_hop, new_val->data.string_val, XPATH_SIZE);
                } else if(sr_xpath_node_name_eq(new_val->xpath, "metric")) {
                    //TODO: LEAF: metric, type: uint32
                } else if(sr_xpath_node_name_eq(new_val->xpath, "recurse")) {
                    //TODO: LEAF: recurse, type: boolean
                }

                if (index_set && next_hop_set) {
                    rc = set_route(ds, next_hop_index,  NULL, next_hop,
                                   static_prefix, true);
                }
            break;

            case SR_OP_MODIFIED:
                if (sr_xpath_node_name_eq(old_val->xpath, "index")) {
                    strncpy(old_next_hop_index, old_val->data.string_val,
                            XPATH_SIZE);
                    old_index_set = true;
                } else if(sr_xpath_node_name_eq(old_val->xpath, "next-hop")) {
                    old_next_hop_set = true;
                    strncpy(old_next_hop, old_val->data.string_val, XPATH_SIZE);
                } else if(sr_xpath_node_name_eq(old_val->xpath, "metric")) {
                    //TODO: LEAF: metric, type: uint32
                } else if(sr_xpath_node_name_eq(old_val->xpath, "recurse")) {
                    //TODO: LEAF: recurse, type: boolean
                }

                if (sr_xpath_node_name_eq(new_val->xpath, "index")) {
                    strncpy(next_hop_index, new_val->data.string_val,
                            XPATH_SIZE);
                    index_set = true;
                } else if(sr_xpath_node_name_eq(new_val->xpath, "next-hop")) {
                    next_hop_set = true;
                    strncpy(next_hop, new_val->data.string_val, XPATH_SIZE);
                } else if(sr_xpath_node_name_eq(new_val->xpath, "metric")) {
                    //TODO: LEAF: metric, type: uint32
                } else if(sr_xpath_node_name_eq(new_val->xpath, "recurse")) {
                    //TODO: LEAF: recurse, type: boolean
                }

                if (old_index_set && old_next_hop_set) {
                    rc = set_route(ds, old_next_hop_index,  NULL, old_next_hop,
                                   static_prefix, false);
                }

                if (index_set && next_hop_set) {
                    rc = set_route(ds, next_hop_index,  NULL, next_hop,
                                  static_prefix, true);
                }
                break;

            case SR_OP_MOVED:
                break;

            case SR_OP_DELETED:
                if (sr_xpath_node_name_eq(old_val->xpath, "index")) {
                    strncpy(old_next_hop_index, old_val->data.string_val,
                            XPATH_SIZE);
                    old_index_set = true;
                } else if(sr_xpath_node_name_eq(old_val->xpath, "next-hop")) {
                    old_next_hop_set = true;
                    strncpy(old_next_hop, old_val->data.string_val, XPATH_SIZE);
                } else if(sr_xpath_node_name_eq(old_val->xpath, "metric")) {
                    //TODO: LEAF: metric, type: uint32
                } else if(sr_xpath_node_name_eq(old_val->xpath, "recurse")) {
                    //TODO: LEAF: recurse, type: boolean
                }

                if (old_index_set && old_next_hop_set) {
                    rc = set_route(ds, old_next_hop_index,  NULL, old_next_hop,
                                   static_prefix, false);
                }
                break;
        }

        if (SR_ERR_OK != rc) {
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

// XPATH: /openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/interface-ref/config/
int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    char *tmp = NULL;
    char static_prefix[XPATH_SIZE] = {0};
    char next_hop_index[XPATH_SIZE] = {0};
    char interface[XPATH_SIZE] = {0};
    char old_interface[XPATH_SIZE] = {0};
    int rc = SR_ERR_OK;

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

    while (sr_get_change_next(ds, it, &oper,
        &old_val, &new_val) == SR_ERR_OK) {

        log_recv_oper(oper, "subtree_change_cb received");

        SRP_LOG_DBG("xpath: %s", new_val->xpath);

        sr_xpath_ctx_t state = {0};

        tmp = xpath_find_first_key(new_val->xpath, "prefix", &state);
        if (NULL == tmp) {
            SRP_LOG_DBG_MSG("static_prefix NOT found.");
            continue;
        }

        strncpy(static_prefix, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        tmp = xpath_find_first_key(new_val->xpath, "index", &state);
        if (NULL == tmp) {
            SRP_LOG_DBG_MSG("next-hop_index NOT found.");
            continue;
        }

        strncpy(next_hop_index, tmp, XPATH_SIZE);
        sr_xpath_recover(&state);

        switch (oper) {
            case SR_OP_CREATED:
                if (sr_xpath_node_name_eq(new_val->xpath, "interface")) {
                    strncpy(interface, new_val->data.string_val, XPATH_SIZE);
                    rc = set_route(ds, next_hop_index, interface, NULL,
                                   static_prefix, true);
                } else if (sr_xpath_node_name_eq(new_val->xpath,
                    "subinterface")) {
                }
                break;

            case SR_OP_MODIFIED:
                if (sr_xpath_node_name_eq(old_val->xpath, "interface")) {
                    strncpy(old_interface, old_val->data.string_val,
                            XPATH_SIZE);
                    rc = set_route(ds, next_hop_index, old_interface, NULL,
                                   static_prefix, false);
                } else if (sr_xpath_node_name_eq(old_val->xpath,
                    "subinterface")) {
                }

                if (sr_xpath_node_name_eq(new_val->xpath, "interface")) {
                    strncpy(interface, new_val->data.string_val, XPATH_SIZE);
                    rc = set_route(ds, next_hop_index, interface, NULL,
                                   static_prefix, true);
                } else if (sr_xpath_node_name_eq(new_val->xpath,
                    "subinterface")) {
                }
                break;

            case SR_OP_MOVED:
                break;

            case SR_OP_DELETED:
                if (sr_xpath_node_name_eq(old_val->xpath, "interface")) {
                    strncpy(old_interface, old_val->data.string_val,
                            XPATH_SIZE);
                    rc = set_route(ds, next_hop_index, old_interface, NULL,
                                   static_prefix, false);
                } else if (sr_xpath_node_name_eq(old_val->xpath,
                    "subinterface")) {
                }
                break;
        }

        if (SR_ERR_OK != rc) {
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

typedef struct
{
    u8 length;
    u8 address[4];
} address_prefix_t;

typedef struct {
    char *next_hop_index;
    address_prefix_t address_prefix;
    const bool is_interface_ref;
    sw_interface_details_query_t sw_interface_details_query;
    sysr_values_ctx_t sysr_values_ctx;
} sysr_ip_fib_details_ctx_t;

static
bool address_prefix_cmp(address_prefix_t* address_prefix,
                        vapi_payload_ip_fib_details* reply)
{
    ARG_CHECK2(false, address_prefix, reply);

    if (address_prefix->length == reply->address_length)
    {
        if (0 == memcmp(address_prefix->address, reply->address, sizeof(reply->address)))
            return true;
    }

    return false;
}

static
bool address_prefix_init(address_prefix_t* address_prefix, char* str_prefix)
{
    ARG_CHECK2(false, address_prefix, str_prefix);

    address_prefix->length = ip_prefix_split(str_prefix);
    if(address_prefix->length < 1)
    {
        SRP_LOG_ERR_MSG("not a valid prefix");
        return false;
    }

    return sc_aton(str_prefix, address_prefix->address,
                     sizeof(address_prefix->address));
}

// XPATH: /openconfig-local-routing:local-routes/static-routes/static/state
int openconfig_local_routing_local_routes_static_routes_static_state_vapi_cb(
    vapi_payload_ip_fib_details *reply,
    sysr_ip_fib_details_ctx_t *sysr_ip_fib_details_ctx)
{
    sr_val_t *vals = NULL;
    int rc = 0;
    int vc = 0;
    char address_prefix[INET_ADDRSTRLEN + 3] = {0};
    vc = 1;

    ARG_CHECK2(SR_ERR_INVAL_ARG, reply, sysr_ip_fib_details_ctx);

    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    //Filling the structure
    snprintf(address_prefix, sizeof(address_prefix), "%s/%u",
             sc_ntoa(reply->address), reply->address_length);

    sr_val_build_xpath(&vals[0], "%s/prefix",
                       sysr_ip_fib_details_ctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[0], SR_STRING_T, address_prefix);

    sysr_ip_fib_details_ctx->sysr_values_ctx.values = vals;
    sysr_ip_fib_details_ctx->sysr_values_ctx.values_cnt = vc;

    return SR_ERR_OK;
}

vapi_error_e
ip_routing_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                vapi_error_e rv, bool is_last,
                vapi_payload_ip_fib_details * reply)
{
    if (is_last)
    {
        assert (NULL == reply);
    }
    else
    {
        sysr_ip_fib_details_ctx_t *dctx = callback_ctx;
        assert(dctx && reply);

        if (address_prefix_cmp(&dctx->address_prefix, reply))
        {
            openconfig_local_routing_local_routes_static_routes_static_state_vapi_cb(reply,
                                                                        dctx);
        }
    }

    return VAPI_OK;
}

int openconfig_local_routing_local_routes_static_routes_static_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    __attribute__((unused)) uint64_t request_id,
    __attribute__((unused)) const char *original_xpath,
    __attribute__((unused)) void *private_ctx)
{
    sr_xpath_ctx_t state = {0};
    vapi_error_e rv = 0;
    char *tmp = NULL;
    char static_prefix[XPATH_SIZE] = {0};
    sysr_ip_fib_details_ctx_t dctx = {0};

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    tmp = xpath_find_first_key(xpath, "prefix", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("static_prefix not found.");
        return SR_ERR_INVAL_ARG;
    }

    strncpy(static_prefix, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    //VPP callback
    snprintf(dctx.sysr_values_ctx.xpath_root, XPATH_SIZE, "/openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/state", static_prefix);

    if (!address_prefix_init(&dctx.address_prefix, static_prefix))
    {
        return SR_ERR_INVAL_ARG;
    }


    vapi_msg_ip_fib_dump *mp = vapi_alloc_ip_fib_dump (g_vapi_ctx_instance);

    VAPI_CALL(vapi_ip_fib_dump(g_vapi_ctx_instance, mp, ip_routing_dump_cb, &dctx));
    if(VAPI_OK != rv)
    {
        SRP_LOG_ERR_MSG("VAPI call failed");
        return SR_ERR_INVAL_ARG;
    }

    sr_xpath_recover(&state);
    *values = dctx.sysr_values_ctx.values;
    *values_cnt = dctx.sysr_values_ctx.values_cnt;

    return SR_ERR_OK;
}

// // XPATH: /openconfig-local-routing:local-routes/static-routes/l/next-hops/next-hop/state
int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_state_vapi_cb(
    vapi_type_fib_path *reply,
    sysr_ip_fib_details_ctx_t *sysr_ip_fib_details_ctx)
{
    sr_val_t *vals = NULL;
    int rc = 0;
    int vc = 0;
    char next_hop[INET_ADDRSTRLEN] = {0};

    ARG_CHECK2(SR_ERR_INVAL_ARG, reply, sysr_ip_fib_details_ctx);

    vc = 3;
    /* convenient functions such as this can be found in sysrepo/values.h */
    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    sr_val_build_xpath(&vals[0], "%s/index",
                       sysr_ip_fib_details_ctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[0], SR_STRING_T,
                        sysr_ip_fib_details_ctx->next_hop_index);

    strncpy(next_hop, sc_ntoa(reply->next_hop), sizeof(next_hop));
    sr_val_build_xpath(&vals[1], "%s/next-hop", sysr_ip_fib_details_ctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[1], SR_STRING_T, next_hop);

    sr_val_build_xpath(&vals[2], "%s/metric", sysr_ip_fib_details_ctx->sysr_values_ctx.xpath_root);
    vals[2].type = SR_UINT32_T;
    vals[2].data.uint32_val = reply->weight;

    // sr_val_build_xpath(&vals[3], "%s/recurse", sysr_ip_fib_details_ctx->sysr_values_ctx.xpath_root);
    // vals[3].type = SR_BOOL_T;
    // vals[3].data.bool_val = YANG INPUT TYPE: boolean;

    sysr_ip_fib_details_ctx->sysr_values_ctx.values = vals;
    sysr_ip_fib_details_ctx->sysr_values_ctx.values_cnt = vc;

    return SR_ERR_OK;
}

// // XPATH: /openconfig-local-routing:local-routes/static-routes/l/next-hops/next-hop/interface-ref/state
int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_state_vapi_cb(
    sysr_ip_fib_details_ctx_t * dctx)
{
    sr_val_t *vals = NULL;
    int rc = 0;
    int vc = 0;

    ARG_CHECK(SR_ERR_INVAL_ARG, dctx);

    vc = 2;
    /* convenient functions such as this can be found in sysrepo/values.h */
    rc = sr_new_values(vc, &vals);
    if (SR_ERR_OK != rc) {
        return rc;
    }

    sr_val_build_xpath(&vals[0], "%s/interface",
                       dctx->sysr_values_ctx.xpath_root);
    sr_val_set_str_data(&vals[0], SR_STRING_T,
        (const char*)dctx->sw_interface_details_query.sw_interface_details.interface_name);

    sr_val_build_xpath(&vals[1], "%s/subinterface",
                       dctx->sysr_values_ctx.xpath_root);
    vals[1].type = SR_UINT32_T;
    vals[1].data.uint32_val = 0;

    dctx->sysr_values_ctx.values = vals;
    dctx->sysr_values_ctx.values_cnt = vc;

    return SR_ERR_OK;
}

vapi_error_e
ip_routing_next_hop_dump_cb (struct vapi_ctx_s *ctx, void *callback_ctx,
                vapi_error_e rv, bool is_last,
                vapi_payload_ip_fib_details * reply)
{
    ARG_CHECK(VAPI_EINVAL, callback_ctx);

    sysr_ip_fib_details_ctx_t *dctx = callback_ctx;

    if (is_last)
    {
        assert (NULL == reply);
    }
    else
    {
        assert (NULL != reply);

        if (reply->count > 0 && address_prefix_cmp(&dctx->address_prefix, reply))
        {
            if (dctx->is_interface_ref)
            {
                dctx->sw_interface_details_query.interface_found = true;
                dctx->sw_interface_details_query.sw_interface_details.sw_if_index = reply->path[0].sw_if_index;
                //sw_interface_dump will have to be called outside this VAPI
            }
            else
            {
                openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_state_vapi_cb(reply->path, dctx);
            }
        }
    }

    return VAPI_OK;
}

int next_hop_inner(
    bool is_interface_ref, const char *xpath, sr_val_t **values,
    size_t *values_cnt, __attribute__((unused)) uint64_t request_id,
    __attribute__((unused)) void *private_ctx)
{
    sr_xpath_ctx_t state = {0};
    vapi_error_e rv;
    char *tmp = NULL;
    char static_prefix[XPATH_SIZE] = {0};
    char next_hop_index[XPATH_SIZE] = {0};
    sysr_ip_fib_details_ctx_t dctx = {.is_interface_ref = is_interface_ref};

    ARG_CHECK3(SR_ERR_INVAL_ARG, xpath, values, values_cnt);

    tmp = xpath_find_first_key(xpath, "prefix", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("static_prefix not found.");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(static_prefix, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    tmp = xpath_find_first_key(xpath, "index", &state);
    if (NULL == tmp) {
        SRP_LOG_ERR_MSG("index not found.");
        return SR_ERR_INVAL_ARG;
    }
    strncpy(next_hop_index, tmp, XPATH_SIZE);
    sr_xpath_recover(&state);

    //VPP callback
    dctx.next_hop_index = next_hop_index;
    if (is_interface_ref) {
        snprintf(dctx.sysr_values_ctx.xpath_root, XPATH_SIZE,
        "/openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/interface-ref/state",
        static_prefix, next_hop_index);
    } else {
        snprintf(dctx.sysr_values_ctx.xpath_root, XPATH_SIZE,
        "/openconfig-local-routing:local-routes/static-routes/static[prefix='%s']/next-hops/next-hop[index='%s']/state",
        static_prefix, next_hop_index);
    }

    if (!address_prefix_init(&dctx.address_prefix, static_prefix))
    {
        return SR_ERR_INVAL_ARG;
    }

    vapi_msg_ip_fib_dump *mp = vapi_alloc_ip_fib_dump (g_vapi_ctx_instance);
    VAPI_CALL(vapi_ip_fib_dump(g_vapi_ctx_instance, mp, ip_routing_next_hop_dump_cb, &dctx));
    if (VAPI_OK != rv)
    {
        SRP_LOG_ERR_MSG("VAPI call failed");
        return SR_ERR_INVAL_ARG;
    }

    if (is_interface_ref)
    {
        if (dctx.sw_interface_details_query.interface_found)
        {
            if (!get_interface_name(&dctx.sw_interface_details_query))
            {
                return SR_ERR_INVAL_ARG;
            }
            if (strlen((const char*)
                dctx.sw_interface_details_query.sw_interface_details.interface_name)) {
                openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_state_vapi_cb(&dctx);
            }
        }
    }

    sr_xpath_recover(&state);
    *values = dctx.sysr_values_ctx.values;
    *values_cnt = dctx.sysr_values_ctx.values_cnt;

    return SR_ERR_OK;
}

int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx)
{
    return next_hop_inner(false, xpath, values, values_cnt, request_id,
                          private_ctx);
}

int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx)
{
    return next_hop_inner(true, xpath, values, values_cnt, request_id,
                          private_ctx);
}

const xpath_t oc_local_routing_xpaths[OC_LROUTING_SIZE] = {
    {
        .xpath = "openconfig-local-routing",
        .method = MODULE,
        .datastore = SR_DS_RUNNING,
        .cb.mcb  = openconfig_local_routing_mod_cb,
        .private_ctx = NULL,
        .priority = 0,
        //.opts = SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY
        .opts = SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/config",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        //.opts = SR_SUBSCR_DEFAULT
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/interface-ref/config",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        //.opts = SR_SUBSCR_DEFAULT
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = openconfig_local_routing_local_routes_static_routes_static_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/interface-ref/state",
        .method = GETITEM,
        .datastore = SR_DS_RUNNING,
        .cb.gcb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
};
