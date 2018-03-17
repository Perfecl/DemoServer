#include "server.h"
#include <myrandom.h>
#include <net/TcpServer.h>
#include <stdlib.h>
#include "config.h"
#include <config_file.h>
#include <cpp/server_message.pb.h>
#include <cpp/message.pb.h>
#include "trie_tree.h"
#include "rank_list.h"
#include <system.h>
#include <signal.h>
#include "army.h"
#include <storage.h>
#include <storage_ext.h>
#include <str_util.h>
#include "legion_war.h"
#include "time_activity.h"

#define PROFILE 1

const int32_t CACHED_TACTIC_KIND = 24;
const size_t CACHED_TACTIC_COUNT = 16;

TcpClient auth_client(ENTRY_TYPE_AUTH_SERVER, SSMessageDecoder);
TcpClient record_client(ENTRY_TYPE_RECORD_SERVER, SSMessageDecoder);
TcpClient center_client(ENTRY_TYPE_CENTER_SERVER, SSMessageDecoder);

//基金造假
//千分比 => MaxCount
static std::pair<int32_t, int32_t> kAddFund[] = {
  std::make_pair(10, 8),
  std::make_pair(220, 150),
  std::make_pair(220, 102),
  std::make_pair(180, 75),
  std::make_pair(180, 75),
  std::make_pair(130, 75),
  std::make_pair(80, 60),
  std::make_pair(60, 40),
};
static int32_t kAddFundCount = 0;

ServerConfig* server_config = new ServerConfig("./logic.xml");
Server* server = NULL;
boost::atomic_bool connected_record(false);

class T1;

void LoadTimeActivity() {
  intranet::MessageRequestLoadTimeActivityNew new_activity;
  new_activity.set_server_id(server_config->server_id());
  server->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_TIME_ACTIVITY_NEW,
                                &new_activity);
}

void SignalUsr1(int) {
  server->LoadConfig();
  server->SendGetServerStartDaysMessage();
  LoadTimeActivity();
}

void SignalUsr2(int) { Server::LoopFlag = false; }

Server::Server()
    : last_active_seconds_(GetSeconds()),
      report_id_(0),
      mail_id_(0),
      tid_(0),
      army_id_(0),
      server_start_time_(0),
      last_active_time_(GetTime()),
      last_virtual_time_(GetTime()),
      minutes_(0),
      server_(2),
      last_random_friend_id_(0),
      astrology_award_refresh_time_(0),
      astrology_award_country_(0),
      cross_server_refresh_time_(0),
      last_heart_beat_(0),
      sweep_stake_clear_time_(0) {
  server = this;
  army_inited_ = false;
  Mkdir("./log");
  Mkdir("./logic_db");
  logger = &Logger::InitDefaultLogger<T1>(
      FormatLogFileName(LOG_PATH, "logic").c_str(), "./log/logic.log",
      kLoggerLevel_Info);

  this->ClearPKRank();

  RANK_LIST.InitRankList();
  this->cached_tactic_.resize(CACHED_TACTIC_KIND);

  signal(SIGUSR1, SignalUsr1);
  signal(SIGUSR2, SignalUsr2);
}

bool Server::LoopFlag = true;

void Server::ClearPKRank() {
  this->pk_rank_.clear();
  for (int32_t i = 0; i < sy::MAX_ROBOT_ID - 1; ++i) {
    this->pk_rank_.push_back(i + 1);
  }
}

bool Server::ConnectToServer(TcpClient& client, const char* server_name,
                             const char* ip, const char* port) {
  bool result = client.Connect(ip, port, server_.GetIoService());
  if (!result) {
    ERROR_LOG(logger)("Connect to %s fail, %s:%s", server_name, ip, port);
  } else {
    if (client.IsValid()) {
      intranet::MessageSSServerLogin message;
      message.set_server_name("logic");
      for (size_t i = 0; i < server_config->server_ids().size(); ++i) {
        message.add_server_ids(server_config->server_ids()[i]);
      }

      SSHead head;
      head.msgid = intranet::MSG_SS_SERVER_LOGIN;
      head.dest_type = ENTRY_TYPE_SERVER;
      head.dest_id = 0;

      client.SendMessage(head, &message);
    }
    TRACE_LOG(logger)("Connect to %s success, %s:%s", server_name, ip, port);
  }
  return result;
}

void Server::SendServerInitMessage() {
  //拉取竞技场排行榜
  intranet::MessageSSRequestGetPKRankList pk_list;
  pk_list.add_server_ids(server_config->server_id());
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_GET_PK_RANK_LIST,
                              &pk_list);
  init_state_.first.set(0, true);
  //获取围剿的数据
  intranet::MessageSSRequestLoadBossList boss_list;
  boss_list.set_server_id(server_config->server_id());
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_BOSS_LIST,
                              &boss_list);
  init_state_.first.set(1, true);
  //全服商店
  //拉取全服商店数据
  intranet::MessageSSRequestLoadServerShop shop_list;
  shop_list.set_server_id(server_config->server_id());
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_SERVER_SHOP,
                              &shop_list);
  init_state_.first.set(2, true);
  //拉取排行榜数据
  RANK_LIST.SendLoadRankList();
  //拉取IP列表
  intranet::MessageSSRequestLoadIPList ip_list;
  ip_list.set_server_id(server_config->server_id());
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_IP_LIST, &ip_list);
  init_state_.first.set(3, true);
  //拉取玩家名字
  intranet::MessageSSRequestLoadAllName request_name;
  for (std::vector<uint32_t>::const_iterator iter =
           server_config->server_ids().begin();
       iter != server_config->server_ids().end(); ++iter) {
    request_name.add_server_id(*iter);
  }
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_ALL_NAME,
                              &request_name);
  init_state_.first.set(4, true);
  //拉取公告
  intranet::MessageSSRequestLoadNotice notice;
  notice.set_server_id(server_config->server_id());
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_NOTICE, &notice);
  init_state_.first.set(5, true);
  //拉取全服邮件
  intranet::MessageSSRequestLoadServerMail server_mail;
  server_mail.set_server_id(server_config->server_id());
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_SERVER_MAIL,
                              &server_mail);
  init_state_.first.set(6, true);
  //拉取军团信息
  army_inited_ = false;
  intranet::MessageSSLoadArmy request_army;
  for (std::vector<uint32_t>::const_iterator iter =
           server_config->server_ids().begin();
       iter != server_config->server_ids().end(); ++iter) {
    request_army.add_server_id(*iter);
  }
  this->SendServerMessageToDB(intranet::MSG_SS_REQUEST_LOAD_ARMY,
                              &request_army);
  init_state_.first.set(7, true);

  //拉取开服时间
  intranet::MessageSSRequestGetServerStart server_start;
  server_start.set_server_id(server_config->server_id());
  this->SendServerMessageToDB(intranet::MSG_SS_REUQEST_GET_SERVER_START,
                              &server_start);
  init_state_.first.set(8, true);
  //拉取限时活动
  LoadTimeActivity();
  init_state_.first.set(9, true);
}

void Server::TryConnectServer() {
  TRACE_LOG(logger)("Reconnect Thread %d Init", GetThreadID());
  while (true) {
    if (!auth_client.IsValid()) {
      if (this->ConnectToServer(
              auth_client, "AuthServer",
              server_config->auth_server_addr().first.c_str(),
              server_config->auth_server_addr().second.c_str())) {
        //连上认证服务器
      }
    }
    if (!center_client.IsValid()) {
      if (this->ConnectToServer(
              center_client, "CenterServer",
              server_config->center_server_addr().first.c_str(),
              server_config->center_server_addr().second.c_str())) {
        //连上中心服务器
      }
    }
    if (!record_client.IsValid()) {
      if (this->ConnectToServer(
              record_client, "RecordServer",
              server_config->record_server_addr().first.c_str(),
              server_config->record_server_addr().second.c_str())) {
        //连上了存档服务器
        this->SendServerInitMessage();

        connected_record = true;
      }
    }

    Yield(5000);
  }
}

bool Server::InitServer() {
  this->InitMessageHandler();
  AddConfigFile(server_config);

  bool load_config = ConfigFileManager::Instance().Load();
  AfterLoadConfig();
  if (!load_config) {
    ERROR_LOG(logger)("load config fail");
    return false;
  }

  Yield(1000);
  this->report_id_ = ::GenUniqueID(server_config->server_id(), this->report_id_);
  this->mail_id_ = ::GenUniqueID(server_config->server_id(), this->mail_id_);
  this->tid_ = GetSeconds() * 1000000000;

  TRACE_LOG(logger)("InitReportID:%ld", this->report_id_);
  //这边初始化服务器的监听端口
  const std::vector<ServerConfig::ListenInfo>& listen_infos =
      server_config->ports();
  for (size_t i = 0; i < listen_infos.size(); ++i) {
    const ServerConfig::ListenInfo& info = listen_infos[i];
    switch (info.second) {
      case ENTRY_TYPE_PLAYER:
        server_.Bind(info.first, info.second, CSMessageDecoder);
        break;
      default:
        server_.Bind(info.first, info.second, SSMessageDecoder);
        break;
    }
  }

  return true;
}

void Server::LoadConfig() {
  if (ConfigFileManager::Instance().Reload()) AfterLoadConfig();
}


