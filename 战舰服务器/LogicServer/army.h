#ifndef __ARMY_H__
#define __ARMY_H__
#include <noncopyable.h>
#include <vector>
#include <cpp/message.pb.h>
#include "logic_player.h"
#include "rank_list.h"

class LogicPlayer;

typedef RankList<CompareRankDamage, 60, sy::RANK_TYPE_ARMY_WAR> ArmyWarRankList;

class Army : NonCopyable {
 public:
  Army(const sy::ArmyInfo& info);


  //成员修改名字
  //可以是军团长(军团长需要修改军团表里面的名字)
  void ChangeName(int64_t uid, const std::string& new_name);

  //增加军团经验
  void AddArmyExp(int32_t exp, int64_t uid, int32_t msgid);

  //军团升级, 返回错误码
  //并且记录军团log
  int32_t ArmyLevelUp(LogicPlayer* player);

  //index: ArmySkill枚举
  //返回错误码
  int32_t ArmySkillLevelUp(int32_t index, LogicPlayer* player);

  int32_t SendMessageToArmy(uint16_t msgid, Message* pMsg);

  void SendMailToArmyMember(int32_t mail_type, const std::string& content,
                            const std::vector<std::pair<int32_t, int32_t> >* reward);

  void ResetArmyWarFreshTime();
 public:
  int64_t army_id() const { return this->info_.army_id(); }
  int32_t level() const { return this->info_.level(); }
  int64_t exp() const { return this->info_.exp(); }
  int32_t skill_level(int32_t index) const;
  const std::string& name() const { return this->info_.army_name(); }
  int32_t GetArmyMaxNum() const;
  int32_t current_chapter() const { return this->current_chapter_; }
  int32_t next_chapter() const { return this->next_chapter_; }
  void next_chapter(int32_t value);
  int32_t max_chapter() const { return this->max_chapter_; }
  ArmyWarRankList& rank_list() { return this->rank_list_; }
  int32_t army_merit() { return this->info_.army_merit(); }
  int32_t AddArmyMerit(int32_t num);
  void ClearArmyMerit();

  int32_t sign_count() { return this->info_.donate_count(); }
  int32_t sign_value() { return this->info_.donate_value(); }

  //当前的副本
  //chapter是0表示当前的副本
  sy::ArmyWarInfo* army_war(int32_t chapter = 0);

  sy::ArmyMemberInfo* GetMember(int64_t player_id);
  sy::ArmyApplyInfo* GetApply(int64_t player_id);
  void AddMemeber(sy::ArmyMemberInfo& member, LogicPlayer* player);
  void UpdateMember(int64_t player_id);
  void DeleteApply(int64_t player_id);
  void DeleteMemeber(int64_t player_id, LogicPlayer* player);

  void AddArmyLog(const std::string& log);

  void SetMaster(int64_t uid, const std::string& name);

  void ChangePosLog(const std::string& name, const std::string& other_name,
                    sy::ArmyPosition pos);

  sy::ArmyInfo& info();
  std::vector<sy::ArmyMemberInfo>& members() { return this->member_; }
  std::vector<sy::ArmyApplyInfo>& applies() { return this->apply_; }
  const sy::ArmyMemberInfo* GetArmyMemberInfo(int64_t uid);

  int32_t ArmySign(LeagueSignBase* sign_base, LogicPlayer* player,
                   bool is_crtical);

  void UpdateArmyExpInfo(int64_t uid, int32_t msgid, int32_t exp_delta);

  ///军团战役的操作///
  //获取今天通关和未通关的军团战役
  void GetArmyWars(sy::ArmyWarInfo*& current,
                   std::vector<sy::ArmyWarInfo>*& today_passed);
  //0,boss没有死
  //1,当前boss死了
  //2,所有boss死了
  int32_t OnArmyWarCopy(const ArmyBossBase* boss, const std::string& name);
  void SaveArmyWar(const sy::ArmyWarInfo& info);

  //更新军团战役伤害排行榜
  void UpdateArmyWarRankList(LogicPlayer* player, int64_t damage);

  void RefreshArmyShop();
  int32_t GetArmyShopNum(int32_t shop_id);
  void AddArmyBuyRecord(int32_t solt, int64_t player_id);
  bool CanCommodityBuy(int32_t solt, int64_t player_id);

  sy::PearlHarborInfo& PearlHarborInfo() { return this->pearl_harbor_info_; }
  void SavePearlHarbor();
  void LoadPearlHarbor();
  void RefreshPearlHarbor();
  void RefreshPearlHarborMonster(int32_t batch);

 private:
  void SaveArmyWars();
  void LoadArmyWar();
  //初始化BOSS血量信息
  void InitArmyWarBoss();
  void InitArmyWarBossPersonalAward(sy::ArmyBossInfo& info,
                                    const ArmyBossBase* boss);
  void UpdateArmyInfo();

  int32_t GetArmyStorageInt32(const std::string& type);
  void SetArmyStorageInt32(const std::string& type, int32_t value);
 private:
  mutable sy::ArmyInfo info_;
  std::vector<sy::ArmyMemberInfo> member_;
  std::vector<sy::ArmyApplyInfo> apply_;
  //军团战役数据
  int32_t last_war_time_;                   //军团战役上次更新时间
  int32_t current_chapter_;                 //当天正在打的章节
  int32_t next_chapter_;                    //下一次重置到的章节(军团长可以手选)
  int32_t max_chapter_;                     //军团战役最大进度
  sy::ArmyWarInfo army_war_;                //当前正在打的战役
  std::vector<sy::ArmyWarInfo> today_wars_; //今天通关的战役
  ArmyWarRankList rank_list_;               //军团战役伤害排行榜

  sy::PearlHarborInfo pearl_harbor_info_;   //捍卫珍珠港信息
};

#endif
