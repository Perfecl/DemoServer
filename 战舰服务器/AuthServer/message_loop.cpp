#include <net/MessageQueue.h>
#include <cpp/server_message.pb.h>
#include <system.h>
#include <array_stream.h>
#include "server.h"

using namespace intranet;

static std::vector<SSMessageEntry> ss_messages;

static boost::atomic_int64_t token_seeds(1);

void Server::DispatchMessage(SSMessageEntry& entry) {
  switch (entry.head.msgid) {
    //处理服务器登录逻辑
    case MSG_SS_SERVER_LOGIN: {
      MessageSSServerLogin* message =
          static_cast<MessageSSServerLogin*>(entry.get());
      if (!message) break;
      int32_t server_count = message->server_ids_size();
      if (server_count <= 0) {
        ERROR_LOG(logger)("ServerLogin Fail, ServerCount:0");
        entry.session_ptr->Close();
        break;
      }
      for (int i = 0; i < server_count; ++i) {
        server->AddServer(message->server_ids(i), entry.session_ptr);
      }
      TRACE_LOG(logger)("New Server Connected:%s, ServerID:%u",
                        message->server_name().c_str(),
                        message->server_ids(0));
    } break;

    //消息派发给worker
    case MSG_SS_REQUEST_GET_UID: {
      MessageSSRequestGetUID* message =
          static_cast<MessageSSRequestGetUID*>(entry.get());
      uint32_t hash_value =
          GetHashValue(message->openid().c_str(), message->openid().length());
      int32_t worker_index = hash_value % AUTH_DB_WORKER_COUNT;

      WorkerMessage message_entry = {entry.head.msgid, hash_value,
                                     entry.session_ptr, entry.message};
      //INFO_LOG(logger)("PushMessage To WorkerThread:%d", worker_index);
      this->workers_[worker_index].PushMessage(message_entry);
    } break;

    case MSG_SS_REQUEST_ACCOUNT_LOGIN: {
      MessageSSRequestAccountLogin* message =
          static_cast<MessageSSRequestAccountLogin*>(entry.get());
      uint32_t hash_value =
          GetHashValue(message->account().c_str(), message->account().length());
      int32_t worker_index = hash_value % AUTH_DB_WORKER_COUNT;

      WorkerMessage message_entry = {entry.head.msgid, hash_value,
                                     entry.session_ptr, entry.message};
      //INFO_LOG(logger)("PushMessage To WorkerThread:%d", worker_index);
      this->workers_[worker_index].PushMessage(message_entry);
    } break;

    case MSG_SS_REQUEST_REGISTE_ACCOUNT:{
      MessageSSRequestRegisteAccount* message =
          static_cast<MessageSSRequestRegisteAccount*>(entry.get());
      uint32_t hash_value =
          GetHashValue(message->account().c_str(), message->account().length());
      int32_t worker_index = hash_value % AUTH_DB_WORKER_COUNT;

      WorkerMessage message_entry = {entry.head.msgid, hash_value,
                                     entry.session_ptr, entry.message};
      //INFO_LOG(logger)("PushMessage To WorkerThread:%d", worker_index);
      this->workers_[worker_index].PushMessage(message_entry);
    } break;

    default:
      WARN_LOG(logger)("Unkown Message:0x%04x", entry.head.msgid);
      break;
  }
}

void FlushLog() {
  while (true) {
    if (logger) logger->Flush();
    Yield(FLUSH_LOG_FRAME_TIME);
  }
}

