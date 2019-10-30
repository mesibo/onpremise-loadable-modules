# JS Module

This repository contains the source code for JS Module. JavaScript Module loads and call functions in ECMAScript. In this example, the module uses the embeddable JS engine [duktape](https://duktape.org)

Basically, The Javscript Module provides a bridge between Javascript and Mesibo. It mirrors all the functionality offered in Mesibo Module to be called through the context of Javascript, using callback functions.

You can download the source and compile it to obtain the module- a shared library file. Also, you can load the pre-compiled module which is provided as `mesibo_mod_js.so`

Refer to the [Skeleton Module](https://github.com/Nagendra1997/mesibo-documentation/blob/master/skeleton.md) for a basic understanding of how Mesibo Modules work. The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/loadable-modules/)

## Overview of JS Module
- JS configuration containing the script path is provided in the module configuration (In the file `mesibo.conf`).
- In module initialisation, all the configuration parameters is obtained from the configuration list and stored in a structure object  `js_config_t`.
- Javascript context is initialized from the script and stored in module context
- All the module callable functions are made accessible from the script through the duktape API
- Callback functions are initialized. When module callback functions are called they inturn trigger a callback function defined in the script. For example, when module recieves a message, `js_on_message` triggers `notify_mesibo_on_message`, which then calls `Mesibo_onMessage` defined in the script.


### 1. The C/C++ Source file
The module name is `js`. The C/C++ Source file is `js.c`. The header files `module.h` and `duktape.h` are included in the C/C++ source as follows:
```cpp

#include "module.h"
#include "duktape.h"
  
```
### 2. Configuring the JS Module
The JS module is configured as follows:
```

module = js{
script = /path/to/script
log = 1
}

```
For example,
```

module = js{
script = /etc/mesibo/mytest.js
log = 1
}

```

### 3. Initializing the JS Module

The JS module is initialized with the Mesibo Module Configuration details - module version, the name of the module and  references to the module callback functions.

```
int mesibo_module_js_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {

        MESIBO_MODULE_SANITY_CHECK(m, version, len);

        if(m->config) {
                js_config_t* jsc = get_config_js(m);
                if(jsc == NULL){
                        mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
                        return MESIBO_RESULT_FAIL;
                }
                m->ctx = (void* )jsc;
                jsc->ctx = mesibo_duk_get_context(m);
                if (!jsc->ctx) {
                        mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "Loading context failed for script %s", jsc->script);
                        return MESIBO_RESULT_FAIL;
                }
        }

        else {
                mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
                return MESIBO_RESULT_FAIL;
        }

        m->flags = 0;
        m->name = strdup("Sample JS Module");
        m->on_cleanup = js_cleanup;
        m->on_message = js_on_message;
        m->on_message_status = js_on_message_status;

        return MESIBO_RESULT_OK;
}
	 
 ``` 
### Storing the configuration in module context

The js configuration parameters is extracted from module configuration list and stored in the configuration context structure `js_config_t` which is defined as follows:

```cpp

typedef struct js_config_s{
        char* script;
        int log; //log level
        long last_changed; // Time Stamp
        duk_context* ctx; // JavaScript file context
} js_config_t;

```
The script name provided in the configuration, is used to allocate the JS context (provide by duktape API). If the file is changed the context is reinitialized. The same context is used to call functions defined in JS.

```cpp

js_config_t* get_config_js(mesibo_module_t* mod){

        js_config_t* jsc = (js_config_t*)calloc(1, sizeof(js_config_t));
        jsc->script = mesibo_util_getconfig(mod, "script");
        jsc->log = atoi(mesibo_util_getconfig(mod, "log"));
        jsc->last_changed = 0; //Initialize TS to zero
        jsc->ctx = NULL;
        mesibo_log(mod, jsc->log, "Javascript Module Configured : script %s log %d\n", jsc->script, jsc->log);

        return jsc;
}

```

### 4. JS Callback functions
To call functions in Javascipt from C, this example uses the embeddable JS Engine Duktape. It is a good idea to read the 
[Getting Started with Duktape](https://duktape.org) guide to understand how this is achieved. Here, we will explain how `notify_on_message` calls `Mesibo_onMessage` in JavaScript.

```cpp

static mesibo_int_t  js_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
                const char *message, mesibo_uint_t len) {

        js_config_t* jsc = (js_config_t*)mod->ctx;
        mesibo_log(mod, jsc->log, " %s on_message called\n", mod->name);
        mesibo_log(mod, jsc->log, "aid %u from %s to %s id %u message %.*s\n", p->aid, p->from,
                        p->to, (uint32_t)p->id, len, message);

        int result = notify_mesibo_on_message(mod, p, message, len);
        return result;  // Either MESIBO_RESULT_CONSUMED or MESIBO_RESULT_PASS
}

```
`notify_mesibo_on_message` which will push all the parameters as duktape objects into the duktape call stack and then evaluate the function `Mesibo_onMessage` which is declared in the Javscript file(Refer `mesibo_test.js`)  

```cpp

static mesibo_int_t  notify_mesibo_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
                const char *message, mesibo_uint_t len) {

        duk_context *ctx = mesibo_duk_get_context(mod);
        js_config_t* jsc = (js_config_t*)mod->ctx;
        mesibo_log(mod, jsc->log, "%s called \n", "notify_mesibo_on_message");

        if (!ctx) {
                mesibo_log(mod, jsc->log, "%s : Error: Load context failed", MESIBO_LISTENER_ON_MESSAGE);
                return -1;
        }

        //Get callback function reference from Javscript
        duk_get_global_string(ctx, MESIBO_LISTENER_ON_MESSAGE);

        //Push parmeters
        duk_push_pointer(ctx, (void *)mod);
        duk_idx_t obj_idx = duk_push_object(ctx);

        mesibo_duk_pushparams(mod, ctx, obj_idx, p);
        duk_push_string(ctx, message);
        duk_push_int(ctx, len);
        duk_push_int(ctx, 5);

        mesibo_log(mod, jsc->log, "%s called \n", MESIBO_LISTENER_ON_MESSAGE);
        //Evaluate JS function in the script
        duk_pcall(ctx, 5);

        int ret =
                duk_to_int(ctx, -1);  // Return value can either be
        // MESIBO_RESULT_CONSUMED or MESIBO_RESULT_PASS

        mesibo_log(mod, jsc->log, " ===> %s returned %d \n", MESIBO_LISTENER_ON_MESSAGE, ret);

        duk_pop(ctx);
        return ret;
}

```
Similarly `notify_mesibo_on_message_status`, `notify_mesibo_on_http_response` are defined.

### 5. Calling Mesibo Module helper functions

The duktape interface is used to call C functions defined in the module source file from Javascript.
Function paramters are read from the script context and then converted to appropriate C data types. All callable functions /helper functions must be loaded into context before being called in the script

```cpp

/** Mesibo Module helper functions callable from Javscript**/
static duk_ret_t mesibo_js_http(duk_context *ctx); //mesibo_http
static duk_ret_t mesibo_js_message(duk_context *ctx); //mesibo_message
static duk_ret_t mesibo_js_log(duk_context *ctx); //mesibo_log

```
For example, consider the definition of `mesibo_js_message`
```cpp

static duk_ret_t mesibo_js_message(duk_context *ctx) {

        int n = duk_get_top(ctx); /* #args */
        if (n == 0) {
                /* throw TypeError if no arguments given */
                return DUK_RET_TYPE_ERROR;
        }       

        mesibo_module_t *mod = (mesibo_module_t *)duk_to_pointer(ctx, 0);
        js_config_t* jsc = (js_config_t*)mod->ctx;

        mesibo_message_params_t *p =
                (mesibo_message_params_t *)calloc(1, sizeof(mesibo_message_params_t));
        mesibo_duk_getparams(ctx, 1, p); 

        const char *message = duk_to_string(ctx, 2);
        mesibo_uint_t len = duk_to_uint(ctx, 3); 

        mesibo_log(mod, jsc->log, "\n %s \n", "Sending Message");
        mesibo_log(mod, jsc->log, "aid %u from %s to %s id %u message %.*s\n", p->aid, p->from,
                        p->to, (uint32_t)p->id, len, message);
                        
        mesibo_message(mod, p, message, len);

        free(p);
        return 0;
}

```
In Javascript(Refer `mesibo_test.js`), the corresponding `mesibo_message` function looks like below:
```js

function Mesibo_onMessage(mod, p, message, len, temp) {
        var msg_log = "Recieved Message" + message + +"from " +p.from +"to " +p.to;
        mesibo_log(mod, msg_log, msg_log.length + len);
        var params = Object.assign({},p);
        params.to = p.from;
        params.from = p.to;
        params.expiry = 3600;
        params.id = parseInt(Math.floor(2147483647*Math.random()))

        var test_message = "Hello from Mesibo  Module";

        mesibo_message(mod, params, test_message, test_message.length);

        return MESIBO_RESULT_PASS; //PASS
}


```
### 6. Compiling JS module
To compile js module, open the sample `MakeFile` provided. Change the `MODULE` to `chatbot`.

For example.
```
MODULE = js 
```

Run `make` from your source directory.

```
sudo make
```

On successful build of your module, verify that the target path should contain your shared library `/usr/lib64/mesibo/mesibo_mod_js.so`

### 7. Loading JS module
To load JS  module, specify the module name in mesibo configuration file `/etc/mesibo/mesibo.conf`. Refer `sample.conf`

Mount the directory containing your library which in this case is `/usr/lib64/mesibo/`, while running the mesibo container as follows. You also need to mount the directory containing the mesibo configuration file which in this case is `/etc/mesibo`

```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
         -v /etc/mesibo:/etc/mesibo -net host -d mesibo/mesibo <app token> 
```


