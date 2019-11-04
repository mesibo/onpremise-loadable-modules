# Skeleton Module

This repository contains a bare-bones version of a Mesibo Module that explains the usage of different aspects of the module, various callback functions, callable functions, and utilities. You can download the source code and compile it to obtain the module- a shared library file. Also, you can load the pre-compiled module which is provided as `mesibo_mod_skeleton.so`

The Skeleton Module serves as a reference for all other Sample Modules. The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/on-premise/loadable-modules/)

### 1. The C/C++ Source file
The module name is `skeleton`. The C/C++ Source file ie; `skeleton.c`. The header file `module.h` containing the definitions of all module related components are included in the C/C++ source as follows:
```cpp
#include "module.h"
```
### 2. Configuring the skeleton module
While loading your module you need to provide your configuration details in the file `/etc/mesibo/mesibo.conf`. The example configuration is provided in the file `sample.conf`. Copy the configuration from `sample.conf`into `/etc/mesibo/mesibo.conf`. 

The skeleton module is configured as follows:
```

module  skeleton {
    source = skeleton.cpp
    name = skeleton
    description = Bare bones version of a Mesibo Module
    log = 1
}

```
Please note that the above is just an example configuration. You can pass name-value pairs as per the configuration needs of your module. Any configuration parameters that you pass will be available through the initialization function.

### 3. Initializing the skeleton module
The skeleton module is initialized with the Mesibo Module Configuration details - module version, the name of the module and references to the module callback functions.
```cpp
int mesibo_module_skeleton_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
        MESIBO_MODULE_SANITY_CHECK(m, version, len);

        if(m->config) {
                mesibo_log(m, 0, "Following configuration item(s) were passed to the module\n");
                int i;
                for(i=0; i < m->config->count; i++) {
                        mesibo_log(m, 0, "module config item: name %s value %s\n",
                                        m->config->items[i].name, m->config->items[i].value);
                }
        }

        m->flags = 0;
        m->description = strdup("Sample Skeleton Module");

        //Initialize Callback functions
        m->on_cleanup = skeleton_on_cleanup;
        m->on_message = skeleton_on_message;
        m->on_message_status = skeleton_on_message_status;
        m->on_login = skeleton_on_login;
        mesibo_log(m, 0, "================> %s init called\n", m->name);

        return MESIBO_RESULT_OK;

}

```
### 4. skeleton_on_message
`on_message` will notify you for any message exchanged between users. You can intercept each message, decide what to do with it ie; process the message and then send a response. 

For example, here we simply send a message `"Hello from Skeleton Module"` to the user who sent the message. Observer that, this message will be sent from the module to any user who sends a message to any other user.

Returns:   
`MESIBO_RESULT_PASS` to pass the message data and parameters as it is 
 OR
`MESIBO_RESULT_CONSUMED` if the message is to be consumed. If consumed, the recipient will not receive THIS message.

```cpp

static mesibo_int_t skeleton_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, const char *message, mesibo_uint_t len) {
        mesibo_log(mod, 1, "================> %s on_message called\n", mod->name);
        mesibo_log(mod, 1, " from %s to %s id %u message %s\n", p->from, p->to, (uint32_t) p->id, message);

        //don't modify original as other module will be use it 
        mesibo_message_params_t np;
        memcpy(&np, p, sizeof(mesibo_message_params_t));
        np.to = p->from;
        np.from = p->to;
        np.id = rand();
        const char* test_message = "Hello from Skeleton Module";

        mesibo_message(mod, &np, test_message, strlen(message));
        return MESIBO_RESULT_PASS;
}

```
### 5. skeleton_on_message_status
This function is called when a message is sent from the skeleton module to indicate the status of the message which corresponds to different status codes such as `MSGSTATUS_SENT`, `MSGSTATUS_DELIVERED`, `MSGSTATUS_READ`, etc
```cpp

static mesibo_int_t skeleton_on_message_status(mesibo_module_t *mod, mesibo_message_params_t *p, mesibo_uint_t status) {
        mesibo_log(mod, 1, " %s on_message_status called\n", mod->name);
        mesibo_log(mod, 1, " from %s id %u status %d\n", p->from, (uint32_t)p->id, status);
        return 0;
}

    
```
For example, when we send the test message `"Hello from Skeleton Module"` to a user `skeleton_on_message_status` will be called with the corresponding message parameters and status data.

Returns:
MESIBO_RESULT_OK

### 6. skeleton_on_login
This function is called when a user logs-in or logs-out.

```cpp

static mesibo_int_t skeleton_on_login(mesibo_module_t *mod, mesibo_user_t *user) {
        mesibo_log(mod, 1, " %s on_login called\n", mod->name);
        return 0;
}
```
### 7. Making an HTTP request

You can make an HTTP POST request using the module function `mesibo_http`.
 
```
For example, if your request URL looks like  `https://example.com/api.php?op=test`   
The `url` parameter will be `"https://example.com/api.php"`
The `post` parameter will be `"op=test"`

The response to the HTTP request is available through the callback function `mesibo_module_http_data_callback_t`  with the callback data (of arbitrary data type) `cbdata`. If there are any additional parameters to the POST request such as authorization header, etc it can be passed in `opt.`
```cpp

static mesibo_int_t skeleton_http(mesibo_module_t *mod) {
        mesibo_http(mod, "https://example.com/api.php", "op=test", mesibo_module_http_data_callback, mod, NULL);
        return MESIBO_RESULT_OK;
}


```
### mesibo_module_http_data_callback : The HTTP callback function

The response to the HTTP request is received through this callback function. In this example, the `cbdata` contains the pointer to `mod` which is available after casting it from `void*` to `mesibo_module_t*`.

The `progress` of the response being received is logged. If `progress` reaches `100` it indicates that the complete response has been received.

```cpp


int mesibo_module_http_data_callback(void *cbdata, mesibo_int_t state, mesibo_int_t progress, const char *buffer, mesibo_int_t size) {
        mesibo_module_t *mod = (mesibo_module_t *)cbdata;
        mesibo_log(mod, 1, " http progress %d\n", progress);
        return MESIBO_RESULT_OK;
}

```

### Compiling the skeleton module

To compile the skeleton module, open the sample `MakeFile` provided. Change the `MODULE` to `skeleton`.

For example.

```
MODULE = skeleton
```

Run `make` from your source directory.

```
sudo make
```
### Loading the skeleton module 
To load your skeleton  module provide the configuration in `/etc/mesibo/mesibo.conf`. You can copy the configuration from `sample.conf` into `/etc/mesibo/mesibo.conf`and modify values accordingly. 

Mount the path to the module while running mesibo container. If `mesibo_mod_skeleton.so` is located at `/path/to/mesibo_mod_skeleton.so`, mount the directory as 
```
 -v path/to/mesibo_mod_skeleton.so:/path/to/mesibo_mod_skeleton.so

```

For example, if `mesibo_mod_skeleton.so` is located at `/usr/lib64/mesibo/`
```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
-v /etc/mesibo:/etc/mesibo
-net=host -d  \ 
mesibo/mesibo <app token>

```
