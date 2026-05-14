#include "ConnectKit.h"

#include <ArduinoHttpClient.h>
#include <ctype.h>
#include <stdlib.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#define CONNECTKIT_HAS_WIFI 1
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#define CONNECTKIT_HAS_WIFI 1
#elif defined(ARDUINO_UNOR4_WIFI)
#include <WiFiS3.h>
#define CONNECTKIT_HAS_WIFI 1
#else
#define CONNECTKIT_HAS_WIFI 0
#endif

#if defined(ARDUINO_ARCH_ESP8266) && defined(MAX_SOCK_NUM)
#undef MAX_SOCK_NUM
#endif

#include <Ethernet.h>

namespace {

const char* errorMessage(int code) {
  switch (code) {
    case CONNECTKIT_ERROR_WIFI_EMPTY_SSID:
      return "WiFi SSID is empty";
    case CONNECTKIT_ERROR_WIFI_TIMEOUT:
      return "WiFi connection timed out";
    case CONNECTKIT_ERROR_WIFI_UNSUPPORTED:
      return "WiFi is not supported on this board";
    case CONNECTKIT_ERROR_ETHERNET_INVALID_MAC:
      return "Ethernet MAC address is invalid";
    case CONNECTKIT_ERROR_ETHERNET_DHCP_FAILED:
      return "Ethernet DHCP connection failed";
    case CONNECTKIT_ERROR_ETHERNET_UNSUPPORTED:
      return "Ethernet is not supported on this board";
    case CONNECTKIT_ERROR_ETHERNET_NO_HARDWARE:
      return "Ethernet hardware was not found";
    case CONNECTKIT_ERROR_NO_ACTIVE_CONNECTION:
      return "No active network connection";
    case CONNECTKIT_ERROR_HTTPS_UNSUPPORTED:
      return "HTTPS URLs are not supported";
    case CONNECTKIT_ERROR_HTTP_MISSING_HOST:
      return "HTTP URL is missing a host";
    case CONNECTKIT_ERROR_HTTP_CONNECT_FAILED:
      return "Could not connect to HTTP host";
    case CONNECTKIT_ERROR_HTTP_INVALID_RESPONSE:
      return "HTTP response was invalid";
    case CONNECTKIT_ERROR_HTTP_TIMEOUT:
      return "HTTP request timed out";
    case CONNECTKIT_ERROR_HTTP_CLIENT:
      return "HTTP client error";
    case CONNECTKIT_ERROR_TOO_MANY_HEADERS:
      return "Too many HTTP headers";
    case CONNECTKIT_ERROR_RESPONSE_TOO_LARGE:
      return "HTTP response was too large";
    default:
      return "";
  }
}

bool isHexByteChar(char c) {
  return isxdigit((unsigned char)c) != 0;
}

int hexValue(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

bool parseMacAddress(const String& mac, byte out[6]) {
  String value = mac;
  value.trim();

  if (value.length() != 17) {
    return false;
  }

  char separator = '\0';
  int index = 0;

  for (int group = 0; group < 6; group++) {
    if (!isHexByteChar(value[index]) || !isHexByteChar(value[index + 1])) {
      return false;
    }

    out[group] = (byte)((hexValue(value[index]) << 4) | hexValue(value[index + 1]));
    index += 2;

    if (group == 5) {
      return index == (int)value.length();
    }

    char currentSeparator = value[index];
    if (currentSeparator != ':' && currentSeparator != '-') {
      return false;
    }

    if (separator == '\0') {
      separator = currentSeparator;
    } else if (separator != currentSeparator) {
      return false;
    }

    index++;
  }

  return false;
}

struct ParsedHttpUrl {
  String host;
  uint16_t port;
  String path;
};

int firstAuthorityTerminator(const String& url, int start) {
  int slash = url.indexOf('/', start);
  int query = url.indexOf('?', start);

  if (slash < 0) {
    return query < 0 ? url.length() : query;
  }

  if (query < 0) {
    return slash;
  }

  return slash < query ? slash : query;
}

bool containsSpace(const String& text) {
  for (unsigned int i = 0; i < text.length(); i++) {
    if (isspace((unsigned char)text[i])) {
      return true;
    }
  }
  return false;
}

bool parsePort(const String& text, uint16_t& port) {
  if (text.length() == 0) {
    return false;
  }

  unsigned long value = 0;
  for (unsigned int i = 0; i < text.length(); i++) {
    if (!isdigit((unsigned char)text[i])) {
      return false;
    }
    value = (value * 10UL) + (unsigned long)(text[i] - '0');
    if (value > 65535UL) {
      return false;
    }
  }

  if (value == 0) {
    return false;
  }

  port = (uint16_t)value;
  return true;
}

bool parseHttpUrl(const String& url, ParsedHttpUrl& parsed, int& errorCode) {
  String value = url;
  value.trim();

  String lower = value;
  lower.toLowerCase();

  if (lower.startsWith("https://")) {
    errorCode = CONNECTKIT_ERROR_HTTPS_UNSUPPORTED;
    return false;
  }

  if (!lower.startsWith("http://")) {
    errorCode = CONNECTKIT_ERROR_HTTP_MISSING_HOST;
    return false;
  }

  const int authorityStart = 7;
  int authorityEnd = firstAuthorityTerminator(value, authorityStart);
  String authority = value.substring(authorityStart, authorityEnd);
  authority.trim();

  if (authority.length() == 0 || containsSpace(authority)) {
    errorCode = CONNECTKIT_ERROR_HTTP_MISSING_HOST;
    return false;
  }

  parsed.port = 80;
  int colon = authority.lastIndexOf(':');
  if (colon >= 0) {
    parsed.host = authority.substring(0, colon);
    String portText = authority.substring(colon + 1);
    parsed.host.trim();
    portText.trim();

    if (parsed.host.length() == 0 || !parsePort(portText, parsed.port)) {
      errorCode = CONNECTKIT_ERROR_HTTP_MISSING_HOST;
      return false;
    }
  } else {
    parsed.host = authority;
  }

  if (parsed.host.length() == 0) {
    errorCode = CONNECTKIT_ERROR_HTTP_MISSING_HOST;
    return false;
  }

  if (authorityEnd >= (int)value.length()) {
    parsed.path = "/";
  } else if (value[authorityEnd] == '?') {
    parsed.path = "/";
    parsed.path += value.substring(authorityEnd);
  } else {
    parsed.path = value.substring(authorityEnd);
  }

  if (parsed.path.length() == 0) {
    parsed.path = "/";
  }

  return true;
}

bool isSupportedMethod(const String& method) {
  return method == "GET" || method == "POST" || method == "PUT" || method == "PATCH" || method == "DELETE";
}

bool methodSendsBody(const String& method) {
  return method == "POST" || method == "PUT" || method == "PATCH";
}

int mapHttpClientError(int code) {
  switch (code) {
    case HTTP_ERROR_CONNECTION_FAILED:
      return CONNECTKIT_ERROR_HTTP_CONNECT_FAILED;
    case HTTP_ERROR_TIMED_OUT:
      return CONNECTKIT_ERROR_HTTP_TIMEOUT;
    case HTTP_ERROR_INVALID_RESPONSE:
      return CONNECTKIT_ERROR_HTTP_INVALID_RESPONSE;
    default:
      return CONNECTKIT_ERROR_HTTP_CLIENT;
  }
}

bool waitForHttpData(HttpClient& client, unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!client.available() && client.connected()) {
    if ((millis() - start) >= timeoutMs) {
      return false;
    }
    delay(1);
  }
  return client.available() > 0;
}

String ipAddressToString(const IPAddress& ip) {
  String value = String(ip[0]);
  value += ".";
  value += String(ip[1]);
  value += ".";
  value += String(ip[2]);
  value += ".";
  value += String(ip[3]);
  return value;
}

String hostHeaderValue(const ParsedHttpUrl& parsed) {
  String value = parsed.host;
  if (parsed.port != 80) {
    value += ":";
    value += String(parsed.port);
  }
  return value;
}

#if CONNECTKIT_HAS_WIFI
WiFiClient connectKitWiFiClient;
#endif

EthernetClient connectKitEthernetClient;

}  // namespace

