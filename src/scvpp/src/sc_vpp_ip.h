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


#ifndef __BAPI_IP_H__
#define __BAPI_IP_H__

#include <vapi/interface.api.vapi.h>
#include <vapi/ip.api.vapi.h>


extern vapi_error_e bin_api_sw_interface_add_del_address(
    u32 sw_if_index,
    bool is_add,
   const char * ip_address,
   u8 address_length);

extern vapi_error_e bin_api_sw_interface_del_all_address(u32 sw_if_index);

//ip_add_del_route
extern vapi_error_e bin_api_ip_add_del_route(
    vapi_payload_ip_add_del_route_reply * reply,
    const char* dst_address,
    u8 dst_address_length,
    const char* next_address,
    u8 is_add,
    u32 table_id,
    const char *interface);

extern vapi_error_e bin_api_ip_fib_dump();
extern vapi_error_e bin_api_table_add_del(u8 is_add, u32 table_id);

#endif /* __BAPI_IP_H__ */
