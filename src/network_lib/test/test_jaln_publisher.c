/**
 * @file test_jaln_publisher.c This file contains tests for jaln_publisher.c functions.
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
#include <jalop/jaln_network_types.h>

#include "jal_alloc.h"

#include "jaln_connection.h"
#include "jaln_connection_callbacks_internal.h"
#include "jaln_publisher.h"
#include "jaln_publisher_callbacks_internal.h"
#include "jaln_context.h"
#include "jaln_channel_info.h"
#include "jaln_digest_info.h"
#include "jaln_digest_resp_info.h"
#include "jaln_sync_msg_handler.h"
#include "jaln_handle_init_replies.h"
#include "jaln_message_helpers.h"

#define FAKE_CHAN_NUM 5
#define SAMPLE_UUID "e25253a3-4986-40b8-8511-56f416cda9b6"

static axlList *calc_dgsts;
static axlList *peer_dgsts;
static axlList *dgst_resp_infos;
static jaln_session *sess;
static jaln_context *ctx;
static int peer_digest_call_cnt;
static int sync_cnt;
static bool fail;
static bool ack_cb;
static bool subscribe_cb;

void peer_digest(
		__attribute__((unused)) jaln_session *sess,
		__attribute__((unused)) const struct jaln_channel_info *ch_info,
			__attribute__((unused)) enum jaln_record_type type,
			__attribute__((unused)) const char *nonce,
			__attribute__((unused)) const uint8_t *local_digest,
			__attribute__((unused)) const uint32_t local_size,
			__attribute__((unused)) const uint8_t *peer_digest,
			__attribute__((unused)) const uint32_t peer_size,
			__attribute__((unused)) void *user_data)
{
	peer_digest_call_cnt++;
}

void on_sync(
		__attribute__((unused)) jaln_session *sess,
		__attribute__((unused)) const struct jaln_channel_info *ch_info,
		__attribute__((unused)) enum jaln_record_type type,
		__attribute__((unused)) enum jaln_publish_mode mode,
		__attribute__((unused)) const char *nonce,
		__attribute__((unused)) struct jaln_mime_header *headers,
		__attribute__((unused)) void *user_data)
{
	sync_cnt++;
}
enum jal_status process_sync_fails(__attribute__((unused)) VortexFrame *frame, __attribute__((unused)) char **nonce)
{
	return JAL_E_INVAL;
}

enum jal_status process_sync_success(__attribute__((unused)) VortexFrame *frame, char **nonce)
{
	*nonce = jal_strdup("sync_nonce");
	return JAL_OK;
}

axl_bool fake_finalize_ans_rpy(__attribute__((unused)) VortexChannel* chan, __attribute__((unused)) int msg_no_rpy)

{
	return axl_true;
}

int fake_vortex_channel_get_number(__attribute__((unused))VortexChannel * channel)
{
	return FAKE_CHAN_NUM;
}

void fake_vortex_channel_set_received_handler(
		__attribute__((unused)) VortexChannel *channel,
		__attribute__((unused)) VortexOnFrameReceived received,
		__attribute__((unused)) axlPointer user_data)
{
	// do nothing
	return;
}

void fake_vortex_connection_set_on_close_full(__attribute__((unused)) VortexConnection *conn,
					__attribute__((unused)) VortexConnectionOnCloseFull on_close,
					__attribute__((unused)) axlPointer data)
{
	return;
}

axl_bool mock_jaln_check_content_type_and_txfr_encoding_are_valid_failure(__attribute__((unused)) VortexFrame *frame)
{
	return axl_false;
}

axl_bool mock_jaln_check_content_type_and_txfr_encoding_are_valid_success(__attribute__((unused)) VortexFrame *frame)
{
	return axl_true;
}

void mock_vortex_connection_shutdown(__attribute__((unused)) VortexConnection *connection)
{
	fail = true;
}

static VortexMimeHeader * mock_vortex_frame_get_mime_header_success(__attribute__((unused)) VortexFrame *frame, __attribute__((unused)) const char * mime_header)
{
	return (VortexMimeHeader *) "dummy";
}

int mock_jaln_handle_initialize_ack_success(__attribute__((unused)) jaln_session *session,
		__attribute__((unused)) enum jaln_role role,
		__attribute__((unused)) VortexFrame *frame)
{
	return 1;
}

int mock_jaln_handle_initialize_nack(__attribute__((unused)) jaln_session *sess,
		__attribute__((unused)) VortexFrame *frame)
{
	return 0;
}

int mock_jaln_handle_initialize_ack_failure(__attribute__((unused)) jaln_session *session,
		__attribute__((unused)) enum jaln_role role,
		__attribute__((unused)) VortexFrame *frame)
{
	return 0;
}

void mock_vortex_channel_set_received_handler(__attribute__((unused)) VortexChannel *channel,
		__attribute__((unused)) VortexOnFrameReceived received,
		__attribute__((unused)) axlPointer user_data)
{
	return;
}

int mock_vortex_channel_get_number_success(__attribute__((unused)) VortexChannel *channel)
{
	return 0;
}

int mock_vortex_channel_get_number_failure(__attribute__((unused)) VortexChannel *channel)
{
	return -1;
}

VortexChannel * mock_vortex_channel_new_fullv(__attribute__((unused)) VortexConnection *connection,
		__attribute__((unused)) int channel_num,
		__attribute__((unused)) const char *serverName,
		__attribute__((unused)) const char *profile,
		__attribute__((unused)) VortexEncoding encoding,
		__attribute__((unused)) VortexOnCloseChannel close,
		__attribute__((unused)) axlPointer close_user_data,
		__attribute__((unused)) VortexOnFrameReceived received,
		__attribute__((unused)) axlPointer received_user_data,
		__attribute__((unused)) VortexOnChannelCreated on_channel_created,
		__attribute__((unused)) axlPointer user_data,
		__attribute__((unused)) const char *profile_content_format,
		...)

{
	return NULL;
}

int fake_publisher_callbacks_is_valid(struct jaln_publisher_callbacks *publisher_callbacks) {
	return (NULL != publisher_callbacks);
}

int fake_connection_callbacks_is_valid(struct jaln_connection_callbacks *conn_callbacks) {
	return (NULL != conn_callbacks);
}

VortexConnection  *fake_vortex_connection_new(
		__attribute__((unused)) VortexCtx *ctx,
		__attribute__((unused)) const char *host,
		__attribute__((unused)) const char *port,
		__attribute__((unused)) VortexConnectionNew on_connected,
		__attribute__((unused)) axlPointer user_data)
{
	return (VortexConnection*) 0xbadf00d;
}

axl_bool fake_vortex_connection_is_ok(__attribute__((unused)) VortexConnection *connection,
		__attribute__((unused)) axl_bool free_on_fail)
{
	return axl_true;
}

VortexChannel *fake_vortex_channel_new(
		__attribute__((unused)) VortexConnection *connection,
		__attribute__((unused)) int channel_num,
		__attribute__((unused)) const char *profile,
		__attribute__((unused)) VortexOnCloseChannel close,
		__attribute__((unused)) axlPointer close_user_data,
		__attribute__((unused)) VortexOnFrameReceived received,
		__attribute__((unused)) axlPointer received_user_data,
		__attribute__((unused)) VortexOnChannelCreated on_channel_created,
		__attribute__((unused)) axlPointer user_data)
{
	jaln_session_unref((jaln_session*) user_data);
	return (VortexChannel*) 0xbadf00d;
}

VortexConnection *fake_vortex_channel_get_connection(
		__attribute__((unused)) VortexChannel *channel)
{
	return (VortexConnection*)0xbadf00d;
}

CURLcode fake_curl_easy_perform(
		__attribute__((unused)) CURL *easy_handle)
{
	return CURLE_OK;
}

CURLcode fake_curl_easy_setopt(
		__attribute__((unused)) CURL *handle,
		__attribute__((unused)) CURLoption option,
		...)
{
	return CURLE_OK;
}

void fake_curl_easy_cleanup(
		__attribute__((unused)) CURL *handle)
{
	return;
}

enum jal_status fake_jaln_create_init_msg(
		__attribute__((unused)) enum jaln_publish_mode mode,
		__attribute__((unused)) enum jaln_record_type type,
		__attribute__((unused)) jaln_context *ctx,
		__attribute__((unused)) struct curl_slist **headers_out)
{
	return JAL_OK;
}

enum jal_status fake_jaln_verify_init_ack_headers(
		__attribute__((unused)) struct jaln_response_header_info *info)
{
	return JAL_OK;
}

void on_connect_ack(
		__attribute__((unused)) const struct jaln_connect_ack *ack,
		__attribute__((unused)) void *user_data)
{
	ack_cb = true;
}

int on_subscribe(
		__attribute__((unused)) jaln_session *sess,
		__attribute__((unused)) const struct jaln_channel_info *ch_info,
		__attribute__((unused)) enum jaln_record_type type,
		__attribute__((unused)) enum jaln_publish_mode mode,
		__attribute__((unused)) struct jaln_mime_header *headers,
		__attribute__((unused)) void *user_data)
{
	subscribe_cb = true;
	return 0;
}


void setup()
{
	replace_function(vortex_channel_finalize_ans_rpy, fake_finalize_ans_rpy);
	replace_function(jaln_process_sync, process_sync_success);
	replace_function(vortex_channel_set_received_handler, fake_vortex_channel_set_received_handler);
	replace_function(vortex_channel_get_number, fake_vortex_channel_get_number);
	replace_function(jaln_process_sync, process_sync_success);
	replace_function(vortex_channel_new, fake_vortex_channel_new);
	replace_function(vortex_connection_new, fake_vortex_connection_new);
	replace_function(vortex_connection_is_ok, fake_vortex_connection_is_ok);
	replace_function(vortex_channel_get_connection, fake_vortex_channel_get_connection);
	replace_function(jaln_publisher_callbacks_is_valid, fake_publisher_callbacks_is_valid);
	replace_function(jaln_connection_callbacks_is_valid, fake_connection_callbacks_is_valid);
	replace_function(jaln_verify_init_ack_headers, fake_jaln_verify_init_ack_headers)
	replace_function(curl_easy_perform, fake_curl_easy_perform);
	replace_function(curl_easy_setopt, fake_curl_easy_setopt);
	replace_function(curl_easy_cleanup, fake_curl_easy_cleanup);
	calc_dgsts = jaln_digest_info_list_create();
	peer_dgsts = jaln_digest_info_list_create();
	dgst_resp_infos = NULL;
	ctx = jaln_context_create();
	sess = jaln_session_create();
	sess->jaln_ctx = ctx;
	sess->curl_ctx = (CURL *)(0xbadf00d);
	sess->ch_info = jaln_channel_info_create();
	ctx->conn_callbacks = jaln_connection_callbacks_create();
	ctx->conn_callbacks->connect_ack = on_connect_ack;

	ctx->pub_callbacks = jaln_publisher_callbacks_create();
	ctx->pub_callbacks->peer_digest = peer_digest;
	ctx->pub_callbacks->sync = on_sync;
	ctx->pub_callbacks->on_subscribe = on_subscribe;

	strcpy(ctx->pub_id,SAMPLE_UUID);

	int dgst_val = 0xf001;
	axl_list_append(calc_dgsts, jaln_digest_info_create("nonce1", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	dgst_val = 0xf002;
	axl_list_append(calc_dgsts, jaln_digest_info_create("nonce2", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	dgst_val = 0xf002;
	axl_list_append(calc_dgsts, jaln_digest_info_create("nonce3", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	dgst_val = 0xf004;
	axl_list_append(calc_dgsts, jaln_digest_info_create("nonce4", (uint8_t*)&dgst_val, sizeof(dgst_val)));

	axl_list_append(peer_dgsts, jaln_digest_info_create("nonce4", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	dgst_val = 0xf003;
	axl_list_append(peer_dgsts, jaln_digest_info_create("nonce3", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	dgst_val = 0xf002;
	axl_list_append(peer_dgsts, jaln_digest_info_create("nonce2", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	dgst_val = 0xf001;
	axl_list_append(peer_dgsts, jaln_digest_info_create("nonce1", (uint8_t*)&dgst_val, sizeof(dgst_val)));

	peer_digest_call_cnt = 0;
	sync_cnt = 0;
	fail = false;
	ack_cb = false;
	subscribe_cb = false;
	replace_function(vortex_frame_get_mime_header, mock_vortex_frame_get_mime_header_success);
}

void teardown()
{
	jaln_session_unref(sess);

	axl_list_free(calc_dgsts);
	axl_list_free(peer_dgsts);
	restore_function(vortex_frame_get_mime_header);
}


void test_pub_does_not_crash_with_bad_input()
{
	axlList *dgst_resp_infos = NULL;
	jaln_pub_notify_digests_and_create_digest_response(NULL, calc_dgsts, peer_dgsts, &dgst_resp_infos);
	jaln_pub_notify_digests_and_create_digest_response(sess, NULL, peer_dgsts, &dgst_resp_infos);
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, NULL, &dgst_resp_infos);
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, NULL);

	dgst_resp_infos = (axlList*) 0xbadf00d;
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, &dgst_resp_infos);
	dgst_resp_infos = NULL;

	sess->jaln_ctx = NULL;
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, &dgst_resp_infos);
	sess->jaln_ctx = ctx;

	sess->jaln_ctx->pub_callbacks->peer_digest = NULL;
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, &dgst_resp_infos);
	sess->jaln_ctx->pub_callbacks->peer_digest = peer_digest;

	jaln_publisher_callbacks_destroy(&sess->jaln_ctx->pub_callbacks);
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, &dgst_resp_infos);

}

void test_pub_notify_digests_works()
{
	axlList *dgst_resp_infos = NULL;
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, &dgst_resp_infos);
	assert_not_equals((void*) NULL, dgst_resp_infos);
	assert_equals(4, peer_digest_call_cnt);
	assert_equals(0, axl_list_length(calc_dgsts));
	assert_equals(4, axl_list_length(peer_dgsts));
	axl_list_free(dgst_resp_infos);
}

void test_pub_notify_digests_works_when_peer_has_extra_dgts()
{
	axlList *dgst_resp_infos = NULL;
	int dgst_val = 0xf005;
	axl_list_append(peer_dgsts, jaln_digest_info_create("nonce5", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, &dgst_resp_infos);
	assert_not_equals((void*) NULL, dgst_resp_infos);
	assert_equals(5, peer_digest_call_cnt);
	assert_equals(0, axl_list_length(calc_dgsts));
	assert_equals(5, axl_list_length(peer_dgsts));
	axl_list_free(dgst_resp_infos);
}

void test_pub_notify_digests_works_when_peer_has_missing_dgst()
{
	axlList *dgst_resp_infos = NULL;
	int dgst_val = 0xf005;
	axl_list_append(calc_dgsts, jaln_digest_info_create("nonce5", (uint8_t*)&dgst_val, sizeof(dgst_val)));
	jaln_pub_notify_digests_and_create_digest_response(sess, calc_dgsts, peer_dgsts, &dgst_resp_infos);
	assert_not_equals((void*) NULL, dgst_resp_infos);
	assert_equals(4, peer_digest_call_cnt);
	assert_equals(1, axl_list_length(calc_dgsts));
	assert_equals(4, axl_list_length(peer_dgsts));
	axl_list_free(dgst_resp_infos);
}

void test_pub_handle_sync_works()
{
	assert_equals(JAL_OK, jaln_publisher_handle_sync(sess, (VortexChannel*) 0xbadf00d, (VortexFrame*) 0xdeadbeef, 1));
}

void test_pub_handle_sync_fails_on_bad_input()
{
	assert_not_equals(JAL_OK, jaln_publisher_handle_sync(NULL, (VortexChannel*) 0xbadf00d, (VortexFrame*) 0xdeadbeef, 1));


	sess->jaln_ctx = NULL;
	assert_not_equals(JAL_OK, jaln_publisher_handle_sync(sess, (VortexChannel*) 0xbadf00d, (VortexFrame*) 0xdeadbeef, 1));
	sess->jaln_ctx = ctx;

	struct jaln_publisher_callbacks *tmp = sess->jaln_ctx->pub_callbacks;
	sess->jaln_ctx->pub_callbacks = NULL;
	assert_not_equals(JAL_OK, jaln_publisher_handle_sync(sess, (VortexChannel*) 0xbadf00d, (VortexFrame*) 0xdeadbeef, 1));
	sess->jaln_ctx->pub_callbacks = tmp;

	sess->jaln_ctx->pub_callbacks->sync = NULL;
	assert_not_equals(JAL_OK, jaln_publisher_handle_sync(sess, (VortexChannel*) 0xbadf00d, (VortexFrame*) 0xdeadbeef, 1));
	sess->jaln_ctx->pub_callbacks->sync = on_sync;

	struct jaln_channel_info *ch_info = sess->ch_info;
	sess->ch_info = NULL;
	assert_not_equals(JAL_OK, jaln_publisher_handle_sync(sess, (VortexChannel*) 0xbadf00d, (VortexFrame*) 0xdeadbeef, 1));
	sess->ch_info = ch_info;
}

void test_pub_create_session_fails_with_bad_input()
{
	jaln_session *my_sess = jaln_publisher_create_session(NULL, "some_host", JALN_RTYPE_JOURNAL);
	assert_equals(1, ctx->ref_cnt);
	assert_pointer_equals((void*) NULL, my_sess);
	assert_equals(1, ctx->ref_cnt);

	my_sess = jaln_publisher_create_session(ctx, NULL, JALN_RTYPE_JOURNAL);
	assert_pointer_equals((void*) NULL, my_sess);
	assert_equals(1, ctx->ref_cnt);

	my_sess = jaln_publisher_create_session(ctx, "some_host", 0);
	assert_pointer_equals((void*) NULL, my_sess);
	assert_equals(1, ctx->ref_cnt);

}

void test_pub_create_session_works()
{
	const char *host = "some_host";
	assert_equals(1, ctx->ref_cnt);
	jaln_session *my_sess = jaln_publisher_create_session(ctx, "some_host", JALN_RTYPE_JOURNAL);
	assert_not_equals((void*) NULL, my_sess);
	assert_equals(2, ctx->ref_cnt);
	assert_equals(JALN_ROLE_PUBLISHER, my_sess->role);
	assert_equals(JALN_RTYPE_JOURNAL, my_sess->ch_info->type);
	assert_not_equals(host, my_sess->ch_info->hostname);
	assert_not_equals((void*) NULL, my_sess->ch_info->hostname);
	assert_string_equals(host, my_sess->ch_info->hostname);

	jaln_session_unref(my_sess);
}

void test_configure_pub_session_fails_on_bad_input()
{
	enum jal_status ret;

	ret = jaln_configure_pub_session(NULL, sess);
	assert_equals(JAL_E_INVAL, ret);

	ret = jaln_configure_pub_session((VortexChannel *)0xbadf00d, NULL);
	assert_equals(JAL_E_INVAL, ret);

	sess->pub_data = (struct jaln_pub_data*) 0xdeadbeef;
	ret = jaln_configure_pub_session((VortexChannel *)0xbadf00d, sess);
	assert_equals(JAL_E_INVAL, ret);
	sess->pub_data = NULL;
}

void test_configure_pub_session_works()
{
	enum jal_status ret;

	replace_function(vortex_connection_set_on_close_full, fake_vortex_connection_set_on_close_full);
	ret = jaln_configure_pub_session((VortexChannel *)0xbadf00d, sess);
	assert_equals(JAL_OK, ret);
	assert_pointer_equals((void*) 0xbadf00d, sess->rec_chan);
	assert_equals(FAKE_CHAN_NUM, sess->rec_chan_num);
	assert_equals(JALN_ROLE_PUBLISHER, sess->role);
	assert_not_equals((void*) NULL, sess->pub_data);

	restore_function(vortex_connection_set_on_close_full);
}

void test_jaln_publisher_send_init()
{
	enum jal_status ret;
	replace_function(jaln_create_init_msg, fake_jaln_create_init_msg)
	sess->pub_data = jaln_pub_data_create(); // since we skipped past session setup
	ret = jaln_publisher_send_init(sess);
	assert_equals(JAL_OK, ret);
	assert_equals(true, ack_cb);
	assert_equals(true, subscribe_cb);
	restore_function(jaln_create_init_msg);
}

void test_jaln_publisher_send_journal_missing()
{
	enum jal_status ret;
	sess->id = jal_strdup("abcd");
	ret = jaln_publisher_send_journal_missing(sess, "abcde");
	assert_equals(JAL_E_INVAL, ret);
}

void test_publish_fails_with_bad_input()
{
	struct jaln_connection *conn = NULL;

	conn = jaln_publish(NULL, "some_host", "1234", JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);

	conn = jaln_publish(ctx, NULL, "1234", JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);

	conn = jaln_publish(ctx, "some_host", NULL, JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);

	conn = jaln_publish(ctx, "some_host", "1234", 0, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);

	conn = jaln_publish(ctx, "some_host", "1234", JALN_RTYPE_ALL | (1 << 4), JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);

	conn = jaln_publish(ctx, "some_host", "1234", JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE -1, NULL);
	assert_pointer_equals((void*) NULL, conn);
}

void test_publish_fails_when_missing_conn_callbacks()
{
	struct jaln_connection *conn = NULL;

	jaln_connection_callbacks_destroy(&ctx->conn_callbacks);
	conn = jaln_publish(ctx, "some_host", "1234", JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);
}

void test_publish_fails_when_missing_pub_callbacks()
{
	struct jaln_connection *conn = NULL;

	jaln_publisher_callbacks_destroy(&ctx->pub_callbacks);
	conn = jaln_publish(ctx, "some_host", "1234", JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);
}

void test_publish_fails_when_missing_pub_id()
{
	struct jaln_connection *conn = NULL;

	memset(ctx->pub_id, '\0', sizeof(ctx->pub_id));
	conn = jaln_publish(ctx, "some_host", "1234", JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);
}

void test_publish_fails_when_already_connected()
{
	struct jaln_connection *conn = NULL;
	ctx->is_connected = axl_true;

	conn = jaln_publish(ctx, "some_host", "1234", JALN_RTYPE_JOURNAL, JALN_ARCHIVE_MODE, NULL);
	assert_pointer_equals((void*) NULL, conn);
}

void test_publish_success_for_all_types()
{
	replace_function(vortex_connection_set_on_close_full, fake_vortex_connection_set_on_close_full);
	struct jaln_connection *conn = NULL;

	conn = jaln_publish(ctx, "some_host", "1234", JALN_RTYPE_ALL, JALN_ARCHIVE_MODE, NULL);
	assert_not_equals((void*) NULL, conn);
	assert_equals(true, ack_cb);
	assert_equals(true, subscribe_cb);
	jaln_connection_destroy(&conn);
	restore_function(vortex_connection_set_on_close_full);
}

void test_register_publisher_id_already_set()
{
	const char *other_uuid = "a9c6ee4b-adb1-4f34-9eb7-057e90e6786b";
	assert_equals(JAL_E_INVAL, jaln_register_publisher_id(ctx, other_uuid));
	assert_string_equals(SAMPLE_UUID, ctx->pub_id);
}

void test_register_publisher_id_nulls()
{
	memset(ctx->pub_id, '\0', sizeof(ctx->pub_id));
	assert_equals(JAL_E_INVAL, jaln_register_publisher_id(ctx, NULL));
	assert_equals(JAL_E_INVAL, jaln_register_publisher_id(NULL, SAMPLE_UUID));
}

void test_register_publisher_id_not_uuid()
{
	memset(ctx->pub_id, '\0', sizeof(ctx->pub_id));
	assert_equals(JAL_E_INVAL, jaln_register_publisher_id(ctx, "Not a UUID"));
}

void test_register_publisher_id_bad_uuid()
{
	// check that any char being invalid will cause a failure
	for (unsigned int i = 0; i < strlen(SAMPLE_UUID); ++i) {
		memset(ctx->pub_id, '\0', sizeof(ctx->pub_id));
		char bad_id[] = SAMPLE_UUID;
		bad_id[i] = 'g';
		assert_equals(JAL_E_INVAL, jaln_register_publisher_id(ctx, bad_id));
	}
	// clear the publisher id from the context to stop cascading failure
	memset(ctx->pub_id, '\0', sizeof(ctx->pub_id));
	char too_long[] = SAMPLE_UUID "1";
	assert_equals(JAL_E_INVAL, jaln_register_publisher_id(ctx, too_long));
	memset(ctx->pub_id, '\0', sizeof(ctx->pub_id));
	char too_short[] = SAMPLE_UUID;
	too_short[strlen(too_short) - 1] = '\0';
	assert_equals(JAL_E_INVAL, jaln_register_publisher_id(ctx, too_short));
}

void test_register_publisher_id_valid()
{
	memset(ctx->pub_id, '\0', sizeof(ctx->pub_id));
	assert_equals(JAL_OK, jaln_register_publisher_id(ctx, SAMPLE_UUID));
	assert_string_equals(SAMPLE_UUID, ctx->pub_id);
}
