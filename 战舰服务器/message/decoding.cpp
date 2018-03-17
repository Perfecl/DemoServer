#include "decoding.h"
#include "message_factory.h"

Message* TryDecodeCSMessage(const char* buffer, uint32_t length,
                            __OUT__ CSHead& head, uint32_t& consume_bytes) {
  consume_bytes = 0;
  if (length < CSHEAD_LENGTH) return NULL;

  head = *(CSHead*)(buffer);

  const char* message_begin = buffer + CSHEAD_LENGTH;
  uint32_t message_length = head.real_length();

  if (message_length >= CS_MSG_MAX_LEN)
    return NULL;
  if (message_length + CSHEAD_LENGTH > length)
    return NULL;

  consume_bytes = message_length + CSHEAD_LENGTH;
  Message* message = MessageFactory::Instance().GetPrototype(head.msgid);
  if (!message) return NULL;

  bool result = message->ParseFromArray(message_begin, message_length);
  //解码失败
  if (!result) {
    delete message;
    return NULL;
  }
  return message;
}

Message* TryDecodeSSMessage(const char* buffer, uint32_t length,
                            __OUT__ SSHead& head, uint32_t& consume_bytes) {
  consume_bytes = 0;
  if (length < SSHEAD_LENGTH) return NULL;

  head = *(SSHead*)(buffer);

  const char* message_begin = buffer + SSHEAD_LENGTH;
  uint32_t message_length = head.real_length();

  if (message_length + SSHEAD_LENGTH > length)
    return NULL;

  consume_bytes = message_length + SSHEAD_LENGTH;
  Message* message = MessageFactory::Instance().GetPrototype(head.msgid);
  if (!message) {
    return NULL;
  }
  bool result = message->ParseFromArray(message_begin, message_length);
  //解码失败
  if (!result) {
#ifdef __INTERNAL_DEBUG
    assert(false && "解码失败");
#endif
    delete message;
    return NULL;
  }
  return message;
}
