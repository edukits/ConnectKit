#ifndef CONNECTKIT_NATIVE_ETHERNET_H
#define CONNECTKIT_NATIVE_ETHERNET_H

#include "Client.h"

enum EthernetHardwareStatus {
  EthernetNoHardware = 0,
  EthernetW5100 = 1,
  EthernetW5200 = 2,
  EthernetW5500 = 3
};

class EthernetClient : public Client {
public:
  EthernetClient() : connected_(false) {}

  int connect(IPAddress ip, uint16_t port) override {
    (void)ip;
    (void)port;
    connected_ = true;
    return 1;
  }

  int connect(const char* host, uint16_t port) override {
    (void)host;
    (void)port;
    connected_ = true;
    return 1;
  }

  int available() override {
    return 0;
  }

  int read() override {
    return -1;
  }

  int read(uint8_t* buffer, size_t size) override {
    (void)buffer;
    (void)size;
    return 0;
  }

  int peek() override {
    return -1;
  }

  void flush() override {
  }

  void stop() override {
    connected_ = false;
  }

  uint8_t connected() override {
    return connected_ ? 1 : 0;
  }

  operator bool() override {
    return true;
  }

  size_t write(uint8_t value) override {
    (void)value;
    return 1;
  }

private:
  bool connected_;
};

class EthernetClass {
public:
  EthernetClass()
    : beginResult(1),
      status(EthernetW5500),
      lastCsPin(-1),
      lastTimeout(0),
      ip("192.168.1.22") {
    std::memset(lastMac, 0, sizeof(lastMac));
  }

  void init(int csPin) {
    lastCsPin = csPin;
  }

  int begin(byte* mac, unsigned long timeout = 60000UL) {
    std::memcpy(lastMac, mac, sizeof(lastMac));
    lastTimeout = timeout;
    return beginResult;
  }

  EthernetHardwareStatus hardwareStatus() {
    return status;
  }

  IPAddress localIP() {
    return IPAddress(ip.c_str());
  }

  int beginResult;
  EthernetHardwareStatus status;
  int lastCsPin;
  unsigned long lastTimeout;
  byte lastMac[6];
  std::string ip;
};

extern EthernetClass Ethernet;

#endif