ConnectKitClass ConnectKit;

ConnectKitClass::ConnectKitClass()
  : _activeClient(NULL),
    _connected(false),
    _localIp(""),
    _lastStatusCode(CONNECTKIT_OK),
    _lastErrorCode(CONNECTKIT_OK),
    _lastError(""),
    _lastResponse(""),
    _headerCount(0),
    _httpTimeoutMs(CONNECTKIT_DEFAULT_HTTP_TIMEOUT),
    _maxResponseSize(CONNECTKIT_DEFAULT_MAX_RESPONSE_SIZE) {
}

bool ConnectKitClass::connectWiFi(const String& ssid, const String& password, unsigned long timeoutMs) {
  String trimmedSsid = ssid;
  trimmedSsid.trim();

  if (trimmedSsid.length() == 0) {
    setError(CONNECTKIT_ERROR_WIFI_EMPTY_SSID);
    return false;
  }

#if CONNECTKIT_HAS_WIFI
  if (password.length() > 0) {
    WiFi.begin(trimmedSsid.c_str(), password.c_str());
  } else {
    WiFi.begin(trimmedSsid.c_str());
  }

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(100);
  }

  if (WiFi.status() != WL_CONNECTED) {
    setError(CONNECTKIT_ERROR_WIFI_TIMEOUT);
    return false;
  }

  activateClient(connectKitWiFiClient, ipAddressToString(WiFi.localIP()));
  return true;
