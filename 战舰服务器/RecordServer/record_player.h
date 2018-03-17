#pragma once
#include <boost/atomic.hpp>
#include <player_container.h>
#include <net/TcpSession.h>
#include <vector>
#include <deque>
#include <array.h>
#include <vector_map.h>
#include <cpp/server_message.pb.h>
#include "item.h"

namespace intranet {
class MessageSSRequestCreatePlayer;
};

typedef VectorMap<int32_t, int32_t> KVType;
class MySqlConnection;

class RecordPlayer : public Player {
 public:
  RecordPlayer(int64_t uid);
  ~RecordPlayer();

  void set_unload() { this->load_complete_ = false; }
  bool loaded() const { return this->load_complete_; }

  sy::PlayerInfo& player_info() { return player_; }

  const boost::weak_ptr<TcpSession>& session() const { return this->session_; }
  void session(const boost::shared_ptr<TcpSession>& session) {
     this->session_ = session;
  }
  virtual bool can_be_delete();

  void army_id(int64_t army_id) { this->army_id_ = army_id; }
  void army_leave_time(int64_t time) { this->army_leave_time_ = time; }
  void set_army_id(int64_t army_id) { this->army_id_ = army_id; }
  void set_army_leave_time(int64_t time) { this->army_leave_time_ = time; }
  void set_dialog_id(std::string dg) { this->player_.set_dialog_id(dg); }

 public:
  //道具相关操作
  void DeleteItem(int64_t uid, int64_t tid, int32_t system, int32_t msgid);
  //如果item的item_count是0,那么就是删除
  //如果item的uid是0, 或者在缓存里面找不到, 那么就是插入
  //否则就是更新
  void UpdateItem(RecordItem* item, int64_t tid, int32_t system, int32_t msgid);

  //没有就加入,有的话就更新
  void UpdateHero(sy::HeroInfo* info, int64_t tid, int32_t system, int32_t msgid);
  void DeleteHero(int64_t hero_uid, int64_t tid, int32_t system, int32_t msgid);
  void UpdateCarrier(sy::CarrierInfo* info, int64_t tid);
  void UpdateTactic(const intranet::MessageSSRequestUpdateTacticInfo& msg);

  void UpdateCurrentCarrier(const sy::CurrentCarrierInfo* info);
  void UpdateCopyStar(
      google::protobuf::RepeatedPtrField<sy::CopyStarInfo>* info);
  void UpdateCopyProgress(const sy::CopyProgress* copy);
  void UpdateResearchHeroInfo(const sy::HeroResearchInfo* info);
  void UpdateChapterAwardInfo(const sy::ChapterAwardInfo* info);
  void UpdateGateAwardInfo(int32_t gate);
  //删除两边的好友关系
  void DeleteFriend(int64_t uid);
  void DeleteFriendCache(int64_t uid);
  void UpdateFriendInfo(const sy::FriendInfo* info, int64_t uid);
  sy::HeroInfo* GetHeroByUID(int64_t uid);

  //发送邮件
  void SendMail(sy::MailInfo& info);
  int64_t GetLastMailID() const;
 public:
  int32_t ProcessCreatePlayer(SSMessageEntry& entry);
  int32_t ProcessChangeName(SSMessageEntry& entry);

