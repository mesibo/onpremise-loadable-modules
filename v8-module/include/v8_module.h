#pragma once

//v8_module.h
#include "mesibo_js_processor.h"

MesiboJsProcessor* mesibo_v8_init(mesibo_module_t* mod, v8_config_t* vc);
v8_config_t* get_config_v8(mesibo_module_t* mod);

//Message Callbacks
mesibo_int_t v8_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, char *message,
                mesibo_uint_t len);
mesibo_int_t v8_on_message_status(mesibo_module_t *mod, mesibo_message_params_t *p);

//HTTP Callbcaks
mesibo_int_t js_mesibo_http_ondata(void *cbdata, mesibo_int_t state,
                mesibo_int_t progress, const char *buffer, mesibo_int_t size);
mesibo_int_t js_mesibo_http_onstatus(void *cbdata, mesibo_int_t status, const char *response_type);
void js_mesibo_http_onclose(void *cbdata,  mesibo_int_t result);

//Socket Callbacks
mesibo_int_t js_mesibo_socket_ondata(void *cbdata, const char *data, mesibo_int_t len);
void js_mesibo_socket_onconnect(void *cbdata, mesibo_int_t asock, mesibo_int_t fd);
void js_mesibo_socket_onwrite(void *cbdata);
void js_mesibo_socket_onclose(void *cbdata, mesibo_int_t type);

