/*
 * Copyright (c) 2019 PANTHEON.tech.
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

#include "ietf_nat.h"
#include "sc_vpp_comm.h"
#include "sc_vpp_interface.h"
#include "sc_vpp_nat.h"

#include <assert.h>
#include <string.h>
#include <sysrepo.h>
#include <sysrepo/xpath.h>
#include <sysrepo/values.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../sys_util.h"

static int ietf_nat_mod_cb(
    __attribute__((unused)) sr_session_ctx_t *session,
    __attribute__((unused)) const char *module_name,
    __attribute__((unused)) sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    SRP_LOG_INF("Module subscribe: %s", module_name);

    return SR_ERR_OK;
}

/**
 * @brief Wrapper struct for VAPI address range payload.
 */
struct address_range_t {
    nat44_add_del_address_range_t payload; //< VAPI payload for address range.
    bool last_ip_address_set; //< Variable last_ip_address in payload is set.
};

// Check input structure, set VAPI payload and call VAPI function to set
// nat44 address range.
static sr_error_t set_address_range(struct address_range_t *address_r)
{
    sr_error_t rc = SR_ERR_OK;
//     char tmp_ip1[VPP_IP4_ADDRESS_STRING_LEN];
//     char tmp_ip2[VPP_IP4_ADDRESS_STRING_LEN];

    ARG_CHECK(SR_ERR_INVAL_ARG, address_r);

    //if end of IP range not provided, then range size = 1 with only first ip
    if (!address_r->last_ip_address_set) {
        memcpy(&address_r->payload.last_ip_address[0],
               &address_r->payload.first_ip_address[0],
               VPP_IP4_ADDRESS_LEN);
    }

    if (hardntohlu32(address_r->payload.last_ip_address) <
        hardntohlu32(address_r->payload.first_ip_address)) {
        SRP_LOG_ERR_MSG("End address less than start address");
        return SR_ERR_INVAL_ARG;
    }

//     strncpy(tmp_ip1, sc_ntoa(address_r->payload.first_ip_address),
//             VPP_IP4_ADDRESS_STRING_LEN);
//     strncpy(tmp_ip2, sc_ntoa(address_r->payload.last_ip_address),
//             VPP_IP4_ADDRESS_STRING_LEN);
//     SRP_LOG_DBG("Fist ip address: %s, last ip address: %s, twice_nat: %u,"
//                 "is_add: %u", tmp_ip1, tmp_ip2, address_r->payload.twice_nat,
//                 address_r->payload.is_add);

    int rv = nat44_add_del_addr_range(&address_r->payload);
    if (0 != rv) {
        SRP_LOG_ERR_MSG("Failed set address range.");
        rc = SR_ERR_OPERATION_FAILED;
    }

    return rc;
}

// parse leaf from this xpath:
// /ietf-nat:nat/instances/instance[id='%s']/policy[id='%s']/external-ip-address-pool[pool-id='%s']/
static int parse_instance_policy_external_ip_address_pool(
    const sr_val_t *val, struct address_range_t *address_range)
{
    int rc;
    char tmp_str[VPP_IP4_PREFIX_STRING_LEN] = {0};
    uint8_t prefix = 0;

    ARG_CHECK2(SR_ERR_INVAL_ARG, val, address_range);

    if (sr_xpath_node_name_eq(val->xpath, "pool-id")) {
        SRP_LOG_WRN("%s not supported.", val->xpath);
    } else if(sr_xpath_node_name_eq(val->xpath, "external-ip-pool")) {
        rc = get_address_from_prefix(tmp_str, val->data.string_val,
                                     VPP_IP4_ADDRESS_STRING_LEN, &prefix);
        if (0 != rc) {
            SRP_LOG_ERR_MSG("Error translate");
            return SR_ERR_INVAL_ARG;
        }

        rc = sc_aton(tmp_str, address_range->payload.first_ip_address,
                     VPP_IP4_ADDRESS_LEN);
        if (0 != rc) {
            SRP_LOG_ERR_MSG("Failed convert string IP address to int.");
            return SR_ERR_INVAL_ARG;
        }

        if (prefix < VPP_IP4_HOST_PREFIX_LEN) {
            // External IP pool is represented as IPv4 prefix in YANG module.
            // VPP support range from first IPv4 address to last IPv4 address, not prefix.
            // In this concept the broadcast IPv4 address of this IPv4 prefix,
            // represent last IPv4 address for VPP
            get_last_ip_address(
                (sc_ipv4_addr*) &address_range->payload.last_ip_address[0],
                (sc_ipv4_addr*) &address_range->payload.first_ip_address[0],
                prefix);
            address_range->last_ip_address_set = true;
        }
    }

    return SR_ERR_OK;
}

