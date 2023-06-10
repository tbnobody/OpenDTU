// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "plugin.h"

#define PLUGINCONFIG_FILENAME "/pluginconfig.json"
#define PLUGINCONFIG_VERSION 0x00011900 // 0.1.24 // make sure to clean all after change

#define MAX_PARAMETERX_STRLEN 32

#define PLUGIN_JSON_BUFFER_SIZE 4096

class PluginConfigurationClass {
public:
    void init();
    bool read(Plugin* p);
    bool write(Plugin* p);
    void reset(Plugin* p);
    bool writeTo(JsonObject o, Plugin* p);
    bool readFrom(JsonObject o, Plugin* p);
    void debug();
private:
    void printDirectory(File dir, int numTabs=3);
};

extern PluginConfigurationClass PluginConfiguration;