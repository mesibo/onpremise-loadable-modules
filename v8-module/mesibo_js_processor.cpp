/** 
 * File: v8.cpp 
 * Mesibo V8 Interface Standalone
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
 *
 */

#include "module.h"
#include "mesibo_js_processor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <iostream>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <string>


Global<ObjectTemplate> MesiboJsProcessor::message_template_;
Global<ObjectTemplate>  MesiboJsProcessor::http_template_;
Global<ObjectTemplate> MesiboJsProcessor::socket_template_;

//Get the timestamp of the last made change in script
long GetLastChanged(const char* script){
	struct stat attr;
	if(!stat(script, &attr) != 0){
		return attr.st_mtime;
	}
	return MESIBO_RESULT_FAIL;
}

/**
 * Recompile if the script is changed.
 * Returns newly allocated context object derived from base context
 * 
 **/
Local<Context> MesiboJsProcessor::GetContext() {

	EscapableHandleScope handle_scope(GetIsolate());

	long latestchange = GetLastChanged(script_);
	if(MESIBO_RESULT_FAIL == latestchange)
		MesiboJsDebug::Log("Error stating file %s \n", script_);

	//lastchanged will be zero if file is loaded for the first time 
	bool isFileChange = (lastchanged_!= 0) && (lastchanged_ != latestchange); 
	lastchanged_ = latestchange;

	int init_status = 0; 
	if(isFileChange){ 
		MesiboJsDebug::Log("File has changed \n");
		init_status = Initialize(); // Recompile & Reinitialize 
		
	}


	if(MESIBO_RESULT_FAIL == init_status)
		MesiboJsDebug::Log("Error initializing context\n");

	return handle_scope.Escape(Local<Context>::New(GetIsolate(), context_));
}

Local<Function> MesiboJsProcessor::GetGlobalFunction(const Local<Context>& context, const char* func_name){

	Isolate::Scope isolateScope(GetIsolate());

	// Create a handle scope to keep the temporary object references.
	EscapableHandleScope handle_scope(GetIsolate());

	Local<Function> js_func;

	if(!func_name)
		return Local<Function>::Cast(v8::Null(GetIsolate())); 

	Local<Value> mesibo_global_ref;
	//The mesibo global object has a prototype like
	// let mesibo = function(){
	//onmessage = function(params, message, len){};
	//onmessagestatus = function(){}; //optional
	//onlogin = function(){}; //optional
	// 
	//xxx: Validate global mesibo prototype and store it 
	if (!context->Global()->Get(context, 
				v8::String::NewFromUtf8(GetIsolate(), 
					JS_MESIBO_GLOBAL_OBJECT)
				.ToLocalChecked())
			.ToLocal(&mesibo_global_ref) 
			|| !mesibo_global_ref->IsObject()) { 
		//Invalid Global Mesibo Object
		MesiboJsDebug::ErrorLog("Invalid Global Object: %s\n", JS_MESIBO_GLOBAL_OBJECT); 
		return Local<Function>::Cast(v8::Null(GetIsolate())); 
	}

	//Absolutely sure this is a valid object 
	Local<Object> mesibo_global_obj = Local<Object>::Cast(mesibo_global_ref);
	Local<Value> mesibo_func_ref;
	if (!mesibo_global_obj->Get(context,v8::String::NewFromUtf8(GetIsolate()
					, func_name )
				.ToLocalChecked())
			.ToLocal(&mesibo_func_ref) 
			|| !mesibo_func_ref->IsFunction()) { 
		//xxx:Check if the function matches the defined signature
		return Local<Function>::Cast(v8::Null(GetIsolate())); 
	}

	//Absolutely sure this is a valid function reference	
	Local<Function> mesibo_func = Local<Function>::Cast(mesibo_func_ref);
	return handle_scope.Escape(mesibo_func);
}	