// XPATH: /ietf-nat:nat/instances/instance[id='%s']/policy[id='%s']/external-ip-address-pool[pool-id='%s']/
static int instances_instance_policy_external_ip_address_pool_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    sr_error_t rc = SR_ERR_OK;
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    bool create = false;
    bool delete = false;
    struct address_range_t new_address_r = {0};
    struct address_range_t old_address_r = {0};

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    new_address_r.payload.vrf_id = ~0;
    old_address_r.payload.vrf_id = ~0;

    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

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

        SRP_LOG_DBG("A change detected in '%s', op=%d",
                    new_val ? new_val->xpath : old_val->xpath, oper);

        switch (oper) {
            case SR_OP_CREATED:
                create = true;
                rc = parse_instance_policy_external_ip_address_pool(new_val,
                                                                &new_address_r);
                break;

            case SR_OP_MODIFIED:
                delete = true;
                rc = parse_instance_policy_external_ip_address_pool(old_val,
                                                                &old_address_r);
                if (SR_ERR_OK != rc) {
                    break;
                }

                create = true;
                rc = parse_instance_policy_external_ip_address_pool(new_val,
                                                                &new_address_r);
                break;

            case SR_OP_MOVED:
                break;

            case SR_OP_DELETED:
                delete = true;
                rc = parse_instance_policy_external_ip_address_pool(old_val,
                                                                &old_address_r);
                break;
        }

        sr_free_val(old_val);
        sr_free_val(new_val);

        if (SR_ERR_OK != rc) {
            rc = SR_ERR_OPERATION_FAILED;
            goto error;
        }
    }

    if (delete) {
        old_address_r.payload.is_add = 0;
        rc = set_address_range(&old_address_r);
        if (SR_ERR_OK != rc) {
            goto error;
        }
    }

    if (create) {
        new_address_r.payload.is_add = 1;
        rc = set_address_range(&new_address_r);
        if (SR_ERR_OK != rc) {
            goto error;
        }
    }

error:
    sr_free_change_iter(it);
    return rc;
}

enum mapping_type {
    STATIC,
    DYNAMIC_IMPLICIT,
    DYNAMIC_EXPLICIT,
    UNKNOWN,
};

/**
 * @brief Wrapper struct for VAPI static mapping payload.
 */
struct static_mapping_t {
    enum mapping_type mtype; //< Mapping type
    bool local_ip_address_set; //< Variable are set in payload.
    bool external_ip_address_set; //< Variable are set in payload.
    bool protocol_set; //< Variable are set in payload.
    bool local_port_set; //< Variable are set in payload.
    bool external_port_set; //< Variable are set in payload.
    bool external_sw_if_index_set; //< Variable are set in payload.
    bool vrf_id_set; //< Variable are set in payload.
    bool twice_nat_set; //< Variable are set in payload.
    nat44_add_del_static_mapping_t payload; //< VAPI payload for static mapping
};

