#include "Configuration.h"
#include <LittleFS.h>

CONFIG_T config;

void ConfigurationClass::init()
{
    memset(&config, 0x0, sizeof(config));
    config.Cfg_SaveCount = 0;
    config.Cfg_Version = CONFIG_VERSION;
}

bool ConfigurationClass::write()
{
    File f = LITTLEFS.open(CONFIG_FILENAME, "w");
    if (!f) {
        return false;
    }
    config.Cfg_SaveCount++;
    uint8_t* bytes = (uint8_t*)&config;
    for (unsigned int i = 0; i < sizeof(CONFIG_T); i++) {
        f.write(bytes[i]);
    }
    f.close();
    return true;
}

bool ConfigurationClass::read()
{
    File f = LITTLEFS.open(CONFIG_FILENAME, "r");
    if (!f) {
        return false;
    }
    uint8_t* bytes = (uint8_t*)&config;
    for (unsigned int i = 0; i < sizeof(CONFIG_T); i++) {
        bytes[i] = f.read();
    }
    f.close();
    return true;
}

void ConfigurationClass::migrate()
{
    config.Cfg_Version = CONFIG_VERSION;
    write();
}

CONFIG_T& ConfigurationClass::get()
{
    return config;
}

ConfigurationClass Configuration;