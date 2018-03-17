#include "army.h"
#include "config.h"
#include "server.h"
#include "rank_list.h"
#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <array_stream.h>
#include <str_util.h>

using namespace sy;
using namespace intranet;
using namespace KVStorage;

template <typename T>
static inline std::string MakeValue(const T* p, size_t length) {
  if (!length) return std::string();
  char* begin = (char*)(void*)const_cast<T*>(p);
  std::string v(begin, begin + length);
  return v;
}

struct TypeVoid {};

template <typename T>
void ArrayStreamAppend(DefaultArrayStream& stream, const T& t) {
  stream.Append(",");
  stream.Append(t);
}

template <>
void ArrayStreamAppend<TypeVoid>(DefaultArrayStream& stream,
                                 const TypeVoid& v) {}

//生成log的帮助函数
template <typename T1, typename T2, typename T3, typename T4, typename T5>
std::string MakeArmyLog(const T1& t1, const T2& t2, const T3& t3, const T4& t4,
                        const T5& t5) {
  DefaultArrayStream stream;
  ArrayStreamAppend<T1>(stream, t1);
  ArrayStreamAppend<int32_t>(stream, GetSeconds());
  ArrayStreamAppend<T2>(stream, t2);
  ArrayStreamAppend<T3>(stream, t3);
  ArrayStreamAppend<T4>(stream, t4);
  ArrayStreamAppend<T5>(stream, t5);

  const std::string& v = stream.str();
  return v.substr(1, v.length() - 1);
}

Army::Army(const sy::ArmyInfo& info) : info_(info) {
  last_war_time_ = 0;
  current_chapter_ = 0;
  next_chapter_ = 0;
  max_chapter_ = 0;

  if (info.army_shop_size() <= 0) RefreshArmyShop();

  this->LoadArmyWar();
  //世界boss荣耀
  info_.set_army_merit(this->GetArmyStorageInt32(kKVTypeArmyWorldBossMerit));
}

void Army::ChangeName(int64_t uid, const std::string& new_name) {}

void Army::AddArmyExp(int32_t exp,int64_t uid, int32_t msgid) {
  this->info_.set_exp(this->info_.exp() + exp);
  this->UpdateArmyExpInfo(uid, msgid, exp);
}

int32_t Army::ArmySkillLevelUp(int32_t index, LogicPlayer* player) {
  if (!player) return ERR_INTERNAL;
  sy::ArmyInfo& info = this->info();

  LeagueSkillBase* skill_base = LEAGUE_SKILL_BASE.GetEntryByID(index).get();
  if (!skill_base) return ERR_PARAM_INVALID;
  if (skill_base->need_league > this->level()) return ERR_PARAM_INVALID;

  int32_t current_level = this->skill_level(index);
  if (current_level >= skill_base->skill_max) return ERR_PARAM_INVALID;

  LeagueLevelBase* level_base =
      LEAGUE_LEVEL_BASE.GetEntryByID(skill_base->cost_type * 1000 +
                                     current_level)
          .get();
  if (!level_base) return ERR_PARAM_INVALID;
  if (this->level() < level_base->need_lvl) return ERR_PARAM_INVALID;
  if (!level_base->exp || level_base->exp > info.exp())
    return ERR_PARAM_INVALID;

  info.set_exp(info.exp() - level_base->exp);
  info.set_skills(index, current_level + level_base->step);

  UpdateArmyExpInfo(player->uid(), MSG_CS_REQUEST_ARMY_SKILL_UP,
                    -level_base->exp);
  return ERR_OK;
}

int32_t Army::skill_level(int32_t index) const {
  if (this->info_.skills_size() < sy::ArmySkill_ARRAYSIZE) {
    this->info_.mutable_skills()->Resize(sy::ArmySkill_ARRAYSIZE, 0);
  }
  if (index >= 0 && index < sy::ArmySkill_ARRAYSIZE) {
    return this->info_.skills(index);
  }
  return 0;
}

