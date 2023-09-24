


#include "plugin.h"
#include "pluginmessagepublisher.h"

PluginMessagePublisher::PluginMessagePublisher(
    std::vector<Plugin *> &p,
    ThreadSafeQueue<std::shared_ptr<PluginMessage>> &q)
    : plugins(p), queue(q) {}

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
}

void PluginMessagePublisher::publishToAll(
    std::shared_ptr<PluginMessage> message) {
  // MessageOutput.printf("plugins publishToAll
  // sender=%d\n",message->getSenderId());
  int pluginid = message->getSenderId();
  for (unsigned int i = 0; i < plugins.size(); i++) {
    if (plugins[i]->getId() != pluginid) {
      if (plugins[i]->isEnabled()) {
        //                MessageOutput.printf("plugins msg sender=%d to
        //                plugin=%d\n",message->getSenderId(),plugins[i]->getId());
        plugins[i]->internalCallback(message);
      }
    }
  }
}

void PluginMessagePublisher::publishInternal() {
  while (queue.size() > 0l) {
    auto message = queue.front();
    char buffer[128];
    message.get()->toString(buffer);
    unsigned long duration = millis();
    PDebug.printf(PDebugLevel::DEBUG,"mainloop start\n----\n");
    if (message->getReceiverId() != 0) {
      publishToReceiver(message);
    } else {
      publishToAll(message);
    }
    duration -= message.get()->getTS();
    PDebug.printf(PDebugLevel::DEBUG,"----\n%s\nduration: %lu [ms]\n----\nmainloop end\n",buffer,duration);

    queue.pop();
    // do i need this? :/
    message.reset();
  }
}
