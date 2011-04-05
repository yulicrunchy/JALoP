#ifndef jalp_PRODUCER_H
#define jalp_PRODUCER_H
/**
 * @file jal_producer.h This file defines the public API available to
 * applications generating JAL data. It provides the interfaces needed to build
 * the appilication metadata sections and submit JAL data (and metadata) to the
 * JAL Local Store.
 *
 * @section LICENSE
 *
 * Source code in 3rd-party is licensed and owned by their respective
 * copyright holders.
 *
 * All other source code is copyright Tresys Technology and licensed as below.
 *
 * Copyright (c) 2011 Tresys Technology LLCColumbia, Maryland, USA
 *
 * This software was developed by Tresys Technology LLC
 * with U.S. Government sponsorship.
 *
 * Licensed under the Apache LicenseVersion 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writingsoftware
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KINDeither express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdint.h>
#include <stdlib.h>
enum jal_status {
	JAL_E_XML_PARSE = -1024,
	JAL_E_XML_SCHEMA,
	JAL_E_NOT_CONNECTED,
	JAL_E_INVAL,
	JAL_E_NO_MEM,
	JAL_OK = 0,
};
/** Indicates if a given entry is considered malicious. */
enum jalp_threat_level {
	/** 
	 * Indications the application didn't checkor could not determine if
	 * the entry is unsafe.
	 */
	JAL_THREAT_UNKNOWN = 0,
	/**
	 * Indicates the application performed some sort of scanning on the
	 * entry and determined it was not a thread.
	 * @note a file could be marked as safe and still contain malicious
	 * code that was undetected.
	 */
	JAL_THREAT_SAFE,
	/**
	 * Indicates the application performed some sort of scanning on the
	 * entry a determined the entry contained malicious code.
	 */
	JAL_THREAT_MALICIOUS,
};


/**
 * The jalp_connection holds information about the connection to the JALoP Local Store.
 * The jalp_connection is not thread safe. Applictions should either create
 * seperate logging connections for each thread, or properly guard the jalp_connection.
 *
 * Because each connection requires resources in the JAL Local Store, a
 * massively threaded process should avoid creating a connection for each thread.
 */
struct jalp_connection_t {
	int socket; /**< The socket used to communicate with the JALoP Local Store */
	char *path; /**< The path that was originally used to connect to the socket */
	char *hostname; /**< The hostname to use when generating the application metadata sections */
	char *app_name; /**< The application name to use when generating the application metadata sections */
};

/**
 * A linked list of structured data elements. Structured data provides a
 * mechanism to list extra key/value pairs. The elements are namespaced using
 * the sd_id field. This corresponds to the Structured Data section of the
 * syslog RFC (RFC 5424, section 6.3)
 */
struct jalp_param {
	/**
	 * The key for this param
	 */
	char *key;
	/**
	 * The value of this element.
	 */
	char *value;
	/**
	 * The next element in the list.
	 */
	struct jalp_param *next;
};
/**
 * Represents a set of structured data elements. Applications should use an ID
 * containing an '@' unless using one of the registered IDs.
 * @see rfc5424
 */
struct jalp_structured_data {
	/**
	 * The SD-ID for all param elements in \p param_list.
	 */
	char *sd_id;
	/**
	 * A list of params
	 */
	struct jalp_param *param_list;
};
/**
 * The syslog_metadata structure should be filled in if an application wishes
 * to generate events that contain data similar to what syslog produces.
 *
 * These correspond to the elments in the JAL application metadata schema
 * (applicationMetadata.xsd).
 *
 * The hostname and application name are taken from the the jalp_connection, while
 * the process ID (PID) is generated automatically. Timestamps are generated
 * automatically if the timestamp field is NULL.
 */
struct jalp_syslog_metadata {
	/**
	 * The time that should be logged for this record. If NULL, the JAL
	 * library will generate a timestamp itself. The timestamp must confrom
	 * to the XML Schema dataTime format.
	 */
	char *timestamp;
	/**
	 * The message ID may be used to identify the 'type' of the log
	 * message. See the MSGID field in RFC 5424.
	 */
	char *messageId;
	/**
	 * A linked list of structured data elmeents. If NULL, no elements are
	 * added.
	 */
	struct jalp_structured_data *sd_head;
	/** 
	 * The faclity, must be between 0 and 23. Value of -1 indicates it should be ignored. 
	 * The numeric values of the facility have the same meanings as for
	 * syslog.
	 */
	int8_t facility;
	/**
	 * The severity indicates the level a log messages is grouped in. The
	 * severity must be between 0 and 7. A value of -1 indicates the
	 * severity should be ignored.
	 */
	int8_t severity;
};

