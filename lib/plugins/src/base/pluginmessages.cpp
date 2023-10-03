

#include "pluginmessages.h"
#include "plugin.h"

    PluginMessage::PluginMessage(TYPEIDS tid, int senderId_) : Entity(tid),  ts(millis()) {
        senderId = senderId_;
        receiverId = 0;
    }
    PluginMessage::PluginMessage(TYPEIDS tid, Plugin &p) : PluginMessage(tid,p.getId()) {}
    PluginMessage::PluginMessage(Plugin &p) : PluginMessage(TYPEIDS::PLUGINMESSAGE_TYPE,p.getId()) {}

int PluginMessage::toString(char* buffer) {
    return sprintf(buffer,"PluginMessage{sender:%d, receiver:%d}",senderId,receiverId);
}
