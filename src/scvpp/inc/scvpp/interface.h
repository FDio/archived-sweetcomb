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

#ifndef __BAPI_INTERFACE_H__
#define __BAPI_INTERFACE_H__

#include <vapi/interface.api.vapi.h>
#include <scvpp/comm.h>

typedef vapi_payload_sw_interface_details sw_interface_dump_t;

/**
 * @brief Change an interface state
 * @param interface_name - name of target interface
 * @param enable - true=state up, false=state down
 * @return 0 for success, else negative SCVPP error code
 */
int interface_enable(const char *interface_name, const bool enable);

/**
 * @brief Dump details about all existing interfaces
 * @return stack structure containing all interfaces or NULL if empty
 */
extern struct elt * interface_dump_all();


/**
 * @brief Dump details about a specific interface
 * @param details - where answer will be written
 * @param interface_name - name of the interface to dump
 * @return 0 for success or negative SCVPP error code
 */
extern int interface_dump_iface(sw_interface_dump_t *details,
                                const char *interface_name);

/*
 * Library internal helper functions. Symbols should not be exported eventually
 */

/*
 * @brief Get VPP internal index for an interface
 * @param interface_name - name of the interface to get id from
 * @param sw_if_index - pointer to interface index to be returned
 * @return 0 upon success or negative SCVPP error code
 */
extern int get_interface_id(const char *interface_name, uint32_t *sw_if_index);

/* @brief Get interface name from an interface ID. This is a super expensive
 * operation !!! Avoid using it.
 * @param interface_name - pointer to string holding returned interface name
 * @param sw_if_index - interface index provided
 * @return 0 upon success or negative SCVPP error code
 */
extern int get_interface_name(char *interface_name, uint32_t sw_if_index);

#endif /* __BAPI_INTERFACE_H__ */
