/*
 * Copyright (c) 2018 HUACHENTEL and/or its affiliates.
 * Copyright (c) 2019 Cisco and/or its affiliates.
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

#include "sc_plugins.h"

#include <dirent.h>
#include <string.h>
#include <errno.h>

#include <vom/hw.hpp>
#include <vom/om.hpp>

static int vpp_pid_start;

sc_plugin_main_t sc_plugin_main;

using namespace VOM;

#define _DIRENT_NAME 256
#define CMDLINE_MAX _DIRENT_NAME + 15
#define VPP_FULL_PATH "/usr/bin/vpp"

sc_plugin_main_t *sc_get_plugin_main()
{
    return &sc_plugin_main;
}

/**
 * @brief get one pid of any running vpp process.
 * @return Return vpp pid or negative value if error.
 */
int get_vpp_pid()
{
    DIR *dir;
    struct dirent *ptr;
    FILE *fp;
    char filepath[CMDLINE_MAX];
    char filetext[strlen(VPP_FULL_PATH)];
    char *first = NULL;
    size_t cnt;

    dir = opendir("/proc");
    if (dir == NULL)
        return -errno;

    /* read vpp pid file in proc, return pid of vpp */
    while (NULL != (ptr = readdir(dir)))
    {
        if ((0 == strcmp(ptr->d_name, ".")) || (0 == strcmp(ptr->d_name, "..")))
            continue;

        if (DT_DIR != ptr->d_type)
            continue;

        /* Open cmdline of PID */
        snprintf(filepath, CMDLINE_MAX, "/proc/%s/cmdline", ptr->d_name);
        fp = fopen(filepath, "r");
        if (fp == NULL)
            continue;

        /* Write '/0' char in filetext array to prevent stack reading */
        bzero(filetext, strlen(VPP_FULL_PATH));

        /* Read the string written in cmdline file */
        cnt = fread(filetext, sizeof(char), sizeof(VPP_FULL_PATH), fp);
        if (cnt == 0)
            continue;
        filetext[cnt] = '\0';

        /* retrieve string before first space */
        first = strtok(filetext, " ");
        if (first == NULL) //unmet space delimiter
            continue;

        /* One VPP process has been found */
        if (!strcmp(first, "vpp") || !strcmp(first, VPP_FULL_PATH)) {
            fclose(fp);
            closedir(dir);
            return atoi(ptr->d_name);
        }

        fclose(fp);
    }

    closedir(dir);

    return -ESRCH;
}


int sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx)
{
    int rc;

    sc_plugin_main.session = session;

    HW::init();
    OM::init();

    SRP_LOG_INF_MSG("Connect to VPP");

    while (HW::connect() != true);

    SRP_LOG_INF_MSG("Success connect to VPP");

    OM::populate("boot");

    rc = sc_call_all_init_function(&sc_plugin_main);
    if (rc != SR_ERR_OK) {
        SRP_LOG_ERR("Call all init function error: %d", rc);
        return rc;
    }

    /* set subscription as our private context */
    *private_ctx = sc_plugin_main.subscription;
    /* get the vpp pid sweetcomb connected, we assumed that only one vpp is run in system */
    vpp_pid_start = get_vpp_pid();
    if (0 > vpp_pid_start)
    {
        return SR_ERR_DISCONNECT;
    }

    return SR_ERR_OK;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_ctx)
{
   sc_call_all_exit_function(&sc_plugin_main);

    /* subscription was set as our private context */
    if (private_ctx != NULL)
        sr_unsubscribe(session, (sr_subscription_ctx_t*) private_ctx);
    SRP_LOG_DBG_MSG("unload plugin ok.");

    HW::disconnect();
    SRP_LOG_DBG_MSG("plugin disconnect vpp ok.");
}

int sr_plugin_health_check_cb(sr_session_ctx_t *session, void *private_ctx)
{
   /* health check, will use shell to detect vpp when plugin is loaded */
   /* health_check will run every 10 seconds in loop*/
   int vpp_pid_now = get_vpp_pid();

   if(vpp_pid_now != vpp_pid_start)
   {
       SRP_LOG_DBG_MSG("vpp is down.\ntry connect to vpp.");
       HW::disconnect();
       while (HW::connect() != true)
       {
           SRP_LOG_DBG_MSG("try connect to vpp.");
       };

       SRP_LOG_DBG_MSG("connect to vpp");
       OM::replay();

       vpp_pid_start = get_vpp_pid();
   }

   return SR_ERR_OK;
}
