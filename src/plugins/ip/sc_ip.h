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

#ifndef _SC_IP_H_
#define _SC_IP_H_

#include <vapi/ip.api.vapi.h>
#include <vnet/mpls/mpls_types.h>
#include "scvpp.h"

i32 sc_ip_add_del_route(u32 next_hop_sw_if_index, u32 table_id, u32 classify_table_index, u32 next_hop_table_id, u32 next_hop_id,
        u8 is_add, u8 is_drop, u8 is_unreach, u8 is_prohibit, u8 is_ipv6, u8 is_local, u8 is_classify, u8 is_multipath,
        u8 is_resolve_host, u8 is_resolve_attached, u8 is_dvr, u8 is_source_lookup, u8 is_udp_encap,
        u8 next_hop_weight, u8 next_hop_preference, u8 next_hop_proto,
        u8 dst_address_length, u8 dst_address[VPP_IP6_ADDRESS_LEN], u8 next_hop_address[VPP_IP6_ADDRESS_LEN],
        u8 next_hop_n_out_labels, u32 next_hop_via_label, vapi_type_fib_mpls_label* next_hop_out_label_stack);

int sc_ip_subscribe_route_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription);


#endif //_SC_IP_H_





