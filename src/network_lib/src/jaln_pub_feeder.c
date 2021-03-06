/**
 * @file jaln_pub_feeder.c This file contains the functions related to the
 * implementation of VortexPayloadFeeder for sending records from a publisher
 * to a subscriber.
 *
 * @section LICENSE
 *
 * Source code in 3rd-party is licensed and owned by their respective
 * copyright holders.
 *
 * All other source code is copyright Tresys Technology and licensed as below.
 *
 * Copyright (c) 2012-2013 Tresys Technology LLC, Columbia, Maryland, USA
 *
 * This software was developed by Tresys Technology LLC
 * with U.S. Government sponsorship.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <limits.h>

#include "jal_alloc.h"
#include "jaln_pub_feeder.h"
#include "jaln_context.h"
#include "jaln_message_helpers.h"
#include "jaln_strings.h"
// This is for the 'copy_buffer' function, which should probably get re-factored
// to a different file.
#include "jaln_subscriber_state_machine.h"

axl_bool jaln_pub_feeder_get_size(jaln_session *sess, int *size)
{
	// expect that the pub_data is already filled out...
	*size = sess->pub_data->vortex_feeder_sz;
	return axl_true;
}

axl_bool jaln_pub_feeder_fill_buffer(jaln_session *sess, char *b, int *size)
{
	uint64_t dst_sz = *size;
	uint64_t dst_off = 0;
	struct jaln_pub_data *pd = sess->pub_data;
	struct jaln_publisher_callbacks *cbs = sess->jaln_ctx->pub_callbacks;
	struct jaln_channel_info *ch_info = sess->ch_info;
	void *ud = sess->jaln_ctx->user_data;
	uint8_t *buffer = (uint8_t*)b;
	enum jal_status ret = JAL_OK;

	if (sess->errored) {
		return axl_false;
	}

	if (!pd->finished_headers && (dst_sz > dst_off)) {
		jaln_copy_buffer(buffer, dst_sz, &dst_off, (uint8_t*) pd->headers, pd->headers_sz, &pd->headers_off, axl_true);
		if (pd->headers_off == pd->headers_sz) {
			pd->finished_headers = axl_true;
			free(pd->headers);
			pd->headers = NULL;
			pd->headers_sz = 0;
			pd->headers_off = 0;
		}
	}

	if (!pd->finished_sys_meta && (dst_sz > dst_off)) {
		jaln_copy_buffer(buffer, dst_sz, &dst_off, pd->sys_meta, pd->sys_meta_sz, &pd->sys_meta_off, axl_true);
		if (pd->sys_meta_sz == pd->sys_meta_off) {
			pd->finished_sys_meta = axl_true;
		}
	}

	if (!pd->finished_sys_meta_break && (dst_sz > dst_off)) {
		jaln_copy_buffer(buffer, dst_sz, &dst_off, (uint8_t*)JALN_STR_BREAK, strlen(JALN_STR_BREAK), &pd->break_off, axl_true);
		if (strlen(JALN_STR_BREAK) == pd->break_off) {
			pd->finished_sys_meta_break = axl_true;
			pd->break_off = 0;
		}
	}

	if (!pd->finished_app_meta && (dst_sz > dst_off)) {
		jaln_copy_buffer(buffer, dst_sz, &dst_off, pd->app_meta, pd->app_meta_sz, &pd->app_meta_off, axl_true);
		if (pd->app_meta_off == pd->app_meta_sz) {
			pd->finished_app_meta = axl_true;
			pd->sys_meta = NULL;
			pd->sys_meta_off = 0;
			pd->sys_meta_sz = 0;
			pd->app_meta = NULL;
			pd->app_meta_sz = 0;
			pd->app_meta_off = 0;
		}
	}

	if (!pd->finished_app_meta_break && (dst_sz > dst_off)) {
		jaln_copy_buffer(buffer, dst_sz, &dst_off, (uint8_t*)JALN_STR_BREAK, strlen(JALN_STR_BREAK), &pd->break_off, axl_true);
		if (strlen(JALN_STR_BREAK) == pd->break_off) {
			pd->finished_app_meta_break = axl_true;
			pd->break_off = 0;
		}
	}

	if (!pd->finished_payload && (dst_sz > dst_off)) {
		switch (ch_info->type) {
		case JALN_RTYPE_AUDIT:
		case JALN_RTYPE_LOG: {
			uint64_t tmp_offset = pd->payload_off;
			jaln_copy_buffer(buffer, dst_sz, &dst_off, pd->payload, pd->payload_sz, &tmp_offset, axl_true);
			pd->payload_off = tmp_offset;
			break;
		}
		case JALN_RTYPE_JOURNAL: {
			uint64_t left_in_buffer = dst_sz - dst_off;
			uint64_t bytes_acquired = left_in_buffer;

			ret = pd->journal_feeder.get_bytes(pd->payload_off,
							buffer + dst_off,
							&bytes_acquired,
							pd->journal_feeder.feeder_data);
			if (ret != JAL_OK || (bytes_acquired > left_in_buffer)) {
				return axl_false;
			}

			ret = sess->dgst->update(pd->dgst_inst, buffer + dst_off, bytes_acquired);
			if (JAL_OK != ret) {
				return axl_false;
			}

			dst_off += bytes_acquired;
			pd->payload_off += bytes_acquired;
			break;
		}
		default:
			return axl_false;
		}
		if (pd->payload_sz == pd->payload_off) {
			pd->finished_payload = axl_true;
			size_t dgst_len = sess->dgst->len;
			if (JAL_OK != sess->dgst->final(pd->dgst_inst, pd->dgst, &dgst_len)) {
				return axl_false;
			}

			jaln_session_add_to_dgst_list(sess, pd->nonce, pd->dgst, dgst_len);
			cbs->notify_digest(sess, ch_info, ch_info->type, pd->nonce, pd->dgst, dgst_len, ud);
			pd->payload = NULL;
		}
	}

	if (!pd->finished_payload_break && (dst_sz > dst_off)) {
		jaln_copy_buffer(buffer, dst_sz, &dst_off, (uint8_t*)JALN_STR_BREAK, strlen(JALN_STR_BREAK), &pd->break_off, axl_true);
		if (strlen(JALN_STR_BREAK) == pd->break_off) {
			pd->finished_payload_break = axl_true;
			pd->break_off = 0;
		}
	}
	*size = dst_off;
	return axl_true;
}

axl_bool jaln_pub_feeder_is_finished(jaln_session *sess, int *finished)
{
	*finished = sess->errored || sess->pub_data->finished_payload_break;
	return *finished;
}

axl_bool jaln_pub_feeder_handler(
		__attribute__((unused)) VortexCtx *ctx,
		VortexPayloadFeederOp op_type,
		__attribute__((unused)) VortexPayloadFeeder *feeder,
		axlPointer param1,
		axlPointer param2,
		axlPointer user_data)
{
	jaln_session *sess = (jaln_session*) user_data;

	int *size = param1;
	char *buffer = param2;

	switch (op_type) {
	case PAYLOAD_FEEDER_GET_SIZE:
		// should return the 'full' size, which may not be storable in
		// an int...
		return jaln_pub_feeder_get_size(sess, size);
		break;
	case PAYLOAD_FEEDER_GET_CONTENT:
		return jaln_pub_feeder_fill_buffer(sess, buffer, size);
		break;
	case PAYLOAD_FEEDER_IS_FINISHED:
		return jaln_pub_feeder_is_finished(sess, size);
		break;
	case PAYLOAD_FEEDER_RELEASE:
		// nothing really to do here.
		return axl_true;
		break;
	}
	// unknown OP, is it better to fail now? or just ignore?
	return axl_false;
}

void jaln_pub_feeder_reset_state(jaln_session *sess)
{
	if (!sess || !sess->pub_data) {
		return;
	}

	struct jaln_pub_data *pd = sess->pub_data;

	free(pd->nonce);

	pd->nonce = NULL;
	pd->sys_meta = NULL;
	pd->app_meta = NULL;
	pd->payload = NULL;
	pd->vortex_feeder_sz = 0;
	pd->headers_off = 0;
	pd->sys_meta_off = 0;
	pd->app_meta_off = 0;
	pd->payload_off = 0;
	pd->break_off = 0;

	pd->finished_headers = axl_false;
	pd->finished_sys_meta = axl_false;
	pd->finished_sys_meta_break = axl_false;
	pd->finished_app_meta = axl_false;
	pd->finished_app_meta_break = axl_false;
	pd->finished_payload = axl_false;
	pd->finished_payload_break = axl_false;

	if (!pd->dgst_inst) {
		pd->dgst_inst = sess->dgst->create();
		if (!pd->dgst_inst) {
			goto err_out;
		}
	}

	if (JAL_OK != sess->dgst->init(pd->dgst_inst)) {
		goto err_out;
	}

	if (pd->dgst) {
		memset(pd->dgst, '\0', sess->dgst->len);
	} else {
		pd->dgst = (uint8_t*)jal_calloc(1, sess->dgst->len);
	}

	return;
err_out:
	jaln_session_set_errored(sess);
}

void jaln_pub_feeder_calculate_size_for_vortex(jaln_session *sess)
{
	if (!sess || !sess->pub_data) {
		return;
	}
	struct jaln_pub_data *pd = sess->pub_data;
	pd->vortex_feeder_sz = 0;
	if (!jaln_pub_feeder_safe_add_size(&pd->vortex_feeder_sz, pd->payload_sz)) {
		return;
	}
	if (!jaln_pub_feeder_safe_add_size(&pd->vortex_feeder_sz, pd->app_meta_sz)) {
		return;
	}
	if (!jaln_pub_feeder_safe_add_size(&pd->vortex_feeder_sz, pd->sys_meta_sz)) {
		return;
	}
	if (!jaln_pub_feeder_safe_add_size(&pd->vortex_feeder_sz, pd->headers_sz)) {
		return;
	}
	jaln_pub_feeder_safe_add_size(&pd->vortex_feeder_sz, 3 * strlen(JALN_STR_BREAK));
}

axl_bool jaln_pub_feeder_safe_add_size(int *cnt, const uint64_t to_add)
{
	if (INT_MAX < to_add) {
		*cnt = INT_MAX;
		return axl_false;
	}
	if ((INT_MAX - to_add) < (uint64_t) *cnt) {
		*cnt = INT_MAX;
		return axl_false;
	}
	*cnt += to_add;
	return axl_true;
}

enum jal_status jaln_pub_begin_next_record_ans(jaln_session *sess,
						struct jaln_record_info *rec_info)
{
	if (!sess || !sess->pub_data) {
		return JAL_E_INVAL;
	}

	enum jal_status ret = JAL_OK;
	struct jaln_pub_data *pd = sess->pub_data;

	ret = jaln_create_record_ans_rpy_headers(rec_info, &pd->headers, &pd->headers_sz);
	if (JAL_OK != ret) {
		goto out;
	}

	jaln_pub_feeder_calculate_size_for_vortex(sess);

	jaln_session_ref(sess);

	VortexPayloadFeeder *feeder = vortex_payload_feeder_new(jaln_pub_feeder_handler, sess);
	vortex_payload_feeder_set_on_finished(feeder, jaln_pub_feeder_on_finished, sess);

	vortex_mutex_lock(&sess->wait_lock);

	vortex_channel_send_ans_rpy_from_feeder(sess->rec_chan, feeder, pd->msg_no);

	vortex_cond_wait(&sess->wait, &sess->wait_lock);
	vortex_mutex_unlock(&sess->wait_lock);
out:
	return ret;
}

void jaln_pub_feeder_on_finished(VortexChannel *chan,
		__attribute__((unused)) VortexPayloadFeeder *feeder,
		axlPointer user_data)
{
	jaln_session *sess = (jaln_session*) user_data;
	struct jaln_channel_info *ch_info = sess->ch_info;
	enum jaln_record_type type = ch_info->type;
	struct jaln_pub_data *pd = sess->pub_data;
	struct jaln_publisher_callbacks *pub_cbs = sess->jaln_ctx->pub_callbacks;

	pub_cbs->on_record_complete(sess, ch_info, type, pd->nonce, sess->jaln_ctx->user_data);
	pd->payload_off = 0;

	if (!sess->errored) {
		if (sess->closing) {
			goto err_out;
		}
	}
	goto out;

err_out:
	jaln_session_set_errored(sess);
	vortex_channel_finalize_ans_rpy(chan, sess->pub_data->msg_no);
out:
	vortex_mutex_lock(&sess->wait_lock);
	vortex_mutex_unlock(&sess->wait_lock);
	jaln_session_unref(sess);
	vortex_cond_signal(&sess->wait);
	return;
}
