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

#include "sc_vpp_comm.h"

#include <vapi/interface.api.vapi.h>

typedef struct _s_vpp_interface_
{
  u32 sw_if_index;
  char interface_name[VPP_INTFC_NAME_LEN];
  u8 l2_address[VPP_MAC_ADDRESS_LEN];
  u32 l2_address_length;
  u64 link_speed;
  u16 link_mtu;
  u8 admin_up_down;
  u8 link_up_down;
}scVppIntfc;

typedef struct _ietf_sw_interface_dump_ctx
{
  u8 last_called;
  int num_ifs;
  int capacity;
  scVppIntfc * intfcArray;
} ietf_sw_interface_dump_ctx;

int ietf_initSwInterfaceDumpCTX(ietf_sw_interface_dump_ctx * dctx);
int ietf_freeSwInterfaceDumpCTX(ietf_sw_interface_dump_ctx * dctx);
int ietf_swInterfaceDump(ietf_sw_interface_dump_ctx * dctx);
u32 ietf_interface_name2index(const char *name, u32* if_index);

i32 ietf_interface_add_del_addr( u32 sw_if_index, u8 is_add, u8 is_ipv6, u8 del_all,
			       u8 address_length, u8 address[VPP_IP6_ADDRESS_LEN] );
i32 ietf_setInterfaceFlags(u32 sw_if_index, u8 admin_up_down);


int
ietf_interface_subscribe_events(sr_session_ctx_t *session,
			      sr_subscription_ctx_t **subscription);

#endif /* SC_INTERFACE_H */