void Server::Loop() {
  this->flush_log_thread_ = new boost::thread(boost::bind(&FlushLog));
  server_.Run();
  for (size_t i = 0; i < sizeof(this->workers_) / sizeof(this->workers_[0]);
       ++i) {
    this->workers_[i].SetIndex(i);
    this->workers_[i].Run();
  }

  while (true) {
    time_t begin = GetMilliSeconds();
    //认证服务器的主循环就写死, 直接通过open id派发给工作线程
    if (PopMessages(ss_messages)) {
      for (size_t i = 0; i < ss_messages.size(); ++i) {
        SSMessageEntry& entry = ss_messages[i];
        INFO_LOG(logger)("ParseSSMessage SessionID:%ld, MSG:0x%04x",
                         entry.session_ptr->GetSessionID(), entry.head.msgid);
        DispatchMessage(entry);
      }
    }
    ss_messages.clear();

    //一秒一次定时器
    if (GetSeconds() != this->last_active_seconds_) this->OnSecondsChanged();
    time_t end = GetMilliSeconds();

    //服务器帧率暂定50帧
    if (end - begin < ONE_FRAME_TIME) {
      Yield(ONE_FRAME_TIME - (end - begin));
    }
  }
}

int64_t WorkerThread::GenUID(int32_t server, int32_t& error_code) {
  ArrayStream<1024> stream;
  int64_t uid = 0;
  stream.Append("call GetUID2(%u, @uid%d)", server, index_);
  error_code = this->conn_.ExecSql(stream.c_str(), stream.size());
  if (error_code) {
    ERROR_LOG(logger)("ExecSql fail:%s, ErrorCode:%d", stream.c_str(), error_code);
    error_code = abs(error_code);
  } else {
    stream.clear();
    stream.Append("select @uid%d", index_);
    const boost::shared_ptr<ResultSet>& result =
        this->conn_.ExecSelect(stream.c_str(), stream.size());
    if (result->error) {
      ERROR_LOG(logger)("ExecSql fail:%s, ErrorCode:%d", stream.c_str(), result->error);
      error_code = abs(result->error);
    } else {
      uid = result->at(0).to_int64();
    }
  }
  return uid;
}

int64_t WorkerThread::ProcessGetUID(WorkerMessage& entry) {
  MessageSSRequestGetUID* message =
      static_cast<MessageSSRequestGetUID*>(entry.message.get());

  int32_t error_code = 0;
  int64_t uid = 0;
  ArrayStream<1024> stream;
  bool create = false;
  ArrayStream<128> token_stream;

  do {
    const std::string& openid = this->conn_.EscapeString(message->openid());
    stream.Append(
        "select uid from account_%d where openid='%s' "
        " and server=%u",
        entry.hash_value % AUTH_DB_TABLE_COUNT, openid.c_str(),
        message->server());
    const boost::shared_ptr<ResultSet>& result =
        this->conn_.ExecSelect(stream.c_str(), stream.size());
    if (result->error) {
      ERROR_LOG(logger)("ExecSql fail:%d, %s", result->error, stream.c_str());
      error_code = abs(result->error);
      break;
    }
    if (result && result->IsValid()) {
      uid = result->at(0).to_int64();

      stream.clear();
      stream.Append("update account_%d set last_login_time=%ld where uid=%ld",
                    entry.hash_value % AUTH_DB_TABLE_COUNT, GetSeconds(), uid);
      conn_.ExecSql(stream.c_str(), stream.size());
    } else {
      uid = this->GenUID(message->server(), error_code);
      if (uid <= 0) {
        ERROR_LOG(logger)("GenUID Fail");
        break;
      }
      stream.clear();
      stream.Append(
          "insert into account_%d(`openid`, `server`, "
          "`uid`, `create_time`, `last_login_time`) values('%s', "
          "'%u', %ld, %ld, %ld)",
          entry.hash_value % AUTH_DB_TABLE_COUNT, openid.c_str(),
          message->server(), uid, GetSeconds(), GetSeconds());

      int32_t result = conn_.ExecSql(stream.c_str(), stream.size());
      if (result < 0) {
        ERROR_LOG(logger)("ExecSql result:%d, %s", result, stream.c_str());
        error_code = abs(result);
      } else {
        INFO_LOG(logger)("ExecSql result:%d, %s", result, stream.c_str());
      }
      create = true;
    }
    TRACE_LOG(logger)("openid:%s, server:%u, uid:%ld, create:%d, channel:%s, device_id:%s, idfa:%s",
                    message->openid().c_str(),
                    message->server(), uid, create, message->channel().c_str(),
                    message->device_id().c_str(), message->idfa().c_str());

    token_stream.Append("%s_%ld", message->openid().c_str(),
                        token_seeds.storage());
    ++token_seeds;
  } while (false);


  MessageSSResponseGetUID response;
  response.set_openid(message->openid());
  response.set_server(message->server());
  response.set_uid(uid);
  response.set_session_id(message->session_id());
  response.set_expiry_time(GetSeconds() + TOKEN_TTL);
  response.set_token(token_stream.str());
  response.set_create(create);
  response.set_channel(message->channel());
  response.set_device_id(message->device_id());
  response.set_idfa(message->idfa());
  if (error_code) {
    response.set_error_code(error_code + AUTH_SQL_ERROR);
  }

  server->SendServerMessage(entry.session_ptr.get(), MSG_SS_RESPONSE_GET_UID,
                            &response);
  return uid;
}

