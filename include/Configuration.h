#pragma once

#include <Arduino.h>

#define CONFIG_FILENAME "/config.bin"
#define CONFIG_VERSION 0x00010000 // 0.1.0

struct CONFIG_T {
    uint32_t Cfg_Version;
    uint Cfg_SaveCount;
};

class ConfigurationClass {
public:
    void init();
    bool read();
    bool write();
    void migrate();
    CONFIG_T& get();
};

extern ConfigurationClass Configuration;