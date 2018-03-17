#include "server.h"
#include "config.h"
#include <stdlib.h>
#include <config_file.h>
#include <array.h>
#include <array_stream.h>
#include <storage.h>
#include <storage_ext.h>
#include "http_handler.h"
#include "legion_war.h"
#include "mutex.h"
#include "coral_sea.h"

Server* server = NULL;
class T1;
class T2;
Logger* sql_log = NULL;

const int32_t kCenterLoggerBufferSize = 32 * 1024 * 1024;
int32_t CENTER_TABLE_COUNT = 0;

void SignalUsr2(int) { Server::LoopFlag = false; }

Server::Server()
    : last_active_seconds_(GetSeconds()),
      last_active_time_(GetTime()),
      minutes_(0),
      server_(2),
      tid_(0) {
  server = this;

  Mkdir("./log");
  logger = &Logger::InitDefaultLogger<T1>(
      FormatLogFileNameWithoutHour(LOG_PATH, "center").c_str(),
      "./log/center.log", kLoggerLevel_Info, kCenterLoggerBufferSize);
  sql_log = &Logger::InitDefaultLogger<T2>(
      FormatLogFileNameWithoutHour(LOG_PATH, "center_sql").c_str(),
      "./log/center_sql.log", kLoggerLevel_Info, kCenterLoggerBufferSize);

  signal(SIGUSR2, SignalUsr2);
}

ServerConfig* server_config = new ServerConfig("./center.xml");
bool Server::InitServer() {
  this->InitMessageHandler();
  AddConfigFile();

  ConfigFileManager::Instance().AddConfigFile(server_config);

  bool load_config = ConfigFileManager::Instance().Load();
  if (!load_config) {
    ERROR_LOG(logger)("加载配置文件失败");
    return false;
  }
  AfterLoadConfig();

  //在服务器启动时候就设定好种子值
  MySqlConnManager::Instance().InitConn(
      CONN_TYPE_AUTH, server_config->auth_mysql(), THREAD_COUNT);
  MySqlConnManager::Instance().InitConn(
      CONN_TYPE_GM, server_config->gm_mysql(), THREAD_COUNT);

  //初始化服务器
  const std::vector<ServerConfig::ListenInfo>& listen_infos =
      server_config->ports();
  for (size_t i = 0; i < listen_infos.size(); ++i) {
    const ServerConfig::ListenInfo& info = listen_infos[i];
    switch (info.second) {
      default:
        server_.Bind(info.first, info.second, SSMessageDecoder);
        break;
    }
  }
  this->http_server_ = new http::server2::http_server(
      "0.0.0.0", server_config->http_port().c_str(), THREAD_HTTP_COUNT);
  this->http_server_->set_http_handler(
      HttpHandlerPtr(new http::server2::HttpRequestHandler()));

  return true;
}

bool Server::LoopFlag = true;

void Server::OnSecondsChanged() {
  const tm& tm_Now = GetTime();
  if (tm_Now.tm_min != this->last_active_time_.tm_min) {
    this->OnMinChanged();
  }
  if (tm_Now.tm_hour != this->last_active_time_.tm_hour) {
    this->OnHourChanged();
  }

  //这边把超时的充值请求干掉
  this->ClearTimeOutPay();

  TEAM_MANAGER.OnSecondChanged();
  this->last_active_seconds_ = GetSeconds();
  this->last_active_time_ = tm_Now;
  this->tid_ = GetSeconds() * 1000000000;
}

void Server::OnMinChanged() {
  ++this->minutes_;
  ConfigFileManager::Instance().Reload();
  ConnectionManager::Instance().RemoveIdleConnection();

  // 5分钟定时器
  if (this->minutes_ % 5 == 0) {
  }
}

void Server::OnHourChanged() {
  if (GetTime().tm_hour == 0) {
    logger->ChangeLoggerFile(FormatLogFileNameWithoutHour(LOG_PATH, "center").c_str());
    sql_log->ChangeLoggerFile(FormatLogFileNameWithoutHour(LOG_PATH, "center_sql").c_str());
    LegionWar::ClearAll();
  }
  if (GetTime().tm_hour == 1) {
    this->GenerateLegionWarRegion();
  }
  for (boost::unordered_map<std::string, int64_t>::const_iterator iter =
           this->order_time_.begin();
       iter != this->order_time_.end();) {
    if (GetSeconds() > iter->second + 3600) {
      iter = this->order_time_.erase(iter);
      continue;
    }
    ++iter;
  }
}

