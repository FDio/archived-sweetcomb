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

#ifndef _SCVPP_H_
#define _SCVPP_H_

#include <vapi/vpe.api.vapi.h>

#include <sysrepo.h>
#include <sysrepo/values.h>
#include <sysrepo/plugins.h>   //for SC_LOG_DBG

///////////////////////////

//SCVPP接口
int sc_connect_vpp();
int sc_disconnect_vpp();

extern vapi_ctx_t app_ctx; //for all plugins to use single vpp session
///////////////////////////
//SCVPP工具
int sc_end_with(const char* str, const char* end);

#define VPP_INTFC_NAME_LEN 64
#define VPP_TAP_NAME_LEN VPP_INTFC_NAME_LEN
#define VPP_IP4_ADDRESS_LEN 4
#define VPP_IP6_ADDRESS_LEN 16
#define VPP_IP4_ADDRESS_STRING_LEN 16
#define VPP_IP6_ADDRESS_STRING_LEN 46
#define VPP_MAC_ADDRESS_LEN 8
#define VPP_TAG_LEN VPP_INTFC_NAME_LEN
#define VPP_IKEV2_PROFILE_NAME_LEN VPP_INTFC_NAME_LEN
#define VPP_IKEV2_PSK_LEN VPP_INTFC_NAME_LEN
#define VPP_IKEV2_ID_LEN 32


#ifndef SC_FUNCNAME
#ifdef __FUNCTION__
#define SC_FUNCNAME __FUNCTION__
#else
#define SC_FUNCNAME __func__
#endif
#endif

#ifndef SC_NOLOG
#define SC_LOG_DBG SRP_LOG_DBG
#define SC_LOG_ERR SRP_LOG_ERR
#define SC_LOG_DBG_MSG SRP_LOG_DBG_MSG
#define SC_LOG_ERR_MSG SRP_LOG_ERR_MSG
#else
#define SC_LOG_DBG //SRP_LOG_DBG
#define SC_LOG_ERR //SRP_LOG_ERR
#define SC_LOG_DBG_MSG //SRP_LOG_DBG_MSG
#define SC_LOG_ERR_MSG //SRP_LOG_ERR_MSG
#endif

#define SC_INVOKE_BEGIN SC_LOG_DBG("inovke %s begin.",SC_FUNCNAME);
#define SC_INVOKE_END   SC_LOG_DBG("inovke %s end,with return ok.",SC_FUNCNAME);
#define SC_INVOKE_ENDX(...)  SC_LOG_DBG("inovke %s end,with %s.",SC_FUNCNAME, ##__VA_ARGS__)

#define SC_SETUP_RPC_EVT_HANDLER(evt_handler) \
do { \
	sr_error_t rc = evt_handler(session, &subscription); \
	if (SR_ERR_OK != rc) \
	{ \
		SC_LOG_ERR("load plugin failed: %s", sr_strerror(rc)); \
		sr_unsubscribe(session, subscription); \
		SC_INVOKE_ENDX(sr_strerror(rc)); \
		return rc; \
	} \
} while(0);

#define SC_DEFINE_SR_PLUGIN_CLEANUP_CB_IMPL(PLUGIN_NAME) \
void sr_plugin_cleanup_cb (sr_session_ctx_t *session, void *private_ctx) \
{ \
	SC_INVOKE_BEGIN; \
	/* subscription was set as our private context */ \
	sr_unsubscribe(session, private_ctx); \
	SC_LOG_DBG("do plugin [%s] cleanup ok.",PLUGIN_NAME); \
	sc_disconnect_vpp(); \
	SC_LOG_DBG_MSG("plugin disconnect scvpp ok."); \
	SC_INVOKE_END; \
}

#endif //_SCVPP_H_
