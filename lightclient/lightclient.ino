#include <esp_now.h>
#include <WiFi.h>

typedef struct brightness {
  int id;
  int value;
} brightness;

int id = 3;

brightness send;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  while (esp_now_init() != ESP_OK){
    Serial.println(("ESP NOW error, retrying"));
    delay(500);
  }

  Serial.println("ESP NOW initialized");

  esp_now_register_recv_cb(esp_now_recv_cb_t(onRcv));
}

void loop() {
  // put your main code here, to run repeatedly:

}

void onRcv(const uint8_t * mac, const uint8_t *incomingdata, int len){
  memcpy(&send, incomingdata, sizeof(brightness));
  if (send.id == id){
    Serial.println("UPDATING BRIGHTNESS");
    analogWrite(7, send.value);
  } else {
    Serial.println("id not equal, ignoring");
  }
}