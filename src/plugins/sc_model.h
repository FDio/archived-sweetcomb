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

#ifndef __SC_MODEL_H__
#define __SC_MODEL_H__

/*
 * IETF or Openconfig YANG models registering/unregistering
 *
 * Every model supports a certain number of operations which require a
 * subscription to sysrepo : xpath_t
 */

#include <sysrepo.h>
#include <sysrepo/xpath.h>
#include <sysrepo/values.h>
#include <sysrepo/plugins.h>

#include "sc_vpp_comm.h" //for ARG_CHECK only

typedef enum {
    MODULE,
    XPATH,
    GETITEM,
    RPC,
} method_e;

#define ARRAY_SIZE(X) (sizeof(X) / sizeof(X[0]))

typedef struct _xpath_t {
    char *xpath;
    method_e method;
    sr_datastore_t datastore;
    union {
        sr_module_change_cb mcb;
        sr_subtree_change_cb scb;
        sr_dp_get_items_cb gcb;
        sr_rpc_cb rcb;
    } cb;
    void *private_ctx;
    uint32_t priority;
    sr_subscr_options_t opts;
} xpath_t;

/*
 * Stateful informations. Keep sesssions informations about a user
 */

typedef struct {
    sr_session_ctx_t *session;
    sr_subscription_ctx_t *subscription;
} plugin_main_t;


int
model_register(plugin_main_t *plugin_main, const xpath_t *xpaths, size_t size);

#endif /* __SC_MODEL_H__ */
