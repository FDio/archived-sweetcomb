/*
 * Copyright (c) 2016 Cisco and/or its affiliates.
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

#ifndef _V3PO__INTERFACE_H__
#define _V3PO__INTERFACE_H__

#include <vapi/tapv2.api.vapi.h>
#include <vapi/l2.api.vapi.h>

/**
 * V3PO defines operations for specific interfaces used by VPP like:
 *   - tapv2
 */

/* tapv2 */

typedef vapi_payload_tap_create_v2 tapv2_create_t;
typedef vapi_payload_sw_interface_tap_v2_details tapv2_dump_t;

/**
 * TODO problem: vapi_payload_sw_interface_tap_v2_details reply is NULL
 * @brief Dump information about a tap interface
 * @param dump - where dump information will be stored
 * @return 0 on success, or negative SCVPP error code
 */
extern int dump_tapv2(tapv2_dump_t *dump);

/**
 * @brief Create a tapv2 interface
 * @param query - required structure for the creation of a VPP tapv2 interface
 * @return 0 on success, or negative SCVPP error code
 */
extern int create_tapv2(tapv2_create_t *query);

/**
 * @brief Delete a tapv2 interface
 * @param interface_name - name of the interface to delete
 * @return 0 on success, or negative SCVPP error code
 */
extern int delete_tapv2(char *interface_name);

#endif /* __V3PO_INTERFACE_H__ */
