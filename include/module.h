#pragma once
#include <inttypes.h>
#include <time.h>

#ifdef MESIBO_COMPILATION
#define MESIBO_EXPORT extern "C"
#else
#define MESIBO_EXPORT
#endif

#define MESIBO_MODULE_SIGNATURE		0xAA55773388FF1100
#define MESIBO_MODULE_VERSION		1

#define MESIBO_MODULES_PADDING	0, 0, 0, 0, 0, 0, 0, 0

#define MESIBO_RESULT_OK	0
#define MESIBO_RESULT_FAIL	-1

#define MESIBO_RESULT_CONSUMED	1
#define MESIBO_RESULT_PASS	0

#define MESIBO_MODULE_SANITY_CHECK(m, version, len) do { \
if(MESIBO_MODULE_VERSION != version) { mesibo_log(m, 0, "module version mismatch\n"); return MESIBO_RESULT_FAIL; } \
if (sizeof(mesibo_module_t) != len) { mesibo_log(m, 0, "module size mismatch\n"); return MESIBO_RESULT_FAIL; } \
if (MESIBO_MODULE_SIGNATURE != m->signature) { mesibo_log(m, 0, "module signature mismatch\n"); return MESIBO_RESULT_FAIL; } } while(0)

typedef struct module_config_item_s {
	char *name;
	char *value;
} module_config_item_t;

typedef struct module_config_s {
	int count;
	module_config_item_t items[0];
} module_config_t;

//forward declaration
typedef struct mesibo_module_s mesibo_module_t; 
typedef intptr_t        mesibo_int_t;
typedef uintptr_t  	mesibo_uint_t;
typedef int(*mesibo_module_init_fn)(mesibo_int_t version, mesibo_module_t *mod, mesibo_int_t len);
typedef int (*mesibo_module_http_data_callback_t)(void *cbdata, mesibo_int_t state, mesibo_int_t progress, const char *buffer, mesibo_int_t size);
typedef enum {MODULE_HTTP_STATE_REQUEST, MODULE_HTTP_STATE_REQBODY, MODULE_HTTP_STATE_RESPHEADER, MODULE_HTTP_STATE_RESPBODY, MODULE_HTTP_STATE_DONE} module_http_state_t;

typedef struct _module_http_option_t {
	const char *proxy;

	//body or post data
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

	mesibo_uint_t maxredirects;

	mesibo_uint_t conn_timeout, header_timeout, body_timeout, total_timeout;

	mesibo_uint_t retries;
	mesibo_uint_t synchronous;

} module_http_option_t;

typedef struct mesibo_message_params_s {
	mesibo_uint_t 	aid;
	mesibo_uint_t 	id; //original id, not the 64 bit
	mesibo_uint_t 	refid;
	mesibo_uint_t 	groupid;
	mesibo_uint_t 	flags;
	mesibo_uint_t 	type;
	mesibo_uint_t 	expiry;
	mesibo_uint_t 	to_online;
	
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


typedef struct mesibo_module_s {
	mesibo_uint_t	version; 	/* mesibo module API version */
	mesibo_uint_t	flags;		/* module flags */

	const char     	*name;		/* module name */	
	const char     	*description;	/* module description */	
	void		*ctx;		/* module context */
	module_config_t *config;	/* module configuration */

	mesibo_int_t	(*on_cleanup)(mesibo_module_t *mod);

	mesibo_int_t	(*on_message)(mesibo_module_t *mod, mesibo_message_params_t *params, const char *message, mesibo_uint_t len);
	mesibo_int_t	(*on_message_status)(mesibo_module_t *mod, mesibo_message_params_t *params, mesibo_uint_t  status);
	
	mesibo_int_t	(*on_call)(mesibo_module_t *mod);
	mesibo_int_t	(*on_call_status)(mesibo_module_t *mod);

	mesibo_int_t	(*on_login)(mesibo_module_t *mod, mesibo_user_t *user);

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
MESIBO_EXPORT mesibo_int_t	mesibo_http(mesibo_module_t *mod, const char *url, const char *post, mesibo_module_http_data_callback_t cb, void *cbdata, module_http_option_t *opt);
MESIBO_EXPORT mesibo_int_t	mesibo_log(mesibo_module_t *mod, mesibo_uint_t level, const char *format, ...);
MESIBO_EXPORT char* mesibo_util_getconfig(mesibo_module_t* mod, const char* item_name);
MESIBO_EXPORT char* mesibo_util_json_extract(char *src, const char *key, char **next);
MESIBO_EXPORT void mesibo_util_create_thread(void *(*threadFunction) (void *), void *data, size_t stacksize, const char *name);