#else
  (void)password;
  (void)timeoutMs;
  setError(CONNECTKIT_ERROR_WIFI_UNSUPPORTED);
  return false;
#endif
}

bool ConnectKitClass::connectEthernet(const String& mac, int csPin, unsigned long timeoutMs) {
  byte macBytes[6];
  if (!parseMacAddress(mac, macBytes)) {
    setError(CONNECTKIT_ERROR_ETHERNET_INVALID_MAC);
    return false;
  }

  Ethernet.init(csPin);

  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    setError(CONNECTKIT_ERROR_ETHERNET_NO_HARDWARE);
    return false;
  }

  if (Ethernet.begin(macBytes, timeoutMs) == 0) {
    setError(CONNECTKIT_ERROR_ETHERNET_DHCP_FAILED);
    return false;
  }

  activateClient(connectKitEthernetClient, ipAddressToString(Ethernet.localIP()));
  return true;
}

void ConnectKitClass::setClient(Client& client, const String& localIp) {
  activateClient(client, localIp);
}

void ConnectKitClass::disconnect() {
  if (_activeClient != NULL) {
    _activeClient->stop();
  }

  _activeClient = NULL;
  _connected = false;
  _localIp = "";
}

bool ConnectKitClass::connected() {
  return _connected && _activeClient != NULL;
}

String ConnectKitClass::localIP() {
  return _localIp;
}

bool ConnectKitClass::setHeader(const String& name, const String& value) {
  String trimmedName = name;
  trimmedName.trim();

  if (trimmedName.length() == 0) {
    return false;
  }

  for (uint8_t i = 0; i < _headerCount; i++) {
    if (_headers[i].name.equalsIgnoreCase(trimmedName)) {
      _headers[i].name = trimmedName;
      _headers[i].value = value;
      return true;
    }
  }

  if (_headerCount >= CONNECTKIT_MAX_HTTP_HEADERS) {
    setError(CONNECTKIT_ERROR_TOO_MANY_HEADERS);
    return false;
  }

  _headers[_headerCount].name = trimmedName;
  _headers[_headerCount].value = value;
  _headerCount++;
  return true;
}

void ConnectKitClass::clearHeaders() {
  for (uint8_t i = 0; i < _headerCount; i++) {
    _headers[i].name = "";
    _headers[i].value = "";
  }
  _headerCount = 0;
}

String ConnectKitClass::request(const String& method, const String& url, const String& body, const String& contentType) {
  resetRequestState();

  if (!connected()) {
    setError(CONNECTKIT_ERROR_NO_ACTIVE_CONNECTION);
    return _lastResponse;
  }

  ParsedHttpUrl parsed;
  int urlError = CONNECTKIT_OK;
  if (!parseHttpUrl(url, parsed, urlError)) {
    setError(urlError);
    return _lastResponse;
  }

  String upperMethod = method;
  upperMethod.trim();
  upperMethod.toUpperCase();

  if (!isSupportedMethod(upperMethod)) {
    setError(CONNECTKIT_ERROR_HTTP_CLIENT);
    return _lastResponse;
  }

  bool hasBody = methodSendsBody(upperMethod) && body.length() > 0;

  HttpClient http(*_activeClient, parsed.host, parsed.port);
  http.setHttpResponseTimeout(_httpTimeoutMs);
  http.noDefaultRequestHeaders();
  http.beginRequest();

  int requestResult = http.startRequest(parsed.path.c_str(), upperMethod.c_str());
  if (requestResult != HTTP_SUCCESS) {
    setError(mapHttpClientError(requestResult));
    http.stop();
    return _lastResponse;
  }

  String hostHeader = hostHeaderValue(parsed);
  http.sendHeader("Host", hostHeader);
  http.sendHeader("User-Agent", "CodeKit/1.0");
  http.sendHeader("Connection", "close");

  if (hasBody) {
    if (contentType.length() > 0) {
      http.sendHeader("Content-Type", contentType);
    }
    http.sendHeader("Content-Length", (int)body.length());
  }

  for (uint8_t i = 0; i < _headerCount; i++) {
    http.sendHeader(_headers[i].name, _headers[i].value);
  }

  if (hasBody) {
    http.beginBody();
    http.print(body);
  } else {
    http.endRequest();
  }

  int statusCode = http.responseStatusCode();
  if (statusCode < 0) {
    setError(mapHttpClientError(statusCode));
    http.stop();
    return _lastResponse;
  }

  _lastStatusCode = statusCode;

  int headerResult = http.skipResponseHeaders();
  if (headerResult != HTTP_SUCCESS) {
    setError(mapHttpClientError(headerResult), false);
    http.stop();
    return _lastResponse;
  }

  long contentLength = http.contentLength();
  bool responseTooLarge = false;
  bool responseTimedOut = false;
  size_t storedBytes = 0;
  long readBytes = 0;

  if (_maxResponseSize > 0) {
    size_t reserveBytes = _maxResponseSize;
    if (contentLength >= 0 && (unsigned long)contentLength < (unsigned long)reserveBytes) {
      reserveBytes = (size_t)contentLength;
    }
    _lastResponse.reserve(reserveBytes);
  }

  while (contentLength < 0 || readBytes < contentLength) {
    if (!waitForHttpData(http, _httpTimeoutMs)) {
      if (contentLength >= 0 && readBytes < contentLength) {
        responseTimedOut = true;
      }
      break;
    }

    int c = http.read();
    if (c < 0) {
      if (!http.connected() && contentLength < 0) {
        break;
      }
      continue;
    }

    readBytes++;
    if (storedBytes < _maxResponseSize) {
      _lastResponse.concat((char)c);
      storedBytes++;
    } else {
      responseTooLarge = true;
      break;
    }
  }

  http.stop();

  if (responseTooLarge || (contentLength >= 0 && (unsigned long)contentLength > (unsigned long)_maxResponseSize)) {
    setError(CONNECTKIT_ERROR_RESPONSE_TOO_LARGE, false);
  } else if (responseTimedOut) {
    setError(CONNECTKIT_ERROR_HTTP_TIMEOUT, false);
  }

  return _lastResponse;
}

