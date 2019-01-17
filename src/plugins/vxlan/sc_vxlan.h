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

#ifndef _SC_VXLAN_TUNNEL_H_
#define _SC_VXLAN_TUNNEL_H_

#include <vapi/vxlan.api.vapi.h>
#include "scvpp.h"

i32 sc_vxlan_add_del_tunnel(u32* tunnel_if_index, u8 is_add,
        u8 is_ipv6, i32 instance,
        u8 src_address[VPP_IP6_ADDRESS_LEN], u8 dst_address[VPP_IP6_ADDRESS_LEN],
        u32 mcast_sw_if_index, u32 encap_vrf_id,
        u32 decap_next_index, u32 vni);

int sc_vxlan_subscribe_tunnel_events(sr_session_ctx_t *session, sr_subscription_ctx_t **subscription);


#endif //_SC_VXLAN_TUNNEL_H_





