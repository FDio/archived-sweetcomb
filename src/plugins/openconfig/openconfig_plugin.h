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

#ifndef __SWEETCOMB_OPENCONFIG_PLUGIN__
#define __SWEETCOMB_OPENCONFIG_PLUGIN__

#include <stdlib.h>
#include <sysrepo.h>

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(*X))

typedef enum {
    MODULE,
    XPATH,
    GETITEM,
    RPC,
} method_e;

typedef enum {
    STARTUP,
    RUNNING,
} datastore_e;

typedef struct _plugin_subcscription_t{
    datastore_e datastore;
    sr_subscription_ctx_t *sr_subscription_ctx;
    struct _plugin_subcscription_t *next;
} plugin_subcscription_t;

typedef struct {
    sr_conn_ctx_t *sr_conn_ctx;
    sr_session_ctx_t *ds_startup;
    sr_session_ctx_t *ds_running;
    //sr_session_ctx_t *ds_candidate;
    plugin_subcscription_t *plugin_subcscription;
} plugin_main_t;

struct _sys_repo_call {
    char *xpath;
    method_e method;
    datastore_e datastore;
    union {
        sr_module_change_cb mcb;
        sr_subtree_change_cb scb;
        sr_dp_get_items_cb gcb;
        sr_rpc_cb rcb;
    } cb;
    void *private_ctx;
    uint32_t priority;
    sr_subscr_options_t opts;
};

int openconfig_register_subscribe(plugin_main_t *plugin_main);
void openconfig_unsubscribe(plugin_main_t *plugin_main);

//FIXME:
// This solution is not good and should be rewrite.
// But first we must discuss the architecture of sweetcomb and how we should
// register the new YANGS modules.
// This function here is only for test.
int openconfig_plugin_init(sr_session_ctx_t *session);
void openconfig_plugin_cleanup();


#endif /* __SWEETCOMB_OPENCONFIG_PLUGIN__ */
