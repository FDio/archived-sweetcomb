/*
 * Copyright (c) 2018 HUACHENTEL and/or its affiliates.
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
#include <vapi/vapi.h>
#include <vapi/vpe.api.vapi.h>
#include <vapi/interface.api.vapi.h>
#include <vapi/l2.api.vapi.h>
#include <vapi/ip.api.vapi.h>
#include <vapi/tapv2.api.vapi.h>
#include <vapi/ipsec.api.vapi.h>
#include <vapi/vxlan.api.vapi.h>
#include <vnet/interface.h>
#include <vnet/mpls/mpls_types.h>

// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_VPE_API_JSON;
DEFINE_VAPI_MSG_IDS_INTERFACE_API_JSON;
DEFINE_VAPI_MSG_IDS_L2_API_JSON;
DEFINE_VAPI_MSG_IDS_IP_API_JSON;
DEFINE_VAPI_MSG_IDS_TAPV2_API_JSON;
DEFINE_VAPI_MSG_IDS_IPSEC_API_JSON;
DEFINE_VAPI_MSG_IDS_VXLAN_API_JSON;

#include "sc_plugins.h"
#include "sc_model.h"

#include "ietf/ietf_interface.h"
#include "openconfig/openconfig_interfaces.h"
#include "openconfig/openconfig_local_routing.h"

int sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx)
{
    SC_INVOKE_BEGIN;
    plugin_main_t plugin_main;
    int rc;

    rc = sc_connect_vpp();
    if (0 != rc) {
        SC_LOG_ERR("vpp connect error , with return %d.", rc);
        return SR_ERR_INTERNAL;
    }

    plugin_main.session = session;
    plugin_main.subscription = NULL;

    /* Use the same sr_subscription_ctx for all models */
    model_register(&plugin_main, ietf_interfaces_xpaths, IETF_INTERFACES_SIZE);
    model_register(&plugin_main, oc_interfaces_xpaths, OC_INTERFACES_SIZE);
    model_register(&plugin_main, oc_local_routing_xpaths, OC_LROUTING_SIZE);

    /* set subscription as our private context */
    *private_ctx = plugin_main.subscription;
    SC_INVOKE_END;

    return SR_ERR_OK;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_ctx)
{
    SC_INVOKE_BEGIN;

    /* subscription was set as our private context */
    if (private_ctx != NULL)
        sr_unsubscribe(session, private_ctx);
    SC_LOG_DBG_MSG("unload plugin ok.");

    sc_disconnect_vpp();
    SC_LOG_DBG_MSG("plugin disconnect vpp ok.");
    SC_INVOKE_END;
}

