#pragma once
#ifdef __cplusplus 
#define MESIBO_EXPORT extern "C"
extern "C" { 
#else
#define MESIBO_EXPORT
#endif

#include <inttypes.h>
#include <time.h>
#include <stdarg.h>

#define MESIBO_MODULE_SIGNATURE		0xAA55773388FF1100
#define MESIBO_MODULE_VERSION		1

#define MESIBO_MODULES_PADDING	0, 0, 0, 0, 0, 0, 0, 0

#define MESIBO_RESULT_OK	0
#define MESIBO_RESULT_FAIL	-1

#define MESIBO_RESULT_CONSUMED	1
#define MESIBO_RESULT_PASS	0

#define MESIBO_MODULE_MSGSTATUS_DELIVERED	2
#define MESIBO_MODULE_MSGSTATUS_READ		3
#define MESIBO_MODULE_MSGSTATUS_MAX		3
#define MESIBO_MODULE_MSGSTATUS_FAIL		0x80
#define MESIBO_MODULE_MSGSTATUS_NOTACTIVE	0x81
#define MESIBO_MODULE_MSGSTATUS_INBOXFULL	0x82
#define MESIBO_MODULE_MSGSTATUS_INVALIDDEST	0x83
#define MESIBO_MODULE_MSGSTATUS_NOINCOMING	0x86
#define MESIBO_MODULE_MSGSTATUS_BLOCKED		0x88
#define MESIBO_MODULE_MSGSTATUS_FAILMAX		0x88

#define MESIBO_MODULE_SANITY_CHECK(m, version, len) do { \
if(MESIBO_MODULE_VERSION != version) { mesibo_log(m, 0, "module version mismatch\n"); return MESIBO_RESULT_FAIL; } \
if (sizeof(mesibo_module_t) != len) { mesibo_log(m, 0, "module size mismatch\n"); return MESIBO_RESULT_FAIL; } \
if (MESIBO_MODULE_SIGNATURE != m->signature) { mesibo_log(m, 0, "module signature mismatch\n"); return MESIBO_RESULT_FAIL; } } while(0)

//forward declaration
typedef struct mesibo_module_s mesibo_module_t; 
typedef intptr_t        mesibo_int_t;

typedef struct module_config_item_s {
	char *name;
	char *value;
} module_config_item_t;

typedef struct module_config_s {
	mesibo_int_t count;
	module_config_item_t items[0];
} module_config_t;

typedef uintptr_t  	mesibo_uint_t;
typedef mesibo_int_t (*mesibo_module_init_fn)(mesibo_int_t version, mesibo_module_t *mod, mesibo_int_t len);

typedef mesibo_int_t (*mesibo_http_ondata_t)(void *cbdata, mesibo_int_t state, mesibo_int_t progress, const char *buffer, mesibo_int_t size);
typedef mesibo_int_t (*mesibo_http_onstatus_t)(void *cbdata, mesibo_int_t status, const char *response_type);
typedef void (*mesibo_http_onclose_t)(void *cbdata,  mesibo_int_t result);

typedef enum {MODULE_HTTP_STATE_REQUEST, MODULE_HTTP_STATE_REQBODY, MODULE_HTTP_STATE_RESPHEADER, MODULE_HTTP_STATE_RESPBODY, MODULE_HTTP_STATE_DONE} module_http_state_t;

typedef struct _mesibo_http_t {
	//const char *proxy;
	const char *url;
	const char *post;

	const char *content_type; //body content type

	const char *extra_header;
	const char *user_agent;
	const char *referrer;
	const char *origin;
	const char *cookie;
	const char *encoding; // could be gzip, deflate, identity, br (do not use 'compress' which is obsolete)
	const char *cache_control; //cache control and expiry
	const char *accept;
	const char *etag;
	mesibo_uint_t ims; //if modified since, gmt time

	//mesibo_uint_t maxredirects;

	mesibo_uint_t conn_timeout, header_timeout, body_timeout, total_timeout;

	mesibo_http_ondata_t on_data;
	mesibo_http_onstatus_t on_status;
	mesibo_http_onclose_t on_close;

} mesibo_http_t;

