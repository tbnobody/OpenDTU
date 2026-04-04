// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2026 Thomas Basler and others
 */
#include "HoymilesRadio_NRF.h"
#include "Hoymiles.h"
#include "Utils.h"
#include "commands/RequestFrameCommand.h"
#include <Every.h>
#include <FunctionalInterrupt.h>
#include <esp_log.h>

#undef TAG
static const char* TAG = "hoymiles";

#define NEW_TX_METHOD
#define NEW_RX_METHOD
#define HOPPING_MICROS 4000

void HoymilesRadio_NRF::init(SPIClass* initialisedSpiBus, const uint8_t pinCE, const uint8_t pinIRQ)
{
    _dtuSerial.u64 = 0;

    _spiPtr.reset(initialisedSpiBus);
    _radio.reset(new RF24(pinCE, initialisedSpiBus->pinSS()));

    _radio->begin(_spiPtr.get());

    _radio->setDataRate(RF24_250KBPS);
    _radio->enableDynamicPayloads();
    _radio->setCRCLength(RF24_CRC_16);
    _radio->setAddressWidth(5);
    _radio->setRetries(0, 0);
    _radio->setStatusFlags(RF24_RX_DR); // enable only receiving interrupts
    if (!_radio->isChipConnected()) {
        ESP_LOGE(TAG, "NRF: Connection error!!");
        return;
    }
    ESP_LOGI(TAG, "NRF: Connection successful");

    attachInterrupt(digitalPinToInterrupt(pinIRQ), std::bind(&HoymilesRadio_NRF::handleIntr, this), FALLING);

    openReadingPipe();
    _radio->startListening();
    _isInitialized = true;
}

void HoymilesRadio_NRF::loop()
{
    if (!_isInitialized) {
        return;
    }

    #ifdef NEW_RX_METHOD
    checkRxCh(); // first check
    #else
    EVERY_N_MILLIS(4)
    {
        switchRxCh();
    }
    #endif

    if (_packetReceived) {
        ESP_LOGV(TAG, "Interrupt received");
        while (_radio->available()) {
            if (_rxBuffer.size() > FRAGMENT_BUFFER_SIZE) {
                ESP_LOGE(TAG, "NRF: Buffer full");
                _radio->flush_rx();
                continue;
            }

            fragment_t f;
            memset(f.fragment, 0xcc, MAX_RF_PAYLOAD_SIZE);
            f.len = std::min<uint8_t>(_radio->getDynamicPayloadSize(), MAX_RF_PAYLOAD_SIZE);
            f.channel = _radio->getChannel();
            f.rssi = _radio->testRPD() ? -30 : -80;
            _radio->read(f.fragment, f.len);
            _rxBuffer.push(f);
        }
        _packetReceived = false;

    } else {
        // Perform package parsing only if no packages are received
        if (!_rxBuffer.empty()) {
            fragment_t f = _rxBuffer.back();
            if (checkFragmentCrc(f)) {
                std::shared_ptr<InverterAbstract> inv = Hoymiles.getInverterByFragment(f);

                if (nullptr != inv) {
                    // Save packet in inverter rx buffer
                    ESP_LOGD(TAG, "RX Channel: %" PRIu8 " --> %s | %" PRId8 " dBm",
                        f.channel, Utils::dumpArray(f.fragment, f.len).c_str(), f.rssi);

                    inv->addRxFragment(f.fragment, f.len, f.rssi);
                } else {
                    ESP_LOGE(TAG, "Inverter Not found!");
                }

            } else {
                ESP_LOGW(TAG, "Frame kaputt");
            }

            // Remove paket from buffer even it was corrupted
            _rxBuffer.pop();
        }
    }

    handleReceivedPackage();

    #ifdef NEW_RX_METHOD
    checkRxCh(); // second check
    #endif
}

void HoymilesRadio_NRF::setPALevel(const rf24_pa_dbm_e paLevel)
{
    if (!_isInitialized) {
        return;
    }
    _radio->setPALevel(paLevel);
}

void HoymilesRadio_NRF::setDtuSerial(const uint64_t serial)
{
    HoymilesRadio::setDtuSerial(serial);

    if (!_isInitialized) {
        return;
    }
    openReadingPipe();
}

bool HoymilesRadio_NRF::isConnected() const
{
    if (!_isInitialized) {
        return false;
    }
    return _radio->isChipConnected();
}

bool HoymilesRadio_NRF::isPVariant() const
{
    if (!_isInitialized) {
        return false;
    }
    return _radio->isPVariant();
}

