## Mesibo Scripting (V8 Module) 

Mesibo is built by developers for developers. We believe in making our platform open and developer friendly to help you realize your next big idea. We have now made building with Mesibo more easier. We present to you Mesibo Scripting - An easy to use interface that allows you to use the core platform of Mesibo with the wildly popular Javscript.

You can write custom logic in Javascript and let Mesibo Scripting take care of converting that logic into high performance native code. This creates unlimited creative possibilities for building with Mesibo. You can build chatbots, filters, translators, implement socket and database logic all from the comfort and ease of Javascript. 

Mesibo scripting enables you to achieve a tighter integration with your application and greater control over the core platform of Mesibo.

The scripting interface is provided through the Mesibo Cloud console and you need not configure or install anything additional to use it. Just login to your application console , load your script and get going! 

You can also use the scripting module on your own premise/servers if you are [hosting Mesibo yourself](https://mesibo.com/documentation/on-premise/).You can load the scripting module and configure it to run your script.

### How to use Mesibo Scripting
You can specify your customised logic for each event such as when you receive a message, when a user logs in,etc and also use core utility functions provided by mesibo to perform different actions like sending a message, making an HTTP request, connecting to a socket, writing to a database, etc.
 
From Mesibo Console , you can configure to run a custom script for each user,etc. 

If you are using Mesibo On-Premise you can use the Scripting Module and configure the same.The module interface is provided in native C/C++ platform which ensures raw performance and stability.

### How Mesibo Scripting Works
The script you configure is executed at runtime through the Mesibo Native Interface. This is done by using a Javascript Engine which compiles Javscript code into Native C/C++ code and then evaluating that script on each call.
To do this, Mesibo uses V8 - Google’s open source high-performance JavaScript and WebAssembly engine, written in C++.

### Build Instructions
The V8 static library along with the associated header files are bundled with the Mesibo Container. The current version of the V8 library used and tested with is Version 7.9. If you would like to build the module with a different version of V8, generate the staticlibrary by checking out the V8 source code and then link with it. Make sure that you mount the path containing the static library and header files.


For more information refer [V8 Docs](https://v8.dev/docs/embed)

A sample Makefile is provided which can be used to build the module and link with the V8 static library.
```
MODULE=v8
V8_PATH=/usr/lib64/mesibo/v8-lib
V8_LIBPATH=$(V8_PATH)/obj
EXTRA_CCFLAGS= -I$(V8_PATH) -I$(V8_PATH)/include -Iinclude 
EXTRA_LDFLAGS=-Wl,-whole-archive -lv8_monolith -L$(V8_LIBPATH) -Wl,--no-whole-archive -lpthread
-include ../make.inc/make.inc

```

### Examples for using Mesibo Scripting

Here is a glimpse of what you can do with Mesibo Scripting. This code snippet sends a custom reply to any message recieved. 

### Sending an automatic reply
```javascript
Mesibo_onMessage(p, data, len){
	var tmp = message.to;
	message.id = parseInt(Math.floor(2147483647*Math.random()));
	message.to = message.from;
	message.from = tmp;
	message.message = "This is an automated response";
	message.send();
}
```
### Making an HTTP Request
Lets do something way more cooler. This message can be a query to your chatbot. You can even connect with a chatbot service of your choice. You can make a REST call your Chatbot API endpoint, get the response and send it back as a reply.

```javascript
function Mesibo_onHttpResponse(http) {
        var rp = JSON.parse(http.responseJSON);
        var fulfillment = rp.queryResult.fulfillmentText;

        var query = http.cbdata;
        var chatbot_response = query;
        chatbotResponse.from = query.to;
        chatbotResponse.to = query.from;
        chatbotResponse.message = fulfillment;
        chatbotResponse.refid = query.id;
        chatbotResponse.id = parseInt(Math.floor(2147483647*Math.random()));

        chatbotResponse.send();

        return MESIBO_RESULT_OK;
}

function Mesibo_onMessage(message) {
        var dHttp = new Http();
        dHttp.url = "https://dialogflow.googleapis.com/v2/projects/mesibo-dialogflow/agent/sessions/"
                +message.id+ ":detectIntent";
        var request_body = {
                "queryInput": {
                        "text": {
                                "text": message.message",
                                "languageCode" : "en"
                        }
                }
        };
        dHttp.post = JSON.stringify(request_body);
        dHttp.ondata = Mesibo_onHttpResponse;
        dHttp.cbdata = message;
                dHttp.extraHeaders = "Authorization: Bearer ya29.c.Kl67B_MR8M_Odn17CquRQ_ALxfNACBs8o97wL0NmwUZSOXjVeoJkWq8KphTk80yKlnT1bVUZAn61zkym660E2DkdxOz6w1cMO5BRPhHV_0ij378huWJaIi_bdcvVN4vT";
        dHttp.contentType = 'application/json';
        dHttp.send();

        return mesibo.RESULT_OK;
}

```
### Connecting to a socket

To connect to a socket you need to provide the host and port number, and a set of callback functions
`on_connect` and `on_data`. Once you connect to the socket sucessfully you will be notified by the `on_connect` callback and you can write to the socket. Any response data you receieve, you will be notified through the `on_data` callback. 

```javascript
var s = new Socket("ws://echo.websocket.org");
s.onconnect = function(s) {
	print("connected");
	s.send("ECHO");
	return;
}
s.ondata = function(s) {
	print("data: " + s.message);
}
s.open();
```

