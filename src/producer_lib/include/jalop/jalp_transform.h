/**
 * @file jalp_transform.h This file defines the public API for accessing jalp_param
 * objects.
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
#ifndef JALP_TRANSFORM_H
#define JALP_TRANSFORM_H
/**
 * Enum to restrict AES keysizes
 */
enum jalp_aes_key_size {
	/** Indicates a 128 bit (16 byte) key */
	JALP_AES128,
	/** Indicates a 192 bit (24 byte) key */
	JALP_AES192,
	/** Indicates a 256 bit (32 byte) key */
	JALP_AES256,
};
/**
 * Structure to describe transforms applied to an entry
 */
struct jalp_transform {
	/** the URI for this transform */
	char *uri;
	/** (optional) snippet of XML code to add as a child of the transform element. */
	char *xml;
	/** The next transform in the sequence, or NULL */
	struct jalp_transform *next;
};

/**
 * Create another transform in the chain.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[in] uri A URI for the transform.
 * @param[in] xml_snippet XML for the body of the transform. This must be valid
 * XML.
 * @return A pointer to the newly created transform
 */
struct jalp_transform *jalp_transform_append(struct jalp_transform *prev,
		char *uri,
		char *xml_snippet);
/**
 * Release all memory associated with a jalp_transform object. This frees all
 * jalp_transform objects in the list and calls free for every member.
 * @param[in] transform the list to destroy.
 */
struct jalp_transform *jalp_transform_destroy(struct jalp_transform **transform);

/**
 * Create an XOR transform in the chain.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[in] key The 4 byte XOR key that was applied to the data, this must
 * not be 0.
 * @return a pointer to the newly created transform
 */

struct jalp_transform *jalp_transform_append_xor(struct jalp_transform *prev,
		uint32_t key);

/**
 * Create a deflate transform in the chain.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[out] new_transform a pointer to the newly created transform, or NULL.
 * @return JAL_OK on success, or an error.
 */
struct jalp_transform *jalp_transform_append_deflate(struct jalp_transform *prev);


/**
 * Create an AES transform in the chain. This should only be used for AES
 * transforms as described in the JALoP-v1.0 specification. The key and IV are
 * not required, but are recommended since the intent is not to provide data
 * security, but protection from accidental execution of malicious code.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[in] key_size The key size. The length of \p key is determined by \p
 * key_sizefor example, if \p key_size is #JALP_AES192, the \p key array is
 * assumed to be 24 bytes in length.
 * @param[in] key The AES key, or NULL, the length of this array is determined
 * by \p key_size.
 * @param[in] iv The initialization vector, or NULL. If the IV is provided, it
 * is assumed to contain a 128 bit (16 byte) IV.
 * @return a pointer to the newly created transform
 */
struct jalp_transform *jalp_transform_append_aes(struct jalp_transform *prev,
		enum jalp_aes_key_size,
		uint8_t *key,
		uint8_t *iv);

#endif // JALP_TRANSFORM_H