void Server::OnSecondsChanged() {
  static bool is_connected_all_server = false;
  if (!is_connected_all_server && record_client.IsValid() &&
      center_client.IsValid() && auth_client.IsValid()) {
    TRACE_LOG(logger)("LOGIC SERVER INIT SUCCESS");
    is_connected_all_server = true;
  }

  //连不上RecordServer就自动关闭服务器
  if (connected_record && !record_client.IsValid()) {
    this->server_.Stop();
    storage::UnInit();
    ERROR_LOG(logger)("RecordServer shutdown");
    ERROR_LOG(logger)("LogicServer shutdown");
    logger->Flush();
    _exit(0);
  }
  if (this->last_active_seconds_ % 10 == 0) {
    this->SendServerMessageToDB(intranet::MSG_SS_SERVER_HEART_BEAT, NULL);
  }

#ifndef DEBUG
  time_t clock = GetClock();
  if (record_client.IsValid() && this->last_heart_beat_ &&
      clock > this->last_heart_beat_ + 60) {
    ERROR_LOG(logger)("RecordServer HeartBeat TimeOut, LastHeatBeat:%ld, CurrentSeconds:%ld"
        , this->last_heart_beat_, clock);
    logger->Flush();
    Server::LoopFlag = false;
  }
#endif

  const tm& tm_Now = GetTime();
  this->report_id_ = ::GenUniqueID(server_config->server_id(), this->report_id_);
  this->mail_id_ = ::GenUniqueID(server_config->server_id(), this->mail_id_);
  this->tid_ = GetSeconds() * 1000000000;

  if (IsWorldBossTime(GetVirtualSeconds())) {
    if (world_boss_.last_dead_time() &&
        (GetVirtualSeconds() - world_boss_.last_dead_time() > 30))
      RefreshWorldBoss(world_boss_.level() + 1);
  }

  //1分钟定时任务
  if (tm_Now.tm_min != this->last_active_time_.tm_min) {
   this->OnMinChanged();
  }

  //整点定时任务
  if (tm_Now.tm_hour != this->last_active_time_.tm_hour) {
    this->OnHourChanged();
  }

  struct tm tm_virtual;
  time_t time_sec = GetVirtualSeconds();
  localtime_r(&time_sec, &tm_virtual);
  if (this->last_virtual_time_.tm_hour != tm_virtual.tm_hour) {
    this->OnVirtualHourChanged();
  }

  this->last_active_seconds_ = GetSeconds();
  this->last_active_time_ = tm_Now;
  this->last_virtual_time_ = tm_virtual;
}

static inline void RandomAddFund() {
  int32_t days = server->GetServerStartDays();
  int32_t percent = kAddFund[0].first;
  int32_t max_count = kAddFund[0].second;
  if (days < 1) return;
  if (days >= 0 && days < ArraySize(kAddFund)) {
    percent = kAddFund[days].first;
    max_count = kAddFund[days].second;
  }
  if (server->active_tm().tm_hour >= 10) {
    int32_t count = server->GetServerShopCount(40200);
    if (count >= 500) return;
    if (RandomBetween(1, 1000) <= percent &&
        kAddFundCount++ < max_count) {
      server->UpdateServerShopCount(40200, 1);
    }
  }
}

void Server::OnMinChanged() {
 ++this->minutes_;

 //1分钟定时
 {
   RandomAddFund();
   this->SendPrizeToAllClient();
   this->EraseAllTimeOutPlayer();
 }

 //2分钟定时器
 if (this->minutes_ % 2 == 0) {
   //尝试着重新加载配置文件
   this->LoadConfig();
 }

 //5分钟定时器
 if (this->minutes_ % 5 == 0) {
  intranet::MessageSSNotifyPlayerNum msg;
  msg.set_server_id(server_config->server_id());
  msg.set_player_num(this->GetOnlinePlayerNum());
  msg.set_version(server_config->version());
  msg.set_update_time(GetVirtualSeconds());
  msg.set_open_days(server->GetServerStartDays());
  SendMessageToCenter(intranet::MSG_SS_NOTIFY_PLAYER_NUM,&msg);

  if (this->server_copy_.info_size()) {
    this->server_copy_.set_server_id(server_config->server_id());
    this->SendServerMessageToDB(intranet::MSG_SS_UPDATE_SERVER_COPY_INFO,
                                &this->server_copy_);
  }
 }
}

void Server::TrySendSweepStakeCarrierAward() {
  const ActivityInfoBase* base =
      ACTIVITY_INFO_BASE.GetEntryByID(sy::ACTIVITY_SWEEP_STAKE_CARRIER).get();
  if (!base) return;
  ValuePair2<int32_t, int32_t> param = base->IsTodayInActivty(-1);
  if (!param.v1 || !param.v2 ||
      param.v1 + param.v2 != this->GetServerStartDays())
    return;
  int32_t carrier_1_core = GetSettingValue(activity_carrier_score2);
  DefaultArrayStream stream;
  //发送俩排行榜的奖励
  const RankItemContainer& container =
      RANK_LIST.GetByType(sy::RANK_TYPE_SWEEP_STAKE_CARRIER).data();
  for (int32_t i = 0; i < container.items_size(); ++i) {
    int32_t rank = i + 1;
    const sy::RankItemInfo& info = container.items(i);
    const ActivityCarrierBase* rank_base = GetSweepStakeCarrier(rank);
    if (!rank_base) continue;
    DEBUG_LOG(logger)("SendSweepStakeCarrierRankAward, PlayerID:%ld, Rank:%d, Score:%d"
        , info.uid(), rank, info.star());
    stream.clear();
    stream.Append("%d,%d", info.star(), rank);
    // MAIL_TYPE_SWEEP_STAKE_2_2(精英榜)
    if (!rank_base->reward1.empty()) {
      LogicPlayer::SendMail(info.uid(), GetSeconds(),
                            sy::MAIL_TYPE_SWEEP_STAKE_2_1, stream.str(),
                            &rank_base->reward1);
    }
    // MAIL_TYPE_SWEEP_STAKE_2_1(普通榜)
    if (info.star() >= carrier_1_core && !rank_base->reward2.empty()) {
      LogicPlayer::SendMail(info.uid(), GetSeconds(),
                            sy::MAIL_TYPE_SWEEP_STAKE_2_2, stream.str(),
                            &rank_base->reward2);
    }
  }
}

void Server::TryClearSweepStakeCarrierAward() {
  const ActivityInfoBase* base =
      ACTIVITY_INFO_BASE.GetEntryByID(sy::ACTIVITY_SWEEP_STAKE_CARRIER).get();
  if (!base) return;
  ValuePair2<int32_t, int32_t> param = base->IsTodayInActivty(0);
  if (param.v1 == this->GetServerStartDays()) {
    if (!IsSameDay(this->sweep_stake_clear_time_, GetVirtualSeconds())) {
      RANK_LIST.ClearRank(sy::RANK_TYPE_SWEEP_STAKE_CARRIER);
      this->sweep_stake_clear_time_ = GetVirtualSeconds();
      char temp[20] = {0};
      sprintf(temp, "%ld", this->sweep_stake_clear_time_);
      this->SetKeyValue(KVStorage::kKVTypeSweepStakeClearTime, temp);
    }
  }
}

void Server::OnVirtualHourChanged() {
  struct tm tm_virtual;
  time_t time_sec = GetVirtualSeconds();
  localtime_r(&time_sec, &tm_virtual);
  this->TryClearSweepStakeCarrierAward();

  //0点重置
  if (tm_virtual.tm_hour == 0) {
    this->TrySendSweepStakeCarrierAward();
    this->ResetArmyDonateCount();
    this->SendSevenDaysRaceAward();
    this->RefreshAstrologyAward();
    if (GetSecondsDiffDays(GetVirtualSeconds(),
                           Setting::kLimitedRecruitStarttime) == 0) {
      RANK_LIST.ClearRank(sy::RANK_TYPE_RESEARCH_ITEM);
    }
    if ((GetSecondsDiffDays(GetVirtualSeconds(),
                            Setting::kLimitedRecruitEndtime) +
         1) == 0) {
      this->SendResearchItemAward();
    }
    if (server->GetServerStartDays() <= GetSettingValue(legion_foreplay_endday))
      this->SendLegionForeplayAward();
    LegionWar::Instance().LoadFromLocal();
    ACTIVITY.ClearMessage();
    this->ArmyTransfer();
  }

  this->OnHourChangePearlHarbor(tm_virtual);
  this->OnHourChangedLegionWar(tm_virtual);
  this->OnHourChangeCrossServer(tm_virtual);

  //世界BOSS刷新
  const std::vector<int32_t>& boss_day =
      Setting::GetValue1(Setting::pirate_boss_activity_time1);
  const std::vector<int32_t>& boss_hour =
      Setting::GetValue1(Setting::pirate_boss_activity_time2);
  for (size_t i = 0; i < boss_day.size(); i++) {
    if (boss_day[i] == tm_virtual.tm_wday) {
      if (tm_virtual.tm_hour == boss_hour[0]) {
        RefreshWorldBoss(1);
        for (boost::unordered_map<int64_t, ArmyPtr>::iterator iter =
                 this->armys_.begin();
             iter != this->armys_.end(); ++iter) {
          ArmyPtr& army = iter->second;
          if (!army) continue;
          army->ClearArmyMerit();
        }
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_ARMY);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_DAMAGE_UK);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_DAMAGE_US);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_DAMAGE_GE);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_DAMAGE_JP);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_MERIT_UK);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_MERIT_US);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_MERIT_GE);
        RANK_LIST.ClearRank(sy::RANK_TYPE_WORLD_BOSS_MERIT_JP);
      }
      if (tm_virtual.tm_hour == boss_hour[1]) {
        SendWorldBossArmyAward();
        SendWorldBossAward();
      }
    }
  }

  //发送21点竞技排行榜奖励
  if (tm_virtual.tm_hour == Setting::kArenaAwardTime) {
    this->Send21ClockReward();
  }
  //凌晨4点清空围剿排行榜
  if (tm_virtual.tm_hour == GetSettingValue(daily_refresh_time)) {
    //排行榜奖励
    DefaultArrayStream stream;
    std::vector<std::pair<int32_t, int32_t> > vct;
    const RankItemContainer& damageRank =
        RANK_LIST.GetByType(sy::RANK_TYPE_DSTRIKE_DAMAGE).data();
    for (int32_t i = 0; i < damageRank.items_size(); i++) {
      const DstrikeRankAwardBase* base =
          DstrikeConfigFile::GetRewardByRank(i + 1);
      if (!base)
        ERROR_LOG(logger)(" DstrikedamageRankAward Fail Rank:%d", i + 1);
      stream.clear();
      stream.Append("%d", i + 1);
      vct.clear();
      vct.push_back(
          std::make_pair(base->damage_rankaward.v1, base->damage_rankaward.v2));

      LogicPlayer::SendMail(damageRank.items(i).uid(), GetSeconds(),
                            sy::MAIL_TYPE_DSTRIKE_DAMAGE, stream.str(), &vct);
    }
    const RankItemContainer& explotRank =
        RANK_LIST.GetByType(sy::RANK_TYPE_DSTRIKE_EXPLOIT).data();
    for (int32_t i = 0; i < explotRank.items_size(); i++) {
      const DstrikeRankAwardBase* base =
          DstrikeConfigFile::GetRewardByRank(i + 1);
      if (!base)
        ERROR_LOG(logger)(" DstrikeExploitRankAward Fail Rank:%d", i + 1);
      stream.clear();
      stream.Append("%d", i + 1);
      vct.clear();
      vct.push_back(
          std::make_pair(base->merit_rankaward.v1, base->merit_rankaward.v2));

      LogicPlayer::SendMail(explotRank.items(i).uid(), GetSeconds(),
                            sy::MAIL_TYPE_DSTRIKE_EXPLOIT, stream.str(), &vct);
    }

    RANK_LIST.ClearRank(sy::RANK_TYPE_DSTRIKE_DAMAGE);
    RANK_LIST.ClearRank(sy::RANK_TYPE_DSTRIKE_EXPLOIT);
  }
}