void WorkerThread::ProcessRegisteAccount(WorkerMessage& entry) {
  MessageSSRequestRegisteAccount* message = static_cast<MessageSSRequestRegisteAccount*>(entry.message.get());
  bool success = false;

  DefaultArrayStream stream;
  stream.Append("insert into password(account, password) values ('%s', '%s')",
                conn_.EscapeString(message->account()).c_str(),
                conn_.EscapeString(message->password()).c_str());
  int32_t result = this->conn_.ExecSql(stream.c_str(), stream.size());

  if (result < 0) {
    ERROR_LOG(logger)("ExecSql fail:%s", stream.c_str());
  } else {
    success = true;
  }

  MessageSSResponseRegisteAccount response;
  response.set_account(message->account());
  response.set_password(message->password());
  response.set_session_id(message->session_id());
  response.set_success(success);

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_REGISTE_ACCOUNT, &response);
}

void WorkerThread::ProcessAccountLogin(WorkerMessage& entry) {
  MessageSSRequestAccountLogin* message = static_cast<MessageSSRequestAccountLogin*>(entry.message.get());
  bool success = false;

  DefaultArrayStream stream;
  stream.Append("select 1 from password where account = '%s' and password = '%s'",
                conn_.EscapeString(message->account()).c_str(),
                conn_.EscapeString(message->password()).c_str());

  const boost::shared_ptr<ResultSet>& result = this->conn_.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%d, %s", result->error, stream.c_str());
  }
  if (result && result->IsValid()) {
    success = true;
  }

  MessageSSResponseAccountLogin response;
  response.set_account(message->account());
  response.set_session_id(message->session_id());
  response.set_success(success);

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_ACCOUNT_LOGIN, &response);
}

void WorkerThread::Loop() {
  static int32_t index = 0;
  int32_t thread = ++index;
  TRACE_LOG(logger)("WorkerThread LoopIndex:%02d, ThreadID:%d", thread - 1, GetThreadID());
  BlockingMessageQueue<WorkerMessage>::container_type temp_queue;
  while (true) {
    time_t begin = GetMilliSeconds();
    this->queue_.Pop(temp_queue);
    INFO_LOG(logger)("WorkerThread:%d, MessageCount:%lu, %p", thread - 1, temp_queue.size(), &this->queue_);
    for (size_t i = 0; i < temp_queue.size(); ++i) {
      WorkerMessage& entry = temp_queue[i];
      INFO_LOG(logger)("ProcessMessage:0x%04X, WorkerThread:%d", entry.msgid, thread - 1);
      if (entry.msgid == MSG_SS_REQUEST_GET_UID) this->ProcessGetUID(entry);
      else if(entry.msgid == MSG_SS_REQUEST_REGISTE_ACCOUNT) this->ProcessRegisteAccount(entry);
      else if (entry.msgid == MSG_SS_REQUEST_ACCOUNT_LOGIN) this->ProcessAccountLogin(entry);
    }
    temp_queue.clear();

    time_t end = GetMilliSeconds();

    if (end - begin < ONE_FRAME_TIME) {
      //Yield(ONE_FRAME_TIME - (end - begin));
    }
  }
}
