/** 
 * File: fcgi.cpp 
 * Description: Minimal FCGI Module 
 *
 * */

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
 * https://mesibo.com/documentation/on-premise/loadable-modules/ 
 *
 * Source Code Repository
 * http://github.com/mesibo/onpremise-loadable-modules
 * 
 * FCGI Module
 * https://github.com/mesibo/onpremise-loadable-modules/fcgi
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module.h"


/**
 * Sample FCGI Module Configuration
 * Refer sample.conf
 */
typedef struct fcgi_config_s{
	const char* host; 
        mesibo_int_t port;
        mesibo_int_t keepalive; 
        const char* root;
        const char* script;
	mesibo_uint_t log;
} fcgi_config_t;

//For logging-errors and exceptions
#define MODULE_LOG_LEVEL_OVERRIDE 0

/**FCGI request Context **/
typedef struct fcgi_context_s {
        mesibo_module_t *mod;
	mesibo_message_params_t params;
} fcgi_context_t;

/*
 * Function: fcgi_on_cleanup
 * -----------------------------
 * This function is called when the fcgi module process is complete and to clean up.
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_cleanup
 *
 */
static mesibo_int_t fcgi_on_cleanup(mesibo_module_t *mod) {
	return MESIBO_RESULT_OK;
}


/*
 * Function: fcgi_on_login
 * ---------------------------
 * This function is called whenever a user logs-in or logs-out.
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_login
 *
 */
static mesibo_int_t fcgi_on_login(mesibo_module_t *mod, mesibo_user_t *user) {
	return MESIBO_RESULT_OK;
}

/* 
 * Function: fcgi_on_message_status
 * ------------------------------------
 * This function is called whenever there is a status update for the messages sent from the module
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_message_status
 * 
 */
static mesibo_int_t fcgi_on_message_status(mesibo_module_t *mod, mesibo_message_params_t *p) {
	fcgi_config_t* fc = (fcgi_config_t*)mod->ctx;

	mesibo_log(mod, fc->log , " %s on_message_status called\n", mod->name);
	mesibo_log(mod, fc->log,  " from %s id %u status %d\n", p->from, (uint32_t)p->id, (int)p->status);
	return MESIBO_RESULT_OK;
}

/**
 * Function: mesibo_fcgi_data_callback_
 * ------------------------------------------
 * Example FCGI callback function, through which response is received
 * Sends the response back to the user who made the FCGI request
 */
mesibo_int_t mesibo_fcgi_data_callback(void *cbdata, mesibo_int_t result, const char *buffer, mesibo_int_t size){
	
	fcgi_context_t *b = (fcgi_context_t*)cbdata;
	mesibo_module_t *mod = b->mod;
	fcgi_config_t* fc = (fcgi_config_t*)mod->ctx;
	
	if(MESIBO_RESULT_FAIL == result){
		mesibo_log(mod, MODULE_LOG_LEVEL_OVERRIDE, "Bad response to fcgi request\n");
		return MESIBO_RESULT_FAIL;
	}

	mesibo_log(mod, fc->log, "%.*s\n", size, buffer);

	//Send response to the requester
	mesibo_message_params_t	np;
	memset(&np, 0, sizeof(mesibo_message_params_t));
	
	mesibo_message_params_t request_params = b->params;	
	np.to =  request_params.from; 
	np.from = request_params.to; 
	np.id = rand();

	mesibo_message(mod, &np, buffer, size);
	
	return MESIBO_RESULT_OK;
}

