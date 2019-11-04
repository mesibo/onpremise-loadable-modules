/** 
 * File: chatbot.c 
 * Description: 
 * Chatbot module to analyze messages using various AI and machine learning tools 
 * like Tensorflow, Dialogflow, etc. and send an automatic reply. 
 * Sample Chatbot Module provides an example using Dialogflow
 * https://dialogflow.com
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
 * Chatbot Module
 * https://github.com/mesibo/onpremise-loadable-modules/chatbot
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module.h"

#define HTTP_BUFFER_LEN (64 * 1024)
#define HTTP_POST_URL_LEN_MAX (1024)
#define MODULE_LOG_LEVEL_0VERRIDE 0

/**
 * Sample Chatbot Module Configuration
 * Refer sample.conf
 */
typedef struct chatbot_config_s {
	/* To be configured in module configuration file */
	const char* project;
	const char* endpoint;
	const char* access_token;
	const char* language;
	const char* address;
	int log;

	/* To be configured by Dialogflow init function */
	char* post_url;
	char* auth_bearer;
	module_http_option_t* chatbot_http_opt;

} chatbot_config_t;

typedef struct http_context_s {
	mesibo_module_t *mod;
	mesibo_message_params_t* params;
	char *from;
	char *to;
	// To copy data in response
	char buffer[HTTP_BUFFER_LEN];
	int datalen;

	char* post_data; //For cleanup after HTTP request is complete	
} http_context_t;

void mesibo_chatbot_destroy_http_context(http_context_t* mc){
	free(mc->from);
	free(mc->to);
	free(mc->params);
	free(mc->post_data);
	free(mc);
}

/**
 * HTTP Callback function
 * Response from Dialogflow is recieved through this callback
 * Once the complete response is received it is sent back to the sender of the query as an automatic reply
 */
static int chatbot_http_callback(void *cbdata, mesibo_int_t state,
		mesibo_int_t progress, const char *buffer,
		mesibo_int_t size) {
	http_context_t *b = (http_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;
	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	mesibo_message_params_t* params = b->params;

	if (0 > progress) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "Error in http callback \n");
		mesibo_chatbot_destroy_http_context(b);
		return MESIBO_RESULT_FAIL;
	}

	if (MODULE_HTTP_STATE_RESPBODY != state) {
		if(size)
			mesibo_log(mod, cbc->log, "Exit http callback %.*s \n", size, buffer);
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
		mesibo_log(mod, cbc->log, "%.*s", b->datalen, b->buffer);
		mesibo_message_params_t p;
		memset(&p, 0, sizeof(mesibo_message_params_t));
		p.id = rand();
		p.refid = params->id;
		p.aid = params->aid;
		p.from = b->to;
		p.to = b->from; // User adress who sent the query is the recipient
		p.expiry = 3600;

		char* extracted_response = mesibo_util_json_extract(b->buffer , "fulfillmentText", NULL);

		mesibo_message(mod, &p, extracted_response , strlen(extracted_response));
		mesibo_chatbot_destroy_http_context(b);

		return MESIBO_RESULT_OK;
	}

	return MESIBO_RESULT_OK;
}

/**
 * Helper function to initialize HTTP options
 **/
static module_http_option_t* mesibo_chatbot_get_http_opt(chatbot_config_t* cbc){
	module_http_option_t *request_options =
		(module_http_option_t *)calloc(1, sizeof(module_http_option_t));

	request_options->extra_header = cbc->auth_bearer;
	request_options->content_type = "application/json";
	request_options->synchronous = 0;	
	return request_options;
}

/**
 * Reads configuration parameters and initializes Dialogflow REST API parameters
 * Constructs base URL for sending POST request
 * Dialogflow V2 API reference https://cloud.google.com/dialogflow/docs
 * POST
 * https://dialogflow.googleapis.com/v2beta1/{session=projects/<project>/agent/sessions/<session id>}:detectIntent
 *
 * Constructs Authentication header using access_token(service account key) provided in configuration
 * https://cloud.google.com/docs/authentication/
 */
static int chatbot_init_dialogflow(mesibo_module_t* mod){
	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	asprintf(&cbc->post_url, "%s/projects/%s/agent/sessions",
			cbc->endpoint, cbc->project);
	mesibo_log(mod, cbc->log, "Configured post URL for HTTP requests: %s \n", cbc->post_url);

	asprintf(&cbc->auth_bearer, "Authorization: Bearer %s", cbc->access_token);
	mesibo_log(mod, cbc->log, "Configured auth bearer for HTTP requests with token: %s \n", cbc->auth_bearer );

	cbc->chatbot_http_opt = mesibo_chatbot_get_http_opt(cbc); 

	return MESIBO_RESULT_OK;
}

