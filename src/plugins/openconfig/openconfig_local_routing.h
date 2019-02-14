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

#ifndef __OPENCONFIG_LOCAL_ROUTING_H__
#define __OPENCONFIG_LOCAL_ROUTING_H__

#include <sysrepo.h>

int openconfig_local_routing_mod_cb(sr_session_ctx_t *session,
                                    const char *module_name,
                                    sr_notif_event_t event,
                                    void *private_ctx);

int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    void *private_ctx);

int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_config_cb(
    sr_session_ctx_t *ds, const char *xpath, sr_notif_event_t event,
    void *private_ctx);

int openconfig_local_routing_local_routes_static_routes_static_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx);

int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx);

int openconfig_local_routing_local_routes_static_routes_static_next_hops_next_hop_interface_ref_state_cb(
    const char *xpath, sr_val_t **values, size_t *values_cnt,
    uint64_t request_id, const char *original_xpath, void *private_ctx);

#endif /* __OPENCONFIG_LOCAL_ROUTING_H__ */
