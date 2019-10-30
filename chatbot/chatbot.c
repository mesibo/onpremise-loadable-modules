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
 * https://mesibo.com/documentation/loadable-modules
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
#define MODULE_LOG_LEVEL_0VERRIDE 0

/**
 * Sample Chatbot Module Configuration
 * Refer sample.conf
 */
typedef struct chatbot_config_s {
	/* To be configured in module configuration file */
	const char* project;
	const char* session;
	const char* endpoint;
	const char* access_token;
	const char* chatbot_uid;
	int log;

	/* To be configured by dialogflow init function */
	char* post_url;
	char* auth_bearer;
	module_http_option_t* chatbot_http_opt;

} chatbot_config_t;

typedef struct message_context_s {
	mesibo_module_t *mod;
	mesibo_message_params_t* params;
	char *from;
	char *to;
	// To copy data in response
	char buffer[HTTP_BUFFER_LEN];
	int datalen;

	char* post_data; //For cleanup after HTTP request is complete	
} message_context_t;

void mesibo_chatbot_destroy_message_context(message_context_t* mc){
	free(mc->params);
	free(mc->from);
	free(mc->to);
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
	message_context_t *b = (message_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;
	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	mesibo_message_params_t* params = b->params;

	if (progress < 0) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, " Error in http callback \n");
		mesibo_chatbot_destroy_message_context(b);
		return MESIBO_RESULT_FAIL;
	}

	if (state != MODULE_HTTP_STATE_RESPBODY) {
		if(size)
			mesibo_log(mod, cbc->log, " Exit http callback %.*s \n", size, buffer);
		return MESIBO_RESULT_OK;
	}

	if ((progress > 0) && (state == MODULE_HTTP_STATE_RESPBODY)) {
		memcpy(b->buffer + b->datalen, buffer, size);
		b->datalen += size;
	}

	if (progress == 100) {
		mesibo_log(mod, cbc->log, "%.*s", b->datalen, b->buffer);
		mesibo_message_params_t p;
		memset(&p, 0, sizeof(mesibo_message_params_t));
		p.id = rand();
		p.refid = params->id;
		p.aid = params->aid;
		p.from = params->to;
		p.to = params->from; // User adress who sent the query is the recipient
		p.expiry = 3600;

		char* extracted_response = mesibo_util_json_extract(b->buffer , "fulfillmentText", NULL);
		mesibo_log(mod, cbc->log, "\n Extracted Response Text \n %s \n", extracted_response); 		
		
		int mv = mesibo_message(mod, &p, extracted_response , strlen(extracted_response));
		mesibo_chatbot_destroy_message_context(b);
		
		return mv;
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
	mesibo_log(mod, cbc->log, "chatbot_init_dialogflow called \n");
	
	int cv;
        cv = asprintf(&cbc->post_url,"%s/projects/%s/agent/sessions/%s:detectIntent",
			cbc->endpoint, cbc->project, cbc->session);
	if(cv == -1)return MESIBO_RESULT_FAIL;
	mesibo_log(mod, cbc->log, "Configured post url for HTTP requests: %s \n", cbc->post_url);

	cv = asprintf(&cbc->auth_bearer,"Authorization: Bearer %s", cbc->access_token);
	if(cv == -1)return MESIBO_RESULT_FAIL;	
	mesibo_log(mod, cbc->log, "Configured auth bearer for HTTP requests with token: %s \n", cbc->auth_bearer );
	
	cbc->chatbot_http_opt = mesibo_chatbot_get_http_opt(cbc); 

	return MESIBO_RESULT_OK;
}

/**
 * Passes the message text receieved into the queryInput params in the POST data  
 * Constructs raw POST data and Authorization header
 * Makes an HTTP request to dialogflow service
 * The response to the request will be received in the callback function chatbot_http_callback
 */
static int chatbot_process_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {

	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	const char* post_url = cbc->post_url; 
	char* raw_post_data;
	int cv;
	cv = asprintf(&raw_post_data,"{\"queryInput\":{\"text\":{\"text\":\" %.*s \"}}}",
			(int)len, message);
	if(cv == -1) return MESIBO_RESULT_FAIL;

	message_context_t *message_context =
		(message_context_t *)calloc(1, sizeof(message_context_t));
	message_context->mod = mod;
	message_context->params = p;
	message_context->from = strdup(p->from);
	message_context->to = strdup(p->to);

	mesibo_log(mod, cbc->log, "POST request %s %s %s %s\n", post_url, raw_post_data, 
			cbc->chatbot_http_opt->extra_header, 
			cbc->chatbot_http_opt->content_type);

	mesibo_http(mod, post_url, raw_post_data, chatbot_http_callback,
			(void *)message_context, cbc->chatbot_http_opt);

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

	mesibo_log(mod, cbc->log, " %s_on_message called\n", mod->name);
	mesibo_log(mod, cbc->log, " aid %u from %s to %s id %u message %s\n", p->aid, p->from,
			p->to, (uint32_t)p->id, message);


	if(strcmp(p->to, cbc->chatbot_uid) == 0){
		// Don't modify original as other module will use it
		mesibo_message_params_t* np = (mesibo_message_params_t*)calloc(1, sizeof(mesibo_message_params_t)); 
		memcpy(np, p, sizeof(mesibo_message_params_t));
		int rv = chatbot_process_message(mod, np, message, len);
		
		if(rv == MESIBO_RESULT_OK) 
			return MESIBO_RESULT_CONSUMED;  // Process the message and CONSUME original
		else
			mesibo_log(mod, cbc->log, " %s_process_message failed\n", mod->name);

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
	cbc->session = mesibo_util_getconfig(mod, "session");
	cbc->endpoint = mesibo_util_getconfig(mod, "endpoint");
	cbc->access_token = mesibo_util_getconfig(mod, "access_token");
	cbc->chatbot_uid = mesibo_util_getconfig(mod, "chatbot_uid");
	cbc->log = atoi(mesibo_util_getconfig(mod, "log"));

	mesibo_log(mod, cbc->log, "Configured DialogFlow :\n project %s \n session %s \n endpoint %s\n access_token %s\n"
			"chatbot_uid %s\n", cbc->project, cbc->session, cbc->endpoint, 
			cbc->access_token, cbc->chatbot_uid);

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
 * Verifies module definition, reads configuration and Inititializes callback functions 
 */
int mesibo_module_chatbot_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
	mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "mesibo_module_%s_init called",m->name);

	MESIBO_MODULE_SANITY_CHECK(m, version, len);
	
	m->flags = 0;
        m->description = strdup("Sample Chatbot Module");
        m->on_message = chatbot_on_message;


	if(m->config) {
		chatbot_config_t* cbc = get_config_dialogflow(m);
		if(cbc  == NULL){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		m->ctx = (void* )cbc;

		int init_status = chatbot_init_dialogflow(m);
		if(init_status != MESIBO_RESULT_OK){
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

