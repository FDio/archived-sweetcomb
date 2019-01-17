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

#ifndef SC_INTERFACE_H
#define SC_INTERFACE_H

#include "scvpp.h"

#include <vapi/interface.api.vapi.h>

int sc_interface_set_flags_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription);
int sc_interface_add_del_address_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription);

#endif /* SC_INTERFACE_H */

