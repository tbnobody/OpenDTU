// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <AsyncWebSocket.h>
#include <HardwareSerial.h>
#include <Stream.h>
#include <mutex>

#define BUFFER_SIZE 500

enum MessageOutputDebugLevel {
  DEBUG_NONE,
  DEBUG_INFO,
  DEBUG_DEBUG,
  DEBUG_TRACE
};

class NoOuptputClass : public Print {
public:
  size_t write(uint8_t c) override { return 1; }
  size_t write(const uint8_t *buffer, size_t size) override { return size; }
};

class MessageOutputClass : public Print {
public:
  void loop();
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  void register_ws_output(AsyncWebSocket *output);

  template <typename... Args> size_t printf(const char *f, Args... args) {
    return printf(MessageOutputDebugLevel::DEBUG_DEBUG, f, args...);
  }
  template <typename... Args> size_t print(Args... args) {
    return print(MessageOutputDebugLevel::DEBUG_DEBUG, args...);
  }
  size_t println() {
    return print(MessageOutputDebugLevel::DEBUG_DEBUG, "\n");
  }

  template <typename... Args> size_t println(const char *f) {
    return printf(MessageOutputDebugLevel::DEBUG_DEBUG, "%s\n", f);
  }
  template <typename... Args> size_t println(const __FlashStringHelper *f) {
    return printf(MessageOutputDebugLevel::DEBUG_DEBUG, "%s\n", reinterpret_cast<const char *>(f));
  }
  
  template <typename... Args>
  size_t print(MessageOutputDebugLevel level, Args... args) {
    if (isLevel(level))
      return Print::print(args...);
    return 0;
  }
  template <typename... Args>
  size_t printf(MessageOutputDebugLevel level, const char *f, Args... args) {
    if (isLevel(level))
      return Print::printf(f, args...);
    return 0;
  }
  void setLevel(MessageOutputDebugLevel l) { level = l; }
  bool isLevel(MessageOutputDebugLevel l) { return (l <= level); }

private:
  AsyncWebSocket *_ws = NULL;
  char _buffer[BUFFER_SIZE];
  uint16_t _buff_pos = 0;
  uint32_t _lastSend = 0;
  bool _forceSend = false;
  MessageOutputDebugLevel level = MessageOutputDebugLevel::DEBUG_DEBUG;
  std::mutex _msgLock;
};

extern MessageOutputClass MessageOutput;