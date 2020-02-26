/** 
 * File: v8_module.cpp 
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
 *
 */
#include "mesibo_js_processor.h"
#include <v8.h>
#include "v8_module.h"

#define MODULE_LOG_LEVEL_0VERRIDE 	 0
#define MODULE_CONFIG_LOG 		"log"
#define MODULE_CONFIG_SCRIPT 		"script"

//Single Global Platform Instance
std::unique_ptr<v8::Platform> gMesiboV8Platform ; //v8 Global Platform Initialization

MesiboJsProcessor* mesibo_v8_init(mesibo_module_t* mod, v8_config_t* vc){

	if(gMesiboV8Platform.get() == NULL){
		//First call to v8. Initialize global platform 	
		gMesiboV8Platform = v8::platform::NewDefaultPlatform();
		v8::V8::InitializePlatform(gMesiboV8Platform.get());
		v8::V8::Initialize();
	}

	v8::Isolate::CreateParams createParams;
	createParams.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	v8::Isolate* isolate = v8::Isolate::New(createParams);

	mesibo_log(mod, vc->log,"Running V8 version %s\n", v8::V8::GetVersion());

	const char* script_path = strdup(vc->script);
	int log_level = vc->log;

	MesiboJsProcessor* mesibo_js = new MesiboJsProcessor(mod, script_path, log_level);
	mesibo_js->SetIsolate(isolate);
	
	if (MESIBO_RESULT_FAIL == mesibo_js->Initialize()){
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "Failed to intitialize MesiboJsProcessor\n");
		return NULL;
	}

	return mesibo_js;
}
/**
 * Helper function for getting v8 module configuration
 * Gets /path/to/script which contains Javascript code
 **/
v8_config_t* get_config_v8(mesibo_module_t* mod){

	v8_config_t* vc = (v8_config_t*)calloc(1, sizeof(v8_config_t));
	vc->script = mesibo_util_getconfig(mod, MODULE_CONFIG_SCRIPT);
	vc->log = atoi(mesibo_util_getconfig(mod, MODULE_CONFIG_LOG));
	vc->last_changed = 0; //Initialize TS to zero
	vc->ctx = NULL;
	mesibo_log(mod, vc->log, "V8 Module Configurations - %s: %s , %s:%d\n", MODULE_CONFIG_SCRIPT, vc->script, 
			MODULE_CONFIG_LOG, vc->log);

	return vc;
}


MESIBO_EXPORT int mesibo_module_v8_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
	MESIBO_MODULE_SANITY_CHECK(m, version, len);

	if(!m->config){
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
		return MESIBO_RESULT_FAIL;
	}

	v8_config_t* vc = get_config_v8(m);
	if(!vc){
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
		return MESIBO_RESULT_FAIL;
	}

	MesiboJsProcessor* mesibo_js = mesibo_v8_init(m, vc);

	if(!mesibo_js){
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Invalid Initialization\n", m->name);
		return MESIBO_RESULT_FAIL;
	}
	vc->ctx = mesibo_js;

	if (!vc->ctx) {
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "Loading context failed for script %s", 
				vc->script);
		return MESIBO_RESULT_FAIL;
	}

	m->ctx = (void*)vc;


	m->flags = 0;
	m->description = strdup("Sample V8 Module");
	m->on_message= v8_on_message;
	m->on_message_status = v8_on_message_status;

	return MESIBO_RESULT_OK;
}


