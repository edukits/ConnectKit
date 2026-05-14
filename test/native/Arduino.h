#ifndef CONNECTKIT_NATIVE_ARDUINO_H
#define CONNECTKIT_NATIVE_ARDUINO_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

typedef uint8_t byte;

extern unsigned long fake_millis;

inline unsigned long millis() {
  return fake_millis;
}

inline void delay(unsigned long ms) {
  fake_millis += ms;
}

class String {
public:
  String() : value_() {}
  String(const char* value) : value_(value == NULL ? "" : value) {}
  String(char value) : value_(1, value) {}
  String(const std::string& value) : value_(value) {}
  String(int value) : value_(std::to_string(value)) {}
  String(unsigned int value) : value_(std::to_string(value)) {}
  String(unsigned long value) : value_(std::to_string(value)) {}

  unsigned int length() const {
    return (unsigned int)value_.length();
  }

  bool reserve(size_t size) {
    value_.reserve(size);
    return true;
  }

  const char* c_str() const {
    return value_.c_str();
  }

  char operator[](int index) const {
    return value_[(size_t)index];
  }

  String& operator=(const char* value) {
    value_ = value == NULL ? "" : value;
    return *this;
  }

  String& operator+=(const String& value) {
    value_ += value.value_;
    return *this;
  }

  String& operator+=(const char* value) {
    value_ += value == NULL ? "" : value;
    return *this;
  }

  String& operator+=(char value) {
    value_ += value;
    return *this;
  }

  bool operator==(const String& other) const {
    return value_ == other.value_;
  }

  bool operator==(const char* other) const {
    return value_ == (other == NULL ? "" : other);
  }

  bool operator!=(const char* other) const {
    return !(*this == other);
  }

  void trim() {
    size_t first = 0;
    while (first < value_.length() && std::isspace((unsigned char)value_[first])) {
      first++;
    }

    size_t last = value_.length();
    while (last > first && std::isspace((unsigned char)value_[last - 1])) {
      last--;
    }

    value_ = value_.substr(first, last - first);
  }

  void toUpperCase() {
    std::transform(value_.begin(), value_.end(), value_.begin(), [](unsigned char c) {
      return (char)std::toupper(c);
    });
  }

  void toLowerCase() {
    std::transform(value_.begin(), value_.end(), value_.begin(), [](unsigned char c) {
      return (char)std::tolower(c);
    });
  }

  bool startsWith(const char* prefix) const {
    std::string needle = prefix == NULL ? "" : prefix;
    return value_.compare(0, needle.length(), needle) == 0;
  }

  bool endsWith(const char* suffix) const {
    std::string needle = suffix == NULL ? "" : suffix;
    if (needle.length() > value_.length()) {
      return false;
    }
    return value_.compare(value_.length() - needle.length(), needle.length(), needle) == 0;
  }

  int indexOf(char needle, int from = 0) const {
    if (from < 0) {
      from = 0;
    }
    size_t found = value_.find(needle, (size_t)from);
    return found == std::string::npos ? -1 : (int)found;
  }

  int lastIndexOf(char needle) const {
    size_t found = value_.rfind(needle);
    return found == std::string::npos ? -1 : (int)found;
  }

  String substring(int start) const {
    if (start < 0) {
      start = 0;
    }
    if ((size_t)start >= value_.length()) {
      return String("");
    }
    return String(value_.substr((size_t)start));
  }

  String substring(int start, int end) const {
    if (start < 0) {
      start = 0;
    }
    if (end < start) {
      end = start;
    }
    if ((size_t)start >= value_.length()) {
      return String("");
    }
    return String(value_.substr((size_t)start, (size_t)(end - start)));
  }

  bool equalsIgnoreCase(const String& other) const {
    if (value_.length() != other.value_.length()) {
      return false;
    }

    for (size_t i = 0; i < value_.length(); i++) {
      if (std::tolower((unsigned char)value_[i]) != std::tolower((unsigned char)other.value_[i])) {
        return false;
      }
    }
    return true;
  }

  bool concat(char value) {
    value_ += value;
    return true;
  }

  bool concat(const String& value) {
    value_ += value.value_;
    return true;
  }

  const std::string& stdString() const {
    return value_;
  }

private:
  std::string value_;
};

inline bool operator==(const char* left, const String& right) {
  return right == left;
}

class IPAddress {
public:
  IPAddress() : value_("0.0.0.0") {
    setOctets(0, 0, 0, 0);
  }

  IPAddress(const char* value) : value_(value == NULL ? "0.0.0.0" : value) {
    parseOctets();
  }

  String toString() const {
    return String(value_);
  }

  uint8_t operator[](int index) const {
    return octets_[index];
  }

private:
  std::string value_;
  uint8_t octets_[4];

  void setOctets(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    octets_[0] = a;
    octets_[1] = b;
    octets_[2] = c;
    octets_[3] = d;
  }

  void parseOctets() {
    unsigned int parts[4] = { 0, 0, 0, 0 };
    std::sscanf(value_.c_str(), "%u.%u.%u.%u", &parts[0], &parts[1], &parts[2], &parts[3]);
    setOctets((uint8_t)parts[0], (uint8_t)parts[1], (uint8_t)parts[2], (uint8_t)parts[3]);
  }
};

class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t value) = 0;

  virtual size_t write(const uint8_t* buffer, size_t size) {
    size_t written = 0;
    for (size_t i = 0; i < size; i++) {
      written += write(buffer[i]);
    }
    return written;
  }

  size_t print(const char* value) {
    if (value == NULL) {
      return 0;
    }
    return write((const uint8_t*)value, std::strlen(value));
  }

  size_t print(const String& value) {
    return print(value.c_str());
  }

  size_t print(char value) {
    return write((uint8_t)value);
  }

  size_t print(int value) {
    return print(String(value));
  }

  size_t print(unsigned int value) {
    return print(String(value));
  }

  size_t println() {
    return print("\r\n");
  }

  size_t println(const char* value) {
    return print(value) + println();
  }

  size_t println(const String& value) {
    return print(value) + println();
  }

  size_t println(int value) {
    return print(value) + println();
  }
};

class Client : public Print {
public:
  virtual int connect(IPAddress ip, uint16_t port) = 0;
  virtual int connect(const char* host, uint16_t port) = 0;
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int read(uint8_t* buffer, size_t size) = 0;
  virtual int peek() = 0;
  virtual void flush() = 0;
  virtual void stop() = 0;
  virtual uint8_t connected() = 0;
  virtual operator bool() = 0;
};

#endif
