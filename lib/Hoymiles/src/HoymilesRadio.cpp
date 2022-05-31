#include "HoymilesRadio.h"
#include "crc.h"
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
    _radio->setRetries(0, 0);
    _radio->maskIRQ(true, true, false); // enable only receiving interrupts
    if (_radio->isChipConnected()) {
        Serial.println(F("Connection successfull"));
    } else {
        Serial.println(F("Connection error!!"));
    }

    attachInterrupt(digitalPinToInterrupt(16), std::bind(&HoymilesRadio::handleIntr, this), FALLING);

    openReadingPipe();
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
                fragment_t* f;
                f = _rxBuffer.getFront();
                memset(f->fragment, 0xcc, MAX_RF_PAYLOAD_SIZE);
                f->rxCh = _rxChLst[_rxChIdx];
                f->len = _radio->getDynamicPayloadSize();
                if (f->len > MAX_RF_PAYLOAD_SIZE)
                    f->len = MAX_RF_PAYLOAD_SIZE;

                _radio->read(f->fragment, f->len);
                _rxBuffer.pushFront(f);
            } else {
                Serial.println(F("Buffer full"));
                _radio->flush_rx();
            }
        }
        _packetReceived = false;

    } else {
        // Perform package parsing only if no packages are received
        if (!_rxBuffer.empty()) {
            fragment_t* f = _rxBuffer.getBack();
            if (checkFragmentCrc(f)) {
                Serial.println("Frame Ok");
            } else {
                Serial.println("Frame kaputt");
            }

            // Remove paket from buffer even it was corrupted
            _rxBuffer.popBack();
        }
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
    _dtuSerial.u64 = serial;
    openReadingPipe();
}

void HoymilesRadio::openReadingPipe()
{
    serial_u s;
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
    radioId.u64 = 0;
    radioId.b[4] = serial.b[0];
    radioId.b[3] = serial.b[1];
    radioId.b[2] = serial.b[2];
    radioId.b[1] = serial.b[3];
    radioId.b[0] = 0x01;
    return radioId;
}

bool HoymilesRadio::checkFragmentCrc(fragment_t* fragment)
{
    uint8_t crc = crc8(fragment->fragment, fragment->len - 1);
    return (crc == fragment->fragment[fragment->len - 1]);
}