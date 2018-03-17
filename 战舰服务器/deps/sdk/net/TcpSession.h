#pragma once
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <common_define.h>
#include <mutex.h>
#include <buffer.h>
#include <net/TcpServer.h>
#include <net/MessageQueue.h>
#include <logger.h>


class TcpSession : public boost::enable_shared_from_this<TcpSession> {
 public:
  TcpSession(IoServicePtr ioService, SocketPtr socket);
  ~TcpSession(void);

 public:
  void Start();
  void Close();
  void Shutdown();
  //������������Ϣ�Ľӿ�,�����滻��ͷ�����Sequence
  bool SyncWrite(const char* pData, size_t nLen);   //ͬ��д
  bool ASyncWrite(const char* pData, size_t nLen);  //�첽д

  void ReadFirstData(size_t nLen);
  const SocketPtr& GetSocketPtr();

  void SetEntryType(uint32_t entry_type) { this->m_EntryType = entry_type; }
  uint32_t GetEntryType() const { return this->m_EntryType; }
  uint64_t GetSessionID() const { return this->m_SessionID; }
  const std::string& GetAccount() const { return this->m_account; }
  void SetAccount(const std::string& account) { this->m_account = account; }
  time_t GetLastActiveTime() const { return this->m_LastActiveTime; }

  void SetDecoder(DecoderFn decoder) {
    this->m_decode = decoder;
  }

  int64_t GetUID() const { return this->m_uid; }
  void SetUID(int64_t uid);
  const std::string& IpAddr() const { return this->m_IpAddr; }
  uint16_t Port() const { return this->m_Port; }

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

    try {
      head.length = this->SendCompressedMessage(
                        head.msgid, buf, head.real_length() + sizeof(HEAD)) -
                    sizeof(HEAD);
    } catch (...) {
    }
  }

  //���滻��ͷ�����Sequence
  int32_t SendCompressedMessage(int16_t msgid, char* begin, int32_t length);

 private:
  //�첽�Ķ�д����
  void HandleRead(const boost::system::error_code& error,
                  size_t bytes_transferred);
  void FirstHandleRead(const boost::system::error_code& error,
                       size_t bytes_transferred);
  bool CheckPerSecondCounter();

  void HandleWrite(const boost::system::error_code& error,
                   size_t bytes_transferred);


 private:
  SocketPtr m_pSocket;
  bool m_isWriting;

  time_t m_LastActiveTime;     //�ϴλʱ��
  uint32_t m_EntryType;        //���ӵ�����
  const uint64_t m_SessionID;  //ΨһID
  int64_t m_uid;               //���ID

  std::mutex m_writeBufferMutex;  //����д�����ź���
  std::mutex m_readBufferMutex;   //
  char m_IOReadBuff[2048];        //����BOOST�첽����BUFF
  Buffer m_recvedBuffer;          //���ܵ���buffer
  Buffer m_BufferWaiting;         //�ȴ�д���buffer
  Buffer m_BufferWriting;         //����д���buffer
  std::string m_account;          //openid
  DecoderFn m_decode;             //����
  std::string m_IpAddr;           //IpAddress
  uint16_t m_Port;                //�˿�
};
