#include "server.h"
#include "config.h"
#include <stdlib.h>
#include <config_file.h>
#include <cpp/server_message.pb.h>
#include <array.h>
#include <array_stream.h>
#include <stdio.h>

Server* server = NULL;
NameThread* name_thread = NULL;
class T1;
class T2;
Logger* sql_log = NULL;

const int32_t kRecordLoggerBufferSize = 32 * 1024 * 1024;
int32_t RECORD_TABLE_COUNT = 1;

extern boost::atomic_int64_t save_counter;
extern boost::atomic_int64_t load_counter;

void SignalUsr2(int) {
  server->StopWorkers();
  server->DumpUnfinishedSQL();
  Server::LoopFlag = false;
}

Server::Server()
    : last_active_seconds_(GetSeconds()),
      last_active_time_(GetTime()),
      minutes_(0),
      server_(1) {
  server = this;

  Mkdir("./log");
  logger = &Logger::InitDefaultLogger<T1>(
      FormatLogFileName(LOG_PATH, "record").c_str(), "./log/record.log",
      kLoggerLevel_Info, kRecordLoggerBufferSize);
  sql_log = &Logger::InitDefaultLogger<T2>(
      FormatLogFileName(LOG_PATH, "sql").c_str(), "./log/sql.log",
      kLoggerLevel_Info, kRecordLoggerBufferSize);
  signal(SIGUSR2, SignalUsr2);
}

bool Server::LoopFlag = true;

static ServerConfig* server_config = new ServerConfig("./record.xml");
bool Server::InitServer() {
  this->InitMessageHandler();

  ConfigFileManager::Instance().AddConfigFile(server_config);

  bool load_config = ConfigFileManager::Instance().Load();
  if (!load_config) {
    ERROR_LOG(logger)("加载配置文件失败");
    return false;
  }

  //在服务器启动时候就设定好种子值
  mysql_conn_.Connect(server_config->mysql().ip.c_str(),
                      server_config->mysql().port,
                      server_config->mysql().user_name.c_str(),
                      server_config->mysql().password.c_str(),
                      server_config->mysql().db_name.c_str());

  //初始化表
  this->InitTableCount();

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

  return true;
}

void Server::OnSecondsChanged() {
  const tm& tm_Now = GetTime();
  if (tm_Now.tm_min != this->last_active_time_.tm_min) {
    this->OnMinChanged();
  }
  if (tm_Now.tm_hour != this->last_active_time_.tm_hour) {
    this->OnHourChanged();
  }

  static int32_t last_sql_queue_size = 0;
  int32_t sql_queue_size = this->SqlQueueSize();
  if (sql_queue_size != last_sql_queue_size) {
    TRACE_LOG(logger)("SQL QUEUE SIZE:%d", sql_queue_size);
    last_sql_queue_size = sql_queue_size;
  }

  this->last_active_seconds_ = GetSeconds();
  this->last_active_time_ = tm_Now;
}

void Server::OnMinChanged() {
  ++this->minutes_;

  // 5分钟定时器
  if (this->minutes_ % 5 == 0) {
    this->EraseAllTimeOutPlayer();
  }
}

void Server::OnHourChanged() {
  TRACE_LOG(logger)("AsyncSqlCount:%ld", save_counter.load());
  TRACE_LOG(logger)("AsyncLoadCount:%ld", load_counter.load());
  save_counter = 0;
  load_counter = 0;

  logger->ChangeLoggerFile(FormatLogFileName(LOG_PATH, "record").c_str());
  sql_log->ChangeLoggerFile(FormatLogFileName(LOG_PATH, "sql").c_str());
}

//发送消息给服务器
void Server::SendPlayerMessage(TcpSession* session, int32_t player_id,
                               uint16_t msgid, Message* msg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_PLAYER;
  head.dest_id = player_id;

  session->SendMessage(head, msg);
}
void Server::SendServerMessage(TcpSession* session, uint16_t msgid,
                               Message* msg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_LOGIC_SERVER;
  head.dest_id = 0;

  session->SendMessage(head, msg);
}

void Server::SendServerMessage(uint32_t* servers, int32_t count, uint16_t msgid,
                               Message* msg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_LOGIC_SERVER;
  head.dest_id = 0;

  Array<boost::shared_ptr<TcpSession>, 256> array;
  for (int32_t i = 0; i < count; ++i) {
    const boost::shared_ptr<TcpSession> session =
        this->GetServerByID(servers[i]);
    if (session && session.get()) {
      array.push_back(session);
    }
  }
  std::sort(array.begin(), array.end());
  boost::shared_ptr<TcpSession>* end = std::unique(array.begin(), array.end());
  for (boost::shared_ptr<TcpSession>* iter = array.begin(); iter != end;
       ++iter) {
    (*iter)->SendMessage(head, msg);
  }
}

