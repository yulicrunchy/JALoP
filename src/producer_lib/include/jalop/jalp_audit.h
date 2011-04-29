/**
 * @file jalp_audit.h This file defines the public API that may interest a
 * program sending CEE audit records to the JAL Local Store.
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
#ifndef JALP_AUDIT_H
#define JALP_AUDIT_H
#include <jalop/jalp_context.h>

/**
 * Send an audit record to the JALoP Local Store
 * Typically audit records will contain all the information needed to identify
 * an audit event. Applications may choose to include auxiliary data via the
 * jalp_application_metadata structure, specifically the
 * jalp_application_metadata::custom field, which may contain any valid XML.
 *
 * In the event the \p ctx was configured to generate digest values
 * (@see jalp_context_set_digest_callbacks), the ProducerLib will generate an
 * applicationMetadata document automatically.
 *
 * @param[in] ctx The context to send the data over
 * @param[in] app_meta An optional struct that the library will convert into an
 * XML document. The resulting document will conform to the applicationMetadata
 * XML Schema defined in the JALoP-v1.0-Specification.
 * @param[in] audit_buffer An optional byte buffer that contains the full contents of
 * a audit entry. An audit entry must conform to the MITRE CEE XML schema. The
 * JPL will verify that \b audit_buffer conforms to the MITRE CEE XML schema
 * and generates and error if it does not.
 * @param[in] audit_buffer_size The size (in bytes) of \p audit_buffer.
 *
 * @note It is an error to pass NULL for both \p app_meta and \p audit_buffer.
 *
 * @return JAL_OK on success.
 *         JAL_EINVAL If the parameters are incorrect.
 *         JAL_NOT_CONNECTED if a connection to the JALoP Local Store couldn't
 *         be made
 *         JAL_SCHEMA_VALIDATION_FAILURE if \b audit_buffer does not conform to
 *         the MITRE CEE XML schema.
 *
 * @note \p ctx will be connected if it isn't already.
 */
enum jal_status jalp_audit(jalp_context *ctx,
		struct jalp_app_metadata *app_meta,
		uint8_t *audit_buffer,
		size_t audit_buffer_size);

#endif // JALP_AUDIT_H

