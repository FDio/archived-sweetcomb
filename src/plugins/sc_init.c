/*
 * Copyright (c) 2019 Tieto and/or its affiliates.
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

int
sc_call_all_init_function(sc_plugin_main_t *pm)
{
    int ret = SR_ERR_OK;
    _sc_init_function_list_elt_t *p = pm->init_function_registrations;
    while (p != NULL) {
        if (!p->is_called) {
            p->is_called = true;
            ret = p->func(pm);
            if (ret != SR_ERR_OK) {
                return ret;
            }
        }
        p = p->next;
    }
    return ret;
}

void
sc_call_all_exit_function(sc_plugin_main_t *pm)
{
    _sc_exit_function_list_elt_t *p = pm->exit_function_registrations;
    while (p != NULL) {
        if (!p->is_called) {
            p->is_called = true;
            p->func(pm);
        }
        p = p->next;
    }
}

