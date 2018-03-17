#pragma once
#include <vector_set.h>
#include <vector_map.h>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/atomic.hpp>
#include <common_define.h>
#include <net/http/io_service_pool.hpp>
#include <net/MessageQueue.h>
#include <singleton.h>
#include <mutex.h>
#include <vector>

typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> AcceptorPtr;

typedef boost::shared_ptr<boost::asio::io_service> IoServicePtr;
class TcpSession;
typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> AcceptorPtr;
typedef boost::shared_ptr<boost::asio::ip::tcp::socket> SocketPtr;
typedef boost::shared_ptr<TcpSession> TcpSessionPtr;

//不同的Acceptor accept出来的TcpSession可能是不一样的
struct TcpAcceptor {
  AcceptorPtr acceptor;
  uint32_t entry_type;
  uint16_t port;
  DecoderFn decoder;
};

class TcpSessionManager : public Singleton<TcpSessionManager>, NonCopyable {
 public:
  void AddTcpSession(TcpSessionPtr& ptr);
  TcpSessionPtr GetTcpSessionBySessionID(uint64_t session_id);
  void RemoveTcpSession(TcpSession* ptr);

  size_t GetSessionCount();
  void ShutDown();

  //一个简单的time wheel用来T掉超时的客户端
  void KickTimeOutSession(int32_t wheel);
  int32_t SessionCount() const { return session_id_map.size(); }
 private:
  std::mutex mutex;
  typedef VectorMap<uint64_t, TcpSessionPtr> SessionIDMap;
  //SessionID => SessionPtr
  SessionIDMap session_id_map;
  VectorSet<uint64_t> idle_sock_set[IDLE_SOCK_TIME];
};

class TcpServer : NonCopyable {
 public:
  TcpServer(int32_t thread_count = 4);
  ~TcpServer(void);

 public:
  //可以监听多个端口,然后每个端口的解码不一样
  void Bind(uint16_t port, uint32_t entry_type, DecoderFn decoder);

  void Run();   //开启服务
  void Stop();  //停止服务
  IoServicePtr GetIoService();

 protected:
  void StartAccept(uint16_t port);
  virtual void HandleAccept(uint16_t port, TcpSessionPtr tsp,
                            const boost::system::error_code& error);

 private:
  void FlushFiles(const boost::system::error_code& /*e*/);
  void KillIdleSocks(const boost::system::error_code& /*e*/);

 protected:
  std::size_t m_nPort;
  http::server2::io_service_pool io_service_pool_;
  IoServicePtr accepte_service_;
  std::vector<TcpAcceptor> acceptors;
};