/**
 * Represents one entry in a stack frame.  All fields are optional and may be
 * set to NULL. Line Numbers are assumed to start counting at 1, so a value of
 * 0 for \p line_number will suppress the filed.
 * If an application is providing multiple levels of stack information, they
 * should list the stack frames from the bottom up. That is, the first
 * stack_frame in the list should be the one where the log actuallyer happened.
 *
 * The depth should be filled in to indicate the level in the stack trace. A
 * value of -1 indicates the depth for this frame should not be included. The
 * frame with depth 0 is considered the inner most (i.e. where the log took
 * place) frame.
 *
 * The JPL does not verify that the depths follow any particular order.
 */
struct stack_frame {
	/** The caller's name */
	char *caller_name;
	/** The filename where the log message was generated */
	char *file_name;
	/** The line number in the source file */
	uint64_t line_number;
	/** The class name that generated the log **/
	char *class_name;
	/** The method/function where the log was generated */
	char *method_name;
	/** The 'depth' of this stack frame */
	int depth;
	/** The next (parent) stack frame. */
	struct stack_frame *next;
};

/**
 * Structure to represent the 'logger' type data in the application metadata.
 *
 * All fields are optional, except the loggerName.
 */
struct jalp_logger_metadata {
	/** The name of this logger*/
	char *logger_name;
	/** The severity of this log entry */
	struct severity *severity;
	/**
	 * The timestamp of this log entry. If NULL, the JPL will generate the
	 * a timestamp. This must conform to the XML Schema dateTime type.
	 */
	char *timestamp;
	/** The thread ID. */
	char *threadId;
	/** The Log message to include */
	char *message;
	/** The nested diagnostic context, checkout Log4J for a description */
	char *nested_diagnostic_context;
	/** The mapped diagnostic context, checkout Log4J for a description */
	char *mapped_diagnostic_context;
	/** A stack trace */
	struct stack_frame *stack;
	/** An extended list of metadata. */
	struct structured_data *sd;
};
/**
 * Holds a digest value for any digest algorithm
 */
struct jalp_digest {
	/** The URI for the digest algorithm */
	char *uri;
	/** the digest value */
	uint8_t *val;
	/** The lengthin bytes, of the digest */
	uint32_t len;
};

/**
 * Structure that represents a App Metadata document.
 */
struct jalp_app_metadata {
	/**
	 * indicates what type of data this contains, syslog metadata,
	 * logger metadata, or some custom format
	 */
	int type;
	/**
	 * An application defined event ID. This can be used to correlate
	 * multiple entries, or be NULL. It's has no meaning to JALoP.
	 */
	char *eventId;
	/**
	 * holds the sys, log, or custom sub-element.
	 */
	union {
		struct jalp_syslog_metadata sys;
		struct jalp_logger_metadata log;
		/**
		 * a block of XML data. It may conform to any schema,
		 * or none at all. This may be a complete document, or just a
		 * snippet.
		 */
		char *custom;
	};
	/** The digest, if any, of the payload */
	struct jalp_digest *payload_digest;
	/** 
	 * Optional metadata for the file. Applications are strongly
	 * encouraged to include this for journal entries.
	 */
	struct jalp_file_metadata *file_metadata;
};
/**
 * Structure to describe transforms applied to an entry
 */
struct jalp_transform {
	/** the URI for this transform */
	char *uri;
	/** Snippet of XML code to add as a child of the transform element. */
	char *xml;
	/** The next transform in the sequenceor NULL */
	struct jalp_transform *next;
};

/**
 * Create another transfrom in the chain.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[in] uri A URI for the transform.
 * @param[in] child_text XML for the body of the transform. This must be valid
 * XML.
 * @param[out] new_transform A pointer to the newly created transformor NULL.
 * @return JAL_OK on success, or an error.
 */
enum jal_status jalp_transform_create(struct jalp_transform *prev, 
		char *uri, 
		char *child_text, 
		struct jalp_transform **new_transform);
/**
 * Create an XOR transform in the chain.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[in] key The 4 byte XOR key that was applied to the datathis must
 * not be 0.
 * @param[out] new_transform a pointer to the newly created transformor NULL.
 * @return JAL_OK on success, or an error.
 */

