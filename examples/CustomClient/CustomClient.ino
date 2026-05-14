#include <ConnectKit.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetClient client;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  Ethernet.init(10);
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Ethernet DHCP connection failed");
    return;
  }

  ConnectKit.setClient(client);
  String response = ConnectKit.get("http://example.com/");
  Serial.println(ConnectKit.lastStatusCode());
  Serial.println(response);
}

void loop() {
}
