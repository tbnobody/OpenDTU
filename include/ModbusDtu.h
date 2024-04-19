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
    uint32_t _lastPublish = 0;
    bool _isstarted = false;
};

extern ModbusDtuClass ModbusDtu;