void NameThread::Run() {
  if (conn_.Connect(server_config->mysql().ip.c_str(),
                     server_config->mysql().port,
                     server_config->mysql().user_name.c_str(),
                     server_config->mysql().password.c_str(),
                     server_config->mysql().db_name.c_str())) {
    ERROR_LOG(logger)("MySQL Connect fail, %s:%d",
                      server_config->mysql().ip.c_str(),
                      server_config->mysql().port);
    TRACE_LOG(logger)("NameThread Exit");
    return;
  }
  thread_ = new boost::thread(boost::bind(&NameThread::Loop, this));
  name_thread = this;
}

void SqlWorkerThread::Run() {
  if (conn_.Connect(server_config->mysql().ip.c_str(),
                     server_config->mysql().port,
                     server_config->mysql().user_name.c_str(),
                     server_config->mysql().password.c_str(),
                     server_config->mysql().db_name.c_str())) {
    ERROR_LOG(logger)("MySQL Connect fail, %s:%d",
                      server_config->mysql().ip.c_str(),
                      server_config->mysql().port);
    TRACE_LOG(logger)("SqlWorkerThread Exit");
    return;
  }
  thread_ = new boost::thread(boost::bind(&SqlWorkerThread::Loop, this));
}

void AsyncClosureThread::Run() {
  if (conn_.Connect(server_config->mysql().ip.c_str(),
                     server_config->mysql().port,
                     server_config->mysql().user_name.c_str(),
                     server_config->mysql().password.c_str(),
                     server_config->mysql().db_name.c_str())) {
    ERROR_LOG(logger)("MySQL Connect fail, %s:%d",
                      server_config->mysql().ip.c_str(),
                      server_config->mysql().port);
    TRACE_LOG(logger)("AsyncClosureThread Exit");
    return;
  }
  thread_ = new boost::thread(boost::bind(&AsyncClosureThread::Loop, this));
}

void Server::EraseAllTimeOutPlayer() {
  this->players_.EraseTimeOutPlayer();
}

void Server::PushSql(int64_t uid, const SqlMessage& entry) {
  this->workers_[uid % RECORD_WORKER_COUNT].PushMessage(entry);
}

void Server::PushAsyncClosure(int64_t uid, const ClosurePtr& entry) {
  this->asyncs_[uid % RECORD_ASYNC_WORKER_COUNT].PushMessage(entry);
}

MySqlConnection& Server::GetAsyncConn(int64_t uid) {
  return this->asyncs_[uid % RECORD_ASYNC_WORKER_COUNT].conn();
}

std::string Server::EscapeString(const char* str, uint32_t len) {
  return mysql_conn_.EscapeString(str, len);
}

void Server::InitTableCount() {
  ArrayStream<16 * 1024> stream;

  stream.Append("show tables like 'copy_star_%%'");
  const boost::shared_ptr<ResultSet>& result =
      this->mysql_conn_.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), this->mysql_conn_.GetLastError().c_str());
    return;
  }
  int32_t count = result->RowCount();
  if (count > 1 && count % 8 == 0) RECORD_TABLE_COUNT = count;
  INFO_LOG(logger)("TableCount:%d", RECORD_TABLE_COUNT);
}

int32_t Server::SqlQueueSize() {
  int32_t size = 0;
  for (int32_t i = 0; i < ArraySize(this->workers_); ++i) {
    size += this->workers_[i].SqlQueueSize();
  }
  return size;
}

//2009-10-12_01
int32_t dump_time_len = 0;
static char dump_time[25] = {0};

static inline void AppendSqlLine(FILE* file, std::string& sql) {
  if (!dump_time[0]) {
    FormatDateHour(dump_time, GetTime());
    dump_time_len = strlen(dump_time);
    dump_time[dump_time_len] = ' ';
    dump_time_len++;
  }
  fwrite(dump_time, 1, dump_time_len, file);
  sql.resize(sql.length() + 1, '\n');
  fwrite(sql.c_str(), 1, sql.length(), file);
}

void Server::DumpUnfinishedSQL() {
  if (!server->SqlQueueSize())
    return;
  FILE* file = fopen("./UnfinishedSQL.txt", "a+");
  if (!file) {
    ERROR_LOG(logger)("Open UnfinishedSQL.txt fail");
    return;
  }

  for (int32_t worker_index = 0; worker_index < ArraySize(this->workers_);
       ++worker_index) {
    SqlWorkerThread& thread = this->workers_[worker_index];
    BlockingMessageQueue<std::string>::container_type container;
    thread.Queue().PopWithoutWait(container);
    std::string sql = thread.current_sql();
    if (sql.length()) {
      AppendSqlLine(file, sql);
    }
    for (BlockingMessageQueue<std::string>::container_type::iterator iter =
             container.begin();
         iter != container.end(); ++iter) {
      AppendSqlLine(file, *iter);
    }
  }
  fclose(file);
}
