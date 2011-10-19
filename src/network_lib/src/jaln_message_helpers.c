/**
 * @file jaln_message_helpers.c This file contains function
 * definitions for internal library functions related to creating JALoP
 * messages
 *
 * @section LICENSE
 *
 * Source code in 3rd-party is licensed and owned by their respective
 * copyright holders.
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

#include <inttypes.h>
#include <jalop/jal_status.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "jal_alloc.h"
#include "jal_asprintf_internal.h"

#include "jaln_message_helpers.h"
#include "jaln_strings.h"

enum jal_status jaln_create_journal_resume_msg(const char *serial_id,
		uint64_t offset, char **msg_out, size_t *msg_out_len)
{
	static const char * const preamble = JALN_MIME_PREAMBLE JALN_MSG_JOURNAL_RESUME JALN_CRLF \
		JALN_HDRS_SERIAL_ID JALN_COLON_SPACE;

	enum jal_status ret = JAL_E_INVAL;
	char *offset_str = NULL;
	if (!msg_out || *msg_out || !msg_out_len) {
		return JAL_E_INVAL;
	}
	if (!serial_id || (offset == 0)) {
		return JAL_E_INVAL;
	}
	jal_asprintf(&offset_str, "%"PRIu64, offset);
	size_t cnt = strlen(preamble) + 1;
	size_t tmp = strlen(serial_id) + strlen(JALN_CRLF);
	if (cnt > (SIZE_MAX - tmp)) {
		goto out;
	}
	cnt += tmp;
	tmp = strlen(JALN_HDRS_JOURNAL_OFFSET JALN_COLON_SPACE);
	if (cnt > (SIZE_MAX - tmp)) {
		goto out;
	}
	cnt += tmp;
	tmp = strlen(offset_str) + strlen(JALN_CRLF) + strlen(JALN_CRLF);
	if (cnt > (SIZE_MAX - tmp)) {
		goto out;
	}
	cnt += tmp;

	char *msg = (char*) jal_malloc(cnt);
	msg[0] = '\0';
	strcat(msg, preamble);
	strcat(msg, serial_id);
	strcat(msg, JALN_CRLF);
	strcat(msg, JALN_HDRS_JOURNAL_OFFSET JALN_COLON_SPACE);
	strcat(msg, offset_str);
	strcat(msg, JALN_CRLF JALN_CRLF);
	*msg_out = msg;
	*msg_out_len = cnt;
	ret = JAL_OK;
	goto out;
out:
	free(offset_str);
	return ret;
}

enum jal_status jaln_create_sync_msg(const char *serial_id, char **msg_out, size_t *msg_len)
{
#define SYNC_MSG_HDRS JALN_MIME_PREAMBLE JALN_MSG_SYNC JALN_CRLF \
		JALN_HDRS_SERIAL_ID JALN_COLON_SPACE "%s" JALN_CRLF JALN_CRLF

	if (!serial_id || !msg_out || *msg_out || !msg_len) {
		return JAL_E_INVAL;
	}
	enum jal_status ret = JAL_E_INVAL;
	char *msg = NULL;

	int len = jal_asprintf(&msg, SYNC_MSG_HDRS, serial_id);
	if (len <= 0) {
		goto err_out;
	}
	// +1 since asprintf will return the length (not including NULL
	// terminator) and this returns the full size of the data.
	*msg_len = (size_t) len + 1;
	*msg_out = msg;
	ret = JAL_OK;

	goto out;

err_out:
	free(msg);
out:
	return ret;

}

enum jal_status jaln_create_subscribe_msg(const char *serial_id, char **msg_out, size_t *msg_out_len)
{
	static const char *preamble = JALN_MIME_PREAMBLE JALN_MSG_SUBSCRIBE JALN_CRLF \
		JALN_HDRS_SERIAL_ID JALN_COLON_SPACE;
	enum jal_status ret = JAL_E_INVAL;
	if (!msg_out || *msg_out || !msg_out_len) {
		goto out;
	}
	if (!serial_id) {
		goto out;
	}
	size_t cnt = strlen(preamble) + 1;
	size_t tmp = strlen(serial_id) + 2 * strlen(JALN_CRLF);
	if (cnt > (SIZE_MAX - tmp)) {
		goto out;
	}
	cnt += tmp;
	char *msg = (char*) jal_malloc(cnt);
	msg[0] = '\0';
	strcat(msg, preamble);
	strcat(msg, serial_id);
	strcat(msg, JALN_CRLF JALN_CRLF);
	*msg_out = msg;
	*msg_out_len = cnt;
	ret = JAL_OK;
out:
	return ret;
}

axl_bool jaln_check_content_type_and_txfr_encoding_are_valid(VortexFrame *frame)
{
	if (!frame) {
		return axl_false;
	}
	const char *ct = VORTEX_FRAME_GET_MIME_HEADER(frame, (JALN_HDRS_CONTENT_TYPE));
	if (!ct) {
		return axl_false;
	}
	if (0 != strcasecmp(ct, JALN_STR_CT_JALOP)) {
		return axl_false;
	}
	// the assumption for beep is if there is no content transfer encoding,
	// it is binary.
	const char *te = VORTEX_FRAME_GET_MIME_HEADER(frame, (JALN_HDRS_CONTENT_TXFR_ENCODING));
	if (te) {
		if (0 != strcasecmp(te, JALN_STR_BINARY)) {
			return axl_false;
		}
	}
	return axl_true;
}
