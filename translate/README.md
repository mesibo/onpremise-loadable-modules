#  Translate Module

This repository contains the source code for Translate Module. The translate module translates each message before sending it to destination.   

To perform translation, you can use any Translation API of your choice such as Google Translate, IBM Watson language Translator, Microsoft Azure Translate, Alibaba Cloud Translation, etc
This  Sample Translate Module provides an example using **Google Translate**

You can download the source and compile it to obtain the module- a shared library file. Also, you can load the pre-compiled module which is provided as `mesibo_mod_translate.so`

Refer to the [Skeleton Module](https://github.com/Nagendra1997/mesibo-documentation/blob/master/skeleton.md) for a basic understanding of how Mesibo Modules work. The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/loadable-modules/)

## Overview of  Translate Module
-  Translation configuration containing : Endpoint/base url for making REST call, target language, source language, access token (Service account key  for authenticating your requests) is provided in the module configuration (In the file `/etc/mesibo/mesibo.conf`).
- In module initialisation, all the configuration parameters is obtained from the configuration list and stored in a structure object  `translate_config_t`. The authorisation header is constructed and stored in the form 
`" Authorisation : Bearer <access token> "`
- In module initialisation,The callback function for `on_message` is initialized to  `translate_on_message`
- When a message is sent from a user, Mesibo invokes the `translate_on_message` callback function of the module with the message data and it’s associated message parameters such as sender, expiry, flags, etc.
- Now, the translate module makes an HTTP request to a REST endpoint of Google Translate .The HTTP Post data contains the message text, target language and source language. The authorisation header containing the access token is also sent in the POST request.
- Google Translate sends the response with the text translated to target language (a JSON string), which is recieved through an http callback function.
- The orginal message is CONSUMED and the translated message is sent to the recipient. (In this example, The entire JSON response string from Google Translate is sent to the recipient)

### Basics of Google Translate

Google Translate is a cloud translation API service. It is available through a REST interface. If you invoke the REST endpoint with your text and target language, it will translate the text into the target language specified.

The target language is specified through [language codes](https://cloud.google.com/translate/docs/languages)

For example, the translate REST endpoint looks like

```
https://translation.googleapis.com/language/translate/v2

```

The POST request format is as follows:

Every request you make needs to be authenticated. This is done by passing the access token(service account key) in the header.
Headers:
```
Authorization: Bearer YOUR_SERVICE_ACCOUNT_KEY
Content-Type: application/json

The POST data will contain your text/message.

```
POST body:
{
  "q": ["Hello world", "My name is Jeff"],
  "target": "de"
}
```

You should see a JSON response similar to the following:
```
{
  "data": {
    "translations": [
      {
        "translatedText": "Hallo Welt",
        "detectedSourceLanguage": "en"
      },
      {
        "translatedText": "Mein Name ist Jeff",
        "detectedSourceLanguage": "en"
      }
    ]
  }
}

```

## Building the Translate Module

### 1. The C/C++ Source file
The module name is `translate`. The C/C++ Source file is `translate.c`. The header file `module.h` containing the definitions of all module related components is included in the C/C++ source as follows:
```cpp
#include "module.h"
```
### 2. Configuring the translate module
While loading your module you need to provide your configuration details in the file `/etc/mesibo/mesibo.conf`. The sample configuration is provided in the file `translate.conf`. Copy the configuration from `translate.conf`into `/etc/mesibo/mesibo.conf`

The translate module is configured as follows:

```

module = translate{
endpoint = https://translation.googleapis.com/language/translate/v2
access_token = <access token>
source = <Source Language Code>
target = <Target Language Code>
log = <Log level>
}


```

For example, To translate from English(`en`) to German(`de`) the configuration looks like:
```
module=translate{
endpoint=https://translation.googleapis.com/language/translate/v2
access_token=xxxxxx.Kl6iBzVH7dvV2XywzpnehLJwczdClfMoAHHOeTVNFkmTjqVX7VagKHH1-xxxxxxx
source=en
target=de
log = 1
}

```
### Configuration paramters:

- `endpoint` The Google Translate REST endpoint to which your message will be sent and translated, Example: `https://translation.googleapis.com/language/translate/v2`

- `access_token` access token linked with your project
- `source` Source Language code from which the text needs to be translated
- `target` Target [language code]( https://cloud.google.com/translate/docs/languages)  into which the text needs  to be translated 
- `log` Log level for printing to container logs

### 3. Initializing the translate module
The translate module is initialized with the Mesibo Module Configuration details - module version, the name of the module and  references to the module callback functions.

```cpp

int mesibo_module_translate_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {

	MESIBO_MODULE_SANITY_CHECK(m, version, len);
	m->flags = 0;
        m->description = strdup("Sample Translate Module");
        m->on_message = translate_on_message;

	if(m->config) {
		translate_config_t* tc = get_config_google_translate(m);
		if(tc  == NULL){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}
		int init_status= translate_init_google(m);
		if(init_status!=MESIBO_RESULT_OK){
			mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Bad Configuration\n", m->name);
			return MESIBO_RESULT_FAIL;
		}

		m->ctx = (void* )tc;
	}

	else {
		mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
		return MESIBO_RESULT_FAIL;
	}


	return MESIBO_RESULT_OK;
}
```
A pointer to the translate configuration is stored in `m->ctx`

### Storing the translate configuration in module context

The translation configuration parameters is extracted from module configuration list and stored in the configuration context structure `translate_config_t` which is defined as follows:
```cpp

typedef struct translate_config_s {
        /* To be configured in module configuration file */
        char* access_token;
        char* endpoint;
        char* source;
        char* target;
        int log;

        /* To be configured by Google Translate init function */
        char* post_url;
        char* auth_bearer;
        module_http_option_t* translate_http_opt;

} translate_config_t;

```

To get the configuaration details the config helper function `get_config_google_translate` is called.

```cpp
static translate_config_t*  get_config_google_translate(mesibo_module_t* mod){
        translate_config_t* tc = (translate_config_t*)calloc(1, sizeof(translate_config_t));
        tc->endpoint = mesibo_util_getconfig(mod, "endpoint");
        tc->source = mesibo_util_getconfig(mod, "source");
        tc->target = mesibo_util_getconfig(mod, "target");
        tc->access_token = mesibo_util_getconfig(mod, "access_token");

        mesibo_log(mod, tc->log,  " Configured Google Translate :\n endpoint %s \n"
                        " source %s \n target %s\n access_token %s\n",
                        tc->endpoint, tc->source, tc->target, tc->access_token);
        mod->ctx = tc;

        return tc;
}

```    

### Initialising the request parmaters
The translate module will interact with the Google Translate Service over REST API. This example uses [Cloud Translate API v2](https://cloud.google.com/translate/docs/translating-text) method to translate text . 

Once we get the configuration details, we can construct the request parameters such as the post url (endpoint) and the authorisation header. To do this we call the `translate_init_google` function as follows:

```cpp
static int translate_init_google(mesibo_module_t* mod){
        translate_config_t* tc = (translate_config_t*)mod->ctx;

        tc->post_url = tc->endpoint;
        mesibo_log(mod, tc->log,  "Configured post url for HTTP requests: %s \n", tc->post_url);

        int cv;
        cv = asprintf(&tc->auth_bearer,"Authorization: Bearer %s", tc->access_token);
        if(cv == -1)return MESIBO_RESULT_FAIL;
        mesibo_log(mod, tc->log, "Configured auth bearer for HTTP requests with token: %s \n", tc->auth_bearer );

        tc->translate_http_opt = mesibo_translate_get_http_opt(tc);

        return MESIBO_RESULT_OK;
```

### 4. `translate_on_message`
`translate_on_message` will notify the translate module when a message is sent from a user. The translate module intercepts each message , translates the message and sends the translated text to the recipient. On recieving a message ,`translate_process_message` is called For translating the message which makes an HTTP request to a Google Translate Service REST endpoint. The response from Google Translate containing the translated text is recieved through an http callback function and is sent to the recipient.


```cpp
static mesibo_int_t translate_on_message(mesibo_module_t *mod, mesibo_message_params_t *p, 
                const char *message, mesibo_uint_t len) { 
 
        translate_config_t* tc = (translate_config_t*)mod->ctx; 
 
        // Don't modify original as other module will use it 
        mesibo_message_params_t *np = (mesibo_message_params_t *)calloc(1, sizeof(mesibo_message_params_t)); 
        memcpy(np, p, sizeof(mesibo_message_params_t)); 
        translate_process_message(mod, np, message, len); 
 
        return MESIBO_RESULT_CONSUMED;  // Process the message and CONSUME original 
} 
```

### 5. Translating the message

Get the translate configuration from module context `mod->ctx` (Casting from `void*` to `translate_config_t*` ).  Following this [POST request format](https://cloud.google.com/translate/docs/translating-text) we send an HTTP request using the function `mesibo_http`. 

Google Translate expects the request data in the JSON format. Ideally, we could have used a JSON library to encode the request.However, JSON libraries are typically slow and is an overkill for this simple project. Hence, we directly construct the raw post data string.

On recieving the response in the http callback function `translate_http_callback`(which we shall define in the next step), from Google Translate we need to send the response to the recipient. So we store the context of the received message ie; message parameters ,the sender of the message,the reciever of the message in the following structure and pass it as callback data. 


```cpp
static int translate_process_message(mesibo_module_t *mod, mesibo_message_params_t *p,
                const char *message, mesibo_uint_t len) {

        translate_config_t* tc = (translate_config_t*)mod->ctx;
        const char* post_url = tc->post_url;

        char* raw_post_data;
        int cv;
        cv = asprintf(&raw_post_data,"{\"q\":[\"%.*s\"], \"target\":\"%s\"}",
                        (int)len, message, tc->target);
        if(cv == -1) return MESIBO_RESULT_FAIL;


        message_context_t *message_context =
                (message_context_t *)calloc(1, sizeof(message_context_t));
        message_context->mod = mod;
        message_context->params = p;
        message_context->from = strdup(p->from);
        message_context->to = strdup(p->to);

    	mesibo_log(mod, tc->log,  "POST request %s %s %s %s \n",
                        post_url, raw_post_data,
                        tc->translate_http_opt->extra_header,
                        tc->translate_http_opt->content_type);

        mesibo_http(mod, post_url, raw_post_data, translate_http_callback,
                        (void *)message_context, tc->translate_http_opt);

        return MESIBO_RESULT_OK;
}
```

### 6. Extracting the translated text
The response from Google Translate is recieved through a callback function.
Google Translate sends the response as a JSON string with the response text encoded in the field `translatedText`. So, we extract the response from the JSON string using the utility function `mesibo_util_json_extract` and send it using `mesibo_message`.


```cpp

static int translate_http_callback(void *cbdata, mesibo_int_t state,
		mesibo_int_t progress, const char *buffer,
		mesibo_int_t size) {
	message_context_t *b = (message_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;
	translate_config_t* tc = (translate_config_t*)mod->ctx;
	mesibo_message_params_t *params = b->params;

	if (progress < 0) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE,  "%s : Error in http callback \n", mod->name);
		mesibo_translate_destroy_message_context(b);
		return MESIBO_RESULT_FAIL;
	}

	if (state != MODULE_HTTP_STATE_RESPBODY) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "Exit http callback\n");
		if(size)
			mesibo_log(mod, tc->log,  "%.*s \n", size, buffer);
		return MESIBO_RESULT_OK;
	}

	if ((progress > 0) && (state == MODULE_HTTP_STATE_RESPBODY)) {
		if(b->datalen + size > HTTP_BUFFER_LEN){
			mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, 
					"%s : Error http callback : Buffer overflow detected \n", mod->name);
			return -1;
		}

		memcpy(b->buffer + b->datalen, buffer, size);
		b->datalen += size;
	}

	if (progress == 100) {
		mesibo_log(mod, tc->log,  "%.*s", b->datalen, b->buffer);
		mesibo_message_params_t p;
                memset(&p, 0, sizeof(mesibo_message_params_t));
                p.id = rand();
                p.refid = params->id;
                p.aid = params->aid;
                p.from = b->from;
                p.to = b->to; 
                p.expiry = 3600;

		char* extracted_response = mesibo_util_json_extract( b->buffer , "translatedText", NULL);
		mesibo_log(mod, tc->log,  "\n Extracted Response Text \n %s \n", extracted_response);
		int mv = mesibo_message(mod, &p, extracted_response , strlen(extracted_response));

		mesibo_translate_destroy_message_context(b);	
	}

	return MESIBO_RESULT_OK;
}

```

### 7. Compiling the translate module

To compile the translate module from source run
```
make
```
from the source directory which uses the sample `Makefile` provided to build a shared object `mesibo_mod_translate.so`. It places the result at the `TARGET` location `/usr/lib64/mesibo/mesibo_mod_translate.so` which you can verify.

### 8. Loading the translate module 
You can load the pre-compiled module (.so) by specifying the matching configuration in `mesibo.conf` as provided in `translate.conf` and mount the directory which contains the module shared library while running the mesibo container.

If you are loading a pre-compiled module make sure that you have mounted the path to the .so file. If `mesibo_mod_translate.so` is located at `/path/to/mesibo_mod_translate.so`,you should mount the directory as 
```
 -v path/to/mesibo_mod_translate.so:/path/to/mesibo_mod_translate.so

```

For example, if `mesibo_mod_translate.so` is located at `/usr/lib64/mesibo/`
```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
-v /etc/mesibo:/etc/mesibo
-net=host -d  \ 
mesibo/mesibo <app token>