static mesibo_int_t fcgi_request(mesibo_module_t *mod, mesibo_message_params_t p, 
		char *message, mesibo_uint_t len) {
	
	fcgi_config_t* fc = (fcgi_config_t*)mod->ctx;
        mesibo_log(mod, fc->log, "fcgi_request called %s %s\n", fc->root, fc->script);
        	
	mesibo_fcgi_t req;
	memset(&req, 0, sizeof(mesibo_fcgi_t));
	req.USER = p.from;
	req.DOCUMENT_ROOT = strdup(fc->root);
	req.SCRIPT_NAME = strdup(fc->script);
	req.body = message;
	req.bodylen = (uint64_t)len;

        mesibo_log(mod, fc->log, "Request parameters %s %s %s %s %u\n", req.USER, req.DOCUMENT_ROOT,
		 req.SCRIPT_NAME, req.body, req.bodylen );
	
	fcgi_context_t* cbdata  = (fcgi_context_t*)calloc(1, sizeof(fcgi_context_t));
	cbdata->mod = mod;
	cbdata->params = p; //Copy message parmeters to fcgi call back context 
	cbdata->params.from = strdup(p.from);
	cbdata->params.to = strdup(p.to);

	mesibo_log(mod, fc->log, "Request parameter object %p\n", &req);
	mesibo_log(mod, fc->log , "host %s, port %lld, keepalive %lld, cb %p, cbdata %p", 
			fc->host, fc->port, fc->keepalive,
			mesibo_fcgi_data_callback, cbdata);
	
	//Host, Port, Keepalive parmeters need to be in configuration
	mesibo_util_fcgi(&req, fc->host, fc->port, fc->keepalive, mesibo_fcgi_data_callback , (void*)cbdata);

	return MESIBO_RESULT_OK;
}
/* 
 * Function: fcgi_on_message
 * -----------------------------
 * This function is called whenever a user sends a message. Module can process message and take appropriate action 
 * or can ignore as appropriate. Module can then indicate Mesibo whether to pass the message to the next 
 * module / recepient OR consume it by returning an appropriate value.
 *
 * Example for recieving the message and sending an automatic reply 
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_message
 */

static mesibo_int_t fcgi_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, char *message, mesibo_uint_t len) {
	mesibo_log(mod, 0, "================> %s on_message called\n", mod->name);
	mesibo_log(mod, 0, " from %s to %s id %u message %s\n", p->from, p->to, (uint32_t) p->id, message);
	
	mesibo_message_params_t np;
	memset(&np, 0 ,sizeof(mesibo_message_params_t));
	memcpy(&np, p, sizeof(mesibo_message_params_t));
	fcgi_request(mod, np, strdup(message), len);	
	return MESIBO_RESULT_PASS; 
}

/**
 * Helper function for getting FCGI configuration
 *
 **/
fcgi_config_t* get_config_fcgi(mesibo_module_t* mod){

        fcgi_config_t* fc = (fcgi_config_t*)calloc(1, sizeof(fcgi_config_t));
        fc->host = mesibo_util_getconfig(mod, "host");
        fc->port = atoi(mesibo_util_getconfig(mod, "port"));
        fc->keepalive = atoi(mesibo_util_getconfig(mod, "keepalive"));
        fc->root = mesibo_util_getconfig(mod, "root");
	fc->script = mesibo_util_getconfig(mod, "script");
        fc->log = atoi(mesibo_util_getconfig(mod, "log"));
        
	mesibo_log(mod, fc->log, "fcgi Module Configured :host %s port %u keepalive %d"
			" root %s script %s log %d\n",
		       	fc->host, (uint16_t)fc->port, fc->keepalive, 
			fc->root, fc->script, fc->log);

        return fc;
}

/* 
 * Function: mesibo_module_fcgi_init 
 * -------------------------------------
 * Skeleton Module Initialization
 *
 * Example for reading configuration items and initialiszing callback functions 
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#module-initialization
 *
 */

int mesibo_module_fcgi_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
	MESIBO_MODULE_SANITY_CHECK(m, version, len);

	if(m->config) {
		fcgi_config_t* fc = get_config_fcgi(m);
		if(NULL == fc){
			mesibo_log(m, MODULE_LOG_LEVEL_OVERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		m->ctx = (void* )fc;
	}

	m->flags = 0;
	m->description = strdup("Sample FCGI Module");
	
	//Initialize Callback functions
	m->on_cleanup = fcgi_on_cleanup;
	m->on_message = fcgi_on_message;
	m->on_message_status = fcgi_on_message_status;
	m->on_login = fcgi_on_login;
	mesibo_log(m, 0, "================> %s init called\n", m->name);

	return MESIBO_RESULT_OK;

}