enum jal_status jalp_xor_transform_create(struct jalp_transform *prev, 
		uint32_t key, 
		struct jalp_transform** new_transform);

/**
 * Create a deflate transform in the chain.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[out] new_transform a pointer to the newly created transformor NULL.
 * @return JAL_OK on success, or an error.
 */

enum jal_status jalp_deflate_transform_create(struct jalp_transform *prev, 
		struct jalp_transform** new_transform);
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
 * Create an AES transform in the chain. This should only be used for AES
 * transforms as described in the JALoP-v1.0 specification. The key and IV are
 * not required, but are recommended since the intent is not to provide data
 * securitybut protection from accidental execution of malicios code.
 *
 * @param[in] prev The transform that should come directly before the new
 * transform. If \p prev already points somewhere for \p next, the new
 * transform is inserted into the list.
 * @param[in] key_size The key size. The length of \p key is determined by \p
 * key_sizefor example, if \p key_size is #JALP_AES192, the \p key array is
 * assumed to be 24 bytes in length.
 * @param[in] key The AES keyor NULL, the length of this array is determined
 * by \p key_size.
 * @param[in] iv The initialization vector, or NULL.
 * @param[out] new_transform a pointer to the newly created transformor NULL.
 * @return JAL_OK on success, or an error.
 */
enum jal_status jalp_aes_transform_create(struct jalp_transform *prev, 
		enum jalp_aes_key_size,
		uint32_t *key,
		struct jalp_transform** new_transform);
/**
 * Describes the content-type of the entry. 
 * @see MIME
 */
struct jalp_content_type {
	/**
	 * The top level media type. This must be one of following:
	 *  - "application"
	 *  - "audio"
	 *  - "example"
	 *  - "image"
	 *  - "message"
	 *  - "model"
	 *  - "test"
	 *  - "video"
	 *
	 *  The JPL provides macros for each of these
	 *  @see contentTypeMacros
	 */
	char *media_type;
	/** A string for the subtypethis may be anything. */
	char *subtype;
	/** A list of optional parameters. */
	struct jalp_param *params;
};

/**
 * @defgroup contentTypeMacros Content Type Macros
 */
/** @ingroup contentTypeMacros */
#define JALP_CT_APPLICATION "application"
#define JALP_CT_AUDIO "audio"
#define JALP_CT_EXAMPLE "example"
#define JALP_CT_IMAGE "image"
#define JALP_CT_MESSAGE "message"
#define JALP_CT_MODEL "model"
#define JALP_CT_TEST "test"
#define JALP_CT_VIDEO "video"
/**
 * Detailed information about the file typeetc.
 */
struct jalp_file_info {
	/** The name of the file. */
	char *filename;
	/** Indicator of whether or not this entry is considered malicious. */
	enum jalp_threat_level threat_level;
	/** The size of the file before any transforms were applied to it. */
	uint64_t original_size;
	/** The size of the file after all transforms were applied */
	uint64_t size;
	/** 
	 * An (optional) content type. If NULL, no content-type is added to
	 * the XML.
	 */
	struct jal_content_type *content_type;
};
/**
 * Structure to provide metadata about the payloadtypically only used for
 * journal entries.
 */
struct jalp_file_metadata {
	/**
	 * Generic metadata about the file.
	 */
	struct jalp_file_info file_info;
	/** 
	 * List of transforms that the application applied before submitting
	 * the entry to the JAL local store
	 */
	struct jalp_transform *transforms;
};

/**
 * opaque pointer for a connection to the JAL local store
 */
typedef struct jalp_connection_t* jalp_connection;

/**
 * Load an RSA private key. If successfulevery application metadata document
 * will get signed (using RSA+SHA256). Once a key is setit cannot be unset.
 * You must createa a new context.
 *
 * @param[in] jalp_connection The connection object to attach the RSA keys to.
 * @param[in] keyfile The path to the private key file.
 * @param[in] password The password for the key file.
 *
 * @return JAL_OK on success, or an error code.
 *
 */
enum jal_status jalp_connection_load_pem_rsa(jalp_connection conn,
		const char *keyfile,
		const char *password);
