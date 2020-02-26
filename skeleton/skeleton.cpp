/** 
 * File: skeleton.cpp 
 * Description: Bare bones version of a Mesibo Module.
 * Explains the usage of different aspects of the module, various callback functions, callable functions and utilities.
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
 * Skeleton Module
 * https://github.com/mesibo/onpremise-loadable-modules/skeleton
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module.h"
#include <assert.h>

#define MESIBO_FLAG_DELIVERYRECEIPT     0x1
#define MESIBO_FLAG_READRECEIPT         0x2

#define HTTP_BUFFER_LEN (64 * 1024)
#define HTTP_RESPONSE_TYPE_LEN (1024)

/*
 *
 * EXAMPLE HTTP
 *
 *
 */

/**Http Context **/
typedef struct http_context_s {
        mesibo_module_t *mod;
	mesibo_int_t status;
        char response_type[HTTP_RESPONSE_TYPE_LEN];
        // To copy data in response
        char buffer[HTTP_BUFFER_LEN];
        int datalen;
} http_context_t;

/**
 * Function: mesibo_http_on_data_callback
 * ------------------------------------------
 * Example HTTP callback function, through which response is received
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#http
 *
 */
static mesibo_int_t mesibo_http_on_data_callback(void *cbdata, mesibo_int_t state, mesibo_int_t progress, const char *buffer, mesibo_int_t size) {
	http_context_t *b = (http_context_t*)cbdata;
	mesibo_module_t *mod = b->mod;
	
	mesibo_log(mod, 0, " http progress %d\n", progress);
	if (progress < 0) {
		mesibo_log(mod, 0, " Error in http callback \n");

		// cleanup

		return -1;
	}

	if (state != MODULE_HTTP_STATE_RESPBODY) {
		return 0;
	}


	memcpy(b->buffer + b->datalen, buffer, size);
	b->datalen += size;

	if (progress == 100) {
		// process it ...
		mesibo_log(mod, 0 ,"%.*s", b->datalen, b->buffer);
	}



	return MESIBO_RESULT_OK;
}

/**
 * Function: mesibo_http_on_status_callback
 * ------------------------------------------
 * Example HTTP callback function, through which response status and type is received
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#http
 *
 */
mesibo_int_t mesibo_http_on_status_callback(void *cbdata, mesibo_int_t status, const char *response_type){

	http_context_t *b = (http_context_t *)cbdata;
	if(!b) return MESIBO_RESULT_FAIL;
	mesibo_module_t* mod = b->mod;
	if(!mod) return MESIBO_RESULT_FAIL;

	b->status = status;
	if(NULL != response_type){
		memcpy(b->response_type, response_type, strlen(response_type));
		mesibo_log(mod, 0, "status: %d, response_type: %s \n", (int)status, response_type); 
	}
	
	return MESIBO_RESULT_OK;
}

/**
 * Function: mesibo_http_on_close_callback
 * ------------------------------------------
 * Example HTTP callback function, called when HTTP connection is closed 
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#http
 *
 */
void mesibo_http_on_close_callback(void *cbdata,  mesibo_int_t result){
	//Complete response and close connection

}
/*
 * Function: skeleton_http
 * ----------------------
 * Example for making an HTTP request
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#http
 *
 */
static mesibo_int_t skeleton_http(mesibo_module_t *mod) {
	fprintf(stderr, "Skeleton HTTP called \n");
	http_context_t* cbdata = (http_context_t*)calloc(1, sizeof(http_context_t));
	cbdata->mod = mod;
	mesibo_http_t req;
	memset(&req, 0, sizeof(req));
	req.url = "https://app.mesibo.com/api.php"; //API endpoint
	req.post = "op=userdel&token=123434343xxxxxxxxx&uid=456"; // POST Request Data
	req.on_data = mesibo_http_on_data_callback;
	req.on_status = mesibo_http_on_status_callback;
	req.on_close = mesibo_http_on_close_callback;
	mesibo_util_http(&req, cbdata);
	
	return MESIBO_RESULT_OK;
}

/*
 *
 * EXAMPLE SOCKET
 *
 *
 */

/*Socket Context*/
typedef struct socket_context_s{
        mesibo_module_t* mod;
}socket_context_t;


