#include <cassert>
#include <iostream>

#include "ConnectKit.h"
#include "Ethernet.h"

unsigned long fake_millis = 0;
EthernetClass Ethernet;

class FakeClient : public Client {
public:
  explicit FakeClient(const char* responseText = "")
    : connectResult(true),
      holdOpenOnEmpty(false),
      connectedPort(0),
      stopped(false),
      connected_(false),
      response_(responseText == NULL ? "" : responseText),
      readIndex_(0) {
  }

  int connect(IPAddress ip, uint16_t port) override {
    (void)ip;
    return connect("0.0.0.0", port);
  }

  int connect(const char* host, uint16_t port) override {
    connectedHost = host == NULL ? "" : host;
    connectedPort = port;
    stopped = false;

    if (!connectResult) {
      connected_ = false;
      return 0;
    }

    connected_ = true;
    readIndex_ = 0;
    return 1;
  }

  int available() override {
    int remaining = (int)(response_.length() - readIndex_);
    if (remaining == 0 && !holdOpenOnEmpty) {
      connected_ = false;
    }
    return remaining;
  }

  int read() override {
    if (readIndex_ >= response_.length()) {
      if (!holdOpenOnEmpty) {
        connected_ = false;
      }
      return -1;
    }

    char c = response_[readIndex_++];
    if (readIndex_ >= response_.length() && !holdOpenOnEmpty) {
      connected_ = false;
    }
    return (unsigned char)c;
  }

  int read(uint8_t* buffer, size_t size) override {
    size_t count = 0;
    while (count < size && available() > 0) {
      int c = read();
      if (c < 0) {
        break;
      }
      buffer[count++] = (uint8_t)c;
    }
    return (int)count;
  }

  int peek() override {
    if (readIndex_ >= response_.length()) {
      return -1;
    }
    return (unsigned char)response_[readIndex_];
  }

  void flush() override {
  }

  void stop() override {
    stopped = true;
    connected_ = false;
  }

  uint8_t connected() override {
    return connected_ ? 1 : 0;
  }

  operator bool() override {
    return true;
  }

  size_t write(uint8_t value) override {
    request.push_back((char)value);
    return 1;
  }

  bool connectResult;
  bool holdOpenOnEmpty;
  std::string request;
  std::string connectedHost;
  uint16_t connectedPort;
  bool stopped;

private:
  bool connected_;
  std::string response_;
  size_t readIndex_;
};

void resetConnectKit() {
  ConnectKit.disconnect();
  ConnectKit.clearHeaders();
  ConnectKit.setHttpTimeout(5);
  ConnectKit.setMaxResponseSize(CONNECTKIT_DEFAULT_MAX_RESPONSE_SIZE);
  fake_millis = 0;
  Ethernet.beginResult = 1;
  Ethernet.status = EthernetW5500;
}

bool contains(const std::string& text, const char* needle) {
  return text.find(needle) != std::string::npos;
}

void testMacParsing() {
  resetConnectKit();
  assert(ConnectKit.connectEthernet("DE:AD:BE:EF:FE:ED", 7, 1234UL));
  assert(Ethernet.lastCsPin == 7);
  assert(Ethernet.lastTimeout == 1234UL);
  assert(Ethernet.lastMac[0] == 0xDE);
  assert(Ethernet.lastMac[1] == 0xAD);
  assert(Ethernet.lastMac[2] == 0xBE);
  assert(Ethernet.lastMac[3] == 0xEF);
  assert(Ethernet.lastMac[4] == 0xFE);
  assert(Ethernet.lastMac[5] == 0xED);

  resetConnectKit();
  assert(!ConnectKit.connectEthernet("DE:AD:BE", 10, 1UL));
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_ETHERNET_INVALID_MAC);
}

void testUrlParsingAndHttpRequest() {
  resetConnectKit();
  FakeClient client("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK");
  ConnectKit.setClient(client);

  String response = ConnectKit.get("http://example.com:8080/path?q=1");
  assert(response == "OK");
  assert(client.connectedHost == "example.com");
  assert(client.connectedPort == 8080);
  assert(contains(client.request, "GET /path?q=1 HTTP/1.1\r\n"));
  assert(contains(client.request, "Host: example.com:8080\r\n"));
  assert(contains(client.request, "User-Agent: CodeKit/1.0\r\n"));
  assert(contains(client.request, "Connection: close\r\n"));
  assert(ConnectKit.lastRequestSucceeded());
}

