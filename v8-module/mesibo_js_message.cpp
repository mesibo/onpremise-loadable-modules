#include "mesibo_js_processor.h"

/** Message Parmeters **/
#define MESSAGE_AID 				"aid"
#define MESSAGE_ID 				"id"
#define MESSAGE_REFID 				"refid"
#define MESSAGE_GROUPID 			"groupid"
#define MESSAGE_FLAGS 				"flags"
#define MESSAGE_TYPE 				"type"
#define MESSAGE_EXPIRY 				"expiry"
#define MESSAGE_TO_ONLINE 			"toOnline"
#define MESSAGE_TO 				"to"
#define MESSAGE_FROM                   		"from"
#define MESSAGE_ENABLE_READ_RECEIPT 		"enableReadReceipt"
#define MESSAGE_ENABLE_DELIVERY_RECEIPT 	"enableDeliveryReceipt"
#define MESSAGE_ENABLE_PRESENCE 		"enablePresence"
#define MESSAGE_SEND_IF_ONLINE 			"sendIfOnline"
#define MESSAGE_SEND  				"send"

#define MESSAGE_MESSAGE 			"message"
#define MESSAGE_DATA 				"data"

//Internal
#define MESIBO_FLAG_DELIVERYRECEIPT     	0x1
#define MESIBO_FLAG_READRECEIPT         	0x2
#define MESIBO_FLAG_PRESENCE           		0x8

//Types Supported
#define MESIBO_JS_TYPE_STRING     	 0 //Text 
#define MESIBO_JS_TYPE_INTEGER 		 1 //Integer(32-bit)
#define MESIBO_JS_TYPE_ARRAY 		 2 //Byte Array: Uint8Array, Uint16Array,...
#define MESIBO_JS_TYPE_OBJECT 		 3 //JSON Object. Stringify Internally 
#define MESIBO_JS_TYPE_INVALID 	 	-1	

//Internal
mesibo_int_t EnableMessageProperty(const v8::FunctionCallbackInfo<v8::Value>& args, const char* property, mesibo_uint_t value);
void EnableReadReceiptCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void EnableDeliveryReceiptCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void EnablePresenceCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
void SendIfOnlineCallback(const v8::FunctionCallbackInfo<v8::Value>& args);
mesibo_int_t GetJsMessageType(v8::Local<Value>& message); 

mesibo_int_t v8_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, char *message, mesibo_uint_t len) {
	
	
	v8_config_t* vc = (v8_config_t*)mod->ctx;

	MesiboJsProcessor* mesibo_js = vc->ctx;

	mesibo_log(mod, vc->log,  "================> %s on_message called\n", mod->name);
	mesibo_log(mod, vc->log, " from %s to %s id %u message %s\n", 
			p->from, p->to, (uint32_t) p->id, message);
	
	// Clone Parameters
	mesibo_message_params_t mp;
	memset(&mp, 0, sizeof(mesibo_message_params_t));
	if(NULL == p) return MESIBO_RESULT_FAIL;

	memcpy(&mp, p, sizeof(mesibo_message_params_t));
	mp.from = NULL != p->from  ? strdup(p->from): NULL; 
	mp.to = NULL != p->to ? strdup(p->to): NULL; 

	mesibo_int_t rv;
	double et = 0;
	int N=1;
	for(int i=1; i <= N; i++){	
	
		mesibo_int_t t1 = mesibo_util_usec();
		rv = mesibo_js->OnMessage(&mp, message, len);	
		mesibo_int_t t2 = mesibo_util_usec();
		et = et +  uint32_t(t2-t1);	
	
		mesibo_log(mod, 0 ,"\n\n Time taken %d onMessage() %u usec\n\n",i, (uint32_t)(t2-t1)); 
	}

	mesibo_log(mod, 0 ,"\n\n Time taken onMessage avg(%d):  %lf usec\n\n", N, et/N);
       	
	return rv; 
}