int MesiboJsProcessor::ExecuteScript(Local<String> script, Local<ObjectTemplate> global) {

	HandleScope handle_scope(GetIsolate());

	// We're just about to compile the script; set up an error handler to
	// catch any exceptions the script might throw.
	TryCatch try_catch(GetIsolate());

	// Each module script gets its own context so different script calls don't
	// affect each other. Context::New returns a persistent handle which
	// is what we need for the reference to remain after we return from
	// this method. 

	Local<Context> context = Context::New(GetIsolate(), NULL, global);
	context_.Reset(GetIsolate(), context);

	// Enter the new context so all the following operations take place within it.
	Context::Scope context_scope(context);

	// Compile the script and check for errors.
	Local<Script> compiled_script;
	if (!Script::Compile(context, script).ToLocal(&compiled_script)) {
		MesiboJsDebug::ReportException(GetIsolate(), &try_catch);
		// The script failed to compile; bail out.
		return MESIBO_RESULT_FAIL;
	}

	// Run the script
	Local<Value> result;
	if (!compiled_script->Run(context).ToLocal(&result)) {
		// The TryCatch above is still in effect and will have caught the error.
		MesiboJsDebug::ReportException(GetIsolate(), &try_catch);
		// Running the script failed; bail out.
		return MESIBO_RESULT_FAIL;
	}
	
	//Initialize global listeners
	//mandatory to define on_message
	js_mesibo_on_message.Reset(GetIsolate(), GetGlobalFunction(context,
				JS_MESIBO_LISTENER_ON_MESSAGE));
	if(js_mesibo_on_message.IsEmpty())
		return MESIBO_RESULT_FAIL;	
	
	//Optional
	js_mesibo_on_message_status.Reset(GetIsolate(), GetGlobalFunction(context,
				JS_MESIBO_LISTENER_ON_MESSAGE_STATUS));
	js_mesibo_on_login.Reset(GetIsolate(), GetGlobalFunction(context,
				JS_MESIBO_LISTENER_ON_LOGIN));

	return MESIBO_RESULT_OK;
}
void MesiboJsProcessor::SetCallables(Local<ObjectTemplate> & global){

	global->Set(String::NewFromUtf8(GetIsolate(), JS_MESIBO_CLASS_MESSAGE, NewStringType::kNormal)
			.ToLocalChecked(),
			FunctionTemplate::New(GetIsolate(),
				MesiboJsMessage::MessageClassCallback, 
				External::New(GetIsolate(), (void*)mod_)));

	if(message_template_.IsEmpty()){	
		// Fetch the template for creating JavaScript Message wrappers.
		// It only has to be created once, which we do on demand.
		Local<ObjectTemplate> raw_template = MesiboJsMessage::MakeMessageTemplate(GetIsolate(), mod_);
		message_template_.Reset(GetIsolate(), raw_template);
	}


	global->Set(String::NewFromUtf8(GetIsolate(), JS_MESIBO_CLASS_HTTP, NewStringType::kNormal)
			.ToLocalChecked(),
			FunctionTemplate::New(GetIsolate(),
				MesiboJsHttp::HttpClassCallback, External::New(GetIsolate(), (void*)mod_)));
	if(http_template_.IsEmpty()){	
		// Fetch the template for creating JavaScript HTTP request wrappers.
		// It only has to be created once, which we do on demand.
		Local<ObjectTemplate> raw_template = MesiboJsHttp::MakeHttpTemplate(GetIsolate(), mod_);
		http_template_.Reset(GetIsolate(), raw_template);
	}



	global->Set(String::NewFromUtf8(GetIsolate(), JS_MESIBO_CLASS_SOCKET, NewStringType::kNormal)
			.ToLocalChecked(),
			FunctionTemplate::New(GetIsolate(),
				MesiboJsSocket::SocketClassCallback, External::New(GetIsolate(), (void*)mod_)));

	if(socket_template_.IsEmpty()){	
		// Fetch the template for creating JavaScript Socket wrappers.
		// It only has to be created once, which we do on demand.
		Local<ObjectTemplate> raw_template = MesiboJsSocket::MakeSocketTemplate(GetIsolate(), mod_);
		socket_template_.Reset(GetIsolate(), raw_template);
	}
}

