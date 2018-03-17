#pragma once
#include <singleton.h>
#include <common_define.h>
#include <logger.h>
#include <system.h>
#include <net/TcpSession.h>
#include <net/TcpServer.h>
#include <decoding.h>
#include <boost/thread.hpp>
#include <message_queue.h>
#include <mysqlcxx.h>
#include <closure.h>
#include "record_player.h"

enum {
#ifdef DEBUG
  RECORD_WORKER_COUNT       = 2,  //2个Update线程
  RECORD_ASYNC_WORKER_COUNT = 2,  //2个异步load线程
#else
  RECORD_WORKER_COUNT       = 8,  //8个Update线程
  RECORD_ASYNC_WORKER_COUNT = 4,  //4个异步load线程
#endif
  LOG_SQL_TIME              = 8,  //8ms以上的SQL写日志
  RECORD_SQL_ERROR          = 110000,
};

extern int32_t RECORD_TABLE_COUNT;

//只是SQL
typedef std::string SqlMessage;

typedef int32_t (*ServerMessageHandler)(SSMessageEntry&);
typedef int32_t (RecordPlayer::*PlayerMessageHandler)(SSMessageEntry&);

extern Logger* sql_log;

class SqlWorkerThread : NonCopyable {
 public:
  SqlWorkerThread() : thread_(NULL) { this->terminal_ = false; }
  ~SqlWorkerThread() {
    Join();
    delete thread_;
  }
  void PushMessage(const SqlMessage& message) { queue_.Push(message); }
  void Run();

  void Join() {
    if (thread_) thread_->join();
  }

  int32_t SqlQueueSize() { return this->queue_.Size(); }
  BlockingMessageQueue<SqlMessage>& Queue() { return this->queue_; }
  const std::string& current_sql() const { return this->current_sql_; }
  void terminal() { this->terminal_ = true; }

 private:
  //主循环
  void Loop();
  //返回sql耗时
  int32_t ExecSingleSql(const char* sql, int32_t length);
 private:
  boost::thread* thread_;
  MySqlConnection conn_;
  BlockingMessageQueue<SqlMessage> queue_;
  std::string current_sql_;
  bool terminal_;
};

class AsyncClosureThread : NonCopyable {
 public:
  AsyncClosureThread() : thread_(NULL) {}
  ~AsyncClosureThread() {
    Join();
    delete thread_;
  }
  void PushMessage(const ClosurePtr& message) { queue_.Push(message); }
  void Run();

  void Join() {
    if (thread_) thread_->join();
  }

  MySqlConnection& conn() { return conn_; }

 private:
  //主循环
  void Loop();
 private:
  boost::thread* thread_;
  MySqlConnection conn_;
  BlockingMessageQueue<ClosurePtr> queue_;
};

class NameThread : NonCopyable {
 public:
  NameThread() : thread_(NULL) {}
  ~NameThread() {
    Join();
    delete thread_;
  }

  void PushMessage(SSMessageEntry& message) { queue_.Push(message); }
  void Run();

  void Join() { if (thread_) thread_->join(); }

  MySqlConnection& mysql_conn() { return conn_; }
 private:
  //主循环
  void Loop();
 private:
  boost::thread* thread_;
  MySqlConnection conn_;
  MessageQueue<SSMessageEntry> queue_;
};

class Server;
extern Server* server;
extern NameThread* name_thread;

class Server : NonCopyable, public Singleton<Server> {
 public:
   friend class NameThread;
  Server();

  //false初始化失败
  bool InitServer();

  void Loop();

  //发送消息给DB的Player
  void SendPlayerMessage(TcpSession* pSession, int32_t player_id,
                         uint16_t msgid, Message* msg);
  //发送消息给DB
  void SendServerMessage(TcpSession* pSession, uint16_t msgid, Message* msg);
  //发送消息给指定服务器
  void SendServerMessage(uint32_t *servers, int32_t count, uint16_t msgid, Message* msg);

  MySqlConnection& mysql_conn() { return mysql_conn_; }

  void PushSql(int64_t uid, const SqlMessage& entry);
  void PushAsyncClosure(int64_t uid, const ClosurePtr& closure);
  MySqlConnection& GetAsyncConn(int64_t uid);

  //增加服务器映射
  void AddServer(uint32_t server_id,
                 const boost::shared_ptr<TcpSession>& session) {
    this->server_session_map_[server_id] = session;
  }

  boost::shared_ptr<TcpSession> GetServerByID(uint32_t server_id) {
    return this->server_session_map_[server_id].lock();
  }

  void AddServerMap(uint32_t server_id, uint32_t main_server_id) {
    this->server_map_[server_id] = main_server_id;
  }

  uint32_t GetMainServerID(uint32_t server_id) const {
    VectorMap<uint32_t, uint32_t>::const_iterator iter = this->server_map_.find(server_id);
    if (iter != this->server_map_.end() && iter->first == server_id) return iter->second;
    return server_id;
  }

 public:
  RecordPlayer* GetOrNewPlayer(int64_t uid) {
    if (!uid) return NULL;
    RecordPlayer* player = this->players_.GetPlayerByID(uid);
    if (player) {
      player->active();
      return player;
    }
    player = this->players_.GetOrNewPlayer(uid);
    player->active();
    player->set_unload();
    return player;
  }

  RecordPlayer* GetPlayerByID(int64_t uid) {
    RecordPlayer* player = this->players_.GetPlayerByID(uid);
    if (player) player->active();
    return player;
  }

  void ErasePlayer(int64_t uid) { return this->players_.Erase(uid); }

  const PlayerContainer<RecordPlayer>& Players() const { return this->players_; }

  void EraseAllTimeOutPlayer();

  std::string EscapeString(const char* str, uint32_t len);

  int32_t SqlQueueSize();

  static bool LoopFlag;

  void StopWorkers() {
    for (int32_t index = 0; index < ArraySize(this->workers_); ++index) {
      this->workers_[index].terminal();
    }
  }
  void DumpUnfinishedSQL();

 private:
  void LoadConfig();
  //通过数据库Conn初始化种子
  void InitSeeds();
  //初始化回调函数
  void InitMessageHandler();

  void ParseSSMessageOnce();

  void OnSecondsChanged();
  void OnMinChanged();
  void OnHourChanged();

  void InitTableCount();
 private:
  time_t last_active_seconds_;
  tm last_active_time_;
  int32_t minutes_;
  TcpServer server_;
  boost::thread* flush_log_thread_;
  SqlWorkerThread workers_[RECORD_WORKER_COUNT];
  AsyncClosureThread asyncs_[RECORD_ASYNC_WORKER_COUNT];
  NameThread name_thread_;
  PlayerContainer<RecordPlayer> players_;
  MySqlConnection mysql_conn_;

  //服务器映射
  VectorMap<uint32_t, boost::weak_ptr<TcpSession> > server_session_map_;
  VectorMap<uint32_t, uint32_t> server_map_;
  //消息回调
  boost::unordered_map<uint16_t, ServerMessageHandler> server_message_handler_;
  boost::unordered_map<uint16_t, PlayerMessageHandler> player_message_handler_;
};