  int32_t ProcessUpdateItemInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateTactcInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateOilInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateMoneyInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateHeroInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateCarrierInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateCurrentCarrierInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateCopyInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateHeroResearchInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateFreshTime(SSMessageEntry& entry);
  int32_t ProcessUpdateEquipsInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateMailID(SSMessageEntry& entry);
  int32_t ProcessUpdateShopInfo(SSMessageEntry& entry);
  int32_t ProcessUpdatePKRankInfo(SSMessageEntry& entry);
  int32_t ProcessUpdatePKRankRewardInfo(SSMessageEntry& entry);
  int32_t ProcessRequestLoadMultiPlayer(SSMessageEntry& entry);
  int32_t ProcessUpdateBuyCount(SSMessageEntry& entry);
  int32_t ProcessUpdatePatrolInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateFriendInfo(SSMessageEntry& entry);//ADD BY YJX
  int32_t ProcessUpdateReportAbstract(SSMessageEntry& entry);
  int32_t ProcessUpdateTowerState(SSMessageEntry& entry);
  int32_t ProcessRequestGetFriend(SSMessageEntry &entry);
  int32_t ProcessUpdateTruceTime(SSMessageEntry& entry);
  int32_t ProcessUpdateFriendEnergy(SSMessageEntry& entry);
  int32_t ProcessUpdateDstrikeInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateSignIn(SSMessageEntry& entry);
  int32_t ProcessRequestGetMail(SSMessageEntry& entry);
  int32_t ProcessUpdateAchievements(SSMessageEntry& entry);
  int32_t ProcessRequestGetMailReward(SSMessageEntry& entry);
  int32_t ProcessNotifyGetMailReward(SSMessageEntry& entry);
  int32_t ProcessUpdateRankID(SSMessageEntry& entry);
  int32_t ProcessUpdateLoginTime(SSMessageEntry& entry);
  int32_t ProcessUpdateDialog(SSMessageEntry& entry);
  int32_t ProcessUpdateClientFlag(SSMessageEntry& entry);
  int32_t ProcessUpdateLoginInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateOtherDstrikeBoss(SSMessageEntry& entry);
  int32_t ProcessUpdateCopyStatus(SSMessageEntry& entry);
  int32_t ProcessHeartBeat(SSMessageEntry& entry);
  int32_t ProcessUpdateCarrierCopy(SSMessageEntry& entry);
  int32_t ProcessUpdateObtainedCarriers(SSMessageEntry& entry);
  int32_t ProcessUpdateRecharge(SSMessageEntry& entry);
  int32_t ProcessChangeAvatar(SSMessageEntry& entry);
  int32_t ProcessUpdateMaxFightAttr(SSMessageEntry& entry);
  int32_t ProcessUpdateArmyInfo(SSMessageEntry& entry);
  int32_t ProcessUpdateDailySign(SSMessageEntry& entry);
  int32_t ProcessUpdateMonthCard(SSMessageEntry& entry);
  int32_t ProcessUpdateVIPWeekly(SSMessageEntry& entry);
  int32_t ProcessAddBuyItemLog(SSMessageEntry& entry);
  int32_t ProcessUpdateTotalRecharge(SSMessageEntry& entry);
  int32_t ProcessUpdateActivityRecordNew(SSMessageEntry& entry);
  int32_t ProcessUpdateCreateTime(SSMessageEntry& entry);
  int32_t ProcessUpdateLoginDays(SSMessageEntry& entry);
  int32_t ProcessUpdateUpdateDeviceID(SSMessageEntry& entry);
  int32_t ProcessUpdateMedalCopyID(SSMessageEntry& entry);

 public:
  void SendMessageToPlayer(int16_t msgid, Message* msg);
  void ExecSqlAsync(const std::string& sql);
  std::pair<int32_t, int32_t> LoadPlayerSync(MySqlConnection& conn);
  void LoadPlayerAsync(int32_t msgid);
  void CreatePlayerAsync(intranet::MessageSSRequestCreatePlayer& entry);
  std::pair<int32_t, int32_t> CreatePlayerSync(
      intranet::MessageSSRequestCreatePlayer& msg, MySqlConnection& conn);
  void UpdateDailyPKRankInfo(int32_t rank, int32_t time);

 public:
  void SendAllInfoToClient(int32_t msgid);

  void SendPlayerInfo();
  void SendItemInfo();
  void SendHeroInfo();
  void SendCarrierInfo();
  void SendTacticInfo();
  void SendCopyInfo();
  void SendMailInfo();
  void SendShopInfo();
  void SendRewardInfo();
  void SendPatrolInfo();
  void SendFriendInfo();
  void SendReportAbstract();

