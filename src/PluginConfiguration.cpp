// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Thomas Basler and others
 */
#include "PluginConfiguration.h"
#include "MessageOutput.h"
#include "defaults.h"
#include <LittleFS.h>

void PluginConfigurationClass::init()
{

}
void PluginConfigurationClass::printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      PDebug.printf(PDebugLevel::DEBUG,"\t");
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      PDebug.printf(PDebugLevel::DEBUG,"/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      PDebug.printf(PDebugLevel::DEBUG,"\t\t%d\n",entry.size());
    }
    entry.close();
  }
}
void PluginConfigurationClass::debug() {
    File dir = LittleFS.open("/");
	// Cycle all the content
	printDirectory(dir);
}

void PluginConfigurationClass::reset(Plugin* p) {
    char path[32];
    snprintf(path,sizeof(path),"/%s",p->getName());
    LittleFS.remove(path);
}
bool PluginConfigurationClass::writeTo(JsonObject o, Plugin* p)
{
    p->saveSettings(o);
    p->savePluginSettings(o);
    return true;
}

bool PluginConfigurationClass::write(Plugin* p)
{
    DynamicJsonDocument doc(PLUGIN_JSON_BUFFER_SIZE);
    JsonObject root = doc.createNestedObject(p->getName());
    writeTo(root,p);
    char path[32];
    snprintf(path,sizeof(path),"/%s",p->getName());
    File f = LittleFS.open(path, "w", true);
    if (!f) {
        return false;
    }

    // Serialize JSON to file
    if (serializeJson(root, f) == 0) {
        PDebug.printf(PDebugLevel::DEBUG,"Failed to write file %s\n",p->getName());
        return false;
    }

    f.close();
    return true;
}

bool PluginConfigurationClass::readFrom(JsonObject o, Plugin* p) {
    p->loadPluginSettings(o);
    p->loadSettings(o);
    return true;
}

bool PluginConfigurationClass::read(Plugin* p)
{
    char path[32];
    snprintf(path,sizeof(path),"/%s",p->getName());
    File f = LittleFS.open(path, "r", false);
    DynamicJsonDocument doc(PLUGIN_JSON_BUFFER_SIZE);
    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, f);
    if (error) {
        PDebug.printf(PDebugLevel::DEBUG,"Failed to read file, %s, using default configuration\n",p->getName());
    } else {
        JsonObject o = doc.as<JsonObject>();
        readFrom(o,p);
    }
    f.close();
    return true;
}

PluginConfigurationClass PluginConfiguration;