/** Socket Callbacks **/
mesibo_int_t mesibo_socket_ondata(void *cbdata, const char *data, mesibo_int_t len){
	
	socket_context_t *b = (socket_context_t*)cbdata;
	mesibo_module_t* mod = b->mod;
	
	if(len)	
		mesibo_log(mod, 0, "Received socket data: %.*s", len, data);
	
	return MESIBO_RESULT_OK;
}

void mesibo_socket_onconnect(void *cbdata, mesibo_int_t asock, mesibo_int_t fd){
	const char* test_data = "GET / HTTP/1.0\r\nHost: example.com\r\nConnection: close\r\n\r\n";
	mesibo_int_t len = strlen(test_data);
	mesibo_util_socket_write(asock, test_data, len);
}

void mesibo_socket_onwrite(void *cbdata){
	socket_context_t *b = (socket_context_t*)cbdata;
	mesibo_module_t* mod = b->mod;
	//Process callback for onwrite
	mesibo_log(mod, 0, "%s called \n", "mesibo_socket_onwrite");
}

void mesibo_socket_onclose(void *cbdata, mesibo_int_t type){
	//Process callback for onclose
	socket_context_t *b = (socket_context_t*)cbdata;
	mesibo_module_t* mod = b->mod;
	//Process callback for onwrite
	mesibo_log(mod, 0, "%s called, type: %d \n", "mesibo_socket_onclose", (int)type);
}

/*
 * Function: skeleton_socket
 * ----------------------
 * Example for Socket I/O 
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#socket
 *
 */
static mesibo_int_t skeleton_socket(mesibo_module_t *mod) {

	socket_context_t* cbdata = (socket_context_t*)calloc(1, sizeof(socket_context_t));
	cbdata->mod = mod;
	mesibo_socket_t sock;
	memset(&sock, 0, sizeof(sock));
	sock.url = "socks://example.com";
	//sock.url = "ws://echo.websocket.org";
	sock.on_connect = mesibo_socket_onconnect;
	sock.on_write = mesibo_socket_onwrite;
	sock.on_data = mesibo_socket_ondata;
	sock.on_close = mesibo_socket_onclose;

	mesibo_int_t rv = mesibo_util_socket_connect(&sock, cbdata);
	mesibo_log(mod, 0 , "connect returned %d\n", rv);
	return rv;
}


/*
 * Function: skeleton_on_cleanup
 * -----------------------------
 * This function is called when the module process is complete and to clean up.
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_cleanup
 *
 */
static mesibo_int_t skeleton_on_cleanup(mesibo_module_t *mod) {
	return MESIBO_RESULT_OK;
}


/*
 * Function: skeleton_on_login
 * ---------------------------
 * This function is called whenever a user logs-in or logs-out.
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_login
 *
 */
static mesibo_int_t skeleton_on_login(mesibo_module_t *mod, mesibo_user_t *user) {
	mesibo_log(mod, 1, " %s on_login called\n", mod->name);
	return 0;
}

/* 
 * Function: skeleton_on_message_status
 * ------------------------------------
 * This function is called whenever there is a status update for the messages sent from the module
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_message_status
 * 
 */
static mesibo_int_t skeleton_on_message_status(mesibo_module_t *mod, mesibo_message_params_t *p) {
	mesibo_log(mod, 0, " %s on_message_status called\n", mod->name);
	mesibo_log(mod, 0, " from %s id %u status 0x%x\n", p->from, (uint32_t)p->id, p->status);
	return 0;
}


static mesibo_int_t skeleton_modify_message(mesibo_module_t *mod, char *message, 
		mesibo_uint_t len, const char* modified_message ){
	
	
	//Modify message
	//Note that you are modifying the **ORIGINAL** message that was sent
	//You can modify the original sent message to hide or mask sensitive info like phone numbers & email,
	// hide profanity, etc
	if(!modified_message)
		return MESIBO_RESULT_FAIL;
	
	//The length of the modified message **MUST** be lesser than or equal to that of
	//the original message that was sent	
	mesibo_uint_t mlen = (mesibo_uint_t)strlen(modified_message);
	if(mlen > len) {
		mlen = len;
	}
	//Copy the modified message contents to the location of original message
	memcpy(message, modified_message, mlen); 	
	if(mlen < len)
		memset(message+mlen, 0x20, len-mlen);

	return MESIBO_RESULT_OK;
}