/**
 * Load an RSA public certificate. This must be the public certificate that
 * corresponds to the private key set with #jalp_connection_load_pem_rsa.
 * If applications add the certificatethe Producer Library will add a KeyInfo
 * block for the certificate.
 *
 * @param[in] jalp_connection The connection object to attach the RSA keys to.
 * @param[in] certfile The path to the x509 public certificate file.
 * @return JAL_OK on success, or an error code.
 */
enum jal_status jalp_connection_load_x509_cert(jalp_connection conn,
		const char *certfile);
/**
 * Create a jalp_connection.
 * @param[in] path The name of the socket to connect to. If this is NULL, the
 * default path defined in jalp_SOCKET_NAME is used.
 *
 * In most cases, it is safe pass NULL as the path. For testing purposes, or
 * when the path to the domain socket in the producer process is different
 * from the path in the JALoP Local Store, the caller may specify a different
 * path.
 *
 * @param[in] hostname A string to use when filling out metadata sections. If
 * this is set to NULL or the empty string, the JPL will try to generate a suitable
 * hostname (i.e. by calling gethostname() on POSIX systems).
 * @param[in] app_name A string to use when filling out metadata sections. If
 * this is set to NULL o the empty string, the JPL will try to generate a
 * suitable process name.
 *
 * @param[out] conn A pointer that will receive the address of the newly created
 * jalp_connection, or NULL.
 *
 * @return JAL_OK on success, or an error code.
 */
enum jal_status jalp_connection_create(char* path,
		char *hostname,
		char *app_name,
		jalp_connection *conn);

/**
 * Connect to the JALoP Local Store. It is safe to call this repeatedly, even
 * after a connection was made. This call will block until the connection is
 * established.
 *
 * @param[in] conn the #jalp_connection to connect
 * @return JAL_OK if the connection is complete.
 */
enum jal_status jalp_connection_connect(jalp_connection conn);
/*
 * Disconect from a JAL Local Store
 * @param[in] conn the connection to disconnect.
 */
enum jal_status jalp_connection_disconnect(jalp_connection conn);
/**
 * Disconnect (if needed) and destroy the connection.
 *
 * @param[in,out] conn The #jalp_connection to destroy. On return, this will be set to
 * NULL.
 */
void jalp_connection_destroy(jalp_connection *conn);

/**
 * Send a log message to the JALoP Local Store
 *
 * @param[in] conn The connection to send the data over
 * @param[in] app_meta Pointer to a jalp_app_metadata structure. The 
 * ProducerLib will convert the structure into an appropriate XML document.
 * @param[in] log_string A preformatted string to send to the JALoP Local Store
 * @return JAL_OK on sucess.
 *         JAL_XML_PARSE_ERROR if app_metadata fails to parse
 *         JAL_XML_SCHEMA_ERROR if app_metadata fails to pass schema validation
 *         JAL_EINVAL if both paramters are NULL.
 *         JAL_NOT_CONNECTED if a connection to the JALoP Local Store couldn't
 *         be made
 *
 * The entire log_string (including the NULL terminator) is sent to the JALoP
 * Local Store. An application that includes a digest of the log_string in
 * their metadata should be aware of this. If an application calculates the
 * digest without the NULL terminator, they should either change their digest
 * calculations, or they should use #jalp_log_buffer to truncate the buffer.
 *
 * The conn will be connected if it isn't already.
 *
 * app_meta or log_string may be NULL, but not both.
 * */
enum jal_status jalp_log(jalp_connection conn,
		struct jalp_app_metadata *app_meta,
		char* log_string);

/**
 * Send a log message to the JALoP Local Store
 *
 * @param[in] conn The connection to send the data over
 * @param[in] app_metadata A struct that the ProducerLib will convert into XML
 * for the application metadata.
 * schema.
 * @param[in] log_buffer A byte buffer
 * @return JAL_OK on sucess.
 *         JAL_XML_PARSE_ERROR if app_metadata fails to parse
 *         JAL_XML_SCHEMA_ERROR if app_metadata fails to pass schema validation
 *         JAL_XML_EINVAL if there is something wrong with the parameters
 *         JAL_NOT_CONNECTED if a connection to the JALoP Local Store couldn't
 *         be made
 *
 * If app_meta is NULL, log_buffer must be non-NULL and log_buffer_size must
 * be > 0. If app_meta is non-NULL, log_buffer may be NULL. If log_buffer is
 * NULL, log_buffer_size must be 0.
 *
 * The conn will be connected if it isn't already.
 */
