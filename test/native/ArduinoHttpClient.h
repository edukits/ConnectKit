#ifndef CONNECTKIT_NATIVE_ARDUINO_HTTP_CLIENT_H
#define CONNECTKIT_NATIVE_ARDUINO_HTTP_CLIENT_H

#include "Client.h"

static const int HTTP_SUCCESS = 0;
static const int HTTP_ERROR_CONNECTION_FAILED = -1;
static const int HTTP_ERROR_API = -2;
static const int HTTP_ERROR_TIMED_OUT = -3;
static const int HTTP_ERROR_INVALID_RESPONSE = -4;

class HttpClient : public Client {
public:
  HttpClient(Client& client, const char* serverName, uint16_t serverPort = 80)
    : client_(&client),
      serverName_(serverName),
      serverPort_(serverPort),
      contentLength_(-1),
      timeout_(30000UL) {
  }

  HttpClient(Client& client, const String& serverName, uint16_t serverPort = 80)
    : client_(&client),
      serverName_(serverName),
      serverPort_(serverPort),
      contentLength_(-1),
      timeout_(30000UL) {
  }

  void beginRequest() {
  }

  void endRequest() {
    beginBody();
  }

  void beginBody() {
    client_->print("\r\n");
  }

  void noDefaultRequestHeaders() {
  }

  int startRequest(const char* path, const char* method, const char* contentType = NULL, int contentLength = -1, const byte body[] = NULL) {
    (void)contentType;
    (void)contentLength;
    (void)body;
    if (client_->connect(serverName_.c_str(), serverPort_) <= 0) {
      return HTTP_ERROR_CONNECTION_FAILED;
    }
    client_->print(method);
    client_->print(" ");
    client_->print(path);
    client_->println(" HTTP/1.1");
    return HTTP_SUCCESS;
  }

  void sendHeader(const char* header) {
    client_->println(header);
  }

  void sendHeader(const String& header) {
    sendHeader(header.c_str());
  }

  void sendHeader(const char* name, const char* value) {
    client_->print(name);
    client_->print(": ");
    client_->println(value);
  }

  void sendHeader(const String& name, const String& value) {
    sendHeader(name.c_str(), value.c_str());
  }

  void sendHeader(const char* name, int value) {
    client_->print(name);
    client_->print(": ");
    client_->println(value);
  }

  int responseStatusCode() {
    String line = readLine();
    if (line.length() == 0) {
      return HTTP_ERROR_TIMED_OUT;
    }

    if (!line.startsWith("HTTP/")) {
      return HTTP_ERROR_INVALID_RESPONSE;
    }

    int firstSpace = line.indexOf(' ');
    if (firstSpace < 0 || firstSpace + 3 >= (int)line.length()) {
      return HTTP_ERROR_INVALID_RESPONSE;
    }

    String statusText = line.substring(firstSpace + 1, firstSpace + 4);
    for (unsigned int i = 0; i < statusText.length(); i++) {
      if (!std::isdigit((unsigned char)statusText[i])) {
        return HTTP_ERROR_INVALID_RESPONSE;
      }
    }

    return std::atoi(statusText.c_str());
  }

  int skipResponseHeaders() {
    contentLength_ = -1;

    while (true) {
      String line = readLine();
      if (line.length() == 0) {
        return HTTP_SUCCESS;
      }

      String lower = line;
      lower.toLowerCase();
      if (lower.startsWith("content-length:")) {
        String value = line.substring(15);
        value.trim();
        contentLength_ = std::atol(value.c_str());
      }
    }
  }

  long contentLength() {
    return contentLength_;
  }

  void setHttpResponseTimeout(uint32_t timeout) {
    timeout_ = timeout;
  }

  size_t write(uint8_t value) override {
    return client_->write(value);
  }

  size_t write(const uint8_t* buffer, size_t size) override {
    return client_->write(buffer, size);
  }

  int connect(IPAddress ip, uint16_t port) override {
    return client_->connect(ip, port);
  }

  int connect(const char* host, uint16_t port) override {
    return client_->connect(host, port);
  }

  int available() override {
    return client_->available();
  }

  int read() override {
    return client_->read();
  }

  int read(uint8_t* buffer, size_t size) override {
    return client_->read(buffer, size);
  }

  int peek() override {
    return client_->peek();
  }

  void flush() override {
    client_->flush();
  }

  void stop() override {
    client_->stop();
  }

  uint8_t connected() override {
    return client_->connected();
  }

  operator bool() override {
    return bool(*client_);
  }

private:
  Client* client_;
  String serverName_;
  uint16_t serverPort_;
  long contentLength_;
  uint32_t timeout_;

  String readLine() {
    String line;
    unsigned long start = millis();

    while ((millis() - start) <= timeout_) {
      if (client_->available() > 0) {
        int c = client_->read();
        if (c < 0) {
          continue;
        }
        if (c == '\n') {
          if (line.endsWith("\r")) {
            line = line.substring(0, line.length() - 1);
          }
          return line;
        }
        line += (char)c;
      } else if (!client_->connected()) {
        return line;
      } else {
        delay(1);
      }
    }

    return String("");
  }
};

#endif