mesibo_int_t v8_on_message_status(mesibo_module_t *mod, mesibo_message_params_t *p) {

	v8_config_t* vc = (v8_config_t*)mod->ctx;

	MesiboJsProcessor* mesibo_js = vc->ctx;
       		
	mesibo_log(mod, 0, "================>%s on_message_status called\n", mod->name);
	mesibo_log(mod, 0, "to %s from %s id %u status %d\n", p->to, p->from, (uint32_t)p->id, (int)p->status);

	// Clone Parameters
	mesibo_message_params_t mp;
	memset(&mp, 0, sizeof(mesibo_message_params_t));
	memcpy(&mp, p, sizeof(mesibo_message_params_t));
	mp.to = NULL != p->to ? strdup(p->to): NULL; 
	mp.from = NULL != p->from  ? strdup(p->from): NULL;
	
	mesibo_int_t rv = mesibo_js->OnMessageStatus(&mp, p->status);	

	return rv;
}


Local<v8::ObjectTemplate> MesiboJsMessage::MakeMessageTemplate(Isolate* isolate, mesibo_module_t* mod){

	EscapableHandleScope handle_scope(isolate);
	v8::Local<v8::ObjectTemplate> message_obj = v8::ObjectTemplate::New(isolate);
	message_obj->Set(isolate, MESSAGE_AID, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_ID, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_REFID, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_GROUPID, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_FLAGS, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_TYPE, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_EXPIRY, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_TO_ONLINE, v8::Null(isolate));	
	message_obj->Set(isolate, MESSAGE_SEND, FunctionTemplate::New(isolate, 
				MesiboJsMessage::MessageCallback, External::New(isolate, (void*)mod)));
	//Set priperties Internally
	message_obj->Set(isolate, MESSAGE_ENABLE_READ_RECEIPT, FunctionTemplate::New(isolate, 
				EnableReadReceiptCallback));
	message_obj->Set(isolate, MESSAGE_ENABLE_DELIVERY_RECEIPT, FunctionTemplate::New(isolate, 
				EnableDeliveryReceiptCallback));
	message_obj->Set(isolate, MESSAGE_ENABLE_PRESENCE, FunctionTemplate::New(isolate, 
				EnablePresenceCallback));
	message_obj->Set(isolate, MESSAGE_SEND_IF_ONLINE, FunctionTemplate::New(isolate, 
				SendIfOnlineCallback));

	return handle_scope.Escape(message_obj);

}


void MesiboJsMessage::MessageClassCallback(const v8::FunctionCallbackInfo<v8::Value>& args){

	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext(); 
	Context::Scope context_scope(context);

	Local<External> mod_cb = args.Data().As<External>();
	mesibo_module_t* mod = static_cast<mesibo_module_t*>(mod_cb->Value()); 
	if(!mod)return;

	Local<ObjectTemplate> message_templ =
		Local<ObjectTemplate>::New(isolate, MesiboJsProcessor::message_template_);

	v8::Local<v8::Object> message_instance = message_templ->NewInstance(context).ToLocalChecked();

	args.GetReturnValue().Set(message_instance);

}

mesibo_int_t GetJsMessageType(v8::Local<Value>& message){
	//xxx: Use enum
	if(message->IsInt32())
		return MESIBO_JS_TYPE_INTEGER;	
	if(message->IsString())
		return MESIBO_JS_TYPE_STRING;
       	//ArrayBuffer is a subtype of object. So compare it for specificity earlier. Consider such cases carefully.
	if(message->IsUint8Array())
		return MESIBO_JS_TYPE_ARRAY;
	if(message->IsObject())
		return MESIBO_JS_TYPE_OBJECT;
	
	return MESIBO_JS_TYPE_INVALID;	
}

