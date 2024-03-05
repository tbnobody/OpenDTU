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

const uint32_t interval = 1000;

static void Soutput(int kanal, int index, int art, int tarif, String Bezeichnung, double value, int timestamp){
  MessageOutput.print(Bezeichnung);
  MessageOutput.print('=');
  MessageOutput.println(value);
}

SMA_HMClass SMA_HM;

void SMA_HMClass::init(Scheduler& scheduler)
{
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
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
      event1();
  }
}

void SMA_HMClass::event1()
{
    uint8_t buffer[1024];
    int packetSize = SMAUdp.parsePacket();
    float Pbezug,BezugL1,BezugL2,BezugL3;
    Pbezug = 0;
    BezugL1 = 0;
    BezugL2 = 0;
    BezugL3 = 0;
    float Peinspeisung,EinspeisungL1,EinspeisungL2,EinspeisungL3;
    Peinspeisung = 0;
    EinspeisungL1 = 0;
    EinspeisungL2 = 0;
    EinspeisungL3 = 0;
    int count =0;
    if (packetSize) {
        int rSize = SMAUdp.read(buffer, 1024);
        if (buffer[0] != 'S' || buffer[1] != 'M' || buffer[2] != 'A') {
            MessageOutput.println("Not an SMA packet?");
            return;
        }
        uint16_t grouplen;
        uint16_t grouptag;
        uint8_t* offset = buffer + 4;
        do {
            grouplen = (offset[0] << 8) + offset[1];
            grouptag = (offset[2] << 8) + offset[3];
            offset += 4;
            if (grouplen == 0xffff) return;
            if (grouptag == 0x02A0 && grouplen == 4) {
                offset += 4;
            } else if (grouptag == 0x0010) {
                uint8_t* endOfGroup = offset + grouplen;
                uint16_t protocolID = (offset[0] << 8) + offset[1];
                offset += 2;
                uint16_t susyID = (offset[0] << 8) + offset[1];
                offset += 2;
                uint32_t serial = (offset[0] << 24) + (offset[1] << 16) + (offset[2] << 8) + offset[3];
                SMA_HM.serial=serial;
                offset += 4;
                uint32_t timestamp = (offset[0] << 24) + (offset[1] << 16) + (offset[2] << 8) + offset[3];
                offset += 4;
                while (offset < endOfGroup) {
                    uint8_t kanal = offset[0];
                    uint8_t index = offset[1];
                    uint8_t art = offset[2];
                    uint8_t tarif = offset[3];
                    offset += 4;
                    if (art == 8) {
                      uint64_t data = ((uint64_t)offset[0] << 56) +
                                      ((uint64_t)offset[1] << 48) +
                                      ((uint64_t)offset[2] << 40) +
                                      ((uint64_t)offset[3] << 32) +
                                      ((uint64_t)offset[4] << 24) +
                                      ((uint64_t)offset[5] << 16) +
                                      ((uint64_t)offset[6] << 8) +
                                      offset[7];
                        offset += 8;
                    } else if (art == 4) {
                      uint32_t data = (offset[0] << 24) +
                      (offset[1] << 16) +
                      (offset[2] << 8) +
                      offset[3];
                      offset += 4;
                      switch (index) {
                      case (1):
                        Pbezug = data * 0.1;
                        count +=1;
                        break;
                      case (2):
                        Peinspeisung = data * 0.1;
                        count +=1;
                        break;
                      case (21):
                        BezugL1 = data * 0.1;
                        count +=1;
                        break;
                      case (22):
                        EinspeisungL1 = data * 0.1;
                        count +=1;
                        break;
                      case (41):
                        BezugL2 = data * 0.1;
                        count +=1;
                        break;
                      case (42):
                        EinspeisungL2 = data * 0.1;
                        count +=1;
                        break;
                      case (61):
                        BezugL3 = data * 0.1;
                        count +=1;
                        break;
                      case (62):
                        count +=1;
                        EinspeisungL3 = data * 0.1;
                        break;
                      default:
                        break; // Wird nicht benötigt, wenn Statement(s) vorhanden sind
                      }
                      if (count == 8){
                        _powerMeterPower = Peinspeisung - Pbezug;
                        _powerMeterL1=EinspeisungL1-BezugL1;
                        _powerMeterL2=EinspeisungL2-BezugL2;
                        _powerMeterL3=EinspeisungL3-BezugL3;
                        Soutput(kanal, index, art, tarif, "Leistung", _powerMeterPower, timestamp);
                        Soutput(kanal, index, art, tarif, "Leistung L1", _powerMeterL1, timestamp);
                        Soutput(kanal, index, art, tarif, "Leistung L2", _powerMeterL2, timestamp);
                        Soutput(kanal, index, art, tarif, "Leistung L3", _powerMeterL3, timestamp);
                        count=0;
                      }
                    } else if (kanal==144) {
                      // Optional: Versionsnummer auslesen... aber interessiert die?
                      offset += 4;
                    } else {
                        offset += art;
                        MessageOutput.println("Strange measurement skipped");
                    }
                }
            } else if (grouptag == 0) {
                // end marker
                offset += grouplen;
            } else {
                MessageOutput.print("unhandled group ");
                MessageOutput.print(grouptag);
                MessageOutput.print(" with len=");
                MessageOutput.println(grouplen);
                offset += grouplen;
            }
        } while (grouplen > 0 && offset + 4 < buffer + rSize);
    }
}
float SMA_HMClass::getPowerTotal()
{
    return _powerMeterPower;
}
float SMA_HMClass::getPowerL1()
{
    return _powerMeterL1;
}
float SMA_HMClass::getPowerL2()
{
    return _powerMeterL2;
}
float SMA_HMClass::getPowerL3()
{
    return _powerMeterL3;
}
