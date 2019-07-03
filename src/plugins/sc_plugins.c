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

#include "sc_plugins.h"

#include <dirent.h>

#include <vpp-api/client/stat_client.h>

sc_plugin_main_t sc_plugin_main;

sc_plugin_main_t *sc_get_plugin_main()
{
    return &sc_plugin_main;
}

int sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx)
{
    int rc = SR_ERR_OK;;

    sc_plugin_main.session = session;

    /* Connect to VAPI */
	if (!(sc_plugin_main.vpp_main = sc_connect_vpp())) {
		SRP_LOG_ERR("VPP connect error: %d", rc);
		return SR_ERR_INTERNAL;
	}

    /* Connect to STAT API */
    rc = stat_segment_connect(STAT_SEGMENT_SOCKET_FILE);
    if (rc != 0) {
        SRP_LOG_ERR("vpp stat connect error , with return %d.", rc);
        return SR_ERR_INTERNAL;
    }

    rc = sc_call_all_init_function(&sc_plugin_main);
    if (rc != SR_ERR_OK) {
        SRP_LOG_ERR("Call all init function error: %d", rc);
        return rc;
    }

    /* set subscription as our private context */
    *private_ctx = sc_plugin_main.subscription;

    return rc;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_ctx)
{
    sc_call_all_exit_function(&sc_plugin_main);

    /* subscription was set as our private context */
    if (private_ctx != NULL)
        sr_unsubscribe(session, private_ctx);
    SRP_LOG_DBG_MSG("unload plugin ok.");

    /* Disconnect from STAT API */
    stat_segment_disconnect();

    /* Disconnect from VAPI */
    sc_disconnect_vpp();
    SRP_LOG_DBG_MSG("plugin disconnect vpp ok.");
}

int sr_plugin_health_check_cb( __attribute__ ((unused)) sr_session_ctx_t *session,
	__attribute__ ((unused)) void *private_ctx)
{
	/* health check, will use shell to detect vpp when plugin is loaded */
	/* health_check will run every 10 seconds in loop*/
	pid_t pid = sc_get_vpp_pid();
	return sc_plugin_main.vpp_main->pid == pid && pid != 0 ? SR_ERR_OK : SR_ERR_INTERNAL;
}
