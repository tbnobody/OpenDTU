#pragma once

#include "Hoymiles.h"
#include "globals.h"
#include "pluginmessages.h"
#include "system.h"
#include <ArduinoJson.h>
#include <functional>
#include <string>
#include "PluginDebug.h"

/**
 * Plugin interface
 */


class Plugin {
public:
  Plugin(int _id, const char *_name);
  Plugin(int _id, const char *_name, bool alwayActive);
  ~Plugin() {}
  int getId();
  const char *getName();
  bool isEnabled();
  void loadPluginSettings(JsonObject s);
  void savePluginSettings(JsonObject s);
  void setSystem(System<Plugin> *s);
  /**
   * setup
   *
   * called at end of main setup
   *
   */
  virtual void setup();
  /**
   * loop
   *
   * called at end of main loop
   *
   */
  virtual void loop();

  /**
   * internalCallback
   *
   * will be called from 'system'
   *
   *  @param PluginMessage
   */
  virtual void internalCallback(std::shared_ptr<PluginMessage> message);
  /**
   * called when json message was posted to /thirdpartyplugins.
   * message must contain either 'pluginid' or 'pluginname'.
   *
   * e.g.
   *
   * {"pluginname":"someplugin","paramname":"paramvalue"}
   *
   *  @param request - JsonObject
   *  @param respone - JsonObject
   *  @return true if request was handled - false otherwise
   */
  virtual bool onRequest(JsonObject request, JsonObject response);
  /**
   * called when mqtt was connected/reconnected
   * subscribe your topics here! :)
   */
  virtual void onMqttSubscribe();
  /**
   * called when tickers should be setup
   */
  virtual void onTickerSetup();
  /**
   * @brief called when settings are loaded
   *
   * @param settings - jsonobject with plugin config
   */
  virtual void loadSettings(JsonObject settings);
  /**
   * called when settings are saved
   *
   * @param settings - jsonobject for plugin config
   */
  virtual void saveSettings(JsonObject settings);

protected:
  /**
   * subscribe mqtt topic.
   * use: onMqttSubscribe()
   *
   * @param topic
   * @param append - true if topic should be appended to DEFAULT TOPIC PREFIX -
   * false otherwise
   */
  void subscribeMqtt(char *topic, bool append);

  /**
   * @brief add timer callback.
   *
   * @param intvaltype - MINUTE / SECOND
   * @param interval
   * @param timerCb - callback function
   * @param timername
   */
  void addTimerCb(PLUGIN_TIMER_INTVAL intvaltype, uint32_t interval,
                  std::function<void(void)> timerCb, const char *timername);
  /**
   * @brief remove timer callback.
   *
   * @param timername
   */
  void removeTimerCb(const char *timername);

  /**
   * @brief publish internal message to all plugins
   *
   * @param valueid - value identifier
   * @param value
   */
  template <typename T>
  void publishMessage(T &message) {
    if (system) {
      auto m = std::make_shared<T>(message);
      system->getPublisher().publish(m);
    }
  }

private:
  int id;
  const char *name;
  System<Plugin> *system = nullptr;
  bool enabled = false;
};
