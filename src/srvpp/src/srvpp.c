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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "srvpp.h"
#include "srvpp_logger.h"

#undef vl_api_version
#define vl_api_version(n,v) static u32 vpe_api_version = v;
#include <vpp/api/vpe.api.h>
#undef vl_api_version


#define SRVPP_RESPONSE_TIMEOUT 2 /**< Maximum time (in seconds) that a client waits for the response(s) to a request or dumprequest. */

#define CHECK_NULL(ARG) \
    if (NULL == ARG) { \
        SRVPP_LOG_ERR("NULL value detected for %s argument of %s", #ARG, __func__); \
        return -1; \
    } \

#define CHECK_NULL_RET(ARG, RET) \
    if (NULL == ARG) { \
        SRVPP_LOG_ERR("NULL value detected for %s argument of %s", #ARG, __func__); \
        return RET; \
    } \

/**
 * @brief Type of the response expected from VPP.
 */
typedef enum srvpp_response_type_e {
    SRVPP_REPLY,                        /**< A reply message (single message). */
    SRVPP_DETAILS,                      /**< Multiple details messages. */
} srvpp_response_type_t;

/**
 * @brief srvpp request context structure.
 */
typedef struct srvpp_request_ctx_s {
    struct srvpp_request_ctx_s *_next;  /**< Pointer to the next request context in the linked-list. */
    u32 ctx_id;                         /**< Context ID used to map responses with requests. */
    srvpp_response_type_t resp_type;    /**< Type of the response expected from VPP. */

    i32 resp_retval;                    /**< Return value of the last response (0 = success). */
    bool resp_data_copy;                /**< Controls whether data of the responses shall be copied (returned) or ignored. */
    void **resp_msg_arr;                /**< Array of the pointers to response messages. */
    size_t *resp_msg_sizes;             /**< Array of sizes of messages in the ::resp_msg_arr. */
    size_t resp_msg_arr_size;           /**< Size of the ::resp_msg_arr array. */
    size_t resp_msg_cnt;                /**< Count of the messages currently stored in ::resp_msg_arr array. */

    bool resp_ready;                    /**< Signals that the expected response has arrived and is ready to be returned. */
    pthread_cond_t resp_cv;             /**< Condition variable for ::resp_ready. */
    pthread_mutex_t lock;               /**< Mutex to protect shared access to the context. */
} srvpp_request_ctx_t;

/**
 * @brief srvpp interface info context structure.
 */
typedef struct srvpp_if_info_s {
    struct srvpp_if_info_s *_next;
    const char *if_name;
    u32 if_index;
} srvpp_if_info_t;

/**
 * @brief srvpp context structure.
 */
typedef struct srvpp_ctx_s {
    size_t ref_cnt;                                /**< Context reference counter. */
    unix_shared_memory_queue_t *vlib_input_queue;  /**< VPP Library input queue. */
    u32 vlib_client_index;                         /**< VPP Library client index. */
    srvpp_request_ctx_t *request_ctx_list;         /**< Linked-list of request contexts. */
    srvpp_if_info_t *if_info_list;                 /**< Linked-list of VPP interfaces information. */
    pthread_key_t request_key;                     /**< Key to the thread-specific request context. */
} srvpp_ctx_t;

/**
 * @brief Generic VPP request structure.
 */
typedef struct __attribute__ ((packed)) vl_generic_request_s {
    unsigned short _vl_msg_id;
    unsigned int client_index;
    unsigned int context;
} vl_generic_request_t;

/**
 * @brief Generic VPP response structure.
 */
typedef struct __attribute__ ((packed)) vl_generic_response_s {
    u16 _vl_msg_id;
    u32 context;
} vl_generic_response_t;

/**
 * @brief Generic VPP reply structure (response with a single message).
 */
typedef struct __attribute__ ((packed)) vl_generic_reply_s {
    u16 _vl_msg_id;
    u32 context;
    i32 retval;
} vl_generic_reply_t;

/**
 * @brief Global srvpp context.
 */
static srvpp_ctx_t *srvpp_ctx = NULL;

/**
 * @brief Mutex for the global context.
 */
static pthread_mutex_t srvpp_ctx_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Not used, just to satisfy external references when -lvlib is not available.
 */
void
vlib_cli_output(struct vlib_main_t *vm, char *fmt, ...)
{
    SRVPP_LOG_WRN_MSG("vlib_cli_output callled!");
}

/**
 * @brief Sets correct VPP API version.
 */
void
vl_client_add_api_signatures(vl_api_memclnt_create_t *mp)
{
    /*
     * Send the main API signature in slot 0. This bit of code must
     * match the checks in ../vpe/api/api.c: vl_msg_api_version_check().
     */
    mp->api_versions[0] = clib_host_to_net_u32(vpe_api_version);
}

/**
 * @brief Returns the request context assigned to the active thread.
 * If no request context exists for the thread, it will be automatically created.
 */
static srvpp_request_ctx_t *
srvpp_get_thread_request_ctx(srvpp_ctx_t *ctx)
{
    srvpp_request_ctx_t *req_ctx = NULL;

    CHECK_NULL_RET(ctx, NULL);

    if (NULL == (req_ctx = pthread_getspecific(ctx->request_key))) {
        /* allocate a new request context */
        req_ctx = calloc(1, sizeof(*req_ctx));
        if (NULL != req_ctx) {
            /* initialize the new context */
            req_ctx->ctx_id = (u32)(((uintptr_t)req_ctx) & 0xFFFFFFFF) << 16;
            SRVPP_LOG_DBG("Creating new request ctx with id=%u", req_ctx->ctx_id);

            pthread_mutex_init(&req_ctx->lock, NULL);
            pthread_cond_init (&req_ctx->resp_cv, NULL);

            /* save the request ctx in the srvpp context */
            pthread_mutex_lock(&srvpp_ctx_lock);
            req_ctx->_next = ctx->request_ctx_list;
            ctx->request_ctx_list = req_ctx;
            pthread_mutex_unlock(&srvpp_ctx_lock);

            /* save the request ctx in the thread-local memory */
            pthread_setspecific(ctx->request_key, req_ctx);
        } else {
            SRVPP_LOG_ERR_MSG("Unable to allocate new request context.");
        }
    }

    return req_ctx;
}

/**
 * @brief Returns the request context matching with the provided context id.
 */
static srvpp_request_ctx_t *
srvpp_get_request_ctx(srvpp_ctx_t *ctx, u32 req_ctx_id)
{
    srvpp_request_ctx_t *req_ctx = NULL, *match = NULL;

    CHECK_NULL_RET(ctx, NULL);

    pthread_mutex_lock(&srvpp_ctx_lock);

    req_ctx = ctx->request_ctx_list;

    while (NULL != req_ctx) {
        if (req_ctx->ctx_id == req_ctx_id) {
            match = req_ctx;
            break;
        }
        req_ctx = req_ctx->_next;
    }

    pthread_mutex_unlock(&srvpp_ctx_lock);

    return match;
}

/**
 * @brief Copy data of the message into the provided request context.
 */
static int
srvpp_msg_data_copy(srvpp_request_ctx_t *req_ctx, void *msg, size_t msg_size)
{
    void **msg_arr = NULL;
    size_t *sizes_arr = NULL;
    void *msg_space = NULL;

    CHECK_NULL(req_ctx);

    if (req_ctx->resp_msg_arr_size < (req_ctx->resp_msg_cnt + 1)) {
        /* reallocate arrays to fit one new message */
        msg_arr = realloc(req_ctx->resp_msg_arr, (req_ctx->resp_msg_cnt + 1) * sizeof(*req_ctx->resp_msg_arr));
        if (NULL == msg_arr) {
            SRVPP_LOG_ERR_MSG("Unable to reallocate message array.");
            return -1;
        }
        sizes_arr = realloc(req_ctx->resp_msg_sizes, (req_ctx->resp_msg_cnt + 1) * sizeof(*req_ctx->resp_msg_sizes));
        if (NULL == sizes_arr) {
            SRVPP_LOG_ERR_MSG("Unable to reallocate message sizes array.");
            return -1;
        }
        req_ctx->resp_msg_arr = msg_arr;
        req_ctx->resp_msg_sizes = sizes_arr;
        req_ctx->resp_msg_arr_size = req_ctx->resp_msg_cnt + 1;

        req_ctx->resp_msg_arr[req_ctx->resp_msg_cnt] = NULL;
        req_ctx->resp_msg_sizes[req_ctx->resp_msg_cnt] = 0;
    }

    if (req_ctx->resp_msg_sizes[req_ctx->resp_msg_cnt] < msg_size) {
        /* reallocate space for the message */
        msg_space = realloc(req_ctx->resp_msg_arr[req_ctx->resp_msg_cnt], msg_size);
        if (NULL == msg_space) {
            SRVPP_LOG_ERR_MSG("Unable to reallocate message space.");
            return -1;
        }
        req_ctx->resp_msg_arr[req_ctx->resp_msg_cnt] = msg_space;
        req_ctx->resp_msg_sizes[req_ctx->resp_msg_cnt] = msg_size;
    }

    /* copy the message content */
    memcpy(req_ctx->resp_msg_arr[req_ctx->resp_msg_cnt], msg, msg_size);

    req_ctx->resp_msg_cnt++;

    return 0;
}

/**
 * @brief Processes a reply to a single VPP request.
 */
static int
srvpp_process_reply_msg(srvpp_request_ctx_t *req_ctx, u16 msg_id, void *msg, size_t msg_size)
{
    vl_generic_reply_t *reply = NULL;
    int rc = 0;

    CHECK_NULL(req_ctx);

    reply = (vl_generic_reply_t *) msg;

    if (req_ctx->ctx_id != reply->context) {
        SRVPP_LOG_ERR_MSG("Invalid request context for provided message, ignoring the message.");
        return -1;
    }

    if (req_ctx->resp_data_copy) {
        /* copy msg data into req_ctx */
        rc = srvpp_msg_data_copy(req_ctx, msg, msg_size);
    }

    if (0 == rc) {
        req_ctx->resp_retval = reply->retval;
    } else {
        req_ctx->resp_retval = rc;
    }

    /* signal the requesting thread */
    req_ctx->resp_ready = true;
    pthread_cond_signal(&req_ctx->resp_cv);

    return rc;
}

/**
 * @brief Processes a reply to a dump request to VPP (response consisting of multiple messages).
 */
static int
srvpp_process_details_msg(srvpp_request_ctx_t *req_ctx, u16 msg_id, void *msg, size_t msg_size)
{
    vl_generic_response_t *response = NULL;
    int rc = 0;

    CHECK_NULL(req_ctx);

    response = (vl_generic_response_t *) msg;

    if (req_ctx->ctx_id != response->context) {
        SRVPP_LOG_ERR_MSG("Invalid request context for provided message, ignoring the message.");
        return -1;
    }

    if (VL_API_CONTROL_PING_REPLY != msg_id) {
        /* details message - copy message data into req contex*/
        rc = srvpp_msg_data_copy(req_ctx, msg, msg_size);
        if (0 != rc && 0 == req_ctx->resp_retval) {
            /* in case of error, propagate it to the req context */
            req_ctx->resp_retval = rc;
        }
    } else {
        /* control ping reply, signal the requesting thread */
        req_ctx->resp_ready = true;
        pthread_cond_signal(&req_ctx->resp_cv);
    }

    return req_ctx->resp_retval;
}

/**
 * @brief Internal callback automatically called by VPP library when a message
 * from VPP is received.
 */
void
_srvpp_receive_msg_handler(void *msg)
{
    srvpp_request_ctx_t *req_ctx = NULL;
    vl_generic_response_t *response = NULL;
    msgbuf_t *msg_header = NULL;
    size_t msg_size = 0;
    u16 msg_id = 0;

    if (NULL == msg) {
        SRVPP_LOG_WRN_MSG("NULL message received, ignoring.");
        return;
    }

    response = (vl_generic_response_t *) msg;

    /* get message details */
    msg_header = (msgbuf_t *) (((u8 *) msg) - offsetof(msgbuf_t, data));
    msg_size = ntohl(msg_header->data_len);
    msg_id = ntohs(*((u16 *)msg));

    SRVPP_LOG_DBG("New message received from VPP (id=%d, size=%zu).", msg_id, msg_size);

    /* get request context matching with context id */
    req_ctx = srvpp_get_request_ctx(srvpp_ctx, response->context);
    if (NULL == req_ctx) {
        SRVPP_LOG_WRN("Unexpected context id=%d within the received message, ignoring.", response->context);
        return;
    }

    pthread_mutex_lock(&req_ctx->lock);

    if (SRVPP_REPLY == req_ctx->resp_type) {
        srvpp_process_reply_msg(req_ctx, msg_id, msg, msg_size);
    } else {
        srvpp_process_details_msg(req_ctx, msg_id, msg, msg_size);
    }

    pthread_mutex_unlock(&req_ctx->lock);
}

/**
 * @brief Blocks the thread until a response from VPP comes or until a timeout expires.
 */
static int
srvpp_wait_response(srvpp_request_ctx_t *req_ctx)
{
    struct timespec ts = { 0, };
    int retval = 0, rc = 0;

    CHECK_NULL(req_ctx);

    pthread_mutex_lock(&req_ctx->lock);

    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += SRVPP_RESPONSE_TIMEOUT;

    while (!req_ctx->resp_ready && (0 == rc)) {
        rc = pthread_cond_timedwait(&req_ctx->resp_cv, &req_ctx->lock, &ts);
    }
    if (0 == rc) {
        SRVPP_LOG_DBG("Received the response from VPP, retval=%d", req_ctx->resp_retval);
        retval = req_ctx->resp_retval;
    } else {
        SRVPP_LOG_ERR("Response not received from VPP within the timeout period (%d sec).", SRVPP_RESPONSE_TIMEOUT);
        retval = -1;
    }

    /* invalidate previous context id */
    ++req_ctx->ctx_id;

    pthread_mutex_unlock(&req_ctx->lock);

    return retval;
}

/**
 * @brief Connects to VPP.
 */
static int
srvpp_vlib_connect(srvpp_ctx_t *ctx)
{
    api_main_t *am = &api_main;
    int rc = 0;

    CHECK_NULL(ctx);

    SRVPP_LOG_DBG_MSG("Connecting to VPP...");

    rc = vl_client_connect_to_vlib("/vpe-api", "srvpp", 32);

    if (rc < 0) {
        SRVPP_LOG_ERR("Unable to connect to VPP, rc=%d.", rc);
    } else {
        SRVPP_LOG_DBG("Connection to VPP established, client index=%d.", am->my_client_index);
        ctx->vlib_client_index = am->my_client_index;
        ctx->vlib_input_queue = am->shmem_hdr->vl_input_queue;
    }

    return rc;
}

/**
 * @brief Disconnects from VPP.
 */
static void
srvpp_vlib_disconnect()
{
    vl_client_disconnect_from_vlib();
}

/**
 * @brief Adds a new interface into interfaces list.
 */
static int
srvpp_if_info_add(srvpp_ctx_t *ctx, const char *if_name, u32 if_index)
{
    srvpp_if_info_t *tmp = NULL, *if_info = NULL;

    SRVPP_LOG_DBG("Adding interface '%s', id=%d", if_name, if_index);

    if_info = calloc(1, sizeof(*if_info));
    if (NULL == if_info) {
        return 1;
    }

    if_info->if_name = strdup(if_name);
    if (NULL == if_info->if_name) {
        return 1;
    }

    if_info->if_index = if_index;

    if (NULL == ctx->if_info_list) {
        ctx->if_info_list = if_info;
    } else {
        tmp = ctx->if_info_list;
        while (NULL != tmp->_next) {
            tmp = tmp->_next;
        }
        tmp->_next = if_info;
    }

    return 0;
}

/**
 * @brief Loads VPP interfaces information.
 */
static int
srvpp_if_info_load(srvpp_ctx_t *ctx)
{
    vl_api_sw_interface_dump_t *if_dump_req = NULL;
    vl_api_sw_interface_details_t *if_details = NULL;
    void **details = NULL;
    size_t details_cnt = 0;
    int ret = 0;

    SRVPP_LOG_DBG_MSG("Loading VPP interfaces information");

    /* dump interfaces */
    if_dump_req = srvpp_alloc_msg(VL_API_SW_INTERFACE_DUMP, sizeof(*if_dump_req));

    ret = srvpp_send_dumprequest(ctx, if_dump_req, &details, &details_cnt);
    if (0 != ret) {
        return ret;
    }

    pthread_mutex_lock(&srvpp_ctx_lock);

    for (size_t i = 0; i < details_cnt; i++) {
        if_details = (vl_api_sw_interface_details_t *) details[i];
        ret = srvpp_if_info_add(ctx, (char*)if_details->interface_name, ntohl(if_details->sw_if_index));
    }

    pthread_mutex_unlock(&srvpp_ctx_lock);

    return 0;
}

/**
 * @brief Cleans up VPP interfaces information.
 */
static void
srvpp_if_info_cleanup(srvpp_ctx_t *ctx)
{
    srvpp_if_info_t *tmp = NULL, *if_info = NULL;

    if (NULL != ctx) {
        if_info = ctx->if_info_list;

        while (NULL != if_info) {
            tmp = if_info;
            if_info = if_info->_next;
            free((void*)tmp->if_name);
            free(tmp);
        }
    }
}

/**
 * @brief Initializes the srvpp context.
 */
srvpp_ctx_t *
srvpp_ctx_init()
{
    srvpp_ctx_t *ctx = NULL;
    int rc = 0;

    srvpp_logger_init();

    ctx = calloc(1, sizeof(*ctx));
    if (NULL == ctx) {
        SRVPP_LOG_ERR_MSG("Unable to allocate srvpp context.");
        return NULL;
    }

    rc = srvpp_vlib_connect(ctx);
    if (0 != rc) {
        SRVPP_LOG_ERR_MSG("Unable to initialize srvpp context.");
        free(ctx);
        return NULL;
    }

    while (pthread_key_create(&ctx->request_key, NULL) == EAGAIN);
    pthread_setspecific(ctx->request_key, NULL);

    ctx->ref_cnt = 1;

    SRVPP_LOG_INF_MSG("srvpp context initialized successfully.");

    return ctx;
}

/**
 * @brief Cleans up the srvpp context.
 */
static void
srvpp_ctx_cleanup(srvpp_ctx_t *ctx)
{
    srvpp_request_ctx_t *tmp = NULL, *req_ctx = NULL;

    if (NULL != ctx) {
        srvpp_vlib_disconnect();

        srvpp_if_info_cleanup(ctx);

        req_ctx = ctx->request_ctx_list;
        while (NULL != req_ctx) {
            tmp = req_ctx;
            req_ctx = req_ctx->_next;

            pthread_mutex_destroy(&tmp->lock);
            pthread_cond_destroy(&tmp->resp_cv);

            for (size_t i = 0; i < tmp->resp_msg_arr_size; i++) {
                free(tmp->resp_msg_arr[i]);
            }
            free(tmp->resp_msg_arr);
            free(tmp->resp_msg_sizes);

            free(tmp);
        }
        free(ctx);

        srvpp_logger_cleanup();

        SRVPP_LOG_INF_MSG("srvpp context cleaned up successfully.");
    }
}

srvpp_ctx_t *
srvpp_get_ctx()
{
    bool setup_handlers = false;

    pthread_mutex_lock(&srvpp_ctx_lock);

    if (NULL == srvpp_ctx) {
        /* initialize a new context */
        SRVPP_LOG_DBG_MSG("Initializing a new srvpp context.");
        srvpp_ctx = srvpp_ctx_init();
        setup_handlers = true;
    } else {
        /* increment ref. count */
        srvpp_ctx->ref_cnt++;
        SRVPP_LOG_DBG("Reusing existing srvpp context, new refcount=%zu.", srvpp_ctx->ref_cnt);
    }

    pthread_mutex_unlock(&srvpp_ctx_lock);

    if (setup_handlers) {
        /* setup required handlers */
        srvpp_setup_handler(CONTROL_PING_REPLY, control_ping_reply);
        srvpp_setup_handler(SW_INTERFACE_DETAILS, sw_interface_details);

        /* load VPP interfaces information */
        srvpp_if_info_load(srvpp_ctx);
    }

    return srvpp_ctx;
}

void
srvpp_release_ctx(srvpp_ctx_t *ctx)
{
    pthread_mutex_lock(&srvpp_ctx_lock);

    if (ctx != srvpp_ctx) {
        SRVPP_LOG_ERR_MSG("Invalid srvpp context passed in, unable to release.");
        pthread_mutex_unlock(&srvpp_ctx_lock);
        return;
    }

    if (srvpp_ctx->ref_cnt > 1) {
        /* there are still some other references */
        srvpp_ctx->ref_cnt--;
        SRVPP_LOG_DBG("Releasing a reference to srvpp context, new refcount=%zu.", srvpp_ctx->ref_cnt);
    } else {
        /* last reference - cleanup */
        SRVPP_LOG_DBG_MSG("Releasing srvpp context (last ctx reference).");
        srvpp_ctx_cleanup(srvpp_ctx);
        srvpp_ctx = NULL;
    }

    pthread_mutex_unlock(&srvpp_ctx_lock);
}

void *
srvpp_alloc_msg(uint16_t msg_id, size_t msg_size)
{
    vl_generic_request_t *req = NULL;

    req = vl_msg_api_alloc(msg_size);
    if (NULL != req) {
        memset(req, 0, msg_size);
        req->_vl_msg_id = ntohs(msg_id);
    }

    return req;
}

int
srvpp_send_request(srvpp_ctx_t *ctx, void *request, void **response)
{
    vl_generic_request_t *req = NULL;
    srvpp_request_ctx_t *req_ctx = NULL;
    int retval = 0;

    CHECK_NULL(ctx);
    CHECK_NULL(request);

    req_ctx = srvpp_get_thread_request_ctx(ctx);
    if (NULL == req_ctx) {
        SRVPP_LOG_ERR_MSG("Unable to obtain a request context.");
        return -1;
    }

    req = (vl_generic_request_t *) request;
    req->client_index = ctx->vlib_client_index;

    pthread_mutex_lock(&req_ctx->lock);

    if (NULL != response) {
        /* response data is requested */
        req_ctx->resp_data_copy = true;
        req_ctx->resp_msg_cnt = 0;
    } else {
        /* not interested in response data */
        req_ctx->resp_data_copy = false;
    }

    req_ctx->resp_type = SRVPP_REPLY;
    req_ctx->resp_ready = false;
    req_ctx->resp_retval = 0;
    req->context = ++req_ctx->ctx_id;

    pthread_mutex_unlock(&req_ctx->lock);

    SRVPP_LOG_DBG_MSG("Sending a request to VPP.");

    vl_msg_api_send_shmem(ctx->vlib_input_queue, (u8*)&req);

    /* wait for expected response */
    retval = srvpp_wait_response(req_ctx);

    if (0 == retval) {
        if (NULL != response && req_ctx->resp_msg_cnt > 0) {
            *response = req_ctx->resp_msg_arr[0];
        }
        SRVPP_LOG_DBG_MSG("VPP request successfully processed.");
    } else {
        SRVPP_LOG_ERR_MSG("Error by handling of a VPP request.");
    }

    return retval;
}

int
srvpp_send_dumprequest(srvpp_ctx_t *ctx, void *request, void ***response_arr, size_t *response_cnt)
{
    vl_generic_request_t *req = NULL;
    srvpp_request_ctx_t *req_ctx = NULL;
    vl_api_control_ping_t *ping = NULL;
    int retval = 0;

    CHECK_NULL(ctx);
    CHECK_NULL(request);
    CHECK_NULL(response_arr);
    CHECK_NULL(response_cnt);

    req_ctx = srvpp_get_thread_request_ctx(ctx);
    if (NULL == req_ctx) {
        SRVPP_LOG_ERR_MSG("Unable to obtain a request context.");
        return -1;
    }

    req = (vl_generic_request_t *) request;
    req->client_index = ctx->vlib_client_index;

    /* allocate control ping request */
    ping = srvpp_alloc_msg(VL_API_CONTROL_PING, sizeof(*ping));
    if (NULL == ping) {
        SRVPP_LOG_ERR_MSG("Unable to allocate control ping message.");
        return -1;
    }
    ping->client_index = ctx->vlib_client_index;

    pthread_mutex_lock(&req_ctx->lock);

    req_ctx->resp_data_copy = true;
    req_ctx->resp_msg_cnt = 0;

    req_ctx->resp_type = SRVPP_DETAILS;
    req_ctx->resp_ready = false;
    req_ctx->resp_retval = 0;

    req_ctx->ctx_id++;
    req->context = req_ctx->ctx_id;
    ping->context = req_ctx->ctx_id;

    pthread_mutex_unlock(&req_ctx->lock);

    SRVPP_LOG_DBG_MSG("Sending a dumprequest to VPP.");

    vl_msg_api_send_shmem(ctx->vlib_input_queue, (u8*)&req);

    vl_msg_api_send_shmem(ctx->vlib_input_queue, (u8*)&ping);

    /* wait for expected response */
    retval = srvpp_wait_response(req_ctx);

    if (0 == retval) {
        *response_arr = req_ctx->resp_msg_arr;
        *response_cnt = req_ctx->resp_msg_cnt;
        SRVPP_LOG_DBG_MSG("VPP dumprequest successfully processed.");
    } else {
        SRVPP_LOG_ERR_MSG("Error by handling of a VPP dumprequest.");
    }

    return retval;
}

int
srvpp_get_if_index(srvpp_ctx_t *ctx, const char *if_name, uint32_t *if_index)
{
    CHECK_NULL(if_name);
    CHECK_NULL(if_index);

    srvpp_if_info_t *if_info = NULL;

    if_info = ctx->if_info_list;

    while ((NULL != if_info) && (NULL != if_info->if_name)) {
        if (0 == strcmp(if_info->if_name, if_name)) {
            *if_index = if_info->if_index;
            return 0;
        }
        if_info = if_info->_next;
    }

    return 1;
}
