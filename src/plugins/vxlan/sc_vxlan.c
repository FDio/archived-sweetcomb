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

#include "sc_vxlan.h"

DEFINE_VAPI_MSG_IDS_VXLAN_API_JSON;

//do register and cleanup
static const char * const SC_PLUGIN_NAME="sc_vxlan";

SC_DEFINE_SR_PLUGIN_CLEANUP_CB_IMPL(SC_PLUGIN_NAME);
int
sr_plugin_init_cb (sr_session_ctx_t *session, void **private_ctx)
{
	SC_INVOKE_BEGIN;
	if (-1 == sc_connect_vpp())
	{
		SC_LOG_ERR("scvpp connect error , with return %d.", SR_ERR_INTERNAL);
		return SR_ERR_INTERNAL;
	}

	sr_subscription_ctx_t *subscription = NULL;
	int rc = SR_ERR_OK;
	//IP
	SC_SETUP_RPC_EVT_HANDLER(sc_vxlan_subscribe_tunnel_events);

	SC_LOG_DBG("load plugin [%s] ok",SC_PLUGIN_NAME);

	/* set subscription as our private context */
	*private_ctx = subscription;
	SC_INVOKE_END;
	return SR_ERR_OK;
}