static inline void LogArmyWar(const sy::ArmyWarInfo& info) {
  INFO_LOG(logger)("CopyID:%d, HP:%ld,%ld,%ld,%ld,%ld,%ld"
      , info.boss1().copy_id()
      , info.boss1().left_hp(0), info.boss1().left_hp(1)
      , info.boss1().left_hp(2), info.boss1().left_hp(3)
      , info.boss1().left_hp(4), info.boss1().left_hp(5)
      );
  INFO_LOG(logger)("CopyID:%d, HP:%ld,%ld,%ld,%ld,%ld,%ld"
      , info.boss2().copy_id()
      , info.boss2().left_hp(0), info.boss2().left_hp(1)
      , info.boss2().left_hp(2), info.boss2().left_hp(3)
      , info.boss2().left_hp(4), info.boss2().left_hp(5)
      );
  INFO_LOG(logger)("CopyID:%d, HP:%ld,%ld,%ld,%ld,%ld,%ld"
      , info.boss3().copy_id()
      , info.boss3().left_hp(0), info.boss3().left_hp(1)
      , info.boss3().left_hp(2), info.boss3().left_hp(3)
      , info.boss3().left_hp(4), info.boss3().left_hp(5)
      );
  INFO_LOG(logger)("CopyID:%d, HP:%ld,%ld,%ld,%ld,%ld,%ld"
      , info.boss4().copy_id()
      , info.boss4().left_hp(0), info.boss4().left_hp(1)
      , info.boss4().left_hp(2), info.boss4().left_hp(3)
      , info.boss4().left_hp(4), info.boss4().left_hp(5)
      );
}

void Army::AddMemeber(sy::ArmyMemberInfo& member, LogicPlayer* player) {
  sy::ArmyMemberInfo* info = this->GetMember(member.player_id());
  if (info) info->CopyFrom(member);
  else this->member_.push_back(member);

  MessageSSUpdateArmyMember update;
  update.mutable_member()->CopyFrom(member);
  update.set_server(server_config->server_id());
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_MEMBER, &update);

  const std::string& log = MakeArmyLog(ARMY_LOG_NEW_MEMBER, player->name(),
                                       member.name(), TypeVoid(), TypeVoid());
  this->AddArmyLog(log);
}

void Army::UpdateMember(int64_t player_id) {
  const sy::ArmyMemberInfo* info = this->GetArmyMemberInfo(player_id);
  if (!info) return;
  MessageSSUpdateArmyMember update;
  update.mutable_member()->CopyFrom(*info);
  update.set_server(server_config->server_id());
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_MEMBER, &update);
}

sy::ArmyMemberInfo* Army::GetMember(int64_t player_id) {
  for (std::vector<sy::ArmyMemberInfo>::iterator iter = this->member_.begin();
       iter != this->member_.end(); ++iter) {
    if (iter->player_id() == player_id) {
      return &*iter;
    }
  }
  return NULL;
}

sy::ArmyApplyInfo* Army::GetApply(int64_t player_id) {
  for (std::vector<sy::ArmyApplyInfo>::iterator iter = this->apply_.begin();
       iter != this->apply_.end(); ++iter) {
    if (iter->player_id() == player_id) {
      return &*iter;
    }
  }
  return NULL;
}

void Army::DeleteApply(int64_t player_id) {
  for (std::vector<sy::ArmyApplyInfo>::iterator iter = this->apply_.begin();
       iter != this->apply_.end(); ++iter) {
    if (iter->player_id() == player_id) {
      this->apply_.erase(iter);
      break;
    }
  }

  MessageSSUpdateArmyApply update;
  update.set_army_id(this->army_id());
  update.set_server(server_config->server_id());
  update.set_player_id(player_id);
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_APPLY, &update);
}

void Army::DeleteMemeber(int64_t player_id, LogicPlayer* player) {
  std::string name;
  for (std::vector<sy::ArmyMemberInfo>::iterator iter = this->member_.begin();
       iter != this->member_.end(); ++iter) {
    if (iter->player_id() == player_id) {
      name = iter->name();
      this->member_.erase(iter);
      break;
    }
  }
  MessageSSUpdateArmyMember update;
  update.set_member_id(player_id);
  update.set_army_id(this->army_id());
  update.set_server(server_config->server_id());
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_MEMBER, &update);

  //军团Log
  if (name.length() && player_id != player->uid()) {
    //被人开除
    if (player_id != player->uid()) {
      const std::string& log = MakeArmyLog(ARMY_LOG_FIRE_MEMBER, player->name(),
                                           name, TypeVoid(), TypeVoid());
      this->AddArmyLog(log);
    } else {
      const std::string& log = MakeArmyLog(ARMY_LOG_MEMBER_EXIT, name,
                                           TypeVoid(), TypeVoid(), TypeVoid());
      this->AddArmyLog(log);
    }
  }
}

void Army::AddArmyLog(const std::string& log) {
  *this->info_.add_log() = log;
  if (this->info_.log_size() > 20) {
    this->info_.mutable_log()->DeleteSubrange(0, 1);
  }
  MessageSSUpdateArmyLog update;
  for (int32_t i = 0; i < this->info_.log_size(); ++i) {
    *update.add_logs() = this->info_.log(i);
  }
  update.set_army_id(this->army_id());
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_LOG, &update);
}