void MesiboJsMessage::MessageCallback(const v8::FunctionCallbackInfo<v8::Value>& args){

	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Context::Scope context_scope(context);

	Local<Object> message_bundle = args.This();

	const char* message;
	mesibo_int_t len; 
	Local<Value> arg_message = message_bundle->Get(context,
			String::NewFromUtf8(args.GetIsolate(), MESSAGE_MESSAGE).ToLocalChecked()).ToLocalChecked();
	
	//Overloaded types for messaging
	mesibo_int_t message_type = GetJsMessageType(arg_message);
	if(MESIBO_JS_TYPE_INVALID == message_type){
		MesiboJsDebug::ReportExceptionInCallable(args, "Invalid: message");
		return;
	}

	switch(message_type){
		
		//Number and string can be safely parsed as a string
		case MESIBO_JS_TYPE_INTEGER:
		case MESIBO_JS_TYPE_STRING:
			message = MesiboJsUtil::JtoC_String(isolate, context, arg_message);
			if(!message) 
			{
				MesiboJsDebug::ReportExceptionInCallable(args, "Invalid: message");
				return;
			}
			len = strlen(message);
			break;
		
		case MESIBO_JS_TYPE_OBJECT:
			//JSON Stringify Internally
			message = MesiboJsUtil::JtoC_ObjectToString(isolate, context, arg_message);
			if(!message) 
			{
				MesiboJsDebug::ReportExceptionInCallable(args, "Invalid: message");
				return;
			}
			len = strlen(message);
			break;
		
		case MESIBO_JS_TYPE_ARRAY:
			return;
			break;

		default: return;
	}

	//Unwrap Params
	Local<External> mod_cb = args.Data().As<External>();
	mesibo_module_t* mod = static_cast<mesibo_module_t*>(mod_cb->Value()); 
	if(!mod) return; //Internal Error

	mesibo_message_params_t* p = JtoC_MessageParams(isolate, context, message_bundle);
	if(NULL == p){
		MesiboJsDebug::ReportExceptionInCallable(args, "Invalid params");
		return;
	}
	if(!p->aid  || !p->id){
		MesiboJsDebug::ReportExceptionInCallable(args, "Invalid params");
		return;
	}



	MesiboJsDebug::Log("=======> Sending Message %p %p %d %s \n", mod, p , len, message);
	mesibo_int_t rv = mesibo_message(mod, p, message, len); 
	Local<v8::Integer> ret = v8::Integer::New(isolate, rv);

	free(p->to);
	free(p->from);
	free(p);

	args.GetReturnValue().Set(ret);
}

Local<Object> MesiboJsMessage::CtoJ_MessageParams(Isolate* isolate,
		Local<Context>& context, mesibo_message_params_t* p){

	EscapableHandleScope handle_scope(isolate);

	if(!p)        
		return Local<Object>::Cast(v8::Null(isolate));	

	Local<ObjectTemplate> templ =
		Local<ObjectTemplate>::New(isolate, MesiboJsProcessor::message_template_);
	v8::Local<v8::Object> message_obj =
		templ->NewInstance(context).ToLocalChecked();

	MesiboJsUtil::CtoJ_Uint(isolate, context, message_obj, MESSAGE_AID, 	  p->aid);
	MesiboJsUtil::CtoJ_Uint(isolate, context, message_obj, MESSAGE_ID,        p->id);
	MesiboJsUtil::CtoJ_Uint(isolate, context, message_obj, MESSAGE_REFID,     p->refid);
	MesiboJsUtil::CtoJ_Uint(isolate, context, message_obj, MESSAGE_GROUPID,   p->groupid);
	MesiboJsUtil::CtoJ_Uint(isolate, context, message_obj, MESSAGE_FLAGS,     p->flags);
	MesiboJsUtil::CtoJ_Uint(isolate, context, message_obj, MESSAGE_TYPE,      p->type);
	MesiboJsUtil::CtoJ_Uint(isolate, context, message_obj, MESSAGE_EXPIRY,    p->expiry);
	MesiboJsUtil::CtoJ_String(isolate, context, message_obj, MESSAGE_TO,      p->to);
	MesiboJsUtil::CtoJ_String(isolate, context, message_obj, MESSAGE_FROM,    p->from);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Escape(message_obj);
}


