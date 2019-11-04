/** 
 * File: skeleton.c 
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

/**
 * Function: mesibo_module_http_data_callback
 * ------------------------------------------
 * Example HTTP callback function, through which response is received
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#http
 *
 */

int mesibo_module_http_data_callback(void *cbdata, mesibo_int_t state, mesibo_int_t progress, const char *buffer, mesibo_int_t size) {
	mesibo_module_t *mod = (mesibo_module_t *)cbdata;
	mesibo_log(mod, 1, " http progress %d\n", progress);
	return MESIBO_RESULT_OK;
}

/*
 * Function: skeleton_http
 * ----------------------
 * Example for making an HTTP request
 * Refer https://mesibo.com/documentation/on-premise/loadable-modules/#http
 *
 */
static mesibo_int_t skeleton_http(mesibo_module_t *mod) {
	mesibo_http(mod, "https://example.com/api.php", "op=test", mesibo_module_http_data_callback, mod, NULL);
	return MESIBO_RESULT_OK;
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
static mesibo_int_t skeleton_on_message_status(mesibo_module_t *mod, mesibo_message_params_t *p, mesibo_uint_t status) {
	mesibo_log(mod, 0, " %s on_message_status called\n", mod->name);
	mesibo_log(mod, 0, " from %s id %u status %d\n", p->from, (uint32_t)p->id, status);
	return 0;
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

static mesibo_int_t skeleton_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, const char *message, mesibo_uint_t len) {
	mesibo_log(mod, 0, "================> %s on_message called\n", mod->name);
	mesibo_log(mod, 0, " from %s to %s id %u message %s\n", p->from, p->to, (uint32_t) p->id, message);
	
	//don't modify original as other module will be use it 
	mesibo_message_params_t	np;
	memcpy(&np, p, sizeof(mesibo_message_params_t));
	np.to = p->from;
	np.from = p->to;
	np.id = rand();
	const char* test_message = "Hello from Skeleton Module";

	mesibo_message(mod, &np, test_message, strlen(message));
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

int mesibo_module_skeleton_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
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
