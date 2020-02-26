#include "mesibo_js_processor.h"
#include "v8_module.h"


/** HTTP options **/
//proxy disabled
#define HTTP_CONTENT_TYPE 			"contentType"
#define HTTP_EXTRA_HEADER 			"headers"
#define HTTP_USER_AGENT 			"userAgent"
#define HTTP_REFERRER 				"referrer"
#define HTTP_ORIGIN 				"origin"
#define HTTP_COOKIE 				"cookie"
#define HTTP_ENCODING 				"encoding"
#define HTTP_CACHE_CONTROL 			"cacheControl"
#define HTTP_ACCEPT 				"accept"
#define HTTP_ETAG 				"etag"
#define HTTP_IMS 				"ims"
#define HTTP_CONN_TIMEOUT 			"connTimeout"
#define HTTP_HEADER_TIMEOUT 			"headerTimeout"
#define HTTP_BODY_TIMEOUT 			"bodyTimeout"
#define HTTP_TOTAL_TIMEOUT 			"totalTimeout"

#define HTTP_DATA 				"data"
#define HTTP_STATUS 				"status"
#define HTTP_RESPONSE 				"response"
#define HTTP_RESPONSE_JSON 			"responseJSON"
#define HTTP_RESPONSE_TYPE  			"responseType"
#define HTTP_RESPONSE_LENGTH 			"responseLength"

#define HTTP_ONDATA 				"ondata"
#define HTTP_URL 				"url"
#define HTTP_POST 				"post"
#define HTTP_CBDATA 				"cbdata"
#define HTTP_SEND 				"send"

void mesibo_js_destroy_http_context(http_context_t* mc){
	free(mc);
}


/**
 * C++ HTTP Callback Function which in turn calls the JS callback reference
 */

