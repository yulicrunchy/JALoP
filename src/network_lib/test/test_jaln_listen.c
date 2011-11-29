/**
 * @file This file contains tests for jaln_listen.c functions.
 *
 * @section LICENSE
 *
 * Source code in 3rd-party is licensed and owned by their respective
 * copyright holders.
 *
 * All other source code is copyright Tresys Technology and licensed as below.
 *
 * Copyright (c) 2011 Tresys Technology LLC, Columbia, Maryland, USA
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

#include <test-dept.h>
#include <test-dept.h>
#include <jalop/jal_digest.h>
#include <jalop/jaln_connection_callbacks.h>
#include <vortex.h>

#include "jaln_listen.h"
#include "jaln_publisher.h"
#include "jaln_session.h"
#include "jaln_subscriber.h"
#include "jaln_init_msg_handler.h"

#define DGST_ONE "dgst_1"
#define ENC_ONE "enc_1"
static jaln_context *ctx;
static struct jaln_session *sess;
static const char *server_name = "some_server";
static struct jaln_connection_callbacks *conn_cbs;
static axl_bool init_ack_sent;
static axl_bool init_nack_sent;
static axl_bool subscribe_sent;
static axl_bool channel_closed;

static enum jaln_connect_error connect_request_handler_sha256(
		__attribute__((unused)) const struct jaln_connect_request *req,
		__attribute__((unused)) int *selected_encoding,
		int *selected_digest,
		__attribute__((unused)) void *user_data)
{
	// The sha256 digest should get appended to the list.
	*selected_digest = req->dgst_cnt - 1;
	return JALN_CE_ACCEPT;
}

static enum jaln_connect_error connect_request_handler_bad_enc(
		__attribute__((unused)) const struct jaln_connect_request *req,
		int *selected_encoding,
		__attribute__((unused)) int *selected_digest,
		__attribute__((unused)) void *user_data)
{
	*selected_encoding = -1;
	return JALN_CE_ACCEPT;
}

static enum jaln_connect_error connect_request_handler_bad_dgst(
		__attribute__((unused)) const struct jaln_connect_request *req,
		__attribute__((unused)) int *selected_encoding,
		int *selected_digest,
		__attribute__((unused)) void *user_data)
{
	*selected_digest = -1;
	return JALN_CE_ACCEPT;
}

static enum jaln_connect_error connect_request_handler_fails(
		__attribute__((unused)) const struct jaln_connect_request *req,
		__attribute__((unused)) int *selected_encoding,
		__attribute__((unused)) int *selected_digest,
		__attribute__((unused)) void *user_data)
{
	return JALN_CE_UNSUPPORTED_MODE;
}

static enum jaln_connect_error my_connect_request_handler(
		__attribute__((unused)) const struct jaln_connect_request *req,
		__attribute__((unused)) int *selected_encoding,
		__attribute__((unused)) int *selected_digest,
		__attribute__((unused)) void *user_data)
{
	return JALN_CE_ACCEPT;
}

static void fake_subscriber_send_subscribe_request(__attribute__((unused)) struct jaln_session *session)
{
	subscribe_sent = axl_true;
	return;
}

static enum jal_status fake_configure_sub_session_no_lock(
		__attribute__((unused)) VortexChannel *chan,
		__attribute__((unused)) struct jaln_session *sess)
{
	return JAL_OK;
}

static int fake_frame_get_msg_no(__attribute__((unused)) VortexFrame *frame)
{
	return 1;
}
static const char *fake_connection_get_host(__attribute__((unused)) VortexConnection *conn)
{
	return server_name;
}

static VortexChannel *fake_connection_get_channel(__attribute__((unused)) VortexConnection *conn, int chan_num)
{
	if (-1 == chan_num) {
		return NULL;
	}
	return (VortexChannel*) 0xaabbccdd;
}

static axl_bool fake_channel_send_err(
		__attribute__((unused)) VortexChannel *chan,
		__attribute__((unused)) const void *msg,
		__attribute__((unused)) size_t msg_sz,
		__attribute__((unused)) int msg_no_rpy)
{
	init_nack_sent = axl_true;
	return axl_true;
}

static axl_bool fake_channel_close(
		__attribute__((unused)) VortexChannel * channel,
		__attribute__((unused)) VortexOnClosedNotification on_closed)
{
	channel_closed = axl_true;
	return axl_true;
}


static axl_bool fake_channel_send_rpy(__attribute__((unused)) VortexChannel *chan,
		__attribute__((unused)) const void *msg,
		__attribute__((unused)) size_t msg_sz,
		__attribute__((unused)) int msg_no_rpy)
{
	init_ack_sent = axl_true;
	return axl_true;
}

static void fake_channel_set_automatic_mime(__attribute__((unused)) VortexChannel *chan,
		__attribute__((unused)) int flag)
{
	return;
}

static void fake_channel_set_serialize(__attribute__((unused)) VortexChannel *chan,
		__attribute__((unused)) axl_bool flag)
{
	return;
}

static struct jaln_session * fake_find_session_by_rec_channel_fails(
		__attribute__((unused)) jaln_context* ctx,
		__attribute__((unused)) char *server_name_cpy,
		__attribute__((unused)) int paired_chan_num)
{
	return NULL;
}

static struct jaln_session * fake_find_session_by_rec_channel_no_lock(
		__attribute__((unused)) jaln_context* ctx,
		__attribute__((unused)) char *server_name_cpy,
		__attribute__((unused)) int paired_chan_num)
{
	return sess;
}

static axl_bool fake_associate_digest_channel_no_lock(
		__attribute__((unused)) struct jaln_session* sess,
		__attribute__((unused)) VortexChannel *chan,
		__attribute__((unused)) int paired_chan_num)
{
	return axl_true;
}

static axl_bool fake_associate_digest_channel_fails(
		__attribute__((unused)) struct jaln_session* sess,
		__attribute__((unused)) VortexChannel *chan,
		__attribute__((unused)) int paired_chan_num)
{
	return axl_false;
}

static enum jal_status fake_process_init_sub(__attribute__((unused)) VortexFrame *frame,
		struct jaln_init_info ** info_out)
{
	*info_out = jaln_init_info_create();
	axl_list_append((*info_out)->encodings, strdup(ENC_ONE));
	axl_list_append((*info_out)->digest_algs, strdup(DGST_ONE));
	(*info_out)->role = JALN_ROLE_SUBSCRIBER;
	return JAL_OK;
}

static enum jal_status fake_process_init_pub(__attribute__((unused)) VortexFrame *frame,
		struct jaln_init_info ** info_out)
{
	*info_out = jaln_init_info_create();
	axl_list_append((*info_out)->encodings, strdup(ENC_ONE));
	axl_list_append((*info_out)->digest_algs, strdup(DGST_ONE));
	(*info_out)->role = JALN_ROLE_PUBLISHER;
	return JAL_OK;
}


static const char *fake_get_mime_content(VortexMimeHeader *header)
{
	return (char*)header;
}

static VortexMimeHeader *fake_get_mime_header(VortexFrame *frame, const char *header_name)
{
	if (!frame) {
		return NULL;
	}
	if (0 == strcasecmp(header_name, "jal-message")) {
		return (VortexMimeHeader*) "initialize";
	//} else if (0 == strcasecmp(header_name, "jal-count")) {
		//return (VortexMimeHeader*) "3";
	}
	return NULL;
}

#define DECL_MIME_HANDLER(func_name__, header_name__, header_val__) \
static VortexMimeHeader * func_name__ (VortexFrame *frame, const char *header_name) \
{ \
	if (!frame) { \
		return NULL; \
	} \
	if (0 == strcasecmp(header_name, header_name__)) { \
		return (VortexMimeHeader*) header_val__ ; \
	} \
	return fake_get_mime_header(frame, header_name); \
}

//DECL_MIME_HANDLER(get_mime_header_returns_unexpected_msg, "jal-message", "jal-sync");

void setup()
{
	replace_function(vortex_channel_close, fake_channel_close);
	replace_function(vortex_channel_send_err, fake_channel_send_err);
	replace_function(vortex_channel_send_rpy, fake_channel_send_rpy);
	replace_function(vortex_channel_set_automatic_mime, fake_channel_set_automatic_mime);
	replace_function(vortex_channel_set_serialize, fake_channel_set_serialize);

	replace_function(vortex_connection_get_channel, fake_connection_get_channel);
	replace_function(vortex_connection_get_host, fake_connection_get_host);
	replace_function(vortex_connection_get_host_ip, fake_connection_get_host);

	replace_function(vortex_frame_get_msgno, fake_frame_get_msg_no);
	replace_function(vortex_frame_get_mime_header, fake_get_mime_header);
	replace_function(vortex_frame_mime_header_content, fake_get_mime_content);

	replace_function(jaln_configure_pub_session_no_lock, fake_configure_sub_session_no_lock);
	replace_function(jaln_configure_sub_session_no_lock, fake_configure_sub_session_no_lock);
	replace_function(jaln_ctx_find_session_by_rec_channel_no_lock, fake_find_session_by_rec_channel_no_lock);
	replace_function(jaln_process_init, fake_process_init_pub);
	replace_function(jaln_session_associate_digest_channel_no_lock, fake_associate_digest_channel_no_lock);
	replace_function(jaln_subscriber_send_subscribe_request, fake_subscriber_send_subscribe_request);

	ctx = jaln_context_create();
	sess = jaln_session_create();
	sess->jaln_ctx = ctx;
	jaln_ctx_ref(ctx);
	conn_cbs = jaln_connection_callbacks_create();
	ctx->conn_callbacks = conn_cbs;
	conn_cbs->connect_request_handler = my_connect_request_handler;
	struct jal_digest_ctx *dgst = jal_sha256_ctx_create();
	free(dgst->algorithm_uri);
	dgst->algorithm_uri = jal_strdup(DGST_ONE);
	jaln_register_digest_algorithm(ctx, dgst);
	jaln_register_encoding(ctx, ENC_ONE);

	init_ack_sent = axl_false;
	init_nack_sent = axl_false;
	subscribe_sent = axl_false;
	channel_closed = axl_false;
}

void teardown()
{
	restore_function(vortex_connection_get_channel);
	restore_function(vortex_channel_set_automatic_mime);
	restore_function(vortex_channel_set_serialize);
	restore_function(jaln_ctx_find_session_by_rec_channel_no_lock);
	restore_function(jaln_session_associate_digest_channel_no_lock);

	if (ctx->ref_cnt > 1) {
		jaln_ctx_unref(ctx);
	}
	jaln_session_unref(sess);
}

void test_add_new_digest_channel_no_lock()
{
	assert_true(jaln_listener_handle_new_digest_channel_no_lock(ctx,
		(VortexConnection *) 0xbadf00d,
		server_name, 2, 4));
}

void test_add_new_digest_channel_no_lock_fails_with_bad_input()
{
	assert_false(jaln_listener_handle_new_digest_channel_no_lock(NULL, (VortexConnection *) 0xbadf00d, server_name, 2, 4));
	assert_false(jaln_listener_handle_new_digest_channel_no_lock(ctx, NULL, server_name, 2, 4));
	assert_false(jaln_listener_handle_new_digest_channel_no_lock(ctx, (VortexConnection *) 0xbadf00d, NULL, 2, 4));
	assert_false(jaln_listener_handle_new_digest_channel_no_lock(ctx, (VortexConnection *) 0xbadf00d, server_name, -1, 4));
	assert_false(jaln_listener_handle_new_digest_channel_no_lock(ctx, (VortexConnection *) 0xbadf00d, server_name, 2, -1));
}

void test_add_new_digest_channel_no_lock_fails_if_cannot_find_session()
{
	replace_function(jaln_ctx_find_session_by_rec_channel_no_lock, fake_find_session_by_rec_channel_fails);
	assert_false(jaln_listener_handle_new_digest_channel_no_lock(ctx, (VortexConnection *) 0xbadf00d, server_name, 2, 4));
}

void test_add_new_digest_channel_no_lock_fails_if_cannot_associate_channel()
{
	replace_function(jaln_session_associate_digest_channel_no_lock, fake_associate_digest_channel_fails);
	assert_false(jaln_listener_handle_new_digest_channel_no_lock(ctx, (VortexConnection *) 0xbadf00d, server_name, 2, 4));
}

void test_init_msg_handler_does_not_crash_with_bad_inputs()
{
	jaln_listener_init_msg_handler(NULL, (VortexConnection *) 0xbadf00d, (VortexFrame *)0xbadf00d, sess);
	assert_true(channel_closed);

	channel_closed = axl_false;
	jaln_listener_init_msg_handler((VortexChannel *)0xbadf00d, NULL, (VortexFrame *)0xbadf00d, sess);
	assert_true(channel_closed);

	channel_closed = axl_false;
	jaln_listener_init_msg_handler((VortexChannel *)0xbadf00d, (VortexConnection *) 0xbadf00d, NULL, sess);
	assert_true(channel_closed);

	channel_closed = axl_false;
	sess->jaln_ctx = NULL;
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	sess->jaln_ctx = ctx;
	assert_true(channel_closed);

	channel_closed = axl_false;
	sess->jaln_ctx->conn_callbacks = NULL;
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	sess->jaln_ctx->conn_callbacks = conn_cbs;
	assert_true(channel_closed);

}

void test_init_msg_handler_works_for_sha256()
{
	conn_cbs->connect_request_handler = connect_request_handler_sha256;
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	assert_true(init_ack_sent);
	assert_false(subscribe_sent);
	assert_false(channel_closed);
}

void test_init_msg_handler_works_for_publisher()
{
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	assert_true(init_ack_sent);
	assert_false(subscribe_sent);
	assert_false(channel_closed);
}

void test_init_msg_handler_works_with_for_subscriber()
{
	replace_function(jaln_process_init, fake_process_init_sub);
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	assert_true(init_ack_sent);
	assert_true(subscribe_sent);
	assert_false(channel_closed);
}

void test_init_msg_handler_fails_for_bad_enc()
{
	conn_cbs->connect_request_handler = connect_request_handler_bad_enc;
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	assert_true(init_nack_sent);
	assert_false(subscribe_sent);
	assert_true(channel_closed);
}

void test_init_msg_handler_fails_for_bad_dgst()
{
	conn_cbs->connect_request_handler = connect_request_handler_bad_dgst;
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	assert_true(init_nack_sent);
	assert_false(subscribe_sent);
	assert_true(channel_closed);
}

void test_init_msg_handler_fails_when_user_cb_fails()
{
	conn_cbs->connect_request_handler = connect_request_handler_fails;
	jaln_listener_init_msg_handler((VortexChannel *) 0xbadf00d, (VortexConnection *) 0xbadf00d, (VortexFrame *) 0xbadf00d, sess);
	assert_true(init_nack_sent);
	assert_false(subscribe_sent);
	assert_true(channel_closed);
}

