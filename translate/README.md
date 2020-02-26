#  Translate Module

This repository contains the source code for Translate Module. The translated module translates each message before sending it to the destination.   

To perform translation, you can use any Translation API of your choice such as Google Translate, IBM Watson language Translator, Microsoft Azure Translate, Alibaba Cloud Translation, etc

This  Sample Translate Module provides an example using **Google Translate**

- You can download the source code and compile it to obtain the module - a shared library file and then load it. 

- The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/on-premise/loadable-modules/)

- For a basic understanding of how Mesibo Modules work refer to the source code for [Skeleton Module](https://github.com/mesibo/onpremise-loadable-modules/tree/master/skeleton)

## Overview of  Translate Module
-  Translation configuration containing: Endpoint/base URL for making REST calls, target language, source language, access token is provided in the module configuration file ( Refer `sample.conf` )
- In module initialization, all the configuration parameters are obtained and stored in a structure object  `translate_config_t`. 
- The callback function for `on_message` is initialized to `translate_on_message`
- When a message is sent from a user, Mesibo invokes the `translate_on_message` callback function of the module with the message data and parameters.
- Then, the translate module makes an HTTP request to a REST endpoint of Google Translate. The HTTP POST data contains the message text and the target language.
- Google Translate sends the response with the text translated to target language which is received through an HTTP callback function.
- The original message is CONSUMED and the translated text is sent to the recipient. 

### Basics of Google Translate

Google Translate is a cloud translation API service. It is available through a REST interface. If you invoke the REST endpoint with your text and target language, it will translate the text into the target language specified.

For more details on using Google Translate, refer [Google Translate](https://translate.google.com)

## Configuring Google Translate
To use Google Translate API you need to to do the following from Google Cloud Console:
- Enable Google Translate for your project
- Obtain the access token 

Following are the steps:

1. Set up a [GCP Console](https://console.cloud.google.com) project.
    - Create or select a project and note the project ID
    - Enable the Cloud Translation API for that project.
    - Create a service account
    - Download a private service account key as JSON
    
2. Set the environment variable `GOOGLE_APPLICATION_CREDENTIALS` pointing to the JSON file downloaded in Step 1.

```
export GOOGLE_APPLICATION_CREDENTIALS="/home/user/Downloads/service-account-file.json"
```

3. [Install and initialize the Cloud SDK](https://cloud.google.com/sdk/docs/)
4. Print your access token by using the following command

```
echo $(gcloud auth application-default print-access-token)
```

which should output something like

```
ya29.c.Kl6iB-r90Gjj4o--m7k7wr4dN4b84U4TLEtPqdEZ2xvfsj01awmUObMDEFwJIJ1lkZPym5dsAw44MbZDSaksLH3xKbsSHGLgWeEXqIPSDmFO6
```
This is the access token, save it for later use.

#### Invoking Google Translate API 
Once we have enabled Google Translate and obtained an access token, invoking Google Translate API is as simple as invoking the REST URL with the access token and the data:

For example, the translate REST endpoint looks like

```
https://translation.googleapis.com/language/translate/v2

```
Now, you send a POST request to the above URL in the following format.

Pass the authentication information in the request header.

```
Authorization: Bearer <YOUR_ACCESS_TOKEN>
Content-Type: application/json
```

and the POST data which contains your text/message and the target [language code]( https://cloud.google.com/translate/docs/languages)

```
{
  "q": "Hello world",
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
      }
    ]
  }
}

```

## Building the Translate Module

### 1. Configuration of the translate module
The translate module uses Google Translate. The sample configuration is provided in the file `sample.conf`

The translate module is configured as follows:

```
module translate {
    endpoint = https://translation.googleapis.com/language/translate/v2
    access_token = <access token>
    source = <Source Language Code>
    target = <Target Language Code>
    log = <Log level>
}
```

where,

- `endpoint` The Google Translate REST endpoint to which your message will be sent and translated
- `access_token` access token linked with your project
- `source` Source Language from which the text needs to be translated
- `target` Target language into which the text needs to be translated 
- `log` Log level for printing to container logs


For example, To translate from English(`en`) to German(`de`) the configuration looks like:

```
module  translate {
    endpoint = https://translation.googleapis.com/language/translate/v2
    access_token = xxxxxx.Kl6iBzVH7dvV2XywzpnehLJwczdClfMoAHHOeTVNFkmTjqVX7VagKHH1-xxxxxxx
    source = en
    target = de
    log = 1 
}
```

### 3. Initialization of the translate module
Since the name of the module is `translate`, the translate module initialization function is `mesibo_module_translate_init`
and is defined as follows

```cpp
int mesibo_module_translate_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {

    MESIBO_MODULE_SANITY_CHECK(m, version, len);
    m->flags = 0;
        m->description = strdup("Sample Translate Module");
        m->on_message = translate_on_message;
    m->on_cleanup = translate_on_cleanup;
    
    if(m->config) {
        translate_config_t* tc = get_config_google_translate(m);
        if(NULL == tc){
            mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
            return MESIBO_RESULT_FAIL;
        }
        int init_status= translate_init_google(m);
        if(MESIBO_RESULT_OK != init_status){
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
First, a sanity check is performed using `MESIBO_MODULE_SANITY_CHECK` and then the configuration is retrieved and stored in the module context `m->ctx` so that it is available whenever module callbacks are invoked.

The configuration structure `translate_config_t` is used to store the configuration:

```cpp
typedef struct translate_config_s {
        /* To be configured in module configuration file */
        const char* access_token;
        const char* endpoint;
        const char* source;
        const char* target;
        int log;

        /* To be configured by Google Translate init function */
        char* auth_bearer;
        module_http_option_t* translate_http_opt;

} translate_config_t;

```

To get the configuaration information the config helper function `get_config_google_translate` is called.

```cpp
static translate_config_t*  get_config_google_translate(mesibo_module_t* mod){
        translate_config_t* tc = (translate_config_t*)calloc(1, sizeof(translate_config_t));
        tc->access_token = mesibo_util_getconfig(mod, "access_token");
        tc->endpoint = mesibo_util_getconfig(mod, "endpoint");
        tc->source = mesibo_util_getconfig(mod, "source");
        tc->target = mesibo_util_getconfig(mod, "target");
        tc->log = mesibo_util_getconfig(mod, "log");

        mesibo_log(mod, tc->log,  " Configured Google Translate :\n endpoint %s \n"
                        " source %s \n target %s\n access_token %s\n", 
                        tc->endpoint, tc->source, tc->target, tc->access_token);
        mod->ctx = tc;

        return tc;
```    

### Initialization of REST API parameters
Once the configuration is obtained, the REST API parameters (URL and header) are constructed so that we can use it when required, rather than constructing them at runtime. 

```cpp
static int translate_init_google(mesibo_module_t* mod){
        translate_config_t* tc = (translate_config_t*)mod->ctx;

        asprintf(&tc->auth_bearer, "Authorization: Bearer %s", tc->access_token);
        mesibo_log(mod, tc->log, "Configured auth bearer for HTTP requests with token: %s \n", tc->auth_bearer );

        tc->translate_http_opt = mesibo_translate_get_http_opt(tc);

        return MESIBO_RESULT_OK;
}
```

### 4. `translate_on_message`
The translate module intercepts each message, translates the message and sends the translated text to the recipient. On receiving a message, `translate_process_message` is called For translating the message.

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

### 5. Message Translation

To translate incoming messages the module sends them to Google Translate and sends the translated text to the recipient.

To invoke Google Translate API, the helper function `mesibo_http` is used. Google Translate expects the request data in JSON format. Ideally, a JSON library could be used to encode the request. However, JSON libraries are typically slow and are overkill for this simple project. Hence, the raw post data string is directly constructed.

Once the response is received from Google Translate, it needs to be sent to the recipient. Hence, the context of the received message ie; message parameters, the sender of the message, the receiver of the message is stored in the following structure and passed as callback data in the HTTP request.


```cpp
typedef struct http_context_s {
        mesibo_module_t *mod;
        mesibo_message_params_t* params;
        char *from;
        char *to;
        char* post_data; //Cleanup after HTTP request is complete
        mesibo_int_t status;
        char response_type[HTTP_RESPONSE_TYPE_LEN];
        // To copy data in response
        char buffer[HTTP_BUFFER_LEN];
        int datalen;
} http_context_t;
```
The function to take the message and send an HTTP request to Google Translate is as follows:

```cpp
static int translate_process_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {

	translate_config_t* tc = (translate_config_t*)mod->ctx;
	const char* post_url = tc->endpoint; 

	char* raw_post_data;
	asprintf(&raw_post_data, "{\"q\":\"%.*s\", \"target\":\"%s\"}",
			(int)len, message, tc->target);

	http_context_t *http_context =
		(http_context_t *)calloc(1, sizeof(http_context_t));
	http_context->mod = mod;
	http_context->params = p;
	http_context->from = strdup(p->from);
	http_context->to = strdup(p->to);
	http_context->post_data= raw_post_data;
	

	mesibo_log(mod, tc->log,  "POST request %s %s %s %s \n", 
			post_url, raw_post_data,
			tc->translate_http_req->extra_header, 
			tc->translate_http_req->content_type);
	
	tc->translate_http_req->url = post_url; 
	tc->translate_http_req->post = raw_post_data;
	
	tc->translate_http_req->on_data = translate_http_on_data_callback;
	tc->translate_http_req->on_status = translate_http_on_status_callback;
	tc->translate_http_req->on_close = translate_http_on_close_callback;
	
	mesibo_util_http(tc->translate_http_req, (void *)http_context);

	return MESIBO_RESULT_OK;
}
```

### 6. Extracting the translated text

The response for the POST request is obtained in the HTTP callback function passed to mesibo_http. The response may be received in multiple chunks. Hence the response data is stored in a buffer until the complete response is received.

Google Translate sends the response as a JSON string with the response text encoded in the field `translatedText`. 
Hence, translated text needs to be extracted from the JSON string before we can send it to the recipient. A helper function `mesibo_util_json_extract` is used to extract the translated text from the JSON response.


```cpp
static mesibo_int_t translate_http_on_data_callback(void *cbdata, mesibo_int_t state,
		mesibo_int_t progress, const char *buffer,
		mesibo_int_t size) {
	http_context_t *b = (http_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;

	if (0 > progress) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE,  "Error in http callback \n");
		mesibo_translate_destroy_http_context(b);
		return MESIBO_RESULT_FAIL;
	}

	if (MODULE_HTTP_STATE_RESPBODY != state) {
		return MESIBO_RESULT_OK;
	}

	if ((MODULE_HTTP_STATE_RESPBODY == state) && buffer!=NULL && size!=0 ) {
		if(HTTP_BUFFER_LEN < (b->datalen + size )){
			mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE,
					"Error in http callback : Buffer overflow detected \n", mod->name);
			return MESIBO_RESULT_FAIL;
		}
		memcpy(b->buffer + b->datalen, buffer, size);
		b->datalen += size;
	}

	if (100 == progress) {
		//Response Complete
	}

	return MESIBO_RESULT_OK;
}

mesibo_int_t translate_http_on_status_callback(void *cbdata, mesibo_int_t status, const char *response_type){

        http_context_t *b = (http_context_t *)cbdata;
        if(!b) return MESIBO_RESULT_FAIL;
        mesibo_module_t* mod = b->mod;
        if(!mod) return MESIBO_RESULT_FAIL;
        translate_config_t* tc = (translate_config_t*)mod->ctx;

        b->status = status;
        if(NULL != response_type){
                memcpy(b->response_type, response_type, strlen(response_type));
                mesibo_log(mod, tc->log, "status: %d, response_type: %s \n", (int)status, response_type);
        }
        return MESIBO_RESULT_OK;
}

void translate_http_on_close_callback(void *cbdata,  mesibo_int_t result){

        http_context_t *b = (http_context_t *)cbdata;
        mesibo_module_t *mod = b->mod;

        if(MESIBO_RESULT_FAIL == result){
                mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "Invalid HTTP response \n");
                return;
        }

        //Send response and cleanup

	mesibo_message_params_t p;
	memset(&p, 0, sizeof(mesibo_message_params_t));
	p.id = rand();
	p.refid = b->params->id;
	p.aid = b->params->aid;
	p.from = b->from;
	p.to = b->to; 
	p.expiry = 3600;

	char* extracted_response = mesibo_util_json_extract( b->buffer , "translatedText", NULL);
	mesibo_message(mod, &p, extracted_response , strlen(extracted_response));

	mesibo_translate_destroy_http_context(b);	
}
```

### 7. Compiling the translate module

To compile the translate module from source run
```
make
```
from the source directory which uses the sample `Makefile` provided to build a shared object `mesibo_mod_translate.so`. It places the result at the `TARGET` location `/usr/lib64/mesibo/mesibo_mod_translate.so`

### 8. Loading the translate module 
To load the translate module provide the configuration in `/etc/mesibo/mesibo.conf`. You can copy the configuration from `sample.conf` into `/etc/mesibo/mesibo.conf`and modify values accordingly. 

Mount the directory which contains the module shared library while running the mesibo container.

If `mesibo_mod_translate.so` is located at `/path/to/mesibo_mod_translate.so`, you should mount the directory as 
```
 -v path/to/mesibo_mod_translate.so:/path/to/mesibo_mod_translate.so

```

For example, if `mesibo_mod_translate.so` is located at `/usr/lib64/mesibo/`
```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
-v /etc/mesibo:/etc/mesibo
-net=host -d  \ 
mesibo/mesibo <app token>
```
