#  Chatbot Module


This repository contains the source code for Chatbot Module. Chatbot module analyzes messages using various AI and machine learning tools like Tensorflow, Dialogflow, etc. and sends an automatic reply.  

To build chatbots, you can use any chatbot provider of your choice such as Dialogflow, IBM Watson,Rasa,etc and connect with them through REST endpoints. This Chatbot Module provides an example using **Dialogflow**

You can download the source and compile it to obtain the module- a shared library file. Also, you can load the pre-compiled module which is provided as `mesibo_mod_chatbot.so`

Refer to the [Skeleton Module](https://github.com/Nagendra1997/mesibo-documentation/blob/master/skeleton.md) for a basic understanding of how Mesibo Modules work. The complete documentation for Mesibo Modules is available [here](https://mesibo.com/documentation/loadable-modules/)

![Module Chatbot Sample](module_chat_bot.jpg)

## Overview of  Chatbot Module
-  Chatbot configuration containing :project name, session id, Endpoint/base url for making REST call, access token (Service account key  for authenticating your requests) is provided in the module configuration (In the file `mesibo.conf`).
- In module initialisation, all the configuration parameters is obtained from the configuration list and stored in a structure object  `chatbot_config_t`. The authorisation header is constructed and stored in the form 
`" Authorisation : Bearer <access token> "`
- In module initialisation,The callback function for `on_message` is initialized to  `chatbot_on_message`
- When a message is sent from a user to a particular user named `bot_endpoint`, Mesibo invokes the `chatbot_on_message` callback function of the module with the message data and it’s associated message parameters such as sender, expiry, flags, etc.
- Now, the chatbot module makes an HTTP request to a REST endpoint of Dialogflow .The HTTP Post body contains the message text as the `queryInput`. The authorisation header containing the access token is also sent in the POST request.
- Dialogflow sends a response from the chatbot, which is recieved through an http callback function.
- The orginal message is CONSUMED . The fulfillment text from the chatbot is extracted from the JSON response and is sent to the sender of the query in the form of an automatic reply. 


### Basics of Dialogflow
Dialogflow is an AI powered Google Service to build interactive conversational interfaces, such as chatbots. Once you train DialogFlow with data of your interest like emails, FAQs, etc., it can answer questions from your users in natural language.  

#### Dialogflow API (V2)
Dialogflow Service is available through a REST interface.  What this means is,  you simply invoke a URL(called the REST Endpoint) with your query.
Dialogflow will process the query and send the appropriate response .
For more details on using Dialogflow , refer [DialogFlow Documentation](https://cloud.google.com/dialogflow/docs/)

### REST Endpoint
To connect with Dialogflow , invoke the URL with the following format:

```
https://dialogflow.googleapis.com/v2beta1/projects/<Project ID>/agent/sessions/<Session ID>
```
Project ID is the Google Project Name linked with Dialogflow.  You can get the project name from [GCP Console](https://console.cloud.google.com) 
Session ID can be a random number or some type of user and session identifier (preferably hashed). 

For example, a sample Dialogflow REST URL looks like

```
https://dialogflow.googleapis.com/v2beta1/mesibo-dialogflow/agent/sessions/4e72c746-7a38-66b6-8798-1a87b00207a2
```

Now, you send a POST request to the Dialogflow REST URL in the following format.

Pass the authentication information(your service account key) in the request header.

Headers:
```
Authorization: Bearer YOUR_SERVICE_ACCOUNT_KEY
Content-Type: application/json

```
The POST data will contain your text/message.

POST body:

```
{
    "queryInput": {
 	{
	"text": {"text": <Message Text>}
	}   
    }
}
```

#### Getting Access Token
Every request you make to the dialogflow service needs to be authenticated. For this, you just need to provide an access token.
To obtain the access token, follow the steps below:
1. Set up a [GCP Console](https://console.cloud.google.com) project.
    - Create or select a project.
    - Create a service account.
    - Download a private key as JSON.
2. Set the environment variable GOOGLE_APPLICATION_CREDENTIALS to the file path of the JSON file that contains your service account key. This variable only applies to your current shell session, so if you open a new session, set the variable again.
Example: **Linux or macOS**

Replace **[PATH]** with the file path of the JSON file that contains your service account key.

```
export GOOGLE_APPLICATION_CREDENTIALS="[PATH]"
```

For example:

```
export GOOGLE_APPLICATION_CREDENTIALS="/home/user/Downloads/service-account-file.json"
```

3. [Install and initialize the Cloud SDK](https://cloud.google.com/sdk/docs/)
4. Print your access token

```
echo $(gcloud auth application-default print-access-token)
```

which should output something like

```
ya29.c.Kl6iB-r90Gjj4o--m7k7wr4dN4b84U4TLEtPqdEZ2xvfsj01awmUObMDEFwJIJ1lkZPym5dsAw44MbZDSaksLH3xKbsSHGLgWeEXqIPSDmFO6
```

### 1. The C/C++ Source file
The module name is `chatbot`. The C/C++ Source file is `chatbot.c`. The header file `module.h` is included in the C/C++ source as follows:
```cpp

#include "module.h"

```
### 2.Configuring the chatbot module
While loading your module you need to provide your configuration details in the file `/etc/mesibo/mesibo.conf`. The sample configuration is provided in the file `sample.conf`. Copy the configuration from `sample.conf`into `/etc/mesibo/mesibo.conf`

The chatbot module is configured as follows:

```
module chatbot{
project = <Project Name>
session = <Session ID>
endpoint = <Dialogflow REST Endpoint>
access_token = <Service Account key>
chatbot_uid = <Chatbot User ID>
log = <log level>
}
```

For example,
where,
- `project` is the Google Project name that contains your dialogflow chatbot.
- `session` The name of the session this query is sent to.
- `endpoint` The dialogflow REST endpoint to which your query will be sent, Example: `https://dialogflow.googleapis.com/v2beta1`
- `access_token` access token linked with your project. In this example we will be passing dialogflow client access token in the auth header like so `Authorisation: Bearer <access token>.`
- `chatbot_uid` User address. In your Mesibo Application, create a user that you can refer to as a chatbot endpoint user.
- `log` Log level for printing to container logs

For example,
```
module chatbot{
project = mesibo-chatbot
session = 4e72c746-7a38-66b6-xxxxxx
endpoint = https://dialogflow.googleapis.com/v2beta1
access_token = x.Kl6iBzVH7dvV2XywzpnehLJwczdClfMoAHHOeTVNFkmTjqVX7VagKH1
chatbot_uid = my_chatbot
log = 1
}
```

### 3. Initializing the chatbot module
The chatbot module is initialized with the Mesibo Module Configuration details - module version, the name of the module and  references to the module callback functions.
```cpp

	int mesibo_module_chatbot_init(mesibo_module_t *m, mesibo_uint_t len) {

    MESIBO_MODULE_SANITY_CHECK(m, version. len);

    m->flags = 0;
        m->description = strdup("Sample Chatbot Module");
        m->on_message = chatbot_on_message;

       if(m->config) {
                chatbot_config_t* cbc = get_config_dialogflow(m);
                if(cbc  == NULL){
                        mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
                        return MESIBO_RESULT_FAIL;
                }
                m->ctx = (void* )cbc;

                int init_status = chatbot_init_dialogflow(m);
                if(init_status != MESIBO_RESULT_OK){
                        mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Bad Configuration\n", m->name);
                        return MESIBO_RESULT_FAIL;
                }
        }

        else {
                mesibo_log(m, MODULE_LOG_LEVEL_0VERRIDE, "%s : Missing Configuration\n", m->name);
                return MESIBO_RESULT_FAIL;
        }

	return MESIBO_RESULT_OK;
```
The configuration for the chatbot is provided in `mesibo.conf`
After reading the configuration, the configuration is stored in the module context `m->ctx`

### Storing the configuration in module context

The chatbot configuration parameters is extracted from module configuration list and stored in the configuration context structure `chatbot_config_t` which is defined as follows:

```cpp
typedef struct chatbot_config_s {
        /* To be configured in module configuration file */
        const char* project;
        const char* session;
        const char* endpoint;
        const char* access_token;
        const char* chatbot_uid;
        int log;

        /* To be configured by Dialogflow init function */
        char* post_url;
        char* auth_bearer;
        module_http_option_t* chatbot_http_opt;

} chatbot_config_t;
```

To get the configuaration details , the config helper function `get_config_dialogflow` is called.

`get_config_dialogflow` fetches all the configuration details as per format and stores into the appropriate members of the structure `chatbot_config_t`

```cpp
static chatbot_config_t*  get_config_dialogflow(mesibo_module_t* mod){
        chatbot_config_t* cbc = (chatbot_config_t*)calloc(1, sizeof(chatbot_config_t));
        cbc->project = mesibo_util_getconfig(mod, "project");
        cbc->session = mesibo_util_getconfig(mod, "session");
        cbc->endpoint = mesibo_util_getconfig(mod, "endpoint");
        cbc->access_token = mesibo_util_getconfig(mod, "access_token");
        cbc->chatbot_uid = mesibo_util_getconfig(mod, "chatbot_uid");
        cbc->log = atoi(mesibo_util_getconfig(mod, "log"));

        return cbc;
}
```

### Initialising the request parmaters
Once we get the configuration details, we can construct the request parameters such as the post url (endpoint) and the authorisation header. To do this we call the `chatbot_init_dialogflow` function as follows

```cpp
static int chatbot_init_dialogflow(mesibo_module_t* mod){
        chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;
        mesibo_log(mod, cbc->log, "chatbot_init_dialogflow called \n");

        int cv;
        cv = asprintf(&cbc->post_url,"%s/projects/%s/agent/sessions/%s:detectIntent",
                        cbc->endpoint, cbc->project, cbc->session);
        if(cv == -1)return MESIBO_RESULT_FAIL;
        mesibo_log(mod, cbc->log, "Configured post url for HTTP requests: %s \n", cbc->post_url);

        cv = asprintf(&cbc->auth_bearer,"Authorization: Bearer %s", cbc->access_token);
        if(cv == -1)return MESIBO_RESULT_FAIL;
        mesibo_log(mod, cbc->log, "Configured auth bearer for HTTP requests with token: %s \n", cbc->auth_bearer );

        cbc->chatbot_http_opt = mesibo_chatbot_get_http_opt(cbc);

        return MESIBO_RESULT_OK;
}
```

### 3.`chatbot_on_message`

We only need the message containing queries addressed to our chatbot. This user name is configured with `chatbot_uid` . `chatbot_on_message` will alert you for any message sent. So, we filter these messages to get only those that are outgoing TO the chatbot and send it for processing. For all other messages, we PASS as it is.

```cpp
static mesibo_int_t chatbot_on_message(mesibo_module_t *mod, mesibo_message_params_t *p,
                const char *message, mesibo_uint_t len) {

        chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

        if(strcmp(p->to, cbc->chatbot_uid) == 0){
                // Don't modify original as other module will use it
                mesibo_message_params_t* np = (mesibo_message_params_t*)calloc(1, sizeof(mesibo_message_params_t));
                memcpy(np, p, sizeof(mesibo_message_params_t));
                chatbot_process_message(mod, np, message, len);

                return MESIBO_RESULT_CONSUMED;  // Process the message and CONSUME original
        }

        return MESIBO_RESULT_PASS;
}
```
### 4. Processing the query 
To process the incoming query we need to send it to DialogFlow and send the response back to the user.

Following the Dialogflow POST request format, we send an HTTP request using the helper function `mesibo_http`.
DialogFlow expects the request data in the JSON format. Ideally, we could have used a JSON library to encode the request.
However, JSON libraries are typically slow and is an overkill for this simple project. Hence, we directly construct the raw post data string.

Once the response is received from DialogFlow, we need to send it to to the user who made the request. Hence, we store the context of the received message ie; message parameters, the sender of the message, the receiver of the message in thefollowing structure and pass it as callback data in http request.

```cpp
typedef struct message_context_s {
        mesibo_module_t *mod;
        mesibo_message_params_t *params;
        char *from;
        char *to;
        // To copy data in response
        char buffer[HTTP_BUFFER_LEN];
        int datalen;
     	
char* post_data; //For cleanup after HTTP request is complete
} message_context_t;
```                     
```cpp
static int chatbot_process_message(mesibo_module_t *mod, mesibo_message_params_t *p,
                const char *message, mesibo_uint_t len) {

        chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

        const char* post_url = cbc->post_url;
        char* raw_post_data;
        asprintf(&raw_post_data, "{\"queryInput\":{\"text\":{\"text\":\" %.*s \"}}}",
                        (int)len, message);

        message_context_t *message_context =
                (message_context_t *)calloc(1, sizeof(message_context_t));
        message_context->mod = mod;
        message_context->params = p;
        message_context->from = strdup(p->from);
        message_context->to = strdup(p->to);

        mesibo_http(mod, post_url, raw_post_data, chatbot_http_callback,
                        (void *)message_context, cbc->chatbot_http_opt);

        return MESIBO_RESULT_OK;
}
```

### 5. Defining the Callback function to receive the response from your bot
In the callback function, we will be getting the response in asynchronous blocks. So,we copy the response data into a temporary buffer and once the complete response is received(indicated by progress=100) we send the response back to the user who sent the query.

```cpp
static int chatbot_http_callback(void *cbdata, mesibo_int_t state,
                mesibo_int_t progress, const char *buffer,
                mesibo_int_t size) {
        message_context_t *b = (message_context_t *)cbdata;
        mesibo_module_t *mod = b->mod;
        chatbot_config_t* cbc = (chatbot_config_t*)mod->ctx;

        mesibo_message_params_t *params = b->params;

        if (progress < 0) {
                mesibo_chatbot_destroy_message_context(b);
                return MESIBO_RESULT_FAIL;
        }

        if (state != MODULE_HTTP_STATE_RESPBODY) {
                return MESIBO_RESULT_OK;
        }

        if ((progress > 0) && (state == MODULE_HTTP_STATE_RESPBODY)) {
                memcpy(b->buffer + b->datalen, buffer, size);
                b->datalen += size;
        }

	if (progress == 100) {

                char* extracted_response = mesibo_util_json_extract(b->buffer , "fulfillmentText", NULL);

		mesibo_message_params_t p;
                memset(&p, 0, sizeof(mesibo_message_params_t));
                p.id = rand();
                p.refid = params->id;
                p.aid = params->aid;
                p.from = params->to;
                p.to = params->from; // User adress who sent the query is the recipient
                p.expiry = 3600;

		mesibo_message(mod, &p, extracted_response , strlen(extracted_response));

                mesibo_chatbot_destroy_message_context(b);
        }

        return MESIBO_RESULT_OK;
}
```

### 6. Compiling the chatbot module
To compile the chatbot module from source run
```
make
```
from the source directory which uses the sample `Makefile` provided to build a shared object `mesibo_mod_chatbot.so`. It places the result at the `TARGET` location `/usr/lib64/mesibo/mesibo_mod_chatbot.so` which you can verify.

 ### 7. Loading your module
 You can load the pre-compiled module (.so) by specifying the matching configuration in `mesibo.conf` as provided in `chatbot.conf` and mount the directory which contains the module shared library while running the mesibo container.

For example, if `mesibo_mod_chatbot.so` is located at `/usr/lib64/mesibo/`
```
sudo docker run  -v /certs:/certs -v  /usr/lib64/mesibo/:/usr/lib64/mesibo/ \
-v /etc/mesibo:/etc/mesibo
-net=host -d  \ 
mesibo/mesibo <app token>
