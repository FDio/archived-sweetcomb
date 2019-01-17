/*
 * Copyright (c) 2018 HUACHENTEL and/or its affiliates.
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
#include "scvpp.h"

DEFINE_VAPI_MSG_IDS_VPE_API_JSON;

static char *APP_NAME = "hctel-hubcpe";
static char *API_PREFIX = NULL;
static const int MAX_OUTSTANDING_REQUESTS = 64;
static const int RESPONSE_QUEUE_SIZE = 32;
/*static*/ vapi_ctx_t app_ctx = NULL; //Global scvpp context

int
sc_connect_vpp()
{
    SC_INVOKE_BEGIN;
    if (NULL == app_ctx)
    {
        vapi_error_e rv = vapi_ctx_alloc(&app_ctx);
        rv = vapi_connect(app_ctx, APP_NAME, API_PREFIX, MAX_OUTSTANDING_REQUESTS, RESPONSE_QUEUE_SIZE, VAPI_MODE_BLOCKING, true);
        if (rv != VAPI_OK)
        {
            SC_LOG_ERR("*failed to connect %s and return -1", APP_NAME);
            return -1;
        }
        SC_LOG_DBG("*connected %s ok", APP_NAME);
    }
    else
    {
        SC_LOG_DBG("*connection to %s is keeping now, just reuse it .", APP_NAME);
    }

    SC_INVOKE_END;
    return 0;
}

int
sc_disconnect_vpp()
{
    if (NULL != app_ctx)
    {
        vapi_disconnect(app_ctx);
        vapi_ctx_free(app_ctx);
        app_ctx = NULL;
    }
    return 0;
}

int
sc_end_with(const char* source, const char* pattern)
{
    if (NULL != source && NULL != pattern)
    {
        int lenSrc = strlen(source);
        int lenPat = strlen(pattern);
        if (lenSrc >= lenPat)
        {
            if (strcmp(source + lenSrc - lenPat, pattern) == 0)
            {
                return 1;
            }
        }
    }
    return 0;
}