void Server::OnHourChanged() {
  TRACE_LOG(logger)("OnHourChanged");
  TRACE_LOG(logger)("PlayerObjectCount:%d", this->players_.size());

  if (PROFILE) {
    for (VectorMap<int32_t, int32_t>::const_iterator iter =
             this->message_size_.begin();
         iter != this->message_size_.end(); ++iter) {
      TRACE_LOG(logger)("MessageSize >= %d, SendCount:%d", iter->first * 16, iter->second);
    }
    this->message_size_.clear();
  }
  logger->ChangeLoggerFile(FormatLogFileName(LOG_PATH, "logic").c_str());

  if (GetTime().tm_hour == 0) {
    kAddFundCount = 0;
    //清掉副本次数统计
    this->server_copy_.clear_info();
    this->copy_player_count_.clear();
    this->ReInitServerStartActivity();
    this->ReInitWeeklyActivity();
  }
}

//发送消息到客户端
void Server::SendMessageToClient(TcpSession* pSession, uint16_t msgid, Message* pMsg) {
  CSHead head;
  head.msgid = msgid;
  pSession->SendMessage(head, pMsg);
  if (PROFILE) ++this->message_size_[head.real_length() / 16];
  if (msgid != sy::MSG_CS_RESPONSE_HEART_BEAT)
  INFO_LOG(logger)("SendMessageToClient SessionID:%ld, MSG:0x%04X", pSession->GetSessionID(), msgid);
}

void Server::SendMessageToServer(TcpSession* pSession, uint16_t msgid,
                                 Message* pMsg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_SERVER;
  head.dest_id = 0;
  pSession->SendMessage(head, pMsg);
  INFO_LOG(logger)("SendMessageToServer SessionID:%ld, MSG:0x%04X", pSession->GetSessionID(), msgid);
}

struct Broadcast {
  Broadcast(uint16_t msgid, const char* data, int32_t length)
      : msgid(msgid), data(data), length(length), send_count(0) {}
  void operator()(LogicPlayer* player) const {
    const boost::shared_ptr<TcpSession>& session = player->session().lock();
    if (session && session->GetUID()) {
      session->SendCompressedMessage(msgid, const_cast<char*>(data), length);
      ++send_count;
    }
  }
  uint16_t msgid;
  const char* data;
  const int32_t length;
  mutable int32_t send_count;
};

struct PlayerCount {
  PlayerCount() : send_count(0) {}
  void operator()(LogicPlayer* player) const {
    if (player->is_online()) ++send_count;
  }
  mutable int32_t send_count;
};

void Server::SendMessageToAllClient(uint16_t msgid, Message* pMsg) {
  CSHead head;
  head.msgid = msgid;
  char buf[SS_MSG_MAX_LEN];
  head.length = pMsg ? pMsg->ByteSize() : 0;
  if (head.length > head.real_length()) {
    ERROR_LOG(logger)("SendMessage Fail, MSG:0x%04X, Name:%s, Len:%d",
                        head.msgid, pMsg->GetTypeName().c_str(), head.length);
    return;
  }
  *(CSHead*)buf = head;
  if (pMsg) {
    uint8_t* begin = (uint8_t*)buf + sizeof(CSHead);
    bool result = pMsg->SerializeWithCachedSizesToArray(begin);
    if (!result) {
      ERROR_LOG(logger)("SerializeMessage Fail, MSG:0x%04X, Name:%s, Len:%d",
                          head.msgid, pMsg->GetTypeName().c_str(), head.length);
      return;
    }
  }
  int32_t length = head.real_length() + sizeof(head);
  Broadcast send(msgid, buf, length);
  this->players_.ForEach(send);
  INFO_LOG(logger)("%s MSGID:%04X, Count:%d", __FUNCTION__, msgid, send.send_count);
  if (PROFILE) this->message_size_[head.real_length() / 16] += send.send_count;
}

//发送消息给认证服务器
void Server::SendMessageToAuth(uint16_t msgid, Message* pMsg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_AUTH_SERVER;
  head.dest_id = 0;

  auth_client.SendMessage(head, pMsg);
  INFO_LOG(logger)("SendMessageToAuth MSG:0x%04X", msgid);
}

//发送消息给中心服务器
void Server::SendMessageToCenter(uint16_t msgid, Message* pMsg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_CENTER_SERVER;
  head.dest_id = 0;

  center_client.SendMessage(head, pMsg);
  INFO_LOG(logger)("SendMessageToCenter MSG:0x%04X", msgid);
}

//发送消息给服务器
void Server::SendPlayerMessageToDB(int32_t player_id, uint16_t msgid,
                                   Message* pMsg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_PLAYER;
  head.dest_id = player_id;

  record_client.SendMessage(head, pMsg);
}

void Server::SendServerMessageToDB(uint16_t msgid, Message* pMsg) {
  SSHead head;
  head.msgid = msgid;
  head.dest_type = ENTRY_TYPE_RECORD_SERVER;
  head.dest_id = 0;

  record_client.SendMessage(head, pMsg);
}

void Server::EraseAllTimeOutPlayer() {
  //这边保活竞技场排行前几名
  for (int32_t i = 0; i <= ARENA_LIST_COUNT; ++i) {
    int64_t player_id = this->GetUIDByRand(i + 1);
    if (player_id <= sy::MAX_ROBOT_ID) continue;
    this->GetPlayerByID(player_id);
  }
  //干掉超时的玩家
  this->players_.EraseTimeOutPlayer();
}

int32_t Server::GetRank(int64_t uid) const {
  std::vector<int64_t>::const_iterator iter =
      std::find(this->pk_rank_.begin(), this->pk_rank_.end(), uid);
  int32_t rank = iter == this->pk_rank_.end()
                     ? (int32_t)sy::MAX_ROBOT_ID
                     : iter - this->pk_rank_.begin() + 1;
  return rank <= sy::MAX_ROBOT_ID ? rank : sy::MAX_ROBOT_ID;
}

int64_t Server::GetUIDByRand(int32_t rank) const {
  if (rank >= 1 && rank <= (int32_t)this->pk_rank_.size()) {
    return this->pk_rank_[rank - 1];
  }
  return 0;
}

int32_t Server::RefreshPKTargets(int64_t uid,
                              VectorMap<int32_t, int64_t>& __OUT__ targets) {
  targets.clear();
  int32_t rank = this->GetRank(uid);
  DEBUG_LOG(logger)("PlayerID:%ld, CurrentRank:%d", uid, rank);

  if (rank <= 11) {
    targets[1] = this->GetUIDByRand(1);
    targets[2] = this->GetUIDByRand(2);
    targets[3] = this->GetUIDByRand(3);
    targets[4] = this->GetUIDByRand(4);
    targets[5] = this->GetUIDByRand(5);
    targets[6] = this->GetUIDByRand(6);
    targets[7] = this->GetUIDByRand(7);
    targets[8] = this->GetUIDByRand(8);
    targets[9] = this->GetUIDByRand(9);
    targets[10] = this->GetUIDByRand(10);
    targets[11] = this->GetUIDByRand(11);
    targets[12] = this->GetUIDByRand(12);
    targets[13] = this->GetUIDByRand(13);
    targets.erase(rank);
  } else {
    int32_t offset = ArenaConfigFile::GetArenaRankOffset(rank);
    int32_t up_count = GetSettingValue(arena_front_num);
    int32_t down_count = GetSettingValue(arena_behind_num);
    for (int32_t i = 0; i < up_count; ++i) {
      int32_t random_rank = rank - (i + 1) * offset;
      int32_t uid = this->GetUIDByRand(random_rank);
      if (uid) targets[random_rank] = uid;
      DEBUG_LOG(logger)("RandomRank%d:%d",i + 1 , random_rank);
    }
    for (int32_t i = 0; i < down_count; ++i) {
      int32_t random_rank = rank + (i + 1) * offset;
      if (random_rank >= sy::MAX_ROBOT_ID) continue;
      int32_t uid = this->GetUIDByRand(random_rank);
      if (uid) targets[random_rank] = uid;
      DEBUG_LOG(logger)("RandomRank%d:%d", -(i + 1), random_rank);
    }
  }
  return rank;
}

