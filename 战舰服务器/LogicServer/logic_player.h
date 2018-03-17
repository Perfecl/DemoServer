#pragma once
#include <net/TcpSession.h>
#include <player_container.h>
#include <array.h>
#include <vector_map.h>
#include "item.h"
#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>

struct CSMessageEntry;
struct SSMessageEntry;
struct PK;
class Army;

typedef std::pair<sy::HeroInfo, HeroBasePtr> LogicHero;
typedef Array<LogicHero*, 6> TacticHeroSet;
typedef Array<int64_t, sy::AttackAttr_ARRAYSIZE> AttackAttrArray;
typedef Array<int32_t, 8> CachedTactic;
typedef Array<int32_t, sy::ArmySkill_ARRAYSIZE> ArmySkillArray;

class LogicPlayer : public Player {
  friend class Tester;
  enum {
    PLAYER_LOADING        = 1 << 0,
    PLAYER_LOAD_COMPLETE  = 1 << 1,
    PLAYER_CREATE_PLAYER  = 1 << 2,
  };

 public:
  LogicPlayer(int64_t uid);
  ~LogicPlayer();

  void set_unload() { this->status_ = 0; }
  bool load() const { return status_ & PLAYER_LOADING; }
  bool load_complete() const { return status_ & PLAYER_LOAD_COMPLETE; }
  bool create_player() const { return status_ & PLAYER_CREATE_PLAYER; }
  bool is_online() const { return !this->session_.expired(); }
  void active();
  virtual bool can_be_delete();

  const boost::weak_ptr<TcpSession>& session() const { return this->session_; }
  void session(boost::shared_ptr<TcpSession>& session) {
   this->session_ = session;
   if (session) {
     session->SetAccount(this->account());
     session->SetUID(this->uid());
   }
  }

  //增减减少道具
  int32_t CheckItem(AddSubItemSet* update_items, DeleteItemSet* delete_items,
                    NotifyItemSet* notify_set);
  int32_t ObtainItem(const AddSubItemSet* update_items,
                     const DeleteItemSet* delete_items,
                     const NotifyItemSet* notify_set, int32_t msg_id,
                     int32_t sys_id);

  void DetachShipItem(const AddSubItemSet& input, AddSubItemSet* out_items,
                      AddSubItemSet* out_ships);
  bool AddShipItem(const AddSubItemSet& ships,int32_t sys_id,int32_t msgid);

  //唯一ID是0, 表示添加/减少(增加需要判断堆叠)
  //非0表示更新
  int32_t CheckItem2(UpdateItemSet* update_set, DeleteItemSet* delete_set,
                     NotifyItemSet* notify_set);
  int32_t ObtainItem2(UpdateItemSet* update_set,
                      const DeleteItemSet* delete_set,
                      NotifyItemSet* notify_set, int32_t msg_id,
                      int32_t sys_id);

  int32_t CheckHero(const DeleteHeroSet* delete_set,
                     const NotifyHeroSet* notify_set);
  int32_t ObtainHero(const DeleteHeroSet* delete_set,
                     const NotifyHeroSet* notify_set, int32_t sys_id,
                     int32_t msg_id);

  int32_t CheckCurrency(const ModifyCurrency& modify);
  int32_t UpdateCurrency(/*const*/ ModifyCurrency& modify);
  int32_t GetCurrency(int kind);

  LogicItemManager& items() { return this->items_; }

  const std::string& name() const { return this->player_.name(); }
  int32_t rank_id() const { return this->player_.rank_id(); }
  sy::PlayerInfo& player() { return this->player_; }
  sy::HeroResearchInfo& research_hero() { return this->hero_research_info_; }
  int32_t avatar() const;
  int32_t dstrike_exploit() const;
  int64_t dstrike_damage() const;
  const sy::CurrentCarrierInfo& carrier_info() const {
    return this->current_carrier_;
  }
  int64_t army_leave_time() const { return this->army_leave_time_; }
  int64_t army_id() const { return this->army_id_; }
  void set_army_leave_time(int64_t time) { this->army_leave_time_ = time; }
  void set_army_id(int64_t army_id) { this->army_id_ = army_id; }

  int32_t research_item_point() {
    return this->achievements_[sy::OTHER_RESEARCH_ITEM_POINT];
  }

  int32_t GetDailyCount(int32_t type) { return this->daily_counter_[type]; }
  sy::ActivitySweepStake* GetSweepStake(int32_t type);
  void UpdateSweepStake(sy::ActivitySweepStake* info);

  void OnGetMonthCard(int32_t id);

  void DispatchQueryPlayerMessage(intranet::MessageSSResponseQueryOtherPlayer& msg);

  //检测账号状态
  //true是禁止登陆
  bool CheckBanToLogin();

  LogicHero* GetHeroByPos(int32_t pos);   //位置只有1到6
  LogicHero* GetHeroByUID(int64_t uid);
  void DeleteHeroByUID(int64_t uid);
  sy::CarrierInfo* GetCarrierByID(int32_t carrier_id);
  int32_t GetCopyStar(int32_t copy_id) const;
  sy::CopyProgress* GetCopyProgress(int32_t type) const;

  int32_t GetCopyBoughtCount(const CopyBase* copy_base); //获取副本购买的次数
  int32_t GetCopyStarByType(int32_t copy_type) const;  //获取副本所有的星数
  int32_t GetBoughtCount(int32_t item_id, int32_t type) const;  //获取已购买数量
  int32_t GetFriendCountByType(int32_t type);
  sy::FriendInfo* GetFriendInfoByID(int64_t uid);
  void GetAllFriends(std::vector<int64_t>& friends);

