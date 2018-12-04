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


typedef struct {
    bool interface_found;
    vapi_payload_sw_interface_details sw_interface_details;
} sw_interface_details_query_t;

extern void sw_interface_details_query_set_name(sw_interface_details_query_t * query,
                                                const char * interface_name);

//input - sw_interface_details_query shall contain interface_name
extern bool get_interface_id(sw_interface_details_query_t * sw_interface_details_query);

//input - sw_interface_details_query shall contain sw_if_index
extern bool get_interface_name(sw_interface_details_query_t * sw_interface_details_query);



extern vapi_error_e bin_api_sw_interface_dump(const char * interface_name);

extern vapi_error_e bin_api_sw_interface_set_flags(u32 if_index, u8 up);

extern vapi_error_e bin_api_sw_interface_set_l2_bridge(u32 bd_id,
                                                       u32 rx_sw_if_index,
                                                       bool enable);

extern vapi_error_e bin_api_create_loopback(
    vapi_payload_create_loopback_reply *reply);


#endif /* __BAPI_INTERFACE_H__ */