void Server::InitPkRank(int64_t uid, int32_t rank) {
  if (rank <= 0 || rank >= (int32_t) this->pk_rank_.size()) {
    this->pk_rank_.push_back(uid);
  }
  if (rank < (int32_t) this->pk_rank_.size())
    this->pk_rank_.insert(this->pk_rank_.begin() + rank - 1, uid);
  this->pk_rank_.resize(sy::MAX_ROBOT_ID - 1);
}

inline bool IsRankValid(int32_t rank) {
  return rank > 0 && rank < sy::MAX_ROBOT_ID;
}

// P1打赢P2
std::pair<int32_t, int32_t> Server::P1RankUp(int64_t player_id_1,
                                               int64_t player_id_2) {
  int32_t rank_1 = this->GetRank(player_id_1);
  int32_t rank_2 = this->GetRank(player_id_2);

  //1和2都有名次, 那么1的名次没2的名次高, 就交换, 否则什么事情都不干
  //1有名次, 2没名次, 那么什么都不干
  //1没有名次, 2有名次, 那么1排名上升, 2T出去
  if (IsRankValid(rank_1) && IsRankValid(rank_2)) {
    if (rank_1 > rank_2)
      std::swap(this->pk_rank_[rank_1 - 1], this->pk_rank_[rank_2 - 1]);
  } else if (IsRankValid(rank_2)) {
    this->pk_rank_[rank_2 - 1] = player_id_1;
  }

  return std::make_pair(this->GetRank(player_id_1), this->GetRank(player_id_2));
}

void Server::Send21ClockReward() {
  static intranet::MessageSSUpdateDailyPkRankInfo request;
  request.Clear();

  int32_t rank = 1;
  for (std::vector<int64_t>::const_iterator iter = this->pk_rank_.begin();
       iter != this->pk_rank_.end(); ++iter, ++rank) {
    if (*iter <= sy::MAX_ROBOT_ID) continue;
    if (rank >= sy::MAX_ROBOT_ID) break;

    request.add_rank(rank);
    request.add_player_ids(*iter);
    request.add_time(0);

    LogicPlayer* player = this->GetPlayerByID(*iter);
    if (player) {
      player->Send21ClockPkReward(rank, 0);
    }
    INFO_LOG(logger)("SendPKRankReward PlayerID:%ld, Rank:%d", *iter, rank);

    ArenaRewardIter it_base = ArenaConfigFile::GetDailyAwardByRank(rank);
    if (it_base == ArenaConfigFile::GetDailyAwardEnd()) {
      ERROR_LOG(logger)("SendPKRankReward Fail Rank:%d", rank);
      return;
    }
    std::vector<std::pair<int32_t, int32_t> > vct;
    for (ValuePair2Vec::const_iterator it_reward = it_base->second.daily_reward.begin();
         it_reward != it_base->second.daily_reward.end(); ++it_reward) {
      vct.push_back(std::make_pair(it_reward->v1, it_reward->v2));
    }
    DefaultArrayStream stream;
    stream.Append("%d", rank);
    LogicPlayer::SendMail(*iter, GetSeconds(), sy::MAIL_TYPE_PK,
                          stream.str(), &vct);
  }
  this->SendServerMessageToDB(intranet::MSG_SS_UPDATE_DAILY_PK_RANK_INFO,
                              &request);
}

void Server::InsertIntoRobList(int32_t item_id, int64_t player_uid) {
  rob_list[item_id].insert(player_uid);
}

void Server::EraseFromRobList(int32_t item_id, int64_t player_uid) {
  if (0 == item_id) {
    for (VectorMap<int32_t, VectorSet<int64_t> >::iterator it =
             rob_list.begin();
         it != rob_list.end(); ++it) {
      it->second.erase(player_uid);
    }
  } else {
    rob_list[item_id].erase(player_uid);
  }
}
const VectorSet<int64_t>* Server::GetRobPlayers(int32_t item_id) {
  VectorMap<int32_t, VectorSet<int64_t> >::const_iterator it =
      rob_list.find(item_id);
  if (it == rob_list.end())
    return NULL;
  else
    return &it->second;
}

void Server::UpdateServerShopCount(int32_t shop_id, int32_t add_count) {
  bool found = false;
  for (int32_t i = 0; i < this->server_shop_.items_size(); ++i) {
    if (this->server_shop_.items(i).key() == shop_id) {
      int32_t count = this->server_shop_.items(i).value() + add_count;
      this->server_shop_.mutable_items(i)->set_value(count);
      found = true;
      break;
    }
  }
  if (!found) {
    sy::KVPair2* pair = this->server_shop_.add_items();
    pair->set_key(shop_id);
    pair->set_value(add_count);
  }

  intranet::MessageSSUpdateServerShop request;
  request.set_server_id(server_config->server_id());
  request.mutable_items()->CopyFrom(server->server_shop().items());
  server->SendServerMessageToDB(intranet::MSG_SS_UPDATE_SERVER_SHOP, &request);
}

int32_t Server::GetServerShopCount(int32_t shop_id) {
  for (int32_t i = 0; i < this->server_shop_.items_size(); ++i) {
    if (this->server_shop_.items(i).key() == shop_id) {
      return this->server_shop_.items(i).value();
    }
  }
  return 0;
}

int32_t Server::GetOnlinePlayerNum() {
  PlayerCount player_count;
  this->players_.ForEach(player_count);
  return player_count.send_count;
}

void Server::SendNotices(LogicPlayer& player) {
  sy::MessageNotifyNotice msg;
  for (boost::unordered_map<uint32_t, sy::NoticeInfo>::iterator it =
           notices_.begin();
       it != notices_.end(); ++it) {
    if (it->second.end_time() > GetSeconds()) {
      sy::NoticeInfo* notice = msg.add_notices();
      notice->CopyFrom(it->second);
    }
  }

  player.SendMessageToClient(sy::MSG_CS_NOTIFY_NOTICE, &msg);
}

void Server::RandomShuffleFriend(LogicPlayer* player) {
  typedef PlayerContainer<LogicPlayer>::container_type container_type;
  typedef PlayerContainer<LogicPlayer>::container_type::iterator iterator;
  container_type& container = this->players_.key_container_;
  iterator iter = container.find(this->last_random_friend_id_);
  iter = iter == container.end() ? container.begin() : iter;

  sy::MessageResponseGetRandomFriend response;
  for (; iter != container.end(); ++iter) {
    if (response.infos_size() >= 10) break;
    if (player->GetFriendInfoByID(iter->first) ||
        iter->first == player->uid() ||
        !iter->second->load_complete())
      continue;
    sy::FriendInfo* info = response.add_infos();
    iter->second->FillFriendInfo(*info);
    if (iter->second->is_online()) {
      info->set_last_active_time(0);
    }
    this->last_random_friend_id_ = iter->first;
  }
  if (iter == container.end()) this->last_random_friend_id_ = 0;
  player->SendMessageToClient(sy::MSG_CS_RESPONSE_GET_RANDOM_FRIEND, &response);
}

void Server::AddPrizeChche(const LogicPlayer* player, int32_t item_id) {
  if(!player) return;
  sy::PlayerPrizeNotice ppn;
  ppn.set_name(player->name());
  ppn.set_item_id(item_id);
  ppn.set_rank(player->rank_id());
  server->prize_cache_.push_back(ppn);
}

void Server::SendPrizeToAllClient() {
  if(prize_cache_.empty()) return;
  sy::MessageNotifyNoticePrize notify;
  for (size_t i = 0; i < prize_cache_.size(); ++i) {
    notify.add_info()->CopyFrom(prize_cache_[i]);
  }
  SendMessageToAllClient(sy::MSG_CS_NOTIFY_NOTICE_PRIZE, &notify);
  prize_cache_.clear();
}

void Server::UpdateTactic(LogicPlayer* player) {
  //缓冲的玩家阵型
  if (player->level() < 30 || !player->load_complete()) return;
  CachedTactic tactic;
  //一定得上阵6个船才能被缓存起来
  if (player->FillCachedTactic(tactic) != 6) {
    return;
  }
  std::deque<CachedTactic>& queue = this->cached_tactic_[player->uid() % CACHED_TACTIC_KIND];
  for (std::deque<CachedTactic>::iterator iter = queue.begin();
       iter != queue.end(); ++iter) {
    if ((*iter)[0] == player->uid()) {
      queue.erase(iter);
      break;
    }
  }
  queue.push_back(tactic);
  if (queue.size() > CACHED_TACTIC_COUNT)
    queue.pop_front();
}

