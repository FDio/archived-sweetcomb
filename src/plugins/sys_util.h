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

#ifndef __SYS_UTIL_H__
#define __SYS_UTIL_H__

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#define XPATH_SIZE 2000

typedef struct
{
    char xpath_root[XPATH_SIZE];
    sr_val_t * values;
    size_t values_cnt;
} sysr_values_ctx_t;

char* xpath_find_first_key(const char *xpath, char *key, sr_xpath_ctx_t *state);
void log_recv_event(sr_notif_event_t event, const char *msg);
void log_recv_oper(sr_change_oper_t oper, const char *msg);
int ip_prefix_split(const char* ip_prefix);

/**
 * @brief Get IPv4 host address from IPv4 prefix.
 *
 * @param[out] dst Host IPv4 address.
 * @param[in] src IPv4 Prefix.
 * @param[in] length dst buffer length.
 * @param[out] prefix Get Prefix length, optional value. Can be NULL.
 * @return -1 when failure, 0 on success.
 */
int get_address_from_prefix(char* dst, const char* src, size_t length,
                            uint8_t* prefix_length);

typedef struct
{
    uint8_t address[4];
} sc_ipv4_addr;

/**
 * @brief Get IPv4 broadcast IP address form IPv4 network address.
 *
 * @param[out] broadcat Broadcast Ipv4 address.
 * @param[in] network Network IPv4 address.
 * @param[in] prefix Prefix number.
 * @return -1 when failure, 0 on success.
 */
int get_network_broadcast(sc_ipv4_addr *broadcast, const sc_ipv4_addr *network,
                          uint8_t prefix_length);

/**
 * @brief Get last IPv4 address from the IP range.
 *
 * @param[out] last_ip_address Last Ipv4 address.
 * @param[in] first_ip_address First IPv4 address.
 * @param[in] prefix Prefix number.
 * @return -1 when failure, 0 on success.
 */
int get_last_ip_address(sc_ipv4_addr *last_ip_address,
                        const sc_ipv4_addr *first_ip_address,
                        uint8_t prefix_length);

#endif /* __SYS_UTIL_H__ */
