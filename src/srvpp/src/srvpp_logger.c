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
#include <pthread.h>

#include "srvpp_logger.h"

#define MAX_LOG_MSG_SIZE 2048  /**< Maximum size of one log entry. */

volatile srvpp_log_cb srvpp_log_callback = NULL;   /**< Global variable used to store logging callback, if set. */

static pthread_once_t srvpp_log_buff_create_key_once = PTHREAD_ONCE_INIT;  /** Used to control that ::srvpp_log_buff_create_key is called only once per thread. */
static pthread_key_t srvpp_log_buff_key;  /**< Key for thread-specific buffer data. */

/**
 * @brief Create key for thread-specific buffer data. Should be called only once per thread.
 */
static void
srvpp_log_buff_create_key(void)
{
    while (pthread_key_create(&srvpp_log_buff_key, free) == EAGAIN);
    pthread_setspecific(srvpp_log_buff_key, NULL);
}

void
srvpp_logger_init()
{
    /* no init needed */
}

void
srvpp_logger_cleanup()
{
    /* since the thread-specific data for the main thread seems to be not auto-freed,
     * (at least on Linux), we explicitly release it here for the thread from which
     * sr_logger_cleanup has been called (which should be always the main thread). */
    pthread_once(&srvpp_log_buff_create_key_once, srvpp_log_buff_create_key);
    char *msg_buff = pthread_getspecific(srvpp_log_buff_key);
    if (NULL != msg_buff) {
        free(msg_buff);
        pthread_setspecific(srvpp_log_buff_key, NULL);
    }
}

void
srvpp_log_to_cb(srvpp_log_level_t level, const char *format, ...)
{
    char *msg_buff = NULL;
    va_list arg_list;

    if (NULL != srvpp_log_callback) {
        /* get thread-local message buffer */
        pthread_once(&srvpp_log_buff_create_key_once, srvpp_log_buff_create_key);
        msg_buff = pthread_getspecific(srvpp_log_buff_key);
        if (NULL == msg_buff) {
            msg_buff = calloc(MAX_LOG_MSG_SIZE, sizeof(*msg_buff));
            pthread_setspecific(srvpp_log_buff_key, msg_buff);
        }
        /* print the message into buffer and call callback */
        if (NULL != msg_buff) {
            va_start(arg_list, format);
            vsnprintf(msg_buff, MAX_LOG_MSG_SIZE - 1, format, arg_list);
            va_end(arg_list);
            msg_buff[MAX_LOG_MSG_SIZE - 1] = '\0';
            /* call the callback */
            srvpp_log_callback(level, msg_buff);
        }
    }
}

