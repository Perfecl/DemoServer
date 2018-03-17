#pragma once
#include <player_container.h>
#include <boost/thread.hpp>
#include <singleton.h>
#include <common_define.h>
#include <logger.h>
#include <system.h>
#include <net/TcpSession.h>
#include <net/TcpServer.h>
#include <net/TcpClient.h>
#include <decoding.h>
#include "tokens.h"
#include "config.h"
#include "logic_player.h"
#include <array_stream.h>
#include <boost/unordered_set.hpp>
class Server;

extern TcpClient auth_client;
extern TcpClient record_client;
extern TcpClient center_client;
extern ServerConfig* server_config;
extern Server* server;

struct CSMessageEntry;
struct SSMessageEntry;
class Army;

typedef int32_t (*ServerCSMessageHandler)(CSMessageEntry&);
typedef int32_t (*ServerSSMessageHandler)(SSMessageEntry&);
typedef int32_t (LogicPlayer::*PlayerCSMessageHandler)(CSMessageEntry&);
typedef int32_t (LogicPlayer::*PlayerSSMessageHandler)(SSMessageEntry&);

namespace KVStorage {
  //前缀要保证前8个字符不能重复(如果超过8个字符)
  //可能会通过*(int64_t*)str.c_str()的方式来快速匹配
  const std::string kKVPrefixServer         = "Server";
  const std::string kKVPrefixPlayer         = "Player";
  const std::string kKVPrefixArmy           = "Army";
  const std::string kKVPrefixLegionWar      = "LegionWar";
  const std::string kKVPrefixLegionWarScore = "ScoreLegionWar";
  const std::string kKVPrefixTimeActivity   = "TimeActivityAward";

  const std::string kKVTypeFriendCound      = "FriendCount";      //好友个数
  const std::string kKVTypeArmyWarFreshTime = "ArmyWarFreshTime"; //军团战役刷新时间
  const std::string kKVTypeArmyWarCurrent   = "ArmyWarCurrent";   //军团战役当前的章节
  const std::string kKVTypeArmyWarNext      = "ArmyWarNext";      //军团战役下一个章节
  const std::string kKVTypeArmyWarCopy      = "ArmyWarCopy";      //军团战役副本
  const std::string kKVTypeArmyWarCopys     = "ArmyWarCopys";     //今天打过的军团战役副本
  const std::string kKVTypeArmyWarMax       = "ArmyWarMax";       //军团战役最高通关的章节
  const std::string kKVTypeArmyWorldBossMerit = "ArmyWorldBossMerit";  //军团世界boss荣誉
  const std::string kKVTypeServerBoss       = "ServerBoss";       //世界BOSS
  const std::string kKVTypeEliteCopy        = "EliteRandomCopy";  //精英副本的随机副本
  const std::string kKVTypeUserDefined      = "UserDefined";      //玩家自定义
  const std::string kKVTypeServerCopy       = "ServerCopyInfo";   //服务器内副本信息
  const std::string kKVTypeOtherPlayer      = "OtherPlayer";      //OtherPlayerInfo
  const std::string kKVTypeCrossServer      = "CrossServer";      //跨服积分战
  const std::string kKVTypeCrossServerRefresh = "CrossServerRefresh";    //跨服积分战刷新时间
  const std::string kKVTypeMap              = "Map";              //地图信息
  const std::string kKVTypePosition         = "Position";         //位置信息
  const std::string kKVTypeUpdateTime       = "UpdateTime";       //更新时间
  const std::string kKVTypeLegionWarScoreTime= "ScoreTimeOfLegionWar";  //制霸全球积分发送时间
  const std::string kKVTypeLegionWarAwardTime="AwardTimeOfLegionWar"; //制霸全球排行榜发奖的时间
  const std::string kKVTypeScore            = "Score";            //积分
  const std::string kKVTypeTarget           = "Target";           //目标
  const std::string kKVTypeResearchItem     = "ResearchItem";     //抽奖刷新时间
  const std::string kKVTypeLegionForeplay   = "LegionForeplay";   //制霸前戏
  const std::string kKVTypeFocus            = "Focus";            //集火目标
  const std::string kKVTypeLOG              = "Log";              //日志
  const std::string kKVTypeSweepStake       = "SweepStake";       //转盘
  const std::string kKVTypeSweepStakeClearTime = "SweepStakeClearTime";   //转盘清空时间
  const std::string kKVTypeFestivalRefreshTime = "FestivalRefreshTime";  //节日活动刷新时间
  const std::string kKVTypeGotVersion       = "GotVersion";       //获奖版本
  const std::string kKVTypePlayerComeBack   = "PlayerComeBack";   //英雄回归
  const std::string kKVTypePearlHarbor      = "PearlHarbor";      //捍卫珍珠港
};

