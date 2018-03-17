#include "server.h"
#include <stdlib.h>
#include "config.h"
#include <config_file.h>

Server* server = NULL;
class T1;

Server::Server()
    : last_active_seconds_(GetSeconds()), last_active_time_(GetTime()), server_(1) {
  server = this;

  Mkdir("./log");
  logger = &Logger::InitDefaultLogger<T1>(
      FormatLogFileName(LOG_PATH, "auth").c_str(), "./log/auth.log",
      kLoggerLevel_Info);
}

static ServerConfig* server_config = new ServerConfig("./auth.xml");

bool Server::InitServer() {
 ConfigFileManager::Instance().AddConfigFile(server_config);

 bool load_config = ConfigFileManager::Instance().Load();
 if (!load_config) {
   ERROR_LOG(logger)("加载配置文件失败");
   return false;
 }

 //在服务器启动时候就设定好种子值
 MySqlConnection conn;
 int32_t conn_result = conn.Connect(server_config->mysql().ip.c_str(),
                                    server_config->mysql().port,
                                    server_config->mysql().user_name.c_str(),
                                    server_config->mysql().password.c_str(),
                                    server_config->mysql().db_name.c_str());
 if (conn_result) {
   ERROR_LOG(logger)(
       "Connect MySQL fail: %s:%hd, user:%s, password:%s, db_name:%s",
       server_config->mysql().ip.c_str(), server_config->mysql().port,
       server_config->mysql().user_name.c_str(),
       server_config->mysql().password.c_str(),
       server_config->mysql().db_name.c_str());
   return false;
 }

 //这边初始化服务器的监听端口
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

void Server::LoadConfig() {
 ConfigFileManager::Instance().Reload();
}

void Server::OnSecondsChanged() {
  const tm& tm_Now = GetTime();

  //1分钟定时任务
  if (tm_Now.tm_min != this->last_active_time_.tm_min) {
    this->LoadConfig();
  }

  //整点定时任务
  if (tm_Now.tm_hour != this->last_active_time_.tm_hour) {
    this->OnHourChanged();
  }

  this->last_active_time_ = tm_Now;
  this->last_active_seconds_ = GetSeconds();
}

void Server::OnHourChanged() {
  TRACE_LOG(logger)("OnHourChanged");
  logger->ChangeLoggerFile(FormatLogFileName(LOG_PATH, "auth").c_str());
}

void Server::SendServerMessage(TcpSession* pSession, uint16_t msgid,
                           Message* pMsg) {
  INFO_LOG(logger)("SendServerMessage SessionID:%ld, MSGID:0x%04X", pSession->GetSessionID(), msgid);

  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_LOGIC_SERVER;
  head.dest_id = 0;

  pSession->SendMessage(head, pMsg);
}

void WorkerThread::Run() {
  if (conn_.Connect(server_config->mysql().ip.c_str(),
                    server_config->mysql().port,
                    server_config->mysql().user_name.c_str(),
                    server_config->mysql().password.c_str(),
                    server_config->mysql().db_name.c_str())) {
    ERROR_LOG(logger)("MySQL Connect fail, %s:%d",
                      server_config->mysql().ip.c_str(),
                      server_config->mysql().port);
    TRACE_LOG(logger)("WorkerThread Exit");
    return;
  }
  thread_ = new boost::thread(boost::bind(&WorkerThread::Loop, this));
}

void WorkerThread::PushMessage(WorkerMessage& message) {
  //INFO_LOG(logger)("PushMessage %p", &this->queue_);
  this->queue_.Push(message);
}