sy::ArmyInfo& Army::info() {
  if (this->info_.army_id()) {
    if (GetSecondsDiffDays(
            this->info_.donate_time() - Setting::kRefreshSeconds,
            GetVirtualSeconds() - Setting::kRefreshSeconds)) {
      this->info_.set_donate_time(GetVirtualSeconds());
      this->info_.set_donate_count(0);
      this->info_.set_donate_value(0);
    }
    if (this->info_.skills_size() < sy::ArmySkill_ARRAYSIZE) {
      this->info_.mutable_skills()->Resize(sy::ArmySkill_ARRAYSIZE, 0);
    }
    this->info_.set_army_war(this->max_chapter());
  }
  return this->info_;
}

int32_t Army::ArmySign(LeagueSignBase* sign_base, LogicPlayer* player,
                       bool is_crtical) {
  if (!player) return sy::ERR_INTERNAL;
  sy::ArmyInfo& army_info = this->info();
  const LeagueBase* base = LEAGUE_BASE.GetEntryByID(army_info.level()).get();
  if (!base) return sy::ERR_INTERNAL;

  int32_t count = army_info.donate_count();
  army_info.set_donate_count(count + 1);
  army_info.set_donate_time(GetVirtualSeconds());
  army_info.set_donate_value(army_info.donate_value() +
                             sign_base->sign_progress);
  int32_t add_exp = 0;
  if (count <= base->limit) {
    add_exp = sign_base->league_exp;
    if (is_crtical) add_exp *= 1.5f;
    army_info.set_exp(army_info.exp() + add_exp);
  }
  this->UpdateArmyExpInfo(player->uid(), MSG_CS_REQUEST_ARMY_SIGN, add_exp);
  this->AddArmyLog(MakeArmyLog(sy::ARMY_LOG_SIGN, player->name(),
                               sign_base->id(), sign_base->league_exp,
                               TypeVoid()));

  return sy::ERR_OK;
}

int32_t Army::SendMessageToArmy(uint16_t msgid, Message* pMsg) {
  int32_t count = 0;
  for (std::vector<sy::ArmyMemberInfo>::iterator iter = this->member_.begin();
       iter != this->member_.end(); ++iter) {
    LogicPlayer* player = server->GetPlayerByID(iter->player_id());
    if (player && player->is_online()) {
      player->SendMessageToClient(msgid, pMsg);
      ++count;
    }
  }
  return count;
}

void Army::UpdateArmyExpInfo(int64_t uid, int32_t msgid, int32_t exp_delta) {
  intranet::MessageSSUpdateArmyExpInfo msg;
  const sy::ArmyInfo& army_info = this->info();
  msg.set_army_id(army_info.army_id());
  msg.set_army_exp(army_info.exp());
  msg.set_army_level(army_info.level());
  msg.set_donate_count(army_info.donate_count());
  msg.set_donate_value(army_info.donate_value());
  msg.set_donate_time(army_info.donate_time());
  for (int32_t i = 0; i < army_info.skills_size(); i++) {
    msg.add_army_skill(army_info.skills(i));
  }
  msg.set_tid(server->GetTID());
  msg.set_exp_delta(exp_delta);
  msg.set_msgid(msgid);
  msg.set_player_id(uid);
  msg.set_server_id(server_config->server_id());
  server->SendServerMessageToDB(intranet::MSG_SS_UPDATE_ARMY_EXP_INFO, &msg);
}

const sy::ArmyMemberInfo* Army::GetArmyMemberInfo(int64_t uid) {
  for (size_t i = 0; i < this->member_.size(); i++) {
    if (this->member_[i].player_id() == uid) return &this->member_[i];
  }

  return NULL;
}

int32_t Army::ArmyLevelUp(LogicPlayer* player) {
  if (!player) return sy::ERR_INTERNAL;
  sy::ArmyInfo& info = this->info();

  LeagueBase* base = LEAGUE_BASE.GetEntryByID(info.level()).get();
  if (!base) return sy::ERR_INTERNAL;
  if (base->exp <= 0) return sy::ERR_PARAM_INVALID;

  if (info.exp() < base->exp) return sy::ERR_PARAM_INVALID;

  info.set_exp(info.exp() - base->exp);
  info.set_level(info.level() + 1);

  UpdateArmyExpInfo(player->uid(), MSG_CS_REQUEST_ARMY_LEVEL_UP, -base->exp);
  //军团升级日志
  const std::string& log =
      MakeArmyLog(ARMY_LOG_LEVEL_UP, player->name(), this->info_.level(),
                  TypeVoid(), TypeVoid());
  this->AddArmyLog(log);

  return sy::ERR_OK;
}

int32_t Army::GetArmyMaxNum() const {
  const LeagueBase* base = LEAGUE_BASE.GetEntryByID(info_.level()).get();
  return base ? base->limit : 0;
}

void Army::ResetArmyWarFreshTime() {
  this->last_war_time_ = 1;
}