enum jal_status jalp_log_buffer(jalp_connection conn,
		struct jalp_app_metadata *app_meta,
		char *app_metadata,
		uint8_t *log_buffer,
		size_t log_buffer_size);

/**
 * Send a journal entry to the JALoP Local Store
 *
 * @param[in] conn The connection to send the data over
 * @param[in] app_metadata Metedata about the journal data.
 * @param[in] log_string A preformatted string to send to the JALoP Local Store
 * @return JAL_OK on sucess.
 *         JAL_XML_PARSE_ERROR if app_metadata fails to parse
 *         JAL_XML_SCHEMA_ERROR if app_metadata fails to pass schema validation
 *         JAL_EINVAL if both paramters are NULL.
 *         JAL_NOT_CONNECTED if a connection to the JALoP Local Store couldn't
 *         be made
 *
 * Sends the entire contents of journal_string to the JALoP Local Store,
 * including the NULL terminator. Applications that include digests of the
 * journal data should include the NULL terminator in their digest
 * calculations or use #jalp_journal_buffer()
 *
 * The conn will be connected if it isn't already.
 */
enum jal_status jalp_journal(jalp_connection conn,
		struct jalp_app_metadata *app_meta,
		char *journal_string);

/**
 * Send a journal data to the JALoP Local Store
 *
 * @param[in] app_metadata An XML document conforming to the application metadata
 * schema.
 * @param[in] journal_buffer A byte buffer that contains the full contents of
 * a journal entry.
 * @return JAL_OK on sucess.
 *         JAL_XML_PARSE_ERROR if app_metadata fails to parse
 *         JAL_XML_SCHEMA_ERROR if app_metadata fails to pass schema validation
 *         JAL_EINVAL If the parameters are incorrect.
 *         JAL_NOT_CONNECTED if a connection to the JALoP Local Store couldn't
 *         be made
 *
 * The conn will be connected if it isn't already.
 */
enum jal_status jalp_journal_buffer(jalp_connection conn,
		struct jalp_app_metadata *app_meta,
		uint8_t * journal_buffer,
		uint64_t journal_buffer_size);

/**
 * Send an open file descriptor to the JAL Local Store. The caller must not
 * write to the file identified by \p fd after making this call, or else the
 * JAL local store may receive incorrect data. This call works by sending just
 * the file descriptor and is not availble on all platforms. You can check for
 * it's existence by checking for JALP_CAN_SEND_FDS, example:
 * @code
 * #ifdef JALP_CAN_SEND_FDS
 * \/\* do something *\/
 * #endif
 * @endcode
 *
 * @param[in] conn The connection to send the record over.
 * @param[in] app_meta the application provided metadata (or null);
 * @param[in] fd The file descriptor of the journal.
 *
 * @note would it be better to fake this call if sending the file descriptor is
 * impossible? Considering that would make this call extremely slow on
 * platforms that don't have SCM_RIGHTS, I don't think it's a good idea to do that.
 * Could be done in a seperate worker thread, but that could still cause confusing 
 * behaviour since the sending thread here would need to prevent the app from
 * exiting while it's sending the data to the JNL....
 */
enum jal_status jalp_journal_fd(jalp_connection conn,
		struct jalp_app_metadata *app_meta,
		int fd);

/**
 * Open the file at \p path and send it the JAL local store. This uses the
 * #jalp_journal_fd internally to send a file descriptor. The application
 * should never write to the file after calling this method, but is free to
 * unlink it.
 *
 * Applications should provide both the \p app_meta adn \p path paramters.
 * One of these paramaters may be NULL, but not both.
 *
 * @param[in] conn The connection to send the journal over.
 * @param[in] app_metadata The application provided metadata about the journal
 * entry.
 * @param[in] path The path to the file to journal.
 *
 * @return JAL_OK if the JPL was successful at opening the file and sending the
 * descriptor to the JAL local store.
 *
 * @note same comment as for #jalp_journal_fd, should we emulate this?
 *
 */
enum jal_status jalp_journal_path(jalp_connection conn,
		struct jalp_app_metadata *app_meta,
		uint8_t * journal_buffer,
		uint64_t journal_buffer_size);
