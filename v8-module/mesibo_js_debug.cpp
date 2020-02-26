#include "mesibo_js_processor.h"
#include <stdarg.h>

#define MODULE_LOG_LEVEL_0VERRIDE 0

int MesiboJsDebug::Log(const char* format, ...) {
	va_list args;
	va_start(args, format);
	return vprintf(format , args);
}

int MesiboJsDebug::ErrorLog(const char* format, ...) {
	va_list args;
	va_start(args, format);
	return vprintf(format, args);
	//return mesibo_log(mod_, MODULE_LOG_LEVEL_0VERRIDE, format, args);
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
	return *value ? *value : "<string conversion failed>";
}

void MesiboJsDebug::ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
	v8::HandleScope handle_scope(isolate);
	v8::String::Utf8Value exception(isolate, try_catch->Exception());
	const char* exception_string = ToCString(exception);
	v8::Local<v8::Message> message = try_catch->Message();
	if (message.IsEmpty()) {
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		ErrorLog("%s\n", exception_string);
	} else {
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(isolate,
				message->GetScriptOrigin().ResourceName());
		v8::Local<v8::Context> context(isolate->GetCurrentContext());
		const char* filename_string = ToCString(filename);

		int linenum = message->GetLineNumber(context).FromJust();
		ErrorLog("%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		v8::String::Utf8Value sourceline(
				isolate, message->GetSourceLine(context).ToLocalChecked());
		const char* sourceline_string = ToCString(sourceline);
		ErrorLog("%s\n", sourceline_string);
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn(context).FromJust();
		for (int i = 0; i < start; i++) {
			ErrorLog(" ");
		}
		int end = message->GetEndColumn(context).FromJust();
		for (int i = start; i < end; i++) {
			ErrorLog("^");
		}
		ErrorLog("\n");
		v8::Local<v8::Value> stack_trace_string;
		if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
				stack_trace_string->IsString() &&
				v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
			v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
			const char* stack_trace_string = ToCString(stack_trace);
			ErrorLog("%s\n", stack_trace_string);
		}
	}
}

Local<Value> MesiboJsDebug::ReportExceptionInCallable(const v8::FunctionCallbackInfo<v8::Value>& args, 
		const char* message){

	EscapableHandleScope handle_scope(args.GetIsolate());

	//Get function data for exception reporting	
	Local<Function> func = Local<Function>::Cast(args.Data());
	if(func.IsEmpty())
		return Local<Value>::Cast(v8::Null(args.GetIsolate())); 

	char* func_name = MesiboJsUtil::JtoC_String(args.GetIsolate(), args.GetIsolate()->GetCurrentContext(), 
			func->GetName());	

	if(!func_name) 
		return Local<Value>::Cast(v8::Null(args.GetIsolate())); 

	char* exception_string;
	if(-1 == asprintf(&exception_string, "Error in Function %s: %s\n", func_name, message))
		return Local<Value>::Cast(v8::Null(args.GetIsolate())); 

	Local<Value> rt =  args.GetIsolate()->ThrowException(
			v8::String::NewFromUtf8(args.GetIsolate(), exception_string, 
				v8::NewStringType::kNormal).ToLocalChecked());
	free(func_name);
	free(exception_string);

	return handle_scope.Escape(rt); 
}


void MesiboJsDebug::LogCallback(const v8::FunctionCallbackInfo<v8::Value>& args) {

	if (args.Length() < 1) {
		ReportExceptionInCallable(args, "Syntax Error: Bad Parameters");	
		return;
	}

	bool first = true;
	Log("Logged: ");
	for (int i = 0; i < args.Length(); i++) {
		v8::HandleScope handle_scope(args.GetIsolate());
		if (first) {
			first = false;
		} else {
			Log(" ");
		}
		v8::String::Utf8Value str(args.GetIsolate(), args[i]);
		const char* cstr = ToCString(str);
		Log("%s", cstr);
	}
	Log("\n");
	//fflush(stdout);

}

