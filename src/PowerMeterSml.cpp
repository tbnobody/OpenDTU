// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterSml.h"
#include "MessageOutput.h"

float PowerMeterSml::getPowerTotal() const
{
    std::lock_guard<std::mutex> l(_mutex);
    return _activePowerTotal;
}

void PowerMeterSml::doMqttPublish() const
{
    std::lock_guard<std::mutex> l(_mutex);
    mqttPublish("power1", _activePowerL1);
    mqttPublish("power2", _activePowerL2);
    mqttPublish("power3", _activePowerL3);
    mqttPublish("voltage1", _voltageL1);
    mqttPublish("voltage2", _voltageL2);
    mqttPublish("voltage3", _voltageL3);
    mqttPublish("current1", _currentL1);
    mqttPublish("current2", _currentL2);
    mqttPublish("current3", _currentL3);
    mqttPublish("import", _energyImport);
    mqttPublish("export", _energyExport);
}

void PowerMeterSml::processSmlByte(uint8_t byte)
{
    switch (smlState(byte)) {
        case SML_LISTEND:
            for (auto& handler: smlHandlerList) {
                if (!smlOBISCheck(handler.OBIS)) { continue; }

                gotUpdate();

                std::lock_guard<std::mutex> l(_mutex);
                handler.decoder(*handler.target);

                if (_verboseLogging) {
                    MessageOutput.printf("[PowerMeterSml] decoded %s to %.2f\r\n",
                            handler.name, *handler.target);
                }
            }
            break;
        default:
            break;
    }
}