void Army::GetArmyWars(sy::ArmyWarInfo*& current,
                       std::vector<sy::ArmyWarInfo>*& today_passed) {
  if (!this->last_war_time_) {
    this->last_war_time_ = this->GetArmyStorageInt32(kKVTypeArmyWarFreshTime);
  }

  //如果过了0点,就重置
  //如果boss没有初始化过,就初始化
  int32_t cross_day = GetSecondsDiffDays(this->last_war_time_, GetSeconds());
  if (cross_day || !this->army_war_.boss1().copy_id()) {
    if (cross_day) {
      this->rank_list_.Clear();
      INFO_LOG(logger)("ArmyID:%ld, CrossDay:%d", this->army_id(), cross_day);
    }
    INFO_LOG(logger)("ArmyID:%ld, BossID:%d", this->army_id(), this->army_war_.boss1().copy_id());
    this->army_war_.Clear();
    this->today_wars_.clear();
    this->last_war_time_ = GetSeconds();

    if (current_chapter_ <= 0) current_chapter_ = 1;
    if (next_chapter_ <= 0) next_chapter_ = 1;
    if (next_chapter_ < max_chapter_) next_chapter_ = max_chapter_;
    if (next_chapter_ &&
        (next_chapter_ == max_chapter_ || next_chapter_ == max_chapter_ + 1))
      this->current_chapter_ = next_chapter_;
    this->InitArmyWarBoss();
  }
  current = &this->army_war_;
  today_passed = &this->today_wars_;
}

void Army::SaveArmyWar(const sy::ArmyWarInfo& info) {
  LogArmyWar(info);
  int32_t chapter = info.boss1().copy_id() / 10;
  TRACE_LOG(logger)("ArmyID:%ld, ArmyWar:%d save", this->army_id(), chapter);
  const std::string& key = MakeKVStorageKey(kKVPrefixArmy, kKVTypeArmyWarCopy, this->army_id(), chapter);
  const std::string& value = info.SerializeAsString();
  server->SetKeyValue(key, value);
}

void Army::UpdateArmyWarRankList(LogicPlayer* player, int64_t damage) {
  static RankItem item;
  int32_t old_damage = 0;
  const RankItem* i = this->rank_list_.GetRankInfoByUID(player->uid());
  if (i) old_damage = i->damage();
  if (damage > old_damage) old_damage = damage;
  item.Clear();
  //RANK_FIELD_LEVEL | RANK_FIELD_AVATAR | RANK_FIELD_DAMAGE
  item.set_uid(player->uid());
  item.set_name(player->name());
  item.set_damage(old_damage);
  item.set_level(player->level());
  item.set_avatar(player->avatar());
  item.set_exploit(player->army_war_count());//攻打次数
  this->rank_list_.Update(item);
}

void Army::InitArmyWarBoss() {
  if (current_chapter_ <= 0) current_chapter_ = 1;
  if (next_chapter_ < max_chapter_) next_chapter_ = max_chapter_;
  TRACE_LOG(logger)("InitArmyWarBoss, ArmyID:%ld, Chapter:%d", this->army_id(), current_chapter_);
  //初始化当前章节的boss
  ArmyBossInfo* array[] = {
      this->army_war_.mutable_boss1(), this->army_war_.mutable_boss2(),
      this->army_war_.mutable_boss3(), this->army_war_.mutable_boss4(),
  };
  for (int32_t index = 0; index < ArraySize(array); ++index) {
    const ArmyBossBase* base = ARMY_BOSS_BASE.GetEntryByID(current_chapter_ * 10 + index + 1).get();
    if (!base || !base->monster_base) {
      ERROR_LOG(logger)("Init ArmyBoss:%d fail", current_chapter_ * 10 + index + 1);
      continue;
    }
    array[index]->set_copy_id(base->id());
    array[index]->mutable_left_hp()->Resize(6, 0);
    for (int32_t pos = 1; pos <= 6; ++pos) {
      if (!base->monster_base->monster_group[pos]) continue;
      int64_t hp = base->monster_base->GetMonsterAttr(pos).Get(sy::ATTACK_ATTR_HP);
      array[index]->mutable_left_hp()->Set(pos - 1, hp);
    }
    INFO_LOG(logger)("CopyID:%d, HP:%ld,%ld,%ld,%ld,%ld,%ld"
        , array[index]->copy_id()
        , array[index]->left_hp(0), array[index]->left_hp(1)
        , array[index]->left_hp(2), array[index]->left_hp(3)
        , array[index]->left_hp(4), array[index]->left_hp(5)
        );
  }
  this->SaveArmyWar(this->army_war_);
  //保存列表
  this->SaveArmyWars();
}

