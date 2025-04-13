// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2025 Thomas Basler and others
 */
#include "InverterSettings.h"
#include "Configuration.h"
#include "MessageOutput.h"
#include "PinMapping.h"
#include "SunPosition.h"
#include <Hoymiles.h>
#include <SpiManager.h>

InverterSettingsClass InverterSettings;

InverterSettingsClass::InverterSettingsClass()
    : _settingsTask(INVERTER_UPDATE_SETTINGS_INTERVAL, TASK_FOREVER, std::bind(&InverterSettingsClass::settingsLoop, this))
    , _hoyTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&InverterSettingsClass::hoyLoop, this))
{
}

void InverterSettingsClass::init(Scheduler& scheduler)
{
    const CONFIG_T& config = Configuration.get();
    const PinMapping_t& pin = PinMapping.get();

    // Initialize inverter communication
    MessageOutput.printf("Initialize Hoymiles interface...\r\n");
    Hoymiles.setMessageOutput(&MessageOutput);
    Hoymiles.init();

    if (!PinMapping.isValidNrf24Config() && !PinMapping.isValidCmt2300Config()) {
        MessageOutput.printf("Invalid pin config\r\n");
        return;
    }

    // Initialize NRF24 if configured
    if (PinMapping.isValidNrf24Config()) {
        MessageOutput.printf("NRF: Initialize communication\r\n");
        auto spi_bus = SpiManagerInst.claim_bus_arduino();
        ESP_ERROR_CHECK(spi_bus ? ESP_OK : ESP_FAIL);

        SPIClass* spiClass = new SPIClass(*spi_bus);
        spiClass->begin(pin.nrf24_clk, pin.nrf24_miso, pin.nrf24_mosi, pin.nrf24_cs);
        Hoymiles.initNRF(spiClass, pin.nrf24_en, pin.nrf24_irq);
    }

    // Initialize CMT2300 if configured
    if (PinMapping.isValidCmt2300Config()) {
        MessageOutput.printf("CMT2300A: Initialize communication\r\n");
        Hoymiles.initCMT(pin.cmt_sdio, pin.cmt_clk, pin.cmt_cs, pin.cmt_fcs, pin.cmt_gpio2, pin.cmt_gpio3);
        MessageOutput.printf("CMT2300A: Setting country mode...\r\n");
        Hoymiles.getRadioCmt()->setCountryMode(static_cast<CountryModeId_t>(config.Dtu.Cmt.CountryMode));
        MessageOutput.printf("CMT2300A: Setting CMT target frequency...\r\n");
        Hoymiles.getRadioCmt()->setInverterTargetFrequency(config.Dtu.Cmt.Frequency);
    }

    // Configure common radio settings
    MessageOutput.printf("RF: Setting radio PA level...\r\n");
    Hoymiles.getRadioNrf()->setPALevel((rf24_pa_dbm_e)config.Dtu.Nrf.PaLevel);
    Hoymiles.getRadioCmt()->setPALevel(config.Dtu.Cmt.PaLevel);

    MessageOutput.printf("RF: Setting DTU serial...\r\n");
    Hoymiles.getRadioNrf()->setDtuSerial(config.Dtu.Serial);
    Hoymiles.getRadioCmt()->setDtuSerial(config.Dtu.Serial);

    MessageOutput.printf("RF: Setting poll interval...\r\n");
    Hoymiles.setPollInterval(config.Dtu.PollInterval);

    // Configure inverters
    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        const auto& inv_cfg = config.Inverter[i];
        if (inv_cfg.Serial == 0) {
            continue;
        }

        MessageOutput.printf("Adding inverter: %0" PRIx32 "%08" PRIx32 " - %s\r\n",
            static_cast<uint32_t>((inv_cfg.Serial >> 32) & 0xFFFFFFFF),
            static_cast<uint32_t>(inv_cfg.Serial & 0xFFFFFFFF),
            inv_cfg.Name);

        auto inv = Hoymiles.addInverter(inv_cfg.Name, inv_cfg.Serial);
        if (inv == nullptr) {
            MessageOutput.printf("Adding inverter failed: Unsupported type\r\n");
            continue;
        }

        inv->setReachableThreshold(inv_cfg.ReachableThreshold);
        inv->setZeroValuesIfUnreachable(inv_cfg.ZeroRuntimeDataIfUnrechable);
        inv->setZeroYieldDayOnMidnight(inv_cfg.ZeroYieldDayOnMidnight);
        inv->setClearEventlogOnMidnight(inv_cfg.ClearEventlogOnMidnight);
        inv->Statistics()->setYieldDayCorrection(inv_cfg.YieldDayCorrection);
        for (uint8_t c = 0; c < INV_MAX_CHAN_COUNT; c++) {
            inv->Statistics()->setStringMaxPower(c, inv_cfg.channel[c].MaxChannelPower);
            inv->Statistics()->setChannelFieldOffset(TYPE_DC, static_cast<ChannelNum_t>(c), FLD_YT, inv_cfg.channel[c].YieldTotalOffset);
        }

        MessageOutput.printf("Adding complete\r\n");
    }
    MessageOutput.printf("Initialization complete\r\n");

    scheduler.addTask(_hoyTask);
    _hoyTask.enable();

    scheduler.addTask(_settingsTask);
    _settingsTask.enable();
}

void InverterSettingsClass::settingsLoop()
{
    const CONFIG_T& config = Configuration.get();
    const bool isDayPeriod = SunPosition.isDayPeriod();

    for (uint8_t i = 0; i < INV_MAX_COUNT; i++) {
        auto const& inv_cfg = config.Inverter[i];
        if (inv_cfg.Serial == 0) {
            continue;
        }
        auto inv = Hoymiles.getInverterBySerial(inv_cfg.Serial);
        if (inv == nullptr) {
            continue;
        }

        inv->setEnablePolling(inv_cfg.Poll_Enable && (isDayPeriod || inv_cfg.Poll_Enable_Night));
        inv->setEnableCommands(inv_cfg.Command_Enable && (isDayPeriod || inv_cfg.Command_Enable_Night));
    }
}

void InverterSettingsClass::hoyLoop()
{
    Hoymiles.loop();
}
