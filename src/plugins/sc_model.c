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

#include "sc_model.h"

#include <sysrepo/plugins.h>

static int
subscribe(sr_session_ctx_t *ds, sr_subscription_ctx_t **sub,
          const xpath_t *xpath)
{
    int rc;

    switch (xpath->method) {
        case MODULE:
            rc = sr_module_change_subscribe(ds, xpath->xpath, xpath->cb.mcb,
                                            xpath->private_ctx, xpath->priority,
                                            xpath->opts, sub);
            break;

        case XPATH:
            rc = sr_subtree_change_subscribe(ds, xpath->xpath, xpath->cb.scb,
                                             xpath->private_ctx,
                                             xpath->priority, xpath->opts, sub);
            break;

        case GETITEM:
            rc = sr_dp_get_items_subscribe(ds, xpath->xpath, xpath->cb.gcb,
                                           xpath->private_ctx, xpath->opts,
                                           sub);
            break;

        case RPC:
            rc = sr_rpc_subscribe(ds, xpath->xpath, xpath->cb.rcb,
                    xpath->private_ctx, xpath->opts, sub);
            break;

        default:
            SRP_LOG_ERR("Unknown method %d", xpath->method);
            return SR_ERR_NOT_FOUND;
    }

    if (SR_ERR_OK != rc) {
        SRP_LOG_ERR("Error subscribing to %s", xpath->xpath);
        return rc;
    }

    SRP_LOG_INF("Subscribed to xpath: %s", xpath->xpath);

    return SR_ERR_OK;
}

int
model_register(plugin_main_t* plugin_main, const xpath_t *xpaths, size_t size)
{
    int rc = 0;
    uint32_t i = 0;

    ARG_CHECK(-1, plugin_main);

    for (i = 0; i < size; i++) {
        rc = subscribe(plugin_main->session, &plugin_main->subscription,
                       &xpaths[i]);
        if (SR_ERR_OK != rc)
            SRP_LOG_ERR("Subscription failed for xpath: %s", xpaths[i].xpath);
    }

    return 0;
}
