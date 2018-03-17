#include <message_queue.h>
#include <net/TcpSession.h>
#include <net/MessageQueue.h>
#include <boost/bind.hpp>
#include <decoding.h>
#include <logger.h>
#include "MessageQueue.h"

static MessageQueue<CSMessageEntry> EntryQueue;
static MessageQueue<SSMessageEntry> SSEntryQueue;

void PushCSMessage(const CSHead& head, const boost::shared_ptr<TcpSession>& ptr,
                   const MessagePtr& message) {
  CSMessageEntry entry = {head, ptr, MessagePtr(message)};
  EntryQueue.Push(entry);
}
void PushCSMessage(const CSHead& head, const boost::shared_ptr<TcpSession>& ptr,
                   Message* message) {
  CSMessageEntry entry = {head, ptr, MessagePtr(message)};
  EntryQueue.Push(entry);
}

void PushSSMessage(const SSHead& head, const boost::shared_ptr<TcpSession>& ptr,
                   Message* message) {
  SSMessageEntry entry = {head, ptr, MessagePtr(message)};
  SSEntryQueue.Push(entry);
}

int32_t PopMessages(std::vector<CSMessageEntry>& vec) {
  vec.clear();
  EntryQueue.Pop(vec);
  return vec.size();
}

int32_t PopMessages(std::vector<SSMessageEntry>& vec) {
  vec.clear();
  SSEntryQueue.Pop(vec);
  return vec.size();
}

int32_t CSMessageDecoder(TcpSession* pSession, Buffer& buffer) {
  int32_t msg_count = 0;
  CSHead head = {0, 0, 0};
  uint32_t bytes_count = 0;
  Message* msg = NULL;
  do {
    if (buffer.ReadableLength() <= 0) break;

    bytes_count = 0;
    msg = TryDecodeCSMessage(buffer.BeginRead(), buffer.ReadableLength(),
                                  head, bytes_count);
    if (bytes_count) {
      PushCSMessage(head, pSession->shared_from_this(), msg);
      buffer.HasRead(bytes_count);
      ++msg_count;
    }

    if (head.real_length() >= CS_MSG_MAX_LEN) {
      ERROR_LOG(logger)("PlayerID:%ld, SessionID:%ld Message Length:%d, MSG:0x%04X",
          pSession->GetUID(), pSession->GetSessionID(), head.real_length(), head.msgid);
      pSession->Shutdown();
    }
    else if (!msg && bytes_count > CSHEAD_LENGTH) {
      ERROR_LOG(logger)("DecodeCSMessage fail, PlayerID:%ld, SessionID:%ld, MSG:0x%04X",
          pSession->GetUID(), pSession->GetSessionID(), head.msgid);
    } else {
      if (head.msgid != 0x2002 && head.msgid != 0x7011)
        INFO_LOG(logger)("DecodeCSMessage success, PlayerID:%ld, SessionID:%ld, MSG:0x%04X",
                        pSession->GetUID(), pSession->GetSessionID(), head.msgid);
    }
  } while (bytes_count > 0);
  return msg_count;
}

int32_t SSMessageDecoder(TcpSession* pSession, Buffer& buffer) {
  int32_t msg_count = 0;
  SSHead head = {0, 0, 0, 0};
  uint32_t bytes_count = 0;
  Message* msg = NULL;
  do {
    if (buffer.ReadableLength() <= 0) break;

    bytes_count = 0;
    msg = TryDecodeSSMessage(buffer.BeginRead(), buffer.ReadableLength(),
                                  head, bytes_count);
    if (bytes_count) {
      PushSSMessage(head, pSession->shared_from_this(), msg);
      buffer.HasRead(bytes_count);
      ++msg_count;
    }
    if (!msg && bytes_count > SSHEAD_LENGTH) {
      ERROR_LOG(logger)("DecodeSSMessage fail, ID:0x%04X", head.msgid);
    } else {
      if (head.msgid != 0x2002 && head.msgid != 0x7011)
        INFO_LOG(logger)("DecodeSSMessage success, SessionID:%ld, MSG:0x%04X",
                       pSession->GetSessionID(), head.msgid);
    }
  } while (bytes_count > 0);
  return msg_count;
}
