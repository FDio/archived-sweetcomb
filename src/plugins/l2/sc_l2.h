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

#ifndef _SC_L2_H_
#define _SC_L2_H_

#include <vapi/l2.api.vapi.h>
#include "scvpp.h"

i32 sc_bridge_domain_add_del(u32 bd_id, u8 flood, u8 uu_flood, u8 forward, u8 learn, u8 arp_term, u8 mac_age, u8 bd_tag[VPP_TAG_LEN], u8 is_add);
i32 sc_interface_set_l2_bridge(u32 rx_sw_if_index, u32 bd_id, u8 shg, u8 bvi, u8 enable);

int sc_l2_bridge_domain_add_del_subscribe_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription);
int sc_l2_interface_set_l2_bridge_subscribe_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription);

#endif //_SC_L2_H_





