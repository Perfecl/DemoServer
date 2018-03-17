#include <net/MessageQueue.h>
#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <boost/atomic.hpp>
#include <system.h>
#include "message_handler.h"
#include "server.h"
#include <storage.h>

using namespace sy;
using namespace intranet;

boost::atomic_int64_t save_counter;
boost::atomic_int64_t load_counter;

void Server::InitMessageHandler() {
  //服务器消息
  this->server_message_handler_[MSG_SS_SERVER_LOGIN] = ProcessServerLogin;
  this->server_message_handler_[MSG_SS_REQUEST_GET_PLAYER_INFO] = ProcessLoadPlayerInfo;
  this->server_message_handler_[MSG_SS_SEND_MAIL] = ProcessSendMail;
  this->server_message_handler_[MSG_SS_REQUEST_GET_PK_RANK_LIST] = ProcessGetPKRankList;
  this->server_message_handler_[MSG_SS_REQUEST_GET_UID_BY_NAME] = ProcessGetUIDByName;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_MULTI_PLAYER] = ProcessLoadMultiPlayer;
  this->server_message_handler_[MSG_SS_UPDATE_DAILY_PK_RANK_INFO] = ProcessUpdateDailyPKRankInfo;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_RANK_LIST] = ProcessLoadRankList;
  this->server_message_handler_[MSG_SS_REQUEST_KICK_USER] = ProcessKickPlayer;
  this->server_message_handler_[MSG_SS_UPDATE_SERVER_SHOP] = ProcessUpdateServerShop;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_SERVER_SHOP] = ProcessRequestLoadServerShop;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_BOSS_LIST] = ProcessLoadDstrikeBossList;
  this->server_message_handler_[MSG_SS_SET_ACCOUNT_STATUS] = ProcessSetAccountStatus;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_SERVER_MAIL] = ProcessRequestLoadServerMail;
  this->server_message_handler_[MSG_SS_ADD_SERVER_MAIL] = ProcessRequestAddServerMail;
  this->server_message_handler_[MSG_SS_UPDATE_DSTRIKE_BOSS_INFO] = ProcessUpdateDstrikeBoss;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_IP_LIST] = ProcessLoadSetIpList;
  this->server_message_handler_[MSG_SS_SERVER_NOTICE] = ProcessSetNotice;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_NOTICE] = ProcessLoadNotice;
  this->server_message_handler_[MSG_SS_UPDATE_CREATE_PLAYER_INFO] = ProcessUpdateCreatePlayerInfo;
  this->server_message_handler_[MSG_SS_UPDATE_RANK_LIST_DETAILS] = ProcessUpdateRankListDetails;
  this->server_message_handler_[MSG_SS_UPDATE_RANK_LIST_PLAYER] = ProcessUpdateRankListPlayer;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_ARMY] = ProcessLoadArmyInfo;
  this->server_message_handler_[MSG_SS_REQUEST_CREATE_ARMY] = ProcessRequestCreateArmy;
  this->server_message_handler_[MSG_SS_REQUEST_DESTORY_ARMY] = ProcessRequestDestoryArmy;
  this->server_message_handler_[MSG_SS_UPDATE_ARMY_EXP_INFO] = ProcessUpdateArmyExpInfo;
  this->server_message_handler_[MSG_SS_UPDATE_ARMY_LOG] = ProcessUpdateArmyLog;
  this->server_message_handler_[MSG_SS_ON_JOIN_ARMY] = ProcessOnPlayerJoinArmy;
  this->server_message_handler_[MSG_SS_UPDATE_LEGUAGE_INFO] = ProcessUpdateArmyMemberStatus;
  this->server_message_handler_[MSG_SS_UPDATE_ARMY_APPLY] = ProcessUpdateArmyApply;
  this->server_message_handler_[MSG_SS_UPDATE_ARMY_NOTICE] = ProcssUpdateArmyNotice;
  this->server_message_handler_[MSG_SS_UPDATE_RECHARGE_DETAILS] = ProcessUpdatePlayerRecharge;
  this->server_message_handler_[MSG_SS_UPDATE_FIRST_RECHAGE_INFO] = ProcessUpdateFirstRechargeInfo;
  this->server_message_handler_[MSG_SS_SERVER_SET_DIALOG] = ProcessUpdateDialogID;
  this->server_message_handler_[MSG_SS_REUQEST_GET_SERVER_START] = ProcessServerStartTime;
  this->server_message_handler_[MSG_SS_UPDATE_ARMY_MEMBER] = ProcessUpdateArmyMember;
  this->server_message_handler_[MSG_SS_UPDATE_ARMY_INFO] = ProcessUpdateArmyOtherInfo;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_TIME_ACTIVITY_NEW] = ProcessResponseLoadTimeActivityNew;
  this->server_message_handler_[MSG_SS_UPDATE_ASTROLOGY_AWARD] = ProcessUpdateAstrologyAward;
  this->server_message_handler_[MSG_SS_UPDATE_SERVER_COPY_INFO] = ProcessUpdateServerCopyInfo;
  this->server_message_handler_[MSG_SS_REQUEST_QUERY_LIVELY_ACCOUNT] = ProcessRequestQueryLivelyAccount;
  this->server_message_handler_[MSG_SS_SERVER_HEART_BEAT] = ProcessRequestServerHeartBeat;
  this->server_message_handler_[MSG_SS_REQUEST_LOAD_ALL_NAME] = ProcessRequestLoadALlName;
  this->server_message_handler_[MSG_SS_UPDATE_SERVER_IP_LIST] = ProcessRequestUpdateIpList;

  //玩家消息
  this->player_message_handler_[MSG_SS_REQUEST_CREATE_PLAYER] = &RecordPlayer::ProcessCreatePlayer;
  this->player_message_handler_[MSG_SS_REQUEST_CHANGE_NAME] = &RecordPlayer::ProcessChangeName;

  this->player_message_handler_[MSG_SS_UPDATE_ITEM_INFO] = &RecordPlayer::ProcessUpdateItemInfo;
  this->player_message_handler_[MSG_SS_UPDATE_TACTIC_INFO] = &RecordPlayer::ProcessUpdateTactcInfo;
  this->player_message_handler_[MSG_SS_UPDATE_OIL_INFO] = &RecordPlayer::ProcessUpdateOilInfo;
  this->player_message_handler_[MSG_SS_UPDATE_MONEY_INFO] = &RecordPlayer::ProcessUpdateMoneyInfo;
  this->player_message_handler_[MSG_SS_UPDATE_HERO_INFO] = &RecordPlayer::ProcessUpdateHeroInfo;
  this->player_message_handler_[MSG_SS_UPDATE_CARRIER_INFO] = &RecordPlayer::ProcessUpdateCarrierInfo;
  this->player_message_handler_[MSG_SS_UPDATE_CURRENT_CARRIER_INFO] = &RecordPlayer::ProcessUpdateCurrentCarrierInfo;
  this->player_message_handler_[MSG_SS_UPDATE_COPY_INFO] = &RecordPlayer::ProcessUpdateCopyInfo;
  this->player_message_handler_[MSG_SS_UPDATE_RESEARCH_HERO_INFO] = &RecordPlayer::ProcessUpdateHeroResearchInfo;
  this->player_message_handler_[MSG_SS_UPDATE_FRESH_TIME] = &RecordPlayer::ProcessUpdateFreshTime;
  this->player_message_handler_[MSG_SS_UPDATE_EQUIP_INFO] = &RecordPlayer::ProcessUpdateEquipsInfo;
  this->player_message_handler_[MSG_SS_UPDATE_MAIL_ID] = &RecordPlayer::ProcessUpdateMailID;
  this->player_message_handler_[MSG_SS_UPDATE_SHOP_INFO] = &RecordPlayer::ProcessUpdateShopInfo;
  this->player_message_handler_[MSG_SS_UPDATE_PK_RANK_REWARD] = &RecordPlayer::ProcessUpdatePKRankRewardInfo;
  this->player_message_handler_[MSG_SS_UPDATE_PK_RANK_INFO] = &RecordPlayer::ProcessUpdatePKRankInfo;
  this->player_message_handler_[MSG_SS_REQUEST_LOAD_MULTI_PLAYER] = &RecordPlayer::ProcessRequestLoadMultiPlayer;
  this->player_message_handler_[MSG_SS_UPDATE_BUY_COUNT] = &RecordPlayer::ProcessUpdateBuyCount;
  this->player_message_handler_[MSG_SS_UPDATE_FRIEND_INFO] = &RecordPlayer::ProcessUpdateFriendInfo;//ADD BY YJX
  this->player_message_handler_[MSG_SS_UPDATE_REPORT_ABSTRACT] = &RecordPlayer::ProcessUpdateReportAbstract;
  this->player_message_handler_[MSG_SS_UPDATE_TOWER_INFO] = &RecordPlayer::ProcessUpdateTowerState;
  this->player_message_handler_[MSG_SS_UPDATE_PATROL_INFO] = &RecordPlayer::ProcessUpdatePatrolInfo;
  this->player_message_handler_[MSG_SS_GET_FRIEND_INFO] = &RecordPlayer::ProcessRequestGetFriend;
  this->player_message_handler_[MSG_SS_UPDATE_TRUCE_TIME] = &RecordPlayer::ProcessUpdateTruceTime;
  this->player_message_handler_[MSG_SS_UPDATE_FRIEND_ENERGY] = &RecordPlayer::ProcessUpdateFriendEnergy;
  this->player_message_handler_[MSG_SS_UPDATE_DSTRIKE_INFO] = &RecordPlayer::ProcessUpdateDstrikeInfo;
  this->player_message_handler_[MSG_SS_UPDATE_SIGN_IN] = &RecordPlayer::ProcessUpdateSignIn;
  this->player_message_handler_[MSG_SS_REQUEST_GET_MAIL] = &RecordPlayer::ProcessRequestGetMail;
  this->player_message_handler_[MSG_SS_UPDATE_ACHIEVEMENT] = &RecordPlayer::ProcessUpdateAchievements;
  this->player_message_handler_[MSG_SS_REQUEST_GET_MAIL_REWARD] = &RecordPlayer::ProcessRequestGetMailReward;
  this->player_message_handler_[MSG_SS_NOTIFY_GET_MAIL_REWARD] = &RecordPlayer::ProcessNotifyGetMailReward;
  this->player_message_handler_[MSG_SS_UPDATE_RANK_ID] = &RecordPlayer::ProcessUpdateRankID;
  this->player_message_handler_[MSG_SS_UPDATE_LOGIN_TIME] = &RecordPlayer::ProcessUpdateLoginTime;
  this->player_message_handler_[MSG_SS_UPDATE_DIALOG] = &RecordPlayer::ProcessUpdateDialog;
  this->player_message_handler_[MSG_SS_UPDATE_LOGIN_INFO] = &RecordPlayer::ProcessUpdateLoginInfo;
  this->player_message_handler_[MSG_SS_UPDATE_OTHER_DSTRIKE_BOSS] = &RecordPlayer::ProcessUpdateOtherDstrikeBoss;
  this->player_message_handler_[MSG_SS_UPDATE_CLIENT_FLAG] = &RecordPlayer::ProcessUpdateClientFlag;
  this->player_message_handler_[MSG_SS_UPDATE_COPY_STATUS] = &RecordPlayer::ProcessUpdateCopyStatus;
  this->player_message_handler_[MSG_SS_PLAYER_HEART_BEAT] = &RecordPlayer::ProcessHeartBeat;
  this->player_message_handler_[MSG_SS_UPDATE_CARRIER_COPY_INFO] = &RecordPlayer::ProcessUpdateCarrierCopy;
  this->player_message_handler_[MSG_SS_UPDATE_OBTAINED_CARRIERS] = &RecordPlayer::ProcessUpdateObtainedCarriers;
  this->player_message_handler_[MSG_SS_UPDATE_RECHARGE] = &RecordPlayer::ProcessUpdateRecharge;
  this->player_message_handler_[MSG_SS_UPDATE_CHANGE_AVATAR] = &RecordPlayer::ProcessChangeAvatar;
  this->player_message_handler_[MSG_SS_UPDATE_MAX_FIGHT_ATTR] = &RecordPlayer::ProcessUpdateMaxFightAttr;
  this->player_message_handler_[MSG_SS_UPDATE_LEGUAGE_INFO] = &RecordPlayer::ProcessUpdateArmyInfo;
  this->player_message_handler_[MSG_SS_UPDATE_DAILY_SIGN] = &RecordPlayer::ProcessUpdateDailySign;
  this->player_message_handler_[MSG_SS_UPDATE_MONTH_CARD] = &RecordPlayer::ProcessUpdateMonthCard;
  this->player_message_handler_[MSG_SS_UPDATE_VIP_WEEKLY] = &RecordPlayer::ProcessUpdateVIPWeekly;
  this->player_message_handler_[MSG_SS_ADD_BUY_ITEM_LOG] = &RecordPlayer::ProcessAddBuyItemLog;
  this->player_message_handler_[MSG_SS_UPDATE_ACTIVITY_RECORD_NEW] = &RecordPlayer::ProcessUpdateActivityRecordNew;
  this->player_message_handler_[MSG_SS_UPDATE_TOTAL_RECHARGE] = &RecordPlayer::ProcessUpdateTotalRecharge;
  this->player_message_handler_[MSG_SS_UPDATE_CREATE_TIME] = &RecordPlayer::ProcessUpdateCreateTime;
  this->player_message_handler_[MSG_SS_UPDATE_LOGIN_DAYS] = &RecordPlayer::ProcessUpdateLoginDays;
  this->player_message_handler_[MSG_SS_UPDATE_DEVICE_ID] = &RecordPlayer::ProcessUpdateUpdateDeviceID;
  this->player_message_handler_[MSG_SS_UPDATE_MEDAL_COPY_ID] = &RecordPlayer::ProcessUpdateMedalCopyID;
}