void testUrlHelpers() {
  resetConnectKit();
  assert(ConnectKit.urlEncode("a b+c/%") == "a+b%2Bc%2F%25");
  assert(ConnectKit.urlEncode("AZaz09-_.~") == "AZaz09-_.~");
  assert(ConnectKit.urlWithQuery("http://example.com/path", "sensor name", "room 1") == "http://example.com/path?sensor+name=room+1");
  assert(ConnectKit.urlWithQuery("http://example.com/path?x=1", "unit", "deg C") == "http://example.com/path?x=1&unit=deg+C");
  assert(ConnectKit.urlWithQuery("http://example.com/path?", "x", "y") == "http://example.com/path?x=y");
}

void testHeaders() {
  resetConnectKit();
  assert(ConnectKit.setHeader(" X-Test ", "first"));
  assert(ConnectKit.setHeader("x-test", "second"));

  for (int i = 0; i < CONNECTKIT_MAX_HTTP_HEADERS - 1; i++) {
    String name = "X-Extra-";
    name += String(i);
    assert(ConnectKit.setHeader(name, "value"));
  }

  assert(!ConnectKit.setHeader("X-Overflow", "value"));
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_TOO_MANY_HEADERS);

  FakeClient client("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
  ConnectKit.setClient(client);
  ConnectKit.get("http://example.com/");

  assert(contains(client.request, "x-test: second\r\n"));
  assert(!contains(client.request, "X-Test: first\r\n"));
}

void testRequestFailures() {
  resetConnectKit();
  ConnectKit.get("http://example.com/");
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_NO_ACTIVE_CONNECTION);

  FakeClient client("HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
  ConnectKit.setClient(client);
  ConnectKit.get("http:///missing");
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_HTTP_MISSING_HOST);

  ConnectKit.get("https://example.com/");
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_HTTPS_UNSUPPORTED);

  FakeClient failingClient;
  failingClient.connectResult = false;
  ConnectKit.setClient(failingClient);
  ConnectKit.get("http://example.com/");
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_HTTP_CONNECT_FAILED);

  FakeClient timeoutClient;
  timeoutClient.holdOpenOnEmpty = true;
  ConnectKit.setClient(timeoutClient);
  ConnectKit.get("http://example.com/");
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_HTTP_TIMEOUT);
}

void testStatusAndResponseHandling() {
  resetConnectKit();
  FakeClient successClient("HTTP/1.1 201 Created\r\nContent-Length: 7\r\n\r\ncreated");
  ConnectKit.setClient(successClient);
  assert(ConnectKit.post("http://example.com/items", "{}", "application/json") == "created");
  assert(ConnectKit.lastStatusCode() == 201);
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_OK);
  assert(ConnectKit.lastRequestSucceeded());
  assert(contains(successClient.request, "POST /items HTTP/1.1\r\n"));
  assert(contains(successClient.request, "Content-Type: application/json\r\n"));
  assert(contains(successClient.request, "Content-Length: 2\r\n"));

  FakeClient failureClient("HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nnot found");
  ConnectKit.setClient(failureClient);
  assert(ConnectKit.get("http://example.com/missing") == "not found");
  assert(ConnectKit.lastStatusCode() == 404);
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_OK);
  assert(!ConnectKit.lastRequestSucceeded());

  FakeClient largeClient("HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nabcdef");
  ConnectKit.setClient(largeClient);
  ConnectKit.setMaxResponseSize(5);
  assert(ConnectKit.get("http://example.com/large") == "abcde");
  assert(ConnectKit.lastStatusCode() == 200);
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_RESPONSE_TOO_LARGE);
  assert(!ConnectKit.lastRequestSucceeded());
}

void testStateResetBetweenRequests() {
  resetConnectKit();
  FakeClient client("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK");
  ConnectKit.setClient(client);

  ConnectKit.get("https://example.com/");
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_ERROR_HTTPS_UNSUPPORTED);

  String response = ConnectKit.get("http://example.com/");
  assert(response == "OK");
  assert(ConnectKit.lastStatusCode() == 200);
  assert(ConnectKit.lastErrorCode() == CONNECTKIT_OK);
  assert(ConnectKit.lastError() == "");
  assert(ConnectKit.lastRequestSucceeded());
}

int main() {
  testMacParsing();
  testUrlParsingAndHttpRequest();
  testUrlHelpers();
  testHeaders();
  testRequestFailures();
  testStatusAndResponseHandling();
  testStateResetBetweenRequests();

  std::cout << "native ConnectKit tests passed\n";
  return 0;
}