  void UpdateShopInfo(int32_t item_id, int32_t type, int32_t count);
  void UpdateFeatsShopInfo(int32_t shop_id);
  sy::DstrikeInfo& dstrikeInfo(){return this->dstrike_info_;} 
  void UpdateBuyCount(int32_t type,int32_t count);
  void UpdateAchievement(int32_t type, int32_t value);

  int32_t ArenaPK(sy::MessageRequestTestPkTarget* message);
  int32_t TestPK(int64_t player_id);

  int32_t AgreeAddFriend(sy::MessageRequestAgreeAddFriend* message);
  int32_t GetTodayRechargeNum(time_t day);
  bool TodayHasRechargeOrder(int32_t money);
  bool TodayHasRechargeOrderID(int32_t good_id);
  static void ChangeCrossInfoPlayer(sy::OtherPlayerInfo& info);
  static bool IsCrossServerTime();
  void ClearCrossServer();
  void UpdateMonthCard(bool notify_client);

 public:
  //常用的属性
  int32_t level() const { return this->player_.level(); }
  int32_t vip_level() const { return this->player_.vip_level(); }
  int32_t carrier_id() const { return this->current_carrier_.carrier_id(); }
  int32_t max_pk_rank()  const { return this->pk_max_rank_; }
  int32_t create_days() const;
  int32_t& arena_time() { return this->last_pk_time_; }
  //玩家战斗力
  int64_t fight_attr() const { return this->fight_attr_; }
  int64_t max_fight_attr() const { return this->max_fight_attr_; }
  const sy::CarrierCopyInfo& carrier_copy_info() const { return this->carrier_copy_info_; }
  //军团战役攻打次数
  int32_t army_war_count() const;

  void UpdateHeroResearchInfo();
  void Send21ClockPkReward(int32_t rank, int32_t time);
  void UpdatePKRankInfo();
  int32_t GetCopyCount(int32_t copy_id) const;
  void UpdateMoneyInfo(/*const*/ ModifyCurrency& modify);

  //这俩个设置等级的函数都会修改经验为0
  void SetLevel(int32_t level);
  void SetVipLevel(int32_t vip_level);

  void MakePlayerSimpleInfo(sy::PlayerSimpleInfo* simple);
  void MakeArmyMemberInfo(sy::ArmyMemberInfo* info);
  void UpdateArmyMember();
  //填充好友信息, 返回变化属性的数量
  int32_t FillFriendInfo(sy::FriendInfo& info);
  //填充缓存阵型信息
  int32_t FillCachedTactic(CachedTactic& tactic);

  //更新军团成员信息
  void UpdateArmyMemberInfo(const sy::ArmyMemberInfo& info);

  //money是实际充值的元宝
  //系统赠送的通过money计算出来
  //add_money可以控制是否增加VIP经验和元宝
  int32_t AddRecharge(int32_t time, std::string& goodid, int32_t money,
                      bool add_money, int32_t msgid);
  int32_t OnRechargeItem(int32_t time, const std::string& goodid, int32_t money,
                         int32_t msgid);
  int32_t OnRecharge1RMB(int32_t time, const std::string& goodid, int32_t money,
                         int32_t msgid);
  int32_t OnDailyRechargeItem(int32_t time, const std::string& goodid,
                              int32_t money, int32_t msgid);
  int32_t RechargeActivity1(int32_t time, const std::string& goodid, int32_t money,
                        int32_t msgid);
  int32_t OnWeeklyCard(int32_t goodid);
  //增加充值记录
  void AddRechargeInfo(int32_t time, int32_t money, int32_t goodid);

  void SetDialog(std::string dialog) { this->player_.set_dialog_id(dialog); }

 private:
  void RefreshAllSystem();
  void UpdateOilInfo(bool force, int32_t sys_id, int32_t msgid);
  void UpdateDstrikeInfo(bool force = false);
  bool UpdateHeroInfo(std::vector<sy::HeroInfo>& infos,int32_t sys_id,int32_t msg_id);
  bool UpdateHeroInfo(sy::HeroInfo& info, int32_t notify,int32_t sys_id,int32_t msg_id);

  void UpdateCopyInfo(int32_t copy_id, int32_t star,
                      __OUT__ sy::MessageNotifyCopyInfo* notify,
                      __OUT__ intranet::MessageSSUpdateCopyInfo* msg);
  void UpdateCopyProgress(const sy::CopyProgress& progress,
                          __OUT__ sy::MessageNotifyCopyInfo* notify,
                          __OUT__ intranet::MessageSSUpdateCopyInfo* msg);
  int32_t GetCurrentCopyProgress(int32_t copy_type) const;
  bool GetPassedCopy(int32_t copy_id) const {
    return std::find(this->passed_copy_.begin(), this->passed_copy_.end(),
                     copy_id) != this->passed_copy_.end();
  }
  void UpdatePassedCopy(int32_t copy_id,
                        __OUT__ sy::MessageNotifyCopyInfo* notify,
                        __OUT__ intranet::MessageSSUpdateCopyInfo* msg);
  void UpdateCopyCount(int32_t copy_id,
                       __OUT__ sy::MessageNotifyCopyInfo* notify,
                       __OUT__ intranet::MessageSSUpdateCopyInfo* msg);
  void SetAllCopyStar(const sy::CopyProgress& progress);

  int32_t GetItemEquipedPos(LogicItem* item);
  google::protobuf::RepeatedField<int64_t>* GetEquipsInfo(int32_t pos);
  void UpdateEquipsInfo(int32_t pos);
  void UpdatePatrolinfo(int32_t patrol_id, sy::PatrolInfo* out);

  int32_t CheckBuyCondition(const ShopBase* base);

  void UpdateVIPDailyWeekly();

  //增加道具, 并添加到容器里面
  //需要调用Obtain函数通知给客户端和存档服务器, 否则会丢失数据
  LogicItem* AddItem(int32_t item_id, int32_t item_count, sy::Item* attr = NULL);

