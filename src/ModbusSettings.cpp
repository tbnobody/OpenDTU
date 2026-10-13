// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024..2026 Bobby Noelte
 */

#include <Print.h>
#include <cstdarg>

 // OpenDTU
#include "Configuration.h"
#include "MessageOutput.h"
#include "ModbusDtu.h"
#include "ModbusSettings.h"

// eModbus
#include "ModbusLogging.h"

namespace {

int modbusLogPrintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = MessageOutputClass::log_vprintf(fmt, args);
    va_end(args);
    return ret;
}

class ModbusLogPrint : public Print {
public:
    size_t write(uint8_t c) override
    {
        return write(&c, 1);
    }

    size_t write(const uint8_t* buf, size_t len) override
    {
        modbusLogPrintf("%.*s", static_cast<int>(len), reinterpret_cast<const char*>(buf));
        return len;
    }
};

ModbusLogPrint modbusLog;

} // namespace


ModbusSettingsClass::ModbusSettingsClass()
{
}

void ModbusSettingsClass::init()
{
    // Set Modbus logging to OpenDTU MessageOutput
    LOGDEVICE = &modbusLog;

    // Start server(s) if enabled
    performConfig();
}

// Start server(s)
void ModbusSettingsClass::startTCP()
{
    const CONFIG_T& config = Configuration.get();

    if (!ModbusTCPServer.isRunning()) {
        // Define server(s)
        ModbusTCPServer.registerWorker(config.Modbus.IDDTUPro, READ_HOLD_REGISTER, &DTUPro);
        ModbusTCPServer.registerWorker(config.Modbus.IDTotal, READ_HOLD_REGISTER, &OpenDTUTotal);
        ModbusTCPServer.registerWorker(config.Modbus.IDMeter, READ_HOLD_REGISTER, &OpenDTUMeter);

        // Start
        ModbusTCPServer.start(Configuration.get().Modbus.Port, config.Modbus.Clients, 20000);
    }
}

// Stop servers(s)
void ModbusSettingsClass::stopTCP()
{
    const CONFIG_T& config = Configuration.get();

    if (ModbusTCPServer.isRunning()) {
        ModbusTCPServer.stop();
    }

    ModbusTCPServer.unregisterWorker(config.Modbus.IDDTUPro);
    ModbusTCPServer.unregisterWorker(config.Modbus.IDTotal);
    ModbusTCPServer.unregisterWorker(config.Modbus.IDMeter);
}

void ModbusSettingsClass::performConfig()
{
   // Force stop of all servers
    stopTCP();

    // (Re-)start servers if enabled
    if (Configuration.get().Modbus.TCPEnabled) {
        startTCP();
    }
}

ModbusSettingsClass ModbusSettings;
