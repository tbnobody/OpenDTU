

#include "pluginmessagepublisher.h"
#include "plugin.h"

PluginSingleQueueMessagePublisher::PluginSingleQueueMessagePublisher(
    std::vector<std::unique_ptr<Plugin>> &p)
    : PluginMessagePublisher(p) {}

void PluginMessagePublisher::publish(const std::shared_ptr<PluginMessage> &m) {
  if (m->isBroadcast())
    publishToAll(m);
  else
    publishToReceiver(std::move(m));
}

PluginMessagePublisher::PluginMessagePublisher(
    std::vector<std::unique_ptr<Plugin>> &p)
    : plugins(p) {}

Plugin *PluginMessagePublisher::getPluginByIndex(int index) {
  if (index >= 0 && index < plugins.size()) {
    return plugins[index].get();
  }
  return NULL;
}

Plugin *PluginMessagePublisher::getPluginById(int pluginid) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->getId() == pluginid) {
      return plugins[i].get();
    }
  }
  return NULL;
}

void PluginMessagePublisher::publishToReceiver(
    const std::shared_ptr<PluginMessage> &mes) {
  Plugin *p = getPluginById(mes->getReceiverId());
  if (NULL != p && p->isEnabled()) {
    p->internalCallback(mes);
  }
};

void PluginMessagePublisher::publishToAll(
    const std::shared_ptr<PluginMessage> &message) {
  int pluginid = message->getSenderId();
  int pcount = getPluginCount();
  PDebug.printf(PDebugLevel::DEBUG,
                "PluginMessagePublisher::publishToAll count:%d\n", pcount);
  for (unsigned int i = 0; i < pcount; i++) {
    Plugin *p = getPluginByIndex(i);
    if (p->getId() != pluginid) {
      if (p->isEnabled()) {
        p->internalCallback(message);
      }
    }
  }
};

void PluginSingleQueueMessagePublisher::publishToReceiver(
    const std::shared_ptr<PluginMessage> &message) {
  queue.push(message);
}

void PluginSingleQueueMessagePublisher::publishToAll(
    const std::shared_ptr<PluginMessage> &message) {
  queue.push(message);
}

void PluginSingleQueueMessagePublisher::loop() {
  while (queue.size() > 0l) {
    auto message = queue.front();
    char buffer[128];
    message.get()->toString(buffer);
    unsigned long duration = millis();
    PDebug.printf(PDebugLevel::DEBUG, "mainloop start\n----\n%s\n----\n",
                  buffer);
    if (message->getReceiverId() != 0) {
      PluginMessagePublisher::publishToReceiver(message);
    } else {
      PluginMessagePublisher::publishToAll(message);
    }
    duration -= message.get()->getTS();
    PDebug.printf(PDebugLevel::DEBUG,
                  "----\n%s\nduration: %lu [ms]\n----\nmainloop end\n", buffer,
                  duration);

    queue.pop();
    // do i need this? :/
    message.reset();
  }
}

PluginMultiQueueMessagePublisher::PluginMultiQueueMessagePublisher(
    std::vector<std::unique_ptr<Plugin>> &p)
    : PluginMessagePublisher(p) {}

void PluginMultiQueueMessagePublisher::publishTo(
    int pluginId, const std::shared_ptr<PluginMessage> &message) {
  if (!(queues.find(pluginId) != queues.end())) {
    queues.insert(
        {pluginId,
         std::make_shared<ThreadSafeQueue<std::shared_ptr<PluginMessage>>>(
             ThreadSafeQueue<std::shared_ptr<PluginMessage>>())});
  }
  queues.at(pluginId)->push(message);
}
void PluginMultiQueueMessagePublisher::publishToReceiver(
    const std::shared_ptr<PluginMessage> &message) {
  publishTo(message->getReceiverId(), message);
}

void PluginMultiQueueMessagePublisher::publishToAll(
    const std::shared_ptr<PluginMessage> &message) {
  for (int i = 0; i < getPluginCount(); i++) {
    publishTo(getPluginByIndex(i)->getId(), message);
  }
}

void PluginMultiQueueMessagePublisher::loop() {
  bool hasMsg = false;
  for (auto &pair : queues) {
    auto queue = pair.second;
    if (queue.get()->size() > 0l)
      hasMsg = true;
    break;
  }
  if (hasMsg)
    PDebug.printf(PDebugLevel::DEBUG, "mainloop start\n----\n");
  unsigned long mainduration = millis();
  for (auto &pair : queues) {
    auto queue = pair.second;
    while (queue.get()->size() > 0l) {
      auto message = queue.get()->front();
      char buffer[128];
      message.get()->toString(buffer);
      unsigned long duration = millis();
      getPluginById(pair.first)->internalCallback(message);
      duration -= message.get()->getTS();
      PDebug.printf(PDebugLevel::DEBUG, "pluginqueue %lu [ms] - %s\n", duration,
                    buffer);

      queue->pop();
      // do i need this? :/
      message.reset();
      yield();
    }
  }
  if (hasMsg) {
    mainduration = millis() - mainduration;
    PDebug.printf(PDebugLevel::DEBUG, "mainloop stop - %lu [ms]\n----\n",
                  mainduration);
  }
}
