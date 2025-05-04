// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
 */
#include "SyslogLogger.h"
#include "Configuration.h"
#include "NetworkSettings.h"
#include "defaults.h"
#include <ESPmDNS.h>
#include <HardwareSerial.h>

#undef TAG
static const char* TAG = "syslog";

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
        ESP_LOGI(TAG, "Syslog not enabled");
        return;
    }

    _port = config.Port;
    _syslog_hostname = config.Hostname;
    if (_syslog_hostname.isEmpty()) {
        ESP_LOGW(TAG, "Hostname not configured");
        return;
    }

    ESP_LOGI(TAG, "Logging to %s!", _syslog_hostname.c_str());

    _header = ">1 - "; // RFC5424: Facility USER, severity INFO, version 1, NIL timestamp.
    _header += hostname;
    _header += " OpenDTU ";
    _header += _proc_id;
    // NIL values for message id and structured data
    _header += " - - ";

    // Enable logger.
    enable();
}

void SyslogLogger::write(const uint8_t* buffer, size_t size)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_enabled || !isResolved()) {
        return;
    }

    String header = "<";
    header += String(calculatePrival(1, buffer[0]));

    _udp.beginPacket(_address, _port);
    _udp.print(header);
    _udp.print(_header);

    for (int i = 0; i < size; i++) {
        uint8_t c = buffer[i];
        if (c != '\r' && c != '\n') {
            // Replace control and non-ASCII characters with '?'.
            _udp.write(c >= 0x20 && c < 0x7f ? c : '?');
        }
    }
    _udp.endPacket();
}

void SyslogLogger::disable()
{
    ESP_LOGI(TAG, "Disable");
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
        ESP_LOGE(TAG, "No sockets available");
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
        _address = _udp.remoteIP(); // Store resolved address.
    }
    return true;
}

uint8_t SyslogLogger::calculatePrival(uint8_t facility, char errorCode)
{
    // ESP LOG ID's are two ahead of syslog ID's
    // e.g. ESP_LOG_ERROR (1) = Syslog ERROR 3
    if (errorCode == 'E') {
        return facility * 8 + ESP_LOG_ERROR + 2;
    } else if (errorCode == 'W') {
        return facility * 8 + ESP_LOG_WARN + 2;
    } else if (errorCode == 'D') {
        return facility * 8 + ESP_LOG_DEBUG + 2;
    } else if (errorCode == 'V') {
        return facility * 8 + ESP_LOG_VERBOSE + 2;
    }

    return facility * 8 + ESP_LOG_INFO + 2;
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