typedef mesibo_int_t (*mesibo_fcgi_ondata_t)(void *cbdata, mesibo_int_t result, const char *buffer, mesibo_int_t size);

typedef struct mesibo_fcgi_s {
	char *USER;
	char *SCRIPT_FILENAME;
	char *QUERY_STRING;
	char *CONTENT_TYPE;
	char *SCRIPT_NAME;
	char *REQUEST_URI;
	char *DOCUMENT_URI;
	char *DOCUMENT_ROOT;
	char *SERVER_PROTOCOL;
	char *REQUEST_SCHEME;
	char *REMOTE_ADDR;
	char *REMOTE_PORT;
	char *SERVER_ADDR;
	char *SERVER_PORT;
	char *SERVER_NAME;
	char *HTTP_HOST;
	char *HTTP_CONNECTION;
	char *HTTP_CACHE_CONTROL;
	char *HTTP_UPGRADE_INSECURE_REQUESTS;
	char *HTTP_USER_AGENT;
	char *HTTP_ACCEPT;
	char *HTTP_ACCEPT_ENCODING;
	char *HTTP_ACCEPT_LANGUAGE;

	char *body;
	uint64_t bodylen;

} mesibo_fcgi_t;

typedef mesibo_int_t (*mesibo_socket_ondata_t)(void *cbdata, const char *data, mesibo_int_t len);
typedef void (*mesibo_socket_onwrite_t)(void *cbdata);
typedef void (*mesibo_socket_onconnect_t)(void *cbdata, mesibo_int_t asock, mesibo_int_t fd);
typedef void (*mesibo_socket_onclose_t)(void *cbdata, mesibo_int_t type);

typedef struct mesibo_socket_s {
	const char *url;	
	const char *ws_protocol; /* only for websock - valid only if host is NULL */

	mesibo_int_t keepalive;
	mesibo_int_t verify_host;
	void *ssl_ctx; //use if provided else generate if enable_ssl=1
	
	mesibo_socket_onconnect_t on_connect;
	mesibo_socket_onwrite_t on_write;
	mesibo_socket_ondata_t on_data;
	mesibo_socket_onclose_t on_close;
} mesibo_socket_t;


#define MOD_MESIBO_MSGFLAG_DELIVERYRECEIPT	0x1
#define MOD_MESIBO_MSGFLAG_READRECEIPT		0x2
#define MOD_MESIBO_MSGFLAG_TRANSIENT		0x4
#define MOD_MESIBO_MSGFLAG_PRESENCE		0x8
#define MOD_MESIBO_MSGFLAG_MEDIA		0x100
#define MOD_MESIBO_MSGFLAG_SENDMESSAGE		(MOD_MESIBO_MSGFLAG_DELIVERYRECEIPT|MOD_MESIBO_MSGFLAG_READRECEIPT|MOD_MESIBO_MSGFLAG_TRANSIENT|MOD_MESIBO_MSGFLAG_PRESENCE)
#define MOD_MESIBO_MSGFLAG_OUTGOING		0x10000

#define MOD_MESIBO_UFLAG_DESTONLINE		0x1

typedef struct mesibo_message_params_s {
	mesibo_uint_t 	aid;
	mesibo_uint_t 	id; //original 32-bit id (not the 64-bit)
	mesibo_uint_t 	refid;
	mesibo_uint_t 	groupid;
	mesibo_uint_t 	flags;
	mesibo_uint_t 	type;
	mesibo_uint_t 	expiry;
	mesibo_uint_t 	uflags;
	mesibo_uint_t 	status;
	mesibo_uint_t 	sid;
	mesibo_uint_t 	device;
	
	uintptr_t	reserved_0;
	uintptr_t	reserved_1;
	uintptr_t	reserved_2;
	uintptr_t	reserved_3;

	char 	*to, *from;
} mesibo_message_params_t;

typedef struct mesibo_user_s {
	mesibo_uint_t 	flags;
	mesibo_int_t 	online;
	char 		*address;

	uintptr_t	reserved_0;
	uintptr_t	reserved_1;
	uintptr_t	reserved_2;
	uintptr_t	reserved_3;
} mesibo_user_t;

