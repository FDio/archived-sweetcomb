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

/* If no IP has been found ip_addr will be "0.0.0.0" */
extern int ipv46_address_dump(const char *interface_name, char *ip_addr,
                              u8 *prefix_len, bool is_ipv6);

extern int ipv46_config_add_remove(const char *if_name, const char *addr,
                                   uint8_t prefix, bool is_ipv6, bool add);

extern int
ipv46_config_add_del_route(const char* dst_address, u8 dst_address_length,
                           const char* next_address, u8 is_add, u32 table_id,
                           const char *interface);

#endif /* __BAPI_IP_H__ */