  int32_t AddNavyExp(LogicItem* navy, int32_t exp);
  void AddHeroExp(LogicHero* ship, int32_t exp);
  void AddCarrier(int32_t carrier_id);
  void AddCarrierExp(sy::CarrierInfo* carrier, int32_t exp);
  void UpdateCarrier(sy::CarrierInfo* info);

  bool GetDailySign(sy::DailySign type);
  void SetDailySign(sy::DailySign type, bool flag);
  void ResetDailySign();
  void UpdateDailySign();
  void RefreshSweepStake();

  int32_t ProcessGM(sy::GMContent* info);

  void OnExpChanged();
  void OnVipExpChanged();
  void OnLevelChanged(int32_t from_level, int32_t dest_level);
  void OnVipLevelChanged(int32_t from_level, int32_t dest_level);
  void OnEquipChanged(int64_t hero_uid);
  void CalcEquipsCondition();
  void OnPlayerLogIn();
  void OnPlayerLogOut();
  void OnBuyCount(int32_t count_type);
  void OnBuyTowerCount();
  void OnBuyCarrierCopyCount();
  void OnLegionWarWin(int32_t city, int32_t pos);

  int32_t CheckEnterCopy(const CopyBase* copy_base);
  void OnCopyPassed(int32_t star, int32_t first_blood,
                       const CopyBase* copy_base);
  void FillCopyAward(int32_t win, const CopyBase* copy_base,
                     ModifyCurrency& modify, AddSubItemSet& item_set,
                     int32_t& first_blood, sy::MessageNotifyFightResult& notify,
                     bool auto_flop);
  void OnItemCountChanged(int32_t item_id, int32_t count);

  enum {
    kNotifyNone = 0,
    kNotifyClient = 1,
    kNotifyServer = 2,
    kNotifyAll = kNotifyClient | kNotifyServer,
  };

  void CalcTowerAttr();
  void CalcCarrier(int32_t notify);

  void CalcHeroAttr(LogicHero *info, TacticHeroSet* set, int32_t notify);
  void CalcHeroAtTacticAttr(LogicHero* info,
                            google::protobuf::RepeatedField<int64_t>* equips,
                            TacticHeroSet& ships_set,
                            AttackAttrArray& attr);
  void CalcHeroPercent(sy::HeroInfo* info);
  void SendOtherPlayerToCenter(bool forece);
  //上阵船只属性
  void CalcTacticAttr(int32_t notify, int32_t sys_id, int32_t msgid);
  //添加军团技能属性
  void AddLeagueAttr(AttackAttrArray& attr);
  //判断船只是否在阵型中
  int32_t IsInTactic(int64_t hero_uid);
  int32_t IsInSupportTactic(int64_t hero_uid);

  //判断玩家是否在黑白单中
  int32_t GetRelationship(int32_t uid);

  //重新计算缘分
  //返回true表示缘分有改变的
  bool ReCalcRelation();
  //检查这个缘分能否存在
  //返回true表示可以存在
  bool CheckRelation(LogicHero* ship, int32_t relation_id);

  void CalcRank();
  void CalcChart();

  void SetTruceTime(int64_t time); //清空免战时间

  void FillOtherPlayerInfo(sy::OtherPlayerInfo* info);
  int32_t OnPKFinished(PK& pk, bool sweep);
  int32_t OnRobProcess(int64_t uid, int32_t item_id);
  int32_t OnDstrikeCopy(sy::DstrikeBoss& boss, const CopyBase* copy_base, std::vector<int64_t>& current_hp);
  int32_t IsRobSuccess(ArmyBase* army_base, bool is_rob_player);
  void RefreshPlayerPKTargets(int64_t player, int32_t new_rank,
                              VectorMap<int32_t, int64_t>& targets);
  void SendPKRankReward(int32_t old_rank, int32_t new_rank);
  void SendPKSuccessFailReward(bool success, int32_t copy_id, int32_t star, bool auto_flop);

  void ResetPatrolInfo(int32_t patrol_id);
  void SendReportToDB(int64_t report_id, sy::MessageResponseFightReport* resp);
  void SendArenaReportResult(int64_t player1, int64_t player2,
                             const std::string& report, int32_t report_type);
  void SendTowerState();
  void PrepareTowerRandomBuff(int32_t order);
  int32_t GetCopyChapterStarCount(int32_t chapter);
  void DeleteFriend(int64_t uid);

  void OnArmyBossCopy(sy::ArmyBossInfo& boss, const ArmyBossBase* base,
                      std::vector<int64_t>& current_hp, PK& pk, Army* army);

  int32_t RecoverShip(
      const google::protobuf::RepeatedField<int64_t>& uids, bool is_resolve,
      google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items);
  int32_t RecoverEquip(
      const google::protobuf::RepeatedField<int64_t>& uids, bool is_resolve,
      google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items);
  int32_t RecoverNavy(
      const google::protobuf::RepeatedField<int64_t>& uids,
      google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items);
  int32_t RecoverCarrier(
      const google::protobuf::RepeatedField<int64_t>& uids, bool is_resolve,
      google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items);

  //在副本挑战或者扫荡时，检查是否会触发围剿boss，返回true时则成功触发boss，false时继续之前的流程
  bool CheckDstrikeBossTrigger(int type);
  int32_t GetItemCountByItemID(int32_t item_id);
  void AddItemCount(int32_t item_id, int32_t count, int32_t msg_id,
                    int32_t sys_id);
  //获取航母副本的玩家对象
  LogicPlayer* GetCarrierPlayer(int32_t id);
  void InitCarrierPlayerSimpleInfo(int32_t level);
  void InitCarrierPlayerCarrier(LogicPlayer* from, int32_t carrier_id, double percent);
  void InitCarrierPlayerHero(LogicPlayer* from, int32_t hero_id, int32_t pos, double percent);
  void UpdateCarrierCopy(bool copys, bool info);

