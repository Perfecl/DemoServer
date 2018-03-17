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
#include <boost/unordered_map.hpp>
#include <net/http/server.hpp>
#include <cpp/message.pb.h>
#include <vector_map.h>
#include <vector_set.h>

enum {
  WORKER_COUNT = 1,        // 1个Update线程
  RECORD_TABLE_COUNT = 8,  //分表8张
  LOG_SQL_TIME = 5,        // 5ms以上的SQL写日志

  THREAD_HTTP_COUNT = 4,  // HTTP线程数
  THREAD_MAIN_COUNT = 1,
  THREAD_ASYNC_SQL = 1,
  THREAD_COUNT = THREAD_HTTP_COUNT + THREAD_MAIN_COUNT + THREAD_ASYNC_SQL,
};

enum MySqlConnType {
  CONN_TYPE_AUTH = 0,
  CONN_TYPE_GM,
  CONN_TYPE_SIZE
};

struct MySQLParams;

//只是SQL
struct SqlMessage {
  SqlMessage(int32_t conn_type, const std::string& str)
      : conn_type(conn_type), sql(str) {}
  int32_t conn_type;
  std::string sql;
};

typedef boost::shared_ptr<MySqlConnection> MySqlConnPtr;

typedef int32_t (*ServerMessageHandler)(SSMessageEntry&);

extern Logger* sql_log;

extern int32_t CENTER_TABLE_COUNT;

class SqlWorkerThread : NonCopyable {
 public:
  SqlWorkerThread() : thread_(NULL) {}
  ~SqlWorkerThread() {
    Join();
    delete thread_;
  }
  void PushMessage(const SqlMessage& message) { queue_.Push(message); }
  void Run();

  void Join() {
    if (thread_) thread_->join();
  }

 private:
  //主循环
  void Loop();
  //返回sql耗时
  int32_t ExecSingleSql(const SqlMessage& message);
 private:
  boost::thread* thread_;
  BlockingMessageQueue<SqlMessage> queue_;
};

class Server;
extern Server* server;

class Server : NonCopyable, public Singleton<Server> {
 public:
  Server();

  //false初始化失败
  bool InitServer();

  void Loop();

  //发送消息
  void SendServerMessage(TcpSession* pSession, uint16_t msgid, Message* msg);
  //发送消息给指定服务器
  //true是成功
  bool SendServerMessage(uint32_t server_id, uint16_t msgid, Message* msg);
  //发送消息给所有服务器
  void SendMessageToAllServer(uint16_t msgid, Message* msg);

  //发送消息给制霸全球的一个大区
  void SendMessageToLegionWarRegion(uint32_t server_id, uint16_t msgid, Message* msg);

  void PushSql(int64_t uid, int32_t conn_type, const std::string& sql);

  //增加服务器映射
  void AddServer(uint32_t server_id,
                 const boost::shared_ptr<TcpSession>& session);

  boost::shared_ptr<TcpSession> GetServerByID(uint32_t server_id) {
    std::lock_guard<std::mutex> guard(this->server_map_mutex_);
    return this->server_map_[server_id].lock();
  }

  std::vector<sy::OtherPlayerInfo> SearchPlayerExcept(
      int32_t level, int32_t count, const VectorSet<int64_t>& except);

  //true是gamedb
  //false是logdb
  static boost::shared_ptr<MySqlConnection> GetGameConn(uint32_t server_id, bool is_game_db);
  //通过ServerID获取服务器名称
  std::string GetServerName(uint32_t server_id);

  boost::asio::io_service& get_io_service() {
    return http_server_->pool().get_io_service();
  }

  void AddItemName(int64_t id, const std::string& name) {
    std::lock_guard<std::mutex> guard(this->item_name_mutex_);
    this->item_name_[id] = name;
  }

  const std::string& GetItemName(int64_t id) {
    static std::string empty = "未知道具";
    std::lock_guard<std::mutex> guard(this->item_name_mutex_);
    boost::unordered_map<int64_t, std::string>::const_iterator iter =
        this->item_name_.find(id);
    return iter != this->item_name_.end() ? iter->second : empty;
  }

  void SetServer2Server(uint32_t server2, uint32_t server1) {
    this->server_2_server_[server1].insert(server2);
  }

  void LoadServerOpenDaysFromLocal();
  void SetServerOpenDays(uint32_t server, int32_t days);

  //设置充值时间
  void SetPlayTime(int64_t conn_id);
  //清除充值时间
  void ClearPay(int64_t conn_id);
  void ClearTimeOutPay();

  int32_t GetTableCount(int32_t server_id);

  void LoadLocalStorage();
  void LoadOtherPlayer();
  void LoadLegionRegion();
  void SaveOtherPlayer(const sy::OtherPlayerInfo& player);

  int32_t GetLegionRegionByServerID(uint32_t server_id);
  static bool LoopFlag;

  //制霸全球的分区表
  const VectorMap<uint32_t, int32_t>& legion_war_region() const {
    return this->legion_war_;
  }

  int64_t& order_time(const std::string& order_id) {
    return this->order_time_[order_id];
  }

  int64_t GetTID() { return ++this->tid_; }

 private:
  void LoadConfig();
  //初始化回调函数
  void InitMessageHandler();

  void ParseSSMessageOnce();

  void GenerateLegionWarRegion();

  void OnSecondsChanged();
  void OnMinChanged();
  void OnHourChanged();

 private:
  time_t last_active_seconds_;
  tm last_active_time_;
  int32_t minutes_;
  TcpServer server_;
  boost::thread* flush_log_thread_;
  SqlWorkerThread workers_[WORKER_COUNT];
  //HTTP服务器
  http::server2::http_server* http_server_;
  int64_t tid_;

  //服务器映射
  std::mutex server_map_mutex_;
  boost::unordered_map<uint32_t, boost::weak_ptr<TcpSession> > server_map_;

  //充值的容器
  std::mutex pay_mutex_;
  boost::unordered_map<int64_t, int64_t> pay_map_;

  //消息回调
  boost::unordered_map<uint16_t, ServerMessageHandler> server_message_handler_;

  //道具名称
  std::mutex item_name_mutex_;
  boost::unordered_map<int64_t, std::string> item_name_;

  //这个也要存档
  //服务器的开服天数
  VectorMap<uint32_t, VectorSet<uint32_t> > server_2_server_;
  VectorMap<uint32_t, int32_t> server_open_days_;
  //制霸全球的战区分配
  VectorMap<uint32_t, int32_t> legion_war_;
  //补发订单时间
  boost::unordered_map<std::string, int64_t> order_time_;
};


class MySqlConnPool : NonCopyable {
 public:
  MySqlConnPool() : index_allocator_(0) {}

  boost::shared_ptr<MySqlConnection> GetConnection();
  void InitConn(const MySQLParams& mysql, int32_t count);

 private:
  boost::atomic_int index_allocator_;
  typedef boost::shared_ptr<MySqlConnection> ConnPtr;
  std::vector<ConnPtr> connections_;
};

class MySqlConnManager : NonCopyable, public Singleton<MySqlConnManager> {
 public:
   boost::shared_ptr<MySqlConnection> GetConnection(MySqlConnType type);

  void InitConn(MySqlConnType type, const MySQLParams& mysql, int32_t count);

 private:
  MySqlConnPool pool_[CONN_TYPE_SIZE];
};
