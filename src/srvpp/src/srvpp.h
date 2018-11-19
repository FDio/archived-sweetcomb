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

#ifndef INC_SRVPP_H_
#define INC_SRVPP_H_

/**
 * @defgroup srvpp Sysrepo-VPP Integration Library
 * @{
 *
 * @brief Provides synchronous interface to VPP binary API aimed primarily for
 * the integration of VPP with Sysrepo datastore.
 *
 * The library is thread-safe - can be used to communicate with VPP from multiple
 * threads simultaneously.
 */

#include <stddef.h>

#include <vlibapi/api.h>
#include <vlibmemory/api.h>
#include <vapi/vpe.api.vapi.h>

#include <vpp/api/vpe_msg_enum.h>

#define vl_typedefs             /* define message structures */
#include <vpp/api/vpe_all_api_h.h>
#undef vl_typedefs

#define vl_endianfun
#include <vpp/api/vpe_all_api_h.h>
#undef vl_endianfun

/* instantiate all the print functions we know about */
#define vl_print(handle, ...)
#define vl_printfun
#include <vpp/api/vpe_all_api_h.h>
#undef vl_printfun

/**
 * @brief Sysrepo - VPP interface context.
 */
typedef struct srvpp_ctx_s srvpp_ctx_t;

/**
 * @brief srvpp logger severity levels.
 */
typedef enum srvpp_log_level_e {
    SRVPP_NONE,  /**< Do not print any messages. */
    SRVPP_ERR,   /**< Print only error messages. */
    SRVPP_WRN,   /**< Print error and warning messages. */
    SRVPP_INF,   /**< Besides errors and warnings, print some other informational messages. */
    SRVPP_DBG,   /**< Print all messages including some development debug messages. */
} srvpp_log_level_t;

/**
 * @brief Sets callback that will be called when a log entry would be populated.
 *
 * @param[in] level Severity level of the log entry.
 * @param[in] message Message of the log entry.
 */
typedef void (*srvpp_log_cb)(srvpp_log_level_t level, const char *message);

/**
 * @brief Returns a reference to the global srvpp context. If the global context
 * does not exists yet, it will be automatically created (a connection to VPP
 * will be established).
 *
 * @note The caller is supposed to call ::srvpp_release_ctx after it finishes
 * the work with VPP.
 *
 * @return srvpp context to be used for subsequent API calls.
 */
srvpp_ctx_t* srvpp_get_ctx();

/**
 * @brief Releases a reference to the global srvpp context. If this is the last
 * reference, the context will be released (the connection to the VPP will be closed).
 *
 * @param[in] ctx srvpp context acquired using ::srvpp_get_ctx call.
 */
void srvpp_release_ctx(srvpp_ctx_t *ctx);

/**
 * @brief Allocates a VPP API message of specified type and size.
 *
 * @param[in] msg_id Message ID.
 * @param[in] msg_size Size of the message.
 *
 * @return Space allocated for the message.
 */
void* srvpp_alloc_msg(uint16_t msg_id, size_t msg_size);

/**
 * @brief Sends a simple request to VPP and receive the response.
 *
 * @note Subsequent ::srvpp_send_request or ::srvpp_send_dumprequest calls from the same
 * thread may overwrite the content of ::response, therefore it is not safe to call
 * them until the response is fully consumed / processed by the caller.
 *
 * @param[in] ctx srvpp context acquired using ::srvpp_get_ctx call.
 * @param[in] request Message with the request to be sent to VPP.
 * @param[out] response (optional) Response to the request received from VPP.
 * Caller must not free it - to save memory allocations, response is always
 * placed into the same thread-local buffer owned by the library.
 *
 * @return 0 on success, non-zero in case of error.
 */
int srvpp_send_request(srvpp_ctx_t *ctx, void *request, void **response);

/**
 * @brief Sends a dump request to VPP and receives all responses with details.
 *
 * @note Subsequent ::srvpp_send_request or ::srvpp_send_dumprequest calls from the same
 * thread may overwrite the content of ::response_arr, therefore it is not safe to call
 * them until the response array is fully consumed / processed by the caller.
 *
 * @param[in] ctx srvpp context acquired using ::srvpp_get_ctx call.
 * @param[in] request Message with the request to be sent to VPP.
 * @param[out] response_arr Array of responses to the request received from VPP.
 * Caller must not free it - to save memory allocations, response_arr is always
 * placed into the same thread-local buffer owned by the library.
 * @param[out] response_cnt Count of the response messages in the response_arr array.
 *
 * @return 0 on success, non-zero in case of error.
 */
int srvpp_send_dumprequest(srvpp_ctx_t *ctx, void *request, void ***response_arr, size_t *response_cnt);

/**
 * @brief Get interface index for provided interface name.
 *
 * @param[in] ctx srvpp context acquired using ::srvpp_get_ctx call.
 * @param[in] if_name Name of an existing VPP interface.
 * @param[out] if_index Index of the interface.
 *
 * @return 0 on success, non-zero in case of error.
 */
int srvpp_get_if_index(srvpp_ctx_t *ctx, const char *if_name, uint32_t *if_index);

/**
 * @brief Sets callback that will be called when a log entry would be populated.
 * Callback will be called for each message with any log level.
 *
 * @param[in] log_callback Callback to be called when a log entry would populated.
 */
void srvpp_set_log_cb(srvpp_log_cb log_callback);

/**
 * @brief Setups message handler to provided VPP API call. Needs to be called
 * for each VPP API function that can arrive as response from VPP.
 *
 * @param[in] ID VPP API function ID.
 * @param[in] NAME VPP API function name.
 */
#define srvpp_setup_handler(ID, NAME)                                  \
    do {                                                               \
        vl_msg_api_set_handlers(VL_API_##ID, #NAME,                    \
                               (void*)_srvpp_receive_msg_handler,      \
                               (void*)vl_noop_handler,                 \
                               (void*)vl_api_##NAME##_t_endian,        \
                               (void*)vl_api_##NAME##_t_print,         \
                               sizeof(vl_api_##NAME##_t), 1);          \
    } while(0)

/**
 * @brief Internal callback automatically called by VPP library when a message
 * from VPP is received.
 */
void _srvpp_receive_msg_handler(void *message);

/**@} srvpp */

#endif /* INC_SRVPP_H_ */
