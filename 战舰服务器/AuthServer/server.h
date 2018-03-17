#pragma once
#include <singleton.h>
#include <common_define.h>
#include <logger.h>
#include <system.h>
#include <boost/shared_ptr.hpp>
#include <net/TcpSession.h>
#include <net/TcpServer.h>
#include <decoding.h>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <message_queue.h>
#include <mysqlcxx.h>
#include <boost/unordered_map.hpp>

enum {
  AUTH_DB_TABLE_COUNT   = 16,//AuthDB分表的个数
  AUTH_DB_WORKER_COUNT  = 4, //工作线程数
  AUTH_SQL_ERROR        = 100000,
};

class Server;
extern Server* server;

struct WorkerMessage {
 uint16_t msgid;
 uint32_t hash_value;
 boost::shared_ptr<TcpSession> session_ptr;
 MessagePtr message;
};

class WorkerThread : NonCopyable {
 public:
  WorkerThread() : index_(0), thread_(NULL) {}
  ~WorkerThread() {
    Join();
    delete thread_;
  }
  void PushMessage(WorkerMessage& message);
  void Run();

  void Join() {
    if (thread_) thread_->join();
  }

  int64_t GenUID(int32_t server, int32_t& error_code);
  void SetIndex(int8_t i) { this->index_ = i; }

 private:
  //主循环
  void Loop();
  //获取Account对应的ID
  int64_t ProcessGetUID(WorkerMessage& entry);
  void ProcessRegisteAccount(WorkerMessage& entry);
  void ProcessAccountLogin(WorkerMessage& entry);
 private:
  int32_t index_;
  boost::thread* thread_;
  MySqlConnection conn_;
  BlockingMessageQueue<WorkerMessage> queue_;
};

class Server : NonCopyable, public Singleton<Server> {
 public:
  Server();

  //false初始化失败
  bool InitServer();
  void Loop();

  //发送消息
  void SendServerMessage(TcpSession* pSession, uint16_t msgid,
                             Message* pMsg);

  //添加服务器
  void AddServer(uint32_t server_id,
                 const boost::shared_ptr<TcpSession>& session) {
    std::lock_guard<std::mutex> lock(this->server_map_lock_);
    this->server_map_[server_id] = session;
  }

  boost::shared_ptr<TcpSession> GetServer(uint32_t server_id) {
    std::lock_guard<std::mutex> lock(this->server_map_lock_);
    return this->server_map_[server_id].lock();
  }

 private:
  void LoadConfig();

  void DispatchMessage(SSMessageEntry& entry);

  void OnSecondsChanged();
  void OnHourChanged();

 private:
  time_t last_active_seconds_;
  tm last_active_time_;
  TcpServer server_;
  boost::thread* flush_log_thread_;
  WorkerThread workers_[AUTH_DB_WORKER_COUNT];
  //服务器映射
  std::mutex server_map_lock_;
  boost::unordered_map<uint32_t, boost::weak_ptr<TcpSession> > server_map_;
};
