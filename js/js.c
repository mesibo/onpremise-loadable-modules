/** 
 * File: js.c 
 * Description:
 * JavaScript Module to load and call functions in ECMAScript
 * http://www.ecma-international.org/ecma-262/5.1/
 * Sample Javascript module uses the embeddable JS Engine Duktape
 * https://duktape.org 
 **/

/** Copyright (c) 2019 Mesibo
 * https://mesibo.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the terms and condition mentioned
 * on https://mesibo.com as well as following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions, the following disclaimer and links to documentation and
 * source code repository.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of Mesibo nor the names of its contributors may be used to
 * endorse or promote products derived from this software without specific prior
 * written permission.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Documentation
 * https://mesibo.com/documentation/on-premise/loadable-modules
 *
 * Source Code Repository
 * https://github.com/mesibo/onpremise-loadable-modules
 *
 * Skeleton Module
 * https://github.com/mesibo/onpremise-loadable-modules/skeleton
 *
 */
#include <stdio.h>
#include <string.h>
#include "module.h"
#include "duktape.h"
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define HTTP_BUFFER_LEN (64 * 1024)
#define MODULE_LOG_LEVEL_0VERRIDE 0

/** Message Parmeters **/
#define MESSAGE_AID 				"aid"
#define MESSAGE_ID 				"id"
#define MESSAGE_REFID 				"refid"
#define MESSAGE_GROUPID 			"groupid"
#define MESSAGE_FLAGS 				"flags"
#define MESSAGE_TYPE 				"type"
#define MESSAGE_EXPIRY 				"expiry"
#define MESSAGE_TO_ONLINE 			"to_online"
#define MESSAGE_TO 				"to"
#define MESSAGE_FROM 				"from"

/** HTTP options **/
#define HTTP_PROXY 				"proxy"
#define HTTP_CONTENT_TYPE 			"content_type"
#define HTTP_EXTRA_HEADER 			"extra_header"
#define HTTP_USER_AGENT 			"user_agent"
#define HTTP_REFERRER 				"referrer"
#define HTTP_ORIGIN 				"origin"
#define HTTP_COOKIE 				"cookie"
#define HTTP_ENCODING 				"encoding"
#define HTTP_CACHE_CONTROL 			"cache_control"
#define HTTP_ACCEPT 				"accept"
#define HTTP_ETAG 				"etag"
#define HTTP_IMS 				"ims"
#define HTTP_MAXREDIRECTS 			"maxredirects"
#define HTTP_CONN_TIMEOUT 			"conn_timeout"
#define HTTP_HEADER_TIMEOUT 			"header_timeout"
#define HTTP_BODY_TIMEOUT 			"body_timeout"
#define HTTP_TOTAL_TIMEOUT 			"total_timeout"
#define HTTP_RETRIES 				"retries"
#define HTTP_SYNCHRONOUS 			"synchronous"

/** Listener functions **/
#define MESIBO_LISTENER_ON_MESSAGE 		"Mesibo_onMessage"
#define MESIBO_LISTENER_ON_MESSAGE_STATUS 	"Mesibo_onMessageStatus"

/** Mesibo module Helper functions **/
#define MESIBO_MODULE_MESSAGE                   "mesibo_message"
#define MESIBO_MODULE_HTTP                      "mesibo_http"
#define MESIBO_MODULE_LOG 			"mesibo_log"  

/**
 * Sample Javascript Module Configuration
 * Refer sample.conf
 */
typedef struct js_config_s{
	const char* script;
	int log; //log level
	long last_changed; // Time Stamp
	duk_context* ctx; // JavaScript file context
} js_config_t;

typedef struct http_context_s {
	mesibo_module_t *mod;
	char buffer[HTTP_BUFFER_LEN];
	int datalen;	
	mesibo_message_params_t *params;

	char *http_cb;
	char *http_cbdata;
	module_http_option_t* http_opt;
} http_context_t;

/** Mesibo Module helper functions callable from Javscript**/
static duk_ret_t mesibo_js_http(duk_context *ctx);
static duk_ret_t mesibo_js_message(duk_context *ctx);
static duk_ret_t mesibo_js_log(duk_context *ctx);


static mesibo_int_t  js_http(mesibo_module_t *mod, const char *url, const char *post,
		const char *cb, const char *cbdata,
		module_http_option_t *opt);
