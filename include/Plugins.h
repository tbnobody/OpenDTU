// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PluginSystem.hpp"
#include "base/plugin.h"
#include "base/pluginmessagepublisher.h"
#include "messages/mqttmessage.h"
#include <Every.h>
#include <ThreadSafeQueue.h>
#include <cstdint>
#include <functional>
#include <list>

class PluginConfigurator : public SystemConfigurator<Plugin> {
  public:
  PluginConfigurator() : SystemConfigurator<Plugin>() {}
  ~PluginConfigurator() {}
  void readConfig(Plugin* p);
  void saveConfig(Plugin* p);
};

class PluginsClass {
public:
  PluginsClass();
  ~PluginsClass();
  void init();
  void loop();
  void loopSystem();
  void subscribeMqtt(int id, const char *topic, bool append);
  void publishMqtt(int id, const char *topic, uint8_t *date, size_t len,
                   bool append);
  void receiveMqtt(int id, const char *topic, uint8_t *date, size_t len);

  Plugin *getPluginByIndex(int pluginindex);
  Plugin *getPluginById(int pluginid);
  Plugin *getPluginByName(const char *pluginname);
  int getPluginCount();

private:
  void addCustomPlugins();

  uint32_t _lastUpdate = 0;
  bool saveTpSettings = false;

  TaskHandle_t pluginTask;
  PluginSystem pluginSystem;
};

extern PluginsClass Plugins;