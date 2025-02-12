#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <AsyncTCP.h>
#include <esp_now.h>
#include <WiFi.h>
#include <WiFiAP.h>
#include <ESPAsyncWebServer.h>

const char* ssid     = "Goodhue 335";
const char* password = "pleaseee";

IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const int lamps = 3;
const int ledPin = 7;

typedef struct brightness {
  int id;
  int value;
} brightness;

brightness send;

esp_now_peer_info_t peerInfo;

uint8_t addrs[][6] = {
  {0x48, 0x31, 0xb7, 0x79, 0xcb, 0xb0},
  {0x48, 0x31, 0xb7, 0x7c, 0x0c, 0x58}
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(ssid, password);
  WiFi.begin();

  Serial.println("WIFI CONFIGURED");

  ws.onEvent(eventHandler);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", generateHomepage());
  });

  server.begin();

  while (esp_now_init() != ESP_OK){
    Serial.println("ESP NOW init failure, retrying");
    delay(500);
  }

  Serial.println("ESP NOW initialized");

  esp_now_register_send_cb(sentCB);

  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  for (int i = 0; i < lamps - 1; i++){
    memcpy(peerInfo.peer_addr, addrs[i], 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
      Serial.print("FAILED TO REGISTER PEER");
    }
  }

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  ws.cleanupClients();
}

void sentCB(const uint8_t *mac_addr, esp_now_send_status_t status){
  char macAddr[18];
  snprintf(macAddr, sizeof(macAddr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macAddr);
  Serial.print(" received message. ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    Serial.println("DATA RECEIVED");
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {

        const uint8_t size = JSON_OBJECT_SIZE(1);
        StaticJsonDocument<size> json;
        DeserializationError err = deserializeJson(json, data);
        if (err) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(err.c_str());
            return;
        }
        
        for (int i = 1; i <= lamps; i++){
          const char* action = json["slider" + String(i)];
          if (action){
            notifyLamp(i, action);
            notifyClients(i, action);
          }
        }  
    }
}

void eventHandler(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void notifyLamp(int ix, const char* strength) {
  Serial.println(ix);
  if (ix == 1){
    analogWrite(ledPin, atoi(strength));
  } else {
    send.id = ix;
    send.value = atoi(strength);
    esp_err_t error = esp_now_send(0, (uint8_t* ) &send, sizeof(brightness));
    if (error == ESP_OK){
      Serial.println("GOOD");
    } else {
      Serial.println("ERROR");
    }
  }
}

void notifyClients(int ix, const char* strength) {
    const uint8_t size = JSON_OBJECT_SIZE(1);
    StaticJsonDocument<size> json;
    json["slider" + String(ix)] = strength;

    char data[18];
    size_t len = serializeJson(json, data);
    Serial.println(data);
    ws.textAll(data, len);
}


String generateHomepage(){
  String h = "<!DOCTYPE html>\n\
    <html>\n\
<head>\n\
  <style>\n\
    .slidecontainer {\n\
      width: 100%;\n\
    }\n\\n\
    .slider {\n\
      -webkit-appearance: none;\n\
      width: 100%;\n\
      height: 25px;\n\
      background: #d3d3d3;\n\
      outline: none;\n\
      opacity: 0.7;\n\
      -webkit-transition: .2s;\n\
      transition: opacity .2s;\n\
    }\n\\n\
    .slider:hover {\n\
      opacity: 1;\n\
    }\n\\n\
    .slider::-webkit-slider-thumb {\n\
      -webkit-appearance: none;\n\
      appearance: none;\n\
      width: 25px;\n\
      height: 25px;\n\
      background: #04AA6D;\n\
      cursor: pointer;\n\
    }\n\\n\
    .slider::-moz-range-thumb {\n\
      width: 25px;\n\
      height: 25px;\n\
      background: #04AA6D;\n\
      cursor: pointer;\n\
    }\n\
  </style>\n\
  <script>\n\
    console.log('asjhdfklasjhdg');\n\
    window.addEventListener('load', function() {\n\
      console.log('on load');\n\
      var websocket = new WebSocket(`ws://${window.location.hostname}/ws`);\n\
      websocket.onopen = function(event) {\n\
        console.log('Connection established');\n\
      }\n\
      websocket.onclose = function(event) {\n\
        console.log('Connection died');\n\
      }\n\
      websocket.onerror = function(error) {\n\
        console.log('error');\n\
      };\n\
      websocket.onmessage = function(event) {\n\
        let data = JSON.parse(event.data);\n\
        console.log(data);\n\
        for (const [key, value] of Object.entries(data)){\n\
          document.getElementById(`${key}`).value = value;\n\
        };\n\
      };\n\  ";

  // add a slider for each light
  for (int i = 1; i <= lamps; i++){
    h += "let slider" + String(i) + " = document.getElementById(\"slider" + String(i) + "\");\n";
    h += "let text" + String(i) + " = document.getElementById(\"demo" + String(i) + "\");\n";
    h += "text" + String(i) + ".innerHTML = slider" + String(i) + ".value;\n";
    h += "slider" + String(i) + ".oninput = function(){\n text" + String(i) + ".innerHTML = this.value;\n websocket.send(JSON.stringify({'slider" + String(i) + "' : this.value}));\n}\n\n";
  }

  h += "\n\
  });\n\
  </script>\n\
</head>\n\
<body>\n\
  <h1>Manage Lights</h1>\n\
  <p>Drag the slider to display the current value.</p>\n\
";

    for (int i = 1; i <= lamps; i++){
      h += "\n\
  <div class=\"slidecontainer\">\n\
    <input type=\"range\" min=\"1\" max=\"255\" value=\"50\" class=\"slider\" id=\"";
      h += "slider" + String(i) + "\">\n\
    <p>Value: <span id=\"";
      h += "demo" + String(i) + "\"></span></p></div>";
    }

    h += "</body></html>";

    return h;
}