


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

    //DBGPRINTMESSAGEFROMTO(DBG_INFO,"mainloop start", message);
    if (message->getReceiverId() != 0) {
      publishToReceiver(message);
    } else {
      publishToAll(message);
    }
    //DBGPRINTMESSAGEDURATION(DBG_INFO,"mainloop end", message);

    queue.pop();
    // do i need this? :/
    message.reset();
  }
}