// Check input structure, set VAPI payload and call VAPI function to set
// nat44 static mapping.
static sr_error_t set_static_mapping(struct static_mapping_t *mapping)
{
    int rc = 0;
//     char tmp_ip1[VPP_IP4_ADDRESS_STRING_LEN];
//     char tmp_ip2[VPP_IP4_ADDRESS_STRING_LEN];

    ARG_CHECK(SR_ERR_INVAL_ARG, mapping);

    if ((mapping->local_port_set != mapping->external_port_set) ||
        !mapping->local_ip_address_set || !mapping->external_ip_address_set) {
        SRP_LOG_ERR_MSG("NAT44 parameter missing.");

        return SR_ERR_VALIDATION_FAILED;
    }

    if (!mapping->local_port_set && !mapping->external_port_set) {
        mapping->payload.addr_only= 1;
        mapping->payload.twice_nat = 0;
    } else {
        mapping->payload.addr_only= 0;
        mapping->payload.twice_nat = 1;
        if (!mapping->protocol_set) {

            SRP_LOG_ERR_MSG("NAT44 protocol missing.");
            return SR_ERR_VALIDATION_FAILED;
        }
    }

    mapping->payload.external_sw_if_index = ~0;

//     strncpy(tmp_ip1, sc_ntoa(mapping->payload.local_ip_address),
//             VPP_IP4_ADDRESS_STRING_LEN);
//     strncpy(tmp_ip2, sc_ntoa(mapping->payload.external_ip_address),
//             VPP_IP4_ADDRESS_STRING_LEN);
//     SRP_LOG_DBG("Local ip address: %s, external ip address: %s, addr_only: %u,"
//                 " protocol: %u, local port: %u, external port: %u, twice_nat: %u,"
//                 "is_add: %u", tmp_ip1, tmp_ip2, mapping->payload.addr_only,
//                 mapping->payload.protocol, mapping->payload.local_port,
//                 mapping->payload.external_port,  mapping->payload.twice_nat,
//                 mapping->payload.is_add);

    rc = nat44_add_del_static_mapping(&mapping->payload);
    if (0 != rc) {
        SRP_LOG_ERR_MSG("Failed set static mapping");
        return SR_ERR_OPERATION_FAILED;
    }

    return SR_ERR_OK;
}

// parse leaf from this xpath:
// /ietf-nat:nat/instances/instance[id='%s']/mapping-table/mapping-entry[index='%s']/
static int parse_instance_mapping_table_mapping_entry(
    const sr_val_t *val,
    struct static_mapping_t *mapping)
{
    int rc;
    char tmp_str[VPP_IP4_PREFIX_STRING_LEN] = {0};

    ARG_CHECK2(SR_ERR_INVAL_ARG, val, mapping);

    sr_xpath_ctx_t state = {0};

    if(sr_xpath_node_name_eq(val->xpath, "type")) {
        if (!strncmp("static", val->data.string_val, strlen("static"))) {
            mapping->mtype = STATIC;
        } else if (!strncmp("dynamic-implicit", val->data.string_val,
            strlen("dynamic-implicit"))) {
            mapping->mtype = DYNAMIC_IMPLICIT;
        } else if (!strncmp("dynamic-explicit", val->data.string_val,
            strlen("dynamic-explicit"))) {
                mapping->mtype = DYNAMIC_EXPLICIT;
        }
    } else if(sr_xpath_node_name_eq(val->xpath, "transport-protocol")) {
        if (SR_UINT8_T != val->type) {
            SRP_LOG_ERR("Wrong transport-protocol, type, current type: %d.",
                        val->type);
            return SR_ERR_INVAL_ARG;
        }

        mapping->payload.protocol = val->data.uint8_val;
        mapping->protocol_set = true;
    } else if(sr_xpath_node_name_eq(val->xpath, "internal-src-address")) {
        if (SR_STRING_T != val->type) {
            SRP_LOG_ERR("Wrong internal-src-address, type, current type: %d.",
                        val->type);
            return SR_ERR_INVAL_ARG;
        }

        rc = get_address_from_prefix(tmp_str, val->data.string_val,
                                     VPP_IP4_PREFIX_STRING_LEN, NULL);
        if (0 != rc) {
            SRP_LOG_ERR_MSG("Error translate");
            return SR_ERR_INVAL_ARG;
        }

        rc = sc_aton(tmp_str, mapping->payload.local_ip_address,
                     VPP_IP4_ADDRESS_LEN);
        if (0 != rc) {
            SRP_LOG_ERR_MSG("Failed convert string IP address to int.");
            return SR_ERR_INVAL_ARG;
        }

        mapping->local_ip_address_set = true;
    } else if(sr_xpath_node_name_eq(val->xpath, "external-src-address")) {
        if (SR_STRING_T != val->type) {
            SRP_LOG_ERR("Wrong external-src-address, type, current type: %d.",
                        val->type);
            return SR_ERR_INVAL_ARG;
        }

        rc = get_address_from_prefix(tmp_str, val->data.string_val,
                                     VPP_IP4_ADDRESS_STRING_LEN, NULL);
        if (0 != rc) {
            SRP_LOG_ERR_MSG("Error translate");
            return SR_ERR_INVAL_ARG;
        }

        rc = sc_aton(tmp_str, mapping->payload.external_ip_address,
                     VPP_IP4_ADDRESS_LEN);
        if (0 != rc) {
            SRP_LOG_ERR_MSG("Failed convert string IP address to int.");
            return SR_ERR_INVAL_ARG;
        }

        mapping->external_ip_address_set = true;
    } else if (sr_xpath_node(val->xpath, "internal-src-port", &state)) {
        sr_xpath_recover(&state);
        if(sr_xpath_node_name_eq(val->xpath, "start-port-number")) {
            mapping->local_port_set = true;
            mapping->payload.local_port = val->data.uint16_val;
        }
    } else if (sr_xpath_node(val->xpath, "external-src-port", &state)) {
        sr_xpath_recover(&state);
        if(sr_xpath_node_name_eq(val->xpath, "start-port-number")) {
            mapping->external_port_set = true;
            mapping->payload.external_port = val->data.uint16_val;
        }
    }