//layer: [1,4]
void Server::RandomTactic(LogicPlayer* player, int32_t layer,
                          Array<sy::CarrierCopy, 12>& copys) {
  static CachedTactic mine;
  static std::vector<CachedTactic*> tactics;
  tactics.clear();

  for (std::vector<std::deque<CachedTactic> >::iterator iter =
           this->cached_tactic_.begin();
       iter != this->cached_tactic_.end(); ++iter) {
    for (std::deque<CachedTactic>::iterator iter_tactic = iter->begin();
         iter_tactic != iter->end(); ++iter_tactic) {
      tactics.push_back(&*iter_tactic);
    }
  }
  if (tactics.empty()) {
    player->FillCachedTactic(mine);
    tactics.push_back(&mine);
  }
  INFO_LOG(logger)("RandomTactic, PlayerID:%ld, CachedTacticCount:%lu"
      , player->uid(), tactics.size());

  copys.clear();
  sy::CarrierCopy copy;
  while (!copys.full()) {
    copy.Clear();
    const CarrierCopyBase* base = CARRIER_COPY_BASE.GetEntryByID(layer * 10 + copys.size() / 3 + 1).get();
    copy.set_index(copys.size() + 1);
    if (!base) {
      copys.clear();
      ERROR_LOG(logger)("RandomTactic, Layer:%d, Level:%lu, CarrierCopyBase not found"
          , layer, copys.size() / 3 + 1);
      break;
    }
    copy.set_fight_attr(base->fight_attr(player->max_fight_attr()));
    copy.set_level(base->level(player->level()));
    if (copy.level() <= 0) copy.set_level(1);
    copy.set_robot_id(RandomBetween(1, 2400) * layer);
    int32_t tactic_index = RandomBetween(0, tactics.size() - 1);
    CachedTactic& t = *tactics[tactic_index];
    //航母和阵型
    copy.set_carrier_id(t[1]);
    for (int32_t i = 0; i < 6; ++i) {
      copy.add_heros(t[2 + i]);
    }
    copys.push_back(copy);
  }
}

void Server::AddArmy(const sy::ArmyInfo& info) {
  ArmyPtr ptr(new Army(info));
  this->armys_[ptr->army_id()] = ptr;
  INFO_LOG(logger)("AddNewArmy, ArmyID:%ld", ptr->army_id());
}

Army* Server::GetArmyByID(int64_t army_id) {
  boost::unordered_map<int64_t, ArmyPtr>::iterator iter = this->armys_.find(army_id);
  return iter != this->armys_.end() ? iter->second.get() : NULL;
}

void Server::DeleteArmy(int64_t army_id) {
  this->armys_.erase(army_id);
}

void Server::AddArmyApply(int64_t player_id, int64_t army_id) {
  this->applies_[player_id].insert(army_id);
}

void Server::DeleteArmyApply(int64_t player_id, int64_t army_id) {
  this->applies_[player_id].erase(army_id);
}

VectorSet<int64_t>& Server::applies(int64_t player_id) {
  return this->applies_[player_id];
}

void Server::ResetArmyDonateCount() {
  for (boost::unordered_map<int64_t, ArmyPtr>::iterator iter =
           this->armys_.begin();
       iter != this->armys_.end(); ++iter) {
    ArmyPtr& army = iter->second;
    if (!army) continue;
    army->info().set_donate_time(0);
    army->info().set_donate_count(0);
    army->info().set_donate_value(0);
  }
}

void Server::UpdatePlayerArmyStatus(int64_t player_id, int64_t army_id,
                                    int64_t leave_army_time) {
  sy::MessageNotifyArmyStatus notify;
  notify.set_army_id(army_id);
  LogicPlayer* player = this->GetPlayerByID(player_id);
  if (player) {
    player->set_army_id(army_id);
    if (leave_army_time) player->set_army_leave_time(leave_army_time);
    notify.set_leave_time(player->army_leave_time());
    player->SendMessageToClient(sy::MSG_CS_NOTIFY_ARMY_STATUS, &notify);
  }

  intranet::MessageSSUpdateLeguageInfo update;
  update.set_army_id(army_id);
  update.set_leave_time(0);
  update.set_player_id(player_id);
  this->SendServerMessageToDB(intranet::MSG_SS_UPDATE_LEGUAGE_INFO, &update);
}

int32_t Server::GetFriendCount(int64_t player_id) {
  const std::string& count = this->GetPlayerValue(player_id, KVStorage::kKVTypeFriendCound);
  return count.empty() ? 0 : atoi(count.c_str());
}

void Server::SetFriendCount(int64_t player_id, int32_t count) {
  DefaultArrayStream stream1;
  stream1.Append("%d", count);
  this->SetPlayerKeyValue(player_id, KVStorage::kKVTypeFriendCound, stream1.str());
  INFO_LOG(logger)("PlayerID:%ld,FriendCount:%d", player_id, count);
}

int32_t Server::GetServerStartDays() const {
  return GetSecondsDiffDays(this->server_start_time_, GetSeconds()) + 1;
}

void Server::SendSevenDaysRaceAward() {
  TRACE_LOG(logger)("ServerStartDays:%d", this->GetServerStartDays());
  if (this->GetServerStartDays() != 8) return;
  DefaultArrayStream stream;
  std::vector<std::pair<int32_t, int32_t> > vct;

  const RankItemContainer& rank =
      RANK_LIST.GetByType(sy::RANK_TYPE_FIGHT).data();
  for (int32_t i = 0; i < 50 && i < rank.items_size(); i++) {
    int32_t rank_n = i + 1;
    const sy::RankItemInfo info = rank.items(i);
    stream.clear();
    vct.clear();
    int64_t player_id = info.uid();
    if (player_id <= sy::MAX_ROBOT_ID) continue;
    const SevenDayRaceBase* base = GetSevenDaysRace(rank_n);
    if (!base || base->reward.empty()) continue;
    INFO_LOG(logger)("SendSevenDaysRaceAward FightRank:%d, PlayerID:%ld, Fight:%ld"
        , rank_n, info.uid(), info.fight_attr());
    for (ValuePair2Vec::const_iterator iter = base->reward.begin();
         iter != base->reward.end(); ++iter) {
      vct.push_back(std::make_pair(iter->v1, iter->v2));
    }
    stream.Append("%d,%ld", rank_n, info.fight_attr());
    LogicPlayer::SendMail(player_id, GetSeconds(),
                          sy::MAIL_TYPE_SEVEN_DAYS_RACE_NEW, stream.str(), &vct);
  }
}

void Server::SetKeyValue(const std::string& key, const std::string& value) {
  storage::Set(key, value);
}

std::string Server::GetKeyValue(const std::string& key) {
  return storage::Get(key);
}

void Server::SetPlayerKeyValue(int64_t player_id, const std::string& type,
                               const std::string& value) {
  const std::string& key = MakeKVStorageKey(KVStorage::kKVPrefixPlayer, type, player_id);
  storage::Set(key, value);
}

std::string Server::GetPlayerValue(int64_t player_id, const std::string& type) {
  const std::string& key = MakeKVStorageKey(KVStorage::kKVPrefixPlayer, type, player_id);
  return storage::Get(key);
}

void Server::DeletePlayerKeyVlaue(int64_t player_id, const std::string& type) {
  const std::string& key =
      MakeKVStorageKey(KVStorage::kKVPrefixPlayer, type, player_id);
  storage::Delete(key);
}

void Server::SendGetServerStartDaysMessage() {
  intranet::MessageSSRequestGetServerStart server_start;
  server_start.set_server_id(server_config->server_id());
  server->SendServerMessageToDB(intranet::MSG_SS_REUQEST_GET_SERVER_START,
                                &server_start);
}

bool Server::IsPlayerApplyArmy(int64_t uid, int64_t army_id) {
  boost::unordered_map<int64_t, VectorSet<int64_t> >::iterator it =
      applies_.find(uid);
  if (it == applies_.end()) return false;
  return it->second.find(army_id) != it->second.end();
}

void Server::LoadLegionForeplay() {
  const std::string& value =
      this->GetKeyValue(KVStorage::kKVTypeLegionForeplay);
  this->legion_foreplayer_info_.ParseFromString(value);
  if (!this->legion_foreplayer_info_.id()) this->RefreshLegionForeplay(1);
}

void Server::RefreshLegionForeplay(int32_t id) {
  LegionForeplayCopyBase* legion_base =
      LEGION_FOREPLAY_COPY_BASE.GetEntryByID(id).get();
  if (legion_base) {
    MonsterGroupBase* group =
        MONSTER_GROUP_BASE.GetEntryByID(legion_base->monster).get();
    if (group) {
      this->legion_foreplayer_info_.set_id(id);
      this->legion_foreplayer_info_.mutable_current_hp()->Resize(6, 0);
      const std::vector<sy::HeroInfo>& heros = group->hero_info();
      int64_t max_hp = 0;
      for (size_t i = 0; i < heros.size(); i++) {
        if (!heros[i].hero_id()) continue;
        this->legion_foreplayer_info_.set_current_hp(i, heros[i].attr1(1));
        max_hp += heros[i].attr1(1);
      }
      this->legion_foreplayer_info_.set_max_hp(max_hp);
    }
  }
  this->SetKeyValue(KVStorage::kKVTypeLegionForeplay,
                    this->legion_foreplayer_info_.SerializeAsString());
}

void Server::LoadWorldBoss() {
  const std::string& value = this->GetKeyValue(KVStorage::kKVTypeServerBoss);
  world_boss_.ParseFromString(value);
  if (!world_boss_.group_id()) RefreshWorldBoss(1);
}

void Server::RefreshWorldBoss(int32_t level) {
  if (1 == level) {
    WorldBossAttrBase::Clear();
    world_boss_.clear_log();
  }

  world_boss_.set_level(level);
  world_boss_.set_last_dead_time(0);
  world_boss_.set_luck_player(0);
  world_boss_.mutable_current_hp()->Resize(6, 0);
  world_boss_.set_refresh_time(GetVirtualSeconds());
  world_boss_.set_kill_player_name("");

  const std::vector<int32_t>& monster_group =
      Setting::GetValue1(Setting::pirate_boss_monster_group);
  if (monster_group.empty()) return;
  world_boss_.set_group_id(
      monster_group[RandomBetween(0, monster_group.size() - 1)]);

  sy::CurrentCarrierInfo carrier;
  std::vector<sy::HeroInfo> monster;
  WorldBossAttrBase::FillMonsterInfos(carrier, monster, world_boss_.level(),
                                      world_boss_.group_id());

  int64_t max_hp = 0;
  for (size_t i = 0; i < monster.size(); i++) {
    if (!monster[i].hero_id()) continue;
    world_boss_.set_current_hp(i, monster[i].attr1(1));
    max_hp += monster[i].attr1(1);
  }

  world_boss_.set_max_hp(max_hp);
  world_boss_.set_fight_count(0);

  SaveWorldBoss();
}

