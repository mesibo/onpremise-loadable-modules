#  Chatbot Module


This repository contains the source code for Chatbot Module. Chatbot module analyzes messages using various AI and machine learning tools like Tensorflow, Dialogflow, etc. and sends an automatic reply.  

To build chatbots, you can use any chatbot provider of your choice such as Dialogflow, IBM Watson, Rasa, etc and connect with them through REST endpoints. 
This Chatbot Module provides an example using **Dialogflow**

- You can download the source and compile it to obtain the module- a shared library file. 

- The complete documentation for Mesibo Modules is available here

- Refer to the [Skeleton Module](https://github.com/mesibo/onpremise-loadable-modules/tree/master/skeleton) for a basic understanding of how Mesibo Modules work. The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/on-premise/loadable-modules/)


## Overview of the Chatbot Module
- Chatbot configuration containing: project name, session id, Endpoint/base URL for making REST calls, access token is provided in the module configuration file (Refer `sample.conf`).
- In module initialization, all the configuration parameters are obtained and stored in a structure object `chatbot_config_t`. 
- The callback function for `on_message` is initialized to  `chatbot_on_message`
- When a message is sent from a user to the chatbot `address`, Mesibo invokes the `chatbot_on_message` callback function of the module with the message data and message parameters.
- Now, the chatbot module makes an HTTP request to a REST endpoint of Dialogflow. The HTTP Post body contains the message text. The authorization header containing the access token is also sent in the POST request.
- Dialogflow sends a response, which is received through an HTTP callback function.
- The response text from the chatbot is extracted from the JSON response and is sent to the user (sender)

### Basics of Dialogflow
Dialogflow is an AI-powered Google service to build interactive conversational interfaces, such as chatbots. Once you train DialogFlow using data of your interest like emails, FAQs, etc., it can answer questions from your users in natural language.

Dialogflow service is available through a REST interface. For more details on using Dialogflow , refer [DialogFlow Documentation](https://cloud.google.com/dialogflow/docs/quick/api)

#### Configuring Dialogflow API 
To use Dialogflow API, you will need two parameters which you can obtain from the Google Cloud Console.

  - Project ID
  - Access Token

Following are the steps:

1. Set up a [GCP Console](https://console.cloud.google.com) project.
    - Create or select a project and note the project ID
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

#### Invoking Dialogflow API  
Once we have project ID and access token, invoking DialogFlow API is as simple as invoking the following URL with the access token and the data:

```
https://dialogflow.googleapis.com/v2/projects/<Project ID>/agent/sessions/<Session ID>
```

Where Project ID is the GCP Project ID obtained earlier. The session ID can be a random number or some type of user and session identifiers (preferably hashed). 

For example, a sample dialogflow REST URL looks like

```
https://dialogflow.googleapis.com/v2/mesibo-dialogflow/agent/sessions/123456789
```

Now, you send a POST request to the above URL in the following format.

Pass the authentication information in the request header.

```
Authorization: Bearer <YOUR_ACCESS_TOKEN>
Content-Type: application/json
```

and the POST data which contains your text/message.

```
{
    "queryInput": {
     {
    
    "text": {
    "text": <message text>,
    "languageCode" : <source language>
        }
    }   
    }
}
```

### Configuring the chatbot module
While loading your module you need to provide your configuration details in the file `/etc/mesibo/mesibo.conf`. The sample configuration is provided in the file `sample.conf`. 
The chatbot module is configured as follows:

```
module chatbot{
        project = <Project ID>
        endpoint = <Dialogflow REST Endpoint>
        access_token = <Service Account key>
        language = <Source Language>
        address = <Chatbot User Address>
        log = <log level>
}
```

where, 
- `project`, GCP Project ID that contains your dialogflow chatbot.
- `endpoint`, The dialogflow REST endpoint to which your query will be sent
- `access_token`, access token linked with your project. 
- `language`, The language code of the query text sent
- `address`, chatbot address. In your Mesibo Application, create a user that you can refer to as a chatbot endpoint user. 
- `log`, Log level for printing to mesibo container logs


For example,
```
module chatbot{
    project = mesibo-chatbot
    endpoint = https://dialogflow.googleapis.com/v2
    access_token = x.Kl6iBzVH7dvV2XywzpnehLJwczdClfMoAHHOeTVNFkmTjqVX7VagKH1
    language = en
    address = my_chatbot
    log = 1
}
```

### 3. Initialization of the chatbot module
The chatbot module is initialized with the module description and references to the module callback functions.
```cpp

int mesibo_module_chatbot_init(mesibo_int_t version, mesibo_module_t *m, mesibo_uint_t len) {
    mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "mesibo_module_%s_init called \n", m->name);

    MESIBO_MODULE_SANITY_CHECK(m, version, len);

    m->flags = 0;
    m->description = strdup("Sample Chatbot Module");
    m->on_message = chatbot_on_message;


    if(m->config) {
        chatbot_config_t* cbc = get_config_dialogflow(m);
        if(NULL == cbc){
            mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
            return MESIBO_RESULT_FAIL;
        }
        m->ctx = (void* )cbc;

        int init_status = chatbot_init_dialogflow(m);
        if(MESIBO_RESULT_OK != init_status){
            mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Bad Configuration\n", m->name);
            return MESIBO_RESULT_FAIL;
        }

    }

    else {
        mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
        return MESIBO_RESULT_FAIL;
    }


    return MESIBO_RESULT_OK;
}

```

First, a sanity check is performed using `MESIBO_MODULE_SANITY_CHECK` and then the configuration is retrieved and stored in the module context m->ctx so that it is available whenever module callbacks are invoked.
The configuration structure chatbot_config_t is used to store the configuration:

```cpp
typedef struct chatbot_config_s {
        /* To be configured in module configuration file */
        const char* project;
        const char* endpoint;
        const char* access_token;
        const char* address;
        const char* language;
        int log;

        /* To be configured by Dialogflow init function */
        char* post_url;
        char* auth_bearer;
        module_http_option_t* chatbot_http_opt;

} chatbot_config_t;

```

To get the configuration information, the config helper function `get_config_dialogflow` is called.

`get_config_dialogflow` fetches all the configuration details as per format and stores into the appropriate members of the structure `chatbot_config_t`

```cpp
static chatbot_config_t*  get_config_dialogflow(mesibo_module_t* mod){
        chatbot_config_t* cbc = (chatbot_config_t*)calloc(1, sizeof(chatbot_config_t));
        cbc->project = mesibo_util_getconfig(mod, "project");
        cbc->endpoint = mesibo_util_getconfig(mod, "endpoint");
        cbc->access_token = mesibo_util_getconfig(mod, "access_token");
        cbc->address = mesibo_util_getconfig(mod, "address");
        cbc->language = mesibo_util_getconfig(mod, "language");
        cbc->log = atoi(mesibo_util_getconfig(mod, "log"));

        mesibo_log(mod, cbc->log, "Configured DialogFlow :\nproject %s\nendpoint %s\naccess_token %s\n"
                        "language %s\naddress %s\n", cbc->project, cbc->endpoint,
                        cbc->access_token, cbc->address);

        return cbc;
}
```

### Initialization of REST API PARAMETERS

Once the configuration is obtained, the REST API parameters (URL and header) are constructed so that we can use it when required, rather than constructing them at runtime.


```cpp
static int chatbot_init_dialogflow(mesibo_module_t* mod){
        chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

        asprintf(&cbc->post_url, "%s/projects/%s/agent/sessions",
                        cbc->endpoint, cbc->project);
        mesibo_log(mod, cbc->log, "Configured post URL for HTTP requests: %s \n", cbc->post_url);

        asprintf(&cbc->auth_bearer, "Authorization: Bearer %s", cbc->access_token);
        mesibo_log(mod, cbc->log, "Configured auth bearer for HTTP requests with token: %s \n", cbc->auth_bearer );

        cbc->chatbot_http_opt = mesibo_chatbot_get_http_opt(cbc);

        return MESIBO_RESULT_OK;
}
```
### 3.`chatbot_on_message`

The module only needs to process messages addressed to the configured `address` of the chatbot. All other messages are passed as it is.

```cpp
static mesibo_int_t chatbot_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
                const char *message, mesibo_uint_t len) {

        chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

        if(0 == strcmp(p->to, cbc->address)){
                // Don't modify original as other module will use it
                mesibo_message_params_t* np = (mesibo_message_params_t*)malloc(sizeof(mesibo_message_params_t));
                memcpy(np, p, sizeof(mesibo_message_params_t));
                chatbot_process_message(mod, np, message, len);

                return MESIBO_RESULT_CONSUMED;  // Process the message and CONSUME original
        }

        return MESIBO_RESULT_PASS;
}
```
### 4. Processing the query 
 
To process the incoming messages, the module needs to send them to DialogFlow and send the response back to the user.

To invoke Dialogflow API, the helper function `mesibo_http` is called. DialogFlow expects the request data in JSON format. Ideally,  a JSON library could be used to encode the request.  However, JSON libraries are typically slow and are overkill for this simple project. Hence, the raw post data string is directly constructed.

Once the response is received from DialogFlow, the module needs to send it to to the user who made the request. Hence, the context of the received message ie; message parameters, the sender of the message, the receiver of the message is stored in the following structure and passed as callback data in the HTTP request. 

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

The function to process the message and send an HTTP request to Dialogflow is as follows:

```cpp
static mesibo_int_t chatbot_process_message(mesibo_module_t *mod, mesibo_message_params_t *p,
		const char *message, mesibo_uint_t len) {

	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	char post_url[HTTP_POST_URL_LEN_MAX];
	sprintf(post_url, "%s/%lu:detectIntent", cbc->post_url, p->id); //Pass Message ID as Session ID
	char* raw_post_data; 
	asprintf(&raw_post_data, "{\"queryInput\":{\"text\":{\"text\":\"%.*s\", \"languageCode\":\"%s\"}}}",
			(int)len, message, cbc->language);

	http_context_t *http_context =
		(http_context_t *)calloc(1, sizeof(http_context_t));
	http_context->mod = mod;
	http_context->params = p;
	http_context->from = strdup(p->from);
	http_context->to = strdup(p->to);
	http_context->post_data = raw_post_data;

	mesibo_log(mod, cbc->log , "%s %s %s %s \n", post_url, raw_post_data, cbc->chatbot_http_req->extra_header,
			cbc->chatbot_http_req->content_type);

	cbc->chatbot_http_req->url = post_url;
	cbc->chatbot_http_req->post = raw_post_data;
	
	cbc->chatbot_http_req->on_data = chatbot_http_on_data_callback;
	cbc->chatbot_http_req->on_status = chatbot_http_on_status_callback;
	cbc->chatbot_http_req->on_close = chatbot_http_on_close_callback;

	mesibo_util_http(cbc->chatbot_http_req, (void *)http_context);

	return MESIBO_RESULT_OK;
}

```

### 5. Extracting the response-text

The response for the POST request is obtained in the HTTP callback function passed to `mesibo_http`. The response may be received in multiple chunks. Hence the response data is stored in a buffer until the complete response is received. 

Dialogflow sends the response as a JSON string with the response text encoded in the field `fulfillmentText`. Hence, the response from the JSON string is extracted before it can be sent back to the user. The helper function `mesibo_util_json_extract` is used to extract textual response from the JSON response.

The message-id of the query message is passed as reference-id for the response message. This way the client who sent the message will be able to match the response received with the query sent. 


```cpp

static mesibo_int_t chatbot_http_on_data_callback(void *cbdata, mesibo_int_t state,
		mesibo_int_t progress, const char *buffer,
		mesibo_int_t size) {
	http_context_t *b = (http_context_t *)cbdata;
	mesibo_module_t *mod = b->mod;

	
	if (progress < 0) {
		mesibo_log(mod, MODULE_LOG_LEVEL_0VERRIDE, "Error in http callback \n");
		mesibo_chatbot_destroy_http_context(b);
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
		//Response complete
		return MESIBO_RESULT_OK;
	}

	return MESIBO_RESULT_OK;
}

mesibo_int_t chatbot_http_on_status_callback(void *cbdata, mesibo_int_t status, const char *response_type){

	http_context_t *b = (http_context_t *)cbdata;
	if(!b) return MESIBO_RESULT_FAIL;
	mesibo_module_t* mod = b->mod;
	if(!mod) return MESIBO_RESULT_FAIL;
	chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

	b->status = status;
	if(NULL != response_type){
		memcpy(b->response_type, response_type, strlen(response_type));
		mesibo_log(mod, cbc->log, "status: %d, response_type: %s \n", (int)status, response_type);
	}
	return MESIBO_RESULT_OK;
}

void chatbot_http_on_close_callback(void *cbdata,  mesibo_int_t result){
	
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
	p.from = b->to;
	p.to = b->from; // User adress who sent the query is the recipient
	p.expiry = 3600;

	char* extracted_response = mesibo_util_json_extract(b->buffer , "fulfillmentText", NULL);

	mesibo_message(mod, &p, extracted_response , strlen(extracted_response));
	mesibo_chatbot_destroy_http_context(b);
}

```

### 6. Compiling the chatbot module
To compile the chatbot module from source run
```
make
```
from the source directory which uses the sample `Makefile` provided to build a shared object `mesibo_mod_chatbot.so`. It places the result at the `TARGET` location `/usr/lib64/mesibo/mesibo_mod_chatbot.so` which you can verify.

### 7. Loading the chatbot module
 
To load the chatbot module provide the configuration in `/etc/mesibo/mesibo.conf`. Refer `sample.conf`.

Mount the directory containing your library which in this case is `/usr/lib64/mesibo/`, while running the mesibo container
as follows. You also need to mount the directory containing the mesibo configuration file which in this case is `/etc/mesibo`

For example, if `mesibo_mod_chatbot.so` is located at `/usr/lib64/mesibo/`
```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
-v /etc/mesibo:/etc/mesibo
-net=host -d  \ 
mesibo/mesibo <app token>