static mesibo_int_t  js_on_message_status(mesibo_module_t *mod,
		mesibo_message_params_t *p,
		mesibo_uint_t status);
static mesibo_int_t  js_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len);

static int mesibo_http_callback(void *cbdata, mesibo_int_t state,
		mesibo_int_t progress, const char *buffer,
		mesibo_int_t size);
static mesibo_int_t  notify_mesibo_on_http_response(mesibo_module_t *mod,
		const char *cb_data, const char *cb,
		const char *response, int rp_len);
static mesibo_int_t  notify_mesibo_on_message_status(mesibo_module_t *mod,
		mesibo_message_params_t *p,
		mesibo_uint_t status);
static mesibo_int_t  notify_mesibo_on_message(mesibo_module_t *mod,
		mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len);

/** Utility Functions to load parameters, get parameters, etc **/
static duk_context *mesibo_duk_get_context(mesibo_module_t* mod); 

void mesibo_duk_pushparams(mesibo_module_t *mod, duk_context *ctx,
		duk_idx_t obj_idx, mesibo_message_params_t *p) {

	duk_push_uint(ctx, p->aid);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_AID);

	duk_push_uint(ctx, p->id);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_ID);

	duk_push_uint(ctx, p->refid);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_REFID);

	duk_push_uint(ctx, p->groupid);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_GROUPID);

	duk_push_uint(ctx, p->flags);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_FLAGS);

	duk_push_uint(ctx, p->type);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_TYPE);

	duk_push_uint(ctx, p->expiry);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_EXPIRY);

	duk_push_uint(ctx, p->to_online);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_TO_ONLINE);

	duk_push_string(ctx, p->from);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_FROM);

	duk_push_string(ctx, p->to);
	duk_put_prop_string(ctx, obj_idx, MESSAGE_TO);

}

static const char *mesibo_duk_get_param_string(duk_context *ctx, duk_idx_t obj_idx,
		const char *param) {

	if (duk_get_prop_string(ctx, obj_idx, param)) {  // Param key exists
		const char *param_value = duk_to_string(ctx, -1);
		return param_value;
	}
	duk_pop(ctx);

	return NULL;  // Default to empty string;
}

static mesibo_uint_t mesibo_duk_get_param_uint(duk_context *ctx, duk_idx_t obj_idx,
		const char *param) {

	if (duk_get_prop_string(ctx, obj_idx, param)) {  // Param key exists
		mesibo_uint_t param_value = duk_to_uint(ctx, -1);
		return param_value;
	}
	duk_pop(ctx);

	return 0;  // Default to NULL;
}

void mesibo_duk_getparams(duk_context *ctx, duk_idx_t obj_idx,
		mesibo_message_params_t *p) {

	p->aid = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_AID);
	p->id = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_ID);
	p->refid = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_REFID);
	p->groupid = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_GROUPID);
	p->flags = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_FLAGS);
	p->type = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_TYPE);
	p->expiry = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_EXPIRY);
	p->to_online = mesibo_duk_get_param_uint(ctx, obj_idx, MESSAGE_TO_ONLINE);
	p->to = strdup(mesibo_duk_get_param_string(ctx, obj_idx, MESSAGE_TO));
	p->from = strdup(mesibo_duk_get_param_string(ctx, obj_idx, MESSAGE_FROM));
}

void mesibo_duk_getoptions(duk_context *ctx, duk_idx_t obj_idx,
		module_http_option_t *opt) {

	opt->proxy = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_PROXY);
	opt->content_type = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_CONTENT_TYPE);
	opt->extra_header = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_EXTRA_HEADER);
	opt->user_agent = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_USER_AGENT);
	opt->referrer = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_REFERRER);
	opt->origin = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_ORIGIN);
	opt->cookie = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_COOKIE);
	opt->encoding = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_ENCODING);
	opt->cache_control = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_CACHE_CONTROL);
	opt->accept = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_ACCEPT);
	opt->etag = mesibo_duk_get_param_string(ctx, obj_idx, HTTP_ETAG);
	opt->ims = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_IMS);
	opt->maxredirects = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_MAXREDIRECTS);
	opt->conn_timeout = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_CONN_TIMEOUT);
	opt->header_timeout = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_HEADER_TIMEOUT);
	opt->body_timeout = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_BODY_TIMEOUT);
	opt->total_timeout = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_TOTAL_TIMEOUT);
	opt->retries = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_RETRIES);
	opt->synchronous = mesibo_duk_get_param_uint(ctx, obj_idx, HTTP_SYNCHRONOUS);
}

