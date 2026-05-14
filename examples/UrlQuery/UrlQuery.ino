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

  String url = "http://example.com/search";
  url = ConnectKit.urlWithQuery(url, "sensor name", "room 1");
  url = ConnectKit.urlWithQuery(url, "unit", "deg C");

  String response = ConnectKit.get(url);
  Serial.println(ConnectKit.lastStatusCode());
  Serial.println(response);
}

void loop() {
}
