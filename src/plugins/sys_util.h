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
#include <sysrepo/plugins.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>

#include <scvpp/comm.h>

/* BEGIN sysrepo utils */

#define foreach_change(ds, it, oper, old, new) \
    while( (event != SR_EV_ABORT) && \
            sr_get_change_next(ds, it, &oper, &old, &new) == SR_ERR_OK)

#define XPATH_SIZE 2000
#define NOT_AVAL "NOT AVAILABLE"

static inline int
get_xpath_key(char *dst, char *xpath, char *node, char *key, int length,
              sr_xpath_ctx_t *state) {
    char *tmp;

    tmp = sr_xpath_key_value(xpath, node, key, state);
    if (!tmp) {
        SRP_LOG_ERR("%s %s not found.", node, key);
        return SR_ERR_INVAL_ARG;
    }
    strncpy(dst, tmp, length);
    sr_xpath_recover(state);

    return 0;
}

/* END of sysrepo utils */

typedef struct
{
    uint8_t address[4];
} sc_ipv4_addr;

/**
 * @brief Extract IP "AAA.BBB.CCC.DDD" from an IP prefix: "AAA.BBB.CCC.DDD/XX"
 * @param prefix - IPv4 or IPv6 prefix:
 * @return prefix length
 */
static inline int ip_prefix_split(const char* ip_prefix)
{
    //find the slash
    char* slash = strchr(ip_prefix, '/');
    if (NULL == slash)
        return -1;

    //extract subnet mask length
    char * eptr = NULL;
    uint8_t plen = strtoul(slash + 1, &eptr, 10);
    if (*eptr || plen <= 0)
        return -1;

    //keep just the address part
    *slash = '\0';         //replace '/' with 0

    //return prefix length
    return plen;
}

/**
 * @brief Get IPv4 host address from IPv4 prefix.
 *
 * @param[out] dst Host IPv4 address.
 * @param[in] src IPv4 Prefix.
 * @param[in] length dst buffer length.
 * @param[out] prefix Get Prefix length, optional value. Can be NULL.
 * @return -1 when failure, 0 on success.
 */
static inline int
prefix2address(char *dst, const char *src, uint8_t *prefix_length)
{
    if (!src || !dst)
        return -1;

    char *p = strchr(src, '/');
    if (!p)
        return -1; // '/' not found

    if ((p - src + 1) > VPP_IP4_ADDRESS_STRING_LEN) //+ 1 needed for \0
        return -1;

    strncpy(dst, src, VPP_IP4_ADDRESS_STRING_LEN);

    if (!prefix_length)
        *prefix_length = atoi(++p);

    return 0;
}

/**
 * @brief Helper function for converting netmask (ex: 255.255.255.0)
 * to prefix length (ex: 24).
 * @param netmask - string of netmask "AAA.BBB.CCC.DDD"
 * @return prefix length
 */
static inline uint8_t netmask_to_prefix(const char *netmask)
{
    in_addr_t n = 0;
    uint8_t i = 0;

    inet_pton(AF_INET, netmask, &n);

    while (n > 0) {
        n = n >> 1;
        i++;
    }

    return i;
}

/**
 * @brief Convert prefix length to netmask
 * @param prefix - prefix length (ex: 24)
 * @return netmask - integer (ex: 111111111111111111111111100000000b )
 */
static inline uint32_t prefix2mask(int prefix)
{
    if (prefix) {
        return htonl(~((1 << (32 - prefix)) - 1));
    } else {
        return htonl(0);
    }
}

/**
 * @brief Get IPv4 broadcast IP address form IPv4 network address.
 *
 * @param[out] broadcat Broadcast Ipv4 address.
 * @param[in] network Network IPv4 address.
 * @param[in] prefix Prefix number.
 * @return -1 when failure, 0 on success.
 */
static inline int get_network_broadcast(sc_ipv4_addr *broadcast, const sc_ipv4_addr *network,
                          uint8_t prefix_length)
{
    uint8_t mask = ~0;
    uint8_t tmp_p = prefix_length;
    int i;

    ARG_CHECK2(-1, network, broadcast);

    if (32 < prefix_length) {
        SRP_LOG_ERR_MSG("Prefix length to big.");
        return -1;
    }

    for (i = 0; i < 4 ; i++) {
        broadcast->address[i] = network->address[i] |
                                            (mask >> (tmp_p > 8 ? 8 : tmp_p));
        if (tmp_p >= 8) {
            tmp_p -= 8;
        } else {
            tmp_p = 0;
        }
    }

    return 0;
}

/**
 * @brief Get last IPv4 address from the IP range.
 *
 * @param[out] last_ip_address Last Ipv4 address.
 * @param[in] first_ip_address First IPv4 address.
 * @param[in] prefix Prefix number.
 * @return -1 when failure, 0 on success.
 */
static inline int get_last_ip_address(sc_ipv4_addr* last_ip_address,
                        const sc_ipv4_addr* first_ip_address,
                        uint8_t prefix_length)
{
    return get_network_broadcast(last_ip_address, first_ip_address,
                                 prefix_length);
}

#endif /* __SYS_UTIL_H__ */