void Server::SaveWorldBoss() {
  this->SetKeyValue(KVStorage::kKVTypeServerBoss,
                    world_boss_.SerializeAsString());
}

bool Server::IsWorldBossTime(time_t time) {
  tm tm_1;
  localtime_r(&time, &tm_1);

  bool is_valid = false;
  const std::vector<int32_t>& vct =
      Setting::GetValue1(Setting::pirate_boss_activity_time1);
  for (size_t i = 0; i < vct.size(); i++) {
    if (vct[i] == tm_1.tm_wday) is_valid = true;
  }
  if (!is_valid) return false;

  const std::vector<int32_t>& vct2 =
      Setting::GetValue1(Setting::pirate_boss_activity_time2);
  if (tm_1.tm_hour < vct2[0] || tm_1.tm_hour > vct2[1] - 1) return false;

  return true;
}

void Server::SendWorldBossArmyAward() {
  const RankItemContainer& world_boss_army_rank =
      RANK_LIST.GetByType(sy::RANK_TYPE_WORLD_BOSS_ARMY).data();
  for (int32_t i = 0; i < world_boss_army_rank.items_size(); ++i) {
    DefaultArrayStream stream;
    std::vector<std::pair<int32_t, int32_t> > award_vct;
    ModifyCurrency money(0, intranet::SYSTEM_ID_WORLD_BOSS);
    AddSubItemSet item_set;
    const WorldBossLeagueRankAwardBase* base =
        WORLD_BOSS_LEAGUE_RANK_AWARD_BASE.GetEntryByID(i + 1).get();
    if (!base) break;
    Army* army = GetArmyByID(world_boss_army_rank.items(i).uid());
    item_set.push_back(ItemParam(base->merit_rankaward, 1));
    for (AddSubItemSet::iterator it = item_set.begin(); it != item_set.end();
         ++it)
      award_vct.push_back(std::make_pair(it->item_id, it->item_count));
    for (int32_t j = sy::MoneyKind_MIN; j < sy::MoneyKind_ARRAYSIZE; ++j)
      if (money[j]) award_vct.push_back(std::make_pair(j, money[j]));
    if (!army) {
      ERROR_LOG(logger)(" Army Not Found ArmyID:%ld", world_boss_army_rank.items(i).uid());
      continue;
    }
    TRACE_LOG(logger)("WorldBoss Army Award Send armyid:%ld", army->army_id());
    stream.Append("%d", i + 1);
    for (std::vector<sy::ArmyMemberInfo>::const_iterator it =
             army->members().begin();
         it != army->members().end(); ++it)
      LogicPlayer::SendMail(it->player_id(), GetSeconds(),
                            sy::MAIL_TYPE_WROLD_BOSS_ARMY, stream.str(),
                            &award_vct);
  }
}

void Server::SendWorldBossAward() {
  for (int32_t i = sy::RANK_TYPE_WORLD_BOSS_DAMAGE_UK;
       i <= sy::RANK_TYPE_WORLD_BOSS_MERIT_JP; i++) {
    const RankItemContainer& rank = RANK_LIST.GetByType(i).data();
    for (int32_t j = 0; j < rank.items_size(); ++j) {
      DefaultArrayStream stream;
      std::vector<std::pair<int32_t, int32_t> > award_vct;
      WorldBossRankAwardBase* base =
          WORLD_BOSS_RANK_AWARD_BASE.GetEntryByID(j + 1).get();
      if (!base) break;

      int32_t mail_type = 0;
      if (i >= sy::RANK_TYPE_WORLD_BOSS_DAMAGE_UK &&
          i <= sy::RANK_TYPE_WORLD_BOSS_DAMAGE_JP) {
        mail_type = sy::MAIL_TYPE_WROLD_BOSS_DAMAGE;
        for (size_t k = 0; k < base->damage_rankaward.size(); ++k)
          award_vct.push_back(std::make_pair(base->damage_rankaward[k].v1,
                                             base->damage_rankaward[k].v2));
      } else if (i >= sy::RANK_TYPE_WORLD_BOSS_MERIT_UK &&
                 i <= sy::RANK_TYPE_WORLD_BOSS_MERIT_JP) {
        mail_type = sy::MAIL_TYPE_WROLD_BOSS_MERIT;
        for (size_t k = 0; k < base->merit_rankaward.size(); ++k)
          award_vct.push_back(std::make_pair(base->merit_rankaward[k].v1,
                                             base->merit_rankaward[k].v2));
      } else {
        ERROR_LOG(logger)(" WorldBoss Rank Type Error:%d", i);
        return;
      }
      switch (i) {
        case sy::RANK_TYPE_WORLD_BOSS_DAMAGE_UK:
        case sy::RANK_TYPE_WORLD_BOSS_MERIT_UK:
          stream.Append("%d,%d", 1, j + 1);
          break;
        case sy::RANK_TYPE_WORLD_BOSS_DAMAGE_US:
        case sy::RANK_TYPE_WORLD_BOSS_MERIT_US:
          stream.Append("%d,%d", 2, j + 1);
          break;
        case sy::RANK_TYPE_WORLD_BOSS_DAMAGE_GE:
        case sy::RANK_TYPE_WORLD_BOSS_MERIT_GE:
          stream.Append("%d,%d", 3, j + 1);
          break;
        case sy::RANK_TYPE_WORLD_BOSS_DAMAGE_JP:
        case sy::RANK_TYPE_WORLD_BOSS_MERIT_JP:
          stream.Append("%d,%d", 4, j + 1);
          break;
      }
      TRACE_LOG(logger)("WorldBoss Award Send player_id:%ld", rank.items(j).uid());
      LogicPlayer::SendMail(rank.items(j).uid(), GetSeconds(), mail_type,
                            stream.str(), &award_vct);
    }
  }
}

void Server::AddWorldBossLog(int32_t type, const std::string& name, int32_t item_id,
                             int32_t item_count) {
  sy::WorldBossLog* new_log = this->world_boss_.add_log();
  new_log->set_level(world_boss_.level());
  new_log->set_type(type);
  new_log->set_time(GetVirtualSeconds());
  new_log->set_player_name(name);
  new_log->set_item_id(item_id);
  new_log->set_item_count(item_count);

  if (this->world_boss_.log_size() > 20)
    this->world_boss_.mutable_log()->DeleteSubrange(0, 1);
}

void Server::DismissArmy(int64_t army_id) {
  intranet::MessageSSRequestDestoryArmy update;
  update.set_army_id(army_id);
  Army* army = server->GetArmyByID(army_id);
  if (!army) return;
  for (std::vector<sy::ArmyMemberInfo>::iterator iter = army->members().begin();
       iter != army->members().end(); ++iter) {
    server->UpdatePlayerArmyStatus(iter->player_id(), 0, GetVirtualSeconds());
    update.add_player_ids(iter->player_id());
    server->OnPlayerJoinArmy(iter->player_id(), NULL);
  }
  for (std::vector<sy::ArmyApplyInfo>::iterator iter = army->applies().begin();
       iter != army->applies().end(); ++iter) {
    server->DeleteArmyApply(iter->player_id(), army->army_id());
  }
  server->DeleteArmy(army->army_id());
  server->SendServerMessageToDB(intranet::MSG_SS_REQUEST_DESTORY_ARMY, &update);
}

void Server::RefreshAstrologyAward() {
  int32_t days =
      GetSecondsDiffDays(astrology_award_refresh_time_, GetVirtualSeconds());
  if (abs(days) < 2) return;

  this->astrology_award_country_++;
  if (this->astrology_award_country_ < 1 || this->astrology_award_country_ > 4)
    this->astrology_award_country_ = 1;
  this->astrology_award_refresh_time_ = GetZeroClock(GetVirtualSeconds());

  intranet::MessageSSUpdateAstrologyAward update;
  update.set_server_id(server_config->server_id());
  update.set_astrology_country_id(this->astrology_award_country_);
  update.set_astrology_refresh_time(this->astrology_award_refresh_time_);
  this->SendServerMessageToDB(intranet::MSG_SS_UPDATE_ASTROLOGY_AWARD, &update);
  TRACE_LOG(logger)("Update astrology_award_refresh_time:%ld, country:%d", this->astrology_award_refresh_time_, this->astrology_award_country_);
}

void Server::UpdateCopyStatistics(int32_t copy_type, int32_t copy_id, int64_t player_id) {
  this->copy_player_count_[copy_type].insert(player_id);
  int32_t player_count = this->copy_player_count_[copy_type].size();
  bool found = false;
  for (int32_t index = 0; index < this->server_copy_.info_size(); ++index) {
    intranet::CopyStatInfo* copy = this->server_copy_.mutable_info(index);
    if (copy->copy_type() == copy_type) {
      found = true;
      copy->set_count(copy->count() + 1);
      if (copy_id > copy->max_copy_id()) copy->set_max_copy_id(copy_id);
      copy->set_player_count(player_count);
      break;
    }
  }
  if (!found) {
    intranet::CopyStatInfo* info = this->server_copy_.add_info();
    info->set_copy_type(copy_type);
    info->set_count(1);
    info->set_max_copy_id(copy_id);
    info->set_player_count(player_count);
  }
  this->SetKeyValue(KVStorage::kKVTypeServerCopy, this->server_copy_.SerializeAsString());
}

