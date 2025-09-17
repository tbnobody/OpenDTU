// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 Holger-Steffen Stapf
 */
#include <powermeter/smahm/udp/Provider.h>
#include <Arduino.h>
#include <WiFiUdp.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "powerMeter";
static const char* SUBTAG = "SMAHM/UDP";

namespace PowerMeters::SmaHM::Udp {

static constexpr unsigned int multicastPort = 9522;  // local port to listen on
static const IPAddress multicastIP(239, 12, 255, 254);
static WiFiUDP SMAUdp;

constexpr uint32_t interval = 1000;

void Provider::Soutput(int kanal, int index, int art, int tarif,
        char const* name, float value, uint32_t timestamp)
{
    DTU_LOGD("%s = %.1f (timestamp %u)", name, value, timestamp);
}

bool Provider::init()
{
    SMAUdp.begin(multicastPort);
    SMAUdp.beginMulticast(multicastIP, multicastPort);
    return true;
}

Provider::~Provider()
{
    SMAUdp.stop();
}

uint8_t* Provider::decodeGroup(uint8_t* offset, uint16_t grouplen)
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
                auto powerTotal = Pbezug - Peinspeisung;
                auto powerL1 = BezugL1 - EinspeisungL1;
                auto powerL2 = BezugL2 - EinspeisungL2;
                auto powerL3 = BezugL3 - EinspeisungL3;

                {
                    auto scopedLock = _dataCurrent.lock();
                    _dataCurrent.add<DataPointLabel::PowerTotal>(powerTotal);
                    _dataCurrent.add<DataPointLabel::PowerL1>(powerL1);
                    _dataCurrent.add<DataPointLabel::PowerL2>(powerL2);
                    _dataCurrent.add<DataPointLabel::PowerL3>(powerL3);
                }

                Soutput(kanal, index, art, tarif, "Leistung", powerTotal, timestamp);
                Soutput(kanal, index, art, tarif, "Leistung L1", powerL1, timestamp);
                Soutput(kanal, index, art, tarif, "Leistung L2", powerL2, timestamp);
                Soutput(kanal, index, art, tarif, "Leistung L3", powerL3, timestamp);
                count = 0;
            }

            continue;
        }

        DTU_LOGI("Skipped unknown measurement: %d %d %d %d", kanal, index, art, tarif);
        offset += art;
    }

    return offset;
}

void Provider::loop()
{
    uint32_t currentMillis = millis();
    if (currentMillis - _previousMillis < interval) { return; }

    _previousMillis = currentMillis;

    int packetSize = SMAUdp.parsePacket();
    if (!packetSize) { return; }

    uint8_t buffer[1024];
    int rSize = SMAUdp.read(buffer, 1024);
    if (buffer[0] != 'S' || buffer[1] != 'M' || buffer[2] != 'A') {
        DTU_LOGE("Not an SMA packet?");
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

        DTU_LOGW("Unhandled group 0x%04x with length %d", grouptag, grouplen);
        offset += grouplen;
    } while (grouplen > 0 && offset + 4 < buffer + rSize);
}

} // namespace PowerMeters::SmaHM::Udp
