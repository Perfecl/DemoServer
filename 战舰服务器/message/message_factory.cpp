#include "message_factory.h"
#include <boost/unordered_map.hpp>
#include <assert.h>

typedef boost::unordered_map<uint32_t, MessageFactory::Message*> ContainerType;
static ContainerType map_;

MessageFactory::MessageFactory()
{
  this->Init();
}

MessageFactory::Message* MessageFactory::GetPrototype(uint32_t msgid) {
  ContainerType::iterator iter = map_.find(msgid);
  if (iter != map_.end()) {
    return iter->second->New();
  }
  return NULL;
}

void MessageFactory::RegisterMessage(uint32_t msgid, MessageFactory::Message* message)
{
#ifdef _DEBUG
  if (map_.find(msgid) != map_.end())
  {
    assert(false && "Message Exist");
  }
#endif
  map_[msgid] = message;
}