bool IsBossValid(const sy::DstrikeBoss& boss);

class Server : NonCopyable, public Singleton<Server> {
 public:
  Server();

  void Loop();

  //发送消息到客户端
  void SendMessageToClient(TcpSession* pSession, uint16_t msgid, Message* pMsg);
  //发送消息到服务器
  void SendMessageToServer(TcpSession* pSession, uint16_t msgid, Message* pMsg);

  //发送消息给认证服务器
  void SendMessageToAuth(uint16_t msgid, Message *pMsg);

  //发送消息给中心服务器
  void SendMessageToCenter(uint16_t msgid, Message *pMsg);

  //发送消息给DB的Player
  void SendPlayerMessageToDB(int32_t player_id, uint16_t msgid, Message* pMsg);
  //发送消息给DB
  void SendServerMessageToDB(uint16_t msgid, Message* pMsg);

  //广播消息
  void SendMessageToAllClient(uint16_t, Message* pMsg);

  //false初始化失败
  bool InitServer();

  TokensSession& tokens() { return this->tokens_; }

  LogicPlayer* GetOrNewPlayer(int64_t uid) {
    if (!uid) return NULL;
    LogicPlayer* player = this->players_.GetPlayerByID(uid);
    if (player) {
      player->active();
      return player;
    }
    player = this->players_.GetOrNewPlayer(uid);
    player->set_unload();
    player->active();
    return player;
  }

  LogicPlayer* GetPlayerByID(int64_t uid) {
    if (!uid) return NULL;
    LogicPlayer *player = this->players_.GetPlayerByID(uid);
    if (player) player->active();
    return player;
  }

  void ErasePlayer(int64_t uid) { return this->players_.Erase(uid); }

  void EraseAllTimeOutPlayer();

  const PlayerContainer<LogicPlayer>& Players() const { return this->players_; }

  void AddNewName(const std::string& name, int64_t uid) {
    this->player_name_[name] = uid;
  }
  void RemoveName(const std::string& name) {
    this->player_name_.erase(name);
  }
  //通过玩家名字查找出来的UID
  //只有曾经在这个服务器上面登录的玩家才会有缓冲
  int64_t GetUIDByName(const std::string& name) const {
    boost::unordered_map<std::string, int64_t>::const_iterator iter =
        this->player_name_.find(name);
    return iter != this->player_name_.end() ? iter->second : 0;
  }

  void ClearPKRank();
  //rank从1开始
  int32_t GetRank(int64_t uid) const;
  //rank从1开始
  int64_t GetUIDByRand(int32_t rank) const;
  //刷新PK对象
  int32_t RefreshPKTargets(int64_t uid, VectorMap<int32_t, int64_t>& __OUT__ targets);
  void InitPkRank(int64_t uid, int32_t rank);
  //P1打赢P2了
  //排名上升
  std::pair<int32_t, int32_t> P1RankUp(int64_t player_id_1, int64_t player_id_2);
  //发送晚上9点奖励
  void Send21ClockReward();

  int64_t GetNewReportID() { return ++this->report_id_; }
  int64_t GetNewMailID() { return ++this->mail_id_; }
  void IncTID() { ++this->tid_; }
  int64_t GetTID() { return this->tid_; }

  void OnGet(int32_t type, const std::string& key, const std::string& value);
  //围剿boss
  int32_t GetDstrikeBossList(LogicPlayer* player,
                             sy::MessageResponseDstrikeList& message);
  void AddDstrikeBoss(const sy::DstrikeBoss& boss, bool with_save = true);
  sy::DstrikeBoss* GetDstrikeBossByPlayerId(int64_t player_id);
  //0, 正常扣血
  //1, 最后一击
  //2, boss不存在
  int32_t SubDstrikeBossHP(int64_t player_id, const std::vector<int64_t>& blood);

  void InsertIntoRobList(int32_t item_id, int64_t player_uid);
  void EraseFromRobList(int32_t item_id, int64_t player_uid);
  const VectorSet<int64_t>* GetRobPlayers(int32_t item_id);

  int32_t GetServerShopCount(int32_t shop_id);
  void UpdateServerShopCount(int32_t shop_id, int32_t add_count);
  sy::MessageResponseGetServerShopInfo& server_shop() { return this->server_shop_;}
  intranet::MessageSSResponseLoadServerMail& server_mail() { return this->server_mail_; }

  void DelNotice(int64_t tid) { this->notices_.erase(tid); }
  void AddNotice(const sy::NoticeInfo& info) { notices_[info.tid()] = info; }
  void SendNotices(LogicPlayer& player);

  const tm& active_tm() const { return this->last_active_time_; }
  boost::unordered_set<std::string>& black_list() { return this->black_list_; }
  boost::unordered_set<std::string>& white_list() { return this->white_list_; }

