/*
 * Copyright (c) 2019 PANTHEON.tech.
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

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <syslog.h>

#define ERROR(fmt, ...) \
        do { syslog(LOG_ERR, "%s:%d:%s():[ERROR]:" fmt "\n", __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while (0)

#define DEBUG(fmt, ...) \
        do { syslog(LOG_DEBUG, "%s:%d:%s():[DEBUG]:" fmt "\n", __FILE__, \
            __LINE__, __func__, ##__VA_ARGS__); } while (0)

void init_log(int option);

void close_log();

#endif