struct MsgCostTimeLog {
  MsgCostTimeLog(uint16_t msgid) : msgid(msgid) {
    this->millisec = GetMilliSeconds();
  }
  ~MsgCostTimeLog() {
    int64_t end_time = GetMilliSeconds();
    if (end_time - this->millisec >= 50) {
      WARN_LOG(logger)("ParseSSMessage MSGID:0x%04X, CostTime:%ldms"
          , this->msgid, end_time - this->millisec);
    }
  }
  uint16_t msgid;
  int64_t millisec;
};

//内部服务器没有CS Message
static std::vector<SSMessageEntry> ss_messages;
void Server::ParseSSMessageOnce() {
  if (!PopMessages(ss_messages)) return;
  for (size_t i = 0; i < ss_messages.size(); ++i) {
    SSMessageEntry& entry = ss_messages[i];
    MsgCostTimeLog v(entry.head.msgid);
    if (entry.head.msgid != MSG_SS_SERVER_HEART_BEAT)
      INFO_LOG(logger)("ParseSSMessage SessionID:%ld, MSG:0x%04X, DestType:%d, DestID:%ld"
        , entry.session_ptr->GetSessionID(), entry.head.msgid
        , entry.head.dest_type, entry.head.dest_id);

    //消息派发到NameThread里面去
    if (entry.head.msgid >= intranet::MSG_SS_NAME_THREAD_BEGIN &&
        entry.head.msgid <= intranet::MSG_SS_NAME_THREAD_END) {
      name_thread->PushMessage(entry);
      continue;
    }
    int32_t result = 0;
    if (entry.head.dest_type == ENTRY_TYPE_PLAYER) {
      RecordPlayer* player = server->GetPlayerByID(entry.head.dest_id);
      if (!player) {
        if (entry.head.dest_id > sy::MAX_ROBOT_ID)
          ERROR_LOG(logger)("ParseSSMessageOnce Player:%ld Not found, MSG:0x%04X",
                          entry.head.dest_id, entry.head.msgid);
        continue;
      }
      player->active();
      boost::unordered_map<uint16_t, PlayerMessageHandler>::iterator iter =
          this->player_message_handler_.find(entry.head.msgid);
      if (iter == this->player_message_handler_.end()) {
        ERROR_LOG(logger)(
            "ParseSSMessageOnce PlayerMessageHandler not found, MSG:0x%04X",
            entry.head.msgid);
        continue;
      }
      result = ((*player).*iter->second)(entry);
      if (result) {
        sy::ResultID error = sy::ResultID(result);
        ERROR_LOG(logger)(
            "ParseSSMessageOnce PlayerID:%ld, MSG:0x%04X, ResultID:%d",
            entry.head.dest_id, entry.head.msgid, result);
        MessageSSServerMessageError message;
        message.set_error_code(error);
        message.set_msg_id(entry.head.msgid);
        player->SendMessageToPlayer(MSG_SS_MESSAGE_ERROR,
                                    &message);
      }
    } else {
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
  }
  ss_messages.clear();
}

void NameThread::Loop() {
  static std::vector<SSMessageEntry> name_messages;
  while (Server::LoopFlag) {
    this->queue_.Pop(name_messages);
    time_t begin = GetMilliSeconds();
    for (size_t i = 0; i < name_messages.size(); ++i) {
      SSMessageEntry& entry = name_messages[i];
      INFO_LOG(logger)("ParseNameMessage SessionID:%ld, MSG:0x%04X",
                     entry.session_ptr->GetSessionID(), entry.head.msgid);

      int32_t result = 0;
      boost::unordered_map<uint16_t, ServerMessageHandler>::iterator iter =
          server->server_message_handler_.find(entry.head.msgid);
      if (iter == server->server_message_handler_.end()) {
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

    time_t end = GetMilliSeconds();
    if (end - begin < ONE_FRAME_TIME) {
      Yield(ONE_FRAME_TIME - (end - begin));
    }
    name_messages.clear();
  }
}

void FlushLog() {
  while (Server::LoopFlag) {
    if (logger) logger->Flush();
    if (sql_log) sql_log->Flush();
    Yield(FLUSH_LOG_FRAME_TIME);
  }
}

void Server::Loop() {
  server_.Run();
  this->flush_log_thread_ = new boost::thread(boost::bind(&FlushLog));
  for (int32_t i = 0; i < ArraySize(this->workers_); ++i) {
    this->workers_[i].Run();
  }
  for (int32_t i = 0; i < ArraySize(this->asyncs_); ++i) {
    this->asyncs_[i].Run();
  }

  name_thread_.Run();

  while (Server::LoopFlag) {
    time_t begin = GetMilliSeconds();
    if (this->last_active_seconds_ != GetSeconds()) this->OnSecondsChanged();

    ParseSSMessageOnce();
    time_t end = GetMilliSeconds();

    if (end - begin < ONE_FRAME_TIME) {
      Yield(ONE_FRAME_TIME - (end - begin));
    }
  }
  this->server_.Stop();
  sql_log->Flush();
  logger->Flush();
}

bool IsOperationalError(int32_t result) {
  if (result <= -2000 && result > -3000) return true;
  if (-1044 == result || -1045 == result || -1040 == result ||
      -1142 == result || -1143 == result)
    return true;

  return false;
}

int32_t SqlWorkerThread::ExecSingleSql(const char* sql, int32_t length) {
  time_t begin = GetMilliSeconds();
  int32_t result = 0;

  do {
    if (result) {
      ERROR_LOG(logger)("ExecSql Result:%d, %s", -result, sql);
      Yield(1000);
    }
    result = this->conn_.ExecSql(sql, length);

  } while (IsOperationalError(result));

  if (result < 0) {
    ERROR_LOG(sql_log)("%s, result:%d", sql, abs(result));
#ifdef __INTERNAL_DEBUG
    fflush(NULL);
    assert(false && "sql error, check the log");
#endif
  }
  else if (result > 0 || length < 10) {
    INFO_LOG(sql_log)("%s", sql);
  }
  else {
    TRACE_LOG(sql_log)("%s", sql);
  }
  return GetMilliSeconds() - begin;
}

void SqlWorkerThread::Loop() {
  while (!this->terminal_) {
    this->queue_.Pop(this->current_sql_);
    int32_t time = this->ExecSingleSql(this->current_sql_.c_str(),
                                       this->current_sql_.length());
    this->current_sql_.clear();
    ++save_counter;
    if (time > LOG_SQL_TIME) {
      WARN_LOG(logger)("ExecSqlTime:%dms", time);
    }
  }
}

void AsyncClosureThread::Loop() {
  BlockingMessageQueue<ClosurePtr>::container_type queue;
  while (Server::LoopFlag) {
    this->queue_.Pop(queue);
    for (size_t i = 0; i < queue.size(); ++i) {
      ClosurePtr& c = queue[i];
      c->Run();
      ++load_counter;
    }

    queue.clear();
  }
}