 private:
  int32_t LoadPlayerInfo(MySqlConnection& conn);
  int32_t LoadItemInfo(MySqlConnection& conn);
  int32_t LoadMailInfo(MySqlConnection& conn);
  int32_t LoadHeroInfo(MySqlConnection& conn);
  int32_t LoadTacticInfo(MySqlConnection& conn);
  int32_t LoadCopyInfo(MySqlConnection& conn);
  int32_t LoadShopInfo(MySqlConnection& conn);
  int32_t LoadRewardInfo(MySqlConnection& conn);
  int32_t LoadFriendInfo(MySqlConnection& conn);
  int32_t LoadReportAbstract(MySqlConnection& conn);
  int32_t LoadCarrier(MySqlConnection& conn);
  int32_t LoadRecharge(MySqlConnection& conn);
  int32_t LoadActivityRecord(MySqlConnection& conn);

 private:
  boost::atomic_bool load_complete_;
  int32_t fresh_time_;
  int32_t last_login_time_;
  boost::weak_ptr<TcpSession> session_;

  sy::PlayerInfo player_;
  sy::CurrentCarrierInfo current_carrier_;
  RecordItemManager items_;
  int64_t last_mail_id_;
  int64_t last_server_mail_id_;
  int64_t max_fight_attr_;
  VectorMap<int32_t, std::deque<sy::MailInfo> > mails_;

  //舰船
  std::vector<sy::HeroInfo> heros_;
  //航母
  std::vector<sy::CarrierInfo> carriers_;
  //阵型
  sy::TacticInfo tactic_;

  //研发船只信息
  sy::HeroResearchInfo hero_research_;
  //副本进度
  typedef std::vector<sy::CopyProgress> CopyProgressType;
  CopyProgressType copy_progress_;
  //通关副本次数
  typedef KVType CopyCountType;
  CopyCountType copy_count_;
  //章节奖励
  KVType chapter_award_;
  std::vector<int32_t> gate_award_;

  //一次性通关副本
  std::vector<int32_t> passed_copy_;
  typedef KVType CopyStarType;
  CopyStarType copy_star_;  //副本星数
  typedef KVType BuyCountType;
  BuyCountType buy_count_;    //购买次数

  sy::EquipsInfo equips_;  //装备信息

  typedef KVType ShopType;
  VectorMap<int32_t, sy::RefreshShopInfo> refresh_info_; //刷新商店
  ShopType normal_shop_info_;  //普通物商品信息
  ShopType life_shop_info_;    //终生物品商品信息
  //竞技场
  int32_t pk_rank_times_; //今天已经挑战次数
  typedef std::vector<int32_t> PkRankType;
  PkRankType pk_rank_;    //可以挑战的人
  int32_t last_pk_rank_;  //上次竞技场排名
  int32_t pk_rank_reward_time_; //上次领取竞技场排名的时间
  int32_t pk_max_rank_;   //竞技场最高排名
  int32_t last_pk_time_;  //上次竞技时间戳

  //巡逻
  int32_t patrol_total_time_;                 //挂机总时间
  std::vector<sy::PatrolInfo> patrol_infos_;  //巡逻点信息
  //好友信息
  std::vector<sy::FriendInfo> friends_;
  //战报摘要
  VectorMap<int32_t, std::deque<std::string> > report_abstract_;
  //爬塔
  KVType tower_buff_;
  sy::TowerState tower_state_;
  //围剿BOSS
  sy::DstrikeInfo dstrike_;

  KVType achievements_;  //成就
  //航母副本
  std::vector<sy::CarrierCopy> carrier_copy;
  sy::CarrierCopyInfo carrier_copy_info;
  //已经获得过的航母
  std::vector<int32_t> obtained_carriers_;
  //最近一个月的充值
  std::vector<sy::RechargeInfo> recharge_;

  int64_t army_id_;    //军团ID
  std::vector<int32_t> army_skill_;  //军团技能
  int64_t army_leave_time_;
  std::vector<int32_t> daily_sign_;  //日常标记

  int64_t month_card_;
  int64_t big_month_card_;
  int64_t life_card_;

  VectorMap<int32_t, int32_t>  vip_weekly_;
  VectorMap<std::pair<int32_t, int64_t>, sy::ActivityRecord>
      activity_record_new_;
  int32_t month_card_1_[3];
  int32_t month_card_2_[3];
  int32_t weekly_card_[3];

  int32_t medal_copy_id_;
  std::string medal_state_;
  int32_t medal_star_;
  int32_t medal_achi_;
};
