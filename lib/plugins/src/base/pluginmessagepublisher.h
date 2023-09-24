#pragma once

#include "messages/demomessage.h"
#include "messages/invertermessage.h"
#include "messages/metermessage.h"
#include "messages/mqttmessage.h"
#include <ThreadSafeQueue.h>

class PluginMessagePublisher {
public:
  PluginMessagePublisher(std::vector<Plugin *> &p,
                         ThreadSafeQueue<std::shared_ptr<PluginMessage>> &q);
  virtual ~PluginMessagePublisher() {}
  template <typename T> void publish(T &message) {
    queue.push(std::make_shared<T>(message));
  }

  Plugin* getPluginById(int pluginid);

  void publishToReceiver(std::shared_ptr<PluginMessage> mes);

  void publishToAll(std::shared_ptr<PluginMessage> message);

  void publishInternal();
  private:
  std::vector<Plugin *> &plugins;
  ThreadSafeQueue<std::shared_ptr<PluginMessage>> &queue;
};