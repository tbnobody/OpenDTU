// SPDX-License-Identifier: GPL-2.0-or-later
#include "PowerMeterSml.h"
#include "MessageOutput.h"

float PowerMeterSml::getPowerTotal() const
{
    std::lock_guard<std::mutex> l(_mutex);
    return _activePower;
}

void PowerMeterSml::doMqttPublish() const
{
    std::lock_guard<std::mutex> l(_mutex);
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
