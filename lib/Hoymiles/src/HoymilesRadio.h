// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <RF24.h>
#include <memory>
#include <nRF24L01.h>
#include <queue>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30

class HoymilesRadio {
public:
    void init(SPIClass* initialisedSpiBus, uint8_t pinCE, uint8_t pinIRQ);
    void loop();
    void setPALevel(rf24_pa_dbm_e paLevel);

    serial_u DtuSerial();
    void setDtuSerial(uint64_t serial);

    bool isIdle();
    bool isConnected();
    bool isPVariant();

    template <typename T>
    T* enqueCommand()
    {
        _commandQueue.push(std::make_shared<T>());
        return static_cast<T*>(_commandQueue.back().get());
    }

private:
    void ARDUINO_ISR_ATTR handleIntr();
    static serial_u convertSerialToRadioId(serial_u serial);
    uint8_t getRxNxtChannel();
    uint8_t getTxNxtChannel();
    void switchRxCh();
    void openReadingPipe();
    void openWritingPipe(serial_u serial);
    bool checkFragmentCrc(fragment_t* fragment);
    void dumpBuf(const char* info, uint8_t buf[], uint8_t len);

    void sendEsbPacket(CommandAbstract* cmd);
    void sendRetransmitPacket(uint8_t fragment_id);
    void sendLastPacketAgain();

    std::unique_ptr<SPIClass> _spiPtr;
    std::unique_ptr<RF24> _radio;
    uint8_t _rxChLst[5] = { 3, 23, 40, 61, 75 };
    uint8_t _rxChIdx = 0;

    uint8_t _txChLst[5] = { 3, 23, 40, 61, 75 };
    uint8_t _txChIdx = 0;

    volatile bool _packetReceived = false;

    std::queue<fragment_t> _rxBuffer;
    TimeoutHelper _rxTimeout;

    serial_u _dtuSerial;

    bool _busyFlag = false;

    std::queue<std::shared_ptr<CommandAbstract>> _commandQueue;
};