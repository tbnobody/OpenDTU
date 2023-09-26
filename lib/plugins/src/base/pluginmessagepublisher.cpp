

#include "pluginmessagepublisher.h"
#include "plugin.h"

PluginSingleQueueMessagePublisher::PluginSingleQueueMessagePublisher(
    std::vector<Plugin *> &p)
    : PluginMessagePublisher(p) {}

void PluginMessagePublisher::publish(std::shared_ptr<PluginMessage> m) {
  // :/ if no broadcast (to all)
  // we bypass queue and hand down to receiver directly
  // maybe we need a queue per plugin?
  // ... over-engineering :)
  if (m->isBroadcast())
    publishToAll(m);
  else
    publishToReceiver(m);
}

PluginMessagePublisher::PluginMessagePublisher(std::vector<Plugin *> &p)
    : plugins(p) {}

Plugin* PluginMessagePublisher::getPluginByIndex(int index) {
    if(index>=0&&index<plugins.size()) {
      return plugins[index];
    }
    return NULL;
}

Plugin *PluginMessagePublisher::getPluginById(int pluginid) {
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->getId() == pluginid) {
      return plugins[i];
    }
  }
  return NULL;
}

void PluginMessagePublisher::publishToReceiver(
    std::shared_ptr<PluginMessage> mes) {
  Plugin *p = getPluginById(mes->getReceiverId());
  if (NULL != p && p->isEnabled()) {
    p->internalCallback(mes);
  }
};

void PluginMessagePublisher::publishToAll(
    std::shared_ptr<PluginMessage> message) {
  int pluginid = message->getSenderId();
  int pcount = getPluginCount();
  PDebug.printf(PDebugLevel::DEBUG, "PluginMessagePublisher::publishToAll count:%d\n",
                  pcount);
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
    std::shared_ptr<PluginMessage> message) {
  queue.push(message);
}

void PluginSingleQueueMessagePublisher::publishToAll(
    std::shared_ptr<PluginMessage> message) {
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
