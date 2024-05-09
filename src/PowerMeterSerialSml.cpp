// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterSerialSml.h"
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

PowerMeterSerialSml::~PowerMeterSerialSml()
{
    if (!_upSmlSerial) { return; }
    _upSmlSerial->end();
}

void PowerMeterSerialSml::loop()
{
    if (!_upSmlSerial) { return; }

    while (_upSmlSerial->available()) {
        processSmlByte(_upSmlSerial->read());
    }

    MessageOutput.printf("[PowerMeterSerialSml]: TotalPower: %5.2f\r\n", getPowerTotal());
}
