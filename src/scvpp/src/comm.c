/*
 * Copyright (c) 2018 HUACHENTEL and/or its affiliates.
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
#include <scvpp/comm.h>

#include <stdio.h>
#include <dirent.h>

#define APP_NAME "sweetcomb_vpp"
#define MAX_OUTSTANDING_REQUESTS 4
#define RESPONSE_QUEUE_SIZE 2

sc_vpp_main_t sc_vpp_main = {
	.vapi_ctx = NULL,
	.vapi_mode = VAPI_MODE_BLOCKING,
	.pid = 0
};

sc_vpp_main_t *sc_connect_vpp()
{
	vapi_error_e rv;

	if (sc_vpp_main.vapi_ctx == NULL) {
		if ((rv = vapi_ctx_alloc(&sc_vpp_main.vapi_ctx)) != VAPI_OK) {
			return NULL;
		}

		if ((rv =
		     vapi_connect(sc_vpp_main.vapi_ctx, APP_NAME, NULL,
				  MAX_OUTSTANDING_REQUESTS, RESPONSE_QUEUE_SIZE,
				  sc_vpp_main.vapi_mode, true)) != VAPI_OK) {
			vapi_ctx_free(sc_vpp_main.vapi_ctx);
			sc_vpp_main.vapi_ctx = NULL;
			return NULL;
		}
	}

	sc_vpp_main.pid = sc_get_vpp_pid();
	pthread_mutex_init(&sc_vpp_main.vapi_lock, NULL);
	return &sc_vpp_main;
}

void sc_disconnect_vpp()
{
	if (NULL != sc_vpp_main.vapi_ctx) {
		pthread_mutex_destroy(&sc_vpp_main.vapi_lock);
		sc_vpp_main.pid = 0;
		vapi_disconnect(sc_vpp_main.vapi_ctx);
		vapi_ctx_free(sc_vpp_main.vapi_ctx);
		sc_vpp_main.vapi_ctx = NULL;
	}
}

/* get vpp pid in system */
pid_t sc_get_vpp_pid()
{
	DIR *dir;
	struct dirent *ptr;
	FILE *fp;
	char cmdline_path[PATH_MAX];
	char cmdline_data[PATH_MAX];
	const char vpp_path[] = "/usr/bin/vpp";

	dir = opendir("/proc");
	pid_t pid = 0;
	/* read vpp pid file in proc, return pid of vpp */
	if (NULL != dir) {
		while (NULL != (ptr = readdir(dir))) {
			if ((0 == strcmp(ptr->d_name, "."))
			    || (0 == strcmp(ptr->d_name, ".."))) {
				continue;
			}

			if (DT_DIR != ptr->d_type) {
				continue;
			}

			sprintf(cmdline_path, "/proc/%s/cmdline", ptr->d_name);
			fp = fopen(cmdline_path, "r");

			if (NULL != fp) {
				fread(cmdline_data, 1, sizeof(vpp_path), fp);
				cmdline_data[sizeof(vpp_path) - 1] = '\0';

				if (cmdline_data ==
				    strstr(cmdline_data, "/usr/bin/vpp")) {
					pid = atoi(ptr->d_name);
				}

				fclose(fp);
			}
		}
		closedir(dir);
	}

	return pid;
}

int sc_end_with(const char* str, const char* end)
{
    if (str != NULL && end != NULL)
    {
        int l1 = strlen(str);
        int l2 = strlen(end);
        if (l1 >= l2)
        {
            if (strcmp(str + l1 - l2, end) == 0)
                return 1;
        }
    }
    return 0;
}

int sc_aton(const char *cp, u8 * buf, size_t length)
{
    ARG_CHECK2(false, cp, buf);

    struct in_addr addr;
    int ret = inet_aton(cp, &addr);

    if (0 == ret)
        return -EINVAL;

    if (sizeof(addr) > length)
        return -EINVAL;

    memcpy(buf, &addr, sizeof (addr));

    return 0;
}

char* sc_ntoa(const u8 * buf)
{
    ARG_CHECK(NULL, buf);

    struct in_addr addr;
    memcpy(&addr, buf, sizeof(addr));
    return inet_ntoa(addr);
}

int sc_pton(int af, const char *cp, u8 * buf)
{
    ARG_CHECK2(false, cp, buf);

    int ret = inet_pton(af, cp, buf);

    if (0 == ret)
        return -EINVAL;

    return 0;
}

const char* sc_ntop(int af, const u8 * buf, char *addr)
{
    ARG_CHECK(NULL, buf);
    ARG_CHECK(NULL, addr);

    socklen_t size = 0;
    if (af == AF_INET)
        size = INET_ADDRSTRLEN;
    else if (af == AF_INET6)
        size = INET6_ADDRSTRLEN;
    else
        return NULL;

    return inet_ntop(af, (void*)buf, addr, size);
}

/**
 * @brief Function converts the u8 array from network byte order to host byte order.
 *
 * @param[in] host IPv4 address.
 * @return host byte order value.
 */
uint32_t hardntohlu32(uint8_t host[4])
{
    uint32_t tmp = host[3] | host[2] | host[1] | host[0];

    return ntohl(tmp);
}
