#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <boost/make_shared.hpp>
#include <system.h>
#include <array_stream.h>
#include <lz4/lz4.h>
#include <inttypes.h>
#include "MessageCache.h"
#include "TcpSession.h"
#include "logger.h"

using boost::asio::ip::tcp;

static boost::atomic_uint64_t SessionID(10000);

TcpSession::TcpSession(IoServicePtr ioService, SocketPtr socket)
    : m_SessionID(++SessionID),
      m_recvedBuffer(1024),
      m_BufferWaiting(1024),
      m_BufferWriting(1024) {
  if (socket) {
    this->m_pSocket = socket;
  } else {
    this->m_pSocket.reset(new tcp::socket(*ioService));
  }
  memset(m_IOReadBuff, 0, sizeof(m_IOReadBuff));
  this->m_LastActiveTime = GetSeconds();
  m_isWriting = false;
  this->m_uid = 0;
}

TcpSession::~TcpSession(void) {
  DEBUG_LOG(logger)("TcpSession Destory, SessionID:%lu", this->GetSessionID());
}

void TcpSession::SetUID(int64_t uid) {
  this->m_uid = uid;
}

const SocketPtr& TcpSession::GetSocketPtr() { return this->m_pSocket; }

void TcpSession::FirstHandleRead(const boost::system::error_code &error,
                                 size_t bytes_transferred) {
  if (!this->m_pSocket) return;
  (void)bytes_transferred;
  if (error) {
    this->Close();
  } else {
    static tcp::socket::non_blocking_io non_blocking_io(true);
    m_pSocket->io_control(non_blocking_io);

    Start();
  }
}

void TcpSession::HandleRead(const boost::system::error_code &error,
                            size_t bytes_transferred) {
  if (!this->m_pSocket) return;
  std::lock_guard<std::mutex> lock(m_readBufferMutex);
  this->m_LastActiveTime = GetSeconds();
  if (error) {
    if (error != boost::asio::error::eof &&
        error != boost::asio::error::connection_reset) {
      DEBUG_LOG(logger)("SessionID:%ld, ReadError, %s", this->GetSessionID(),
                        error.message().c_str());
    }
    this->Close();
  } else {
    if (this->GetEntryType() == ENTRY_TYPE_PLAYER) {
      bool bVal = CheckPerSecondCounter();
      if (bVal) return;
    }

    this->m_recvedBuffer.Append(m_IOReadBuff, bytes_transferred);
    // decoding here
    // and push message to message queue
    if (this->m_decode) {
      int32_t msg_count = this->m_decode(this, this->m_recvedBuffer);
      (void)msg_count;
    } else {
     ERROR_LOG(logger)("DOSE NOT SET Decoder");
    }
    this->m_recvedBuffer.Retrieve();
    Start();
  }
}