void Server::SendServerMessage(TcpSession* session, uint16_t msgid,
                               Message* msg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_CENTER_SERVER;
  head.dest_id = 0;

  session->SendMessage(head, msg);
}

bool Server::SendServerMessage(uint32_t server_id, uint16_t msgid,
                               Message* msg) {
  const TcpSessionPtr& session = this->GetServerByID(server_id);
  if (session) {
    this->SendServerMessage(session.get(), msgid, msg);
    return true;
  }
  return false;
}

void SqlWorkerThread::Run() {
  thread_ = new boost::thread(boost::bind(&SqlWorkerThread::Loop, this));
}

void Server::PushSql(int64_t uid, int32_t conn_type, const std::string& sql) {
  this->workers_[uid % WORKER_COUNT].PushMessage(SqlMessage(conn_type, sql));
}

boost::shared_ptr<MySqlConnection> MySqlConnPool::GetConnection() {
  static boost::shared_ptr<MySqlConnection> empty;
  static __thread int32_t t_index = -1;
  if (t_index < 0) t_index = index_allocator_++;

  if(connections_.empty())
    return empty;

  if (t_index >= (int32_t)connections_.size())
    return connections_[0];
  return connections_[t_index];
}

void MySqlConnPool::InitConn(const MySQLParams& mysql, int32_t count) {
  for (int32_t i = 0; i < count; ++i) {
    ConnPtr conn(new MySqlConnection);
    if (conn->Connect(mysql.ip.c_str(), mysql.port, mysql.user_name.c_str(),
                      mysql.password.c_str(), mysql.db_name.c_str())) {
      ERROR_LOG(logger)("MySQL Connect fail, %s:%d",
                      mysql.ip.c_str(),
                      mysql.port);
      return;
    }
    DEBUG_LOG(logger)("MySQLConnection, %s:%d, %s", mysql.ip.c_str(), mysql.port, mysql.db_name.c_str());
    connections_.push_back(conn);
  }
}

void MySqlConnManager::InitConn(MySqlConnType type,
                                      const MySQLParams& mysql, int32_t count) {
  if (type >= CONN_TYPE_SIZE) return;

  pool_[type].InitConn(mysql, count);
}

boost::shared_ptr<MySqlConnection> MySqlConnManager::GetConnection(MySqlConnType type) {
  static boost::shared_ptr<MySqlConnection> empty;
  if (type < CONN_TYPE_SIZE)
    return pool_[type].GetConnection();
  return empty;
}

std::mutex g_lock;

boost::shared_ptr<MySqlConnection> Server::GetGameConn(uint32_t server_id, bool is_game_db) {
  boost::shared_ptr<MySqlConnection> conn = boost::make_shared<MySqlConnection>();
  ArrayStream<1024> stream;
  stream.Append(
      "select dbwip, dbwport, dbwuser, dbwpassword, db_game_name, db_log_name "
      "from server where server_id = '%u'",
      server_id);

  std::lock_guard<std::mutex> clock(g_lock);
  boost::shared_ptr<ResultSet> result = MySqlConnManager::Instance().GetConnection(CONN_TYPE_GM)->ExecSelect(stream.c_str(), stream.size());
  if (result && result->IsValid()) {
    const std::string& ip = result->at(0).to_str();
    int16_t port = result->at(1).to_int32();
    const std::string& gamedb = result->at(4).to_str();
    const std::string& logdb = result->at(5).to_str();

    std::string user = result->at(2).to_str();
    std::string password = result->at(3).to_str();
    const std::string& db_name = is_game_db ? gamedb : logdb;
    //!!!
    //CenterServer不能写游戏库
    //!!!
    //if (is_game_db) {
    //  user = "root";
    //  password = "1q2w3e";
    //}
    DEBUG_LOG(logger)("MySql Connect, ServerID:%u, Addr:%s:%d, DBName:%s"
        , server_id, ip.c_str(), port, db_name.c_str());
    if (conn->Connect(ip.c_str(), port, user.c_str(), password.c_str(),
                      db_name.c_str())) {
      ERROR_LOG(logger)("MySql Connect Fail, ServerID:%u, IsGameDB:%d, %s:%d, DBName:%s"
          , server_id, is_game_db, ip.c_str(), port
          , db_name.c_str()
          );
    }
  } else {
    ERROR_LOG(logger)("Get ServerID:%u's DB fail", server_id);
  }
  return conn;
}