void Army::LoadArmyWar() {
  //load刷新时间戳
  this->last_war_time_ = this->GetArmyStorageInt32(kKVTypeArmyWarFreshTime);
  //当前章节
  this->current_chapter_ = this->GetArmyStorageInt32(kKVTypeArmyWarCurrent);
  this->next_chapter_ = this->GetArmyStorageInt32(kKVTypeArmyWarNext);
  //最高章节
  this->max_chapter_ = this->GetArmyStorageInt32(kKVTypeArmyWarMax);
  INFO_LOG(logger)("Load ArmyID:%ld, LastWarTime:%d, CurrentChapter:%d, MaxChapter:%d, NextChapter:%d"
      , this->army_id(), this->last_war_time_, this->current_chapter_, this->max_chapter_, this->next_chapter_);
  //load今天打过的副本
  {
    const std::string& key =
        MakeKVStorageKey(kKVPrefixArmy, kKVTypeArmyWarCopys, this->army_id());
    const std::string& value = server->GetKeyValue(key);
    if (!value.empty()) {
      int32_t* chapter = (int32_t*)const_cast<char*>(value.c_str());
      int32_t size = value.size() / sizeof(int32_t);
      sy::ArmyWarInfo info;
      for (int32_t i = 0; i < size; ++i) {
        info.Clear();
        int32_t c = chapter[i];
        const std::string& k1 = MakeKVStorageKey(
            kKVPrefixArmy, kKVTypeArmyWarCopy, this->army_id(), c);
        const std::string& v = server->GetKeyValue(k1);
        if (info.ParseFromString(v)) {
          this->today_wars_.push_back(info);
        } else {
          ERROR_LOG(logger)("Parse ArmyWarInfo fail, ArmyID:%ld, Chapter:%d", this->army_id(), c);
        }
      }
    }
  }
  //正在打的副本
  {
    const std::string& key = MakeKVStorageKey(kKVPrefixArmy, kKVTypeArmyWarCopy, this->army_id(), this->current_chapter_);
    const std::string& value = server->GetKeyValue(key);
    if (!value.empty()) {
      if (!this->army_war_.ParseFromString(value)) {
        ERROR_LOG(logger)("Parse ArmyWarInfo fail, ArmyID:%ld, CurrentChapter:%d"
            , this->army_id(), this->current_chapter_);
        this->army_war_.Clear();
      }
    }
    this->army_war_.mutable_boss1()->mutable_left_hp()->Resize(6, 0);
    this->army_war_.mutable_boss2()->mutable_left_hp()->Resize(6, 0);
    this->army_war_.mutable_boss3()->mutable_left_hp()->Resize(6, 0);
    this->army_war_.mutable_boss4()->mutable_left_hp()->Resize(6, 0);
    LogArmyWar(this->army_war_);
  }
}

int32_t Army::GetArmyStorageInt32(const std::string& type) {
  const std::string& key = MakeKVStorageKey(kKVPrefixArmy, type, this->army_id());
  const std::string& value = server->GetKeyValue(key);
  if (value.empty()) return 0;
  return *(int32_t*)const_cast<char*>(value.c_str());
}

void Army::SetArmyStorageInt32(const std::string& type, int32_t value) {
  const std::string& key = MakeKVStorageKey(kKVPrefixArmy, type, this->army_id());
  const std::string& v = MakeValue(&value, sizeof(int32_t));
  server->SetKeyValue(key, v);
}

void Army::SaveArmyWars() {
  INFO_LOG(logger)("Set ArmyID:%ld, LastWarTime:%d, CurrentChapter:%d, MaxChapter:%d, NextChapter:%d"
      , this->army_id(), this->last_war_time_, this->current_chapter_, this->max_chapter_, this->next_chapter_);
  {
    const std::string& key =
        MakeKVStorageKey(kKVPrefixArmy, kKVTypeArmyWarCopys, this->army_id());
    std::vector<int32_t> today_copy;
    for (size_t i = 0; i < this->today_wars_.size(); ++i) {
      today_copy.push_back(this->today_wars_[i].boss1().copy_id() / 10);
    }
    const std::string& value = MakeValue(&today_copy[0], today_copy.size() * sizeof(int32_t));
    server->SetKeyValue(key, value);
  }
  this->SetArmyStorageInt32(kKVTypeArmyWarNext, this->next_chapter_);
  this->SetArmyStorageInt32(kKVTypeArmyWarMax, this->max_chapter_);
  this->SetArmyStorageInt32(kKVTypeArmyWarFreshTime, this->last_war_time_);
  this->SetArmyStorageInt32(kKVTypeArmyWarCurrent, this->current_chapter_);
}

void Army::next_chapter(int32_t value) {
  this->next_chapter_ = value;
  this->SaveArmyWars();
}