String ConnectKitClass::get(const String& url) {
  return request("GET", url, "", "");
}

String ConnectKitClass::post(const String& url, const String& body, const String& contentType) {
  return request("POST", url, body, contentType);
}

int ConnectKitClass::lastStatusCode() {
  return _lastStatusCode;
}

int ConnectKitClass::lastErrorCode() {
  return _lastErrorCode;
}

String ConnectKitClass::lastError() {
  return _lastError;
}

bool ConnectKitClass::lastRequestSucceeded() {
  return _lastErrorCode == CONNECTKIT_OK && _lastStatusCode >= 200 && _lastStatusCode < 300;
}

String ConnectKitClass::lastResponse() {
  return _lastResponse;
}

String ConnectKitClass::urlEncode(const String& text) {
  const char hex[] = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(text.length() * 3);

  for (unsigned int i = 0; i < text.length(); i++) {
    unsigned char c = (unsigned char)text[i];
    bool safe = (c >= 'A' && c <= 'Z') ||
                (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~';

    if (safe) {
      encoded += (char)c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      encoded += '%';
      encoded += hex[(c >> 4) & 0x0F];
      encoded += hex[c & 0x0F];
    }
  }

  return encoded;
}

String ConnectKitClass::urlWithQuery(const String& url, const String& name, const String& value) {
  String result = url;

  if (!result.endsWith("?") && !result.endsWith("&")) {
    result += result.indexOf('?') >= 0 ? "&" : "?";
  }

  result += urlEncode(name);
  result += "=";
  result += urlEncode(value);
  return result;
}

void ConnectKitClass::setHttpTimeout(unsigned long timeoutMs) {
  _httpTimeoutMs = timeoutMs;
}

void ConnectKitClass::setMaxResponseSize(size_t bytes) {
  _maxResponseSize = bytes;
}

int ConnectKitClass::ethernetHardwareStatus() {
  return Ethernet.hardwareStatus();
}

void ConnectKitClass::activateClient(Client& client, const String& localIp) {
  if (_activeClient != NULL && _activeClient != &client) {
    _activeClient->stop();
  }

  _activeClient = &client;
  _connected = true;
  _localIp = localIp;
  clearErrorState();
}

void ConnectKitClass::resetRequestState() {
  _lastResponse = "";
  _lastErrorCode = CONNECTKIT_OK;
  _lastError = "";
  _lastStatusCode = CONNECTKIT_OK;
}

void ConnectKitClass::clearErrorState() {
  _lastErrorCode = CONNECTKIT_OK;
  _lastError = "";
  _lastStatusCode = CONNECTKIT_OK;
}

void ConnectKitClass::setError(int code, bool updateStatusCode) {
  _lastErrorCode = code;
  _lastError = errorMessage(code);
  if (updateStatusCode) {
    _lastStatusCode = code;
  }
}