  void RefreshTimeActivity();

  void RefreshWeeklySystem(int32_t diff_day);

  int32_t CheckDailyActivityCondition(const DailyAwardBase* base);

  int32_t CarrierCopyPK(PK& pk);

  void SendCopyStatus(const CopyBase* base, int32_t star);

  //充值金额(单位是元宝)
  void OnRecharge(int32_t goodid, int32_t money);

  //删除一个军团成员
  void DeleteArmyMember(int64_t army_id, int64_t member_id);

  void RefreshEliteRandomCopy();
  void LoadEliteRandomCopy();
  void UpdateEliteRandomCopy();
  void FreshEliteHourCopy(int32_t hour,
                          boost::shared_ptr<VectorSet<int32_t> >& left_copy);
  void FillCopyExpAndMoney(int32_t power, int32_t energy, int32_t& add_exp,
                           int32_t& add_money);

  void DayRebate(sy::TimeActivityType type);

  void RefreshFestivalShop();

 public:
  int32_t ProcessIgnorePlayerMessage(CSMessageEntry& entry);
  int32_t ProcessServerErrorMessage(SSMessageEntry& entry);

  int32_t ProcessRequestHeartBeat(CSMessageEntry& entry);

  int32_t ProcessPlayerNotExist(SSMessageEntry& entry);

  //角色相关
  int32_t ProcessRequestCreatePlayer(CSMessageEntry& entry);
  int32_t ProcessResponseCreatePlayer(SSMessageEntry& entry);
  int32_t ProcessRequestChangeName(CSMessageEntry& entry);
  int32_t ProcessResponseChangeName(SSMessageEntry& entry);

  int32_t ProcessResponseGetPlayerInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetItemInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetHeroInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetCarrierInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetTacticInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetCopyInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetShopInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetRewardInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetPatrolInfo(SSMessageEntry& entry);
  int32_t ProcessResponseGetReportAbstract(SSMessageEntry& entry);

  //拉取邮件信息
  int32_t ProcessResponseGetMailInfo(SSMessageEntry& entry);
  //拉取玩家信息
  int32_t ProcessLoadPlayerBegin(CSMessageEntry& entry);
  int32_t ProcessLoadPlayerEnd(SSMessageEntry& entry);

  //设置阵型信息
  int32_t ProcessRequestSetTacticInfo(CSMessageEntry& entry);
  int32_t ProcessRequestEquipPlaneNew(CSMessageEntry& entry);

  //军团操作
  int32_t ProcessRequestCreateArmy(CSMessageEntry& entry);
  int32_t ProcessResponseCreateArmy(sy::ArmyInfo& army);
  //获取自己的军团信息
  int32_t ProcessRequestGetArmyInfo(CSMessageEntry& entry);
  //申请加入军团
  int32_t ProcessRequestArmyApply(CSMessageEntry& entry);
  //批准加入军团
  int32_t ProcessRequestAgreeArmyApply(CSMessageEntry& entry);
  //军团升级
  int32_t ProcessRequestArmyLevelUp(CSMessageEntry& entry);
  //T玩家出军团
  int32_t ProcessRequestKickArmyMember(CSMessageEntry& entry);
  //自己离开军团
  int32_t ProcessRequestLeaveArmy(CSMessageEntry& entry);
  //更改军团公告和头像
  int32_t ProcessRequestChangeArmyAnnouncement(CSMessageEntry& entry);
  //解散军团
  int32_t ProcessRequestDismissArmy(CSMessageEntry& entry);
  //军团签到领取
  int32_t ProcessRequestGetArmySignAward(CSMessageEntry& entry);
  //军团签到奖励
  int32_t ProcessRequsetArmyMaxLevelUp(CSMessageEntry& entry);

  //激活新的航母
  int32_t ProcessRequestActiveCarrier(CSMessageEntry& entry);

  //抽船
  int32_t ProcessRequestResearchHero(CSMessageEntry& entry);
  //领取抽船的奖励
  int32_t ProcessRequestGetResearchHero(CSMessageEntry& entry);

  //装备改造
  int32_t ProcessRequestEquipRefine(CSMessageEntry& entry);
  //装备升级
  int32_t ProcessRequestEquipLevelUp(CSMessageEntry& entry);
  //船升级
  int32_t ProcessRequestHeroLevelUp(CSMessageEntry& entry);
  //飞机升级
  int32_t ProcessRequestPlaneLevelUpNew(CSMessageEntry& entry);
  int32_t ProcessRequestPlaneChange(CSMessageEntry& entry);
  //船突破
  int32_t ProcessRequestHeroGradeLevelUp(CSMessageEntry& entry);
  //穿卸装备
  int32_t ProcessRequestEquipItem(CSMessageEntry& entry);
  //签到
  int32_t ProcessRequestSignIn(CSMessageEntry& entry);
  //改变航母
  int32_t ProcessRequestChangeCarrier(CSMessageEntry& entry);

  //翻牌
  int32_t ProcessRequestFlop(CSMessageEntry& entry);
  //领取副本奖励
  int32_t ProcessRequestGetCopyAward(CSMessageEntry& entry);
  //船洗属性
  int32_t ProcessRequestHeroRandomAttr(CSMessageEntry& entry);
  int32_t ProcessRequestSaveRandomAttr(CSMessageEntry& entry);
  //宝物合成
  int32_t ProcessRequestComposeNavy(CSMessageEntry& entry);
  //宝物升级
  int32_t ProcessRequestNavyLevelUp(CSMessageEntry& entry);