static inline int64_t GetArmyBossLeftHp(const sy::ArmyBossInfo& info) {
  int64_t result = 0;
  for (int32_t i = 0; i < info.left_hp_size(); ++i) {
    int64_t left = info.left_hp(i);
    result += std::max(left, 0l);
  }
  return result;
}

void Army::InitArmyWarBossPersonalAward(sy::ArmyBossInfo& info, const ArmyBossBase* boss) {
  TRACE_LOG(logger)("ArmyID:%ld, BossID:%d Dead, InitPersonalAward"
      , this->army_id(), info.copy_id());
  std::vector<int32_t> award_index;
  for (size_t i = 0; i < boss->reward_choose.size(); ++i) {
    int32_t count = boss->reward_choose[i].v3;
    for (int32_t c = 0; c < count; ++c) award_index.push_back(i);
  }

  award_index.resize(this->member_.size(), 0);

  std::random_shuffle(award_index.begin(), award_index.end());
  for (size_t i = 0; i < this->member_.size(); ++i) {
    const sy::ArmyMemberInfo& member = this->member_[i];
    sy::ArmyBossMemAward* award = info.add_awards();
    award->set_player_id(member.player_id());
    award->set_name(member.name());
    award->set_award(award_index[i]);
    award->set_index(0);
    INFO_LOG(logger)("PlayerID:%ld, Award:%d", member.player_id(), award_index[i]);
  }
}

int32_t Army::OnArmyWarCopy(const ArmyBossBase* boss, const std::string& name) {
  sy::ArmyBossInfo* array[] = {
      this->army_war_.mutable_boss1(), this->army_war_.mutable_boss2(),
      this->army_war_.mutable_boss3(), this->army_war_.mutable_boss4()};
  for (int32_t i = 0; i < ArraySize(array); ++i) {
    if (array[i]->copy_id() != boss->id()) continue;
    if (GetArmyBossLeftHp(*array[i])) return 0;
    array[i]->set_name(name);
    this->InitArmyWarBossPersonalAward(*array[i], boss);
    this->SaveArmyWar(this->army_war_);
    break;
  }

  //增加军团日志
  this->AddArmyLog(MakeArmyLog(ARMY_LOG_BOSS, boss->id(), boss->reward_kill,
                               TypeVoid(), TypeVoid()));

  //检测BOSS是不是全死了
  //如果全死了,进入下一关
  if (GetArmyBossLeftHp(*this->army_war_.mutable_boss1()) ||
      GetArmyBossLeftHp(*this->army_war_.mutable_boss2()) ||
      GetArmyBossLeftHp(*this->army_war_.mutable_boss3()) ||
      GetArmyBossLeftHp(*this->army_war_.mutable_boss4())) {
    return 1;
  }

  this->current_chapter_ = this->army_war_.mutable_boss1()->copy_id() / 10;
  this->max_chapter_ = std::max(this->current_chapter_, this->max_chapter_);
  this->current_chapter_++;
  this->next_chapter_++;
  this->today_wars_.push_back(this->army_war_);
  this->army_war_.Clear();
  this->InitArmyWarBoss();
  return 2;
}

sy::ArmyWarInfo* Army::army_war(int32_t chapter) {
  sy::ArmyWarInfo* _1 = NULL;
  std::vector<sy::ArmyWarInfo>* _2 = NULL;
  this->GetArmyWars(_1, _2);

  if (!chapter || chapter == this->army_war_.boss1().copy_id() / 10)
    return &this->army_war_;
  for (std::vector<sy::ArmyWarInfo>::iterator iter = this->today_wars_.begin();
       iter != this->today_wars_.end(); ++iter) {
    if (iter->boss1().copy_id() / 10 == chapter) {
      return &*iter;
    }
  }
  return NULL;
}

void Army::RefreshArmyShop() {
  if (IsSameDay(this->info_.shop_refresh_time(), GetVirtualSeconds())) {
    int32_t last_refresh_hour = 0;
    const std::vector<int32_t>& refresh_time =
        Setting::GetValue1(Setting::shop9_refresh_time);
    for (std::vector<int32_t>::const_iterator it = refresh_time.begin();
         it != refresh_time.end(); ++it)
      if (*it <= GetTime().tm_hour)
        last_refresh_hour = last_refresh_hour > *it ? last_refresh_hour : *it;
    tm time_tm;
    time_t temp_t = this->info_.shop_refresh_time();
    localtime_r(&temp_t, &time_tm);
    if (time_tm.tm_hour >= last_refresh_hour) return;
  }

  ShopBase::RandomFeatsCommodity(9, this->level(), info_.mutable_army_shop(),
                                 server->GetServerStartDays());
  this->info_.mutable_buy_record()->Clear();
  this->info_.set_shop_refresh_time(GetVirtualSeconds());
  UpdateArmyInfo();
}

