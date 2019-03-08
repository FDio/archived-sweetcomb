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

#include "sys_util.h"
#include "sc_vpp_comm.h"

#include <string.h>
#include <sysrepo/plugins.h>

char* xpath_find_first_key(const char *xpath, char *key, sr_xpath_ctx_t *state)
{
    char *value = NULL;

    if (sr_xpath_next_node((char *)xpath, state) == NULL) {
        return NULL;
    }

    while (((value = sr_xpath_node_key_value(NULL,
            key, state)) == NULL) &&
            (sr_xpath_next_node(NULL, state) != NULL));

    return value;
}

// we can call sr_get_item after change is called to see if the value has or has not
// changed (simpleton solution :D)
// sr_get_item(sess, "/ietf-interfaces:interfaces/interface[name='eth0']/enabled", &value);

void log_recv_event(sr_notif_event_t event, const char *msg) {
    const char *event_s;
    switch (event) {
        case SR_EV_VERIFY: event_s = "SR_EV_VERIFY"; break;
        case SR_EV_APPLY: event_s = "SR_EV_APPLY"; break;
        case SR_EV_ABORT: event_s = "SR_EV_ABORT"; break;
        default: event_s = "SR_EV_UNKNOWN";
    }
    SRP_LOG_DBG("%s: %s\n", msg, event_s);
}

void log_recv_oper(sr_change_oper_t oper, const char *msg) {
    const char *oper_s;
    switch (oper) {
        case SR_OP_CREATED: oper_s = "SR_OP_CREATED"; break;
        case SR_OP_DELETED: oper_s = "SR_OP_DELETED"; break;
        case SR_OP_MODIFIED: oper_s = "SR_OP_MODIFIED"; break;
        case SR_OP_MOVED: oper_s = "SR_OP_MOVED"; break;
        default: oper_s = "SR_OP_UNKNOWN"; break;
    }
    SRP_LOG_DBG("%s: %s\n", msg, oper_s);
}

int ip_prefix_split(const char* ip_prefix)
{
    //find the slash
    char* slash = strchr(ip_prefix, '/');
    if (NULL == slash)
        return -1;

    //extract subnet mask length
    char * eptr = NULL;
    u8 mask = strtoul(slash + 1, &eptr, 10);
    if (*eptr || mask <= 0)
        return -1;

    //keep just the address part
    *slash = '\0';         //replace '/' with 0

    //return mask length
    return mask;
}
