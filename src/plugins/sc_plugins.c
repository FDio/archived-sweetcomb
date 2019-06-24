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
#include "sys_util.h"
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <vom/hw.hpp>
#include <vom/om.hpp>

static int vpp_pid_start;
bool export_backup = false;

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

#define RESTORECMDLINE_MAX (79 + _DIRENT_NAME + _DIRENT_NAME)

static int restore_backup()
{
    DIR *dir;
    struct dirent *ptr;
    char module_name[_DIRENT_NAME] = {0};
    int rc = 0;
    char cmd[RESTORECMDLINE_MAX];
    const char *pos = NULL;

    dir = opendir(BACKUP_DIR_PATH);
    if (NULL == dir)
    {
        if (ENOENT != errno && ENOTDIR != errno) {
            SRP_LOG_ERR("Failed read directory: %s, errno: %u, %s",
                        BACKUP_DIR_PATH, errno, strerror(errno));
            return -1;
        }

        export_backup = true;
        return 0;
    }

    while (NULL != (ptr = readdir(dir)))
    {
        if ((0 == strcmp(ptr->d_name, ".")) || (0 == strcmp(ptr->d_name, "..")))
        {
            continue;
        }

        if (DT_REG != ptr->d_type)
        {
            continue;
        }

        pos = strstr(ptr->d_name, ".json");
        if (!pos) {
            continue;
        }

        strncpy(module_name, ptr->d_name, pos - ptr->d_name);

        snprintf(cmd, sizeof(cmd), "/usr/bin/sysrepocfg --format=json -i "\
        "/tmp/sweetcomb/%s --datastore=running %s &", ptr->d_name,
        module_name);

        rc = system(cmd);
        if (0 != rc) {
            SRP_LOG_ERR("Failed restore backup for module: %s, errno: %s",
                        module_name, strerror(errno));
            break;
        }
    }

    closedir(dir);

    export_backup = true;

    return rc;
}

int sr_plugin_init_cb(sr_session_ctx_t *session, void **private_ctx)
{
    int rc = 0;

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

    rc = restore_backup();
    if (0 != rc) {
        SRP_LOG_DBG_MSG("Backup restore failed");
        return SR_ERR_INIT_FAILED;
    }

    SRP_LOG_DBG_MSG("Backup successfully restore");

    return SR_ERR_OK;
}

static int remove_directory(const char *path)
{
    DIR *dir = opendir(path);
    struct dirent *ptr;
    size_t path_len = strlen(path);
    char *buf;
    size_t len;
    int rc = 0;

    if (NULL == dir)
    {
        if (ENOTDIR != errno) {
            SRP_LOG_ERR("Failed read directory: %s, errno: %s",
                        BACKUP_DIR_PATH, strerror(errno));
            return -1;
        }

        return 0;
    }

    while (NULL != (ptr = readdir(dir)))
    {
        if ((0 == strcmp(ptr->d_name, ".")) ||(0 == strcmp(ptr->d_name, "..")))
        {
            continue;
        }

        len = path_len + strlen(ptr->d_name) + 2;
        buf = (char *) malloc(len * sizeof(char));
        if (NULL == buf)
        {
            SRP_LOG_ERR("Out off memory, errno: %s", strerror(errno));
            return SR_ERR_NOMEM;
        }

        snprintf(buf, len, "%s/%s", path, ptr->d_name);

        if (DT_DIR == ptr->d_type)
        {
            remove_directory(buf);
        }

        rc = remove(buf);
        if (0 != rc)
        {
            SRP_LOG_ERR("failed remove file: %s, error: %s.",
                        buf, strerror(errno));
        }

        free(buf);
    }

    closedir(dir);

    rc = remove(path);
    if (0 == rc)
    {
        SRP_LOG_DBG("remove directory: %s.", path);
    }
        else
    {
        SRP_LOG_ERR("failed remove directory: %s, error: %s.",
                    path, strerror(errno));
    }

    return rc;
}

void sr_plugin_cleanup_cb(sr_session_ctx_t *session, void *private_ctx)
{
    int rc = 0;
    struct stat st = {0};

    sc_call_all_exit_function(&sc_plugin_main);

    /* subscription was set as our private context */
    if (private_ctx != NULL)
        sr_unsubscribe(session, (sr_subscription_ctx_t*) private_ctx);
    SRP_LOG_DBG_MSG("unload plugin ok.");

    if (0 == stat(BACKUP_DIR_PATH, &st)) {
        rc = remove_directory(BACKUP_DIR_PATH);
        if (0 == rc) {
            SRP_LOG_DBG_MSG("remove backup ok.");
        } else {
            SRP_LOG_ERR_MSG("failed remove backup.");
        }
    }

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