void Server::LoadServerCopyInfo() {
  const std::string& value = this->GetKeyValue(KVStorage::kKVTypeServerCopy);
  this->server_copy_.ParseFromString(value);
  if (GetSecondsDiffDays(GetSeconds(), this->server_copy_.update_time())) {
    this->copy_player_count_.clear();
    this->server_copy_.clear_info();
    this->server_copy_.set_update_time(GetSeconds());
  }
}

void Server::LoadLocalStorage() {
  this->LoadWorldBoss();
  this->LoadServerCopyInfo();
  cross_server_refresh_time_ =
      atol(this->GetKeyValue(KVStorage::kKVTypeCrossServerRefresh).c_str());
  LegionWar::Instance().LoadFromLocal();
  this->LoadLegionForeplay();
  sweep_stake_clear_time_ =
      atol(this->GetKeyValue(KVStorage::kKVTypeSweepStakeClearTime).c_str());
}

void Server::SendResearchItemAward() {
  const RankItemContainer& rank =
      RANK_LIST.GetByType(sy::RANK_TYPE_RESEARCH_ITEM).data();
  for (int32_t i = 0; i < rank.items_size(); ++i) {
    DefaultArrayStream stream;
    stream.Append("%d,%d", rank.items(i).star(), i + 1);

    std::vector<std::pair<int32_t, int32_t> > award_vct;

    const ValuePair2Vec* awards = LimitedRecruitRankBase::GetAwardByRank(i + 1);
    if (!awards) continue;

    for (ValuePair2Vec::const_iterator it = awards->begin();
         it != awards->end(); ++it)
      award_vct.push_back(std::make_pair(it->v1, it->v2));

    TRACE_LOG(logger)("Research Item Send player_id:%ld", rank.items(i).uid());
    LogicPlayer::SendMail(rank.items(i).uid(), GetSeconds(),
                          sy::MAIL_TYPE_RESEARCH_ITEM, stream.str(),
                          &award_vct);
  }
}

void Server::OnHourChangedLegionWar(const tm& tm1) {
  const std::vector<int32_t>& open_days = Setting::GetValue1(Setting::legion_war_region_time1);
  const std::vector<int32_t>& open_hour = Setting::GetValue1(Setting::legion_war_region_time2);

  if ((tm1.tm_wday == open_days[0] - 1 && tm1.tm_hour == 0) ||
      (tm1.tm_wday == open_days[0] && tm1.tm_hour >= open_hour[0] - 1 &&
       tm1.tm_hour <= open_hour[0])) {
    TRACE_LOG(logger)("LegionWar ClearAll");
    RANK_LIST.ClearRank(sy::RANK_TYPE_LEGION_WAR_1);
    RANK_LIST.ClearRank(sy::RANK_TYPE_LEGION_WAR_2);
    RANK_LIST.ClearRank(sy::RANK_TYPE_LEGION_WAR_3);
    LegionWar::Instance().ClearAll();
  }
  //发送制霸全球占领积分
  if (std::find(open_days.begin(), open_days.end(), tm1.tm_wday) !=
          open_days.end() &&
      tm1.tm_hour >= open_hour[1]) {
    int64_t score_time = 0;
    storage_ext::Load(KVStorage::kKVTypeLegionWarScoreTime, score_time);
    DEBUG_LOG(logger)("TrySendLegionWarScore, Time:%ld", score_time);
    if (GetSecondsDiffDays(GetSeconds(), score_time)) {
      score_time = GetSeconds();
      TRACE_LOG(logger)("SendLegionWarScore, Time:%ld", score_time);
      this->SendLegionWarScore();
      storage_ext::Save(KVStorage::kKVTypeLegionWarScoreTime, score_time);
    }
  }
  //发送制霸全球排名奖励
  if (tm1.tm_wday == open_days[open_days.size() - 1] &&
      tm1.tm_hour > open_hour[1]) {
    int64_t award_time = 0;
    storage_ext::Load(KVStorage::kKVTypeLegionWarAwardTime, award_time);
    DEBUG_LOG(logger)("TrySendLegionWarAward, Time:%ld", award_time);
    if (GetSecondsDiffDays(GetSeconds(), award_time)) {
      award_time = GetSeconds();
      TRACE_LOG(logger)("SendLegionWarAward, Time:%ld", award_time);
      this->SendLegionWarAward();
      storage_ext::Save(KVStorage::kKVTypeLegionWarAwardTime, award_time);
    }
  }
}

void Server::OnHourChangeCrossServer(const tm& tm1) {
  const std::vector<int32_t>& begin =
      Setting::GetValue1(Setting::crossserverpk1_rankpk_time1);
  const std::vector<int32_t>& end =
      Setting::GetValue1(Setting::crossserverpk1_rankpk_time2);
  if (begin.size() < 2 || end.size() < 2) return;
  if (tm1.tm_wday == begin[0] && tm1.tm_hour == begin[1]) {
    RANK_LIST.ClearRank(sy::RANK_TYPE_CROSS_SERVER_UK);
    RANK_LIST.ClearRank(sy::RANK_TYPE_CROSS_SERVER_US);
    RANK_LIST.ClearRank(sy::RANK_TYPE_CROSS_SERVER_GE);
    RANK_LIST.ClearRank(sy::RANK_TYPE_CROSS_SERVER_JP);
  }
  if (tm1.tm_wday == end[0] && tm1.tm_hour == end[1]) {
    for (int32_t i = sy::RANK_TYPE_CROSS_SERVER_UK;
         i <= sy::RANK_TYPE_CROSS_SERVER_JP; i++) {
      const RankItemContainer& rank = RANK_LIST.GetByType(i).data();
      for (int32_t j = 0; j < rank.items_size(); ++j) {
        DefaultArrayStream stream;
        std::vector<std::pair<int32_t, int32_t> > award_vct;
        int32_t award_id =
            CrossServerPK1RankAwardBase::GetAwardBaseByRank(j + 1);
        if (!award_id) break;
        award_vct.push_back(std::make_pair(award_id, 1));
        int32_t mail_type = sy::MAIL_TYPE_CROSS_SERVER;
        stream.Append("%d,%d,%d", i - 19, rank.items(j).star(), j + 1);
        TRACE_LOG(logger)("Cross Server Send player_id:%ld", rank.items(j).uid());
        LogicPlayer::SendMail(rank.items(j).uid(), GetSeconds(), mail_type,
                              stream.str(), &award_vct);
      }
    }
  }
  if (LogicPlayer::IsCrossServerTime()) {
    tm tm2;
    localtime_r(&cross_server_refresh_time_, &tm2);
    if (tm2.tm_wday < begin[0] || tm2.tm_wday > end[0]) RefreshCrossServer();
    if (tm2.tm_wday == begin[0] && tm2.tm_hour < begin[1]) RefreshCrossServer();
    if (tm2.tm_wday == end[0] && tm2.tm_hour > end[1]) RefreshCrossServer();
  }
}

struct ClearCrossServer {
  void operator()(LogicPlayer* player) const { player->ClearCrossServer(); }
};

void Server::RefreshCrossServer() {
  this->cross_server_refresh_time_ = GetVirtualSeconds();
  char temp[20] = {0};
  sprintf(temp, "%ld", this->cross_server_refresh_time_);
  this->SetKeyValue(KVStorage::kKVTypeCrossServerRefresh, temp);
  storage_ext::DeleteItemByPrefixFn fn(KVStorage::kKVPrefixPlayer + ":" +
                                       KVStorage::kKVTypeCrossServer);
  storage::ForEach(fn);
  ClearCrossServer clear;
  this->players_.ForEach(clear);
}

void Server::SendCrossServerRankItem(const std::vector<sy::RankItemInfo>& info,
                             int32_t rank_type) {
  intranet::MessageSSUpdateCrossServerRankList update_rank;
  update_rank.set_rank_type(rank_type);
  update_rank.set_server_id(server_config->server_id());
  for (std::vector<sy::RankItemInfo>::const_iterator iter = info.begin();
       iter != info.end(); ++iter) {
    update_rank.add_info()->CopyFrom(*iter);
  }
  this->SendMessageToCenter(intranet::MSG_SS_UPDATE_CROSS_SERVER_RANK_LIST,
                            &update_rank);
}

void Server::SendCrossServerRankItem(const sy::RankItemInfo& info,
                                     int32_t rank_type) {
  intranet::MessageSSUpdateCrossServerRankList update_rank;
  update_rank.set_rank_type(rank_type);
  update_rank.set_server_id(server_config->server_id());
  update_rank.add_info()->CopyFrom(info);
  this->SendMessageToCenter(intranet::MSG_SS_UPDATE_CROSS_SERVER_RANK_LIST,
                            &update_rank);
}

void Server::SendLegionForeplayAward() {
  const RankItemContainer& rank =
      RANK_LIST.GetByType(sy::RANK_TYPE_LEGION_FOREPLAY).data();
  for (int32_t i = 0; i < rank.items_size(); ++i) {
    DefaultArrayStream stream;
    stream.Append("%d", i + 1);

    const LegionForeplayRankBase* base =
        LEGION_FOREPLAY_RANK_BASE.GetEntryByID(i + 1).get();
    if (!base) continue;

    std::vector<std::pair<int32_t, int32_t> > award_vct;
    for (ValuePair2Vec::const_iterator it = base->damage_rankaward.begin();
         it != base->damage_rankaward.end(); ++it)
      award_vct.push_back(std::make_pair(it->v1, it->v2));

    TRACE_LOG(logger)("legion foreplay Send player_id:%ld", rank.items(i).uid());
    LogicPlayer::SendMail(rank.items(i).uid(), GetSeconds(),
                          sy::MAIL_TYPE_LEGION_FOREPLAY, stream.str(),
                          &award_vct);
  }

  RANK_LIST.ClearRank(sy::RANK_TYPE_LEGION_FOREPLAY);
}

