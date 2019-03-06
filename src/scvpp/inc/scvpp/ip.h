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

typedef vapi_payload_ip_fib_details fib_dump_t;

/*
 * @brief Dump IPv4/IPv6 address from an interface.
 * @param interface_name - name of the interface to get ip from.
 * @param ip_addr - where dump will store IP. If IP not found, returns 0.0.0.0
 * @param prefix_len - pointer where dump will store prefix
 * @param is_ipv6 - true = IPv6, false = IPv4
 * @return 0 on success, or nergative SCVPP error code
 */
extern int ipv46_address_dump(const char *interface_name, char *ip_addr,
                              u8 *prefix_len, bool is_ipv6);

/**
 * @brief Add or remove IPv4/IPv6 address to/from an interface.
 * @param interface_name - name of interface to configure
 * @param addr - address to add
 * @param prefix_len - prefix length of interface
 * @param is_ipv6 - true if ipv6, false otherwise
 * @param add - true to add, false to remove
 */
extern int ipv46_config_add_remove(const char *interface_name, const char *addr,
                                   uint8_t prefix_len, bool is_ipv6, bool add);

/*
 * TODO should add a field is_ipv6 because it only do ipv4 now
 * @brief Add or remove an IP route
 * @param dst_address - subnet IP you wish to route
 * @param dst_address_length - prefix length for subnet you wish to route
 * @param next_address - Next hop IP (can use next_hop_interface instead)
 * @param is_add - true to add, false to remove
 * @param table_id - id of the tab in FIB
 * @param next_hop_interface - Next hop interface (can use next_address instead)
 */
extern int
ipv46_config_add_del_route(const char* dst_address, u8 dst_address_length,
                           const char* next_address, u8 is_add, u32 table_id,
                           const char *next_hop_interface);

/**
 * @brief Dump all FIB tables entries
 * @return stacked answers on success, or NULL on failure
 */
extern struct elt* ipv4_fib_dump_all();

/*
 * @brief Dump information about a prefix, based on fib_dump_all
 * @param prefix_xpath - prefix to look for in FIB
 * @param reply - FIB entry dump replied
 * @return SCVPP_OK if prefix found or SCVPP_NOT_FOUND
 */
extern int ipv4_fib_dump_prefix(const char *prefix_xpath, fib_dump_t **reply);

#endif /* __BAPI_IP_H__ */