/**
 * Send an audit record to the JALoP Local Store
 *
 * @param[in] conn The connection to send the data over
 * @param[in] app_metadata A structure that the ProducerLib will convert into XML.
 * @param[in] audit_string An audit document conforming to the MITRE CEE event.xsd document.
 * @return JAL_OK on sucess.
 *         JAL_XML_PARSE_ERROR if one of the documents failed to parse
 *         JAL_XML_SCHEMA_ERROR if one of the documents failed schema
 *         validation.
 *         JAL_EINVAL if both paramters are NULL.
 *         JAL_NOT_CONNECTED if a connection to the JALoP Local Store couldn't
 *         be made
 *
 * Send the audit record to the the JALoP Local Store. Both the app_metadata
 * and the audit_record are validated against the appropriate schemas. The
 * entire audit record, including the NULL terminator, is sent to the JALoP
 * Local Store. Applications that include a digest of the audit record as part
 * of the app_metadata should include the NULL terminator or use
 * #jalp_audit_buffer()
 *
 * \p conn will be connected if it isn't already.
 */
enum jal_status jalp_audit(jalp_connection conn,
		struct jalp_app_metadata *app_meta,
		char *audit_string);

/**
 * Send an audit record to the JALoP Local Store
 *
 * @param[in] conn The connection to send the data over
 * @param[in] app_metadata Metadata about the audit entry.
 * @param[in] audit_buffer A byte buffer that contains the full contents of
 * a audit entry.
 * @return JAL_OK on sucess.
 *         JAL_XML_PARSE_ERROR if app_metadata fails to parse
 *         JAL_XML_SCHEMA_ERROR if app_metadata fails to pass schema validation
 *         JAL_EINVAL If the parameters are incorrect.
 *         JAL_NOT_CONNECTED if a connection to the JALoP Local Store couldn't
 *         be made
 *
 * \p conn will be connected if it isn't already.
 */
enum jal_status jalp_audit_buffer(jalp_connection conn,
			    struct jalp_app_metadata *app_meta,
			    uint8_t *audit_buffer,
			    size_t audit_buffer_size);

/**
 * Create a jalp_structured_data element.
 *
 * @param[in] prev The location in a list to add the element. If prev is not the
 * end of the list, then the new node is created as the next element of prev,
 * and prev->next becomes the new node's next element.
 * This function may be used to add elements to the end, or middle
 * of a list.
 * @param[in] sd_id The sd_id to use for the new element
 * @param[in] name The name of this element
 * @param[in] value The value of this element.
 *
 * @return a newly created jalp_structred_data pointer. This must be freed with
 * jalp_destroy_structured_data_list(struct jalp_structured_data*).
 *
 * sd_id, name, and value are copied to allocated buffers.
 */
struct jalp_structured_data *jalp_create_structured_data(struct jalp_structured_data *prev,
						       char *sd_id);
/**
 * Insert a new param element in \p group. If there are already elements in the
 * list, this will insert this param at the head of the list.
 * @see #jalp_param_insert
 *
 * @param[in] group the SD element to add to.
 * @param[in] key The key of this param.
 * @param[in] value The value of this param.
 *
 * @return the newly created param
 */
struct jalp_param *jalp_structured_data_insert_param(struct jalp_structured_data *group,
						       char *name,
						       char *value);

/**
 * Insert a new param element in \p group. If there are already elements in the
 * list, this will insert this param at the head of the list.
 * @see #jalp_param_insert
 *
 * @param[in] the jalp_content_type structure to add the param to.
 * @param[in] key The key of this param.
 * @param[in] value The value of this param.
 *
 * @return the newly created param
 */
struct jalp_param *jalp_content_type_insert_param(struct jalp_content_type *content_type,
						       char *name,
						       char *value);
/**
 * Create a new structured_data element as the next element in the list. If
 * #prev already has elements, this is inserted between the 2 existing
 * elements.
 * @param[in] prev The list to add to.
 * @param[in] key The key of this param.
 * @param[in] value The value of this param.
 *
 * @return the newly created param
 */
struct jalp_param *jalp_param_insert(struct jalp_param *group,
						       char *name,
						       char *value);
/**
 * Release all memory associated with this structured data list. This frees all
 * params of #group and all #jalp_structured_data elements.
 * @param[in,out] group The SD group to destory
 */
void jalp_free_structured_data_list(struct jalp_structured_data **group);
/**
 *
 * Destroy an entire list. This must only be called on the head of the list. If
 * called anywhere else, memory leaks will occur.
 *
 * @param[in,out] head The head of the list to free.
 */
void jalp_destroy_structured_data_list(struct jalp_structured_data **head);


#endif /* jalp_PRODUCER_H */
