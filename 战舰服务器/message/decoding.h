#pragma once
#include <google/protobuf/message.h>
#include <common_define.h>

typedef google::protobuf::Message Message;

Message* TryDecodeCSMessage(const char* buffer, uint32_t length,
                            __OUT__ CSHead& head, uint32_t& consume_bytes);
Message* TryDecodeSSMessage(const char* buffer, uint32_t length,
                            __OUT__ SSHead& head, uint32_t& consume_bytes);
