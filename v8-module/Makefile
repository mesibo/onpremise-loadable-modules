MODULE=v8
V8_PATH=./v8-lib
V8_LIBPATH=$(V8_PATH)/obj
EXTRA_CCFLAGS= -I$(V8_PATH) -I$(V8_PATH)/include -Iinclude 
EXTRA_LDFLAGS=-Wl,-whole-archive -lv8_monolith -L$(V8_LIBPATH) -Wl,--no-whole-archive -lpthread
-include ../make.inc/make.inc

