/** 
 * File: translate.c 
 * Description:
 * Translator module to translate each message before sending it to destination. 
 * Sample translate Module provides an example using Google Translate 
 * https://cloud.google.com/translate
 *
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
 * Translate Module
 * https://github.com/mesibo/onpremise-loadable-modules/translate
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module.h"

#define HTTP_BUFFER_LEN (64 * 1024)
#define MODULE_LOG_LEVEL_0VERRIDE 0

/**
 * Sample Translate Module Configuration
 * Refer sample.conf
**/
typedef struct translate_config_s {
	/* To be configured in module configuration file */
	const char* access_token;
	const char* endpoint;
	const char* source;
	const char* target;
	int log;

	/* To be configured by Google Translate init function */
	char* auth_bearer;
	module_http_option_t* translate_http_opt;

} translate_config_t;

/**Message Context **/
typedef struct http_context_s {
	mesibo_module_t *mod;
	mesibo_message_params_t *params;
	char *from;
	char *to;
	// To copy data in response
	char buffer[HTTP_BUFFER_LEN];
	int datalen;

	char* post_data; //For cleanup after HTTP request is complete
} http_context_t;


void mesibo_translate_destroy_http_context(http_context_t* mc){
        free(mc->from);
        free(mc->to);
	free(mc->params);
        free(mc->post_data);
        free(mc);
}

/**
 * HTTP Callback function
 * Response from Google Translate is recieved through this callback
 * Once the complete response is received the translated message is sent to the recipient
 */
static int translate_http_callback(void *cbdata, mesibo_int_t state,
		mesibo_int_t progress, const char *buffer,
		mesibo_int_t size) {
	http_context_t *b = (http_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;
	translate_config_t* tc = (translate_config_t*)mod->ctx;
	mesibo_message_params_t *params = b->params;

	if (0 > progress) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE,  "Error in http callback \n");
		mesibo_translate_destroy_http_context(b);
		return MESIBO_RESULT_FAIL;
	}

	if (MODULE_HTTP_STATE_RESPBODY != state) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "Exit http callback\n");
		if(size)
			mesibo_log(mod, tc->log,  "%.*s \n", size, buffer);
		return MESIBO_RESULT_OK;
	}

	if ((0 < progress) && (MODULE_HTTP_STATE_RESPBODY == state)) {
		if(HTTP_BUFFER_LEN < (b->datalen + size )){
			mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, 
					"Error in http callback : Buffer overflow detected \n", mod->name);
			return MESIBO_RESULT_FAIL;
		}

		memcpy(b->buffer + b->datalen, buffer, size);
		b->datalen += size;
	}

	if (100 == progress) {
		mesibo_log(mod, tc->log,  "%.*s", b->datalen, b->buffer);
		mesibo_message_params_t p;
                memset(&p, 0, sizeof(mesibo_message_params_t));
                p.id = rand();
                p.refid = params->id;
                p.aid = params->aid;
                p.from = b->from;
                p.to = b->to; 
                p.expiry = 3600;

		char* extracted_response = mesibo_util_json_extract( b->buffer , "translatedText", NULL);
		mesibo_log(mod, tc->log,  "\n Extracted Response Text \n %s \n", extracted_response);
		mesibo_message(mod, &p, extracted_response , strlen(extracted_response));
		
		mesibo_translate_destroy_http_context(b);	
	}

	return MESIBO_RESULT_OK;
}

/**
 * Helper function to initialize HTTP options
**/
static module_http_option_t* mesibo_translate_get_http_opt(translate_config_t* tc){
	tc->translate_http_opt = (module_http_option_t *)calloc(1, sizeof(module_http_option_t));
        tc->translate_http_opt->extra_header = tc->auth_bearer;
        tc->translate_http_opt->content_type = "application/json";
}
/**
 * Reads configuration parameters and initializes Google Translate(Cloud Translate Service) REST API parameters
 * Constructs Authentication header using access_token(service account key) provided in configuration
 * https://cloud.google.com/docs/authentication/
**/
static int translate_init_google(mesibo_module_t* mod){
	translate_config_t* tc = (translate_config_t*)mod->ctx;
        
	asprintf(&tc->auth_bearer, "Authorization: Bearer %s", tc->access_token);
        mesibo_log(mod, tc->log, "Configured auth bearer for HTTP requests with token: %s \n", tc->auth_bearer );

        tc->translate_http_opt = mesibo_translate_get_http_opt(tc);
	
	return MESIBO_RESULT_OK;
}

