#pragma once

#include "messages/invertermessage.h"
#include "messages/metermessage.h"
#include "messages/mqttmessage.h"
#include <ThreadSafeQueue.h>
#include <map>

class PluginMessagePublisher {
public:
  PluginMessagePublisher(std::vector<std::unique_ptr<Plugin>> &p);
  virtual ~PluginMessagePublisher() {}

  void publish(const std::shared_ptr<PluginMessage> &message);

  virtual void loop(){}

protected:
  Plugin *getPluginById(int pluginid);
  Plugin *getPluginByIndex(int index);

  int getPluginCount() { return plugins.size(); }

  virtual void publishTo(int pluginId,const std::shared_ptr<PluginMessage> &mes);

  virtual void publishToReceiver(const std::shared_ptr<PluginMessage> &mes);

  virtual void publishToAll(const std::shared_ptr<PluginMessage> &message);

private:
  std::vector<std::unique_ptr<Plugin>> &plugins;
};

class PluginSingleQueueMessagePublisher : public PluginMessagePublisher {
public:
  PluginSingleQueueMessagePublisher(std::vector<std::unique_ptr<Plugin>> &p);
  virtual ~PluginSingleQueueMessagePublisher() {}

  void loop();

protected:
  virtual void publishTo(int pluginId,const std::shared_ptr<PluginMessage> &mes);
  virtual void publishToReceiver(const std::shared_ptr<PluginMessage> &mes);

  virtual void publishToAll(const std::shared_ptr<PluginMessage> &message);

private:
  ThreadSafeQueue<std::shared_ptr<PluginMessage>> queue;
};

class PluginMultiQueueMessagePublisher : public PluginMessagePublisher {
public:
  PluginMultiQueueMessagePublisher(std::vector<std::unique_ptr<Plugin>> &p, bool subscriptionForced_=true);
  virtual ~PluginMultiQueueMessagePublisher() {}

  void loop();

protected:
  virtual void publishToReceiver(const std::shared_ptr<PluginMessage> &mes);
  virtual void publishToAll(const std::shared_ptr<PluginMessage> &message);
  virtual void publishTo(int pluginId,const std::shared_ptr<PluginMessage> &message);
    
private:
  bool subscriptionForced;
  std::map<int,std::shared_ptr<ThreadSafeQueue<std::shared_ptr<PluginMessage>>>> queues;
};