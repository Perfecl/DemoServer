#pragma once
#include <decoding.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <buffer.h>

class TcpSession;
typedef int32_t (*DecoderFn)(TcpSession*, Buffer&);

int32_t CSMessageDecoder(TcpSession*, Buffer&);
int32_t SSMessageDecoder(TcpSession*, Buffer&);
typedef boost::shared_ptr<Message> MessagePtr;

struct CSMessageEntry {
  CSHead head;
  boost::shared_ptr<TcpSession> session_ptr;
  MessagePtr message;

  Message* get() { return this->message.get(); }
};

struct SSMessageEntry {
  SSHead head;
  boost::shared_ptr<TcpSession> session_ptr;
  MessagePtr message;

  Message* get() { return this->message.get(); }
};

void PushCSMessage(const CSHead& head, const boost::shared_ptr<TcpSession>& ptr,
                   const MessagePtr& message);
void PushCSMessage(const CSHead& head, const boost::shared_ptr<TcpSession>& ptr,
                   Message* message);
void PushSSMessage(const SSHead& head, const boost::shared_ptr<TcpSession>& ptr,
                   Message* message);

int32_t PopMessages(std::vector<CSMessageEntry>& vec);
int32_t PopMessages(std::vector<SSMessageEntry>& vec);

//消息循环
//需要在服务器内自己实现
extern void ParseCSMessageOnce();
extern void ParseSSMessageOnce();