mesibo_int_t js_mesibo_http_ondata(void *cbdata, mesibo_int_t state, 
		mesibo_int_t progress, const char *buffer, mesibo_int_t size) {
	http_context_t *b = (http_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;

	
	if (progress < 0) {
		mesibo_log(mod, 0, "Error in http callback \n");
		mesibo_js_destroy_http_context(b);
		return MESIBO_RESULT_FAIL;
	}

	if (MODULE_HTTP_STATE_RESPBODY != state) {
		return MESIBO_RESULT_OK;
	}

	if ((MODULE_HTTP_STATE_RESPBODY == state) && buffer!=NULL && size!=0 ) {
		
		//MesiboJsDebug::Log("=====> HTTP Body of len: %u , %.*s\n", (unsigned long)size, size, buffer);
		if(HTTP_BUFFER_LEN < (b->datalen + size )){
			mesibo_log(mod, 0,
					"Error in http callback : Buffer overflow detected \n", mod->name);
			return MESIBO_RESULT_FAIL;
		}
		memcpy(b->buffer + b->datalen, buffer, size);
		b->datalen += size;
	}

	if (100 == progress) {
		//Response complete
		return MESIBO_RESULT_OK;
	}

	return MESIBO_RESULT_OK;
}

mesibo_int_t js_mesibo_http_onstatus(void *cbdata, mesibo_int_t status, const char *response_type){
	
	if(!response_type)
		return MESIBO_RESULT_FAIL;

	http_context_t *b = (http_context_t *)cbdata;
	if(!b) return MESIBO_RESULT_FAIL;	
	mesibo_module_t* mod = b->mod;
	if(!mod) return MESIBO_RESULT_FAIL;

	b->status = status;
	memcpy(b->response_type, response_type, strlen(response_type));
	
	return MESIBO_RESULT_OK;
}

void js_mesibo_http_onclose(void *cbdata,  mesibo_int_t result){
	if(MESIBO_RESULT_FAIL == result){
		MesiboJsDebug::ErrorLog("Intenral Error: HTTP Response failed \n");
		return;
	}
		
	http_context_t *b = (http_context_t *)cbdata;
	if(!b) return;
	
	mesibo_module_t* mod = b->mod;
	if(!mod) return;

	MesiboJsProcessor* mp = ((v8_config_t*)mod->ctx)->ctx;
	if(!mp) return;

	Isolate* isolate = mp->GetIsolate();
	if(!isolate) return;
	
	v8::Locker locker(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::Local<v8::Context> context = mp->GetContext(); 

	int argc = 1;
	Local<Value> argv[argc];

	Local<Object> js_http_obj = b->http_obj.Get(isolate);
	if(js_http_obj->IsNullOrUndefined()){
		mesibo_js_destroy_http_context(b);
		return ;
	}

	Local<Function> js_http_cb = b->http_cb.Get(isolate);
	if(js_http_cb->IsNullOrUndefined()){
		mesibo_js_destroy_http_context(b);
		return ;
	}

	MesiboJsUtil::CtoJ_String(isolate, context, js_http_obj, HTTP_RESPONSE, b->buffer);
	MesiboJsUtil::CtoJ_String(isolate, context, js_http_obj, HTTP_RESPONSE_TYPE, b->response_type);
	MesiboJsUtil::CtoJ_Uint(isolate, context, js_http_obj, HTTP_STATUS, b->status);

	argv[0] = Local<Value>::Cast(js_http_obj);
	mesibo_int_t rv = mp->ExecuteJsFunctionObj(context, js_http_cb, argc, argv);
	//xxx: If callback returns undefined
	//Then, it defaults to MESIBO_RESULT_FAIL

	if(MESIBO_RESULT_FAIL == rv){	
		MesiboJsDebug::ErrorLog("Error calling HTTP callback \n");
	}

	mesibo_js_destroy_http_context(b);

}


// Extracts a C string from a V8 Utf8Value.
const char* JtoC_String(const v8::String::Utf8Value& value) {
	return *value ? *value : NULL; 
}

Local<v8::ObjectTemplate> MesiboJsHttp::MakeHttpTemplate(Isolate* isolate, mesibo_module_t* mod){
	
	EscapableHandleScope handle_scope(isolate);
	v8::Local<v8::ObjectTemplate> http_obj = v8::ObjectTemplate::New(isolate);
	http_obj->Set(isolate, HTTP_CONTENT_TYPE, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_EXTRA_HEADER, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_USER_AGENT, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_REFERRER, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_ORIGIN, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_COOKIE, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_ENCODING, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_CACHE_CONTROL, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_ACCEPT, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_ETAG, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_IMS, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_CONN_TIMEOUT, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_HEADER_TIMEOUT, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_BODY_TIMEOUT, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_BODY_TIMEOUT, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_TOTAL_TIMEOUT, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_DATA, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_STATUS, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_RESPONSE, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_RESPONSE_JSON, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_RESPONSE_TYPE, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_URL, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_POST, v8::Null(isolate));	
	http_obj->Set(isolate, HTTP_SEND, FunctionTemplate::New(isolate, 
				MesiboJsHttp::HttpCallback, External::New(isolate, (void*)mod)));
	
	return handle_scope.Escape(http_obj);
}

mesibo_http_t* MesiboJsHttp::JtoC_HttpOptions(Isolate* isolate, 
		const Local<Context>& context,
		const Local<Object>& options){

	if(!isolate)
		return NULL;
	HandleScope scope(isolate);

	mesibo_http_t* opt = (mesibo_http_t*) calloc(1, sizeof(mesibo_http_t)); 

	//xxx: Collect the given keys. Check if option is present, then unwrap	
	//The string references are allocated by strdup, needs to be freed 
	opt->url = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_URL, options);
	if(!opt->url)
		return NULL;
	opt->post = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_POST, options);
	if(!opt->post)
		return NULL;
	opt->content_type = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_CONTENT_TYPE, options);
	opt->extra_header = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_EXTRA_HEADER, options);
	opt->referrer = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_REFERRER, options);
	opt->origin  = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_ORIGIN, options);
	opt->cookie = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_COOKIE, options);
	opt->encoding = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_ENCODING, options);
	opt->cache_control = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_CACHE_CONTROL, options);
	opt->accept = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_ACCEPT, options);
	opt->etag = MesiboJsUtil::JtoC_ParamString(isolate, context, HTTP_ETAG, options);

	//In case of integer params, All are considered 32-bit unsigned integers	
	opt->ims = MesiboJsUtil::JtoC_ParamUint(isolate, context, HTTP_IMS, options);
	opt->conn_timeout = MesiboJsUtil::JtoC_ParamUint(isolate, context, HTTP_CONN_TIMEOUT, options);
	opt->header_timeout = MesiboJsUtil::JtoC_ParamUint(isolate, context, HTTP_HEADER_TIMEOUT, options);
	opt->body_timeout = MesiboJsUtil::JtoC_ParamUint(isolate, context, HTTP_BODY_TIMEOUT, options);
	opt->total_timeout= MesiboJsUtil::JtoC_ParamUint(isolate, context, HTTP_TOTAL_TIMEOUT, options);
	
	opt->on_data = js_mesibo_http_ondata;
	opt->on_status = js_mesibo_http_onstatus;
	opt->on_close = js_mesibo_http_onclose;

	return opt;
}

