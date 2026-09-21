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

static constexpr uint32_t CHANNEL_HOPPING_MICROS = 5192; // frame time in microseconds (2 * 2596)

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
    }

    switchRxCh(); // first check

    while (!_rxBuffer.empty()) {
        fragment_t f = _rxBuffer.front();
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

    handleReceivedPackage();

    switchRxCh(); // second check
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

uint8_t HoymilesRadio_NRF::getTxNxtChannel()
{
    // we sync start of transmitting mode to start of the next time frame.
    // This leads to an additional delay, and the transmission channel is determined randomly.
    uint32_t nowMicros = micros();
    uint32_t diffMicros = nowMicros - _refMicros;
    uint32_t addCh = diffMicros / CHANNEL_HOPPING_MICROS + 1;
    _refMicros = _refMicros + addCh * CHANNEL_HOPPING_MICROS;
    uint32_t delayMicros = _refMicros - nowMicros;

    if (delayMicros < CHANNEL_HOPPING_MICROS) {
        delayMicroseconds(delayMicros); // delay to the next time frame, 0ms - HOPPING_MICROS
    }

    // For example, if we are on channel 61, we will sync start of transmitting to channel 75
    _rxChIdx = (_rxChIdx + addCh) % sizeof(_rxChLst);
    return _rxChLst[_rxChIdx];

}

void HoymilesRadio_NRF::switchRxCh(bool const immediately)
{
    // channel hopping should be kept as precise as possible, even if the function has not been called for
    // a longer period of time or if the function is called multiple times in the same time frame.
    // Only if the immediately flag is set, the channel will be switched without checking the time.
    uint32_t diffMicros = micros() - _refMicros;
    if ((diffMicros >= CHANNEL_HOPPING_MICROS) || immediately) {

        // addCh can be 0, in this case we keep the current channel and just switch back to receiving mode.
        uint32_t addCh = diffMicros / CHANNEL_HOPPING_MICROS;
        _refMicros = _refMicros + addCh * CHANNEL_HOPPING_MICROS;
        _rxChIdx = (_rxChIdx + addCh) % sizeof(_rxChLst);

        _radio->stopListening();
        _radio->setChannel(_rxChLst[_rxChIdx]);
        _radio->startListening();
    }
}

void HoymilesRadio_NRF::sendEsbPacket(CommandAbstract& cmd)
{
    cmd.incrementSendCount();

    cmd.setRouterAddress(DtuSerial().u64);

    _radio->stopListening();
    _radio->setChannel(getTxNxtChannel());

    serial_u s;
    s.u64 = cmd.getTargetAddress();
    openWritingPipe(s);

    // the Automatic Retry Delay and the Automatic Retry Attempts are dynamically adjusted based
    // on the payload to optimize transmission time and success rate.
    uint8_t dataSize = std::min<uint8_t>(cmd.getDataSize(), sizeof(_ARD) - 1);
    uint8_t ard = _ARD[dataSize];   // ARD based on payload size, 0-32 bytes
    uint8_t art = 9;                // ART = 9 means that we try to send the packet up to 10 times
    _radio->setRetries(ard, art);

    ESP_LOGD(TAG, "TX %s Channel: %" PRIu8 " --> %s",
        cmd.getCommandName().c_str(), _radio->getChannel(), cmd.dumpDataPayload().c_str());
    auto result = _radio->write(cmd.getDataPayload(), cmd.getDataSize());

    _radio->setRetries(0, 0);
    openReadingPipe();
    switchRxCh(true); // switch back to the correct RX channel to be ready for the response.
    _busyFlag = true;
    _rxTimeout.set(cmd.getTimeout());

    _txCounter++;
    if (!result) { _txFailCounter++; }
    if (_txCounter > 100000) { _txCounter /= 2; _txFailCounter /= 2; }

    ESP_LOGD(TAG, "TX Result: %s, ARC Count: %u, Rx-Channel: %u", result ? "Ok" : "Fail",
        _radio->getARC(), _radio->getChannel());
    ESP_LOGD(TAG, "TX PayLoad: %u, ARD: %u, ART: %u", cmd.getDataSize(), ard, art);
    ESP_LOGD(TAG, "TX Acknowledge statistics: %0.2f%%",
        (_txCounter - _txFailCounter) * 100.0f / _txCounter);
}
