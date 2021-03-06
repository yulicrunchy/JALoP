/**
 * @file jaldb_reocrd_xml.h This file declares functions to deal with
 * converting jaldb_record to a system meta-data document.
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

#ifndef _JALDB_RECORD_XML_H_
#define _JALDB_RECORD_XML_H_

#include <openssl/pem.h>

#include "jaldb_status.h"
#include <jalop/jal_status.h>
#include <jalop/jal_digest.h>

#ifdef __cplusplus
extern "C" {
#endif

struct jaldb_record;

/**
 * Generate a XML document for the system meta-data.
 * Note that although the returned buffer is NULL terminated, the length
 * returned in \p dsize, will be the number of characters in the
 * buffer, not including the NULL terminator.
 *
 * @param [in] rec The record to create the document for.
 * @param [in] signing_key The key to sign the document with. If NULL, the
 * document will be left unsigned
 * @param [in] app_meta_dgst The digest of the application metadata. If NULL
 * the manifest will not include this digest.
 * @param [in] payload_dgst The digest of the payload. If NULL the manifest
 * will not include this digest.
 * @param [out] doc Upon successful return, this will be assigned to a memory
 * buffer that contains the XML document.
 * @param [out] dsize Upon successful return, this will be the size of the
 * buffer.
 *
 * @return JALDB_OK on success, or an error code.
 */
enum jaldb_status jaldb_record_to_system_metadata_doc(struct jaldb_record *rec,
		RSA *signing_key,
		uint8_t *app_meta_dgst, size_t app_meta_dgst_len, const char *app_meta_algorithm_uri,
		uint8_t *payload_dgst, size_t payload_dgst_len, const char *payload_algorithm_uri,
		char **doc,
		size_t *dsize);

/*
 * Function to parse sys metadata xml into a jaldb_record structure
 * @param xml [in] buffer containing the xml to parse
 * @param xml_len [in] Length of buffer
 * @param sys_meta [out] The populated structure
 */
enum jal_status jaldb_xml_to_sys_metadata(uint8_t *xml, size_t xml_len, struct jaldb_record **sys_meta);



#ifdef __cplusplus
}
#endif

#endif

