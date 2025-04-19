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

    Task _loopTask;
    std::mutex _mutex;
    WiFiUDP _udp;
    IPAddress _address;
    String _syslog_hostname;
    String _proc_id;
    String _header;
    uint16_t _port;
    bool _enabled;
};

extern SyslogLogger Syslog;
