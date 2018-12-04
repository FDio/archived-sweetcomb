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

#ifndef __SYS_UTIL_H__
#define __SYS_UTIL_H__

#include <sysrepo.h>
#include <sysrepo/xpath.h>

#define XPATH_SIZE 2000

typedef struct
{
    char xpath_root[XPATH_SIZE];
    sr_val_t * values;
    size_t values_cnt;
} sysr_values_ctx_t;

char* xpath_find_first_key(const char *xpath, char *key, sr_xpath_ctx_t *state);
void log_recv_event(sr_notif_event_t event, const char *msg);
void log_recv_oper(sr_change_oper_t oper, const char *msg);
int ip_prefix_split(const char* ip_prefix);

#endif /* __SYS_UTIL_H__ */
