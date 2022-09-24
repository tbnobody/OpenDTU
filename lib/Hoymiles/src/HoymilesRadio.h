#pragma once

#include "CircularBuffer.h"
#include "TimeoutHelper.h"
#include "commands/CommandAbstract.h"
#include "types.h"
#include <RF24.h>
#include "mp_memory.h"
#include <nRF24L01.h>
#include "mp_queue.h"

#ifdef __AVR__
#define ARDUINO_ISR_ATTR
#endif

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30

#define MAX_RESEND_COUNT 4

#ifndef HOYMILES_PIN_MISO
#define HOYMILES_PIN_MISO 19
#endif

#ifndef HOYMILES_PIN_MOSI
#define HOYMILES_PIN_MOSI 23
#endif

#ifndef HOYMILES_PIN_SCLK
#define HOYMILES_PIN_SCLK 18
#endif

#ifndef HOYMILES_PIN_IRQ
#define HOYMILES_PIN_IRQ 16
#endif

#ifndef HOYMILES_PIN_CE
#define HOYMILES_PIN_CE 4
#endif

#ifndef HOYMILES_PIN_CS
#define HOYMILES_PIN_CS 5
#endif

class HoymilesRadio {
public:
    void init(_SPI* initialisedSpiBus);
    void loop();
    void setPALevel(rf24_pa_dbm_e paLevel);

    serial_u DtuSerial();
    void setDtuSerial(uint64_t serial);

    bool isIdle();
    void sendEsbPacket(CommandAbstract* cmd);
    void sendRetransmitPacket(uint8_t fragment_id);
    void sendLastPacketAgain();

    template <typename T>
    T* enqueCommand()
    {
        _commandQueue.push(std::make_shared<T>());
        return static_cast<T*>(_commandQueue.back().get());
    }

    volatile static bool _packetReceived;

private:
    static void ARDUINO_ISR_ATTR handleIntr();
    static serial_u convertSerialToRadioId(serial_u serial);
    uint8_t getRxNxtChannel();
    uint8_t getTxNxtChannel();
    void switchRxCh();
    void openReadingPipe();
    void openWritingPipe(serial_u serial);
    bool checkFragmentCrc(fragment_t* fragment);
    void dumpBuf(const char* info, uint8_t buf[], uint8_t len);

    std::unique_ptr<_SPI> _spiPtr;
    std::unique_ptr<RF24> _radio;
    uint8_t _rxChLst[5] = { 3, 23, 40, 61, 75 };
    uint8_t _rxChIdx = 0;

    uint8_t _txChLst[5] = { 3, 23, 40, 61, 75 };
    uint8_t _txChIdx = 0;

    CircularBuffer<fragment_t, FRAGMENT_BUFFER_SIZE> _rxBuffer;
    TimeoutHelper _rxTimeout;

    serial_u _dtuSerial;

    bool _busyFlag = false;

    std::queue<std::shared_ptr<CommandAbstract>> _commandQueue;
};