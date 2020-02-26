#pragma once

#include "module.h"
#include <string.h>
#include <iostream>
#include <v8.h>
#include <include/libplatform/libplatform.h>

using std::pair;
using std::string;
using std::cout;
using std::endl;

using v8::Context;
using v8::EscapableHandleScope;
using v8::External;
using v8::Function;
using v8::FunctionTemplate;
using v8::Global;
using v8::HandleScope;
using v8::Isolate;
using v8::Local;
using v8::MaybeLocal;
using v8::Name;
using v8::NamedPropertyHandlerConfiguration;
using v8::NewStringType;
using v8::Object;
using v8::ObjectTemplate;
using v8::PropertyCallbackInfo;
using v8::Script;
using v8::String;
using v8::TryCatch;
using v8::Value;
using v8::Persistent;

/* Global Mesibo Object */
#define JS_MESIBO_GLOBAL_OBJECT 		"mesibo"
#define JS_MESIBO_CLASS_MESSAGE 		"Message"
#define JS_MESIBO_CLASS_HTTP 			"Http"
#define JS_MESIBO_CLASS_SOCKET 			"Socket"

/* Global Listener functions */
#define JS_MESIBO_LISTENER_ON_MESSAGE 		"onmessage"
#define JS_MESIBO_LISTENER_ON_MESSAGE_STATUS 	"onmessagestatus"
#define JS_MESIBO_LISTENER_ON_LOGIN 		"onlogin"

/* Global Mesibo Utils */
#define JS_MESIBO_LOG 				"log" 
#define JS_MESIBO_RESULT_FAIL 			"RESULT_FAIL"
#define JS_MESIBO_RESULT_OK 			"RESULT_OK"



typedef struct v8_config_s v8_config_t;
typedef struct http_context_s http_context_t;
typedef struct socket_context_s socket_context_t;

class MesiboJsProcessor {
	public:
		MesiboJsProcessor(mesibo_module_t* mod, const char* script, int log_level)
			:mod_(mod), script_(script), log_(log_level), lastchanged_(0) {
			}
		virtual int Initialize();
		virtual ~MesiboJsProcessor() { };

		mesibo_int_t ExecuteJsFunctionObj(Local<Context>& context,
				Local<Function>& js_func, int argc, Local<Value> argv[]);
		
		void SetCallables(Local<ObjectTemplate> & global);
		Local<Function> GetGlobalFunction(const Local<Context>& context, const char* func_name);	
		
		//Core Message Module Callbacks
		mesibo_int_t OnMessage(mesibo_message_params_t* p, const char* message,
				mesibo_uint_t len);
		mesibo_int_t OnMessageStatus(mesibo_message_params_t* p, mesibo_uint_t status);
		
		void SetIsolate(Isolate* isolate){ isolate_ = isolate;}	
		Isolate* GetIsolate() { return isolate_; }
		Local<Context> GetContext(); //Return allocated context initialized to base context_
		
		//Scripting Module Configuration
		mesibo_module_t* mod_;
		const char* script_;	
		mesibo_int_t log_;
		int lastchanged_;
	
		int ExecuteScript(Local<String> script, Local<ObjectTemplate> global);
		MaybeLocal<String> ReadFile(Isolate* isolate, const string& name);
		
		Isolate* isolate_;
		Global<Context> context_; //Load base context from here 
		
		//Global Mesibo Listeners
		Global<Function> js_mesibo_on_message; 
		Global<Function> js_mesibo_on_message_status; 
		Global<Function> js_mesibo_on_login;

		static Global<ObjectTemplate> message_template_;
		static Global<ObjectTemplate>  http_template_;
		static Global<ObjectTemplate> socket_template_;
};

class MesiboJsDebug {
	public:
		static void LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args); 
		//Debug/Log
		static int Log(const char* format, ...);

		//Exception Handling
		static int ErrorLog(const char* format, ...);
		static void ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch);
		static Local<Value> ReportExceptionInCallable(
				const v8::FunctionCallbackInfo<v8::Value>& args,
				const char* message);
};

class MesiboJsMessage {
	public:	
		//Create a Message Template. Called once in global space
		static Local<v8::ObjectTemplate> MakeMessageTemplate(Isolate* isolate, mesibo_module_t* mod);

		//Create a Message class object in Javascript
		static void MessageClassCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

		//Send a message from Javscript
		static void MessageCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

		static Local<Object> CtoJ_MessageParams(Isolate* isolate, Local<Context>& context , 
				mesibo_message_params_t* p);
		static mesibo_message_params_t* JtoC_MessageParams(Isolate* isolate, 
				const Local<Context>& context, const Local<Object>& params);
};