  //随机推荐10个好友
  void RandomShuffleFriend(LogicPlayer* player);

  void LoadConfig();

  void AddPrizeChche(const LogicPlayer *player, int32_t item_id);
  void SendPrizeToAllClient();
  void UpdateTactic(LogicPlayer* player);
  void RandomTactic(LogicPlayer* player, int32_t layer, Array<sy::CarrierCopy, 12>& copys);

  bool army_inited() const { return this->army_inited_; }
  Army* GetArmyByID(int64_t army_id);
  void AddArmy(const sy::ArmyInfo& info);
  void InitArmyID(int64_t id) {
    this->army_id_ = id;
    this->army_inited_ = true;
  }
  void DeleteArmy(int64_t army_id);
  int64_t GetNewArmyID() { return ++this->army_id_; }
  void AddArmyApply(int64_t player_id, int64_t army_id);
  void DeleteArmyApply(int64_t player_id, int64_t army_id);
  VectorSet<int64_t>& applies(int64_t player_id);
  void ResetArmyDonateCount();
  void SendSevenDaysRaceAward();

  void SendResearchItemAward();

  void SendWorldBossArmyAward();
  void SendWorldBossAward();

  void SendLegionForeplayAward();

  void AddWorldBossLog(int32_t type,const std::string& name, int32_t item_id,
                       int32_t item_count);

  //leave_army_time是0, 表示不更新
  void UpdatePlayerArmyStatus(int64_t player_id, int64_t army_id, int64_t leave_army_time);

  //获取好友个数
  int32_t GetFriendCount(int64_t player_id);
  void SetFriendCount(int64_t player_id, int32_t count);

  //获取服务器开服天数,从1开始算
  int32_t GetServerStartDays() const;
  //获取服务器的开服周数,从1开始计算
  int32_t GetServerStartWeeks() const;
  int64_t server_start_time() const { return this->server_start_time_; }
  void set_server_start_time(int64_t t) { this->server_start_time_ = t; }

  void ReInitServerStartActivity();
  void ReInitWeeklyActivity();

  //KeyValueStorage
  //KeyString => ValueString
  void SetKeyValue(const std::string& key, const std::string& value);
  std::string GetKeyValue(const std::string& key);

  //PlayerID:{PlayerID}:{Type} => ValueString
  //比如PlayerId是10001, Type是FriendCount
  //Key就是"PlayerID:10001:FriendCount"
  void SetPlayerKeyValue(int64_t player_id, const std::string& type, const std::string& value);
  std::string GetPlayerValue(int64_t player_id, const std::string& type);
  void DeletePlayerKeyVlaue(int64_t player_id, const std::string& type);

  void SendGetServerStartDaysMessage();

  typedef boost::shared_ptr<Army> ArmyPtr;
  boost::unordered_map<int64_t, ArmyPtr>& army_list() { return armys_; }

  bool IsPlayerApplyArmy(int64_t uid, int64_t army_id);

  sy::WorldBossInfo& world_boss() { return world_boss_; }
  void RefreshWorldBoss(int32_t level);
  void SaveWorldBoss();
  static bool IsWorldBossTime(time_t time);

  sy::LegionForeplayInfo& LegionForeplayInfo() {
    return this->legion_foreplayer_info_;
  }

  void DismissArmy(int64_t army_id);

  time_t AstrologyRefreshTime(){return this->astrology_award_refresh_time_;}
  int32_t AstrologyAwardCountry(){return this->astrology_award_country_;}
  void AstrologyRefreshTime(time_t t){this->astrology_award_refresh_time_ = t;}
  void AstrologyAwardCountry(int32_t id){this->astrology_award_country_ = id;}

  void UpdateCopyStatistics(int32_t copy_type, int32_t copy_id, int64_t player_id);

  void OnHourChangeCrossServer(const tm& tm1);
  void OnHourChangedLegionWar(const tm& tm1);
  void SendLegionWarAward();
  void SendLegionWarScore();
  void RefreshCrossServer();
  void RefreshLegionForeplay(int32_t id);

  void OnHourChangePearlHarbor(const tm& tm1);
  void RefreshPearlHarbor();
  void RefreshPearlHarborMonster(int32_t batch);
  bool IsPearlHarborTime();
  void PearlHarborRankAward();

  void SendCrossServerRankItem(const sy::RankItemInfo& info, int32_t rank_type);
  void SendCrossServerRankItem(const std::vector<sy::RankItemInfo>& info, int32_t rank_type);

  static bool LoopFlag;

  void SendMessageToWorldBossPlayers(uint16_t msgid, Message* pMsg,
                                     int64_t except_uid);
  void SendMessageToForeplayPlayers(uint16_t msgid, Message* pMsg,
                                    int64_t except_uid);

