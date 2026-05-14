#include <ConnectKit.h>

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  if (!ConnectKit.connectWiFi(ssid, password, 20000UL)) {
    Serial.println(ConnectKit.lastError());
    return;
  }

  String response = ConnectKit.get("http://example.com/");
  Serial.println(ConnectKit.lastStatusCode());
  Serial.println(response);
}

void loop() {
}