/**
 * Passes the message text receieved into the queryInput params in the POST data  
 * Constructs raw POST data 
 * Makes an HTTP request to dialogflow service
 * The response to the request will be received in the callback function chatbot_http_callback
 */
static int chatbot_process_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {
	(mod, "chatbot_process_message", p);
	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	char post_url[HTTP_POST_URL_LEN_MAX];
	sprintf(post_url, "%s/%lu:detectIntent", cbc->post_url, p->id); //Pass Message ID as Session ID
	char* raw_post_data; 
	asprintf(&raw_post_data, "{\"queryInput\":{\"text\":{\"text\":\"%.*s\", \"languageCode\":\"%s\"}}}",
			(int)len, message, cbc->language);

	http_context_t *http_context =
		(http_context_t *)calloc(1, sizeof(http_context_t));
	http_context->mod = mod;
	http_context->params = p;
	http_context->from = strdup(p->from);
	http_context->to = strdup(p->to);
	http_context->post_data = raw_post_data;

	mesibo_log(mod, cbc->log , "%s %s %s %s \n", post_url, raw_post_data, cbc->chatbot_http_opt->extra_header,
			cbc->chatbot_http_opt->content_type);

	mesibo_http(mod, post_url, raw_post_data, chatbot_http_callback,
			(void *)http_context, cbc->chatbot_http_opt);

	return MESIBO_RESULT_OK;
}

/**
 * Callback function to on_message
 * Called when any users sends a Message TO a particular user identified by mesibo user-id as the chatbot endpoint
 * The chatbot-uid is passed in the configuration
 * Refer sample.conf
 */
static mesibo_int_t chatbot_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {

	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	if(0 == strcmp(p->to, cbc->address)){
		// Don't modify original as other module will use it
		mesibo_message_params_t* np = (mesibo_message_params_t*)malloc(sizeof(mesibo_message_params_t)); 
		memcpy(np, p, sizeof(mesibo_message_params_t));
		chatbot_process_message(mod, np, message, len);

		return MESIBO_RESULT_CONSUMED;  // Process the message and CONSUME original
	}

	return MESIBO_RESULT_PASS;
}

/**
 * Helper function for getting chatbot configuration
 * Here, it gets the required configuration for dialogflow based chatbot
 */
static chatbot_config_t*  get_config_dialogflow(mesibo_module_t* mod){
	chatbot_config_t* cbc = (chatbot_config_t*)calloc(1, sizeof(chatbot_config_t));
	cbc->project = mesibo_util_getconfig(mod, "project");
	cbc->endpoint = mesibo_util_getconfig(mod, "endpoint");
	cbc->access_token = mesibo_util_getconfig(mod, "access_token");
	cbc->language = mesibo_util_getconfig(mod, "language");
	cbc->address = mesibo_util_getconfig(mod, "address");
	cbc->log = atoi(mesibo_util_getconfig(mod, "log"));

	mesibo_log(mod, cbc->log, "Configured DialogFlow :\nproject %s\nendpoint %s\naccess_token %s\n"
			"language %s\naddress %s\n", cbc->project, cbc->endpoint, 
			cbc->access_token, cbc->language, cbc->address);

	return cbc;
}

/**
 * Cleanup once Module work is complete
 *
 **/ 
static  mesibo_int_t  chatbot_on_cleanup(mesibo_module_t* mod){
	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;
	free(cbc->post_url);
	free(cbc->auth_bearer);
	free(cbc->chatbot_http_opt);
	free(cbc);

	return MESIBO_RESULT_OK;
}
/**
 * Module Initialization function
 * Verifies module definition, reads configurSERVICE ACation and Inititializes callback functions 
 */
int mesibo_module_chatbot_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
	mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "mesibo_module_%s_init called \n", m->name);

	MESIBO_MODULE_SANITY_CHECK(m, version, len);

	m->flags = 0;
	m->description = strdup("Sample Chatbot Module");
	m->on_message = chatbot_on_message;


	if(m->config) {
		chatbot_config_t* cbc = get_config_dialogflow(m);
		if(NULL == cbc){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		m->ctx = (void* )cbc;

		int init_status = chatbot_init_dialogflow(m);
		if(MESIBO_RESULT_OK != init_status){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Bad Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}

	}

	else {
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
		return MESIBO_RESULT_FAIL;
	}



	return MESIBO_RESULT_OK;
}

