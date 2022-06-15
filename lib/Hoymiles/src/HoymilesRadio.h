#pragma once

#include "CircularBuffer.h"
#include "TimeoutHelper.h"
#include "inverters/InverterAbstract.h"
#include "types.h"
#include <RF24.h>
#include <memory>
#include <nRF24L01.h>

// number of fragments hold in buffer
#define FRAGMENT_BUFFER_SIZE 30



class HoymilesRadio {
public:
    void init();
    void loop();
    void setPALevel(rf24_pa_dbm_e paLevel);

    serial_u DtuSerial();
    void setDtuSerial(uint64_t serial);

    bool isIdle();
    void sendEsbPacket(serial_u target, uint8_t mainCmd, uint8_t subCmd, uint8_t payload[], uint8_t len, uint32_t timeout, bool resend = false);
    void sendTimePacket(std::shared_ptr<InverterAbstract> iv, time_t ts);
    void sendRetransmitPacket(uint8_t fragment_id);
    void sendLastPacketAgain();

private:
    void ARDUINO_ISR_ATTR handleIntr();
    static serial_u convertSerialToRadioId(serial_u serial);
    static void convertSerialToPacketId(uint8_t buffer[], serial_u serial);
    uint8_t getRxNxtChannel();
    uint8_t getTxNxtChannel();
    bool switchRxCh(uint8_t addLoop = 0);
    void openReadingPipe();
    void openWritingPipe(serial_u serial);
    bool checkFragmentCrc(fragment_t* fragment);
    void dumpBuf(const char* info, uint8_t buf[], uint8_t len);
    void u32CpyLittleEndian(uint8_t dest[], uint32_t src);

    std::unique_ptr<RF24> _radio;
    uint8_t _rxChLst[4] = { 3, 23, 61, 75 };
    uint8_t _rxChIdx;
    uint16_t _rxLoopCnt;

    uint8_t _txChLst[1] = { 40 };
    uint8_t _txChIdx;

    volatile bool _packetReceived;

    CircularBuffer<fragment_t, FRAGMENT_BUFFER_SIZE> _rxBuffer;
    TimeoutHelper _rxTimeout;

    serial_u _dtuSerial;
    serial_u _activeSerial;

    bool _busyFlag = false;
};