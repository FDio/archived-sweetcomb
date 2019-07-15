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

#ifndef __SC_PLUGINS_H__
#define __SC_PLUGINS_H__

extern "C" {
    #include <sysrepo.h>
    #include <sysrepo/values.h>
    #include <sysrepo/plugins.h>
}

#include "sc_init.h"
#include "sys_util.h"

typedef struct sc_plugin_main_t {
    /* Session context acquired with ::sr_session_start call. */
    sr_session_ctx_t *session;

    /* Subscription context that is supposed to be released by ::sr_unsubscribe */
    sr_subscription_ctx_t *subscription;

    /* List of init/exit functions to call, setup by constructors */
    _sc_init_function_list_elt_t *init_function_registrations;
    _sc_exit_function_list_elt_t *exit_function_registrations;

} sc_plugin_main_t;

sc_plugin_main_t *sc_get_plugin_main();

//functions that sysrepo-plugin need
extern "C" int sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx);
extern "C" void sr_plugin_cleanup_cb(sr_session_ctx_t *session,
                                     void *private_ctx);
extern "C" int sr_plugin_health_check_cb(sr_session_ctx_t *session,
                                         void *private_ctx);

#endif //__SC_PLUGINS_H__
