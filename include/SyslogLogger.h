// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <WiFiUdp.h>
#include <TaskSchedulerDeclarations.h>
#include <mutex>

class SyslogLogger {
public:
    SyslogLogger();
    void init(Scheduler& scheduler);
    void updateSettings(const String&& hostname);
    void write(const uint8_t *buffer, size_t size);

private:
    void loop();
    void disable();
    void enable();
    bool resolveAndStart();
    bool isResolved() const {
        return _address != INADDR_NONE;
    }
    static uint8_t calculatePrival(uint8_t facility, char errorCode);
    bool consumeToken();

    Task _loopTask;
    std::mutex _mutex;
    WiFiUDP _udp;
    IPAddress _address;
    String _syslog_hostname;
    String _proc_id;
    String _header;
    uint16_t _port;
    bool _enabled;

    static constexpr uint32_t RATE_LIMIT_WINDOW_MS = 1000;
    static constexpr size_t RATE_LIMIT_MAX_TOKENS = 100;
    size_t _available_tokens = RATE_LIMIT_MAX_TOKENS;
    uint32_t _last_token_refill_millis = 0;
    size_t _rate_limited_packets = 0;
    uint32_t _last_rate_limit_warning_millis = 0;
    static constexpr uint32_t RATE_LIMIT_WARNING_INTERVAL_MS = 1000;
};

extern SyslogLogger Syslog;
