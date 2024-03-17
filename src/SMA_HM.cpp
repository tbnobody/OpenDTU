// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#include "SMA_HM.h"
#include <Arduino.h>
#include "Configuration.h"
#include "NetworkSettings.h"
#include <WiFiUdp.h>
#include "MessageOutput.h"

unsigned int multicastPort = 9522;  // local port to listen on
IPAddress multicastIP(239, 12, 255, 254);
WiFiUDP SMAUdp;

constexpr uint32_t interval = 1000;

SMA_HMClass SMA_HM;

void SMA_HMClass::Soutput(int kanal, int index, int art, int tarif,
        char const* name, float value, uint32_t timestamp)
{
    if (!_verboseLogging) { return; }

    MessageOutput.printf("SMA_HM: %s = %.1f (timestamp %d)\r\n",
            name, value, timestamp);
}

void SMA_HMClass::init(Scheduler& scheduler, bool verboseLogging)
{
    _verboseLogging = verboseLogging;
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&SMA_HMClass::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();
    SMAUdp.begin(multicastPort);
    SMAUdp.beginMulticast(multicastIP, multicastPort);
}

void SMA_HMClass::loop()
{
    uint32_t currentMillis = millis();
    if (currentMillis - _previousMillis >= interval) {
        _previousMillis = currentMillis;
        event1();
    }
}

uint8_t* SMA_HMClass::decodeGroup(uint8_t* offset, uint16_t grouplen)
{
    float Pbezug = 0;
    float BezugL1 = 0;
    float BezugL2 = 0;
    float BezugL3 = 0;
    float Peinspeisung = 0;
    float EinspeisungL1 = 0;
    float EinspeisungL2 = 0;
    float EinspeisungL3 = 0;

    uint8_t* endOfGroup = offset + grouplen;

    // not used: uint16_t protocolID = (offset[0] << 8) + offset[1];
    offset += 2;

    // not used: uint16_t susyID = (offset[0] << 8) + offset[1];
    offset += 2;

    _serial = (offset[0] << 24) + (offset[1] << 16) + (offset[2] << 8) + offset[3];
    offset += 4;

    uint32_t timestamp = (offset[0] << 24) + (offset[1] << 16) + (offset[2] << 8) + offset[3];
    offset += 4;

    unsigned count = 0;
    while (offset < endOfGroup) {
        uint8_t kanal = offset[0];
        uint8_t index = offset[1];
        uint8_t art = offset[2];
        uint8_t tarif = offset[3];
        offset += 4;

        if (kanal == 144) {
            // Optional: Versionsnummer auslesen... aber interessiert die?
            offset += 4;
            continue;
        }

        if (art == 8) {
            offset += 8;
            continue;
        }

        if (art == 4) {
            uint32_t data = (offset[0] << 24) +
                (offset[1] << 16) +
                (offset[2] << 8) +
                offset[3];
            offset += 4;

            switch (index) {
                case (1):
                    Pbezug = data * 0.1;
                    ++count;
                    break;
                case (2):
                    Peinspeisung = data * 0.1;
                    ++count;
                    break;
                case (21):
                    BezugL1 = data * 0.1;
                    ++count;
                    break;
                case (22):
                    EinspeisungL1 = data * 0.1;
                    ++count;
                    break;
                case (41):
                    BezugL2 = data * 0.1;
                    ++count;
                    break;
                case (42):
                    EinspeisungL2 = data * 0.1;
                    ++count;
                    break;
                case (61):
                    BezugL3 = data * 0.1;
                    ++count;
                    break;
                case (62):
                    EinspeisungL3 = data * 0.1;
                    ++count;
                    break;
                default:
                    break;
            }

            if (count == 8) {
                _powerMeterPower = Peinspeisung - Pbezug;
                _powerMeterL1 = EinspeisungL1 - BezugL1;
                _powerMeterL2 = EinspeisungL2 - BezugL2;
                _powerMeterL3 = EinspeisungL3 - BezugL3;
                Soutput(kanal, index, art, tarif, "Leistung", _powerMeterPower, timestamp);
                Soutput(kanal, index, art, tarif, "Leistung L1", _powerMeterL1, timestamp);
                Soutput(kanal, index, art, tarif, "Leistung L2", _powerMeterL2, timestamp);
                Soutput(kanal, index, art, tarif, "Leistung L3", _powerMeterL3, timestamp);
                count = 0;
            }

            continue;
        }

        MessageOutput.printf("SMA_HM: Skipped unknown measurement: %d %d %d %d\r\n",
                kanal, index, art, tarif);
        offset += art;
    }

    return offset;
}

void SMA_HMClass::event1()
{
    int packetSize = SMAUdp.parsePacket();
    if (!packetSize) { return; }

    uint8_t buffer[1024];
    int rSize = SMAUdp.read(buffer, 1024);
    if (buffer[0] != 'S' || buffer[1] != 'M' || buffer[2] != 'A') {
        MessageOutput.println("SMA_HM: Not an SMA packet?");
        return;
    }

    uint16_t grouplen;
    uint16_t grouptag;
    uint8_t* offset = buffer + 4; // skips the header 'SMA\0'

    do {
        grouplen = (offset[0] << 8) + offset[1];
        grouptag = (offset[2] << 8) + offset[3];
        offset += 4;

        if (grouplen == 0xffff) return;

        if (grouptag == 0x02A0 && grouplen == 4) {
            offset += 4;
            continue;
        }

        if (grouptag == 0x0010) {
            offset = decodeGroup(offset, grouplen);
            continue;
        }

        if (grouptag == 0) {
            // end marker
            offset += grouplen;
            continue;
        }

        MessageOutput.printf("SMA_HM: Unhandled group 0x%04x with length %d\r\n",
                grouptag, grouplen);
        offset += grouplen;
    } while (grouplen > 0 && offset + 4 < buffer + rSize);
}
