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

// the NRF shall use the second externally usable HW SPI controller
// for ESP32 that is the so-called VSPI, for ESP32-S2/S3 it is now called implicitly
// HSPI, as it has shifted places for these chip generations
// for all generations, this is equivalent to SPI3_HOST in the lower level driver
// For ESP32-C2, the only externally usable HW SPI controller is SPI2, its signal names
// being prefixed with FSPI.
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define SPI_NRF HSPI
#elif CONFIG_IDF_TARGET_ESP32C3
#define SPI_NRF FSPI
#else
#define SPI_NRF VSPI
#endif

InverterSettingsClass InverterSettings;

void InverterSettingsClass::init()
{
    const CONFIG_T& config = Configuration.get();
    const PinMapping_t& pin = PinMapping.get();

    // Initialize inverter communication
    MessageOutput.print("Initialize Hoymiles interface... ");

    Hoymiles.setMessageOutput(&MessageOutput);
    Hoymiles.init();

    if (PinMapping.isValidNrf24Config() || PinMapping.isValidCmt2300Config()) {
        if (PinMapping.isValidNrf24Config()) {
            SPIClass* spiClass = new SPIClass(SPI_NRF);
            spiClass->begin(pin.nrf24_clk, pin.nrf24_miso, pin.nrf24_mosi, pin.nrf24_cs);
            Hoymiles.initNRF(spiClass, pin.nrf24_en, pin.nrf24_irq);
        }

        if (PinMapping.isValidCmt2300Config()) {
            Hoymiles.initCMT(pin.cmt_sdio, pin.cmt_clk, pin.cmt_cs, pin.cmt_fcs, pin.cmt_gpio2, pin.cmt_gpio3);
            MessageOutput.println(F("  Setting CMT target frequency... "));
            Hoymiles.getRadioCmt()->setInverterTargetFrequency(config.Dtu_CmtFrequency);
        }

        MessageOutput.println("  Setting radio PA level... ");
        Hoymiles.getRadioNrf()->setPALevel((rf24_pa_dbm_e)config.Dtu_NrfPaLevel);
        Hoymiles.getRadioCmt()->setPALevel(config.Dtu_CmtPaLevel);

        MessageOutput.println("  Setting DTU serial... ");
        Hoymiles.getRadioNrf()->setDtuSerial(config.Dtu_Serial);
        Hoymiles.getRadioCmt()->setDtuSerial(config.Dtu_Serial);

        MessageOutput.println("  Setting poll interval... ");
        Hoymiles.setPollInterval(config.Dtu_PollInterval);

        for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
            if (config.Inverter[i].Serial > 0) {
                MessageOutput.print("  Adding inverter: ");
                MessageOutput.print(config.Inverter[i].Serial, HEX);
                MessageOutput.print(" - ");
                MessageOutput.print(config.Inverter[i].Name);
                auto inv = Hoymiles.addInverter(
                    config.Inverter[i].Name,
                    config.Inverter[i].Serial);

                if (inv != nullptr) {
                    inv->setReachableThreshold(config.Inverter[i].ReachableThreshold);
                    inv->setZeroValuesIfUnreachable(config.Inverter[i].ZeroRuntimeDataIfUnrechable);
                    inv->setZeroYieldDayOnMidnight(config.Inverter[i].ZeroYieldDayOnMidnight);
                    for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
                        inv->Statistics()->setStringMaxPower(c, config.Inverter[i].channel[c].MaxChannelPower);
                        inv->Statistics()->setChannelFieldOffset(TYPE_DC, static_cast<ChannelNum_t>(c), FLD_YT, config.Inverter[i].channel[c].YieldTotalOffset);
                    }
                }
                MessageOutput.println(" done");
            }
        }
        MessageOutput.println("done");
    } else {
        MessageOutput.println("Invalid pin config");
    }
}

void InverterSettingsClass::loop()
{
    if (millis() - _lastUpdate > INVERTER_UPDATE_SETTINGS_INTERVAL) {
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