  //宝物改造 wmj
  int32_t ProcessRequestNavyRefine(CSMessageEntry& entry);
  //装备船只合成wmj
  int32_t ProcessRequestComposeEquip(CSMessageEntry& entry);
  //出售物品wmj
  int32_t ProcessRequestSellItem(CSMessageEntry& entry);
  //出售船只wmj
  int32_t ProcessRequestSellShip(CSMessageEntry& entry);
  //购买物品
  int32_t ProcessRequestBuyItem(CSMessageEntry& entry);
  //刷新兑换商店
  int32_t ProcessRequestRefreshFeats(CSMessageEntry& entry);
  //天命升级
  int32_t ProcessRequestFateLevelUp(CSMessageEntry& entry);
  //开始巡逻
  int32_t ProcessRequestStartPatrol(CSMessageEntry& entry);
  //获取巡逻奖励
  int32_t ProcessRequestGetPatrolAwards(CSMessageEntry& entry);
  //巡逻领地升级
  int32_t ProcessRequestPatrolLevelUp(CSMessageEntry& entry);
  //回收
  int32_t ProcessRequestRecover(CSMessageEntry& entry);
  //使用免战牌
  int32_t ProcessRequestUseTruce(CSMessageEntry& entry);
  //使用道具
  int32_t ProcessRequestUseItem(CSMessageEntry& entry);
  //提升军衔
  int32_t ProcessRequestUpRankInfo(CSMessageEntry& entry);
  //更新新手引导
  int32_t ProcessRequestDialog(CSMessageEntry& entry);
  //更新客户端标识
  int32_t ProcessRequestClientFlag(CSMessageEntry& entry);
  //清除CD
  //int32_t ProcessRequestClearCD(CSMessageEntry& entry);
  //阅读邮件
  int32_t ProcessRequestUpdateMailID(CSMessageEntry& entry);
  //发送私聊邮件
  int32_t  ProcessRequestSendMail(CSMessageEntry& entry);
  //获取竞技排行榜前几名
  int32_t ProcessRequestGetPKRankList(CSMessageEntry& entry);
  //获取自己的竞技场信息
  int32_t ProcessRequestGetMyPkRankInfo(CSMessageEntry& entry);
  int32_t ProcessRequestRefreshPKTargets(CSMessageEntry& entry);
  //设置正在挑战的对象
  int32_t ProcessRequestTestPkTarget(CSMessageEntry& entry);
  //获取他人信息
  int32_t ProcessRequestGetOtherPlayerInfo(CSMessageEntry& entry);
  int32_t ProcessResponseGetOtherPlayerInfo(SSMessageEntry& entry);
  //拉取排行榜
  int32_t ProcessRequestGetRankList(CSMessageEntry& entry);
  //购买次数
  int32_t ProcessRequestBuyCount(CSMessageEntry& entry);
  //聊天
  int32_t ProcessRequestTalk(CSMessageEntry& entry);
  //玩家登出
  int32_t ProcessRequestLogOut(CSMessageEntry& entry);
  //爬塔购买buff
  int32_t ProcessRequestTowerBuyBuff(CSMessageEntry& entry);
  //获取爬塔奖励
  int32_t ProcessRequestTowerAward(CSMessageEntry& entry);
  //爬塔随机商店
  int32_t ProcessRequestTowerBuyBox(CSMessageEntry& entry);
  //通过名字查找UID
  int32_t ProcessRequestGetUIDByName(CSMessageEntry& entry);
  int32_t ProcessResponseGetUIDByName(SSMessageEntry& entry);
  //副本扫荡
  int32_t ProcessRequestCopySweep(CSMessageEntry& entry);
  //获取夺宝对手
  int32_t ProcessRequestGetRobOpponent(CSMessageEntry& entry);
  //获取排行榜奖励
  int32_t ProcessRequestGetRankAward(CSMessageEntry& entry);
  //领取日常任务奖励
  int32_t ProcessRequestGetDailyAward(CSMessageEntry& entry);
  //领取成就
  int32_t ProcessRequestGetAchievement(CSMessageEntry& entry);
  //获取邮件奖励
  int32_t ProcessRequestGetMailReward(CSMessageEntry& entry);
  int32_t ProcessResponseGetMailReward(SSMessageEntry& entry);
  //获取七日奖励
  int32_t ProcessRequestGetSevenDays(CSMessageEntry& entry);
  int32_t ProcessRequestGetFourteenDays(CSMessageEntry& entry);

  //获取全服商店信息
  int32_t ProcessRequestGetServerShopInfo(CSMessageEntry& entry);
  //战队技能提升
  int32_t ProcessRequestArmySkillUp(CSMessageEntry& entry);
  //战队签到
  int32_t ProcessRequestArmySign(CSMessageEntry& entry);
  //募集军资
  int32_t ProcessRequestRaiseFunding(CSMessageEntry& entry);

  //拉取战报
  int32_t ProcessRequestGetReprot(CSMessageEntry& entry);

  //航母副本
  int32_t ProcessRequestCarrierCopyNextLevel(CSMessageEntry& entry);
  int32_t ProcessRequestGetCarrierCopyAward(CSMessageEntry& entry);
  int32_t ProcessRequestFightCarrierCopy(CSMessageEntry& entry);

  //好友
  int32_t ProcessRequestAddFriend(CSMessageEntry &entry);
  int32_t ProcessRequestAgreeAddFriend(CSMessageEntry &entry);
  int32_t ProcessRequestGetFriendInfo(CSMessageEntry& entry);
  int32_t ProcessResponseGetFriendInfo(SSMessageEntry &entry);
  int32_t ProcessAddFriend(int64_t uid);
  int32_t ProcessDeleteFriend(int64_t uid);
  int32_t ProcessBanAFriend(sy::FriendInfo& info);
  int32_t ProcessRequestFriendEnergy(CSMessageEntry& entry);
  int32_t ProcessRequestGetFriendEnergy(CSMessageEntry& entry);

