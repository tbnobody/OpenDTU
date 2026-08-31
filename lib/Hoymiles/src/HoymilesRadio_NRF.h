// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HoymilesRadio.h"
#include "commands/CommandAbstract.h"
#include <RF24.h>
#include <memory>
#include <nRF24L01.h>
#include <queue>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30
#define MAX_CHANNELS 5
#define MAX_COUNTER 3

class HoymilesRadio_NRF : public HoymilesRadio {
public:
    void init(SPIClass* initialisedSpiBus, const uint8_t pinCE, const uint8_t pinIRQ);
    void loop();
    void setPALevel(const rf24_pa_dbm_e paLevel);

    virtual void setDtuSerial(const uint64_t serial);

    bool isConnected() const;
    bool isPVariant() const;

private:
    void ARDUINO_ISR_ATTR handleIntr();
    uint8_t getTxNxtChannel();
    void switchRxCh(bool const ignoreTime = false);
    void openReadingPipe();
    void openWritingPipe(const serial_u serial);

    void sendEsbPacket(CommandAbstract& cmd);

    std::unique_ptr<SPIClass> _spiPtr;
    std::unique_ptr<RF24> _radio;
    uint8_t _chLst[MAX_CHANNELS] = { 3, 23, 40, 61, 75 };
    uint8_t _rxChIdx = 0;
    uint8_t _txChIdx = 0;

    uint32_t _refMicros = 0;
    uint64_t _syncInverterSerial = 0;
    uint32_t _syncLifetime = 0;
    bool _inSync = false;

    uint16_t _txCounter = 0;
    uint16_t _txFailCounter = 0;
    uint16_t _inPercentCounter[MAX_COUNTER] = {0, 0, 0};

    volatile bool _packetReceived = false;
    volatile uint32_t _packetMicros = 0;

    std::queue<fragment_t> _rxBuffer;
};
