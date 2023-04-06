//document.getElementById("datetime").innerHTML = "WebSocket is not connected";

var websocket = new WebSocket('ws://'+location.hostname+'/');
var vrx = 0.0;
var vry = 0.0;
var deg2rad = 0.0174533;

function sendText(name) {
	console.log('sendText');
	var data = {};
	data["id"] = name;
	console.log('data=', data);
	json_data = JSON.stringify(data);
	console.log('json_data=' + json_data);
	websocket.send(json_data);
}

websocket.onopen = function(evt) {
	console.log('WebSocket connection opened');
	var data = {};
	data["id"] = "init";
	console.log('data=', data);
	json_data = JSON.stringify(data);
	console.log('json_data=' + json_data);
	websocket.send(json_data);
	//document.getElementById("datetime").innerHTML = "WebSocket is connected!";
}

websocket.onmessage = function(evt) {
	var msg = evt.data;
	console.log("msg=" + msg);
	var values = msg.split('\4'); // \4 is EOT
	//console.log("values=" + values);
	switch(values[0]) {
		case 'HEAD':
			console.log("HEAD values[1]=" + values[1]);
			var h1 = document.getElementById( 'header' );
			h1.textContent = values[1];
			break;

		case 'METER':
			//console.log("gauge1=" + Object.keys(gauge1.options));
			//console.log("gauge1.options.units=" + gauge1.options.units);
			console.log("METER values[1]=" + values[1]);
			console.log("METER values[2]=" + values[2]);
			gauge1.options.units = values[1];
			document.getElementById("canvas1").style.display = "inline-block";
			gauge2.options.units = values[2];
			document.getElementById("canvas2").style.display = "inline-block";
			break;

		case 'DATA':
			console.log("DATA values[1]=" + values[1]);
			var val1 = parseInt(values[1], 10);
			console.log("DATA values[2]=" + values[2]);
			var val2 = parseInt(values[2], 10);
			console.log("DATA values[3]=" + values[3]);
			var val3 = parseInt(values[3], 10);
			if (val3 == 1) {
				vrx = 0.0;
				vry = 0.0;
			} else {
				vrx = vrx + val1/100.0;
				vry = vry + val2/100.0;
				if (vrx > 10.0) vrx = 10.0;
				if (vry > 10.0) vry = 10.0;
				if (vrx < -10.0) vrx = -10.0;
				if (vry < -10.0) vry = -10.0;
			}
			Plotly.update('myDiv', { x: [[vrx]], y: [[vry]] })

			gauge1.value = vrx;
			gauge1.update({ valueText: vrx.toFixed(2) });
			gauge2.value = vry;
			gauge2.update({ valueText: vry.toFixed(2) });
			break;

		default:
			break;
	}
}

websocket.onclose = function(evt) {
	console.log('Websocket connection closed');
	//document.getElementById("datetime").innerHTML = "WebSocket closed";
}

websocket.onerror = function(evt) {
	console.log('Websocket error: ' + evt);
	//document.getElementById("datetime").innerHTML = "WebSocket error????!!!1!!";
}