int MesiboJsProcessor::Initialize(){
	// Create a handle scope to hold the temporary references.
	HandleScope handle_scope(GetIsolate());

	// Create a template for the global object where we set the
	// built-in global functions.
	Local<ObjectTemplate> global = ObjectTemplate::New(GetIsolate());
	SetCallables(global);

	//xxx:Create Object template only once and then use it	
	v8::Local<v8::ObjectTemplate> mesibo_global = v8::ObjectTemplate::New(GetIsolate());

	//Global JS Callbacks
	mesibo_global->Set(GetIsolate(), JS_MESIBO_LISTENER_ON_MESSAGE, v8::Undefined(GetIsolate()));	
	mesibo_global->Set(GetIsolate(), JS_MESIBO_LISTENER_ON_MESSAGE_STATUS, v8::Undefined(GetIsolate()));	
	mesibo_global->Set(GetIsolate(), JS_MESIBO_LISTENER_ON_LOGIN, v8::Undefined(GetIsolate()));	

	//Global JS Callables
	mesibo_global->Set(String::NewFromUtf8(GetIsolate(), JS_MESIBO_LOG, NewStringType::kNormal)
			.ToLocalChecked(),
			FunctionTemplate::New(GetIsolate(),
				MesiboJsDebug::LogCallback, External::New(GetIsolate(), (void*)mod_)));

	// Codes & flags 
	mesibo_global->Set(GetIsolate(), JS_MESIBO_RESULT_OK, 
			v8::Number::New(GetIsolate(), MESIBO_RESULT_OK));	
	mesibo_global->Set(GetIsolate(), JS_MESIBO_RESULT_FAIL, 
			v8::Number::New(GetIsolate(), MESIBO_RESULT_FAIL));	

	//Global JS Mesibo Object		
	global->Set(String::NewFromUtf8(GetIsolate(), JS_MESIBO_GLOBAL_OBJECT, NewStringType::kNormal).ToLocalChecked(), 
			mesibo_global);

	if(!script_)
		return MESIBO_RESULT_FAIL;

	if( access( script_, F_OK ) == -1 ){
		MesiboJsDebug::ErrorLog("%s : Script does not exist in the current path \n", script_);
		return MESIBO_RESULT_FAIL;
	}


	Local<String> source = ReadFile(GetIsolate(), script_).ToLocalChecked();
	if(source->IsNullOrUndefined()){
		MesiboJsDebug::ErrorLog("Error reading file %s \n", script_);
		return MESIBO_RESULT_FAIL;
	}	
	int rv = ExecuteScript(source, global);
	if(MESIBO_RESULT_FAIL == rv){
		MesiboJsDebug::ErrorLog("Error executing script %s\n", script_);
		return MESIBO_RESULT_FAIL;
	}



	return MESIBO_RESULT_OK;

}

/**
Local<Function> GetGlobalMesiboListener(Isolate* isolate, Local<Context>& context, const char* listener_name){

	EscapableHandleScope handle_scope(isolate);
	Local<Function> js_mesibo_listener = GetGlobalMesiboListener(context, listener_name);

	return handle_scope.Escape(js_mesibo_listener);
}
**/
mesibo_int_t MesiboJsProcessor::ExecuteJsFunctionObj(Local<Context>& context,
		Local<Function>& js_fun, int argc, Local<Value> argv[]){

	v8::Isolate::Scope isolateScope(GetIsolate());

	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(GetIsolate());

	//Enter the context
	Context::Scope context_scope(context);

	v8::Local<v8::Value> js_result ;

	v8::TryCatch try_catch(GetIsolate());
	if(!js_fun->Call(context, context->Global(), argc, argv).ToLocal(&js_result)){
		MesiboJsDebug::ReportException(GetIsolate(), &try_catch);
		String::Utf8Value fun_name(GetIsolate(), js_fun->GetName());
		return MESIBO_RESULT_FAIL;
	}

	if(!js_result->IsInt32()) //Either MESIBO_RESULT_OK or MESIBO_RESULT_FAIL 
		return MESIBO_RESULT_FAIL;

	return MesiboJsUtil::JtoC_Int(GetIsolate(), context, js_result);
}
// Reads a file into a v8 string.
MaybeLocal<String> MesiboJsProcessor::ReadFile(Isolate* isolate, const string& name) {
	FILE* file = fopen(name.c_str(), "rb");
	if (file == NULL) return MaybeLocal<String>(); //Error reading file. Return MESIBO_RESULT_FAIL	

	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	rewind(file);

	std::unique_ptr<char> chars(new char[size + 1]);
	chars.get()[size] = '\0';
	for (size_t i = 0; i < size;) {
		i += fread(&chars.get()[i], 1, size - i, file);
		if (ferror(file)) {
			fclose(file);
			return MaybeLocal<String>();
		}
	}
	fclose(file);
	MaybeLocal<String> result = String::NewFromUtf8(
			isolate, chars.get(), NewStringType::kNormal, static_cast<int>(size));
	return result;
}