void HoymilesRadio_NRF::openReadingPipe()
{
    const serial_u s = convertSerialToRadioId(_dtuSerial);
    _radio->openReadingPipe(1, s.b);
}

void HoymilesRadio_NRF::openWritingPipe(const serial_u serial)
{
    const serial_u s = convertSerialToRadioId(serial);
    _radio->stopListening(s.b);
}

void ARDUINO_ISR_ATTR HoymilesRadio_NRF::handleIntr()
{
    _packetReceived = true;
}

uint8_t HoymilesRadio_NRF::getRxNxtChannel()
{
    if (++_rxChIdx >= sizeof(_rxChLst))
        _rxChIdx = 0;
    return _rxChLst[_rxChIdx];
}

uint8_t HoymilesRadio_NRF::getTxNxtChannel()
{
    if (++_txChIdx >= sizeof(_txChLst))
        _txChIdx = 0;
    return _txChLst[_txChIdx];
}

void HoymilesRadio_NRF::switchRxCh()
{
    _radio->stopListening();
    _radio->setChannel(getRxNxtChannel());
    _radio->startListening();
}

void HoymilesRadio_NRF::checkRxCh(bool const immediately)
{
    // channel hopping should be kept as precise as possible,
    // even if the function has not been called for a longer
    // period of time or if the function is called multiple times
    // in the same period.
    // Only if the immediately flag is set, the channel will be
    // switched without checking the time.
    uint32_t diffMicros = micros() - _refMicros;
    if (diffMicros >= HOPPING_MICROS || immediately) {
        uint32_t addCh = diffMicros / HOPPING_MICROS;
        _rxChIdx = (_rxChIdx + addCh) % sizeof(_rxChLst);
        _refMicros = _refMicros + addCh * HOPPING_MICROS;

        _radio->stopListening();
        _radio->setChannel(_rxChLst[_rxChIdx]);
        _radio->startListening();
    }
}

uint8_t HoymilesRadio_NRF::syncToRxCh()
{
    // we sync to the next rx channel to avoid long delays.
    // That means the tx channel is randomly switched between the 5 channels.
    // If we want to step continuously through all tx channels,
    // we would have to accept longer delays.
    uint32_t nowMicros = micros();
    uint32_t diffMicros = nowMicros - _refMicros;
    uint32_t addCh = diffMicros / HOPPING_MICROS + 1;
    _refMicros = _refMicros + addCh * HOPPING_MICROS;
    uint32_t delayMicros = _refMicros - nowMicros;
    if (delayMicros < HOPPING_MICROS) {
        delayMicroseconds(delayMicros); // delay of 0-4ms
    }

    // we sync transmit to the next rx channel.
    //For example, if we are on channel 61, we will sync to channel 75
    // rx-Channel:     40|61|75|03|23|40|61
    // transmit sync:       |75|
    _rxChIdx = (_rxChIdx + addCh) % sizeof(_rxChLst);

    return _rxChLst[_rxChIdx];
}

void HoymilesRadio_NRF::sendEsbPacket(CommandAbstract& cmd)
{
    cmd.incrementSendCount();

    cmd.setRouterAddress(DtuSerial().u64);

    _radio->stopListening();

    #ifdef NEW_TX_METHOD
    _radio->setChannel(syncToRxCh());
    #else
    _radio->setChannel(getTxNxtChannel());
    #endif

    serial_u s;
    s.u64 = cmd.getTargetAddress();
    openWritingPipe(s);

    #ifdef NEW_TX_METHOD
    // the Automatic Retry Delay and the Automatic Retry Attempts are dynamically adjusted based
    // on the payload to optimize transmission time and success rate. (5 * 4ms = 20ms)
    uint8_t dataSize = std::min<uint8_t>(cmd.getDataSize(), sizeof(_ARD) - 1);
    uint8_t ard = _ARD[dataSize];
    uint8_t art = _ART[dataSize];
    #else
    uint8_t ard = 3;
    uint8_t art = 15;
    #endif

    _radio->setRetries(ard, art);

    ESP_LOGD(TAG, "TX %s Channel: %" PRIu8 " --> %s",
        cmd.getCommandName().c_str(), _radio->getChannel(), cmd.dumpDataPayload().c_str());
    _radio->write(cmd.getDataPayload(), cmd.getDataSize());

    _radio->setRetries(0, 0);
    openReadingPipe();

    #ifdef NEW_RX_METHOD
    checkRxCh(true); // switch back to the correct RX channel to be ready for the response.
    #else
    _radio->setChannel(getRxNxtChannel());
    _radio->startListening();
    #endif

    _busyFlag = true;
    _rxTimeout.set(cmd.getTimeout());
}