  //激活缘分
  int32_t ProcessRequestActiveRelation(CSMessageEntry& entry);
  int32_t ProcessRequestActiveRelationAll(CSMessageEntry& entry);

  //围剿boss
  int32_t ProcessRequestDstrikeList(CSMessageEntry &entry);
  int32_t ProcessRequestDstrikePreFight(CSMessageEntry &entry);
  int32_t ProcessRequestDstrikeShare(CSMessageEntry &entry);
  int32_t ProcessRequestDstrikeRank(CSMessageEntry &entry);
  int32_t ProcessRequestDstrikeRankAwards(CSMessageEntry& entry);
  int32_t ProcessRequestDstrikeDailyAward(CSMessageEntry& entry);

  int32_t ProcessRequestGetDailyActivity(CSMessageEntry& entry);
  int32_t ProcessRequestGetDailyVIPAward(CSMessageEntry& entry);

  //领地攻讨灭火
  int32_t ProcessRequestPatrolHelp(CSMessageEntry& entry);

  //测试命令
  int32_t ProcessRequestTest(CSMessageEntry& entry);
  //切磋
  int32_t ProcessRequestTestPK(CSMessageEntry& entry);
  //战斗
  int32_t ProcessRequestFight(CSMessageEntry& entry);
  //精英副本随机副本
  int32_t ProcessRequestEliteRandomCopyFight(CSMessageEntry& entry);
  //推荐好友
  int32_t ProcessRequestGetRandomFriend(CSMessageEntry& entry);
  //装备航母
  int32_t ProcessRequestEquipCarrier(CSMessageEntry& entry);
  //航母升级
  int32_t ProcessRequestCarrierLevelUp(CSMessageEntry& entry);
  //航母突破
  int32_t ProcessRequestCarrierReformUp(CSMessageEntry& entry);

  //获取军团战役详细情况
  int32_t ProcessRequestGetArmyWarInfo(CSMessageEntry& entry);
  //军团战役开始
  int32_t ProcessRequestArmyWarFight(CSMessageEntry& entry);
  //获取军团战役章节奖励
  int32_t ProcessRequestGetArmyWarChapterAward(CSMessageEntry& entry);
  //获取军团战役个人奖励
  int32_t ProcessRequestGetArmyWarBossAward(CSMessageEntry& entry);
  //军团长设置下一个章节
  int32_t ProcessRequestSetArmyWarChapter(CSMessageEntry& entry);

  //开服活动
  int32_t ProcessRequestServerOpenFund(CSMessageEntry& entry);
  int32_t ProcessRequestServerOpenWelfare(CSMessageEntry& entry);
  //首冲礼包
  int32_t ProcessRequestFirstRecharge(CSMessageEntry& entry);
  // VIP每周礼包
  int32_t ProcessRequestGetVIPWeekly(CSMessageEntry& entry);
  //获取联盟列表
  int32_t ProcessRequestArmyList(CSMessageEntry& entry);
  //搜索联盟
  int32_t ProcessRequestSearchArmy(CSMessageEntry& entry);
  //更改军团职位
  int32_t ProcessRequestChangeArmyPos(CSMessageEntry& entry);
  //获取军团商店信息
  int32_t ProcessReuqestGetArmyShopInfo(CSMessageEntry& entry);

  //刷新钻石商店
  int32_t ProcessRequestRefreshDiamondShop(CSMessageEntry& entry);

  //世界boss
  int32_t ProcessRequestGetWorldBossInfo(CSMessageEntry& entry);
  int32_t ProcessRequestFightWorldBoss(CSMessageEntry& entry);
  int32_t ProcessRequestWorldBossCountry(CSMessageEntry& entry);
  int32_t ProcessRequestGetWorldBossMeritAward(CSMessageEntry& entry);
  int32_t ProcessRequestGetWorldBossKillAward(CSMessageEntry& entry);
  int32_t ProcessRequestGetWorldBossMeritRank(CSMessageEntry& entry);
  int32_t ProcessRequestGetWorldBossDamageRank(CSMessageEntry& entry);

  //觉醒道具
  int32_t ProcessRequestMakeWakeItem(CSMessageEntry& entry);
  int32_t ProcessRequestRecoverWakeItem(CSMessageEntry& entry);
  int32_t ProcessRequestEquipWakeItem(CSMessageEntry& entry);
  int32_t ProcessRequestShipWake(CSMessageEntry& entry);

  int32_t ProcessRequestSetUserDefined(CSMessageEntry& entry);

  int32_t ProcessRequestGetTimeActivityAwardNew(CSMessageEntry& entry);

  //占星
  int32_t ProcessRequestAstrology(CSMessageEntry& entry);
  int32_t ProcessRequestAstrologyChange(CSMessageEntry& entry);
  int32_t ProcessRequestAstrologyInfo(CSMessageEntry& entry);

  //道具抽奖
  int32_t ProcessRequestResearchItem(CSMessageEntry& entry);
  int32_t ProcessRequestGetResearchItemAward(CSMessageEntry& entry);

  //获取累计登录奖励
  int32_t ProcessRequestGetLoginAward(CSMessageEntry& entry);
  //获取周基金/小月基金/大月基金奖励
  int32_t ProcessRequestGetWeeklyCardAward(CSMessageEntry& entry);
  int32_t ProcessRequestWeeklyCardSign(CSMessageEntry& entry);