/**
 * Load Mesibo Module helper functions to be callable from Javscript
 **/
void mesibo_duk_load_functions(duk_context *ctx) {

	duk_push_c_function(ctx, mesibo_js_http, 6);
	duk_put_global_string(ctx, MESIBO_MODULE_HTTP);

	duk_push_c_function(ctx, mesibo_js_message, 4);
	duk_put_global_string(ctx, MESIBO_MODULE_MESSAGE );

	duk_push_c_function(ctx, mesibo_js_log, DUK_VARARGS);
	duk_put_global_string(ctx, MESIBO_MODULE_LOG);
}

/**
 * Get the timestamp of the last made change in script
 **/

static long get_last_changed(mesibo_module_t* mod, const char *path) {
	js_config_t* jsc = (js_config_t*)mod->ctx;
	struct stat attr;
	if(stat(path, &attr) != 0){
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "Error in reading file: %s, get_last_changed failed \n", path);
		return 0;
	}
	return attr.st_mtime;
}

/**
 * Open file and load it into the duktape context (push file buffer into stack)
 * https://wiki.duktape.org/gettingstartedlineprocessing
 * For brevity assumes a maximum file length of 16kB. 
 **/
static int push_file_as_string(duk_context *ctx, const char *filename) {

	FILE *f;
	size_t len;
	char buf[16384];

	f = fopen(filename, "rb");
	if (f) {
		len = fread((void *)buf, 1, sizeof(buf), f);
		fclose(f);
		duk_push_lstring(ctx, (const char *)buf, (duk_size_t)len);
	} else {
		duk_push_undefined(ctx);
		return -1;
	}
	return 0;
}

/**
 * Create and initialize duktape context by loading the script provided
 * Loads Mesibo Module helper functions to be made callable from the script
 * 
 * Note: The context may be reused with every function call. 
 * So, to prevent any context conflicts avoid the usage of global variables in your script 
 **/ 
static duk_context* mesibo_duk_create_context(mesibo_module_t* mod){

	js_config_t* jsc = (js_config_t*)mod->ctx;

	duk_context *ctx = duk_create_heap_default();

	if (push_file_as_string(ctx, jsc->script) != 0) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, " Error loading script: %s\n", jsc->script);
		return NULL;
	}

	if (duk_peval(ctx) != 0) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, " Error running: %s\n", duk_safe_to_string(ctx, -1));
		return NULL;
	}
	duk_pop(ctx); /* ignore result */
	mesibo_duk_load_functions(ctx);

	return ctx;
}

/**
 * Reinitialize context if the script is changed
 * Returns duktape context
 **/
static duk_context *mesibo_duk_get_context(mesibo_module_t* mod) {

	js_config_t* jsc = (js_config_t*)mod->ctx;

	mesibo_log(mod, jsc->log, "mesibo_duk_get_context called\n");

	if(jsc == NULL){
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "%s : Bad configuration. mesibo_duk_get_context failed\n",
				mod->name);
		return NULL;
	}
	long latest_last_changed = get_last_changed(mod, jsc->script);

	//last_changed will be zero if file is loaded for the first time 
	if((jsc->last_changed == 0 )|| (jsc->last_changed == latest_last_changed)){
		//No file change
		if(jsc->ctx != NULL)
			return jsc->ctx;
		else {
			jsc->ctx = mesibo_duk_create_context(mod); //File to be loaded for first time
			jsc->last_changed = latest_last_changed;
			mod->ctx = (void*)jsc;  //Update mod ctx with new JS ctx

		}
	}

	/* File has changed */
	else { 
		mesibo_log(mod, jsc->log , "File has changed \n");
		if(jsc->ctx)
			duk_destroy_heap(jsc->ctx); //Clear existing context
		jsc->ctx = mesibo_duk_create_context(mod); //Creates and evaluates script , loads functions
		jsc->last_changed = latest_last_changed;
	}
	return jsc->ctx;
}

/**
 * mesibo_http helper callable from Javascript
 * Parameters from js are convert into native C data types
 *
 **/