#define MOD_MESIBO_FLAG_INCOMING	0x1
#define MOD_MESIBO_FLAG_OUTGOING	0x2

typedef struct mesibo_module_s {
	mesibo_uint_t	version; 	/* mesibo module API version */
	mesibo_uint_t	flags;		/* module flags */

	const char     	*name;		/* module name */	
	const char     	*description;	/* module description */	
	void		*ctx;		/* module context */
	module_config_t *config;	/* module configuration */

	mesibo_int_t	(*on_cleanup)(mesibo_module_t *mod);

	mesibo_int_t	(*on_message)(mesibo_module_t *mod, mesibo_message_params_t *params, char *message, mesibo_uint_t len);
	mesibo_int_t	(*on_message_status)(mesibo_module_t *mod, mesibo_message_params_t *params);
	
	mesibo_int_t	(*on_call)(mesibo_module_t *mod);
	mesibo_int_t	(*on_call_status)(mesibo_module_t *mod);

	mesibo_int_t	(*on_login)(mesibo_module_t *mod, mesibo_user_t *user);

	mesibo_int_t	(*on_config)(mesibo_module_t *mod, mesibo_uint_t type, void *config);
	
	void		*custom_fns; //custom functions

	mesibo_uint_t	signature;	/* module signature */

	uintptr_t	reserved_0;
	uintptr_t	reserved_1;
	uintptr_t	reserved_2;
	uintptr_t	reserved_3;
	uintptr_t	reserved_4;
	uintptr_t	reserved_5;
	uintptr_t	reserved_6;
	uintptr_t	reserved_7;
	
	//this function will be initialized by Mesibo
	mesibo_int_t	(*invoke)(mesibo_int_t id, mesibo_module_t *mod, ...);
} mesibo_module_t;


#if 0
#define MESIBO_CALLID_LOG	1
//#define mesibo_call_log(mod, level, format, ...) do { mod->invoke(MESIBO_CALLID_LOG, mod, level, format, ## __VA_ARGS__); } while(0)
static mesibo_int_t mesibo_call_log(mesibo_module_t *mod, mesibo_uint_t level, const char *format, ...) { 
	return mod->invoke(MESIBO_CALLID_LOG, mod, level, format); 
}
#endif

MESIBO_EXPORT mesibo_int_t	mesibo_message(mesibo_module_t *mod, mesibo_message_params_t *params, const char *message, mesibo_uint_t len);
MESIBO_EXPORT mesibo_int_t	mesibo_log(mesibo_module_t *mod, mesibo_uint_t level, const char *format, ...);
MESIBO_EXPORT mesibo_int_t 	mesibo_vlog(mesibo_module_t *mod, mesibo_uint_t level, const char *format, va_list args);

// Utility functions
MESIBO_EXPORT mesibo_int_t	mesibo_util_http(mesibo_http_t *req, void *cbdata);
MESIBO_EXPORT mesibo_int_t 	mesibo_util_fcgi(mesibo_fcgi_t *req, const char *host, mesibo_int_t port, mesibo_int_t keepalive, mesibo_fcgi_ondata_t cb, void *cbdata);

MESIBO_EXPORT mesibo_int_t 	mesibo_util_socket_connect(mesibo_socket_t *sock, void *cbdata);
MESIBO_EXPORT mesibo_int_t 	mesibo_util_socket_write(mesibo_int_t sock, const char *data, mesibo_int_t len);
MESIBO_EXPORT void 	mesibo_util_socket_close(mesibo_int_t sock);

MESIBO_EXPORT char* 		mesibo_util_getconfig(mesibo_module_t* mod, const char* item_name);
MESIBO_EXPORT char* 		mesibo_util_json_extract(char *src, mesibo_int_t len, const char *key, char **next);
MESIBO_EXPORT void 		mesibo_util_create_thread(void *(*threadFunction) (void *), void *data, size_t stacksize, const char *name);
MESIBO_EXPORT mesibo_int_t 	mesibo_util_usec();
#ifdef __cplusplus 
}
#endif
