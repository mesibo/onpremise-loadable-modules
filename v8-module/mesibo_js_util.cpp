#include "mesibo_js_processor.h"

mesibo_uint_t MesiboJsUtil::JtoC_Uint(Isolate* isolate, const Local<Context>& context, 
		const Local<Value>& integer){ 

	uint32_t value;
	if(!integer->IsUint32()){
		MesiboJsDebug::ErrorLog("Invalid Integer(unsigned 32-bit)\n");	
		return -1;
	}

	value = integer->Uint32Value(context).ToChecked();

	return (mesibo_uint_t)value;

}

mesibo_int_t MesiboJsUtil::JtoC_Int(Isolate* isolate, const Local<Context>& context, 
		const Local<Value>& integer){ 

	int64_t value;
	if(!integer->IsInt32()){
		MesiboJsDebug::ErrorLog("Invalid Integer(32-bit)\n");	
		return -1;
	}

	value = integer->IntegerValue(context).ToChecked(); 

	return (mesibo_int_t)value;
}

char* MesiboJsUtil::JtoC_String(Isolate* isolate, const Local<Context>& context, 
		const Local<Value>& string){

	if(string->IsNullOrUndefined()){
		return NULL;
	}

	String::Utf8Value value(isolate, string);
	return strdup(*value);
}

char* MesiboJsUtil::JtoC_ObjectToString(Isolate* isolate, const Local<Context>& context,
		const Local<Value>& json_object){

	Local<String>json_string = v8::JSON::Stringify(context, json_object).ToLocalChecked();

	if(json_string->IsNullOrUndefined())		
		return NULL;

	return JtoC_String(isolate, context, json_string);
}

Local<Value> MesiboJsUtil::GetParamValue(Isolate* isolate, const Local<Context>& context, 
		const char* param_name, const Local<Object>& params){

	// Create a handle scope to keep the temporary object references.
	EscapableHandleScope handle_scope(isolate);

	Local<Value> p_key;
	Local<Value> p_value;

	p_key = String::NewFromUtf8(isolate, param_name).ToLocalChecked();
	p_value = params->Get(context, p_key).ToLocalChecked();

	return  handle_scope.Escape(p_value);	
}

Local<Function> MesiboJsUtil::JtoJ_ParamFunction(Isolate* isolate, const Local<Context>& context,
		const char* func_name, const Local<Object>& params){
	// Create a handle scope to keep the temporary object references.
	EscapableHandleScope handle_scope(isolate);

	Local<Value> value = GetParamValue(isolate, context, func_name, params);

	//Absolutely sure it's a function
	Local<Function> func = value->IsFunction()?Local<Function>::Cast(value)
		: Local<Function>::Cast(v8::Null(isolate));

	return handle_scope.Escape(func);
}

mesibo_int_t MesiboJsUtil::JtoC_ParamInt(Isolate* isolate, const Local<Context>& context,     
		const char* param_name, const Local<Object>& params){

	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(isolate);

	Local<Value> value = GetParamValue(isolate, context, param_name, params);
	mesibo_int_t int_val;
	int_val =  value->IsNullOrUndefined()? 0 : JtoC_Int(isolate, context, value);		

	return int_val;
}
mesibo_uint_t MesiboJsUtil::JtoC_ParamUint(Isolate* isolate, const Local<Context>& context,     
		const char* param_name, const Local<Object>& params){

	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(isolate);

	Local<Value> value = GetParamValue(isolate, context, param_name, params);
	mesibo_uint_t uint_val;
	uint_val =  value->IsNullOrUndefined()? 0 : JtoC_Uint(isolate, context, value);		

	return uint_val;
}

char* MesiboJsUtil::JtoC_ParamString(Isolate* isolate, const Local<Context>& context,     
		const char* param_name, const Local<Object>& params){

	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(isolate);

	Local<Value> value = GetParamValue(isolate, context, param_name, params);
	if(value->IsNullOrUndefined()){
		return NULL; //Parameter Not defined
	}
	String::Utf8Value str_value(isolate, value);
	if(! (*str_value)){
		MesiboJsDebug::ErrorLog("String Conversion Failed");	
		return NULL;
	}
	return strdup(*str_value); 
}

int MesiboJsUtil::CtoJ_Uint(Isolate* isolate, Local<Context>& context, Local<Object> &js_params, 
		const char* key, mesibo_uint_t value){

	if(NULL == key) return MESIBO_RESULT_FAIL;
	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(isolate);

	Local<Value> p_key;
	Local<Value> p_value;

	p_key = String::NewFromUtf8(isolate, key).ToLocalChecked();
	p_value = v8::Integer::NewFromUnsigned(isolate, (uint32_t)value); //Casting from uint32

	if(js_params->Set(context, p_key, p_value).IsNothing()) //Unlikely exception
		return MESIBO_RESULT_FAIL;


	return MESIBO_RESULT_OK;
}


int MesiboJsUtil::CtoJ_String(Isolate* isolate, Local<Context>& context, Local<Object> &js_params, 
		const char* key, const char* value, mesibo_int_t len){

	if(NULL == key ) return MESIBO_RESULT_FAIL;

	// Create a handle scope to keep the temporary object references.
	HandleScope handle_scope(isolate);

	Local<Value> p_key;
	Local<Value> p_value;

	p_key = String::NewFromUtf8(isolate, key).ToLocalChecked();

	if(NULL == value)
		p_value =  Local<Value>::Cast(v8::Null(isolate));	
	else
		//p_value = String::NewFromUtf8(isolate, value, NewStringType::kNormal, len).ToLocalChecked();
		p_value = String::NewFromUtf8(isolate, value).ToLocalChecked();


	if(js_params->Set(context, p_key, p_value).IsNothing()) //Unlikely exception
		return MESIBO_RESULT_FAIL;

	return MESIBO_RESULT_OK;
}
