/**
 * @file jaln_listen.h This file contains function definitions
 * related to listening for a remote peer to connect over the JALoP
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
#ifndef _JALN_LISTENER_H_
#define _JALN_LISTENER_H_

#include "jal_alloc.h"
#include "jaln_context.h"
#include "jaln_session.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Helper function called when a new channel is created by the peer and has
 * indicated it's use as a 'digest' channel.
 *
 * @param[in] ctx The jaln_context to associate with the new channel.
 * @param[in] conn The Vortex connection where the channel was created.
 * @param[in] server_name The name/IP of the remote server
 * @param[in] chan_num The channel number assigned to the new channel.
 * @param[in] paired_chan_num The channel number (indicated by the peer) to
 * associate this new 'digest' channel with.
 *
 * @return axl_true on success, axl_false if there is an error.
 */
axl_bool jaln_listener_handle_new_digest_channel_no_lock(jaln_context *ctx,
		VortexConnection *conn,
		const char *server_name,
		int new_chan_num,
		int paired_chan_num);

/**
 * Vortex Frame handler to deal with an 'initialize' message.
 *
 * @param[in] chan The channel that received the message
 * @param[in] conn The connection for the channel
 * @param[in] frame The complete frame for the 'initialize' message
 * @param[in] user_data Expected to be a pointer to a jaln_session
 */
void jaln_listener_init_msg_handler(VortexChannel *chan,
		VortexConnection *conn,
		VortexFrame *frame,
		axlPointer user_data);

/**
 * Helper function called when a new channel is created by the peer and has
 * indicated it's use as a 'digest' channel.
 *
 * @param[in] ctx The jaln_context to associate with the new channel.
 * @param[in] conn The Vortex connection where the channel was created.
 * @param[in] server_name The name/IP of the remote server
 * @param[in] chan_num The channel number assigned to the new channel.
 *
 * @return axl_true on success, axl_false if there is an error.
 */
axl_bool jaln_listener_handle_new_record_channel_no_lock(jaln_context *ctx,
		VortexConnection *conn,
		const char *server_name,
		int chan_num);

/**
 * Helper function for the listener to respond to a request to create a new
 * channel.
 *
 * @param[in] ctx The jaln_context
 * @param[in] chan_num The channel number
 * @param[in] conn The connection
 * @param[in] server_name The name/IP of the peer
 * @param[in] profile_content Addition snippet of data sent in the message to
 * create the channel. For a new record channel, this must be blank, for a new
 * digest channel, this will indicate the existing record channel to associate
 * the new digest channel with.
 *
 * @return axl_true if everything appears in order, this indicates that channel
 * creation should be allowed.
 */
axl_bool jaln_listener_start_channel_no_lock(jaln_context *ctx,
		int chan_num,
		VortexConnection *conn,
		const char *server_name,
		const char *profile_content);

/**
 * Function that is registered with Vortex to decide whether or not the
 * creation of a new channel should be allowed.
 *
 * @param[in] profile The profile
 * @param[in] chan_num The channel number for the new channel
 * @param[in] conn The connection where this channel will be created
 * @param[in] server_name the name/IP of the peer
 * @param[in] profile_content additional data sent with the message to create
 * the channel.
 * @param[out] profile_content_reply additional data to be sent with the
 * response to creating the channel.
 * @param[out] encoding The encoding for \p profile_content_reply
 * @param[in] user_data This is expected to be a jaln_context.
 *
 * @see jaln_listener_start_channel_no_lock
 *
 * @return axl_true if the channel should be created, axl_false otherwise.
 */
axl_bool jaln_listener_start_channel_extended(
		const char *profile,
		int chan_num,
		VortexConnection *v_conn,
		const char *server_name,
		const char *profile_content,
		char **profile_content_reply,
		VortexEncoding encoding,
		axlPointer user_data);


#ifdef __cplusplus
}
#endif
#endif