class MesiboJsHttp {
	public:
		static Local<v8::ObjectTemplate> MakeHttpTemplate(Isolate* isolate, mesibo_module_t* mod);

		//Create an Http class object in Javascript
		static void HttpClassCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

		//Send an HTTP request in Javascript
		static void HttpCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

		static mesibo_http_t* JtoC_HttpOptions(Isolate* isolate, 
				const Local<Context>& context, const Local<Object>& options);
		static http_context_t* JtoC_HttpCallbackContext(Isolate* isolate, 
				Local<Context>& context, Local<Object>& js_obj,
				Local<Function>& js_cb, Local<Value>& js_cbdata, 
				mesibo_module_t* mod);	
};

class MesiboJsSocket {
	public:
		static Local<v8::ObjectTemplate> MakeSocketTemplate(Isolate* isolate, mesibo_module_t* mod);

		//Create a Socket class in Javascript
		static void SocketClassCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

		//Connect to a Socket in Javascript
		static void SocketConnectCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void SocketWriteCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void SocketCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& args);

		static mesibo_socket_t* JtoC_SocketParams(Isolate* isolate, 
				const Local<Context>& context, 
				const Local<Object>& socket_params, socket_context_t* sc);		
		static Local<Value> JtoJ_SocketCallback(Isolate* isolate, 
				const Local<Context>& context, 
				const Local<Object>& socket_params, socket_context_t* sc);
};


//Utility methods for wrapping C++ objects as JavaScript objects,
// and going back again.
class MesiboJsUtil {
	public:
		static int CtoJ_Uint(Isolate* isolate, Local<Context>& context, Local<Object> &js_params, 
				const char* key, mesibo_uint_t value);
		static int CtoJ_String(Isolate* isolate, Local<Context>& context, Local<Object> &js_params, 
				const char* key, const char* value, mesibo_int_t len = -1);

		static mesibo_int_t JtoC_Int(Isolate* isolate,
				const Local<Context>& context, const Local<Value>& integer); 
		static mesibo_uint_t JtoC_Uint(Isolate* isolate,
				const Local<Context>& context, const Local<Value>& integer); 
		static char* JtoC_String(Isolate* isolate, 
				const Local<Context>& context, const Local<Value>& byte_string);
		static char* JtoC_ObjectToString(Isolate* isolate, const Local<Context>& context,
				const Local<Value>& json_object);

		static Local<Value> GetParamValue(Isolate* isolate, 
				const Local<Context>& context, const char* param_name,
				const Local<Object>& params);

		static mesibo_int_t JtoC_ParamInt(Isolate* isolate, 
				const Local<Context>& context, const char* param_name, 
				const Local<Object>& params);
		static mesibo_uint_t JtoC_ParamUint(Isolate* isolate, 
				const Local<Context>& context, const char* param_name, 
				const Local<Object>& params);
		static char* JtoC_ParamString(Isolate* isolate, 
				const Local<Context>& context, const char* param_name,
				const Local<Object>& params);
		static Local<Function> JtoJ_ParamFunction(Isolate* isolate,
				const Local<Context>& context, const char* func_name,
				const Local<Object>& params); 

};
/**
 * Sample V8 Module Configuration
 * Refer sample.conf
 */
struct v8_config_s{
	const char* script;
	int log; //log level
	long last_changed; // Time Stamp
	MesiboJsProcessor* ctx; // v8 context
};


struct socket_context_s{
	//Pure JS Obj, Callback data
	Persistent<Value> js_cbdata;

	//Pure JS Callback function references
	Persistent<Function> js_onconnect;
	//JSValue js_onwrite;
	Persistent<Function> js_ondata;
	//JSValue js_onclose;

	mesibo_module_t* mod;
};

#define HTTP_BUFFER_LEN (64 * 1024)
#define HTTP_RESPONSE_TYPE_LEN (1024)
typedef struct http_context_s {
	mesibo_int_t status;
	char response_type[HTTP_RESPONSE_TYPE_LEN];
	char buffer[HTTP_BUFFER_LEN];
	int datalen;
	MesiboJsProcessor* ctx_;
	mesibo_module_t* mod;

	Isolate* isolate;
	Local<Context> context;

	Persistent<Object> http_obj; //Js HTTP Object
	Persistent<Function> http_cb; //Js HTTP Callback function
	Persistent<Value> http_cbdata; //Js HTTP Callback Data 
}http_context_t;
