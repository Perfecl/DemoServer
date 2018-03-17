#pragma once
#include "boost/enable_shared_from_this.hpp"
#include "TcpSession.h"

class TcpClient {
 public:
  TcpClient(uint32_t entry_type, DecoderFn decoder);
  ~TcpClient(void);

 public:
  bool Connect(const char *szIP, const char *szPort, const IoServicePtr& io);

  bool ASyncWrite(char *pData, size_t nLen);
  TcpSessionPtr GetTcpSession() { return m_pSession; }
  uint32_t GetEntryType() const { return this->m_EntryType; }

  bool IsValid() const { return this->m_pSocket && this->m_pSocket->is_open(); }

 public:
  template <typename HEAD>
  void SendMessage(HEAD& head, Message* pMsg) {
    char buf[SS_MSG_MAX_LEN];
    head.length = pMsg ? pMsg->ByteSize() : 0;
    if (head.length > head.real_length()) {
      ERROR_LOG(logger)("SendMessage Fail, MSG:0x%04x, Name:%s, Len:%d",
                        head.msgid, pMsg->GetTypeName().c_str(), head.length);
      return;
    }
    *(HEAD*)buf = head;
    if (pMsg) {
      uint8_t* begin = (uint8_t*)buf + sizeof(HEAD);
      bool result = pMsg->SerializeWithCachedSizesToArray(begin);
      if (!result) {
        ERROR_LOG(logger)("SerializeMessage Fail, MSG:0x%04x, Name:%s, Len:%d",
                          head.msgid, pMsg->GetTypeName().c_str(), head.length);
        return;
      }
    }
    this->ASyncWrite(buf, head.real_length() + sizeof(HEAD));
  }
 private:
  void CloseSocket();

 private:
  TcpSessionPtr m_pSession;
  SocketPtr m_pSocket;
  DecoderFn m_decoder;
  uint32_t m_EntryType;
};