static duk_ret_t mesibo_js_http(duk_context *ctx) {

	int n = duk_get_top(ctx); /* #args */
	if (n != 6) {
		/* throw TypeError if incorrect arguments given */
		return DUK_RET_TYPE_ERROR;
	}

	mesibo_module_t *mod = (mesibo_module_t *)duk_to_pointer(
			ctx, 0);  // Mesibo Module object pointer
	js_config_t* jsc = (js_config_t*)mod->ctx;
	mesibo_log(mod, jsc->log, " %s called \n", "mesibo_js_http");

	const char *url = duk_to_string(ctx, 1);     // Base URL string
	const char *post = duk_to_string(ctx, 2);    // POST data
	const char *duk_cb = duk_to_string(ctx, 3);  // Callback function name
	const char *duk_cbdata =
		duk_to_string(ctx, 4);  // A string object Eg; JSON string

	module_http_option_t *opt =
		(module_http_option_t *)calloc(1, sizeof(module_http_option_t));

	mesibo_duk_getoptions(ctx, 5, opt);  // JSON object

	mesibo_log(mod, jsc->log, "\n ===> Sending http %s %s %s %s %s %s \n", url,
			opt->extra_header, opt->content_type, post, duk_cb, duk_cbdata);
	js_http(mod, url, post, duk_cb, duk_cbdata, opt);

	return MESIBO_RESULT_OK;
}

/**
 * mesibo_message helper callable from Javascript
 * Parameters from js are convert into native C data types
 *
 **/
static duk_ret_t mesibo_js_message(duk_context *ctx) {

	int n = duk_get_top(ctx); /* #args */
	if (n == 0) {
		/* throw TypeError if no arguments given */
		return DUK_RET_TYPE_ERROR;
	}

	mesibo_module_t *mod = (mesibo_module_t *)duk_to_pointer(ctx, 0);
	js_config_t* jsc = (js_config_t*)mod->ctx;

	mesibo_message_params_t *p =
		(mesibo_message_params_t *)calloc(1, sizeof(mesibo_message_params_t));
	mesibo_duk_getparams(ctx, 1, p);

	const char *message = duk_to_string(ctx, 2);
	mesibo_uint_t len = duk_to_uint(ctx, 3);

	mesibo_log(mod, jsc->log, "\n %s \n", "Sending Message");
	mesibo_log(mod, jsc->log, "aid %u from %s to %s id %u message %.*s\n", p->aid, p->from,
			p->to, (uint32_t)p->id, len, message);

	mesibo_message(mod, p, message, len);

	free(p);
	return MESIBO_RESULT_OK;
}

/**
 * mesibo_log helper callable from Javascript
 * Parameters from js are convert into native C data types
 *
 **/
static duk_ret_t mesibo_js_log(duk_context *ctx) {

	int n = duk_get_top(ctx); /* #args */
	if (n == 0) {
		/* throw TypeError if no arguments given */
		return DUK_RET_TYPE_ERROR;
	}
	mesibo_module_t *mod = (mesibo_module_t *)duk_to_pointer(ctx, 0);
	js_config_t* jsc = (js_config_t*)mod->ctx;

	const char *log_string = duk_to_string(ctx, 1);
	int datalen = duk_to_int(ctx, 2);
	mesibo_log(mod, jsc->log, " %.*s \n", datalen, log_string);

	return MESIBO_RESULT_OK;
}

/**
 * Call HTTP callback defined in Javascript
 * Push all the parameters into duktape context stack and then evaluate
 *
 **/

static mesibo_int_t notify_mesibo_on_http_response(mesibo_module_t *mod,
		const char *cb_data, const char *cb,
		const char *response, int rp_len) {

	js_config_t* jsc = (js_config_t*)mod->ctx;
	mesibo_log(mod, jsc->log, " ===> notify_mesibo_on_http_response called\n");

	duk_int_t rc;
	duk_context *ctx = mesibo_duk_get_context(mod); 

	if (!ctx) {
		mesibo_log(mod, jsc->log, "Load context failed %s ", cb);
		return MESIBO_RESULT_FAIL;
	}

	duk_get_global_string(ctx, cb);

	duk_push_pointer(ctx, (void *)mod);
	duk_push_string(ctx, cb_data);
	duk_push_string(ctx, response);
	duk_push_uint(ctx, rp_len);

	rc = duk_pcall(ctx, 4);
	if (rc != 0) {
		mesibo_log(mod, jsc->log, "Response Callback failed: %s\n",
				duk_safe_to_string(ctx, -1));
	} else {
		mesibo_log(mod, jsc->log, "Response Callback success\n");
	}
	duk_pop(ctx); /* pop result */
	return rc;
}