  //GVG
  int32_t ProcessRequestGetLegionWarInfo(CSMessageEntry& entry);
  int32_t ProcessRequestLegionWarFight(CSMessageEntry& entry);
  int32_t ProcessRequestGetLegionWarPlayer(CSMessageEntry& entry);
  int32_t ProcessRequestLegionWarRegister(CSMessageEntry& entry);
  int32_t ProcessRequestGetLegionWarReward(CSMessageEntry& entry);
  int32_t ProcessRequestGetLegionWarTarget(CSMessageEntry& entry);
  int32_t ProcessRequestGetLegionWarTargetAward(CSMessageEntry& entry);
  int32_t ProcessRequestLegionForeplayFight(CSMessageEntry& entry);
  int32_t ProcessRequestLegionForeplayGetServerAward(CSMessageEntry& entry);
  int32_t ProcessRequestLegionForeplayGetDamageAward(CSMessageEntry& entry);
  int32_t ProcessRequestLegionForeplayGetInfo(CSMessageEntry& entry);
  int32_t ProcessRequestSetFocusCity(CSMessageEntry& entry);
  int32_t ProcessRequestGetLegionWarLog(CSMessageEntry& entry);

  //转盘
  int32_t ProcessRequestSweepStakeEquip(CSMessageEntry& entry);
  int32_t ProcessRequestSweepStakeCarrier(CSMessageEntry& entry);
  int32_t ProcessRequestGetSweepStakeEquipAward(CSMessageEntry& entry);
  int32_t ProcessRequestGetSweepStakeCountAward(CSMessageEntry& entry);

  //跨服积分战
  int32_t CrossServerQueryPlayers(int32_t msg_id);
  int32_t ProcessRequestCrossServerCountry(CSMessageEntry& entry);
  int32_t ProcessResponseCrossServerCountry(
      intranet::MessageSSResponseQueryOtherPlayer& msg);
  int32_t ProcessRequestCrossServerRandomPlayer(CSMessageEntry& entry);
  int32_t ProcessResponseCrossServerRandomPlayer(
      intranet::MessageSSResponseQueryOtherPlayer& msg);
  int32_t ProcessRequestCrossServerFight(CSMessageEntry& entry);
  int32_t ProcessRequestCrossServerGetAward(CSMessageEntry& entry);

  int32_t ProcessRequestEnterStage(CSMessageEntry& entry);
  int32_t ProcessRequestLeaveStage(CSMessageEntry& entry);

  int32_t ProcessNotifyMoneyInfo(CSMessageEntry& entry);

  int32_t ProcessRequestFestivalReplenishSign(CSMessageEntry& entry);
  int32_t ProcessRequestGetVersionAward(CSMessageEntry& entry);

  int32_t ProcessRequestMedalResearch(CSMessageEntry& entry);
  int32_t ProcessRequestMedalFight(CSMessageEntry& entry);
  int32_t ProcessRequestMedalFightRefresh(CSMessageEntry& entry);
  int32_t ProcessRequestMedalActive(CSMessageEntry& entry);
  int32_t ProcessRequestMedalActiveAchi(CSMessageEntry& entry);

  int32_t ProcessRequestRedEquipStarLevelUp(CSMessageEntry& entry);

  int32_t ProcessRequestGetComeBackLoginAward(CSMessageEntry& entry);
  int32_t ProcessRequestGetComeBackRechargeAward(CSMessageEntry& entry);

  int32_t ProcessRequestSellItemEx(CSMessageEntry& entry);

  int32_t ProcessRequestPearlHarborFight(CSMessageEntry& entry);
  int32_t ProcessRequestPearlHarborGetInfo(CSMessageEntry& entry);
  int32_t ProcessRequestPearlHarborWarZone(CSMessageEntry& entry);
  int32_t ProcessRequestPearlHarborStartBuff(CSMessageEntry& entry);
  int32_t ProcessRequestPearlHarborArmyScore(CSMessageEntry& entry);

  //被动更新
  //可以用来更新体力之类的
  void Update();

 private:
  void SendAllDataToClient();
  void SendPlayerData();
  void SendHeroData();
  void SendCopyData();
  void SendTacticData();
  void SendItemData();
  void SendShopData();
  void SendSweepStakeData();
  void SendFirstServerReward();

  void FetchServerMail();
  void LoadLocalStorage();
 public:
  //发送消息到客户端
  void SendMessageToClient(uint16_t msgid, Message* pMsg);
  //发送消息给DB的Player
  void SendMessageToDB(uint16_t msgid, Message* pMsg);
  //发送错误消息给客户端
  void SendErrorCodeToClient(int32_t error_code, uint16_t msgid);
  //发送拉取角色信息
  void SendLoadPlayer(int64_t sys_id);
  void SendMessageToFriends(uint16_t msgid, Message* pMsg);

  //发送消息给自己(模拟客户端发送消息)
  void SendMessageToSelf(uint16_t msgid, const boost::shared_ptr<Message>& msg);

  //发送邮件
  static void SendMail(int64_t player_id, int64_t mail_time, int32_t mail_type,
                const std::string& mail_content,
                const std::vector<std::pair<int32_t, int32_t> >* reward);

  int32_t FightWorldBoss();
  void UpdateWorldBossInfo();

  const sy::PlayerWorldBossInfo world_boss_info() { return world_boss_info_; }

  int32_t GetActivityRecordNew(sy::TimeActivityType type, int64_t id,
                               int32_t key);
  sy::ActivityRecord* GetActivityRecordX(sy::TimeActivityType type, int64_t id);
  void SetActivityRecordNew(int32_t type, int64_t id, int32_t key,
                            int32_t value);
  void AddActivityRecordNew(int32_t type, int64_t id, int32_t key,
                            int32_t value);
  void UpdateActivityRecordNew(int32_t type, int64_t id);

  void DeleteApply(int32_t player_id);

  //0周卡, 1小月基金, 2大月基金
  int32_t* GetWeeklyCardByType(int32_t type, int32_t& valid_days,
                               int32_t& buy_count);
  void channel(const std::string& c) { this->channel_ = c; }
  void device_id(const std::string& d) { this->device_id_ = d; }
  void idfa(const std::string& d) { this->idfa_ = d; }

