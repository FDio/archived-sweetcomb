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

#ifndef __SC_INIT_H__
#define __SC_INIT_H__

#include <sysrepo.h>

struct sc_plugin_main_t;

/* Init/exit functions: called at start/end of sr plugin init/cleaup. */
typedef int (sc_init_function_t) (struct sc_plugin_main_t *pm);
typedef void (sc_exit_function_t) (struct sc_plugin_main_t *pm);

/* Declaration of init/exit Linked List structures */
#define foreach_sc_declare_function_list    \
_(init)                                     \
_(exit)

#define _(tag)                                  \
typedef struct _sc_##tag##_function_list_elt {  \
    struct _sc_##tag##_function_list_elt *next; \
    sc_##tag##_function_t *func;                \
    bool is_called;                             \
} _sc_##tag##_function_list_elt_t;
foreach_sc_declare_function_list
#undef _

/* Declaration of add/rm function that need to be add/remote link list */
#define SC_DECLARE_FUNCTION(f, tag)                                                 \
static void __sc_add_##tag##_function_##f() __attribute__((__constructor__));       \
static void __sc_add_##tag##_function_##f()                                         \
{                                                                                   \
    sc_plugin_main_t *pm = sc_get_plugin_main();                                    \
    static _sc_##tag##_function_list_elt_t _sc_##tag##_function;                    \
    _sc_##tag##_function.next = pm->tag##_function_registrations;                   \
    _sc_##tag##_function.func = &f;                                                 \
    _sc_##tag##_function.is_called = false;                                         \
    pm->tag##_function_registrations = &_sc_##tag##_function;                       \
}                                                                                   \
static void __sc_rm_##tag##_function_##f() __attribute__((__destructor__));         \
static void __sc_rm_##tag##_function_##f()                                          \
{                                                                                   \
    sc_plugin_main_t *pm = sc_get_plugin_main();                                    \
    _sc_##tag##_function_list_elt_t *p;                                             \
    if (pm->tag##_function_registrations->func == &f) {                             \
        pm->tag##_function_registrations = pm->tag##_function_registrations->next;  \
        return;                                                                     \
    }                                                                               \
    p = pm->tag##_function_registrations;                                           \
    while (p->next != NULL) {                                                       \
        if (p->next->func == &f) {                                                  \
            p->next = p->next->next;                                                \
            return;                                                                 \
        }                                                                           \
        p = p->next;                                                                \
    }                                                                               \
}

/* Macros to be called from yang model implementation.
   Register init and exit functions for YANG model implementation in Linked List. */
#define SC_INIT_FUNCTION(f) SC_DECLARE_FUNCTION(f, init)
#define SC_EXIT_FUNCTION(f) SC_DECLARE_FUNCTION(f, exit)

/* Call given init/exit function: used for init/exit function dependencies. */
#define sc_call_init_function(f, pm)                                    \
({                                                                      \
    int _ret = SR_ERR_OK;                                               \
    _sc_init_function_list_elt_t *p = pm->init_function_registrations;  \
    while (p != NULL) {                                                 \
        if (p->func == f && !p->is_called) {                            \
            p->is_called = true;                                        \
            _ret = p->func(pm);                                         \
            break;                                                      \
        }                                                               \
        p = p->next;                                                    \
    }                                                                   \
    _ret;                                                               \
})

#define sc_call_exit_function(f, pm)                                    \
({                                                                      \
    _sc_exit_function_list_elt_t *p = pm->exit_function_registrations;  \
    while (p != NULL) {                                                 \
        if (p->func == f && !p->is_called) {                            \
            p->is_called = true;                                        \
            p->func(pm);                                                \
            break;                                                      \
        }                                                               \
        p = p->next;                                                    \
    }                                                                   \
})

/* Run through all registered init/exit functions of models. */
int
sc_call_all_init_function(struct sc_plugin_main_t *pm);

void
sc_call_all_exit_function(struct sc_plugin_main_t *pm);

#endif //__SC_INIT_H__
