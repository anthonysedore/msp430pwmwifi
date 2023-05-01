#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char* ssid = "testtest";
const char* password = "test1234";

const int output = 2;
String sliderValue = "0";

const char* PARAM_INPUT = "value";
const char* PARAM_INPUT_1 = "output";
const char* PARAM_INPUT_2 = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Web Server</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    h3 {font-size: 1.4rem; color: blue;}
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider2 { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider2::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider2::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; }
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none} 
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 6px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    .slider3 {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #F5F6CE; border-radius: 6px}
    .slider3:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #6b5; -webkit-transition: .4s; transition: .4s; border-radius: 3px}
    input:checked+.slider {background-color: #b30000}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
    input:checked+.slider3 {background-color: #0BF933}
    input:checked+.slider3:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}

  </style>
</head>
<body>
  <h2>ESP32 Web Server</h2>
  <h3>Made by Anthony Sedore</h3>
  <p><span id="textSliderValue">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="100" value="%SLIDERVALUE%" step="1" class="slider2"></p>
  %BUTTONPLACEHOLDER%
<script>
function updateSliderPWM(element) {
  var sliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/slider?value="+sliderValue, true);
  xhr.send();
}

function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ xhr.open("GET", "/update?output="+element.id+"&state=1", true); }
  else { xhr.open("GET", "/update?output="+element.id+"&state=0", true); }
  xhr.send();
}

</script>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var) {
  //Serial.println(var);
  if (var == "SLIDERVALUE") {
    return sliderValue;
  }

  if (var == "BUTTONPLACEHOLDER") {
    String buttons = "";
    buttons += "<h4>Forward | Backward</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"2\" " + outputState(2) + "><span class=\"slider3\"></span></label>";
    buttons += "<h4>OFF | ON</h4><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"4\" " + outputState(4) + "><span class=\"slider\"></span></label>";
    return buttons;
  }

  return String();
}

String outputState(int output) {
  if (digitalRead(output)) {
    return "checked";
  } else {
    return "";
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  pinMode(2,OUTPUT);
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  digitalWrite(2, HIGH);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/slider?value=<inputMessage>
  server.on("/slider", HTTP_GET, [](AsyncWebServerRequest* request) {
    String inputMessage;
    // GET input1 value on <ESP_IP>/slider?value=<inputMessage>
    if (request->hasParam(PARAM_INPUT)) {
      inputMessage = request->getParam(PARAM_INPUT)->value();
      sliderValue = inputMessage;
    } else {
      inputMessage = "No message sent";
    }
    Serial.println("PWM");
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2)) {
      inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
      //digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    } else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }

    if (inputMessage1 == "2" && inputMessage2 == "0") {
      Serial.println("FORWARD");
    } else if (inputMessage1 == "2" && inputMessage2 == "1") {
      Serial.println("BACKWARD");
    }

    if (inputMessage1 == "4" && inputMessage2 == "0") {
      Serial.println("OFF");
    } else if (inputMessage1 == "4" && inputMessage2 == "1") {
      Serial.println("ON");
    }

    // Serial.print("GPIO: ");
    // Serial.print(inputMessage1);
    // Serial.print(" - Set to: ");
    // Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  // Start server
  server.begin();
}

void loop() {
}