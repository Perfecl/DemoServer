#ifndef __CALLBACK_HPP__
#define __CALLBACK_HPP__
#include "record_player.h"
#include <boost/atomic.hpp>
#include <cpp/server_message.pb.h>
#include <closure.h>
#include "server.h"

const int32_t kAsyncOperationTimeOut = 5 * 60;
typedef boost::shared_ptr<boost::atomic_int32_t> SharedCounter;

class ClosureCreatePlayer : public Closure {
 public:
  ClosureCreatePlayer(RecordPlayer* player, MySqlConnection& conn,
                      intranet::MessageSSRequestCreatePlayer& entry)
      : player_(player),
        player_id_(player->uid()),
        conn_(conn),
        entry_(entry) {}

  virtual void Run() {
    if (this->time() + kAsyncOperationTimeOut < GetSeconds()) {
      WARN_LOG(logger)("ClosureCreatePlayer:%ld TimeOut:%ld, %ld"
          , this->player_id_, this->time(), GetSeconds());
      return;
    }
    int64_t begin_time = GetMilliSeconds();
    std::pair<int32_t, int32_t> result = this->player_->CreatePlayerSync(entry_, conn_);
    int64_t end_time = GetMilliSeconds();
    INFO_LOG(logger)("CreatePlayer:%ld, CostTime:%ldms"
        , player_id_, end_time - begin_time);

    if (result.first) {
      intranet::MessageSSServerMessageError message;
      message.set_error_code(sy::ResultID(result.first));
      message.set_msg_id(intranet::MSG_SS_REQUEST_CREATE_PLAYER);
      player_->SendMessageToPlayer(intranet::MSG_SS_MESSAGE_ERROR,
                                      &message);
      return;
    }
  }
  virtual void TimeOut() {}

 private:
  RecordPlayer* player_;
  const int64_t player_id_;
  MySqlConnection& conn_;
  intranet::MessageSSRequestCreatePlayer entry_;
};

class ClosureLoadSinglePlayer : public Closure {
 public:
  ClosureLoadSinglePlayer(RecordPlayer* player, MySqlConnection& conn, int32_t msgid)
      : player_(player),
        conn_(conn),
        player_id_(player->uid()),
        msgid_(msgid) {}

  virtual void Run() {
    if (this->time() + kAsyncOperationTimeOut < GetSeconds()) {
      WARN_LOG(logger)("ClosureLoadSinglePlayer:%ld TimeOut:%ld, %ld"
          , this->player_id_, this->time(), GetSeconds());
      return;
    }
    if (player_->loaded()) {
      INFO_LOG(logger)("ClosureLoadSinglePlayer:%ld loaded", this->player_id_);
      player_->SendAllInfoToClient(msgid_);
      return;
    }

    int64_t begin = GetMilliSeconds();
    std::pair<int32_t, int32_t> result = player_->LoadPlayerSync(conn_);
    int64_t end = GetMilliSeconds();
    INFO_LOG(logger)("LoadPlayer cost:%ldms", end - begin);

    if (result.first) {
      ERROR_LOG(logger)("LoadPlayerInfo, Player:%ld result:%d, line:%d", this->player_id_, result.first, result.second);
      intranet::MessageSSResponsePlayerNotExist response;
      response.set_msgid(msgid_);
      player_->SendMessageToPlayer(intranet::MSG_SS_RESPONSE_PLAYER_NOT_EXIST, &response);
      return;
    }
    player_->SendAllInfoToClient(msgid_);
  }
  virtual void TimeOut() {}
 private:
  RecordPlayer* player_;
  MySqlConnection& conn_;
  const int64_t player_id_;
  const int32_t msgid_;
};

class ClosureLoadMultiPlayer : public Closure {
 public:
  ClosureLoadMultiPlayer(RecordPlayer* player, MySqlConnection& conn,
                         SSMessageEntry& entry,
                         intranet::MessageSSResponseLoadMultiPlayer& msg,
                         SharedCounter& counter)
      : player_(player),
        conn_(conn),
        player_id_(player->uid()),
        entry_(entry),
        response_(msg),
        complete_count_(counter) {}

  virtual void Run() {
    bool loaded = false;
    do {
      if (this->time() + kAsyncOperationTimeOut < GetSeconds()) {
        WARN_LOG(logger)("ClosureLoadMultiPlayer:%ld TimeOut:%ld, %ld"
          , this->player_id_, this->time(), GetSeconds());
        break;
      }
      if (player_->loaded()) {
        INFO_LOG(logger)("ClosureLoadMultiPlayer:%ld loaded", this->player_id_);
        loaded = true;
        break;
      }

      int64_t begin = GetMilliSeconds();
      std::pair<int32_t, int32_t> result = player_->LoadPlayerSync(conn_);
      int64_t end = GetMilliSeconds();
      INFO_LOG(logger)("LoadPlayer cost:%ldms", end - begin);

      if (result.first) {
        ERROR_LOG(logger)("LoadPlayerInfo, Player:%ld result:%d, line:%d", player_id_, result.first, result.second);
        break;
      }
      loaded = true;
    } while (false);

    intranet::MessageSSRequestLoadMultiPlayer* message =
        static_cast<intranet::MessageSSRequestLoadMultiPlayer*>(entry_.get());
    int32_t msgid = message ? message->msg_id() : 0;
    int32_t dest_count = message ? message->player_ids_size() : 1;
    if (loaded) {
      const TcpSessionPtr& old_session = player_->session().lock();
      // FIXME
      // 这边不是线程安全的
      player_->session(entry_.session_ptr);
      player_->SendAllInfoToClient(msgid);
      player_->session(old_session);
    }
    ++*complete_count_;
    TRACE_LOG(logger)("LoadMultiPlayer:%ld, %d/%d", this->player_id_, complete_count_->load(), dest_count);

    if (complete_count_->compare_exchange_strong(dest_count, 0)) {
      if (entry_.head.dest_type == ENTRY_TYPE_PLAYER) {
        server->SendPlayerMessage(entry_.session_ptr.get(), entry_.head.dest_id,
                                  intranet::MSG_SS_RESPONSE_LOAD_MULTI_PLAYER, &response_);
      } else {
        server->SendServerMessage(entry_.session_ptr.get(),
                                  intranet::MSG_SS_RESPONSE_LOAD_MULTI_PLAYER, &response_);
      }
    }
  }
  virtual void TimeOut() {}

 private:
  RecordPlayer* player_;
  MySqlConnection& conn_;
  const int64_t player_id_;
  SSMessageEntry entry_;
  intranet::MessageSSResponseLoadMultiPlayer response_;
  SharedCounter complete_count_;
};

#endif
