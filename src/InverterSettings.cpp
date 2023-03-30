// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 Thomas Basler and others
 */
#include "InverterSettings.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include "SunPosition.h"
#include <Hoymiles.h>

#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define VSPI FSPI
#endif

InverterSettingsClass InverterSettings;

void InverterSettingsClass::init()
{
    const CONFIG_T& config = Configuration.get();
    const PinMapping_t& pin = PinMapping.get();

    // Initialize inverter communication
    MessageOutput.print(F("Initialize Hoymiles interface... "));
    if (PinMapping.isValidNrf24Config()) {
        SPIClass* spiClass = new SPIClass(VSPI);
        spiClass->begin(pin.nrf24_clk, pin.nrf24_miso, pin.nrf24_mosi, pin.nrf24_cs);
        Hoymiles.setMessageOutput(&MessageOutput);
        Hoymiles.init(spiClass, pin.nrf24_en, pin.nrf24_irq);

        MessageOutput.println(F("  Setting radio PA level... "));
        Hoymiles.getRadio()->setPALevel((rf24_pa_dbm_e)config.Dtu_PaLevel);

        MessageOutput.println(F("  Setting DTU serial... "));
        Hoymiles.getRadio()->setDtuSerial(config.Dtu_Serial);

        MessageOutput.println(F("  Setting poll interval... "));
        Hoymiles.setPollInterval(config.Dtu_PollInterval);

        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            if (config.Inverter[i].Serial > 0) {
                MessageOutput.print(F("  Adding inverter: "));
                MessageOutput.print(config.Inverter[i].Serial, HEX);
                MessageOutput.print(F(" - "));
                MessageOutput.print(config.Inverter[i].Name);
                auto inv = Hoymiles.addInverter(
                    config.Inverter[i].Name,
                    config.Inverter[i].Serial);

                if (inv != nullptr) {
                    for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
                        inv->Statistics()->setStringMaxPower(c, config.Inverter[i].channel[c].MaxChannelPower);
                        inv->Statistics()->setChannelFieldOffset(TYPE_DC, static_cast<ChannelNum_t>(c), FLD_YT, config.Inverter[i].channel[c].YieldTotalOffset);
                    }
                }
                MessageOutput.println(F(" done"));
            }
        }
        MessageOutput.println(F("done"));
    } else {
        MessageOutput.println(F("Invalid pin config"));
    }
}

void InverterSettingsClass::loop()
{
    if (millis() - _lastUpdate > SUNPOS_UPDATE_INTERVAL) {
        const CONFIG_T& config = Configuration.get();

        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            auto const& inv_cfg = config.Inverter[i];
            if (inv_cfg.Serial == 0) {
                continue;
            }
            auto inv = Hoymiles.getInverterBySerial(inv_cfg.Serial);
            if (inv == nullptr) {
                continue;
            }

            inv->setEnablePolling(inv_cfg.Poll_Enable && (SunPosition.isDayPeriod() || inv_cfg.Poll_Enable_Night));
            inv->setEnableCommands(inv_cfg.Command_Enable && (SunPosition.isDayPeriod() || inv_cfg.Command_Enable_Night));
        }
    }

    Hoymiles.loop();
}