    return SR_ERR_OK;
}

// XPATH: /ietf-nat:nat/instances/instance[id='%s']/mapping-table/mapping-entry[index='%s']/
static int instances_instance_mapping_table_mapping_entry_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    __attribute__((unused)) void *private_ctx)
{
    sr_error_t rc = SR_ERR_OK;
    sr_change_iter_t *it = NULL;
    sr_change_oper_t oper;
    sr_val_t *old_val = NULL;
    sr_val_t *new_val = NULL;
    bool create = false;
    bool delete = false;
    struct static_mapping_t new_mapping = {0};
    struct static_mapping_t old_mapping = {0};

    ARG_CHECK2(SR_ERR_INVAL_ARG, ds, xpath);

    new_mapping.mtype = UNKNOWN;
    old_mapping.mtype = UNKNOWN;

    SRP_LOG_DBG("'%s' modified, event=%d", xpath, event);

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

        SRP_LOG_DBG("A change detected in '%s', op=%d",
                    new_val ? new_val->xpath : old_val->xpath, oper);

        switch (oper) {
            case SR_OP_CREATED:
                create = true;
                rc = parse_instance_mapping_table_mapping_entry(new_val,
                                                                &new_mapping);
                break;

            case SR_OP_MODIFIED:
                delete = true;
                rc = parse_instance_mapping_table_mapping_entry(old_val,
                                                                &old_mapping);
                if (SR_ERR_OK != rc) {
                    break;
                }

                create = true;
                rc = parse_instance_mapping_table_mapping_entry(new_val,
                                                                &new_mapping);
               break;

            case SR_OP_MOVED:
                break;

            case SR_OP_DELETED:
                delete = true;
                rc = parse_instance_mapping_table_mapping_entry(old_val,
                                                                &old_mapping);
                break;
        }

        sr_free_val(old_val);
        sr_free_val(new_val);

        if (SR_ERR_OK != rc) {
            rc = SR_ERR_OPERATION_FAILED;
            goto error;
        }
    }

    if (delete) {
        old_mapping.payload.is_add = 0;
        if (STATIC == old_mapping.mtype) {
            rc = set_static_mapping(&old_mapping);
            if (SR_ERR_OK != rc) {
                goto error;
            }
        }
    }

    if (create) {
        new_mapping.payload.is_add = 1;
        if (STATIC == new_mapping.mtype) {
            rc = set_static_mapping(&new_mapping);
            if (SR_ERR_OK != rc) {
                goto error;
            }
        }
    }

error:
    sr_free_change_iter(it);
    return rc;
}

const xpath_t ietf_nat_xpaths[IETF_NAT_SIZE] = {
    {
        .xpath = "ietf-nat",
        .method = MODULE,
        .datastore = SR_DS_RUNNING,
        .cb.mcb  = ietf_nat_mod_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/ietf-nat:nat/instances/instance/policy/external-ip-address-pool",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = instances_instance_policy_external_ip_address_pool_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/ietf-nat:nat/instances/instance/mapping-table/mapping-entry",
        .method = XPATH,
        .datastore = SR_DS_RUNNING,
        .cb.scb = instances_instance_mapping_table_mapping_entry_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
};
