# ConnectKit

ConnectKit is the Arduino networking library used by Code Kit IoT blocks. Sketches include one header and use the same HTTP API whether the active transport is WiFi, WIZnet SPI Ethernet, or a user-provided `Client`.

```cpp
#include <ConnectKit.h>

void setup() {
  ConnectKit.connectWiFi("My WiFi", "password");
}

void loop() {
  String response = ConnectKit.get("http://example.com/api");
}
```

## Supported Transports

- ESP8266 WiFi through `ESP8266WiFi.h`
- ESP32 WiFi through `WiFi.h`
- Arduino Uno R4 WiFi through `WiFiS3.h`
- WIZnet W5100, W5200, and W5500 SPI Ethernet hardware through the Arduino Ethernet library
- Any future or custom transport that implements Arduino's `Client` API via `setClient()`

ConnectKit v1 does not support HTTPS, MQTT, ENC28J60 Ethernet, or native ESP32 Ethernet PHYs such as LAN8720.

## Ethernet Hardware Notes

ESP32 and ESP8266 boards normally need W5500 modules wired to SPI rather than Uno-format shields. Those modules must be 3.3 V safe.

Arduino Uno R4 WiFi compatibility depends on SPI/CS wiring and shield voltage behavior. ENC28J60 modules are not supported by this API.

## Limits

ConnectKit stores up to `CONNECTKIT_MAX_HTTP_HEADERS` custom headers and defaults to `CONNECTKIT_DEFAULT_MAX_RESPONSE_SIZE` bytes of response body. Responses larger than the configured limit are truncated and reported with `CONNECTKIT_ERROR_RESPONSE_TOO_LARGE`.
