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
#include <scvpp/comm.h>

#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define APP_NAME "sweetcomb_vpp"
#define MAX_OUTSTANDING_REQUESTS 4
#define RESPONSE_QUEUE_SIZE 2

vapi_ctx_t g_vapi_ctx = NULL;
vapi_mode_e g_vapi_mode = VAPI_MODE_NONBLOCKING;

int sc_connect_vpp()
{
    if (g_vapi_ctx == NULL)
    {
        vapi_error_e rv = vapi_ctx_alloc(&g_vapi_ctx);
        rv = vapi_connect(g_vapi_ctx, APP_NAME, NULL,
                          MAX_OUTSTANDING_REQUESTS, RESPONSE_QUEUE_SIZE,
                          VAPI_MODE_BLOCKING, true);
        if (rv != VAPI_OK)
        {
            vapi_ctx_free(g_vapi_ctx);
            g_vapi_ctx = NULL;
            return -1;
        }
    }

    return 0;
}

int sc_disconnect_vpp()
{
    if (NULL != g_vapi_ctx)
    {
        vapi_disconnect(g_vapi_ctx);
        vapi_ctx_free(g_vapi_ctx);
        g_vapi_ctx = NULL;
    }
    return 0;
}

int sc_end_with(const char* str, const char* end)
{
    if (str != NULL && end != NULL)
    {
        int l1 = strlen(str);
        int l2 = strlen(end);
        if (l1 >= l2)
        {
            if (strcmp(str + l1 - l2, end) == 0)
                return 1;
        }
    }
    return 0;
}

int sc_aton(const char *cp, u8 * buf, size_t length)
{
    ARG_CHECK2(false, cp, buf);

    struct in_addr addr;
    int ret = inet_aton(cp, &addr);

    if (0 == ret)
        return -EINVAL;

    if (sizeof(addr) > length)
        return -EINVAL;

    memcpy(buf, &addr, sizeof (addr));

    return 0;
}

char* sc_ntoa(const u8 * buf)
{
    ARG_CHECK(NULL, buf);

    struct in_addr addr;
    memcpy(&addr, buf, sizeof(addr));
    return inet_ntoa(addr);
}

int sc_pton(int af, const char *cp, u8 * buf)
{
    ARG_CHECK2(false, cp, buf);

    int ret = inet_pton(af, cp, buf);

    if (0 == ret)
        return -EINVAL;

    return 0;
}

const char* sc_ntop(int af, const u8 * buf, char *addr)
{
    ARG_CHECK(NULL, buf);
    ARG_CHECK(NULL, addr);

    socklen_t size = 0;
    if (af == AF_INET)
        size = INET_ADDRSTRLEN;
    else if (af == AF_INET6)
        size = INET6_ADDRSTRLEN;
    else
        return NULL;

    return inet_ntop(af, (void*)buf, addr, size);
}

/**
 * @brief Function converts the u8 array from network byte order to host byte order.
 *
 * @param[in] host IPv4 address.
 * @return host byte order value.
 */
uint32_t hardntohlu32(uint8_t host[4])
{
    uint32_t tmp = host[3] | host[2] | host[1] | host[0];

    return ntohl(tmp);
}
