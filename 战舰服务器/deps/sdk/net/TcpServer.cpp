#include "TcpServer.h"
#include "TcpSession.h"
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/make_shared.hpp>
#include <net/MessageQueue.h>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <stdio.h>
#include <logger.h>
#include <system.h>

using boost::asio::ip::tcp;

const int32_t kBufferSize = 256 * 1024;
static boost::asio::deadline_timer* flush_file_timer;
static int32_t wheel = 0;
//1秒定时器, 清掉没有数据通讯的链接
static boost::asio::deadline_timer* idle_sock_timer;

TcpServer::TcpServer(int32_t thread_count) : io_service_pool_(thread_count) {
  signal(SIGCHLD,SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  this->acceptors.reserve(128);
  this->accepte_service_ = io_service_pool_.get_io_service_ptr();
  flush_file_timer = new boost::asio::deadline_timer(
      *this->accepte_service_, boost::posix_time::milliseconds(100));
  idle_sock_timer = new boost::asio::deadline_timer(
      *this->accepte_service_, boost::posix_time::seconds(1));
}

TcpServer::~TcpServer(void) {}

void TcpServer::Bind(uint16_t port, uint32_t entry_type,
                     DecoderFn decoder) {
  //这边bind失败,服务器就不要启动了
  try {
    AcceptorPtr ptr(new tcp::acceptor(*(io_service_pool_.get_io_service_ptr()),
                                      tcp::endpoint(tcp::v4(), port)));
    TcpAcceptor acceptor = {ptr, entry_type, port, decoder};
    if (ptr && ptr->is_open()) {
      static tcp::no_delay option(true);
      ptr->set_option(option);
      //内部服务器设置滑动窗口大小
      if (entry_type != ENTRY_TYPE_PLAYER) {
        static boost::asio::socket_base::send_buffer_size SNDBUF(kBufferSize);
        static boost::asio::socket_base::receive_buffer_size RCVBUF(kBufferSize);
        ptr->set_option(SNDBUF);
        ptr->set_option(RCVBUF);
      }

      this->acceptors.push_back(acceptor);
      TRACE_LOG(logger)("Listen port:%d success", port);
    } else {
      ERROR_LOG(logger)("Listen port:%d fail", port);
    }
  } catch (...) {
    ERROR_LOG(logger)("Listen port:%d fail", port);
    exit(-1);
  }
}

void TcpServer::KillIdleSocks(const boost::system::error_code& /*e*/) {
  TcpSessionManager::Instance().KickTimeOutSession(wheel++);
  idle_sock_timer->expires_from_now(boost::posix_time::seconds(1));
  idle_sock_timer->async_wait(boost::bind(&TcpServer::KillIdleSocks, this,
                                           boost::asio::placeholders::error));
}

void TcpServer::Run() {
  StartAccept(0);
  io_service_pool_.run();
  idle_sock_timer->async_wait(boost::bind(&TcpServer::KillIdleSocks, this,
                                           boost::asio::placeholders::error));
}

IoServicePtr TcpServer::GetIoService() {
  return io_service_pool_.get_io_service_ptr();
}

void TcpServer::StartAccept(uint16_t port) {
  for (std::vector<TcpAcceptor>::iterator iter = this->acceptors.begin();
       iter != this->acceptors.end(); ++iter) {
    TcpAcceptor& acceptor = *iter;
    if (acceptor.acceptor && acceptor.acceptor->is_open() && acceptor.port) {
      if (!(!port || port == acceptor.port)) continue;

      TcpSessionPtr tsp = boost::make_shared<TcpSession>(accepte_service_, SocketPtr());
      tsp->SetEntryType(acceptor.entry_type);
      tsp->SetDecoder(acceptor.decoder);

      acceptor.acceptor->async_accept(
          *tsp->GetSocketPtr(),
          boost::bind(&TcpServer::HandleAccept, this, acceptor.port, tsp,
                      boost::asio::placeholders::error));
    }
  }
}

void TcpServer::HandleAccept(uint16_t port, TcpSessionPtr tsp,
                             const boost::system::error_code& e) {
  if (e) {
    ERROR_LOG(logger)("Accept fail, %s", e.message().c_str());
    tsp->Close();
  } else {
    TcpSessionManager::Instance().AddTcpSession(tsp);
    tsp->Start();
  }

  StartAccept(port);
}

void TcpServer::Stop() { io_service_pool_.stop(); }

void TcpSessionManager::AddTcpSession(TcpSessionPtr& ptr) {
  std::lock_guard<std::mutex> guard(this->mutex);
  if (ptr) {
    this->session_id_map[ptr->GetSessionID()] = ptr;
    VectorSet<uint64_t>& set = this->idle_sock_set[ptr->GetLastActiveTime() % IDLE_SOCK_TIME];
    set.insert(ptr->GetSessionID());
  }
}

TcpSessionPtr TcpSessionManager::GetTcpSessionBySessionID(uint64_t session_id) {
  std::lock_guard<std::mutex> guard(this->mutex);
  SessionIDMap::iterator iter = this->session_id_map.find(session_id);
  return iter != this->session_id_map.end() ? iter->second : TcpSessionPtr();
}

void TcpSessionManager::RemoveTcpSession(TcpSession* ptr) {
  std::lock_guard<std::mutex> guard(this->mutex);
  if (ptr) {
    this->session_id_map.erase(ptr->GetSessionID());
  }
}

size_t TcpSessionManager::GetSessionCount() {
  std::lock_guard<std::mutex> guard(this->mutex);
  return this->session_id_map.size();
}

void TcpSessionManager::ShutDown() {
  std::lock_guard<std::mutex> guard(this->mutex);
  for (SessionIDMap::iterator iter = this->session_id_map.begin();
       iter != this->session_id_map.end(); ++iter) {
    iter->second->Shutdown();
  }
  this->session_id_map.clear();
}

void TcpSessionManager::KickTimeOutSession(int32_t wheel) {

  std::lock_guard<std::mutex> guard(this->mutex);
  const time_t tm_now = GetSeconds();
  const int32_t index = wheel % IDLE_SOCK_TIME;
  VectorSet<uint64_t>& session_ids = this->idle_sock_set[index];

  for (VectorSet<uint64_t>::iterator iter = session_ids.begin();
       iter != session_ids.end(); /**/) {
    uint64_t session_id = *iter;
    bool erase = false;
    SessionIDMap::iterator session_iter = this->session_id_map.find(session_id);
    if (session_iter != this->session_id_map.end()) {
      TcpSessionPtr& ptr = session_iter->second;
      if (ptr->GetEntryType() == ENTRY_TYPE_PLAYER &&
          ptr->GetLastActiveTime() + IDLE_SOCK_TIME < tm_now) {
        erase = true;
        //这边会把Session从容器里面T掉
        INFO_LOG(logger)("KickTimeOutSession SessionID:%lu, GetSeconds:%ld, GetLastActiveTime:%ld"
            , ptr->GetSessionID(), tm_now, ptr->GetLastActiveTime()
            );
        ptr->Close();
      }
    } else {
      erase = true;
    }

    if (erase)
      iter = session_ids.erase(iter);
    else
      ++iter;
  }
}