std::string Server::GetServerName(uint32_t server_id) {
  ArrayStream<1024> stream;
  stream.Append("select server_name from server where server_id='%u'", server_id);
  const boost::shared_ptr<ResultSet>& result =
      MySqlConnManager::Instance()
          .GetConnection(CONN_TYPE_GM)
          ->ExecSelect(stream.c_str(), stream.size());
  ;
  if (result && result->IsValid())
  {
    return result->at(0).to_str();
  }
  ERROR_LOG(logger)("Server::GetServerName(%u) Fail", server_id);
  return "";
}

void Server::ClearTimeOutPay() {
  std::lock_guard<std::mutex> guard(this->pay_mutex_);
  std::vector<int64_t> time_out_conn;
  int64_t sec = GetSeconds();
  for (boost::unordered_map<int64_t, int64_t>::iterator iter =
           this->pay_map_.begin();
       iter != this->pay_map_.end(); ++iter) {
    if (sec > iter->second + 10) {
      time_out_conn.push_back(iter->first);
    }
  }
  for (std::vector<int64_t>::iterator iter = time_out_conn.begin();
       iter != time_out_conn.end(); ++iter) {
    http::server2::connection_ptr conn = ConnectionManager::Instance().FetchConnection(*iter);
    if (conn) {
      conn->async_send("{\n\"code\":\"1\",\n\"message\":\"TimeOut\"\n}");
      ERROR_LOG(logger)("Recharge TimeOut, ConnID:%ld", conn->id());
    }
    this->pay_map_.erase(*iter);
  }
}

void Server::SetPlayTime(int64_t conn_id) {
  std::lock_guard<std::mutex> guard(this->pay_mutex_);
  this->pay_map_[conn_id] = GetSeconds();
}

void Server::ClearPay(int64_t conn_id) {
  std::lock_guard<std::mutex> guard(this->pay_mutex_);
  this->pay_map_.erase(conn_id);
}

int32_t Server::GetTableCount(int32_t server_id) {
  if (CENTER_TABLE_COUNT) return CENTER_TABLE_COUNT;

  ArrayStream<16 * 1024> stream;

  stream.Append("show tables like 'copy_star_%%'");
  const boost::shared_ptr<ResultSet>& result =
      this->GetGameConn(server_id, true)
          ->ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), this->GetGameConn(server_id,true)->GetLastError().c_str());
    return 1;
  }
  int32_t count = result->RowCount();
  if (count > 1 && count % 8 == 0) CENTER_TABLE_COUNT = count;
  else return 1;
  INFO_LOG(logger)("TableCount:%d", RECORD_TABLE_COUNT);
  return CENTER_TABLE_COUNT;
}

const std::string& kServerOpenDays = "ServerOpenDays";

void Server::LoadServerOpenDaysFromLocal() {
  storage_ext::Load(kServerOpenDays, this->server_open_days_);
}

void Server::SetServerOpenDays(uint32_t server, int32_t days) {
  if (days <= 0 || days >= 10000) return;
  if (this->server_open_days_[server] == days) return;
  DEBUG_LOG(logger)("ServerOpenDays, ServerID:%u, Days:%d", server, days);

  this->server_open_days_[server] = days;
  for (VectorSet<uint32_t>::iterator iter =
           this->server_2_server_[server].begin();
       iter != this->server_2_server_[server].end(); ++iter) {
    this->server_open_days_[*iter] = days;
  }

  storage_ext::Save(kServerOpenDays, this->server_open_days_);
}

const std::string& kLegionWarRegion = "LegionWarRegion";

void Server::LoadLegionRegion() {
  storage_ext::Load(kLegionWarRegion, this->legion_war_);
  if (this->legion_war_.empty()) {
    this->GenerateLegionWarRegion();
  }
}

