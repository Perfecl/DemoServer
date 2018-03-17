#include <net/MessageQueue.h>
#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <system.h>
#include <storage.h>
#include "config.h"
#include "message_handler.h"
#include "server.h"

using namespace sy;
using namespace intranet;

extern ServerConfig* server_config;

void Server::InitMessageHandler() {
  //服务器消息
  this->server_message_handler_[MSG_SS_SERVER_LOGIN] = ProcessServerLogin;
  this->server_message_handler_[MSG_SS_NOTIFY_PLAYER_NUM] = ProcessPlayerNum;
  this->server_message_handler_[MSG_SS_RESPONSE_ADD_GOODS] = ProcessAddGoods;
  this->server_message_handler_[MSG_SS_UPDATE_RECHARGE_DETAILS] = ProcessUpdateRecharge;
  this->server_message_handler_[MSG_SS_UPDATE_OTHER_PLAYER_INFO] = ProcessUpdateOtherPlayerInfo;
  this->server_message_handler_[MSG_SS_REQUEST_QUERY_OTHER_PLAYER] = ProcessRequestQueryOtherPlayer;
  this->server_message_handler_[MSG_SS_REQUEST_LEGION_WAR_NEW_PLAYER] = ProcessRequestRegisterLegionPlayer;
  this->server_message_handler_[MSG_SS_UPDATE_LEGION_WAR_POS] = ProcessRequestLegionWarSwapPlayer;
  this->server_message_handler_[MSG_SS_UPDATE_CROSS_SERVER_RANK_LIST] = ProcessRequestUpdateCrossServerRankList;
  this->server_message_handler_[MSG_SS_UPDATE_LEGION_WAR_PLAYER] = ProcessUpdateLegionWarOtherPlayer;
  this->server_message_handler_[MSG_SS_REQUEST_PLAYER_LOGIN] = ProcessPlayerLogin;
  this->server_message_handler_[MSG_SS_REQUEST_CREATE_CORAL_SEA_TEAM] = ProcessRequestCreateCoralSeaTeam;
  this->server_message_handler_[MSG_SS_REQUEST_SEARCH_CORAL_SEA_TEAM] = ProcessRequestSearchCoralSeaTeam;
  this->server_message_handler_[MSG_SS_REQUEST_JOIN_CORAL_SEA_TEAM] = ProcessRequestJoinCoralSeaTeam;
  this->server_message_handler_[MSG_SS_REQUEST_LEAVE_CORAL_SEA_TEAM] = ProcessRequestLeaveCoralSeaTeam;
  this->server_message_handler_[MSG_SS_REQUEST_CORAL_SEA_DEBUG] = ProcessRequestCoralSeaDebug;
}

//内部服务器没有CS Message
static std::vector<SSMessageEntry> ss_messages;
void Server::ParseSSMessageOnce() {
  if (!PopMessages(ss_messages)) return;
  for (size_t i = 0; i < ss_messages.size(); ++i) {
    SSMessageEntry& entry = ss_messages[i];
    INFO_LOG(logger)("ParseSSMessage SessionID:%ld, MSG:0x%04X",
                     entry.session_ptr->GetSessionID(), entry.head.msgid);

    int32_t result = 0;
    boost::unordered_map<uint16_t, ServerMessageHandler>::iterator iter =
        this->server_message_handler_.find(entry.head.msgid);
    if (iter == this->server_message_handler_.end()) {
      ERROR_LOG(logger)(
            "ParseSSMessageOnce ServerMessageHandler not found, MSG:0x%04X",
            entry.head.msgid);
      continue;
    }
    result = (*iter->second)(entry);
    if (result) {
      ERROR_LOG(logger)("ParseSSMessageOnce MSG:0x%04X, ResultID:%d",
                          entry.head.msgid, result);
    }
  }
  ss_messages.clear();
}

void Server::LoadLocalStorage() {
  storage::Init("./center_db", 128 * 1024 * 1024);
  this->LoadOtherPlayer();
  this->LoadServerOpenDaysFromLocal();

  //放在最后
  this->LoadLegionRegion();
}

void FlushLog() {
  while (Server::LoopFlag) {
    if (logger) logger->Flush();
    if (sql_log) sql_log->Flush();
    Yield(FLUSH_LOG_FRAME_TIME);
  }
}

void Server::Loop() {
  this->http_server_->run();
  this->flush_log_thread_ = new boost::thread(boost::bind(&FlushLog));
  this->LoadLocalStorage();
  server_.Run();
  for (size_t i = 0; i < sizeof(this->workers_) / sizeof(this->workers_[0]);
       ++i) {
      this->workers_[i].Run();
  }

  while (Server::LoopFlag) {
    time_t begin = GetMilliSeconds();
    if (this->last_active_seconds_ != GetSeconds()) this->OnSecondsChanged();

    ParseSSMessageOnce();
    time_t end = GetMilliSeconds();

    if (end - begin < ONE_FRAME_TIME) {
      Yield(ONE_FRAME_TIME - (end - begin));
    }
  }

  logger->Flush();
  storage::UnInit();
  TRACE_LOG(logger)("Storage UnInit");
  logger->Flush();
}

int32_t SqlWorkerThread::ExecSingleSql(const SqlMessage& message) {
  time_t begin = GetMilliSeconds();

  const char* sql = message.sql.c_str();
  int32_t length = message.sql.length();

  const MySqlConnPtr& conn = MySqlConnManager::Instance().GetConnection(MySqlConnType(message.conn_type));

  int32_t result = conn->ExecSql(sql, length);
  if (result < 0) {
    ERROR_LOG(sql_log)("%s", sql);
  } else if (result > 0 || length < 10) {
    INFO_LOG(sql_log)("%s", sql);
  } else {
    TRACE_LOG(sql_log)("%s", sql);
  }
  return GetMilliSeconds() - begin;
}

void SqlWorkerThread::Loop() {
  BlockingMessageQueue<SqlMessage>::container_type queue;
  while (true) {
    time_t begin = GetMilliSeconds();
    this->queue_.Pop(queue);

    for (size_t i = 0; i < queue.size(); ++i) {
      int32_t time = this->ExecSingleSql(queue[i]);
      if (time > LOG_SQL_TIME) {
        WARN_LOG(logger)("ExecSqlTime:%dms, Sql:%s", time, queue[i].sql.c_str());
      }
    }
    queue.clear();

    time_t end = GetMilliSeconds();

    if (end - begin < WORKER_FRAME_TIME) {
      //Yield(WORKER_FRAME_TIME - (end - begin));
    }
  }
}