  void TrySendSweepStakeCarrierAward();
  void TryClearSweepStakeCarrierAward();

  VectorMap<int64_t, boost::weak_ptr<TcpSession> >& ForeplayPlayers() {
    return this->foreplay_players_;
  }
  VectorMap<int64_t, boost::weak_ptr<TcpSession> >& WorldBossPlayers() {
    return this->world_boss_players_;
  }
  void LastHeartBeat() { this->last_heart_beat_ = GetClock(); }

  void OnPlayerJoinArmy(int64_t player_id, Army* army);

  void set_init_state(int32_t index) {
    this->init_state_.second.set(index, true);
  }
  bool init_state() {
    return this->init_state_.second == this->init_state_.first;
  }
  void RefreshAstrologyAward();

 private:
  void InitMessageHandler();
  void TryConnectServer();
  //true是连接成功了
  bool ConnectToServer(TcpClient& client, const char* server_name,
                       const char* ip, const char* port);

  void OnSecondsChanged();
  void OnMinChanged();
  void OnHourChanged();
  void OnVirtualHourChanged();

  void ParseCSMessageOnce();
  void ParseSSMessageOnce();

  //获取在线人数
  int32_t GetOnlinePlayerNum();

  void LoadLocalStorage();
  void LoadWorldBoss();
  void LoadServerCopyInfo();
  void LoadLegionForeplay();

  void SendServerInitMessage();

  void ArmyTransfer();

 private:
  time_t last_active_seconds_;
  //战报,邮件,唯一ID
  int64_t report_id_;
  int64_t mail_id_;
  //每个消息处理的唯一ID
  int64_t tid_;
  int64_t army_id_;

  time_t server_start_time_;

  tm last_active_time_;
  tm last_virtual_time_;
  int32_t minutes_;
  TcpServer server_;
  boost::thread *reconnect_thread_;
  boost::thread *flush_log_thread_;
  TokensSession tokens_;

  //玩家容器
  PlayerContainer<LogicPlayer> players_;
  //玩家名字缓冲,为了减少SQL调用
  boost::unordered_map<std::string, int64_t> player_name_;

  //消息处理函数
  boost::unordered_map<uint16_t, ServerCSMessageHandler> cs_server_handler_;
  boost::unordered_map<uint16_t, ServerSSMessageHandler> ss_server_handler_;
  boost::unordered_map<uint16_t, PlayerCSMessageHandler> cs_player_handler_;
  boost::unordered_map<uint16_t, PlayerSSMessageHandler> ss_player_handler_;
  //发送消息频率
  VectorMap<int32_t, int32_t> message_size_;
  std::vector<int64_t> pk_rank_;
  //围剿BOSS列表
  typedef boost::unordered_map<int64_t, sy::DstrikeBoss> BossContainer;
  BossContainer dstrike_boss_;
  //夺宝
  VectorMap<int32_t, VectorSet<int64_t> > rob_list;
  //全服商店购买个数
  sy::MessageResponseGetServerShopInfo server_shop_;
  //全服邮件
  intranet::MessageSSResponseLoadServerMail server_mail_;
  //白黑名单
  boost::unordered_set<std::string> white_list_;
  boost::unordered_set<std::string> black_list_;
  //公告
  boost::unordered_map<uint32_t, sy::NoticeInfo> notices_;
  int64_t last_random_friend_id_;

  std::vector<sy::PlayerPrizeNotice> prize_cache_;
  //缓冲的玩家阵型
  //UID, CarrierID, HeroID[6]
  std::vector<std::deque<CachedTactic> > cached_tactic_;

  //军团
  bool army_inited_;
  boost::unordered_map<int64_t, ArmyPtr> armys_;
  //玩家的申请
  boost::unordered_map<int64_t, VectorSet<int64_t> > applies_;

  sy::WorldBossInfo world_boss_;
  //服务器内副本统计信息
  intranet::MessageSSUpdateServerCopyInfo server_copy_;
  //服务器内副本人数统计
  std::map<int32_t, VectorSet<int64_t> > copy_player_count_;

  time_t astrology_award_refresh_time_;
  int32_t astrology_award_country_;

  time_t cross_server_refresh_time_;

  sy::LegionForeplayInfo legion_foreplayer_info_;

  VectorMap<int64_t, boost::weak_ptr<TcpSession> > foreplay_players_;
  VectorMap<int64_t, boost::weak_ptr<TcpSession> > world_boss_players_;
  int64_t last_heart_beat_;

  time_t sweep_stake_clear_time_;

  boost::unordered_map<int64_t, sy::OtherPlayerInfo> other_player_info_;

  std::pair<std::bitset<64>, std::bitset<64> > init_state_;
};
