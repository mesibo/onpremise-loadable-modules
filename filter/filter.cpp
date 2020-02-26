/** 
 * File: filter.cpp 
 * Description: Profinity filter module to drop messages containing profanity 
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
 * https://mesibo.com/documentation/on-premise/loadable-modules
 *
 * Source Code Repository
 * https://github.com/mesibo/onpremise-loadable-modules
 *
 * Filter Module
 * https://github.com/mesibo/onpremise-loadable-modules/filter
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "module.h"
#include <ctype.h>

#define MODULE_LOG_LEVEL_0VERRIDE 0

/**
 * Sample Filter Module configuration
 * Refer sample.conf
 * */
typedef struct filter_config_s{
	int  count;
	char** blocked_words;
	int log;
}filter_config_t;

/** Convert string input to UPPER_CASE **/
static char* mesibo_strupr(char* s){
	char* tmp = s;

	for(; *tmp; ++tmp){
		*tmp = toupper((unsigned char)*tmp);
	}
	return s;
}

/**
 * Callback function for on_message
 * Called when any user sends a message
 * Reads each message, loops through a list of blocked words and matches if any message string contains profanity 
 * 
 * Disclaimer: This is an extremely simplified implementation of a profanity filter.
 *
 */
static mesibo_int_t filter_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		char *message, mesibo_uint_t len) {

	char* in_message = strndup(message, len);
	in_message = mesibo_strupr(in_message) ; //Convert to UPPER_CASE
	filter_config_t* fc = (filter_config_t*)mod->ctx;
	
	mesibo_log(mod, fc->log, "%s\n", in_message);
	int i;
	for(i =0; i< fc->count ;i++){ //Loop through list of blocked words
		if(strstr(in_message, fc->blocked_words[i])){ //Message Contains blocked word 
			mesibo_log(mod, 0, "Message dropped. Contains profanity \n");
			free(in_message);
			//drop message and prevent message from  reaching the recipient
			return MESIBO_RESULT_CONSUMED; 
		}
	}

	free(in_message);
	return MESIBO_RESULT_PASS;  
	// PASS the message as it is, after checking that it is SAFE
}

/**Number of blocked words = Number of commas + 1 **/
static int get_count_words(const char* s, const char* c){
	int i = 0;
	for (i=0; '\0'!= s[i];s++)
		if(*c == s[i])
			i++;	
	
	return i+1;
}

/**
 * Helper function for getting filter configuration
 * Gets the list of comma seperated blocked words
 * Breaks the raw string and stores the words in an array
 */
static filter_config_t* get_config_filter(mesibo_module_t* mod){
	filter_config_t* fc = (filter_config_t*)calloc(1, sizeof(filter_config_t));
	char* bw = mesibo_util_getconfig(mod, "blocked_words"); //comma seperated blocked words	
	fc->log = atoi( mesibo_util_getconfig(mod, "log")); //loglevel
	
	const char* delimitter = ", "; // blocked_words: bw1, bw2, bw3, ...  
	fc->count = get_count_words(bw, delimitter);

	fc->blocked_words = (char**)malloc(sizeof(char *)*fc->count);
	
	int i = 0;	
	const char* token = strtok(bw, delimitter); 
	while(token != NULL ){
		fc->blocked_words[i] = mesibo_strupr(strdup(token));
		token = strtok(NULL, delimitter);
		i++;
	}
	return fc;
}

/**
 * Cleanup Memory on finishing work by the module
 */
static  mesibo_int_t  filter_on_cleanup(mesibo_module_t *mod){
	filter_config_t* fc = (filter_config_t*)mod->ctx;
	int i;
	for (i = 0; i < fc->count; i++) {
		free(fc->blocked_words[i]);
	}

	free(fc);

	return MESIBO_RESULT_OK;
}

/**
 * Module Initialization function
 * Verifies module definition, reads configuration and Inititializes callback functions 
 */

MESIBO_EXPORT int mesibo_module_filter_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
	MESIBO_MODULE_SANITY_CHECK(m, version, len);

	m->flags = 0;
	m->description = strdup("Sample Filter Module");
	m->on_message = filter_on_message;

	if(m->config){
		filter_config_t* fc = get_config_filter(m);
		if(fc == NULL){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		m->ctx = (void*)fc;
	}
	else {
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
		return MESIBO_RESULT_FAIL;
	}

	m->on_cleanup = filter_on_cleanup; //Cleanup memory on exit
	return MESIBO_RESULT_OK;
}