mesibo_message_params_t* MesiboJsMessage::JtoC_MessageParams(Isolate* isolate, 
		const Local<Context>& context,
		const Local<Object>& params){

	HandleScope scope(isolate);

	mesibo_message_params_t *mp = (mesibo_message_params_t*)calloc(1, sizeof(mesibo_message_params_t)); 

	//In case of integer params, All are considered 32-bit unsigned integers	
	mp->aid = MesiboJsUtil::JtoC_ParamUint(isolate, context, MESSAGE_AID, params);	
	mp->id = MesiboJsUtil::JtoC_ParamUint(isolate, context, MESSAGE_ID, params);	
	mp->refid = MesiboJsUtil::JtoC_ParamUint(isolate, context, MESSAGE_REFID, params);	
	mp->groupid = MesiboJsUtil::JtoC_ParamUint(isolate, context, MESSAGE_GROUPID, params);	
	mp->flags = MesiboJsUtil::JtoC_ParamUint(isolate, context, MESSAGE_FLAGS, params);	
	mp->type = MesiboJsUtil::JtoC_ParamUint(isolate, context, MESSAGE_TYPE, params);	
	mp->expiry = MesiboJsUtil::JtoC_ParamUint(isolate, context, MESSAGE_EXPIRY, params);	
	mp->to = MesiboJsUtil::JtoC_ParamString(isolate, context, MESSAGE_TO, params);	
	mp->from = MesiboJsUtil::JtoC_ParamString(isolate, context, MESSAGE_FROM, params);	

	return mp;
}

//Usage Note: Property must be of type mesibo_uint_t
mesibo_int_t EnableMessageProperty(const v8::FunctionCallbackInfo<v8::Value>& args, const char* property, mesibo_uint_t value){

	Isolate* isolate = args.GetIsolate();
	HandleScope scope(isolate);

	v8::Local<v8::Context> context = args.GetIsolate()->GetCurrentContext();
	Context::Scope context_scope(context);

	Local<Object> message_bundle = args.This();

	if(!property)
		return MESIBO_RESULT_FAIL;

	if(!args[0]->IsBoolean()){
		MesiboJsDebug::ReportExceptionInCallable(args, "Bad Parameters");
		return MESIBO_RESULT_FAIL;
	}

	mesibo_int_t rv = MESIBO_RESULT_OK; //Do nothing if 'false'
	if(true == args[0]->BooleanValue(isolate)){
		mesibo_uint_t param_value = MesiboJsUtil::JtoC_ParamUint(isolate, context, property, message_bundle);
		rv = MesiboJsUtil::CtoJ_Uint(isolate, context, message_bundle, property, param_value | value);	
	}

	return rv;
}

void EnableReadReceiptCallback(const v8::FunctionCallbackInfo<v8::Value>& args){
	mesibo_int_t rv = EnableMessageProperty(args, MESSAGE_FLAGS, MESIBO_FLAG_READRECEIPT);
	if(MESIBO_RESULT_FAIL == rv)
		return;
	Local<v8::Integer> ret = v8::Integer::New(args.GetIsolate(), rv);
	args.GetReturnValue().Set(ret);
}


void EnableDeliveryReceiptCallback(const v8::FunctionCallbackInfo<v8::Value>& args){
	mesibo_int_t rv = EnableMessageProperty(args, MESSAGE_FLAGS, MESIBO_FLAG_DELIVERYRECEIPT);
	if(MESIBO_RESULT_FAIL == rv)
		return;
	Local<v8::Integer> ret = v8::Integer::New(args.GetIsolate(), rv);
	args.GetReturnValue().Set(ret);
}

void EnablePresenceCallback(const v8::FunctionCallbackInfo<v8::Value>& args){
	mesibo_int_t rv = EnableMessageProperty(args, MESSAGE_FLAGS, MESIBO_FLAG_PRESENCE);
	if(MESIBO_RESULT_FAIL == rv)
		return;
	Local<v8::Integer> ret = v8::Integer::New(args.GetIsolate(), rv);
	args.GetReturnValue().Set(ret);
}

void SendIfOnlineCallback(const v8::FunctionCallbackInfo<v8::Value>& args){
	mesibo_int_t rv = EnableMessageProperty(args, MESSAGE_TO_ONLINE, 1);
	if(MESIBO_RESULT_FAIL == rv)
		return;
	Local<v8::Integer> ret = v8::Integer::New(args.GetIsolate(), rv);
	args.GetReturnValue().Set(ret);
}