void TcpSession::Start() {
  if (this->m_IpAddr.empty()) {
    boost::system::error_code err;
    try {
      this->m_IpAddr = this->m_pSocket->remote_endpoint().address().to_string();
      this->m_Port = this->m_pSocket->remote_endpoint().port();
      TRACE_LOG(logger)("NewSession, SessionID:%ld, %s:%u"
          , this->GetSessionID(), this->IpAddr().c_str(), this->Port());
    } catch (...) {
      DEBUG_LOG(logger)("GetIPAddr fail:%s", err.message().c_str());
    }
  }

  m_pSocket->async_read_some(
      boost::asio::buffer(m_IOReadBuff, sizeof(m_IOReadBuff)),
      boost::bind(&TcpSession::HandleRead, shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void TcpSession::ReadFirstData(size_t nLen) {
  if (!this->m_pSocket) return;

  m_pSocket->async_read_some(
      boost::asio::buffer(m_IOReadBuff, nLen),
      boost::bind(&TcpSession::FirstHandleRead, shared_from_this(),
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void TcpSession::HandleWrite(const boost::system::error_code &error,
                             size_t bytes_transferred) {
  (void)bytes_transferred;
  if (!GetSocketPtr()) return;

  if (!error) {
    std::lock_guard<std::mutex> lock(m_writeBufferMutex);

    this->m_BufferWriting.HasRead(bytes_transferred);
    this->m_BufferWriting.Retrieve();

    if (this->m_BufferWaiting.ReadableLength()) {
      this->m_BufferWriting.Append(this->m_BufferWaiting.BeginRead(), this->m_BufferWaiting.ReadableLength());
      this->m_BufferWaiting.HasRead(this->m_BufferWaiting.ReadableLength());
      this->m_BufferWaiting.Retrieve();
    }

    m_isWriting = false;
    if (this->m_BufferWriting.ReadableLength())
    {
      m_isWriting = true;
      boost::asio::async_write(
          *GetSocketPtr(),
          boost::asio::buffer(m_BufferWriting.BeginRead(),
                              m_BufferWriting.ReadableLength()),
          boost::bind(&TcpSession::HandleWrite, shared_from_this(),
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
    }
  }
}

bool TcpSession::ASyncWrite(const char *pData, size_t nLen) {
  std::lock_guard<std::mutex> lock(m_writeBufferMutex);

  if (!GetSocketPtr() || !GetSocketPtr()->is_open()) {
    return false;
  }

  m_BufferWaiting.Append(pData, nLen);
  if (!m_isWriting) {
    m_isWriting = true;

    m_BufferWriting.Append(m_BufferWaiting.BeginRead(),
                           m_BufferWaiting.ReadableLength());
    m_BufferWaiting.HasRead(m_BufferWaiting.ReadableLength());
    m_BufferWaiting.Retrieve();

    try {
      boost::asio::async_write(
          *GetSocketPtr(),
          boost::asio::buffer(m_BufferWriting.BeginRead(),
                              m_BufferWriting.ReadableLength()),
          boost::bind(&TcpSession::HandleWrite, shared_from_this(),
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred));
    } catch (...) {
      ERROR_LOG(logger)("ERROR : TcpSession::AsyncWrite error");
    }
  }
  return true;
}

bool TcpSession::SyncWrite(const char *pData, size_t nLen) {
  std::lock_guard<std::mutex> lock(m_writeBufferMutex);

  try {
    if (!m_pSocket || !m_pSocket->is_open()) {
      ERROR_LOG(logger)("SyncWrite sesion is closed , failed to write!");
      return false;
    }

    boost::asio::write(*GetSocketPtr(), boost::asio::buffer(pData, nLen));

  } catch (...) {
    ERROR_LOG(logger)("TcpSession::SyncWrite Error, SessionID:%ld",
                      this->GetSessionID());
  }

  return true;
}

void TcpSession::Shutdown() {
  if (!this->m_pSocket) return;
  boost::system::error_code ignored_ec;
  this->m_pSocket->shutdown(boost::asio::ip::tcp::socket::shutdown_receive,
                            ignored_ec);
  INFO_LOG(logger)("Shutdown TcpSession, SessionID:%ld", this->GetSessionID());
}

void TcpSession::Close() {
  std::lock_guard<std::mutex> lock(m_writeBufferMutex);
  if (m_pSocket) {
    DEBUG_LOG(logger)("CloseSession, PlayerID:%ld, SessionID:%lu, %s:%u", this->GetUID(),
        this->GetSessionID(), this->m_IpAddr.c_str(), this->m_Port);
  }
  try {
    if (m_pSocket && m_pSocket->is_open()) {
      m_pSocket->close();
    }
    m_pSocket.reset();
  } catch (boost::lock_error e) {
    ERROR_LOG(logger)(
        "TcpSession Error:%d, SessionID:%lu, %s:%u", e.native_error(),
        this->GetSessionID(),
        this->IpAddr().c_str(), this->Port());
  }
  if (this->GetUID()) {
    CSHead head;
    head.length = 0;
    head.msgid = 0x2014;
    this->m_recvedBuffer.Append(&head, sizeof (head));
    if (this->m_decode) this->m_decode(this, this->m_recvedBuffer);
  }
  TcpSessionManager::Instance().RemoveTcpSession(this);
}

bool TcpSession::CheckPerSecondCounter() {
  // TODO:egmkang
  //检测发包次数
  return false;
}

int32_t TcpSession::SendCompressedMessage(int16_t msgid, char *begin,
                                          int32_t length) {
  if (this->GetEntryType() == ENTRY_TYPE_PLAYER) {
    uint16_t seq = this->GetUID()
                       ? MessageCache::Instance().GetSequence(this->GetUID())
                       : 0;
    CSHead *head = (CSHead*)begin;
    head->msgid = msgid;
    head->seq = seq;
    //INFO_LOG(logger)("MSGID:0x%04X, SessionID:%ld, Seq:%u", msgid, this->GetSessionID(), head->seq);
  }
  //客户端逻辑消息的缓存
  if (this->GetEntryType() == ENTRY_TYPE_PLAYER && this->GetUID() &&
      msgid >= 0x204F && msgid <= 0x7000) {
    MessageCache::Instance().SendRawMessage(this->GetUID(), msgid, begin, length);
  }
  if (this->GetEntryType() == ENTRY_TYPE_PLAYER &&
      length > COMPRESS_MESSAGE_MIN_SIZE &&
      //这几个消息不能被压缩
      (msgid != 0x20A2)) {
    char buf[SS_MSG_MAX_LEN];
    CSHead *head = (CSHead*)buf;
    head->msgid = msgid;
    head->seq = ((CSHead*)begin)->seq;
    int32_t compressed_length = LZ4_compress_default(
        begin + sizeof(CSHead), buf + sizeof(CSHead), length - sizeof(CSHead),
        SS_MSG_MAX_LEN - sizeof(CSHead));
    if (compressed_length <= 0) {
      this->ASyncWrite(begin, length);
    } else {
      INFO_LOG(logger)("CompressMessage MSGID:0x%04X, Length:%ld => Length:%d"
          , msgid, length - sizeof(CSHead), compressed_length);
      head->length = compressed_length;
      head->length |= CSHEAD_COMPRESS_FLAG;
      this->ASyncWrite(buf, head->real_length() + sizeof(CSHead));
      return head->real_length() + sizeof(CSHead);
    }
  } else {
    this->ASyncWrite(begin, length);
  }
  return length;
}
