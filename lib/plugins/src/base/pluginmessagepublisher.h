#pragma once

#include "messages/demomessage.h"
#include "messages/invertermessage.h"
#include "messages/metermessage.h"
#include "messages/mqttmessage.h"
#include <ThreadSafeQueue.h>

class PluginMessagePublisher {
public:
  PluginMessagePublisher(std::vector<Plugin *> &p);
  virtual ~PluginMessagePublisher() {}

  void publish(std::shared_ptr<PluginMessage> message);

  virtual void loop(){}

protected:
  Plugin *getPluginById(int pluginid);
Plugin *getPluginByIndex(int index);

  int getPluginCount() { return plugins.size(); }

  virtual void publishToReceiver(std::shared_ptr<PluginMessage> mes);

  virtual void publishToAll(std::shared_ptr<PluginMessage> message);

private:
  std::vector<Plugin *> &plugins;
};

class PluginSingleQueueMessagePublisher : public PluginMessagePublisher {
public:
  PluginSingleQueueMessagePublisher(std::vector<Plugin *> &p);
  virtual ~PluginSingleQueueMessagePublisher() {}

  void loop();

protected:
  virtual void publishToReceiver(std::shared_ptr<PluginMessage> mes);

  virtual void publishToAll(std::shared_ptr<PluginMessage> message);

private:
  ThreadSafeQueue<std::shared_ptr<PluginMessage>> queue;
};