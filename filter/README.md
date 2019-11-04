# Filter Module

This repository contains the source code for the Filter Module to monitor and moderate all the messages between your users (for example, to drop messages containing profanity).

In this filter example, we will implement a simple profanity filter to drop messages containing profanity. You can download the source and compile it to obtain the module- a shared library file. 

Refer to the [Skeleton Module](https://github.com/Nagendra1997/mesibo-documentation/blob/master/skeleton.md) for a basic understanding of how Mesibo Modules work. The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/on-premise/loadable-modules/)

## Overview of Filter Module
- A list of words defined as profanity or blocked words is provided in the module configuration (In the file `mesibo.conf`).
- In module initialization, the profanity list is obtained from the configuration items list and the callback function for `on_message` is initialized to  `filter_on_message`
- When a message is exchanged between users, Mesibo invokes the `filter_on_message` callback function of the module with the message data and it’s associated message parameters such as sender, expiry, flags, etc.
- The filter module analyzes the message to find any profanity or objectionable content and returns whether the message is safe or not.
- If no profanity was found, `filter_on_message` returns `MESIBO_RESULT_PASS` and the message is safely sent to the recipient. If the message is found to contain profanity `filter_on_message` returns `MESIBO_RESULT_CONSUMED` and the unsafe message is dropped and prevented from reaching the receiver.

## Building the Filter Module

### 1. The C/C++ Source file
The module name is `filter`. The C/C++ Source file is `filter.c`. The header file `module.h` containing the definitions of all module related components are included in the C/C++ source as follows:
```cpp
#include "module.h"
```
### 2. Configuring the filter module
While loading your module you need to provide your configuration details in the file `/etc/mesibo/mesibo.conf`. The example configuration is provided in the file `filter.conf`. Copy the configuration from `filter.conf`into `/etc/mesibo/mesibo.conf`

The filter module is configured as follows:
- The number of blocked words/profanity is provided in `count`
- The list of blocked words/profanity is provided as a list of comma-separated values. 
ie;`blocked_word_1`, `blocked_word_2`, `blocked_word_3`, ... `blocked_word_N`. 
```

For example,
module=filter{
count= 3
blocked_words =alpha, beta, gamma 
log = 1
}

```
### 3. Initializing the filter module
The filter module is initialized with the Mesibo Module Configuration details - module version, the name of the module and references to the module callback functions.
```cpp
int mesibo_module_filter_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
        MESIBO_MODULE_SANITY_CHECK(m, version, len);

        m->flags = 0;
        m->description = strdup("Sample Filter Module");
        m->on_message = filter_on_message;

        if(m->config){
                filter_config_t* fc = get_config_filter(m);
                if(fc == NULL){
                        mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
                        return MESIBO_RESULT_FAIL;
                }
                m->ctx = (void*)fc;
        }
        else {
                mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
                return MESIBO_RESULT_FAIL;
        }

        m->on_cleanup = filter_on_cleanup; //Cleanup memory on exit
        return MESIBO_RESULT_OK;
}

```
The profanity list is obtained from the configuration items list and the callback function for `on_message` is initialized to  `filter_on_message`

### Storing the configuration in the module context
The profanity list is extracted from the module configuration list and stored in the configuration context structure `filter_config_t` which is defined as follows:

```cpp
typedef struct filter_config_s{
        int  count;
        char** blocked_words;
        int log;
}filter_config_t;
```

To get the configuration details the config helper function `get_config_filter` is called.
The list of blocked words is a single string (It is a `value` in the list of name-value pairs in configuration items. `blocked_words` is the `name` and the list of comma separated blocked words is the `value` which is stored as a SINGLE string). `strtok` breaks down the string into a sequence of tokens. We traverse through the sequence and store each blocked word in an array.

```cpp
static filter_config_t* get_config_filter(mesibo_module_t* mod){
        filter_config_t* fc = (filter_config_t*)calloc(1, sizeof(filter_config_t));
        char* bw = mesibo_util_getconfig(mod, "blocked_words"); //comma seperated blocked words 
        int log = atoi( mesibo_util_getconfig(mod, "log")); //loglevel

        fc->blocked_words = (char**)malloc(sizeof(char *)*fc->count);

        int i = 0;
        const char* delimitter = ", ";
        const char* token = strtok(bw, delimitter); // blocked_words: bw1, bw2, bw3, ...  
        while(token != NULL ){
                fc->blocked_words[i] = mesibo_strupr(strdup(token));
                token = strtok(NULL, delimitter);
                i++;
        }
        return fc;
}
```    

A pointer to the filter configuration is stored in `m->ctx`

### 4. filter_on_message
`filter_on_message` will notify the filter module when any message is exchanged between users. The filter module intercepts each message and decides what to do with it. It analyzes the message for profanity and decides to PASS the message as SAFE or CONSUME the message to prevent the unsafe message reaching the recipient. 

### Analyzing the message for profanity
Get the filter configuration from module context `mod->ctx` (Casting from `void*` to `filter_config_t*` ). Traverse the list of blocked words and check if the message contains the blocked word. This example is an extremely simplified implementation of a profanity filter. It is not a regex matching type filter, so it only detects profanity or blocked word if it is seen as an individual string. It cannot detect if the blocked word is a substring of a word.

If no profanity was found return `MESIBO_RESULT_PASS` and the message is safely sent to the recipient. If the message is found to contain profanity return `MESIBO_RESULT_CONSUMED` and the unsafe message is dropped and prevented from reaching the receiver.


```cpp
static mesibo_int_t filter_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
                const char *message, mesibo_uint_t len) {

        char* in_message = strndup(message, len);
        in_message = mesibo_strupr(in_message) ; //Convert to UPPER_CASE

        filter_config_t* fc = (filter_config_t*)mod->ctx;
        int i;
        for(i =0; i< fc->count ;i++){ //Loop through list of blocked words

                if(strstr(in_message, fc->blocked_words[i])){ //Message Contains blocked word 
                        free(in_message);
                        //drop message and prevent message from  reaching the recipient
                        return MESIBO_RESULT_CONSUMED;
                }
        }

        free(in_message);
        return MESIBO_RESULT_PASS;
        // PASS the message as it is, after checking that it is SAFE
}

```


Returns:   
`MESIBO_RESULT_PASS` If no profanity was found. The message is safely sent to the recipient
 OR
`MESIBO_RESULT_CONSUMED` If the message is found to contain profanity. The unsafe message is dropped and prevented from reaching the receiver.

### 5. Compiling the filter module
To compile the filter module, open the sample `MakeFile` provided. Change the `MODULE` to `filter`.

For example.

```
MODULE = filter 
```

Run `make` from your source directory.

```
sudo make
```

It places the result at the `TARGET` location `/usr/lib64/mesibo/mesibo_mod_filter.so` which you can verify.

### 6. Loading the filter module 

Mount the directory containing the module while running the mesibo container.
If `mesibo_mod_filter.so` is located at `/usr/lib64/mesibo/`
```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
-v /etc/mesibo:/etc/mesibo
-net=host -d  \ 
mesibo/mesibo <app token>
