#include <ConnectKit.h>

const char* mac = "DE:AD:BE:EF:FE:ED";
const int csPin = 10;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  if (!ConnectKit.connectEthernet(mac, csPin, 20000UL)) {
    Serial.println(ConnectKit.lastError());
    return;
  }

  String body = "{\"message\":\"hello\"}";
  String response = ConnectKit.post("http://example.com/api", body);
  Serial.println(ConnectKit.lastStatusCode());
  Serial.println(response);
}

void loop() {
}
