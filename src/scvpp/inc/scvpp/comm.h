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

typedef enum {
    SCVPP_OK = 0,       /* Success */
    SCVPP_EINVAL,       /* Invalid value encountered */
    SCVPP_EAGAIN,       /* Operation would block */
    SCVPP_ENOTSUP,      /* Operation not supported */
    SCVPP_ENOMEM,       /* Out of memory */
    SCVPP_NOT_FOUND,    /* Required element can not be found */
} scvpp_error_e;

// Use VAPI macros to define symbols
DEFINE_VAPI_MSG_IDS_VPE_API_JSON;

#define VPP_IP4_HOST_PREFIX_LEN 32
#define VPP_INTFC_NAME_LEN 64           /* Interface name max length */
#define VPP_IP4_ADDRESS_LEN 4           /* IPv4 length in VPP format */
#define VPP_IP6_ADDRESS_LEN 16          /* IPv6 length in VPP format */
#define VPP_MAC_ADDRESS_LEN 8           /* MAC length in VPP format  */
/* IPv4 and IPv6 length in string format */
#define VPP_IP4_ADDRESS_STRING_LEN INET_ADDRSTRLEN //16, include '\0'
#define VPP_IP6_ADDRESS_STRING_LEN INET6_ADDRSTRLEN //46, include '\0'
#define VPP_IP4_PREFIX_STRING_LEN \
    INET_ADDRSTRLEN + sizeof('/') + 2 // include '\0'
#define VPP_IP6_PREFIX_STRING_LEN \
    INET6_ADDRSTRLEN + sizeof('/') + 3 // include '\0'

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

/* Suppress compiler warning about unused variable.
 * This must be used only for callback function else suppress your unused
 * parameter in function prototype. */
#define UNUSED(x) (void)x

#define VAPI_REQUEST_CB(api_name) \
static vapi_error_e \
api_name##_cb (struct vapi_ctx_s *ctx, void *callback_ctx, vapi_error_e rv, \
		bool is_last, vapi_payload_##api_name##_reply * reply) \
{ \
    UNUSED(ctx); UNUSED(rv); UNUSED(is_last); \
    if (callback_ctx) { \
        memcpy(callback_ctx, reply, sizeof(vapi_payload_##api_name##_reply)); \
    } \
    return reply->retval; \
}

#define VAPI_REQUEST_CB_EXTRA(api_name, member) \
static vapi_error_e \
api_name##_cb (struct vapi_ctx_s *ctx, void *callback_ctx, vapi_error_e rv, \
		bool is_last, vapi_payload_##api_name##_reply * reply) \
{ \
    UNUSED(ctx); UNUSED(rv); UNUSED(is_last); \
    vapi_payload_##api_name##_reply **p = callback_ctx; \
    int size = sizeof(vapi_payload_##api_name##_reply) + reply->member; \
    if (p) { \
        *p = malloc(size); \
        if (!*p) { \
            return VAPI_ENOMEM; \
        } \
        memcpy(*p, reply, size); \
    } \
    return reply->retval; \
}

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
            if (rv != VAPI_OK) { /* try once more to get reply */ \
                rv = vapi_dispatch (g_vapi_ctx); \
            } \
        } \
    } \
    while (0)

#define VAPI_CALL(call_code) VAPI_CALL_MODE(call_code, g_vapi_mode)

struct elt {
    void *data; //vapi_payload structure
    struct elt *next;
    int id; //id of the stack element to count total nb of elements
};

static inline int push(struct elt **stack, void *data, int length)
{
    struct elt *el;

    //new stack node
    el = malloc(sizeof(struct elt));
    if (!el)
        return -ENOMEM;
    el->data = malloc(length);
    if (!el->data) {
        free(el);
        return -ENOMEM;
    }


    memcpy(el->data, data, length);
    if (*stack)
        el->id = (*stack)->id + 1;
    else
        el->id = 0;
    el->next = *stack; //point to old value of stack
    *stack = el; //el is new stack head

    return 0;
}

static inline void * pop(struct elt **stack)
{
    struct elt *prev;
    void *data;

    if (!(*stack))
        return NULL;

    data = (*stack)->data; //get data at stack head
    prev = *stack; //save stack to free memory later
    *stack = (*stack)->next; //new stack

    free(prev);
    prev = NULL;

    return data;
}

#define VAPI_DUMP_LIST_CB(api_name) \
static vapi_error_e \
api_name##_all_cb(vapi_ctx_t ctx, void *caller_ctx, vapi_error_e rv, bool is_last, \
              vapi_payload_##api_name##_details *reply) \
{ \
    UNUSED(ctx); UNUSED(rv); \
    struct elt **stackp; \
    \
    if (is_last) { \
        return VAPI_OK; \
    } \
    \
    ARG_CHECK2(VAPI_EINVAL, caller_ctx, reply); \
    \
    stackp = (struct elt**) caller_ctx; \
    push(stackp, reply, sizeof(*reply)); \
    \
    return VAPI_OK; \
}

#define foreach_stack_elt(stack)  \
    for(void *data = pop(&stack); data != NULL ; data = pop(&stack))
//for(void *data = pop(&stack); stack != NULL ; data = pop(&stack)) // No!!

int sc_aton(const char *cp, u8 * buf, size_t length);
char * sc_ntoa(const u8 * buf);
int sc_pton(int af, const char *cp, u8 * buf);
const char * sc_ntop(int af, const u8 * buf, char *addr);

/**
 * @brief Function converts the u8 array from network byte order to host byte order.
 *
 * @param[in] host IPv4 address.
 * @return host byte order value.
 */
uint32_t hardntohlu32(uint8_t host[4]);

/*
 * VPP
 */

extern vapi_ctx_t g_vapi_ctx;
extern vapi_mode_e g_vapi_mode;

int sc_connect_vpp();
int sc_disconnect_vpp();
int sc_end_with(const char* str, const char* end);

#endif //__SC_VPP_COMMM_H__
