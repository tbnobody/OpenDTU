// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterSerialSml.h"
#include "Configuration.h"
#include "PinMapping.h"
#include "MessageOutput.h"

bool PowerMeterSerialSml::init()
{
    const PinMapping_t& pin = PinMapping.get();

    MessageOutput.printf("[PowerMeterSerialSml] rx = %d\r\n", pin.powermeter_rx);

    if (pin.powermeter_rx < 0) {
        MessageOutput.println("[PowerMeterSerialSml] invalid pin config "
                "for serial SML power meter (RX pin must be defined)");
        return false;
    }

    pinMode(pin.powermeter_rx, INPUT);
    _upSmlSerial = std::make_unique<SoftwareSerial>();
    _upSmlSerial->begin(9600, SWSERIAL_8N1, pin.powermeter_rx, -1, false, 128, 95);
    _upSmlSerial->enableRx(true);
    _upSmlSerial->enableTx(false);
    _upSmlSerial->flush();

    return true;
}

void PowerMeterSerialSml::deinit()
{
    if (!_upSmlSerial) { return; }
    _upSmlSerial->end();
}

void PowerMeterSerialSml::doMqttPublish() const
{
    std::lock_guard<std::mutex> l(_mutex);
    mqttPublish("import", _energyImport);
    mqttPublish("export", _energyExport);
}

void PowerMeterSerialSml::loop()
{
    if (!_upSmlSerial) { return; }

    while (_upSmlSerial->available()) {
        double readVal = 0;
        unsigned char smlCurrentChar = _upSmlSerial->read();
        sml_states_t smlCurrentState = smlState(smlCurrentChar);
        if (smlCurrentState == SML_LISTEND) {
            for (auto& handler: smlHandlerList) {
                if (smlOBISCheck(handler.OBIS)) {
                    handler.Fn(readVal);
                    std::lock_guard<std::mutex> l(_mutex);
                    *handler.Arg = readVal;
                }
            }
        } else if (smlCurrentState == SML_FINAL) {
            gotUpdate();
        }
    }

    MessageOutput.printf("[PowerMeterSerialSml]: TotalPower: %5.2f\r\n", getPowerTotal());
}
