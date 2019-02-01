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

#include "bapi.h"

#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

DEFINE_VAPI_MSG_IDS_VPE_API_JSON

static char *api_prefix = NULL;
static const int max_outstanding_requests = 64;
static const int response_queue_size = 32;

vapi_ctx_t g_vapi_ctx;
vapi_mode_e g_vapi_mode = VAPI_MODE_NONBLOCKING;

///
/// Connect to binary api.
///
vapi_error_e bin_api_connect(const char *client_name, vapi_mode_e vapi_mode) {
    vapi_error_e rv = vapi_ctx_alloc(&g_vapi_ctx);
    if (VAPI_OK != rv) {
        SC_LOG_DBG_MSG("cannot allocate context");
        return rv;
    }

    rv = vapi_connect (g_vapi_ctx, client_name, api_prefix, max_outstanding_requests,
                    response_queue_size, vapi_mode, true);

    if (VAPI_OK != rv) {
        SC_LOG_DBG_MSG("error: connecting to vlib");
        vapi_ctx_free(g_vapi_ctx);
        return rv;
    }

    return rv;
}

///
/// Disconnect from binary api.
///
vapi_error_e bin_api_disconnect(void) {
    vapi_error_e rv = vapi_disconnect (g_vapi_ctx);
    if (VAPI_OK != rv) {
        SC_LOG_DBG("error: (rc:%d)", rv);
        //return rv;
    }

    vapi_ctx_free (g_vapi_ctx);

    return rv;
}

bool bapi_aton(const char *cp, u8 * buf)
{
    ARG_CHECK2(false, cp, buf);

    struct in_addr addr;
    int ret = inet_aton(cp, &addr);

    if (0 == ret)
    {
        SC_LOG_DBG("error: ipv4 address %s", cp);
        return false;
    }

    memcpy(buf, &addr, sizeof (addr));
    return true;
}

char* bapi_ntoa(u8 * buf)
{
    struct in_addr addr;
    memcpy(&addr, buf, sizeof(addr));
    return inet_ntoa(addr);
}

vapi_error_e
vapi_retval_cb(const char* func_name, i32 retval)
{
    if (retval)
    {
        SC_LOG_DBG("%s: bad retval=%d", func_name, retval);
    }

    return VAPI_OK;
}
