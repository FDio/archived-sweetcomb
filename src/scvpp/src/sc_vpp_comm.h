/*
 * Copyright (c) 2018 HUACHENTEL and/or its affiliates.
 * Copyright (c) 2018 PANTHEON.tech
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

#ifndef __SC_VPP_COMMM_H__
#define __SC_VPP_COMMM_H__

#include <errno.h>

#include <vapi/vapi.h>
#include <vapi/vapi_common.h>
#include <vapi/vpe.api.vapi.h>

// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_VPE_API_JSON;

#define VPP_INTFC_NAME_LEN 64
#define VPP_TAPV2_NAME_LEN VPP_INTFC_NAME_LEN
#define VPP_IP4_ADDRESS_LEN 4
#define VPP_IP6_ADDRESS_LEN 16
#define VPP_IP4_ADDRESS_STRING_LEN 16
#define VPP_IP6_ADDRESS_STRING_LEN 46
#define VPP_MAC_ADDRESS_LEN 8
#define VPP_TAG_LEN VPP_INTFC_NAME_LEN
#define VPP_IKEV2_PROFILE_NAME_LEN VPP_INTFC_NAME_LEN
#define VPP_IKEV2_PSK_LEN VPP_INTFC_NAME_LEN
#define VPP_IKEV2_ID_LEN 32

/**********************************MACROS**********************************/
#define ARG_CHECK(retval, arg) \
    do \
    { \
        if (NULL == (arg)) \
        { \
            return (retval); \
        } \
    } \
    while (0)

#define ARG_CHECK2(retval, arg1, arg2) \
    ARG_CHECK(retval, arg1); \
    ARG_CHECK(retval, arg2)

#define ARG_CHECK3(retval, arg1, arg2, arg3) \
    ARG_CHECK(retval, arg1); \
    ARG_CHECK(retval, arg2); \
    ARG_CHECK(retval, arg3)

#define ARG_CHECK4(retval, arg1, arg2, arg3, arg4) \
    ARG_CHECK(retval, arg1); \
    ARG_CHECK(retval, arg2); \
    ARG_CHECK(retval, arg3); \
    ARG_CHECK(retval, arg4)

#define VAPI_RETVAL_CB(api_name) \
static vapi_error_e \
api_name##_cb (vapi_ctx_t ctx, void *caller_ctx, vapi_error_e rv, bool is_last, \
                vapi_payload_##api_name##_reply * reply) \
{ \
    return reply->retval; \
}

#define VAPI_COPY_CB(api_name) \
static vapi_error_e \
api_name##_cb (vapi_ctx_t ctx, void *caller_ctx, vapi_error_e rv, bool is_last, \
                vapi_payload_##api_name##_reply * reply) \
{ \
    if (caller_ctx) \
    { \
        vapi_payload_##api_name##_reply * passed = (vapi_payload_##api_name##_reply *)caller_ctx; \
        *passed = *reply; \
    } \
    return VAPI_OK; \
}\

#define VAPI_CALL_MODE(call_code, vapi_mode) \
    do \
    { \
        if (VAPI_MODE_BLOCKING == (vapi_mode)) \
        { \
            rv = call_code; \
        } \
        else \
        { \
            while (VAPI_EAGAIN == (rv = call_code)); \
            rv = vapi_dispatch (g_vapi_ctx_instance); \
        } \
    } \
    while (0)

#define VAPI_CALL(call_code) VAPI_CALL_MODE(call_code, g_vapi_mode)

int sc_aton(const char *cp, u8 * buf, size_t length);
char * sc_ntoa(const u8 * buf);

/*
 * VPP
 */

extern vapi_ctx_t g_vapi_ctx_instance;
extern vapi_mode_e g_vapi_mode;

int sc_connect_vpp();
int sc_disconnect_vpp();
int sc_end_with(const char* str, const char* end);

#endif //__SC_VPP_COMMM_H__
