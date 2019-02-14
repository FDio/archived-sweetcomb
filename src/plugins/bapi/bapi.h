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


#ifndef __BAPI_H__
#define __BAPI_H__


#include "sc_vpp_comm.h"
#include <vapi/vapi.h>
#include <vapi/vapi_common.h>
#include <vapi/vpe.api.vapi.h>

/**********************************GLOBALS**********************************/
extern vapi_ctx_t g_vapi_ctx;
extern vapi_mode_e g_vapi_mode;

/**********************************MACROS**********************************/
#define VAPI_RETVAL_CB(api_name) \
vapi_error_e \
api_name##_cb (vapi_ctx_t ctx, void *caller_ctx, vapi_error_e rv, bool is_last, \
		    vapi_payload_##api_name##_reply * reply) \
{ \
    return vapi_retval_cb(__FUNCTION__, reply->retval); \
}

#define VAPI_COPY_CB(api_name) \
vapi_error_e \
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
            rv = vapi_dispatch (g_vapi_ctx); \
        } \
    } \
    while (0)

#define VAPI_CALL(call_code) VAPI_CALL_MODE(call_code, g_vapi_mode)


/**********************************FUNCTIONS**********************************/
extern vapi_error_e bin_api_connect(const char *client_name, vapi_mode_e mode);
extern vapi_error_e bin_api_disconnect(void);


//returns true on success
bool bapi_aton(const char *cp, u8 * buf);
char * bapi_ntoa(u8 * buf);

vapi_error_e
vapi_retval_cb(const char* func_name, i32 retval);


#endif //__BAPI_H__