http_context_t* MesiboJsHttp::JtoC_HttpCallbackContext(Isolate* isolate, 
		Local<Context>& context, Local<Object>& js_obj, Local<Function>& js_cb, 
		Local<Value>& js_cbdata, mesibo_module_t* mod){

	if(!isolate) return NULL;
	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(isolate);

	if((!js_cb->IsFunction())){ 
		MesiboJsDebug::ErrorLog("Invalid HTTP Callback");
		return NULL;
	}

	http_context_t* hc = (http_context_t*)calloc(1, sizeof(http_context_t));
	if(!hc) return NULL;

	//V8 Context	
	hc->isolate = isolate;
	hc->context = context;	

	//Persistent<Function> http_callback;
	hc->http_obj.Reset(isolate, js_obj);
	hc->http_cb.Reset(isolate, js_cb);
	hc->http_cbdata.Reset(isolate, js_cbdata);

	hc->mod = mod;

	return hc; //The caller is responsible for freeing this pointer
}

void MesiboJsHttp::HttpClassCallback(const v8::FunctionCallbackInfo<v8::Value>& args){
	if(!args.IsConstructCall()){
		MesiboJsDebug::ErrorLog("Invalid constructor: %s \n", "Http");
		return;
	}

	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext(); 
	Context::Scope context_scope(context);

	Local<External> mod_cb = args.Data().As<External>();
	mesibo_module_t* mod = static_cast<mesibo_module_t*>(mod_cb->Value()); 
	if(!mod){
		MesiboJsDebug::ErrorLog("Invalid Object: %s \n", "mod");
		return; //Internal Error
	}
	// Return a new http object back to the javascript caller
	Local<ObjectTemplate> http_obj 	=
		Local<ObjectTemplate>::New(isolate, MesiboJsProcessor::http_template_);

	v8::Local<v8::Object> http_instance = http_obj->NewInstance(context).ToLocalChecked();

	args.GetReturnValue().Set(http_instance);

}

void MesiboJsHttp::HttpCallback(const v8::FunctionCallbackInfo<v8::Value>& args){

	if (args.IsConstructCall()){ 
		MesiboJsDebug::ReportExceptionInCallable(args, "Error: Bad call to class method");
		return;
	}

	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext(); 
	Context::Scope context_scope(context);

	//If referred inside a class, it  will refer to class instance.
	//Else will refer to global object
	Local<Object> http_bundle = args.This(); 


	//JtoJ_ParamFunction	
	Local<Value> arg_on_data = http_bundle->Get(context, 
			String::NewFromUtf8(args.GetIsolate(), HTTP_ONDATA).ToLocalChecked()).ToLocalChecked();	
	if(!arg_on_data->IsFunction()){
		MesiboJsDebug::ReportExceptionInCallable(args, "Bad function");
		return;
	}

	Local<Value> arg_cbdata = http_bundle->Get(context, 
			String::NewFromUtf8(args.GetIsolate(), HTTP_CBDATA).ToLocalChecked()).ToLocalChecked();	

	//Unwrap params
	Local<External> mod_cb = args.Data().As<External>();
	mesibo_module_t* mod = static_cast<mesibo_module_t*>(mod_cb->Value()); 
	if(!mod) return; //Internal Error


	//Casting to JS callback function ref, making sure that it is a function object 
	Local<Function> js_cb = Local<Function>::Cast(arg_on_data);
	if(js_cb.IsEmpty()){
		MesiboJsDebug::ReportExceptionInCallable(args, "Invalid Function: http_cb");
		return;
	}

	Local<Value> js_cbdata = arg_cbdata; //The JS callback data (arbitrary & optional)
	//xxx Passing the entire HTTP bundle , so no need to pass redundant args	
	http_context_t* hc =  JtoC_HttpCallbackContext(isolate, context, http_bundle, js_cb, js_cbdata, mod); 

	if(hc == NULL){	
		MesiboJsDebug::ErrorLog("Invalid HTTP Context\n");
		return;
	}
	
	mesibo_http_t* req = JtoC_HttpOptions(args.GetIsolate(), context, http_bundle);
	
	mesibo_int_t rv = mesibo_util_http(req, (void*)hc);

	args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), (int32_t)rv));
}
