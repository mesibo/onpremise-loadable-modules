# FCGI Module

This repository contains the source code for the Fast-CGI Module.

You can use the FCGI Module to act as an interface for your Web server. It can handle API requests by connecting to your backend serices and database.

Refer to the [Skeleton Module](https://github.com/Nagendra1997/mesibo-documentation/blob/master/skeleton.md) for a basic understanding of how Mesibo Modules work. The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/on-premise/loadable-modules/)

## Building the FCGI Module

### 1. The C/C++ Source file
The module name is `fcgi`. The C/C++ Source file is `fcgi.cpp`. The header file `module.h` containing the definitions of all module related components are included in the C/C++ source as follows:
```cpp
#include "module.h"
```
### 2. Configuring the FCGI module
The Fast-CGI interface provide many configuration options when sending an [FCGI request](). However, in this example module we describe( refer to sample.comf) only a small set of essential options which are as follows. You can add more options as per your needs. 

- host The host address
- port Port number
- keealive Persistent Connection
- root The path to the script and assets
- log Log level

```
module fcgi{
	host = 192.168.0.101
	port = 9000
	keepalive = 0
	root = /usr/share/nginx/html/
	script = test.php
	log = 0 
}

```
### 3. Initializing the FCGI module
The FCGI module is initialized with the Mesibo Module Configuration details - module version, the name of the module and references to the module callback functions.
```cpp
int mesibo_module_fcgi_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
	MESIBO_MODULE_SANITY_CHECK(m, version, len);

	if(m->config) {
		fcgi_config_t* fc = get_config_fcgi(m);
		if(NULL == fc){
			mesibo_log(m, MODULE_LOG_LEVEL_OVERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		m->ctx = (void* )fc;
	}

	m->flags = 0;
	m->description = strdup("Sample FCGI Module");
	
	//Initialize Callback functions
	m->on_cleanup = fcgi_on_cleanup;
	m->on_message = fcgi_on_message;
	m->on_message_status = fcgi_on_message_status;
	m->on_login = fcgi_on_login;
	mesibo_log(m, 0, "================> %s init called\n", m->name);

	return MESIBO_RESULT_OK;

}

```
### Storing the configuration in the module context
The Fast CGI configuartion is extracted from the module configuration list and stored in the configuration context structure `fcgi_config_t` which is defined as follows:

```cpp
typedef struct fcgi_config_s{
	const char* host; 
        mesibo_int_t port;
        mesibo_int_t keepalive; 
        const char* root;
        const char* script;
	mesibo_uint_t log;
} fcgi_config_t;
```

To get the configuration details the config helper function `get_config_fcgi` is called.

```cpp
fcgi_config_t* get_config_fcgi(mesibo_module_t* mod){

        fcgi_config_t* fc = (fcgi_config_t*)calloc(1, sizeof(fcgi_config_t));
        fc->host = mesibo_util_getconfig(mod, "host");
        fc->port = atoi(mesibo_util_getconfig(mod, "port"));
        fc->keepalive = atoi(mesibo_util_getconfig(mod, "keepalive"));
        fc->root = mesibo_util_getconfig(mod, "root");
	fc->script = mesibo_util_getconfig(mod, "script");
        fc->log = atoi(mesibo_util_getconfig(mod, "log"));
        
	mesibo_log(mod, fc->log, "fcgi Module Configured :host %s port %u keepalive %d"
			" root %s script %s log %d\n",
		       	fc->host, (uint16_t)fc->port, fc->keepalive, 
			fc->root, fc->script, fc->log);

        return fc;
}
```    

A pointer to the fcgi configuration is stored in `m->ctx`

### 4. fcgi_on_message
When a message is recevived, you need to process it by the executing the required script logic. You can achieve this by sending an FCGI request your server. 

```cpp
static mesibo_int_t fcgi_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, const char *message, mesibo_uint_t len) {
	mesibo_log(mod, 0, "================> %s on_message called\n", mod->name);
	mesibo_log(mod, 0, " from %s to %s id %u message %s\n", p->from, p->to, (uint32_t) p->id, message);
	
	mesibo_message_params_t np;
	memset(&np, 0 ,sizeof(mesibo_message_params_t));
	memcpy(&np, p, sizeof(mesibo_message_params_t));
	fcgi_request(mod, np, strdup(message), len);	
	return MESIBO_RESULT_PASS; 
}

```
### Sending an FCGI Request
As per the provided configuration, you need to construct the FCGI request structure `mesibo_fcgi_t` which is defined as follows:
```cpp
typedef struct mesibo_fcgi_s {
	char *USER;
	char *SCRIPT_FILENAME;
	char *QUERY_STRING;
	char *CONTENT_TYPE;
	char *SCRIPT_NAME;
	char *REQUEST_URI;
	char *DOCUMENT_URI;
	char *DOCUMENT_ROOT;
	char *SERVER_PROTOCOL;
	char *REQUEST_SCHEME;
	char *REMOTE_ADDR;
	char *REMOTE_PORT;
	char *SERVER_ADDR;
	char *SERVER_PORT;
	char *SERVER_NAME;
	char *HTTP_HOST;
	char *HTTP_CONNECTION;
	char *HTTP_CACHE_CONTROL;
	char *HTTP_UPGRADE_INSECURE_REQUESTS;
	char *HTTP_USER_AGENT;
	char *HTTP_ACCEPT;
	char *HTTP_ACCEPT_ENCODING;
	char *HTTP_ACCEPT_LANGUAGE;

	char *body;
	uint64_t bodylen;

} mesibo_fcgi_t;
```
Mesibo provides a FCGI utility named `mesibo_util_fcgi` whose syntax is as follows:
```cpp
mesibo_int_t 	mesibo_util_fcgi(mesibo_fcgi_t *req, const char *host, mesibo_int_t port, mesibo_int_t keepalive, mesibo_fcgi_ondata_t cb, void *cbdata);
```
You can send any required callback data , which will be available in the context of the callback function , a callback function with the signature `mesibo_fcgi_ondata_t` . Through this callback you receive data from your server.

```cpp
static mesibo_int_t fcgi_request(mesibo_module_t *mod, mesibo_message_params_t p, 
		char *message, mesibo_uint_t len) {
	
	fcgi_config_t* fc = (fcgi_config_t*)mod->ctx;
        mesibo_log(mod, fc->log, "fcgi_request called %s %s\n", fc->root, fc->script);
        	
	mesibo_fcgi_t req;
	memset(&req, 0, sizeof(mesibo_fcgi_t));
	req.USER = p.from;
	req.DOCUMENT_ROOT = strdup(fc->root);
	req.SCRIPT_NAME = strdup(fc->script);
	req.body = message;
	req.bodylen = (uint64_t)len;

        mesibo_log(mod, fc->log, "Request parameters %s %s %s %s %u\n", req.USER, req.DOCUMENT_ROOT,
		 req.SCRIPT_NAME, req.body, req.bodylen );
	
	fcgi_context_t* cbdata  = (fcgi_context_t*)calloc(1, sizeof(fcgi_context_t));
	cbdata->mod = mod;
	cbdata->params = p; //Copy message parmeters to fcgi call back context 
	cbdata->params.from = strdup(p.from);
	cbdata->params.to = strdup(p.to);

	mesibo_log(mod, fc->log, "Request parameter object %p\n", &req);
	mesibo_log(mod, fc->log , "host %s, port %lld, keepalive %lld, cb %p, cbdata %p", 
			fc->host, fc->port, fc->keepalive,
			mesibo_fcgi_data_callback, cbdata);
	
	//Host, Port, Keepalive parmeters need to be in configuration
	mesibo_util_fcgi(&req, fc->host, fc->port, fc->keepalive, mesibo_fcgi_data_callback , (void*)cbdata);

	return MESIBO_RESULT_OK;
}
```
### FCGI Response
The callback must be of the signature `mesibo_fcgi_t`

```cpp
typedef mesibo_int_t (*mesibo_fcgi_ondata_t)(void *cbdata, mesibo_int_t result, const char *buffer, mesibo_int_t size);
```

The following is an example, of receiving the response from your server and processing that response by sending it back to the user who made the request. (A response to the query message)

```cpp
int mesibo_fcgi_data_callback(void *cbdata, mesibo_int_t result, const char *buffer, mesibo_int_t size){
	
	fcgi_context_t *b = (fcgi_context_t*)cbdata;
	mesibo_module_t *mod = b->mod;
	fcgi_config_t* fc = (fcgi_config_t*)mod->ctx;
	
	if(MESIBO_RESULT_FAIL == result){
		mesibo_log(mod, MODULE_LOG_LEVEL_OVERRIDE, "Bad response to fcgi request\n");
		return MESIBO_RESULT_FAIL;
	}

	mesibo_log(mod, fc->log, "%.*s\n", size, buffer);

	//Send response to the requester
	mesibo_message_params_t	np;
	memset(&np, 0, sizeof(mesibo_message_params_t));
	
	mesibo_message_params_t request_params = b->params;	
	np.to =  request_params.from; 
	np.from = request_params.to; 
	np.id = rand();

	mesibo_message(mod, &np, buffer, size);
	
	return MESIBO_RESULT_OK;
}
```
### 5. Compiling the FCGI module
To compile the fcgi module, open the sample `MakeFile` provided. Change the `MODULE` to `fcgi`.

For example.

```
MODULE = fcgi
```

Run `make` from your source directory.

```
sudo make
```

It places the result at the `TARGET` location `/usr/lib64/mesibo/mesibo_mod_fcgi.so` which you can verify.

### 6. Loading the filter module 

Mount the directory containing the module while running the mesibo container.
If `mesibo_mod_fcgi.so` is located at `/usr/lib64/mesibo/`
```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
-v /etc/mesibo:/etc/mesibo
-net=host -d  \ 
mesibo/mesibo <app token>
