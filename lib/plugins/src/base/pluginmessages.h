#pragma once

#include "base/plugintypes.h"
#if __has_include("base/pluginids.h")
#include "base/pluginids.h"
#endif

class Plugin;

enum class METADATA_TAGS { EMPTY_TAG, SENDERID, RECEIVERID, MSGTS, TESTTAG };
class MetaData : public ContainerMap<METADATA_TAGS, Entity> {

public:
  int getSenderId() {
    return getValueAs<IntValue>(METADATA_TAGS::SENDERID).value;
  }
  int getReceiverId() {
    return getValueAs<IntValue>(METADATA_TAGS::RECEIVERID).value;
  }
  template <typename T> T getTagAs(METADATA_TAGS id) {
    return getValueAs<T>(id);
  }
  bool hasTag(METADATA_TAGS id) { return hasKey(id); }
  void setReceiverId(int id) {
    add(METADATA_TAGS::RECEIVERID, IntValue(0, id));
  }

protected:
  void setSenderId(int id) { add(METADATA_TAGS::SENDERID, IntValue(0, id)); }
  template <typename T> void addTag(METADATA_TAGS id, T tag) { add(id, tag); }

  friend class PluginMessage;
};

class EntityError {};

class PluginMessage : public Entity {

public:
  PluginMessage(Plugin &p);
  PluginMessage(TYPEIDS tid, int senderId);
  PluginMessage(TYPEIDS tid, Plugin &p);

  bool from(int senderid) { return (this->senderId == senderid); }
  int getSenderId() { return senderId; }
  int getReceiverId() { return receiverId; }
  void setReceiverId(int id) { receiverId = id; }
  bool isBroadcast() { return (receiverId == 0); }
  unsigned long getTS() { return ts; }
  virtual int toString(char *buffer);
  template <typename U> bool isMessageType() {
    return (EntityIds<U>::type_id == type_id);
  }
  template <typename U> U &getMessageAs() {
    auto v = std::static_pointer_cast<U>(this);
    return *v.get();
  }
  virtual const char* getMessageTypeString() {
    return "PluginMessage";
  }

protected:
  void setSenderId(int id) { senderId = id; }

protected:
  int senderId = 0;
  int receiverId = 0;
  unsigned long ts = millis();
};
