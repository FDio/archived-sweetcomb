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

#include "openconfig_plugin.h"
#include "sys_util.h"
#include "openconfig_interfaces.h"
#include "openconfig_local_routing.h"
#include "sc_vpp_comm.h"

#include <assert.h>
#include <string.h>
#include <sysrepo/xpath.h>
#include <sysrepo/values.h>

#define XPATH_SIZE 2000

static struct _sys_repo_call sysrepo_callback[] = {
    {
        .xpath = "openconfig-interfaces",
        .method = MODULE,
        .datastore = RUNNING,
        .cb.mcb  = openconfig_interface_mod_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY
    },
    {
        .xpath = "openconfig-local-routing",
        .method = MODULE,
        .datastore = RUNNING,
        .cb.mcb  = openconfig_local_routing_mod_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/config",
        .method = XPATH,
        .datastore = RUNNING,
        .cb.scb = openconfig_interfaces_interfaces_interface_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_DEFAULT
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/state",
        .method = GETITEM,
        .datastore = RUNNING,
        .cb.gcb = openconfig_interfaces_interfaces_interface_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/state",
        .method = GETITEM,
        .datastore = RUNNING,
        .cb.gcb = openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address/openconfig-if-ip:config",
        .method = XPATH,
        .datastore = RUNNING,
        .cb.scb = openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_DEFAULT
    },
    {
        .xpath = "/openconfig-interfaces:interfaces/interface/subinterfaces/subinterface/openconfig-if-ip:ipv4/openconfig-if-ip:addresses/openconfig-if-ip:address/openconfig-if-ip:state",
        .method = GETITEM,
        .datastore = RUNNING,
        .cb.gcb = openconfig_interfaces_interfaces_interface_subinterfaces_subinterface_oc_ip_ipv4_oc_ip_addresses_oc_ip_address_oc_ip_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/config",
        .method = XPATH,
        .datastore = RUNNING,
        .cb.scb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_DEFAULT
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/interface-ref/config",
        .method = XPATH,
        .datastore = RUNNING,
        .cb.scb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_config_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_DEFAULT
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/state",
        .method = GETITEM,
        .datastore = RUNNING,
        .cb.gcb = openconfig_local_routing_local_routes_static_routes_static_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/state",
        .method = GETITEM,
        .datastore = RUNNING,
        .cb.gcb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
    {
        .xpath = "/openconfig-local-routing:local-routes/static-routes/static/next-hops/next-hop/interface-ref/state",
        .method = GETITEM,
        .datastore = RUNNING,
        .cb.gcb = openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_state_cb,
        .private_ctx = NULL,
        .priority = 0,
        .opts = SR_SUBSCR_CTX_REUSE
    },
};

static inline void
append(plugin_subcscription_t **list, plugin_subcscription_t *new)
{
	plugin_subcscription_t **tmp = list;

	while (*tmp)
		tmp = &(*tmp)->next;

	*tmp = new;
}

static int sys_repo_subscribe(plugin_main_t *plugin_main, sr_session_ctx_t *ds,
                              struct _sys_repo_call *call)
{
    plugin_subcscription_t *end, *new;
    int rc;

	new = calloc(sizeof(plugin_subcscription_t), 1);
    if (new == NULL)
        return SR_ERR_NOMEM;
    new->datastore = call->datastore;

    if (call->method == MODULE) {
		rc = sr_module_change_subscribe(ds, call->xpath, call->cb.mcb,
										call->private_ctx, call->priority,
										call->opts,
										&(new->sr_subscription_ctx));
		if (SR_ERR_OK != rc)
			goto error;
    } else if (call->method == XPATH) {
		rc = sr_subtree_change_subscribe(ds, call->xpath, call->cb.scb,
            							 call->private_ctx, call->priority,
                                         call->opts,
                                         &(new->sr_subscription_ctx));
		if (SR_ERR_OK != rc)
			goto error;
	} else if (call->method == GETITEM) {
		rc = sr_dp_get_items_subscribe(ds, call->xpath, call->cb.gcb,
                                       call->private_ctx, call->opts,
                                       &(new->sr_subscription_ctx));
        if (SR_ERR_OK != rc)
			goto error;
	} else if (call->method == RPC) {
		rc = sr_rpc_subscribe(ds, call->xpath, call->cb.rcb,
							  call->private_ctx, call->opts,
							  &(new->sr_subscription_ctx));
		if (SR_ERR_OK != rc)
				goto error;
    }

    SRP_LOG_DBG("Subscribed to xpath: %s", call->xpath);

	//add new subscription to the end of plugin subscription Linked List
	append(&(plugin_main->plugin_subcscription), new);

    return SR_ERR_OK;

error:
	SRP_LOG_ERR("Error subscribed to RPC: %s", call->xpath);
	return rc;
}

int openconfig_register_subscribe(plugin_main_t* plugin_main)
{
    int rc = 0;
    uint32_t i = 0;
    sr_session_ctx_t *datastore = NULL;

    ARG_CHECK(-1, plugin_main);

    for (i = 0; i < ARRAY_SIZE(sysrepo_callback); i++) {
        if (STARTUP == sysrepo_callback[i].datastore) {
            datastore = plugin_main->ds_startup;
        } else if (RUNNING == sysrepo_callback[i].datastore) {
            datastore = plugin_main->ds_running;
        } else {
            SRP_LOG_ERR_MSG("Error: wrong database type");
            return -1;
        }

        rc = sys_repo_subscribe(plugin_main, datastore, &sysrepo_callback[i]);
        if (SR_ERR_OK != rc) {
            SRP_LOG_ERR("Error: subscribing to subtree change, xpath: %s",
                  sysrepo_callback[i].xpath);
//             return -1;
        }
    }

    return 0;
}

void openconfig_unsubscribe(plugin_main_t* plugin_main)
{
    plugin_subcscription_t *plugin_subcscription, *tmp;

    if (plugin_main->plugin_subcscription == NULL)
	    return;

    plugin_subcscription = plugin_main->plugin_subcscription;
    do {
        tmp = plugin_subcscription;
        plugin_subcscription = plugin_subcscription->next;

        if (tmp->datastore == STARTUP)
			sr_unsubscribe(plugin_main->ds_startup, tmp->sr_subscription_ctx);
		else if (tmp->datastore == RUNNING)
			sr_unsubscribe(plugin_main->ds_running, tmp->sr_subscription_ctx);

        free(tmp);
    } while (plugin_subcscription != NULL);
}

plugin_main_t plugin_main;

int openconfig_plugin_init(sr_session_ctx_t* session)
{
    memset((void*) &plugin_main, 0, sizeof(plugin_main));

    plugin_main.ds_running = session;

    openconfig_register_subscribe(&plugin_main);
}

void openconfig_plugin_cleanup()
{
    openconfig_unsubscribe(&plugin_main);
}
