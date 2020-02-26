#include "mesibo_js_processor.h"
#include "v8_module.h"

/**Socket Parameters**/
#define SOCKET_URL 				"url"
#define SOCKET_KEEPALIVE                        "keepalive"
#define SOCKET_VERIFY_HOST                      "verifyHost"

/**Socket Callback functions **/
#define SOCKET_ON_CONNECT                       "onconnect"
#define SOCKET_ON_DATA                          "ondata"

/** Mesibo Socket Utils **/
#define SOCKET_CONNECT 		  		"connect" 
#define SOCKET_WRITE 		  		"write"
#define SOCKET_CLOSE 		  		"close" 

/** Socket Callbacks **/
mesibo_int_t js_mesibo_socket_ondata(void *cbdata, const char *data, mesibo_int_t len){

	socket_context_t *b = (socket_context_t*)cbdata;
	if(!b) return MESIBO_RESULT_FAIL;
	mesibo_module_t* mod = b->mod;
	if(!mod) return MESIBO_RESULT_FAIL;

	MesiboJsProcessor* mp = ((v8_config_t*)mod->ctx)->ctx;
	if(!mp) return MESIBO_RESULT_FAIL;

	Isolate* isolate = mp->GetIsolate();
	v8::Locker locker(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::Local<v8::Context> context = mp->GetContext();

	int argc = 3;
	Local<Value> argv[argc];

	Local<Function> js_cb_ondata = b->js_ondata.Get(isolate);

	if(js_cb_ondata->IsNullOrUndefined() || (len < 0))
		return MESIBO_RESULT_FAIL;


	argv[0] = b->js_cbdata.Get(isolate);//Can be undefined
	argv[1] = v8::String::NewFromUtf8(isolate, data, NewStringType::kNormal, len)
		.ToLocalChecked();
	argv[2] = v8::Integer::NewFromUnsigned(isolate, len);

	mesibo_int_t rv = mp->ExecuteJsFunctionObj(context, js_cb_ondata, argc, argv);

	if(MESIBO_RESULT_FAIL == rv){   //Blocking Call
		MesiboJsDebug::Log("Error calling Socket OnData\n");
		return MESIBO_RESULT_FAIL;
	}

	return rv;

}

void js_mesibo_socket_onconnect(void *cbdata, mesibo_int_t asock, mesibo_int_t fd){

	socket_context_t *b = (socket_context_t*)cbdata;
	if(!b) return ;
	mesibo_module_t* mod = b->mod;
	if(!mod) return;

	MesiboJsProcessor* mp = ((v8_config_t*)mod->ctx)->ctx;
	if(!mp) return ;

	Isolate* isolate = mp->GetIsolate();
	v8::Locker locker(isolate);
	v8::HandleScope handle_scope(isolate);
	v8::Local<v8::Context> context = mp->GetContext();

	int argc = 2;
	Local<Value> argv[argc];

	Local<Function> js_cb_onconnect = b->js_onconnect.Get(isolate);

	if(js_cb_onconnect->IsNullOrUndefined())
		return ;

	argv[0] = b->js_cbdata.Get(isolate);//Can be undefined
	argv[1] = v8::Integer::NewFromUnsigned(isolate, asock);

	mesibo_int_t rv = mp->ExecuteJsFunctionObj(context, js_cb_onconnect, argc, argv);

	if(MESIBO_RESULT_FAIL == rv){   //Blocking Call
		MesiboJsDebug::Log("Error calling Socket OnConnect \n");
	}

	return ;
}

void js_mesibo_socket_onwrite(void *cbdata){
}

void js_mesibo_socket_onclose(void *cbdata, mesibo_int_t type){
}

Local<v8::ObjectTemplate> MesiboJsSocket::MakeSocketTemplate(Isolate* isolate, mesibo_module_t* mod){
	
	EscapableHandleScope handle_scope(isolate);

	v8::Local<v8::ObjectTemplate> socket_obj = v8::ObjectTemplate::New(isolate);
	socket_obj->Set(isolate, SOCKET_URL, v8::Null(isolate));	
	
	socket_obj->Set(isolate, SOCKET_ON_CONNECT, v8::Undefined(isolate));	
	socket_obj->Set(isolate, SOCKET_ON_DATA, v8::Undefined(isolate));	
	
	socket_obj->Set(isolate, SOCKET_CONNECT, FunctionTemplate::New(isolate, 
				SocketConnectCallback, External::New(isolate, (void*)mod)));
	socket_obj->Set(isolate, SOCKET_WRITE, FunctionTemplate::New(isolate, 
				SocketWriteCallback, External::New(isolate, (void*)mod)));
	socket_obj->Set(isolate, SOCKET_CLOSE, FunctionTemplate::New(isolate, 
				SocketCloseCallback, External::New(isolate, (void*)mod)));
	
	return handle_scope.Escape(socket_obj);
}

mesibo_socket_t* MesiboJsSocket::JtoC_SocketParams(Isolate* isolate, const Local<Context>& context,
		const Local<Object>& socket_params, socket_context_t* sc){

	if(!isolate)
		return NULL;

	if(!socket_params->IsNullOrUndefined())
		return NULL;
	if(!sc)
		return NULL;

	HandleScope scope(isolate);
	
	mesibo_socket_t *s = (mesibo_socket_t*)malloc(sizeof(mesibo_socket_t));

	s->keepalive = MesiboJsUtil::JtoC_ParamInt(isolate, context, SOCKET_KEEPALIVE, socket_params);
	s->verify_host = MesiboJsUtil::JtoC_ParamInt(isolate, context, SOCKET_VERIFY_HOST, socket_params);

	Local<Value> ret_socket_cb;
	ret_socket_cb = JtoJ_SocketCallback(isolate, context, socket_params, sc);
	if(ret_socket_cb->IsNullOrUndefined()){
		return NULL;
	}

	s->on_connect = js_mesibo_socket_onconnect;
	s->on_write = js_mesibo_socket_onwrite;
	s->on_data = js_mesibo_socket_ondata;
	s->on_close = js_mesibo_socket_onclose;

	return s;

}
void MesiboJsSocket::SocketClassCallback(const v8::FunctionCallbackInfo<v8::Value>& args){
	if(!args.IsConstructCall()){
		MesiboJsDebug::ErrorLog("Invalid constructor: %s \n", "Socket");
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
	Local<ObjectTemplate> socket_obj 	=
		Local<ObjectTemplate>::New(isolate, MesiboJsProcessor::socket_template_);

	// Return a new http object back to the javascript caller
	v8::Local<v8::Object> socket_instance = socket_obj->NewInstance(context).ToLocalChecked();

	args.GetReturnValue().Set(socket_instance);

}
void MesiboJsSocket::SocketConnectCallback(const v8::FunctionCallbackInfo<v8::Value>& args){

	if (args.IsConstructCall()) return;
	Isolate* isolate = args.GetIsolate();
	if(!isolate) return;
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = Context::New(isolate);
	Context::Scope context_scope(context);

	//If referred inside a class, it  will refer to class instance.
	//Else will refer to global object
	Local<Object> socket_bundle = args.This();

	Local<Value> arg_cbdata = socket_bundle->Get(context,
			String::NewFromUtf8(args.GetIsolate(), "cbdata").ToLocalChecked()).ToLocalChecked();

	Local<External> mod_cb = args.Data().As<External>();
	mesibo_module_t* mod = static_cast<mesibo_module_t*>(mod_cb->Value()); 
	if(!mod) return;

	//Unwrap params	
	socket_context_t* cbdata = (socket_context_t*)calloc(1, sizeof(socket_context_t));	
	cbdata->js_cbdata.Reset(isolate, arg_cbdata);

	mesibo_socket_t* sock = JtoC_SocketParams(isolate, context, socket_bundle, cbdata);
	if(!sock) return;

	mesibo_int_t rv = mesibo_util_socket_connect(sock, cbdata);
	args.GetReturnValue().Set(v8::Integer::New(args.GetIsolate(), (int32_t)rv));
}

void MesiboJsSocket::SocketWriteCallback(const v8::FunctionCallbackInfo<v8::Value>& args){

	if (args.Length() != 3) return;
	Isolate* isolate = args.GetIsolate();
	if(!isolate) return;
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = Context::New(isolate);
	Context::Scope context_scope(context);

	Local<External> mod_cb = args.Data().As<External>();

	mesibo_module_t* mod = static_cast<mesibo_module_t*>(mod_cb->Value()); 

	if(!mod) return;
	//Basic checks
	if(!args[0]->IsNumber())
		return;
	if(!args[2]->IsNumber())
		return;

	//Unwrap params
	mesibo_int_t sock = MesiboJsUtil::JtoC_Int(isolate, context, args[0]);
	mesibo_int_t len = MesiboJsUtil::JtoC_Int(isolate, context, args[2]);	
	char* data = MesiboJsUtil::JtoC_String(isolate, context, args[1]); 
	if(!data)
		return;

	mesibo_int_t rv = mesibo_util_socket_write(sock, data, len);
	if(MESIBO_RESULT_FAIL == rv)
		MesiboJsDebug::ErrorLog("Socket Write Failed \n");

	free(data);
}
void MesiboJsSocket::SocketCloseCallback(const v8::FunctionCallbackInfo<v8::Value>& args){

	if (args.Length() < 1) return;
	Isolate* isolate = args.GetIsolate();
	if(!isolate) return;
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = Context::New(isolate);
	Context::Scope context_scope(context);

	Local<External> mod_cb = args.Data().As<External>();

	mesibo_module_t* mod = static_cast<mesibo_module_t*>(mod_cb->Value()); 
	if(!mod) return;

	//Basic Checks
	if(!args[0]->IsNumber())
		return;

	//Unwrap params	
	mesibo_int_t sock = MesiboJsUtil::JtoC_Int(isolate, context, args[0]);

	mesibo_util_socket_close(sock);
}

Local<Value> MesiboJsSocket::JtoJ_SocketCallback(Isolate* isolate, const Local<Context>& context, 
		const Local<Object>& socket_params, socket_context_t* sc){

	Local<Function> func_obj = MesiboJsUtil::JtoJ_ParamFunction(isolate, context, 
			SOCKET_ON_CONNECT, socket_params); 
	if(func_obj->IsNullOrUndefined())
		return Local<Value>::Cast(v8::Null(isolate)); //xxx: Return Exception 
	sc->js_onconnect.Reset(isolate, func_obj); 

	func_obj = MesiboJsUtil::JtoJ_ParamFunction(isolate, context, SOCKET_ON_DATA, socket_params); 
	if(func_obj->IsNullOrUndefined())
		return Local<Value>::Cast(v8::Null(isolate)); //xxx: Return Exception 
	sc->js_ondata.Reset(isolate, func_obj);

	return v8::Integer::NewFromUnsigned(isolate, MESIBO_RESULT_OK); 
}