  int32_t RefreshMedalCopyID();
  void AddMedalAttr(AttackAttrArray& attr);

  void HeroComeBack();
  void HeroComeBackLogin();
  bool IsHeroComeBack() {
    return come_back_info_.end_time() > GetVirtualSeconds();
  }
  int32_t medal_star() { return this->medal_star_; }
  int32_t medal_state_count();

 private:
  int32_t status_;
  int32_t fresh_time_;
  int64_t last_update_time_;
  int32_t online_time_;
  int64_t last_heart_beat_time_;  //RecordPlayer保活

  int64_t item_seed_;
  int64_t ship_seed_;

  boost::weak_ptr<TcpSession> session_;
  LogicItemManager items_;

  sy::PlayerInfo player_;
  sy::CurrentCarrierInfo current_carrier_;

  sy::HeroResearchInfo hero_research_info_;

  //没有删除要求的容器可以用数组
  //有删除要求的容器得用迭代器稳定的容器
  std::vector<LogicHero> ships_;
  std::vector<sy::CarrierInfo> carriers_;
  sy::TacticInfo tactic_;

  VectorMap<int32_t, int32_t> copy_star_;       //副本星级
  VectorMap<int32_t, int32_t> copy_count_;      //副本通关次数
  std::vector<int32_t> passed_copy_;            //一次性通关副本
  std::vector<sy::CopyProgress> copy_progress_; //副本进度
  VectorMap<int32_t, int32_t> chapter_award_;   //章节奖励
  std::vector<int32_t> gate_award_;             //关卡奖励

  //副本翻牌的Session
  LootBasePtr flop_1_;
  LootBasePtr flop_2_;
  int32_t flop_2_count_;

  //装备
  sy::EquipsInfo equips_;

  //商店
  VectorMap<int32_t, sy::RefreshShopInfo>  refresh_shop_info_; //刷新商店商品信息
  VectorMap<int32_t,int32_t> normal_shop_info_;         //普通物商品信息
  VectorMap<int32_t,int32_t> life_shop_info_;           //终生物品商品信息

  //竞技场
  typedef VectorMap<int32_t, int64_t> PkTargetType;
  PkTargetType pk_targets_;  //可以挑战的人
  int32_t pk_rank_reward_time_;   //上次领取竞技场奖励的时间
  int32_t pk_rank_reward_rank_;   //上次竞技场排名
  int32_t pk_max_rank_;           //历史最高排名
  int32_t pk_current_rank_;       //当前排名
  int32_t last_pk_time_;          //上次竞技的时间戳
  int64_t pk_player_id_;          //正在挑战的对象
  uint64_t last_server_mail_id_;  //上次拉取服务器邮件ID

  int32_t patrol_total_time_;     //挂机总时间
  VectorMap<int32_t, sy::PatrolInfo> patrol_infos_;  //巡逻点信息

  std::vector<sy::RechargeInfo> recharge_;  //最近一个月的充值信息

  //好友
  typedef sy::MessageResponseLoadFriend FriendContaier;
  FriendContaier friends_;
  VectorMap<int32_t, int32_t>     daily_counter_; //每日计数器
  VectorMap<int32_t, std::deque<std::string> > report_abstract_;  //战报摘要
  //爬塔
  sy::TowerState tower_state_;
  VectorMap<int16_t, int16_t> tower_buff_;
  int32_t tower_shop_;            //爬塔的商店
  sy::DstrikeInfo dstrike_info_;  //玩家的围剿信息

  int64_t max_fight_attr_;  //历史最高战斗力
  int64_t fight_attr_;    //战斗力

  time_t  chat_time_;     //聊天时间戳

  VectorMap<int32_t,int32_t>  achievements_; //成就

  VectorSet<int32_t> mail_reward_temp_id_;   //邮件领取奖励临时ID集

  Array<int64_t, sy::AttackAttr_ARRAYSIZE> rank_attr_; //军衔附加属性
  Array<int64_t, sy::AttackAttr_ARRAYSIZE> chart_attr_;    //航母图鉴附加属性

  sy::CarrierCopyInfo carrier_copy_info_;  //航母副本状况
  Array<sy::CarrierCopy, 12> carrier_copy_;//航母副本
  Array<boost::shared_ptr<LogicPlayer>, 12> carrier_player_; //伪装的12个玩家

  VectorSet<int32_t> obtained_carriers_;

  ArmySkillArray army_skill_;
  int64_t army_id_;
  int64_t army_leave_time_;
  std::vector<int32_t> daily_sign_;

  int64_t month_card_;
  int64_t big_month_card_;
  int64_t life_card_;

  VectorMap<int32_t, int32_t>  vip_weekly_;

  sy::PlayerWorldBossInfo world_boss_info_;

  VectorMap<std::pair<int32_t, int64_t>, sy::ActivityRecord> activity_record_new_;

  //精英副本里面的随机副本
  sy::MessageNotifyEliteRandomCopy elite_random_copy_;
  std::string user_defined_;
  sy::CrossServerInfo cross_server_info_;

  //time, login, status
  int32_t month_card_1_[3];  //小月基金
  int32_t month_card_2_[3];  //大月基金
  int32_t weekly_card_[3];   //周基金

  VectorMap<int64_t, boost::weak_ptr<TcpSession> > foreplay_players_;
  VectorMap<int64_t, boost::weak_ptr<TcpSession> > world_boss_players_;
  std::string channel_;  //渠道ID
  std::string device_id_;//设备ID
  std::string idfa_;
  //转盘活动
  sy::MessageResponseActivitySweepStake sweep_stake_;

  time_t festival_shop_refresh_time_;

  std::string got_award_version_;

  int32_t medal_copy_id_;
  std::string medal_state_;
  int32_t medal_star_;
  int32_t medal_achi_;

  sy::PlayerComeBackInfo come_back_info_;
};

