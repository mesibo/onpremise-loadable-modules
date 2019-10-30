//mesibo_test.js

const MESIBO_RESULT_OK        	= 0
const MESIBO_RESULT_FAIL      	= -1

const MESIBO_RESULT_CONSUMED	= 1 ; 
const MESIBO_RESULT_PASS	= 0 ;

// Example http response callback
function Mesibo_onHttpResponse(mod, cbdata, response, datalen) {
	var rp_log = " ----- Recieved http response ----";
	mesibo_log(mod, rp_log, rp_log.length);
	mesibo_log(mod, response, datalen);

	p = JSON.parse(cbdata);
	mesibo_log(mod, rp_log, rp_log.length);

	return MESIBO_RESULT_OK;
}

function Mesibo_onMessage(mod, p, message, len, temp) {
	var msg_log = "Recieved Message" + message + +"from " +p.from +"to " +p.to;
	mesibo_log(mod, msg_log, msg_log.length + len);
	var params = Object.assign({},p);
	params.to = p.from;
	params.from = p.to;
	params.expiry = 3600;
	params.id = parseInt(Math.floor(2147483647*Math.random())) 
	
	var test_message = "Hello from Mesibo Module";
	
	mesibo_message(mod, params, test_message, test_message.length);
	
	return MESIBO_RESULT_PASS; //PASS
}

function Mesibo_onMessageStatus(mod, p, pStatus) {
	var msg_status_log = "Sent message from "+ p.from + "with status"+ pStatus;
	mesibo_log(mod, msg_status_log, msg_status_log.length);
	return MESIBO_RESULT_OK; 
}
