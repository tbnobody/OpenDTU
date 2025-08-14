// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2025 Thomas Basler and others
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
    _radio->maskIRQ(true, true, false); // enable only receiving interrupts
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

    EVERY_N_MILLIS(4)
    {
        switchRxCh();
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
    _radio->openReadingPipe(1, s.u64);
}

void HoymilesRadio_NRF::openWritingPipe(const serial_u serial)
{
    const serial_u s = convertSerialToRadioId(serial);
    _radio->openWritingPipe(s.u64);
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

void HoymilesRadio_NRF::sendEsbPacket(CommandAbstract& cmd)
{
    cmd.incrementSendCount();

    cmd.setRouterAddress(DtuSerial().u64);

    _radio->stopListening();
    _radio->setChannel(getTxNxtChannel());

    serial_u s;
    s.u64 = cmd.getTargetAddress();
    openWritingPipe(s);
    _radio->setRetries(3, 15);

    ESP_LOGD(TAG, "TX %s Channel: %" PRIu8 " --> %s",
        cmd.getCommandName().c_str(), _radio->getChannel(), cmd.dumpDataPayload().c_str());
    _radio->write(cmd.getDataPayload(), cmd.getDataSize());

    _radio->setRetries(0, 0);
    openReadingPipe();
    _radio->setChannel(getRxNxtChannel());
    _radio->startListening();
    _busyFlag = true;
    _rxTimeout.set(cmd.getTimeout());
}
