
#include "Plugins.h"
#include "MessageOutput.h"
#include "MqttSettings.h"
#include "PluginConfiguration.h"
#include "string.h"

#if __has_include("customplugin.h")
#include "customplugin.h"
#endif

#include "PluginSystem.hpp"

PluginsClass Plugins;
PluginConfigurator configurator;

void PluginConfigurator::readConfig(Plugin *p) { PluginConfiguration.read(p); }
void PluginConfigurator::saveConfig(Plugin *p) { PluginConfiguration.write(p); }

PluginsClass::PluginsClass() : pluginSystem(PluginSystem(configurator)) {}

PluginsClass::~PluginsClass() {}

void PluginsClass::subscribeMqtt(int id, const char *topic, bool append) {
  //   PDebug.printf(PDebugLevel::DEBUG,"PluginsClass::subscribeMqtt %s: %s\n",
  //   plugin->name, topic);
  MqttSettings.subscribe(
      topic, 0,
      [&, id, append](const espMqttClientTypes::MessageProperties &properties,
                      const char *topic, const uint8_t *payload, size_t len,
                      size_t index, size_t total) {
        //       PDebug.printf(PDebugLevel::DEBUG,"PluginsClass::mqttCb
        //       topic=%s\n", topic);
        pluginSystem.receiveMqtt(pluginSystem.getPluginById(id), topic, payload,
                                 len);
      });
}

Plugin *PluginsClass::getPluginByIndex(int pluginindex) {
  return pluginSystem.getPluginByIndex(pluginindex);
}

Plugin *PluginsClass::getPluginById(int pluginid) {
  return pluginSystem.getPluginById(pluginid);
}

Plugin *PluginsClass::getPluginByName(const char *pluginname) {
  return pluginSystem.getPluginByName(pluginname);
}

int PluginsClass::getPluginCount() { return pluginSystem.getPluginCount(); }

void PluginsClass::publishMqtt(int id, const char *topic, uint8_t *data,
                               size_t len, bool append) {
  // we dont care about real topic length, one size fit's all ;)
  //   char topic[128];
  if (!MqttSettings.getConnected()) {
    PDebug.printf(PDebugLevel::WARN,
                  "PluginsClass: mqtt not connected. can not send message!");
    return;
  }
  PDebug.printf(PDebugLevel::DEBUG, "PluginsClass: publish mqtt nmessage!");

  MqttSettings.publishGeneric(topic, (const char *)data, append, 0);
}

void PluginsClass::loopSystem() { pluginSystem.loop(); }

void PluginsClass::loop() {
#ifndef ESPSINGLECORE
#else
  loopSystem();
#endif
}

#ifndef ESPSINGLECORE
extern "C" {
void pluginTaskFunc(void *parameter);
}

void pluginTaskFunc(void *parameter) {
  for (;;) {
    Plugins.loopSystem();
  }
}
#endif

void PluginsClass::addCustomPlugins() {
  // add additional plugins here
  // plugins.push_back(std::make_unique<{PluginClassName}>({PluginClassName}()));
}

void PluginsClass::init() {
  pluginSystem.setMqttPublishCb(
      std::bind(&PluginsClass::publishMqtt, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5));
  pluginSystem.setMqttSubscribeCb(
      std::bind(&PluginsClass::subscribeMqtt, this, std::placeholders::_1,
                std::placeholders::_2, std::placeholders::_3));
  MessageOutput.setLevel(MessageOutputDebugLevel::DEBUG_INFO);
  PDebug.setPrint(&MessageOutput);
  pluginSystem.init();

#ifndef ESPSINGLECORE
  xTaskCreatePinnedToCore(pluginTaskFunc, /* Function to implement the task */
                          "pluginTask",   /* Name of the task */
                          10000,          /* Stack size in words */
                          NULL,           /* Task input parameter */
                          0,              /* Priority of the task */
                          &pluginTask,    /* Task handle. */
                          0);             /* Core where the task should run */
#endif
  PDebug.printf(PDebugLevel::INFO, "PluginsClass::init\n");
}
