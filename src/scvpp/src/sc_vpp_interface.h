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

int interface_enable(const char *interface_name, const bool enable);

//TODO remove the following structures ASAP
typedef struct {
    bool interface_found;
    vapi_payload_sw_interface_details sw_interface_details;
} sw_interface_details_query_t;

typedef struct _vpp_interface_t
{
    u32 sw_if_index;
    char interface_name[VPP_INTFC_NAME_LEN];
    u8 l2_address[VPP_MAC_ADDRESS_LEN];
    u32 l2_address_length;
    u64 link_speed;
    u16 link_mtu;
    u8 admin_up_down;
    u8 link_up_down;
} vpp_interface_t;

typedef struct _dump_all_ctx
{
    int num_ifs;
    int capacity;
    vpp_interface_t * intfcArray;
} dump_all_ctx;

/* return the number of interfaces or a negative error code */
extern int interface_dump_all(dump_all_ctx * dctx);

extern void sw_interface_details_query_set_name(sw_interface_details_query_t * query,
                                                const char * interface_name);

//input - sw_interface_details_query shall contain interface_name
extern int get_interface_id(sw_interface_details_query_t * sw_interface_details_query);

//input - sw_interface_details_query shall contain sw_if_index
extern int get_interface_name(sw_interface_details_query_t * sw_interface_details_query);

#endif /* __BAPI_INTERFACE_H__ */
