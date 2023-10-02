

#include "plugin.h"

/**
 * Plugin interface
 */

Plugin::Plugin(int _id, const char *_name) : id(_id), name(_name) {}
Plugin::Plugin(int _id, const char *_name, bool alwaysActive)
    : id(_id), name(_name), enabled(alwaysActive) {}
int Plugin::getId() { return id; }
const char *Plugin::getName() { return name; }
bool Plugin::isEnabled() { return enabled; }
void Plugin::loadPluginSettings(JsonObject s) {
  if (s.containsKey(F("enabled"))) {
    enabled = s[F("enabled")];
  }
}
void Plugin::savePluginSettings(JsonObject s) { s[F("enabled")] = enabled; }
void Plugin::setSystem(System<Plugin> *s) { system = s; }
/**
 * setup
 *
 * called at end of ahoi main setup
 *
 * @param app - pointer to ahoi settings
 */
void Plugin::setup() {}
/**
 * loop
 *
 * called at end of ahoi main loop
 *
 * @param app - pointer to ahoi app
 */
void Plugin::loop() {}

void Plugin::internalCallback(std::shared_ptr<PluginMessage> message) {}

bool Plugin::onRequest(JsonObject request, JsonObject response) {
  return false;
}

void Plugin::onMqttSubscribe() {}

void Plugin::onTickerSetup() {}

void Plugin::loadSettings(JsonObject settings) {}

void Plugin::saveSettings(JsonObject settings) {}

void Plugin::subscribeMqtt(char *topic, bool append) {
  if (system) {
    system->subscribeMqtt(this, topic, append);
  }
}

void Plugin::addTimerCb(PLUGIN_TIMER_INTVAL intvaltype, uint32_t interval,
                        std::function<void(void)> timerCb,
                        const char *timername) {
  if (system) {
    system->addTimerCb(this, timername, intvaltype, interval, timerCb);
  }
}

bool Plugin::hasTimerCb(const char *timername) {
  if (system) {
    return system->hasTimerCb(this, timername);
  }
  return false;
}

void Plugin::removeTimerCb(const char *timername) {
  if (system) {
    system->removeTimerCb(this, timername);
  }
}

bool Plugin::isSubscribed(const std::shared_ptr<PluginMessage>& m) {
  auto b = subscriptions.find(m.get()->getMessageTypeId());
  if (b != subscriptions.end())
    return b->second;
  else
    return false;
}
