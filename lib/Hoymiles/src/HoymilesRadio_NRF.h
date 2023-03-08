// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "HoymilesRadio.h"
#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include <RF24.h>
#include <memory>
#include <nRF24L01.h>
#include <queue>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30

class HoymilesRadio_NRF : public HoymilesRadio {
public:
    void init(SPIClass* initialisedSpiBus, uint8_t pinCE, uint8_t pinIRQ);
    void loop();
    void setPALevel(rf24_pa_dbm_e paLevel);

    virtual void setDtuSerial(uint64_t serial);

    bool isConnected();
    bool isPVariant();

private:
    void ARDUINO_ISR_ATTR handleIntr();
    uint8_t getRxNxtChannel();
    uint8_t getTxNxtChannel();
    void switchRxCh();
    void openReadingPipe();
    void openWritingPipe(serial_u serial);

    void sendEsbPacket(CommandAbstract* cmd);

    std::unique_ptr<SPIClass> _spiPtr;
    std::unique_ptr<RF24> _radio;
    uint8_t _rxChLst[5] = { 3, 23, 40, 61, 75 };
    uint8_t _rxChIdx = 0;

    uint8_t _txChLst[5] = { 3, 23, 40, 61, 75 };
    uint8_t _txChIdx = 0;

    volatile bool _packetReceived = false;

    std::queue<fragment_t> _rxBuffer;
    TimeoutHelper _rxTimeout;
};