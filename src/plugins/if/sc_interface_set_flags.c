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

#include "sc_interface.h"

#define SC_INT_SET_FLAGS_SW_IF_INDEX   "/sc-interface-set-flags:sc-interface-set-flags/sw-if-index"
#define SC_INT_SET_FLAGS_ADMIN_UP_DOWN "/sc-interface-set-flags:sc-interface-set-flags/admin-up-down"

static i32
sc_interface_set_flags (u32 sw_if_index, u8 admin_up_down);
static int
sc_interface_set_flags_cb (sr_session_ctx_t *session,
						   const char *module_name,
						   sr_notif_event_t event,
						   void *private_ctx);

int
sc_interface_set_flags_events (sr_session_ctx_t *session,
							   sr_subscription_ctx_t **subscription)
{
	SC_INVOKE_BEGIN;
	int rc = SR_ERR_OK;
	rc = sr_module_change_subscribe(session, "sc-interface-set-flags",
									sc_interface_set_flags_cb, NULL, 0,
									SR_SUBSCR_EV_ENABLED | SR_SUBSCR_APPLY_ONLY | SR_SUBSCR_CTX_REUSE,
									subscription);
	if (SR_ERR_OK != rc)
	{
		SC_INVOKE_ENDX(sr_strerror(rc));
	}
	else
	{
		SC_INVOKE_END;
	}
	return rc;
}

int
sc_interface_set_flags_cb (sr_session_ctx_t *session, const char *module_name,
						   sr_notif_event_t event, void *private_ctx)
{
	SC_INVOKE_BEGIN;

	int rc = SR_ERR_OK;
	if (SR_EV_APPLY != event)
	{
		return rc;
	}

	//sc-interface-set-flags
	u32 sw_if_index; //the index of a interface
	u8 admin_up_down = 1; //state of a interface,default=1

	////////////////////////////////////////////////////////////////////////////
	//retrieval items
	sr_val_t *val = NULL;

	rc = sr_get_item(session, SC_INT_SET_FLAGS_SW_IF_INDEX, &val);
	if (rc != SR_ERR_OK)
	{
		goto sr_error;
	}
	sw_if_index = val->data.uint32_val;
	sr_free_val(val);
	val = NULL;

	rc = sr_get_item(session, SC_INT_SET_FLAGS_ADMIN_UP_DOWN, &val);
	if (rc == SR_ERR_OK)
	{
		admin_up_down = val->data.uint8_val;
		sr_free_val(val);
		val = NULL;
	}

	SC_LOG_DBG("After [%s] Retrieves:", SC_FUNCNAME);
	SC_LOG_DBG("         sw_if_index: [%d]", sw_if_index);
	SC_LOG_DBG("       admin_up_down: [%d]", admin_up_down);

	sc_interface_set_flags(sw_if_index, admin_up_down);

	SC_INVOKE_END;
	return SR_ERR_OK;
sr_error:
	SC_LOG_ERR("[%s] unsupported item: %s \n", SC_FUNCNAME, sr_strerror(rc));
	return rc;
}

i32
sc_interface_set_flags (u32 sw_if_index, u8 admin_up_down)
{
	i32 ret = -1;
	vapi_msg_sw_interface_set_flags *msg = vapi_alloc_sw_interface_set_flags(app_ctx);
	msg->payload.sw_if_index = sw_if_index;
	msg->payload.admin_up_down = admin_up_down;
	vapi_msg_sw_interface_set_flags_hton(msg);

	vapi_error_e rv = vapi_send(app_ctx, msg);
	vapi_msg_sw_interface_set_flags_reply *resp;

	//    SC_VPP_VAPI_RECV;

	vapi_msg_sw_interface_set_flags_reply_ntoh(resp);
	SC_LOG_DBG("setInterfaceFlags:%d ", resp->payload.retval);
	ret = resp->payload.retval;
	vapi_msg_free(app_ctx, resp);
	return ret;
}
