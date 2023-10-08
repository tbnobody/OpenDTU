
#include "base/plugin.h"
#include "base/pluginmessagepublisher.h"
#include "base/pluginmessages.h"
#include "messages/limitcontrolmessage.h"
#include "messages/limitmessage.h"
#include "messages/metermessage.h"
#include "messages/powercontrolmessage.h"
#include "messages/powermessage.h"

#include "base/system.h"

#ifndef UNITY_INCLUDE_PRINT_FORMATTED
#define UNITY_INCLUDE_PRINT_FORMATTED
#endif

#include "unity.h"

class PublisherMock : public PluginMessagePublisher {
public:
  PublisherMock(std::vector<std::unique_ptr<Plugin>> &p)
      : PluginMessagePublisher(p), cb(nullptr) {}
  ~PublisherMock() {}
  std::function<void(const std::shared_ptr<PluginMessage> p)> cb;

protected:
  void publishTo(int pluginId, const std::shared_ptr<PluginMessage> &mes) {
    TEST_MESSAGE("publishTo");
  }
  void publishToReceiver(const std::shared_ptr<PluginMessage> &mes) {
    TEST_MESSAGE("publishToReceiver");
    if (cb != nullptr)
      cb(mes);
  }

  void publishToAll(const std::shared_ptr<PluginMessage> &message) {
    TEST_MESSAGE("publishToAll");
    if (cb != nullptr)
      cb(message);
  }
};

class PluginMock : public Plugin {
public:
  PluginMock() : Plugin(9999999, "pluginmock") { TEST_MESSAGE("PluginMock()"); }
};

class ConfiguratorMock : public SystemConfigurator<Plugin> {
public:
  std::function<void(Plugin *p)> readCB = nullptr;
  std::function<void(Plugin *p)> saveCB = nullptr;
  void readConfig(Plugin *t) {
    if (readCB)
      readCB(t);
  }
  void saveConfig(Plugin *t) {
    if (saveCB)
      saveCB(t);
  }
};

class SystemMock : public System<Plugin> {
public:
  SystemMock() : System<Plugin>(), publisher(plugins) {}
  ~SystemMock() {}
  void setConfigurator(SystemConfigurator<Plugin> &configurator) {}
  void readPluginConfig(Plugin *p, JsonObject &config) {
    p->loadPluginSettings(config);
    p->loadSettings(config);
    TEST_PRINTF("plugin %s config loaded!\n", p->getName());
  }

  void initPlugin(Plugin *p, JsonObject &config) {
    p->setSystem(this);
    readPluginConfig(p, config);
    p->setup();
    p->onTickerSetup();
    p->onMqttSubscribe();
    TEST_PRINTF("plugin %s initialized!\n", p->getName());
  }
  template <typename T>
  std::shared_ptr<Plugin> createPlugin(T plugin, JsonObject &config) {
    std::shared_ptr<Plugin> p = std::make_shared<T>(plugin);
    initPlugin(p.get(), config);
    return p;
  }

  Plugin *getMockPlugin() { return &pluginMock; }
  std::shared_ptr<PowerMessage> createPowerMessage(float f) {
    PowerMessage m(pluginMock);
    m.deviceId = "inverterSerial";
    m.value = f;
    return std::make_shared<PowerMessage>(m);
  }
  std::shared_ptr<MeterMessage> createMeterMessage(float f) {
    MeterMessage m(pluginMock);
    m.serial = "meterSerial";
    m.power = f;
    return std::make_shared<MeterMessage>(m);
  }
  std::function<void(const std::shared_ptr<PluginMessage> p)> cb;
  void receiveMqtt(Plugin *plugin, const char *topic, const uint8_t *data,
                   size_t len) {}
  void subscribeMqtt(Plugin *plugin, char *topic, bool append) {}
  PluginMessagePublisher &getPublisher() {
    publisher.cb = cb;
    return publisher;
  }
  virtual void addTimerCb(Plugin *plugin, const char *timername,
                          PLUGIN_TIMER_INTVAL intval, uint32_t interval,
                          std::function<void(void)> timerCb) {}
  virtual bool hasTimerCb(Plugin *plugin, const char *timername) {
    return false;
  }
  virtual void removeTimerCb(Plugin *plugin, const char *timername) {}
  Plugin *getPluginById(int pluginid) { return NULL; }
  Plugin *getPluginByName(const char *pluginname) { return NULL; }
  int getPluginCount() { return 0; }
  PublisherMock publisher;
  PluginMock pluginMock;

private:
  std::vector<std::unique_ptr<Plugin>> plugins;
};