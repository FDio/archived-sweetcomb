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

#ifndef SRC_SRVPP_LOGGER_H_
#define SRC_SRVPP_LOGGER_H_

#include "srvpp.h"

/**
 * @brief Pointer to logging callback, if set.
 */
extern volatile srvpp_log_cb srvpp_log_callback;

/**
 * @brief Returns the string representing the specified log level.
 */
#define _SRVPP_LL_STR(LL) \
    ((SRVPP_ERR == LL) ? "ERR" : \
     (SRVPP_WRN == LL) ? "WRN" : \
     (SRVPP_INF == LL) ? "INF" : \
     (SRVPP_DBG == LL) ? "DBG" : \
      "")

/**
 * @brief Internal logging macro for formatted messages.
 */
#ifdef NDEBUG
#define _SRVPP_LOG_FMT(LL, MSG, ...) \
    do { \
        if (NULL != srvpp_log_callback) { \
            srvpp_log_to_cb(LL, MSG, __VA_ARGS__); \
        } else { \
            fprintf(stderr, "[%s] " MSG "\n", _SRVPP_LL_STR(LL), __VA_ARGS__); \
        } \
} while(0)
#else
#define _SRVPP_LOG_FMT(LL, MSG, ...) \
    do { \
        if (NULL != srvpp_log_callback) { \
            srvpp_log_to_cb(LL, "(%s:%d) " MSG, __func__, __LINE__, __VA_ARGS__); \
        } else { \
            fprintf(stderr, "[%s] (%s:%d) " MSG "\n", _SRVPP_LL_STR(LL), __func__, __LINE__, __VA_ARGS__); \
        } \
} while(0)
#endif

/* Logging macros for formatted messages. */
#define SRVPP_LOG_ERR(MSG, ...) _SRVPP_LOG_FMT(SRVPP_ERR, MSG, __VA_ARGS__);
#define SRVPP_LOG_WRN(MSG, ...) _SRVPP_LOG_FMT(SRVPP_WRN, MSG, __VA_ARGS__);
#define SRVPP_LOG_INF(MSG, ...) _SRVPP_LOG_FMT(SRVPP_INF, MSG, __VA_ARGS__);
#define SRVPP_LOG_DBG(MSG, ...) _SRVPP_LOG_FMT(SRVPP_DBG, MSG, __VA_ARGS__);

/* Logging macros for unformatted messages. */
#define SRVPP_LOG_ERR_MSG(MSG) _SRVPP_LOG_FMT(SRVPP_ERR, MSG "%s", "");
#define SRVPP_LOG_WRN_MSG(MSG) _SRVPP_LOG_FMT(SRVPP_WRN, MSG "%s", "");
#define SRVPP_LOG_INF_MSG(MSG) _SRVPP_LOG_FMT(SRVPP_INF, MSG "%s", "");
#define SRVPP_LOG_DBG_MSG(MSG) _SRVPP_LOG_FMT(SRVPP_DBG, MSG "%s", "");

/**
 * @brief Initializes logger.
 */
void srvpp_logger_init();

/**
 * @brief Cleans up resources used by he logger.
 */
void srvpp_logger_cleanup();

/**
 * @brief Logs into the callback pre-specified by ::srvpp_set_log_cb.
 *
 * @param[in] level Log level.
 * @param[in] format Format message.
 */
void srvpp_log_to_cb(srvpp_log_level_t level, const char *format, ...);

#endif /* SRC_SRVPP_LOGGER_H_ */
