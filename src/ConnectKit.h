#ifndef CONNECTKIT_H
#define CONNECTKIT_H

#include <Arduino.h>
#include <Client.h>

#define CONNECTKIT_OK 0

#define CONNECTKIT_ERROR_WIFI_EMPTY_SSID -10
#define CONNECTKIT_ERROR_WIFI_TIMEOUT -11
#define CONNECTKIT_ERROR_WIFI_UNSUPPORTED -12

#define CONNECTKIT_ERROR_ETHERNET_INVALID_MAC -20
#define CONNECTKIT_ERROR_ETHERNET_DHCP_FAILED -21
#define CONNECTKIT_ERROR_ETHERNET_UNSUPPORTED -22
#define CONNECTKIT_ERROR_ETHERNET_NO_HARDWARE -23

#define CONNECTKIT_ERROR_NO_ACTIVE_CONNECTION -30
#define CONNECTKIT_ERROR_HTTPS_UNSUPPORTED -31
#define CONNECTKIT_ERROR_HTTP_MISSING_HOST -32
#define CONNECTKIT_ERROR_HTTP_CONNECT_FAILED -33
#define CONNECTKIT_ERROR_HTTP_INVALID_RESPONSE -34
#define CONNECTKIT_ERROR_HTTP_TIMEOUT -35
#define CONNECTKIT_ERROR_HTTP_CLIENT -36
#define CONNECTKIT_ERROR_TOO_MANY_HEADERS -38
#define CONNECTKIT_ERROR_RESPONSE_TOO_LARGE -39

#define CONNECTKIT_MAX_HTTP_HEADERS 8
#define CONNECTKIT_DEFAULT_MAX_RESPONSE_SIZE 1024
#define CONNECTKIT_DEFAULT_HTTP_TIMEOUT 30000UL

class ConnectKitClass {
public:
  ConnectKitClass();

  bool connectWiFi(const String& ssid, const String& password, unsigned long timeoutMs = 20000UL);
  bool connectEthernet(const String& mac, int csPin = 10, unsigned long timeoutMs = 20000UL);
  void setClient(Client& client, const String& localIp = "");
  void disconnect();

  bool connected();
  String localIP();

  bool setHeader(const String& name, const String& value);
  void clearHeaders();

  String request(const String& method, const String& url, const String& body = "", const String& contentType = "");
  String get(const String& url);
  String post(const String& url, const String& body, const String& contentType = "application/json");

  int lastStatusCode();
  int lastErrorCode();
  String lastError();
  bool lastRequestSucceeded();
  String lastResponse();

  String urlEncode(const String& text);
  String urlWithQuery(const String& url, const String& name, const String& value);

  void setHttpTimeout(unsigned long timeoutMs);
  void setMaxResponseSize(size_t bytes);

  int ethernetHardwareStatus();

private:
  struct Header {
    String name;
    String value;
  };

  Client* _activeClient;
  bool _connected;
  String _localIp;
  int _lastStatusCode;
  int _lastErrorCode;
  String _lastError;
  String _lastResponse;
  Header _headers[CONNECTKIT_MAX_HTTP_HEADERS];
  uint8_t _headerCount;
  unsigned long _httpTimeoutMs;
  size_t _maxResponseSize;

  void activateClient(Client& client, const String& localIp);
  void resetRequestState();
  void clearErrorState();
  void setError(int code, bool updateStatusCode = true);
};

extern ConnectKitClass ConnectKit;

#endif