void mesibo_js_destroy_http_context(http_context_t* mc){

	free(mc->http_cb);
	free(mc->http_cbdata);
	free(mc->http_cbdata);
	free(mc);
}

/**
 * HTTP Callback function
 * Response to a POST request is recieved through this callback
 * Once the complete response is received it is sent the callback defined in Javascipt
 */
static int mesibo_http_callback(void *cbdata, mesibo_int_t state,
		mesibo_int_t progress, const char *buffer,
		mesibo_int_t size) {

	http_context_t *b = (http_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;
	js_config_t* jsc = (js_config_t*)mod->ctx;


	if (0 > progress) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, " Error in http callback \n");
		mesibo_js_destroy_http_context(b);
		return MESIBO_RESULT_FAIL;
	}

	if (MODULE_HTTP_STATE_RESPBODY != state) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, " Exit http callback \n");
		if(size)
			mesibo_log(mod, jsc->log, "%.*s \n", size, buffer);
		return MESIBO_RESULT_OK;
	}

	if ((0 < progress) && (MODULE_HTTP_STATE_RESPBODY == state)) {
		if(HTTP_BUFFER_LEN < (b->datalen + size )){
			mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "%s :"
					" Error http callback :Buffer overflow detected \n", mod->name);
			mesibo_js_destroy_http_context(b);
			return MESIBO_RESULT_FAIL;
		}
		memcpy(b->buffer + b->datalen, buffer, size);
		b->datalen += size;
	}

	if (100 == progress) {
		mesibo_log(mod, jsc->log, "%.*s", b->datalen, b->buffer);
		mesibo_log(mod, jsc->log, "\n JS Callback %s %s \n", b->http_cbdata, b->http_cb);
		notify_mesibo_on_http_response(mod, b->http_cbdata, b->http_cb, b->buffer,
				b->datalen);

		mesibo_js_destroy_http_context(b);
	}

	return MESIBO_RESULT_OK;
}

/**
 * Cleanup on Module exit
 **/
static mesibo_int_t js_cleanup(mesibo_module_t *mod) {

	js_config_t* jsc = (js_config_t*)mod->ctx;
	if(jsc->ctx)
		duk_destroy_heap(jsc->ctx);
	return MESIBO_RESULT_OK;
}

/**
 * Make an HTTP request 
 **/
static mesibo_int_t  js_http(mesibo_module_t *mod, const char *url, const char *post,
		const char *cb, const char *cbdata,
		module_http_option_t *opt) {

	js_config_t* jsc = (js_config_t*)mod->ctx;

	mesibo_log(mod, jsc->log, "js_http called \n");
	http_context_t *b = (http_context_t *)calloc(1, sizeof(http_context_t));

	b->mod = mod;
	b->http_cb = strdup(cb);
	b->http_cbdata = strdup(cbdata);
	b->http_opt = opt;

	mesibo_log(mod, jsc->log, " ===> http POST data %s %s %s %s \n", HTTP_EXTRA_HEADER,
			opt->extra_header, HTTP_CONTENT_TYPE, opt->content_type);
	mesibo_log(mod, jsc->log, "%s %s \n", url, post);
	mesibo_log(mod, jsc->log, " HTTP Callback context stored %s %s \n", b->http_cbdata,
			b->http_cb);

	mesibo_http(mod, url, post, mesibo_http_callback, (void *)b, opt);
	return MESIBO_RESULT_OK;
}

/**
 * Module Callback function for on_message_status
 * This function is called when a message is sent from the module to indicate the status of the message.
 * Notify Javascript callback Mesibo_onMessageStatus
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_message_status
 **/
static mesibo_int_t  js_on_message_status(mesibo_module_t *mod,
		mesibo_message_params_t *p,
		mesibo_uint_t status) {

	js_config_t* jsc = (js_config_t*)mod->ctx;

	mesibo_log(mod, jsc->log, "%s on_message_status called\n", mod->name);
	mesibo_log(mod, jsc->log, "from %s id %u status %d\n", p->from, (uint32_t)p->id,
			status);
	int result = notify_mesibo_on_message_status(mod, p, status);
	return result;  
}

/**
 * Module Callback function for on_message
 * This function is called when a user sends a message.
 * Notify Javascript callback Mesibo_onMessage
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_message
 **/