void Server::GenerateLegionWarRegion() {
  int32_t region_count = 0;
  VectorMap<uint32_t, int32_t> temp;
  VectorMap<uint32_t, int32_t> region_0;

  VectorMap<int32_t, VectorMap<uint32_t, int32_t> > all_regions;
  for (VectorMap<uint32_t, int32_t>::const_iterator iter =
           this->server_open_days_.begin();
       iter != this->server_open_days_.end(); ++iter) {
    int32_t platform = iter->first / 10000;
    if (iter->second <= 7) {
      DEBUG_LOG(logger)("ServerOpenDays, ServerID:%u, Days:%d"
          , iter->first, iter->second);
      TRACE_LOG(logger)("GenerateRegion, ServerID:%u, Region:%d"
          , iter->first, 0);
      region_0[iter->first] = 0;
      continue;
    }
    all_regions[platform][iter->first] = 0;
  }
  for (VectorMap<int32_t, VectorMap<uint32_t, int32_t> >::iterator iter =
           all_regions.begin();
       iter != all_regions.end(); ++iter) {
    VectorMap<uint32_t, int32_t>& platform = iter->second;
    ++region_count;

    SettingConfigFile::GenerateRegion(platform, region_count);
    for (VectorMap<uint32_t, int32_t>::const_iterator iter_server =
             platform.begin();
         iter_server != platform.end(); ++iter_server) {
      temp.insert(*iter_server);
      TRACE_LOG(logger)("GenerateRegion, ServerID:%u, Region:%d"
          , iter_server->first, iter_server->second);
    }
  }
  for (VectorMap<uint32_t, int32_t>::iterator iter = region_0.begin();
       iter != region_0.end(); ++iter) {
    temp[iter->first] = iter->second;
  }
  if (!temp.empty()) {
    this->legion_war_ = temp;
    storage_ext::Save(kLegionWarRegion, this->legion_war_);
  }
}

int32_t Server::GetLegionRegionByServerID(uint32_t server_id) {
  VectorMap<uint32_t, int32_t>::const_iterator iter =
      this->legion_war_.find(server_id);
  if (iter != this->legion_war_.end()) return iter->second;
  if (!this->legion_war_.empty()) return this->legion_war_.begin()->second;
  return 0;
}

void Server::SendMessageToLegionWarRegion(uint32_t server_id, uint16_t msgid,
                                          Message* msg) {
  VectorSet<boost::shared_ptr<TcpSession> > sessions;
  uint32_t server_start = (server_id - 1) / SettingConfigFile::GetRegionSize() *
                          SettingConfigFile::GetRegionSize() + 1;
  uint32_t server_end = server_start + 9;
  DefaultArrayStream stream;
  stream.Append("SendMessageToLegionWarRegion, MSGID:0x%04X, ServerID:%u", msgid, server_id);

  for (uint32_t s = server_start; s <= server_end; ++s) {
    boost::shared_ptr<TcpSession> ptr = server->GetServerByID(s);
    if (ptr) {
      sessions.insert(ptr);
      stream.Append(" , ServerID:%u", s);
    }
  }
  for (VectorSet<boost::shared_ptr<TcpSession> >::iterator iter =
           sessions.begin();
       iter != sessions.end(); ++iter) {
    server->SendServerMessage((*iter).get(), msgid, msg);
  }
  DEBUG_LOG(logger)("%s", stream.c_str());
}

void Server::AddServer(uint32_t server_id,
                       const boost::shared_ptr<TcpSession>& session) {
  std::lock_guard<std::mutex> guard(this->server_map_mutex_);

  boost::unordered_map<uint32_t, boost::weak_ptr<TcpSession> >::iterator it =
      this->server_map_.find(server_id);
  if (it != this->server_map_.end()) {
    const boost::shared_ptr<TcpSession>& addr = it->second.lock();
    if (addr) {
      ERROR_LOG(logger)("add server error,server_id:%d, old IP %s:%u,new IP %s:%u",
          server_id,addr->IpAddr().c_str(), addr->Port() , session->IpAddr().c_str(), session->Port());
    }
  }

  this->server_map_[server_id] = session;
}

void Server::SendMessageToAllServer(uint16_t msgid, Message* msg) {
  VectorSet<TcpSessionPtr> servers;
  {
    std::lock_guard<std::mutex> guard(this->server_map_mutex_);
    for (boost::unordered_map<uint32_t, boost::weak_ptr<TcpSession> >::iterator
             it = this->server_map_.begin();
         it != this->server_map_.end(); ++it) {
      const TcpSessionPtr& ptr = it->second.lock();
      if (ptr) servers.insert(ptr);
    }
  }
  for (VectorSet<TcpSessionPtr>::iterator iter = servers.begin();
       iter != servers.end(); ++iter) {
    this->SendServerMessage(iter->get(), msgid, msg);
  }
}
