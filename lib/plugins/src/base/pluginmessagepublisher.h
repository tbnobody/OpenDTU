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
    // :/ if no broadcast (to all)
    // we bypass queue and hand down to receiver directly
    // maybe we need a queue per plugin? 
    // ... over-engineering :)
    auto m = std::make_shared<T>(message);
    if (message.isBroadcast())
      queue.push(m);
    else
      publishToReceiver(m);
  }

  Plugin *getPluginById(int pluginid);

  void publishToReceiver(std::shared_ptr<PluginMessage> mes);

  void publishToAll(std::shared_ptr<PluginMessage> message);

  void publishInternal();

private:
  std::vector<Plugin *> &plugins;
  ThreadSafeQueue<std::shared_ptr<PluginMessage>> &queue;
};