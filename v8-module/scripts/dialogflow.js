//dialogflow.js

initialize();

function initialize(){
        mesibo.onmessage = Mesibo_onMessage;
        mesibo.onmessagestatus = Mesibo_onMessageStatus;
}

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

function Mesibo_onMessageStatus(message) {
        return mesibo.RESULT_OK;
}

