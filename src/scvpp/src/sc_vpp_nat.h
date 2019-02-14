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

#ifndef __BAPI_NAT_H__
#define __BAPI_NAT_H__

#include <vapi/nat.api.vapi.h>

typedef vapi_payload_nat44_interface_details nat44_interface_details_t;
typedef vapi_payload_nat44_add_del_interface_addr nat44_add_del_interface_addr_t;
typedef vapi_payload_nat44_add_del_address_range nat44_add_del_address_range_t;
typedef vapi_payload_nat44_add_del_static_mapping nat44_add_del_static_mapping_t;
typedef vapi_payload_nat44_static_mapping_details nat44_static_mapping_details_t;
typedef vapi_payload_nat44_forwarding_enable_disable nat44_forwarding_enable_disable_t;
typedef vapi_payload_nat_set_workers nat_set_workers_t;


//Wrapper function, if we want hide the VAPI return value
extern int nat44_interface_dump(nat44_interface_details_t *reply);
extern int nat44_add_del_interface_addr(
                                    const nat44_add_del_interface_addr_t *msg);
extern int nat44_add_del_addr_range(const nat44_add_del_address_range_t *range);
extern int nat44_add_del_static_mapping(
                                    const nat44_add_del_static_mapping_t *msg);
extern int nat44_static_mapping_dump(nat44_static_mapping_details_t *reply);
extern int nat44_forwarding_enable_disable(
                                    const nat44_forwarding_enable_disable_t *msg);
extern int nat_set_workers(const nat_set_workers_t *msg);


// Alternative, if we don't want hide VAPI return value

// extern vapi_error_e bin_api_nat44_interface_dump(nat44_interface_details_t *reply);
// extern vapi_error_e bin_api_nat44_add_del_interface_addr(
//                                         const nat44_add_del_interface_addr_t *msg);
// extern vapi_error_e bin_api_nat44_add_del_addr_range(
//                         const nat44_add_del_address_range_t *range);
// extern vapi_error_e bin_api_nat44_add_del_static_mapping(
//                                     const nat44_add_del_static_mapping_t *msg);
// extern vapi_error_e bin_api_nat44_static_mapping_dump(
//                                         nat44_static_mapping_details_t *reply);
// extern vapi_error_e bin_api_nat44_forwarding_enable_disable(
//                                     const nat44_forwarding_enable_disable_t *msg);
// extern vapi_error_e bin_api_nat_set_workers(const nat_set_workers_t *msg);


#endif /* __BAPI_NAT_H__ */

