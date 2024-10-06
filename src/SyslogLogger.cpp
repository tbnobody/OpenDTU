// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */
#include <HardwareSerial.h>
#include <ESPmDNS.h>
#include "defaults.h"
#include "SyslogLogger.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "NetworkSettings.h"

SyslogLogger::SyslogLogger()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&SyslogLogger::loop, this))
{
}

void SyslogLogger::init(Scheduler& scheduler)
{
    // PROCID change indicates a restart.
    _proc_id = String(esp_random(), HEX);

    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void SyslogLogger::updateSettings(const String&& hostname)
{
    auto& config = Configuration.get().Syslog;

    // Disable logger while it is reconfigured.
    disable();

    if (!config.Enabled) {
        MessageOutput.println("[SyslogLogger] Syslog not enabled");
        return;
    }

    _port = config.Port;
    _syslog_hostname = config.Hostname;
    if (_syslog_hostname.isEmpty()) {
        MessageOutput.println("[SyslogLogger] Hostname not configured");
        return;
    }

    MessageOutput.printf("[SyslogLogger] Logging to %s!\r\n", _syslog_hostname.c_str());

    _header = "<14>1 - ";  // RFC5424: Facility USER, severity INFO, version 1, NIL timestamp.
    _header += hostname;
    _header += " OpenDTU ";
    _header += _proc_id;
    // NIL values for message id and structured data
    _header += " - - ";

    // Enable logger.
    enable();
}

void SyslogLogger::write(const uint8_t *buffer, size_t size)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_enabled || !isResolved()) {
        return;
    }
    for (int i = 0; i < size; i++) {
        uint8_t c = buffer[i];
        bool overflow = false;
        if (c != '\r' && c != '\n') {
            // Replace control and non-ASCII characters with '?'.
            overflow = !_udp.write(c >= 0x20 && c < 0x7f ? c : '?');
        }
        if (c == '\n' || overflow) {
            _udp.endPacket();
            _udp.beginPacket(_address, _port);
            _udp.print(_header);
        }
    }
}

void SyslogLogger::disable()
{
    MessageOutput.println("[SyslogLogger] Disable");
    std::lock_guard<std::mutex> lock(_mutex);
    if (_enabled) {
        _enabled = false;
        _address = INADDR_NONE;
        _udp.stop();
    }
}

void SyslogLogger::enable()
{
    // Bind random source port.
    if (!_udp.begin(0)) {
        MessageOutput.println("[SyslogLogger] No sockets available");
        return;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    _enabled = true;
}

bool SyslogLogger::resolveAndStart()
{
    if (Configuration.get().Mdns.Enabled) {
        _address = MDNS.queryHost(_syslog_hostname); // INADDR_NONE if failed
    }
    if (_address != INADDR_NONE) {
        if (!_udp.beginPacket(_address, _port)) {
            return false;
        }
    } else {
        if (!_udp.beginPacket(_syslog_hostname.c_str(), _port)) {
            return false;
        }
        _address = _udp.remoteIP();  // Store resolved address.
    }
    _udp.print(_header);
    _udp.print("[SyslogLogger] Logging to ");
    _udp.print(_syslog_hostname);
    _udp.endPacket();
    _udp.beginPacket(_address, _port);
    _udp.print(_header);
    return true;
}

void SyslogLogger::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_enabled || !NetworkSettings.isConnected() || isResolved()) {
        return;
    }
    if (!resolveAndStart()) {
        _enabled = false;
    }
}

SyslogLogger Syslog;
