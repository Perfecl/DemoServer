#pragma once
#include <stdint.h>
#include <singleton.h>
#include <google/protobuf/message.h>

class MessageFactory : public Singleton<MessageFactory> {
 public:
  typedef google::protobuf::Message Message;
  MessageFactory();

  Message* GetPrototype(uint32_t msgid);
  void RegisterMessage(uint32_t id, Message* message);

 private:
  void Init();
};