void Server::SendMessageToWorldBossPlayers(uint16_t msgid, Message* pMsg,
                                           int64_t except_uid) {
  for (VectorMap<int64_t, boost::weak_ptr<TcpSession> >::iterator it =
           this->world_boss_players_.begin();
       it != this->world_boss_players_.end();) {
    if (it->first == except_uid) {
      ++it;
      continue;
    }

    const boost::shared_ptr<TcpSession>& ptr = it->second.lock();
    if (!ptr) {
      it = this->world_boss_players_.erase(it);
    } else {
      this->SendMessageToClient(ptr.get(), msgid, pMsg);
      ++it;
    }
  }
}

void Server::SendMessageToForeplayPlayers(uint16_t msgid, Message* pMsg,
                                          int64_t except_uid) {
  for (VectorMap<int64_t, boost::weak_ptr<TcpSession> >::iterator it =
           this->foreplay_players_.begin();
       it != this->foreplay_players_.end();) {
    if (it->first == except_uid) {
      ++it;
      continue;
    }

    const boost::shared_ptr<TcpSession>& ptr = it->second.lock();
    if (!ptr) {
      it = this->foreplay_players_.erase(it);
    } else {
      this->SendMessageToClient(ptr.get(), msgid, pMsg);
      ++it;
    }
  }
}

void Server::OnPlayerJoinArmy(int64_t player_id, Army* army) {
  intranet::MessageSSUpdateLegionWarPlayer update_other_player;
  update_other_player.set_server_id(server_config->server_id());
  update_other_player.set_player_id(player_id);
  if (army) {
    update_other_player.set_army_id(army->army_id());
    update_other_player.set_army_name(army->name());
    update_other_player.set_army_avatar(army->info().avatar());
  }
  server->SendMessageToCenter(intranet::MSG_SS_UPDATE_LEGION_WAR_PLAYER,
                                  &update_other_player);
}

void Server::ArmyTransfer() {
  for (boost::unordered_map<int64_t, ArmyPtr>::iterator army_it =
           this->armys_.begin();
       army_it != this->armys_.end(); ++army_it) {
    sy::ArmyMemberInfo* master = NULL;
    std::vector<sy::ArmyMemberInfo*> vp;
    std::vector<sy::ArmyMemberInfo*> memb;
    std::vector<sy::ArmyMemberInfo>& members = army_it->second->members();
    for (std::vector<sy::ArmyMemberInfo>::iterator member_it = members.begin();
         member_it != members.end(); ++member_it) {
      if (member_it->position() == sy::ARMY_POSITION_MASTER)
        master = &*member_it;
      else if (member_it->position() == sy::ARMY_POSITION_VP)
        vp.push_back(&*member_it);
      else
        memb.push_back(&*member_it);
    }
    if (!master) break;
    if (GetSecondsDiffDays(master->update_time(), GetVirtualSeconds()) <=
        GetSettingValue(league_replace_day))
      break;
    vp.insert(vp.end(), memb.begin(), memb.end());

    for (size_t i = 0; i < vp.size(); i++) {
      if (GetSecondsDiffDays(vp[i]->update_time(), GetVirtualSeconds()) <= 1) {
        master->set_position(sy::ARMY_POSITION_MEMBER);
        vp[i]->set_position(sy::ARMY_POSITION_MASTER);
        army_it->second->UpdateMember(master->player_id());
        army_it->second->UpdateMember(vp[i]->player_id());
        army_it->second->SetMaster(vp[i]->player_id(), vp[i]->name());

        DefaultArrayStream stream;
        for (std::vector<sy::ArmyMemberInfo>::iterator member_it =
                 members.begin();
             member_it != members.end(); ++member_it) {
          stream.clear();
          stream.Append("%s", vp[i]->name().c_str());
          LogicPlayer::SendMail(member_it->player_id(), GetVirtualSeconds(),
                                sy::MAIL_TYPE_ARMY_TRANSFER, stream.str(),
                                NULL);
        }
        break;
      }
    }
  }
}

void Server::RefreshPearlHarbor() {
  for (boost::unordered_map<int64_t, ArmyPtr>::iterator it = armys_.begin();
       it != armys_.end(); ++it)
    it->second->RefreshPearlHarbor();
}

void Server::RefreshPearlHarborMonster(int32_t batch) {
  for (boost::unordered_map<int64_t, ArmyPtr>::iterator it = armys_.begin();
       it != armys_.end(); ++it)
    it->second->RefreshPearlHarborMonster(batch);
}

bool Server::IsPearlHarborTime() {
  tm now = GetTime();
  const std::vector<int32_t>& begin =
      Setting::GetValue1(Setting::pearlharbor_time1);
  const std::vector<int32_t>& end =
      Setting::GetValue1(Setting::pearlharbor_time2);
  if (begin.size() < 2 || end.size() < 2) return false;

  int32_t week_day = now.tm_wday;
  if (week_day == 0) week_day = 7;

  if (week_day < begin[0] || week_day > end[0]) return false;
  if (week_day == begin[0] && week_day < begin[1]) return false;
  if (week_day == end[0] && week_day > end[1]) return false;

  if (GetSettingValue(pearlharbor_start_time) < server->GetServerStartDays())
    return false;

  return true;
}

void Server::OnHourChangePearlHarbor(const tm& tm1) {
  const std::vector<int32_t>& end =
      Setting::GetValue1(Setting::pearlharbor_time2);
  const std::vector<int32_t>& setp1 =
      Setting::GetValue1(Setting::pearlharbor_time3);
  const std::vector<int32_t>& setp2 =
      Setting::GetValue1(Setting::pearlharbor_time4);
  const std::vector<int32_t>& setp3 =
      Setting::GetValue1(Setting::pearlharbor_time5);
  if (setp1.size() < 2 || setp2.size() < 2 || setp3.size() < 2 ||
      end.size() < 2)
    return;

  if (0 == tm1.tm_hour) this->RefreshPearlHarbor();
  if (setp1[0] == tm1.tm_hour) RefreshPearlHarborMonster(1);
  if (setp2[0] == tm1.tm_hour) RefreshPearlHarborMonster(2);
  if (setp3[0] == tm1.tm_hour) RefreshPearlHarborMonster(3);

  int32_t wday = end[0] + 1;
  if (wday > 6) wday -= 7;
  if (tm1.tm_wday == wday && 0 == tm1.tm_hour) PearlHarborRankAward();
}

void Server::PearlHarborRankAward() {
  DefaultArrayStream stream;
  std::vector<std::pair<int32_t, int32_t> > award_vct;
  const RankItemContainer& player_rank =
      RANK_LIST.GetByType(sy::RANK_TYPE_PEARL_HARBOR_PLAYER).data();
  for (int32_t i = 0; i < player_rank.items_size(); ++i) {
    int32_t rank = i + 1;
    PearlharborRankPersonalawardBase* base =
        PEARLHARBOR_RANK_PERSONALAWARD_BASE.GetEntryByID(rank).get();
    if (!base) continue;
    for (ValuePair2Vec::const_iterator iter = base->award.begin();
         iter != base->award.end(); ++iter)
      award_vct.push_back(std::make_pair(iter->v1, iter->v2));
    stream.clear();
    stream.Append("%d", rank);
    LogicPlayer::SendMail(player_rank.items(i).uid(), GetSeconds(),
                          sy::MAIL_TYPE_PEARL_HARBOR_PLAYER_RANK, stream.str(),
                          &award_vct);
  }
  RANK_LIST.ClearRank(sy::RANK_TYPE_PEARL_HARBOR_PLAYER);

  award_vct.clear();
  const RankItemContainer& army_rank =
      RANK_LIST.GetByType(sy::RANK_TYPE_PEARL_HARBOR_ARMY).data();
  for (int32_t i = 0; i < army_rank.items_size(); ++i) {
    int32_t rank = i + 1;
    PearlharborRankLeagueawardBase* base =
        PEARLHARBOR_RANK_LEAGUEAWARD_BASE.GetEntryByID(rank).get();
    if (!base) continue;
    for (ValuePair2Vec::const_iterator iter = base->award.begin();
         iter != base->award.end(); ++iter)
      award_vct.push_back(std::make_pair(iter->v1, iter->v2));
    stream.clear();
    stream.Append("%d", rank);
    LogicPlayer::SendMail(army_rank.items(i).uid(), GetSeconds(),
                          sy::MAIL_TYPE_PEARL_HARBOR_ARMY_RANK, stream.str(),
                          &award_vct);
  }
  RANK_LIST.ClearRank(sy::RANK_TYPE_PEARL_HARBOR_ARMY);
}

void Server::ReInitServerStartActivity() {
  ACTIVITY.ClearServerActivity();
  const std::vector<ActivityInfoConfig>& config = GetServerStartActivityByDay(this->GetServerStartDays());
  for (std::vector<ActivityInfoConfig>::const_iterator iter = config.begin();
       iter != config.end(); ++iter) {
    ACTIVITY.AddActivityTemplate(*iter);
  }

  ACTIVITY.PrintLog();
}

int32_t Server::GetServerStartWeeks() const {
  if (this->GetServerStartDays() <= 0) return -1;
  time_t t = MakeSundayTime(this->server_start_time());
  return (GetVirtualSeconds() - t) / (7 * 24 * 3600) + 1;
}

void Server::ReInitWeeklyActivity() {
  ACTIVITY.ClearWeeklyActivity();
  const std::vector<ActivityInfoConfig>& config = GetWeeklyActivityByWeeks(this->GetServerStartWeeks());
  for (std::vector<ActivityInfoConfig>::const_iterator iter = config.begin();
       iter != config.end(); ++iter) {
    ACTIVITY.AddActivityTemplate(*iter);
  }

  ACTIVITY.PrintLog();
}
