#include "HoymilesRadio.h"
#include <Every.h>
#include <FunctionalInterrupt.h>

void HoymilesRadio::init()
{
    _dtuSerial.u64 = 0;

    _radio.reset(new RF24(4, 5));
    _radio->begin();
    _radio->setDataRate(RF24_250KBPS);
    _radio->enableDynamicPayloads();
    _radio->setCRCLength(RF24_CRC_16);
    _radio->setAddressWidth(5);
    _radio->setAutoAck(false);
    _radio->setRetries(0, 0);
    _radio->setPALevel(RF_PWR_LOW);
    _radio->maskIRQ(true, true, false); // enable only receiving interrupts
    if (_radio->isChipConnected()) {
        Serial.println(F("Connection successfull"));
    } else {
        Serial.println(F("Connection error!!"));
    }

    setDtuSerial(_dtuSerial.u64);

    attachInterrupt(digitalPinToInterrupt(16), std::bind(&HoymilesRadio::handleIntr, this), FALLING);
    _radio->startListening();
}

void HoymilesRadio::loop()
{
    EVERY_N_MILLIS(4)
    {
        switchRxCh(1);
    }

    // Irgendwie muss man hier die paket crc prüfen und ggf. einen retransmit anfordern
    // ggf aber immer nur ein paket analysieren damit die loop schnell bleibt

    if (_packetReceived) {
        Serial.println(F("Interrupt received"));
        while (_radio->available()) {
            if (!_rxBuffer.full()) {
                packet_t* p;
                uint8_t len;
                p = _rxBuffer.getFront();
                memset(p->packet, 0xcc, MAX_RF_PAYLOAD_SIZE);
                p->rxCh = _rxChLst[_rxChIdx];
                len = _radio->getPayloadSize();
                if (len > MAX_RF_PAYLOAD_SIZE)
                    len = MAX_RF_PAYLOAD_SIZE;

                _radio->read(p->packet, len);
                _rxBuffer.pushFront(p);
            } else {
                Serial.println(F("Buffer full"));
                _radio->flush_rx();
            }
        }

        _packetReceived = false;
    }
}

void HoymilesRadio::setPALevel(rf24_pa_dbm_e paLevel)
{
    _radio->setPALevel(paLevel);
}

serial_u HoymilesRadio::DtuSerial()
{
    return _dtuSerial;
}

void HoymilesRadio::setDtuSerial(uint64_t serial)
{
    serial_u s;
    _dtuSerial.u64 = serial;
    s = convertSerialToRadioId(_dtuSerial);
    _radio->openReadingPipe(1, s.u64);
}

void ARDUINO_ISR_ATTR HoymilesRadio::handleIntr()
{
    _packetReceived = true;
}

uint8_t HoymilesRadio::getRxNxtChannel()
{
    if (++_rxChIdx >= 4)
        _rxChIdx = 0;
    return _rxChLst[_rxChIdx];
}

bool HoymilesRadio::switchRxCh(uint8_t addLoop)
{
    _rxLoopCnt += addLoop;
    if (_rxLoopCnt != 0) {
        _rxLoopCnt--;
        // portDISABLE_INTERRUPTS();
        _radio->stopListening();
        _radio->setChannel(getRxNxtChannel());
        _radio->startListening();
        // portENABLE_INTERRUPTS();
    }
    return (0 == _rxLoopCnt); // receive finished
}

serial_u HoymilesRadio::convertSerialToRadioId(serial_u serial)
{
    serial_u radioId;
    radioId.b[4] = serial.b[0];
    radioId.b[3] = serial.b[1];
    radioId.b[2] = serial.b[2];
    radioId.b[1] = serial.b[3];
    radioId.b[0] = 0x01;
    return radioId;
}