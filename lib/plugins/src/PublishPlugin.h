#pragma once

#include "base/plugin.h"
#include "messages/mqttmessage.h"
#include <functional>

class PublishPlugin : public Plugin {

public:
  PublishPlugin() : Plugin(100, "publish", true) {}

  void mqttMessageCB(std::function<void(MqttMessage*)> callback) {
      cb = callback;
  }
  void internalCallback(std::shared_ptr<PluginMessage> message) {

    //DBGPRINTMESSAGELNCB(DBG_INFO, getName(), message);
    if (message->isMessageType<MqttMessage>()) {
      MqttMessage *m = (MqttMessage *)message.get();
      MessageOutput.printf("publishplugin %s = %s\n",
                         m->topic.get(),m->payloadToChar().get());
      if(cb!=NULL) {
        cb(m);
      }
    }
  }
  private:
  std::function<void(MqttMessage*)> cb = NULL;
};
