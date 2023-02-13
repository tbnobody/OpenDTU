#pragma once

#include <cstdint>
#include "Configuration.h"
#include <ModbusIP_ESP8266.h>
#include <Hoymiles.h>

class ModbusDtuClass {
public:
    void init();
    void loop();

private:
    uint32_t _lastPublish;
    uint8_t _channels;
};

extern ModbusDtuClass ModbusDtu;