void Army::SetMaster(int64_t uid, const std::string& name) {
  this->info_.set_master_id(uid);
  this->info_.set_master_name(name);
  UpdateArmyInfo();
}

void Army::UpdateArmyInfo() {
  intranet::MessageSSUpdateArmyInfo update;
  update.set_army_id(this->army_id());
  update.set_master_id(this->info_.master_id());
  update.set_master_name(this->info_.master_name());
  update.mutable_army_shop()->CopyFrom(this->info_.army_shop());
  update.mutable_buy_record()->CopyFrom(this->info_.buy_record());
  update.set_shop_refresh_time(this->info_.shop_refresh_time());
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_INFO, &update);
}

void Army::AddArmyBuyRecord(int32_t solt, int64_t player_id) {
  sy::ArmyShopRecord* record = this->info_.mutable_buy_record();
  if (!record) return;
  switch (solt) {
    case 1:
      record->add_slot1(player_id);
      break;
    case 2:
      record->add_slot2(player_id);
      break;
    case 3:
      record->add_slot3(player_id);
      break;
    case 4:
      record->add_slot4(player_id);
      break;
    case 5:
      record->add_slot5(player_id);
      break;
    case 6:
      record->add_slot6(player_id);
      break;
    default:
      return;
  }
  if (solt > 0 && solt <= info_.army_shop_size()) {
    ShopCommodityInfo* cf = info_.mutable_army_shop(solt - 1);
    cf->set_bought_count(cf->bought_count() + 1);
  }
  UpdateArmyInfo();
}

int32_t Army::GetArmyShopNum(int32_t shop_id) {
  for (int32_t i = 0; i < info_.army_shop_size(); i++) {
    const sy::ShopCommodityInfo cf = info_.army_shop(i);
    if (cf.commodity_id() == shop_id) return cf.bought_count();
  }
  return 0;
}

bool Army::CanCommodityBuy(int32_t solt, int64_t player_id) {
  const AttrVec* vce = NULL;
  switch (solt) {
    case 1:
      vce = &this->info_.buy_record().slot1();
      break;
    case 2:
      vce = &this->info_.buy_record().slot2();
      break;
    case 3:
      vce = &this->info_.buy_record().slot3();
      break;
    case 4:
      vce = &this->info_.buy_record().slot4();
      break;
    case 5:
      vce = &this->info_.buy_record().slot5();
      break;
    case 6:
      vce = &this->info_.buy_record().slot6();
      break;
    default:
      return false;
  }
  if (!vce) return false;
  for (int32_t i = 0; i < vce->size(); i++) {
    if (vce->Get(i) == player_id) return false;
  }
  return true;
}

void Army::ChangePosLog(const std::string& name, const std::string& other_name,
                        ArmyPosition pos) {
  if (pos == ARMY_POSITION_MASTER) {
    AddArmyLog(MakeArmyLog(sy::ARMY_LOG_CHANGE_MASTER, name, other_name,
                           TypeVoid(), TypeVoid()));
  }
  if (pos == ARMY_POSITION_VP) {
    AddArmyLog(MakeArmyLog(sy::ARMY_LOG_POS_CHANGED, other_name, TypeVoid(),
                           TypeVoid(), TypeVoid()));
  }
}

int32_t Army::AddArmyMerit(int32_t num) {
  info_.set_army_merit(info_.army_merit() + num);
  this->SetArmyStorageInt32(kKVTypeArmyWorldBossMerit, info_.army_merit());
  return info_.army_merit();
}

void Army::ClearArmyMerit() {
  info_.set_army_merit(0);
  this->SetArmyStorageInt32(kKVTypeArmyWorldBossMerit, info_.army_merit());
}

void Army::SendMailToArmyMember(
    int32_t mail_type, const std::string& content,
    const std::vector<std::pair<int32_t, int32_t> >* reward) {
  int64_t mail_time = GetSeconds();
  for (size_t index = 0; index < this->member_.size(); ++index) {
    LogicPlayer::SendMail(this->member_[index].player_id(), mail_time,
                          mail_type, content, reward);
  }
}

void Army::SavePearlHarbor() {
  const std::string& key = MakeKVStorageKey(
      KVStorage::kKVPrefixArmy, KVStorage::kKVTypePearlHarbor, this->army_id());
  server->SetKeyValue(key, pearl_harbor_info_.SerializeAsString());
}

void Army::LoadPearlHarbor() {
  const std::string& key = MakeKVStorageKey(
      KVStorage::kKVPrefixArmy, KVStorage::kKVTypePearlHarbor, this->army_id());
  pearl_harbor_info_.ParseFromString(server->GetKeyValue(key));
}