static mesibo_int_t  js_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {

	js_config_t* jsc = (js_config_t*)mod->ctx;

	int result = notify_mesibo_on_message(mod, p, message, len);
	return result;  // Either MESIBO_RESULT_CONSUMED or MESIBO_RESULT_PASS
}

/**
 * Call JS Callback function Mesibo_onMessageStatus
 * Push parmeters as objects into the duktape stack and evaluate
 **/
static mesibo_int_t  notify_mesibo_on_message_status(mesibo_module_t *mod,
		mesibo_message_params_t *p,
		mesibo_uint_t status) {

	js_config_t* jsc = (js_config_t*)mod->ctx;

	duk_context *ctx = mesibo_duk_get_context(mod);
	if (!ctx) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "%s :Error: Load context failed ",
				MESIBO_LISTENER_ON_MESSAGE_STATUS);
		return MESIBO_RESULT_FAIL;
	}

	//Get callback function reference from Javascript
	duk_get_global_string(ctx, MESIBO_LISTENER_ON_MESSAGE_STATUS);

	//Push parameters
	duk_push_pointer(ctx, (void *)mod);
	duk_idx_t obj_idx = duk_push_object(ctx);
	mesibo_duk_pushparams(mod, ctx, obj_idx, p);
	duk_push_uint(ctx, status);

	//Evaluate JS function in the script
	duk_pcall(ctx, 3);

	//Obtain return value
	int ret = duk_to_int(ctx, -1);  

	duk_pop(ctx);
	return ret;
}

/**
 * Call JS Callback function Mesibo_onMessage
 * Push parmeters as objects into the duktape stack and evaluate
 **/
static mesibo_int_t  notify_mesibo_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {

	duk_context *ctx = mesibo_duk_get_context(mod);
	js_config_t* jsc = (js_config_t*)mod->ctx;

	if (!ctx) {
		mesibo_log(mod, jsc->log, "%s : Error: Load context failed", MESIBO_LISTENER_ON_MESSAGE);
		return -1;
	}

	//Get callback function reference from Javscript
	duk_get_global_string(ctx, MESIBO_LISTENER_ON_MESSAGE);

	//Push parmeters
	duk_push_pointer(ctx, (void *)mod);
	duk_idx_t obj_idx = duk_push_object(ctx);

	mesibo_duk_pushparams(mod, ctx, obj_idx, p);
	duk_push_string(ctx, message);
	duk_push_int(ctx, len);
	duk_push_int(ctx, 5);

	//Evaluate JS function in the script
	duk_pcall(ctx, 5);

	int ret =
		duk_to_int(ctx, -1);  // Return value can either be
	// MESIBO_RESULT_CONSUMED or MESIBO_RESULT_PASS


	duk_pop(ctx);
	return ret;
}

/**
 * Helper function for getting JS configuration
 * Gets /path/to/script which contains Javascript code
 **/
js_config_t* get_config_js(mesibo_module_t* mod){

	js_config_t* jsc = (js_config_t*)calloc(1, sizeof(js_config_t));
	jsc->script = mesibo_util_getconfig(mod, "script");
	jsc->log = atoi(mesibo_util_getconfig(mod, "log"));
	jsc->last_changed = 0; //Initialize TS to zero
	jsc->ctx = NULL;
	mesibo_log(mod, jsc->log, "Javascript Module Configured : script %s log %d\n", jsc->script, jsc->log);

	return jsc;
}

/**
 * Module Initialization function
 * Verifies module definition, reads configuration and Inititializes callback functions 
 */
int mesibo_module_js_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {

	MESIBO_MODULE_SANITY_CHECK(m, version, len);

	if(m->config) {
		js_config_t* jsc = get_config_js(m);
		if(NULL == jsc){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		m->ctx = (void* )jsc;
		jsc->ctx = mesibo_duk_get_context(m);  
		if (!jsc->ctx) {
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "Loading context failed for script %s", jsc->script);
			return MESIBO_RESULT_FAIL;
		}
	}

	else {
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
		return MESIBO_RESULT_FAIL;
	}

	m->flags = 0;
	m->name = strdup("Sample JS Module");
	m->on_cleanup = js_cleanup;
	m->on_message = js_on_message;
	m->on_message_status = js_on_message_status;

	return MESIBO_RESULT_OK;
}

