#include <ConnectKit.h>

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  String response = ConnectKit.get("https://example.com/");

  if (!ConnectKit.lastRequestSucceeded()) {
    Serial.print("Error ");
    Serial.print(ConnectKit.lastErrorCode());
    Serial.print(": ");
    Serial.println(ConnectKit.lastError());
    return;
  }

  Serial.println(response);
}

void loop() {
}