/**
 * Passes the message text receieved into the the POST data  
 * Constructs raw POST data and Authorization header
 * Makes an HTTP request to Cloud Translate service
 * The response to the request will be received in the callback function translate_http_callback
**/
static int translate_process_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {
	
	translate_config_t* tc = (translate_config_t*)mod->ctx;
	const char* post_url = tc->endpoint; 
	
	char* raw_post_data;
        asprintf(&raw_post_data, "{\"q\":\"%.*s\", \"target\":\"%s\"}",
                        (int)len, message, tc->target);
	
	http_context_t *http_context =
		(http_context_t *)calloc(1, sizeof(http_context_t));
	http_context->mod = mod;
	http_context->params = p;
	http_context->from = strdup(p->from);
	http_context->to = strdup(p->to);
	http_context->post_data= raw_post_data;


	mesibo_log(mod, tc->log,  "POST request %s %s %s %s \n", 
			post_url, raw_post_data,
		       	tc->translate_http_opt->extra_header, 
			tc->translate_http_opt->content_type);
	
	mesibo_http(mod, post_url, raw_post_data, translate_http_callback , (void *)http_context, tc->translate_http_opt);
	
	return MESIBO_RESULT_OK;
}



/**
 * Callback function to on_message
 * Called when any user sends a Message 
**/
static mesibo_int_t translate_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {
	
	translate_config_t* tc = (translate_config_t*)mod->ctx;
	
	mesibo_message_params_t *np = (mesibo_message_params_t *)calloc(1, sizeof(mesibo_message_params_t));
	memcpy(np, p, sizeof(mesibo_message_params_t));
	translate_process_message(mod, np, message, len);
	
	return MESIBO_RESULT_CONSUMED;  // Process the message and CONSUME original
}

/**
 * Helper function for getting translate configuration
 * Here, it gets the required configuration for Google Translate
**/
static translate_config_t*  get_config_google_translate(mesibo_module_t* mod){
	translate_config_t* tc = (translate_config_t*)calloc(1, sizeof(translate_config_t));
	tc->access_token = mesibo_util_getconfig(mod, "access_token");
	tc->endpoint = mesibo_util_getconfig(mod, "endpoint");
	tc->source = mesibo_util_getconfig(mod, "source");
	tc->target = mesibo_util_getconfig(mod, "target");
	tc->log = atoi(mesibo_util_getconfig(mod, "log"));

	mesibo_log(mod, tc->log,  " Configured Google Translate :\n endpoint %s \n"
			" source %s \n target %s\n access_token %s\n", 
			tc->endpoint, tc->source, tc->target, tc->access_token);
	mod->ctx = tc;

	return tc;
}

/**
 * Cleanup once Module work is complete
 *
 **/
static  mesibo_int_t  translate_on_cleanup(mesibo_module_t* mod){
        translate_config_t* tc = (translate_config_t*)mod->ctx;
        free(tc->auth_bearer);
        free(tc->translate_http_opt);
        free(tc);

        return MESIBO_RESULT_OK;
}

/**
 * Module Initialization function
 * Verifies module definition, reads configuration and Inititializes callback functions 
**/
int mesibo_module_translate_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {

	MESIBO_MODULE_SANITY_CHECK(m, version, len);
	m->flags = 0;
        m->description = strdup("Sample Translate Module");
        m->on_message = translate_on_message;
	m->on_cleanup = translate_on_cleanup;
	
	if(m->config) {
		translate_config_t* tc = get_config_google_translate(m);
		if(NULL == tc){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		int init_status= translate_init_google(m);
		if(MESIBO_RESULT_OK != init_status){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Bad Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}

		m->ctx = (void* )tc;
	}

	else {
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
		return MESIBO_RESULT_FAIL;
	}


	return MESIBO_RESULT_OK;
}