/**
 * Notify JS Callback function Mesibo_onMessage
 **/
mesibo_int_t MesiboJsProcessor::OnMessage(mesibo_message_params_t* p, const char* message,
		mesibo_uint_t len){

	v8::Locker locker(GetIsolate());
	v8::Isolate::Scope isolateScope(GetIsolate());

	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(GetIsolate());

	//To each it's own context
	v8::Local<v8::Context> context = GetContext(); 
	Context::Scope context_scope(context);

	const int argc = 1;
	v8::Handle<v8::Value> args_bundle[argc];

	v8::Local<v8::Object> message_instance = MesiboJsMessage::CtoJ_MessageParams(GetIsolate(), context, p); 
	
	Local<Function> js_fun = Local<Function>::New(GetIsolate(), js_mesibo_on_message); 
	//Local<Function> js_fun = GetGlobalFunction(context, JS_MESIBO_LISTENER_ON_MESSAGE);

	if(js_fun->IsNullOrUndefined()){
		MesiboJsDebug::ErrorLog("Invalid Function Constructor %s\n", JS_MESIBO_LISTENER_ON_MESSAGE);
		return MESIBO_RESULT_FAIL;
	}

	mesibo_int_t t5 = mesibo_util_usec();
	args_bundle[0] = Local<Value>::Cast(message_instance);
	mesibo_int_t rv = ExecuteJsFunctionObj(context, js_fun, argc, args_bundle);
	mesibo_int_t t6 = mesibo_util_usec();

	mesibo_log(mod_, 0 , "\n ExecuteJsFunction " JS_MESIBO_LISTENER_ON_MESSAGE" %u usec\n\n",
			(uint32_t)(t6-t5));

	return rv;;

}

/**
 * Notify JS Callback function Mesibo_onMessageStatus
 **/
mesibo_int_t MesiboJsProcessor::OnMessageStatus(mesibo_message_params_t* p, mesibo_uint_t status){
	MesiboJsDebug::Log("OnMessageStatus called\n");	

	v8::Locker locker(GetIsolate());

	v8::Isolate::Scope isolateScope(GetIsolate());

	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(GetIsolate());

	//To each it's own context
	v8::Local<v8::Context> context = GetContext(); 

	Context::Scope context_scope(context);

	const int argc = 2;
	v8::Handle<v8::Value> args_bundle[argc];
	v8::Local<v8::Object> params = MesiboJsMessage::CtoJ_MessageParams(GetIsolate(), context, p); // Don't pass context

	args_bundle[0] = params; 
	args_bundle[1] = v8::Integer::NewFromUnsigned(GetIsolate(), status); //use utils to check for errors

	Local<Function> js_fun = GetGlobalFunction(context, JS_MESIBO_LISTENER_ON_MESSAGE_STATUS);
	if(js_fun->IsNullOrUndefined()){
		MesiboJsDebug::ErrorLog("Invalid Function Constructor %s\n", JS_MESIBO_LISTENER_ON_MESSAGE_STATUS);
		return MESIBO_RESULT_FAIL;
	}

	mesibo_int_t t5 = mesibo_util_usec();
	mesibo_int_t rv = ExecuteJsFunctionObj(context, js_fun, argc, args_bundle);
	mesibo_int_t t6 = mesibo_util_usec();

	mesibo_log(mod_, 0 , "\n ExecuteJsFunction " JS_MESIBO_LISTENER_ON_MESSAGE_STATUS" %u usec\n\n",
			(uint32_t)(t6-t5));

	//Return whatever JS Callback is returning, But that would mean blocking until return value
	return rv;;

}

//Pending: OnLogin