static mesibo_int_t skeleton_send_automatic_reply(mesibo_module_t *mod, mesibo_message_params_t *p, 
		const char* message, mesibo_uint_t len,
	       	const char* automatic_reply){
	//You can send automatic replies from a chatbot, translator, etc.
	// For example, In case of a chatbot, you need to process the original message(query) 
	// and then send an appropriate response
	//To do this you consume the original message that was sent, and send a new message with new data params
	
	if(!automatic_reply) return MESIBO_RESULT_FAIL;
	if(!p) return MESIBO_RESULT_FAIL;

	mesibo_message_params_t	np;
	memcpy(&np, p, sizeof(mesibo_message_params_t));
	
	//Switch TO and FROM	
	if(!p->from) return MESIBO_RESULT_FAIL;
	np.to = p->from;
	
	if(!p->to) return MESIBO_RESULT_FAIL;
	np.from = p->to;
	np.flags = MESIBO_FLAG_DELIVERYRECEIPT; 
	np.id = (uint32_t)rand();

	mesibo_log(mod, 0, " Sending an automatic reply from %s to %s id %u flags %08x message %s\n", 
			np.from, np.to, (uint32_t)np.id, np.flags, automatic_reply);
	mesibo_message(mod, &np, automatic_reply, len); 
	
	return MESIBO_RESULT_OK; 
}

/* 
 * Function: skeleton_on_message
 * -----------------------------
 * This function is called whenever a user sends a message. Module can process message and take appropriate action 
 * or can ignore as appropriate. Module can then indicate Mesibo whether to pass the message to the next 
 * module / recepient OR consume it by returning an appropriate value.
 *
 * Example for recieving the message and sending an automatic reply 
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#on_message
 */

static mesibo_int_t skeleton_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, char *message, mesibo_uint_t len) {
	mesibo_log(mod, 0, "================> %s on_message called\n", mod->name);
	mesibo_log(mod, 0, "from:%s to:%s \n", p->from, p->to);
	

	if(message)
		mesibo_log(mod, 0, " from %s to %s id %u message %.*s\n", p->from, p->to, (uint32_t) p->id, len, message);
	
	//Send an automatic reply like a chatbot	
	if(MESIBO_RESULT_OK == skeleton_send_automatic_reply(mod, p, message, len, "AUTOMATIC_REPLY"))
		return MESIBO_RESULT_CONSUMED;
	
	//Modified Message Sucessfully
	if(MESIBO_RESULT_OK == skeleton_modify_message(mod, message, len, "MODIFIED_MESSAGE"))
			return MESIBO_RESULT_PASS;
	
	
	//Send an HTTP request	
	skeleton_http(mod);
	
	//Make a socket connection
	skeleton_socket(mod);
	
	

	return MESIBO_RESULT_PASS;
}

/* 
 * Function: mesibo_module_skeleton_init 
 * -------------------------------------
 * Skeleton Module Initialization
 *
 * Example for reading configuration items and initialiszing callback functions 
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#module-initialization
 *
 */

MESIBO_EXPORT int mesibo_module_skeleton_init(mesibo_int_t version, mesibo_module_t *m, mesibo_int_t len) {
	MESIBO_MODULE_SANITY_CHECK(m, version, len);

	if(m->config) {
		mesibo_log(m, 0, "Following configuration item(s) were passed to the module\n");
		int i;
		for(i=0; i < m->config->count; i++) {
			mesibo_log(m, 0, "module config item: name %s value %s\n", 
					m->config->items[i].name, m->config->items[i].value);
		}
	}

	m->flags = 0;
	m->description = strdup("Sample Skeleton Module");

	//Initialize Callback functions
	m->on_cleanup = skeleton_on_cleanup;
	m->on_message = skeleton_on_message;
	m->on_message_status = skeleton_on_message_status;
	m->on_login = skeleton_on_login;
	mesibo_log(m, 0, "================> %s init called\n", m->name);

	return MESIBO_RESULT_OK;

}