int32_t PearlMonsterLiveCount(sy::PearlHarborInfo& info) {
  int32_t count = 0;
  for (int32_t i = 0; i < info.monster_hp_size(); i++) {
    for (int32_t j = 0; j < info.monster_hp(i).hp_size(); j++) {
      if (info.monster_hp(i).hp(j) > 0) {
        ++count;
        break;
      }
    }
  }
  return count;
}

void Army::RefreshPearlHarborMonster(int32_t batch) {
  do {
    if (batch > 1) {
      int32_t live_count = PearlMonsterLiveCount(this->pearl_harbor_info_);
      int32_t dead_count =
          this->pearl_harbor_info_.monster_hp_size() - live_count;
      if (dead_count > 0) {
        int32_t break_id = this->pearl_harbor_info_.war_zone() * 10 +
                           this->pearl_harbor_info_.monster_hp(0).quality();
        PearlharborBreakRewardBase* break_base =
            PEARLHARBOR_BREAK_REWARD_BASE.GetEntryByID(break_id).get();
        if (break_base) {
          for (size_t i = 0; i < break_base->reward.size(); i++) {
            KVPair2* pair = this->pearl_harbor_info_.add_break_award();
            pair->set_key(break_base->reward[i].v1);
            pair->set_value(break_base->reward[i].v2 * dead_count);
            KVPair2* pair_ship = this->pearl_harbor_info_.add_break_ship();
            pair_ship->set_key(this->pearl_harbor_info_.monster_hp(0).quality());
            pair_ship->set_value(dead_count);
          }
        }
      }
      this->pearl_harbor_info_.set_invade_count(
          this->pearl_harbor_info_.invade_count() + live_count);
      if (this->pearl_harbor_info_.invade_count() >=
          GetSettingValue(pearlharbor_missing_enemy))
        break;
    }
    PearlharborCopyBase* base =
        PEARLHARBOR_COPY_BASE.GetEntryByID(this->pearl_harbor_info_.war_zone() *
                                               10 +
                                           batch)
            .get();
    if (!base) break;
    this->pearl_harbor_info_.clear_monster_hp();
    this->pearl_harbor_info_.set_batch(batch);

    std::vector<int32_t> group_monster = base->RandomGroupMonster();

    for (size_t i = 0; i < group_monster.size(); i++) {
      sy::PearlHarborMonsterHP* monster_info =
          this->pearl_harbor_info_.add_monster_hp();
      monster_info->set_monster_group(group_monster[i]);
      MonsterGroupBase* group =
          MONSTER_GROUP_BASE.GetEntryByID(group_monster[i]).get();
      if (!group) continue;
      const std::vector<sy::HeroInfo>& heros = group->hero_info();
      int32_t max_hp = 0;
      for (size_t j = 0; j < heros.size(); j++) {
        if (heros[j].hero_id()) {
          max_hp += heros[j].attr1(1);
          monster_info->add_hp(heros[j].attr1(1));
          monster_info->set_quality(heros[j].quality());
        }
      }
      monster_info->set_monster_group_max_hp(max_hp);
    }
  } while (false);

  this->SavePearlHarbor();
}

void Army::RefreshPearlHarbor() {
  int32_t war_zone = this->pearl_harbor_info_.tomorrow_type()
                         ? this->pearl_harbor_info_.max_war_zone()
                         : this->pearl_harbor_info_.max_war_zone() - 1;
  if (war_zone < 1) war_zone = 1;

  this->pearl_harbor_info_.set_war_zone(war_zone);
  this->pearl_harbor_info_.set_buff_end_time(0);
  this->pearl_harbor_info_.set_invade_count(0);
  this->pearl_harbor_info_.set_fresh_time(GetVirtualSeconds());

  if (!server->IsPearlHarborTime())
    this->pearl_harbor_info_.set_buff_used_count(0);

  if (this->pearl_harbor_info_.break_award_size()) {
    std::vector<std::pair<int32_t, int32_t> > award;
    for (int32_t i = 0; i < this->pearl_harbor_info_.break_award_size(); i++) {
      const KVPair2& pair = this->pearl_harbor_info_.break_award(i);
      award.push_back(std::make_pair(pair.key(), pair.value()));
    }
    for (size_t i = 0; i < member_.size(); i++)
      LogicPlayer::SendMail(member_[i].player_id(), GetSeconds(),
                            MAIL_TYPE_PEARL_HARBOR_BREAK, "", &award);
    this->pearl_harbor_info_.mutable_break_award()->Clear();
  }
  this->pearl_harbor_info_.mutable_break_ship()->Clear();
  this->pearl_harbor_info_.mutable_player_today_score()->Clear();

  this->SavePearlHarbor();
}
