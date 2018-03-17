#include "pk.h"
#include "config.h"
#include "server.h"
#include <myrandom.h>
#include <logger.h>
#include <array_stream.h>

static int32_t BuffUniqueID = 1;
typedef ArrayStream<1028 * 1024> Stream;

typedef google::protobuf::RepeatedField<google::protobuf::int64> RepeatedInt64;
void AttrToString(Stream& stream, const RepeatedInt64& attr) {
  stream.Append("[");
  int32_t output_count = 0;
  for (int32_t index = 0; index < sy::AttackAttr_ARRAYSIZE; ++index) {
    if (attr.Get(index)) {
      if (output_count) stream.Append(",");
      stream.Append("%d|%ld", index, attr.Get(index));
      output_count++;
    }
  }
  stream.Append("]");
}

std::string ToString(const sy::ReportRecord& report) {
  Stream stream;

  stream.Append("ReportUID:%ld\nMapID:%d\nRandom:%d\nHpPercent:%d\n", report.report_uid(),
                report.map_id(), report.random(), report.hp_percent());

  if (report.players_size())
    stream.Append("PlayerID:%ld, Name:%s, Level:%d\n",
                  report.players(0).player_id(),
                  report.players(0).name().c_str(), report.players(0).level());
  if (report.players_size() > 1)
    stream.Append("PlayerID:%ld, Name:%s, Level:%d\n",
                  report.players(1).player_id(),
                  report.players(1).name().c_str(), report.players(1).level());
  for (int32_t i = 0; i < report.ship_info_size(); ++i) {
    stream.Append(
        "HeroUID:%ld, HeroID:%d, Pos:%d, Grade:%d, Level:%d, Fate:%d, Quality:%d, "
        "HP:%ld, Anger:%d, Attr:",
        report.ship_info(i).hero_info().uid(),
        report.ship_info(i).hero_info().hero_id(),
        report.ship_info(i).position(), report.ship_info(i).hero_info().grade(),
        report.ship_info(i).hero_info().level(),
        report.ship_info(i).hero_info().fate_level(),
        report.ship_info(i).hero_info().quality(),
        report.ship_info(i).current_hp(),
        report.ship_info(i).anger());
    AttrToString(stream, report.ship_info(i).hero_info().attr1());
    stream.Append("\n");
  }

  for (int32_t i = 0; i < report.carrier_info_size(); ++i) {
    stream.Append("CarrierID:%d", report.carrier_info(i).carrier().carrier_id());
    stream.Append(", Quality:%d", report.carrier_info(i).carrier().quality());
    stream.Append(", Attr:");
    AttrToString(stream, report.carrier_info(i).carrier().attr1());

    stream.Append(", TowerAttr:");
    AttrToString(stream, report.carrier_info(i).carrier().tower_attr1());
    stream.Append("\n");
  }

  for (int32_t i = 0; i < report.report_info_size(); ++i) {
    if (report.report_info(i).content_size() < 1) continue;
    if (report.report_info(i).content(0) == 17) {
      //属性调试信息
      int32_t index = 0;
      stream.Append("%ld", int64_t(report.report_info(i).content(index++)));
      stream.Append(", %ld", int64_t(report.report_info(i).content(index++)));
      stream.Append(", Attr:[");
      for (; index < report.report_info(i).content_size(); index += 2) {
        if (index != 2) stream.Append(",");
        stream.Append("%d|%d", int32_t(report.report_info(i).content(index)),
                      int32_t(report.report_info(i).content(index + 1)));
      }
      stream.Append("]\n");
      continue;
    }
    for (int32_t index = 0; index < report.report_info(i).content_size();
         ++index) {
      if (index != 0) stream.Append(", ");
      stream.Append("%ld", int64_t(report.report_info(i).content(index)));
    }
    stream.Append("\n");
  }

  return stream.str();
}

void LogReportToFile(const sy::ReportRecord& report) {
  const std::string& str = ToString(report);
  char file_name[256] = {0};
  snprintf(file_name, sizeof file_name, "./log/report_%lu.txt", report.report_uid());
  FILE* file = fopen(file_name, "a+");
  fwrite(str.c_str(), str.length(), 1, file);
  fclose(file);
}

BuffState::BuffState() : uid(0), attacker(NULL), target(NULL), a_side(true) {
  this->r.reset(new int32_t(0));
  this->r1.reset(new int32_t(0));
}

int64_t PkHero::AddHp(int64_t delta) {
  int64_t left = delta;
  if (delta < 0) {
    if (delta + this->state.current_shield >= 0) {
      this->state.current_shield += delta;
      return delta;
    }
    left += this->state.current_shield;
    this->state.current_shield = 0;
    this->state.current_hp += left;
    if (this->state.current_hp < 0) this->state.current_hp = 0;
    return delta;
  }
  this->state.current_hp += left;
  if (this->state.current_hp > this->hero.attr1(sy::ATTACK_ATTR_HP))
    this->state.current_hp = this->hero.attr1(sy::ATTACK_ATTR_HP);
  return delta;
}

bool PkHero::CheckPass() {
  if (this->hp() <= 0 && this->state.buffs2.empty()) return true;
  return false;
}

enum {
  kReportTag_Round      = 1,    //回合开始
  kReportTag_Skill      = 2,    //技能释放
  kReportTag_Buff       = 6,    //Buff起效
  kReportTag_AddBuff    = 8,    //加Buff
  kReportTag_Group      = 9,    //波数开始
  kReportTag_Star       = 10,   //战斗结束
  kReportTag_Attack     = 11,   //攻击开始
  kReportTag_RemoveBuff = 12,   //卸载Buff
  kReportTag_HeroState  = 13,   //船状态
  kReportTag_Anger      = 14,   //怒气
  kReportTag_GroupInfo  = 15,   //站位情况(血量要恢复到战斗开始前的起始值)
  kReportTag_Shield     = 16,   //护盾
  kReportTag_DebugAttr  = 17,   //调试属性
  kReportTag_SceneAttr  = 18,   //场景增加的属性
  kReportTag_ExtraDamage= 19,   //额外伤害
};

//增加调试属性
void PkHero::AddDebugAttr(PK& pk) {
  if (!server_config->report_attr()) return;
  pk.line.Clear();
  pk.line.Add(kReportTag_DebugAttr, this->uid());
#define ADD_DEBUG_ATTR(k,v) if (v) pk.line.Add(k,v)

  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_AP,             this->state.ap);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_AP_PERCENT,     this->state.ap_ratio);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_SP,             this->state.sp);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_SP_PERCENT,     this->state.sp_ratio);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_WF,             this->state.wf);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_WF_PERCENT,     this->state.wf_ratio);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_FF,             this->state.ff);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_FF_PERCENT,     this->state.ff_ratio);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_HIT_PERCENT,    this->state.hit);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_MISS_PERCENT,   this->state.miss);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_CRIT_PERCENT,   this->state.crit);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_RESIST_PERCENT, this->state.resist);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_DAMAGE,         this->state.damage);
  ADD_DEBUG_ATTR(sy::ATTACK_ATTR_DAMAGE_DECREASE,this->state.damage_dec);

#undef ADD_DEBUG_ATTR
  pk.AddReportContent(pk.line);
}

void PkHero::AddStateReport(PK& pk, ReportLine& line) {
  line.Clear().Add(kReportTag_HeroState, this->uid(), this->state.state);
  pk.AddReportContent(pk.line);
}

enum {
  kAttack_Normal        = 0,      //命中
  kAttack_Crit          = 1 << 1, //暴击
  kAttack_Miss          = 1 << 2, //闪避
  kAttack_Ignore        = 1 << 3, //免疫
  kAttack_Fail          = 1 << 4, //失败
  kAttack_God           = 1 << 5, //无敌
  kAttack_Double        = 1 << 6, //双倍
  kAttack_AddHp         = 1 << 7, //生命之光
  kAttack_Damage        = 1 << 8, //伤害反弹
  kAttack_Bullying      = 1 << 9, //破势
  kAttack_IgnoreDefence = 1 << 10,  //破防
  kAttack_Vampiric      = 1 << 11,  //吸血
};



//PK公式
//返回值: first代表命中/暴击/闪避, second代表具体的伤害
//A攻击B, 需要传入A和B
//hit_ratio, 命中修正, 如果B是NULL, 那么hit_ratio就开始作用
std::pair<int64_t, int64_t> NormalDamage(IPkObject* a, IPkObject* b, bool is_ap,
                                         int32_t skill_ratio,
                                         int32_t skill_damage,
                                         int32_t hit_ratio, RandomNubmer& rand,
                                         int64_t* add_hp,
                                         int64_t* damage_reback) {
  int64_t damage_mask = kAttack_Normal;
  int64_t real_damage = 0;
  //命中判断
  do {
    if (!b) hit_ratio = 1000;
    double hit =
        (hit_ratio ? hit_ratio : (1000 + a->hit() - b->miss())) / 1000.0;
    hit = hit <= 0.3 ? 0.3 : hit;
    if (!rand.success(hit)) {
      damage_mask = kAttack_Miss;
      break;
    }

    int32_t extra_damage_ratio = 0;
    //千分比
    int32_t wf_sub_percent = 0;
    damage_mask = 0;
    //额外伤害判断
    if (a->pos()) {
      double self_hp_percent = 1.0 * a->hp() / a->max_hp();
      double target_hp_percent = 1.0 * b->hp() / b->max_hp();
      //破势
      if (a->bullying() && self_hp_percent >= target_hp_percent) {
        extra_damage_ratio += a->bullying();
        damage_mask |= kAttack_Bullying;
      }
      if (self_hp_percent <= 0.5) {
        extra_damage_ratio += a->tenacity();
      }
      extra_damage_ratio -= is_ap ? b->ap_bullying_sub() : b->sp_bullying_sub();
      //破防
      double ignore_defence_percent = a->ignore_defence() / 1000.0;
      if (rand.success(ignore_defence_percent)) {
        damage_mask |= kAttack_IgnoreDefence;
        wf_sub_percent = 350;
        extra_damage_ratio += a->ignore_defence_inc();
        extra_damage_ratio -= b->ignore_defence_sub();
      }
    }

    //普通伤害 = (max(A攻击 * 5%, (A攻击-B防御) * 伤害百分) + 固定值伤害)
    //         * max(0.1, (1 + A伤害加成 - B伤害减免 + 额外伤害))
    int64_t ap = is_ap ? a->ap() : a->sp();
    int64_t wf = b ? (is_ap ? b->wf() : b->ff()) : 0;
    wf = wf * (1000 - wf_sub_percent) / 1000;
    int64_t min_damage = ap * 5 / 100;
    int64_t damage = (ap - wf) * skill_ratio / 1000;
    int32_t damage_ratio =
        std::max(100, 1000 + extra_damage_ratio + a->damage() -
                          b->damage_dec() + a->country_damage(b->country()) -
                          b->country_damage_dec(a->country()));
    real_damage =
        (std::max(min_damage, damage) + skill_damage) * damage_ratio / 1000;
    //暴击和双倍伤害计算
    if (b) {
      double pk_hit = (a->pk_hit() - b->pk_hit_sub()) / 1000.0;
      double crit = (a->crit() - b->resist()) / 1000.0;
      if (rand.success(crit)) {
        real_damage = real_damage * Setting::kCrit / 100;
        damage_mask |= kAttack_Crit;
      } else if (rand.success(pk_hit)) {
        real_damage = real_damage * 2;
        damage_mask |= kAttack_Double;
      }
    }
    if (a->pos()) {
      //伤害反弹
      if (damage_reback && (b->ap_reback() + b->sp_reback()) && rand.success(0.5)) {
        int64_t damage = (is_ap ? b->ap_reback() : b->sp_reback()) * real_damage / 1000;
        *damage_reback += damage;
      }
      //吸血
      if (add_hp && a->vampiric() && rand.success(0.1)) {
        int32_t damage = real_damage * a->vampiric() / 1000;
        *add_hp += damage;
      }
    }
  } while (false);

  return std::make_pair<int64_t, int64_t>(damage_mask, real_damage);
}

std::pair<int64_t, int64_t> NormalAddHp(IPkObject* a, IPkObject* b, bool is_ap,
                                        int32_t skill_ratio,
                                        int32_t skill_damage, int32_t hit_ratio,
                                        RandomNubmer& rand) {
  //命中判断
  if (!b) hit_ratio = 1000;
  double hit =  (hit_ratio ? hit_ratio : (1000 + a->hit() - b->miss())) / 1000.0;
  if (!rand.success(hit)) {
    return std::make_pair<int64_t, int64_t>(kAttack_Miss, 0);
  }

  //普通伤害 = (max(A攻击 * 5%, (A攻击-B防御) * 伤害百分) + 固定值伤害)
  //         * max(0.1, (1 + A伤害加成 - B伤害减免))
  int64_t ap = is_ap ? a->ap() : a->sp();
  int64_t min_damage = ap * 5 / 100;
  int64_t damage = ap * skill_ratio / 1000;
  int64_t real_damage = (std::max(min_damage, damage) + skill_damage);
  if (b) {
    double crit = (a->crit() - b->resist()) / 1000.0;
    if (rand.success(crit)) {
      real_damage = real_damage * Setting::kCrit / 100;
      return std::make_pair<int64_t, int64_t>(kAttack_Crit, real_damage);
    }
  }
  return std::make_pair<int64_t, int64_t>(kAttack_Normal, real_damage);
}

////////////////////////////////
//BUFF BEGIN
////////////////////////////////
const int32_t kBuffTypeCount = 256;
typedef void (*BuffCallback)(BuffParam& param);
static BuffCallback kBuffCallback[kBuffTypeCount] = {NULL};

static inline void DoBuffAttack(BuffParam& param, const BuffBasePtr& buff_base);

#define REG_BUFF(ID)                                                      \
static void    BuffFn_##ID(BuffParam& param);                             \
static int32_t reg_##ID() { kBuffCallback[ID] = BuffFn_##ID; return 1; }  \
static int32_t __##ID = reg_##ID();                                       \
static void    BuffFn_##ID(BuffParam& param)

static inline int32_t GetConfigRatio(const BuffBasePtr& buff_base) {
  int32_t delta = buff_base->pro_type1.v1 == 2 ? buff_base->pro_type1.v2 : 0;
  delta = buff_base->buff_value_type == 2 ? -delta : delta;
  return delta;
}

static inline int32_t GetConfigValue(const BuffBasePtr& buff_base) {
  int32_t delta = buff_base->pro_type1.v1 == 1 ? buff_base->pro_type1.v2 : 0;
  delta = buff_base->buff_value_type == 2 ? -delta : delta;
  return delta;
}

//1 火炮攻击(属性类型为2时，单位为千分比)
REG_BUFF(1) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  int32_t value = GetConfigValue(buff_base);
  int32_t ratio = GetConfigRatio(buff_base);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.ap +=       value;
      hero->state.ap_ratio += ratio;
      hero->state.sp +=       value;
      hero->state.sp_ratio += ratio;
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.ap -=       value;
      hero->state.ap_ratio -= ratio;
      hero->state.sp -=       value;
      hero->state.sp_ratio -= ratio;
    } break;
  }
}
//3 火炮防御(属性类型为2时，单位为千分比)
REG_BUFF(3) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  int32_t value = GetConfigValue(buff_base);
  int32_t ratio = GetConfigRatio(buff_base);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.wf +=       value;
      hero->state.wf_ratio += ratio;
      hero->state.ff +=       value;
      hero->state.ff_ratio += ratio;
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.wf -=       value;
      hero->state.wf_ratio -= ratio;
      hero->state.ff -=       value;
      hero->state.ff_ratio -= ratio;
    } break;
  }
}
//5 命中率(单位为千分比)
REG_BUFF(5) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.hit += GetConfigValue(buff_base);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.hit -= GetConfigValue(buff_base);
    } break;
  }
}
//6 闪避率(单位为千分比)
REG_BUFF(6) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.miss += GetConfigValue(buff_base);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.miss -= GetConfigValue(buff_base);
    } break;
  }
}
//7 暴击率(单位为千分比)
REG_BUFF(7) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.crit += GetConfigValue(buff_base);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.crit -= GetConfigValue(buff_base);
    } break;
  }
}
//8 抗暴率(单位为千分比)
REG_BUFF(8) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.resist += GetConfigValue(buff_base);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.resist -= GetConfigValue(buff_base);
    } break;
  }
}
//9 伤害加成(单位为千分比)
REG_BUFF(9) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.damage += GetConfigValue(buff_base);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.damage -= GetConfigValue(buff_base);
    } break;
  }
}
//10  伤害减免(单位为千分比)
REG_BUFF(10) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      hero->state.damage_dec += GetConfigValue(buff_base);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
      hero->state.damage_dec -= GetConfigValue(buff_base);
    } break;
  }
}
//11  怒气（舰船的怒气值范围为：0-读表的值；航母的怒气值范围为：0-读表的值）
REG_BUFF(11) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      hero->AddAnger(GetConfigValue(buff_base), param.pk, param.line);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad:{
    } break;
  }
}

//12  护盾（可吸收伤害的护盾，在护盾破损前自身血量不会减少，可被加血。）
REG_BUFF(12) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  PkHero* hero = static_cast<PkHero*>(param.target);
  switch (param.action) {
    case kBuff_Load: {
      int32_t shield = int64_t(buff_base->pro_type3.v1) *
                           std::max(param.buff_state.attacker->ap(),
                                    param.buff_state.attacker->sp()) /
                           10000 +
                       buff_base->pro_type3.v2;

      hero->state.current_shield = shield;
      param.pk.AddReportContent(param.line.Clear().Add(
          kReportTag_Shield, hero->uid(), hero->state.current_shield));
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
      hero->state.current_shield = 0;
      param.pk.AddReportContent(param.line.Clear().Add(
          kReportTag_Shield, hero->uid(), hero->state.current_shield));
    } break;
  }
}

static inline void DoBuffAttack(BuffParam& param, const BuffBasePtr& buff_base) {
  PkHero* hero = static_cast<PkHero*>(param.target);
  (void)hero;
  bool add_or_sub = buff_base->buff_value_type == 1 ? true : false;
  int32_t hit = 0;
  int64_t modify_hp = 0;
  //按照目标血量计算
  if (buff_base->pro_type2.v1 || buff_base->pro_type2.v2) {
    // 1是当前血量, 2是最大血量
    int64_t base_hp = buff_base->pro_type2.v1 == 1 ? param.target->hp()
                                                   : param.target->max_hp();
    int32_t delta = base_hp * buff_base->pro_type2.v2 / 100 / 100;
    hit |= param.pk.rand.success(buff_base->pro / 1000000.0) ? (1 << 2) : 0;
    modify_hp = add_or_sub ? delta : -delta;
  }
  //按照伤害公式计算
  if (buff_base->pro_type3.v1 || buff_base->pro_type3.v2) {
    std::pair<int64_t, int64_t> result =
        add_or_sub ? NormalAddHp(param.buff_state.attacker, param.target,
                                 param.buff_state.skill->fight_type1 == 1,
                                 buff_base->pro_type3.v1 / 10,
                                 buff_base->pro_type3.v2, 1000, param.pk.rand)
                   : NormalDamage(param.buff_state.attacker, param.target,
                                  param.buff_state.skill->fight_type1 == 1,
                                  buff_base->pro_type3.v1 / 10,
                                  buff_base->pro_type3.v2, 1000, param.pk.rand,
                                  NULL, NULL);
    hit = result.first;
    modify_hp = add_or_sub ? result.second : -result.second;
  }
  //无敌
  if (param.target->IsSetState(PkState_God) && !add_or_sub) {
    modify_hp = std::max(modify_hp, 0l);
    hit = modify_hp ? 0 : kAttack_God;
  }
  //如果闪避, 伤害就为0
  if (hit & kAttack_Miss) modify_hp = 0;
  param.target->AddHp(modify_hp);
  if (modify_hp || hit)
    param.pk.AddReportContent(param.line.Clear().Add(
        kReportTag_Buff, buff_base->id(), param.target->uid(), modify_hp, hit));
}

//100  血量
REG_BUFF(100) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  switch (param.action) {
    case kBuff_Load: {
    } break;
    case kBuff_Tick_A: {
    } break;
    case kBuff_Tick_B: {
      param.buff_state.roundB()++;
      DoBuffAttack(param, buff_base);
    } break;
    case kBuff_UnLoad: {
    } break;
  }
}

//50  无敌（每次只会受到0点伤害）
REG_BUFF(50) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_God, true);
      hero->AddStateReport(param.pk, param.line);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_God, false);
      hero->AddStateReport(param.pk, param.line);
    } break;
  }
}
//51  瘫痪（眩晕，跳过本回合，本回合无法行动）
REG_BUFF(51) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Stun, true);
      hero->AddStateReport(param.pk, param.line);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Stun, false);
      hero->AddStateReport(param.pk, param.line);
    } break;
  }
}
//52  混乱（本回合内只能进行不分敌我的普通攻击，不能自己攻击自己）
REG_BUFF(52) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Confuse, true);
      hero->AddStateReport(param.pk, param.line);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Confuse, false);
      hero->AddStateReport(param.pk, param.line);
    } break;
  }
}
//53  沉默（本回合内只能进行普通攻击)
REG_BUFF(53) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Slicent, true);
      hero->AddStateReport(param.pk, param.line);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Slicent, false);
      hero->AddStateReport(param.pk, param.line);
    } break;
  }
}
//54  驱散伤害（移除友军身上的持续扣血类DEBUFF，BUFF类型：100
REG_BUFF(54) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);(void)hero;
      std::vector<BuffState> buffs = hero->state.buffs;
      for (std::vector<BuffState>::iterator iter = buffs.begin();
           iter != buffs.end(); ++iter) {
        const BuffBasePtr& base = iter->base;
        if (base->buff_value_type == 1 && base->target == 100) {
          hero->RemoveBuff(hero, base->id(), param.pk, param.line);
        }
      }
    } break;
    case kBuff_Tick_A: {
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
    } break;
  }
}
//55  驱散不良BUFF
REG_BUFF(55) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);(void)hero;
      std::vector<BuffState> buffs = hero->state.buffs;
      for (std::vector<BuffState>::iterator iter = buffs.begin();
           iter != buffs.end(); ++iter) {
        const BuffBasePtr& base = iter->base;
        if (base->buff_value_type == 2) {
          hero->RemoveBuff(hero, base->id(), param.pk, param.line);
        }
      }
    } break;
    case kBuff_Tick_A: {
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
    } break;
  }
}
//56  驱散增益BUFF（移除敌军身上的增益BUFF，BUFF类型：1-10）
REG_BUFF(56) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);(void)hero;
      std::vector<BuffState> buffs = hero->state.buffs;
      for (std::vector<BuffState>::iterator iter = buffs.begin();
           iter != buffs.end(); ++iter) {
        const BuffBasePtr& base = iter->base;
        if (base->buff_value_type == 1) {
          hero->RemoveBuff(hero, base->id(), param.pk, param.line);
        }
      }
    } break;
    case kBuff_Tick_A: {
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
    } break;
  }
}
//57  免疫所有DEBUFF（先移除目标身上的所有DEBUFF，然后目标在本回合内免疫所有类型的DEBUFF，该BUFF不能被其他BUFF移除，BUFF类型：所有类型的DEBUFF）
REG_BUFF(57) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Immuno, true);
      hero->AddStateReport(param.pk, param.line);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Immuno, false);
      hero->AddStateReport(param.pk, param.line);
    } break;
  }
}
//BOSS免疫, 免疫眩晕,混乱,沉默
REG_BUFF(58) {
  switch (param.action) {
    case kBuff_Load: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Boss, true);
      hero->AddStateReport(param.pk, param.line);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
      PkHero* hero = static_cast<PkHero*>(param.target);
      hero->state.SetState(PkState_Boss, false);
      hero->AddStateReport(param.pk, param.line);
    } break;
  }
}
//立即伤害
REG_BUFF(59) {
  const BuffBasePtr& buff_base = param.buff_state.base;
  switch (param.action) {
    case kBuff_Load: {
      DoBuffAttack(param, buff_base);
    } break;
    case kBuff_Tick_A: {
      param.buff_state.roundA()++;
    } break;
    case kBuff_Tick_B: {
    } break;
    case kBuff_UnLoad: {
    } break;
  }
}

////////////////////////////////
//BUFF END
////////////////////////////////

struct BuffEqualUid {
  BuffEqualUid(int32_t uid) : uid(uid) {}
  bool operator()(const BuffState& buff) const { return buff.uid == uid; }
  int32_t uid;
};

struct BuffEqualId {
  BuffEqualId(int32_t id) : id(id) {}
  bool operator()(const BuffState& buff) const { return buff.base->id() == id; }
  int32_t id;
};

struct BuffEqualGroup {
  BuffEqualGroup(int32_t group) : group(group) {}
  bool operator()(const BuffState& buff) const { return buff.base->guorp_id == group; }
  int32_t group;
};

void PkHero::RemoveBuff(IPkObject* hero, int32_t buff_id, PK& pk,
                        ReportLine& line) {
  for (std::vector<BuffState>::iterator iter = hero->Buffs().begin();
       iter != hero->Buffs().end(); ++iter) {
    if (iter->base->id() == buff_id) {
      BuffParam param(*iter, hero, pk, line);
      param.action = kBuff_UnLoad;
      BuffCallback fn = kBuffCallback[iter->base->buff_type];
      if (fn) fn(param);
      pk.AddReportContent(
          line.Clear().Add(kReportTag_RemoveBuff, buff_id, hero->uid()));
      if (iter->base->buff_type < 50) {
        this->AddDebugAttr(pk);
      }
      hero->Buffs().erase(iter);
      break;
    }
  }
}

void IPkObject::BuffTick(PK& pk, ReportLine& line) {
  if (this->hp() <= 0) return;

  std::vector<BuffState> buffs;
  //B Side's Buff
  for (std::vector<BuffState>::iterator iter = this->Buffs().begin();
       iter != this->Buffs().end(); ++iter) {
    buffs.push_back(*iter);
    buffs.back().target = static_cast<PkHero*>(this);
    buffs.back().a_side = false;
  }

  //卸载buff
  for (std::vector<BuffState>::iterator iter = buffs.begin();
       iter != buffs.end(); ++iter) {
    BuffParam param(*iter, this, pk, line);
    param.action = iter->a_side ? kBuff_Tick_A : kBuff_Tick_B;
    BuffCallback fn = kBuffCallback[iter->base->buff_type];
    if (fn) fn(param);

    if (iter->roundB() >= iter->base->last) {
      this->RemoveBuff(iter->target, iter->base->id(), pk, line);
    }
  }
}

void PkHero::AddBuffIndex(int64_t hero_uid, int32_t buff_uid) {
  this->state.buffs2.push_back(std::make_pair(hero_uid, buff_uid));
}

void PkHero::AddBuff(const BuffState& state) {
  this->state.buffs.push_back(state);
}

void PkHero::AddBuff(IPkObject* attacker, SkillBase* skill, BuffBase* buff,
                     PK& pk, ReportLine& line) {
  //卸载Buff
  int32_t result = 0;
  //免疫(所有DEBUFF)
  if ((this->state.IsSetState(PkState_Immuno)) && buff->buff_value_type == 2) {
    result |= kAttack_Ignore;
  }
  //BOSS免疫(眩晕,沉默,混乱)
  if ((this->state.IsSetState(PkState_Boss)) &&
      (buff->buff_type == 51 || buff->buff_type == 52 ||
       buff->buff_type == 53)) {
    result |= kAttack_Ignore;
  }
  //随机没成功
  if (!pk.rand.success(buff->pro / 1000000.0)) {
    result |= kAttack_Fail;
#ifdef DEBUG
#else
    return;
#endif
  }

  if (!result) {
    //T掉同组buff
    std::vector<BuffState> buffs;
    this->FindBuffs(buffs, BuffEqualGroup(buff->guorp_id));
    for (std::vector<BuffState>::iterator iter = buffs.begin();
         iter != buffs.end(); ++iter) {
      this->RemoveBuff(this, iter->base->id(), pk, line);
    }

    BuffState buff_state;
    buff_state.uid = BuffUniqueID++;
    buff_state.target = this;
    buff_state.attacker = attacker;
    buff_state.skill =
        skill ? boost::static_pointer_cast<SkillBase>(skill->shared_from_this())
              : SkillBasePtr();
    buff_state.base = boost::static_pointer_cast<BuffBase>(buff->shared_from_this());
    this->AddBuff(buff_state);
    attacker->AddBuffIndex(this->uid(), buff_state.uid);

    pk.AddReportContent(line.Clear().Add(kReportTag_AddBuff, attacker->uid(),
                                         skill ? skill->id() : 0, buff->id(),
                                         this->uid(), result));

    BuffParam param(buff_state, this, pk, line);
    param.action = kBuff_Load;
    BuffCallback fn = kBuffCallback[buff_state.base->buff_type];
    if (fn) fn(param);
    if (buff_state.base->buff_type < 50) {
      this->AddDebugAttr(pk);
    }
  } else {
    pk.AddReportContent(line.Clear().Add(kReportTag_AddBuff, attacker->uid(),
                                         skill ? skill->id() : 0, buff->id(),
                                         this->uid(), result));
  }
}

bool PkHero::CanUseCombo(PK& pk) const {
  if (!this->skill3) return false;
  if (this->hp() <= 0) return false;
  //眩晕, 混乱, 沉默状态不能使用技能
  if (this->state.IsSetState(PkState_Stun | PkState_Confuse | PkState_Slicent))
    return false;
  if (skill3 && this->anger() < skill3->fury) return false;
  for (std::vector<int32_t>::const_iterator iter = this->combo.begin();
       iter != this->combo.end(); ++iter) {
    if (!pk.CheckHeroAlive(this->uid() > 0, *iter)) return false;
  }
  return true;
}

SkillBasePtr PkHero::UseSkill(PK& pk) const {
  static SkillBasePtr empty;
  if (this->CanUseCombo(pk))
    return this->skill3;
  else if (this->CanUseSkill())
    return this->skill2;
  else if (this->CanAttack())
    return skill1;
  return empty;
}

void IPkObject::Attack(PK& pk, ReportLine& line) {
  if (this->hp() <= 0) return;

  SkillBasePtr skill = this->UseSkill(pk);
  if (!skill) return;

  std::vector<PkHero*> target;
  if (this->IsSetState(PkState_Confuse)) {
    pk.GetTarget(this, skill->target_type == 1, target, 21);
  } else {
    pk.GetTarget(this, skill->target_type == 1, target, skill->target);
  }

  //技能释放前加buff
  for (std::vector<BuffBasePtr>::iterator iter = skill->buff_ptr.begin();
       iter != skill->buff_ptr.end(); ++iter) {
    const BuffBasePtr& buff = *iter;
    if (buff->bufftimes == 1) {
      std::vector<PkHero*> buff_target;
      if (buff->target == 50 || buff->target == 51) {
        buff_target = target;
      } else {
        pk.GetTarget(this, buff->target_type == 1, buff_target, buff->target);
      }
      //增加BUFF
      for (std::vector<PkHero*>::iterator iter_target = buff_target.begin();
           iter_target != buff_target.end(); ++iter_target) {
        (*iter_target)->AddBuff(this, skill.get(), buff.get(), pk, line);
      }
    }
  }

  //加怒气
  int32_t delta_anger = (skill->fury ? -skill->fury : 2) +
                        this->attr().Get(sy::ATTACK_ATTR_ANGER_RECOVER);
  this->AddAnger(delta_anger, pk, line);

  int64_t add_hp = 0;
  int64_t damage_reback = 0;
  int32_t add_or_sub = skill->target_type == 1;
  //技能伤害
  for (std::vector<PkHero*>::iterator iter = target.begin();
       iter != target.end(); ++iter) {
    //战斗系统改造
    std::pair<int64_t, int64_t> result =
        add_or_sub ? NormalAddHp(this, *iter,
                                 //技能伤害类型只有1/2
                                 skill->fight_type1 == 1,
                                 this->skill_damage_ratio(skill.get()),
                                 this->skill_damage(skill.get()), 1000, pk.rand)
                   : NormalDamage(this, *iter,
                                  //技能伤害类型只有1/2
                                  skill->fight_type1 == 1,
                                  this->skill_damage_ratio(skill.get()),
                                  this->skill_damage(skill.get()), 0, pk.rand,
                                  &add_hp, &damage_reback);
    int32_t hit = result.first;
    int64_t modify_hp = add_or_sub ? result.second : -result.second;
    //无敌
    if ((*iter)->state.IsSetState(PkState_God) && !add_or_sub) {
      modify_hp = std::max(modify_hp, 0l);
      hit = modify_hp ? 0 : kAttack_God;
    }

    //如果闪避, 伤害就为0
    if (hit & kAttack_Miss) modify_hp = 0;
    (*iter)->AddHp(modify_hp);
    if (modify_hp || hit)
      pk.AddReportContent(line.Clear().Add(kReportTag_Skill, this->uid(),
                                                skill->id(), (*iter)->uid(),
                                                modify_hp, hit));
    //生命之光,并且还活着
    if (modify_hp && (*iter)->hp()) {
      int64_t add_hp = ((*iter)->add_hp() - this->add_hp_sub()) * (*iter)->max_hp() / 1000;
      double percent = 300 / 1000.0;
      if (add_hp > 0 && percent > 0 && pk.rand.success(percent)) {
        (*iter)->AddHp(add_hp);
        pk.AddReportContent(line.Clear().Add(
            kReportTag_ExtraDamage, (*iter)->uid(), add_hp, kAttack_AddHp));
      }
    }
  }
  //吸血和伤害反弹
  if (add_hp) {
    this->AddHp(add_hp);
    pk.AddReportContent(line.Clear().Add(kReportTag_ExtraDamage, this->uid(),
                                         add_hp, kAttack_Vampiric));
  }
  if (damage_reback) {
    this->AddHp(-damage_reback);
    pk.AddReportContent(line.Clear().Add(kReportTag_ExtraDamage, this->uid(),
                                         -damage_reback, kAttack_Damage));
  }

  //技能释放后加buff
  for (std::vector<BuffBasePtr>::iterator iter = skill->buff_ptr.begin();
       iter != skill->buff_ptr.end(); ++iter) {
    const BuffBasePtr& buff = *iter;
    if (buff->bufftimes == 2) {
      std::vector<PkHero*> buff_target;
      if (buff->target == 50 || buff->target == 51) {
        buff_target = target;
      } else {
        pk.GetTarget(this, buff->target_type == 1, buff_target, buff->target);
      }
      //增加BUFF
      for (std::vector<PkHero*>::iterator iter_target = buff_target.begin();
           iter_target != buff_target.end(); ++iter_target) {
        (*iter_target)->AddBuff(this, skill.get(), buff.get(), pk, line);
      }
    }
  }
}

void IPkObject::AutoUnloadBuff(PK& pk, ReportLine& line) {
  std::vector<BuffState> buffs = this->Buffs();
  for (std::vector<BuffState>::iterator iter = buffs.begin();
       iter != buffs.end(); ++iter) {
    if (iter->roundB() >= iter->base->last) {
      this->RemoveBuff(this, iter->base->id(), pk, line);
    }
  }

  buffs.clear();
  //A Side's Buff
  for (std::vector<std::pair<int64_t, int32_t> >::iterator iter =
           this->Buffs2().begin();
       iter != this->Buffs2().end();
       /*++iter*/) {
    PkHero* hero = pk.GetHeroByUID(iter->first);
    if (!hero || hero->hp() <= 0) {
      iter = this->Buffs2().erase(iter);
      continue;
    }
    hero->FindBuffs(buffs, BuffEqualUid(iter->second));
    ++iter;
  }

  for (std::vector<BuffState>::iterator iter = buffs.begin();
       iter != buffs.end(); ++iter) {
    //卸载
    if (iter->roundA() >= iter->base->last) {
      this->RemoveBuff(iter->target, iter->base->id(), pk, line);
      continue;
    }

    BuffParam param(*iter, this, pk, line);
    param.action = iter->a_side ? kBuff_Tick_A : kBuff_Tick_B;
    BuffCallback fn = kBuffCallback[iter->base->buff_type];
    if (fn) fn(param);
  }
}

std::vector<BuffState>& PkHero::Buffs() {
  return this->state.buffs;
}

void PkHero::AddAnger(int32_t anger, PK& pk, ReportLine& line) {
  if (anger < 0 && this->IsSetState(PkState_Boss)) return;

  this->state.current_anger += anger;
  this->state.current_anger =
      std::min<int32_t>(this->state.current_anger, GetSettingValue(fury_max));
  this->state.current_anger = std::max<int32_t>(this->state.current_anger, 0);
  pk.AddReportContent(
      line.Clear().Add(kReportTag_Anger, this->uid(), this->anger(), anger));
}

//这边要根据航母两个技能随机一下
SkillBasePtr PkCarrier::UseSkill(PK& pk) const {
  int32_t sum = 0;
  Array<SkillBasePtr, 10> skills;
  if (this->skill1) {
    sum += this->skill1->fury;
    skills.push_back(this->skill1);
  }
  if (this->skill2) {
    sum += this->skill2->fury;
    skills.push_back(this->skill2);
  }

  if (sum > 0) {
    int32_t random = RandomBetween(0, sum - 1);
    for (size_t i = 0; i < skills.size(); ++i) {
      random -= skills[i]->fury;
      if (random <= 0) return skills[i];
    }
  }

  return SkillBasePtr();
}

//航母接口实现
void PkCarrier::AddBuff(const BuffState& state) {
  return;
}

std::vector<BuffState>& PkCarrier::Buffs() {
  static std::vector<BuffState> empty;
  empty.clear();
  return empty;
}

int32_t PkCarrier::skill_damage_ratio(SkillBase* skill) const {
  return skill->GetSkillDamage(1).first;
}

int32_t PkCarrier::skill_damage(SkillBase* skill) const {
  return skill->GetSkillDamage(1).second;
}

void PkObject::SetCarrier(const sy::CurrentCarrierInfo& info) {
  if (this->carrier.uid()) {
    this->helpers.push_back(info.carrier_id());
  }
  this->carrier.carrier_country = 0;
  this->carrier.carrier = info;
  this->carrier.hero_add_ap = this->carrier.hero_add_sp = 0;
  this->carrier.ap_mean = this->carrier.sp_mean = 0;
  this->carrier.carrier.mutable_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  this->carrier.carrier.mutable_tower_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  const CarrierBasePtr& carrier_base = CARRIER_BASE.GetEntryByID(abs(info.carrier_id()));
  if (carrier_base) {
    this->carrier.carrier_country = carrier_base->country;
    this->carrier.carrier.set_quality(carrier_base->quality);
    std::pair<int32_t, int32_t> skills = carrier_base->GetSkillByGrade(info.grade());
    this->carrier.skill1 = SKILL_BASE.GetEntryByID(skills.first);
    this->carrier.skill2 = SKILL_BASE.GetEntryByID(skills.second);
    if ((skills.first && !this->carrier.skill1) or
        (skills.second && !this->carrier.skill2)) {
      ERROR_LOG(logger)("CarrierID:%d, SkillID:%d, %d, at least one not found"
          , info.carrier_id(), skills.first, skills.second);
    }
  } else {
    const MonsterBasePtr& monster_base = MONSTER_BASE.GetEntryByID(abs(info.carrier_id()));
    if (monster_base) {
      this->carrier.carrier_country = monster_base->country;
      this->carrier.skill1 = SKILL_BASE.GetEntryByID(monster_base->skill.v1);
      this->carrier.skill2 = SKILL_BASE.GetEntryByID(monster_base->skill.v2);
      if ((monster_base->skill.v1 && !this->carrier.skill1) or
          (monster_base->skill.v2 && !this->carrier.skill2)) {
        ERROR_LOG(logger)("CarrierID:%d, SkillID:%d, %d, at least one not found"
          , info.carrier_id(), monster_base->skill.v1, monster_base->skill.v2);
      }
    }
  }

  //this->fight += this->carrier.attr().Get(0);
  INFO_LOG(logger)("SetCarrier:%d", this->carrier.id());;
}

PkHero* PkObject::GetHeroByPos(int32_t pos) {
  for (std::vector<PkHero>::iterator iter = this->heros.begin();
       iter != this->heros.end(); ++iter) {
    if (iter->pos() == pos) return &*iter;
  }
  return NULL;
}

PkHero* PkObject::GetHeroByHeroID(int32_t hero_id) {
  for (std::vector<PkHero>::iterator iter = this->heros.begin();
       iter != this->heros.end(); ++iter) {
    if (iter->id() == hero_id) return &*iter;
  }
  return NULL;
}

void PkObject::InitSceneAttr(const RepeatedInt64* attr) {
  this->scene_attr.resize(0, 0);
  this->scene_attr.resize(sy::AttackAttr_ARRAYSIZE, 0);
  if (!attr) return;
  for (int32_t i = 0; i < attr->size(); ++i) {
    this->scene_attr[i] = attr->Get(i);
  }
}

void PkObject::CalcHeroAttr(PkHero& hero, PK& pk) {
  //计算场景增加的属性
  RepeatedInt64* fields = hero.hero.mutable_attr1();
  const static std::pair<int32_t, int32_t> array[] = {
      std::make_pair(sy::ATTACK_ATTR_HP, sy::ATTACK_ATTR_HP_PERCENT),
      std::make_pair(sy::ATTACK_ATTR_AP, sy::ATTACK_ATTR_AP_PERCENT),
      std::make_pair(sy::ATTACK_ATTR_SP, sy::ATTACK_ATTR_SP_PERCENT),
      std::make_pair(sy::ATTACK_ATTR_WF, sy::ATTACK_ATTR_WF_PERCENT),
      std::make_pair(sy::ATTACK_ATTR_FF, sy::ATTACK_ATTR_FF_PERCENT),
  };
  for (int32_t i = 0; i < ArraySize(array); ++i) {
    fields->Set(array[i].second, 0);
  }
  for (size_t i = 0; i < this->scene_attr.size(); ++i) {
    int32_t value = this->scene_attr[i];
    fields->Set(i, value + fields->Get(i));
  }
  for (int32_t i = 0; i < ArraySize(array); ++i) {
    int32_t index = array[i].first;
    int32_t ratio = fields->Get(array[i].second);
    if (!ratio) continue;
    fields->Set(index, fields->Get(index) * (1000 + ratio) / 1000);
  }
  hero.state.current_hp = hero.state.current_hp * (fields->Get(array[0].second) + 1000) / 1000;
  //63000 世界boss
  //62550 围剿海盗
  //62551 围剿海盗
  if (pk.copy->id() == 62550 || pk.copy->id() == 62551 || pk.copy->id() == 63000) {
    int32_t mul = pk.copy->id() == 62551 ? 2500 : 1000;
    mul *= (hero.hero.grade() + 1);
    fields->Set(sy::ATTACK_ATTR_AP, fields->Get(sy::ATTACK_ATTR_AP) * mul / 1000);
    fields->Set(sy::ATTACK_ATTR_SP, fields->Get(sy::ATTACK_ATTR_SP) * mul / 1000);
  }
}

void FillHeroComboSkill(PkHero& hero, const std::vector<int32_t>& combo, int32_t super_combo) {
  if (combo.size() < 3u || !combo[0]) return;
  hero.skill3 = hero.hero.grade() >= 10 && super_combo
                    ? SKILL_BASE.GetEntryByID(super_combo)
                    : SKILL_BASE.GetEntryByID(combo[1]);
  hero.combo = combo;
  hero.combo.erase(hero.combo.begin());
  hero.combo.erase(hero.combo.begin());
}

//HP大于0, 表示当前血量
//HP等于, 表示使用默认血量
//HP小于0, 表示怪物死亡
void PkObject::AddHero(PK& pk, const sy::HeroInfo& info, int32_t pos,
                       bool minus, bool overwrite, int64_t hp) {
  if (hp < 0) return;
  this->carrier.ap_mean = 0;
  this->carrier.sp_mean = 0;
  bool found = false;
  if (overwrite) {
    for (std::vector<PkHero>::iterator iter = this->heros.begin();
         iter != this->heros.end(); ++iter) {
      if (iter->pos() == pos) {
        found = true;
        this->fight -= iter->attr().Get(0);
        INFO_LOG(logger)("EraseHero, HeroUID:%ld, HeroID:%d", iter->uid(), iter->id());
        std::iter_swap(iter, this->heros.end() - 1);
        this->heros.pop_back();
        break;
      }
    }
  }
  PkHero hero;
  hero.hero = info;
  int64_t uid = labs(hero.hero.uid());
  if (minus) uid = -uid;
  hero.hero.set_uid(uid);
  hero.position = pos;
  hero.state.current_hp = hp > 0 ? hp : info.attr1(sy::ATTACK_ATTR_HP);
  hero.state.current_anger = info.attr1(sy::ATTACK_ATTR_ANGER);
  this->CalcHeroAttr(hero, pk);

  //初始化国家, 技能
  const HeroBasePtr& hero_base = HERO_BASE.GetEntryByID(hero.id());
  if (hero_base) {
    hero.job = hero_base->job;
    hero.hero_country = hero_base->country;
    hero.skill1 = SKILL_BASE.GetEntryByID(hero_base->skill.v1);
    hero.skill2 = SKILL_BASE.GetEntryByID(hero_base->skill.v2);
    FillHeroComboSkill(hero, hero_base->combo, hero_base->super_combo);
    hero.hero.set_quality(hero_base->quality);

    if ((hero_base->skill.v1 && !hero.skill1) or
        (hero_base->skill.v2 && !hero.skill2)) {
      ERROR_LOG(logger)("HeroID:%d, SKillID:%d, %d, at least one not found"
          , hero.id(), hero_base->skill.v1, hero_base->skill.v2);
    }
  }
  const MonsterBasePtr& monster_base = MONSTER_BASE.GetEntryByID(hero.id());
  if (monster_base) {
    hero.job = monster_base->job;
    hero.hero_country = monster_base->country;
    hero.skill1 = SKILL_BASE.GetEntryByID(monster_base->skill.v1);
    hero.skill2 = SKILL_BASE.GetEntryByID(monster_base->skill.v2);
    hero.state.boss_buff = monster_base->start_buff;
    FillHeroComboSkill(hero, monster_base->combo, 0);

    if ((monster_base->skill.v1 && !hero.skill1) or
        (monster_base->skill.v2 && !hero.skill2)) {
      ERROR_LOG(logger)("HeroID:%d, SKillID:%d, %d, at least one not found"
          , hero.id(), monster_base->skill.v1, monster_base->skill.v2);
    }
  }
  if (hero.hero.attr1_size() < sy::AttackAttr_ARRAYSIZE) {
    hero.hero.mutable_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  }
  this->heros.push_back(hero);
  if (overwrite) this->helpers.push_back(hero.uid());

  if (!found && hero.uid() > 0) {
    this->carrier.hero_add_ap += hero.ap();
    this->carrier.hero_add_sp += hero.sp();
  }
  this->fight += hero.attr().Get(0);
}

void PkObject::AutoAddBuff(PK& pk, ReportLine& line) {
  for (std::vector<PkHero>::iterator iter = this->heros.begin();
       iter != this->heros.end(); ++iter) {
    PkHero& hero = *iter;
    if (hero.state.boss_buff) hero.state.state |= PkState_Boss;
  }
}

int32_t PkObject::AliveHero() const {
  int32_t count = 0;
  for (std::vector<PkHero>::const_iterator iter = this->heros.begin();
       iter != this->heros.end(); ++iter) {
      count += iter->hp() ? 1 : 0;
  }
  return count;
}

void PkObject::Clear() {
  this->fight = 0;
  this->carrier.ap_mean = 0;
  this->carrier.sp_mean = 0;
  this->carrier.hero_add_ap = 0;
  this->carrier.hero_add_sp = 0;
  this->carrier.carrier.mutable_attr1()->Resize(0, 0);
  this->carrier.carrier.mutable_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  this->carrier.carrier.mutable_tower_attr1()->Resize(0, 0);
  this->carrier.carrier.mutable_tower_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  this->carrier.carrier.set_carrier_id(0);
  this->carrier.buffs2.clear();
  this->helpers.clear();

  this->heros.clear();
}

void PkObject::FillGroupInfo(ReportLine& line) {
  line.Add(this->carrier.uid());
  for (int32_t i = 1; i <= 6; ++i) {
    PkHero* hero = this->GetHeroByPos(i);
    line.Add(hero ? hero->uid() : 0);
  }
}

void PkObject::FillHelpersInfo(ReportLine& line) {
  for (uint32_t i = 0; i < this->helpers.size(); ++i) {
    line.Add(this->helpers[i]);
  }
}

static __thread sy::MessageResponseFightReport* thread_report = NULL;

static inline sy::MessageResponseFightReport& GetReport() {
  if (!thread_report) thread_report = new sy::MessageResponseFightReport();
  thread_report->Clear();
  return *thread_report;
}

//战斗过程
PK::PK(const CopyBase* copy)
    : copy(copy),
      hero_uid(0),
      seed(RandomIn10000()),
      rand(seed),
      response(GetReport()),
      report(*response.mutable_info()),
      wave(0),
      round(0),
      use_helpers(true),
      line() {
  this->report.set_report_uid(server->GetTID());
  this->report.set_map_id(copy->id());
  this->report.set_random(seed);
  this->report.add_players();
  this->report.add_players();
  this->report.mutable_players(1)->set_player_id(0);
  this->report.mutable_players(1)->set_name("");
  this->report.mutable_report_info()->Reserve(64);
}

void PK::InitPlayer(LogicPlayer* player, bool attacker,
                    const int32_t* hp_percent, float atk_percent) {
  PkObject& o = attacker ? this->a : this->b;
  o.Clear();
  char c = attacker ? 'A' : 'B';
  INFO_LOG(logger)("Init %c Player:%ld", c, player->uid());

  if (!o.fill_player) {
    player->MakePlayerSimpleInfo(&o.player);
    this->report.mutable_players(!attacker)->CopyFrom(o.player);
    o.fill_player = true;
  }

  sy::CurrentCarrierInfo carrier = player->carrier_info();
  carrier.set_carrier_id(attacker ? carrier.carrier_id()
                                  : -carrier.carrier_id());
  o.SetCarrier(carrier);
  this->AddReportCarrierInfo(o.carrier);

  for (int32_t pos = 1; pos < 7; ++pos) {
    LogicHero* hero = player->GetHeroByPos(pos);
    if (!hero) continue;
    int64_t current_hp = hp_percent ? hp_percent[pos - 1] : 10000;
    current_hp = current_hp * hero->first.attr1(sy::ATTACK_ATTR_HP) / 10000;
    if (current_hp > 0) {
      this->hero_uid = std::max(this->hero_uid, hero->first.uid());
      o.AddHero(*this, hero->first, pos, attacker ? false : true, false, current_hp);
      o.heros.back().state.ap *= atk_percent;
      o.heros.back().state.sp *= atk_percent;
      this->AddReportHeroInfo(o.heros.back());
    }
  }
}

void PK::InitPlayer(const sy::OtherPlayerInfo* other_player, bool attacker,
                    float uk_atk, float us_atk, float ge_atk, float jp_atk) {
  PkObject& o = attacker ? this->a : this->b;
  o.Clear();
  char c = attacker ? 'A' : 'B';
  INFO_LOG(logger)("Init %c Player:%ld", c, other_player->player_id());

  if (!o.fill_player) {
    o.player.set_avatar(other_player->avatar());
    o.player.set_name(other_player->name());
    o.player.set_player_id(other_player->player_id());
    o.player.set_vip_level(other_player->vip_level());
    o.player.set_level(other_player->level());
    o.player.set_rank_id(other_player->rank_id());
    this->report.mutable_players(!attacker)->CopyFrom(o.player);
    o.fill_player = true;
  }

  sy::CurrentCarrierInfo carrier = other_player->carrier();
  carrier.set_carrier_id(attacker ? carrier.carrier_id()
                                  : -carrier.carrier_id());
  o.SetCarrier(carrier);
  this->AddReportCarrierInfo(o.carrier);

  for (int i = 0; i < other_player->battle_pos1_size(); ++i) {
    const sy::PositionInfo& pos = other_player->battle_pos1(i);

    for (int j = 0; j < other_player->heros_size(); ++j) {
      if (other_player->heros(j).uid() == pos.hero_uid()) {
        const sy::HeroInfo& hero = other_player->heros(j);
        int64_t current_hp = hero.attr1(sy::ATTACK_ATTR_HP);
        if (current_hp > 0) {
          this->hero_uid = std::max(this->hero_uid, hero.uid());
          o.AddHero(*this, hero, pos.position(), attacker ? false : true, false,
                    current_hp);
          HeroBase* base = HERO_BASE.GetEntryByID(hero.hero_id()).get();
          if (base) {
            if (1 == base->country) {
              o.heros.back().state.ap *= uk_atk;
              o.heros.back().state.sp *= uk_atk;
            }
            if (2 == base->country) {
              o.heros.back().state.ap *= us_atk;
              o.heros.back().state.sp *= us_atk;
            }
            if (3 == base->country) {
              o.heros.back().state.ap *= ge_atk;
              o.heros.back().state.sp *= ge_atk;
            }
            if (4 == base->country) {
              o.heros.back().state.ap *= jp_atk;
              o.heros.back().state.sp *= jp_atk;
            }
          }
          this->AddReportHeroInfo(o.heros.back());
        }
        break;
      }
    }
  }
}

void PK::InitMonsterGroup(int64_t player_id, const MonsterGroupBase* group,
                          bool attacker) {
  PkObject& o = attacker ? this->a : this->b;
  o.player.set_player_id(player_id);
  this->report.mutable_players(!attacker)->CopyFrom(o.player);
  this->InitMonsterGroup(group, attacker, false);
}

void PK::InitMonsterGroup(const MonsterGroupBase* group, bool attacker,
                          bool overwrite) {
  PkObject& o = attacker ? this->a : this->b;
  if (!overwrite) o.Clear();
  char c = attacker ? 'A' : 'B';
  INFO_LOG(logger)("Init %c Monster:%ld", c, group->id());
  if (group->monster_group[0]) {
    sy::CurrentCarrierInfo carrier = group->carrier_info();
    carrier.set_carrier_id(attacker ? carrier.carrier_id()
                                    : -carrier.carrier_id());
    o.SetCarrier(carrier);
    this->AddReportCarrierInfo(o.carrier);
  }
  std::vector<sy::HeroInfo>& heros = group->hero_info();
  for (size_t i = 0; i < heros.size(); ++i) {
    int32_t pos = i + 1;
    ++this->hero_uid;
    if (heros[i].hero_id()) {
      heros[i].set_uid(this->hero_uid);
      o.AddHero(*this, heros[i], pos, !attacker, overwrite, 0);
      this->AddReportHeroInfo(o.heros.back());
    }
  }
}

void PK::InitMonsterInfo(sy::CurrentCarrierInfo& carrier,
                         std::vector<sy::HeroInfo>& monster,
                         const int64_t* hp) {
  PkObject& o = this->b;
  o.Clear();
  INFO_LOG(logger)("Init BossInfo, HP:%ld,%ld,%ld,%ld,%ld,%ld",
      hp[0], hp[1], hp[2], hp[3], hp[4], hp[5]
      );
  if (carrier.carrier_id()) {
    carrier.set_carrier_id(-abs(carrier.carrier_id()));
    o.SetCarrier(carrier);
    this->AddReportCarrierInfo(o.carrier);
  }
  std::vector<sy::HeroInfo>& heros = monster;
  for (size_t i = 0; i < heros.size(); ++i) {
    int32_t pos = i + 1;
    ++this->hero_uid;
    if (heros[i].hero_id()) {
      heros[i].set_uid(this->hero_uid);
      int64_t current_hp = hp && hp[pos - 1] > 0 ? hp[pos - 1] : -1;
      if (current_hp >= 0) {
        o.AddHero(*this, heros[i], pos, true, false, current_hp);
        this->AddReportHeroInfo(o.heros.back());
      }
    }
  }
}

void PK::TryGetTarget(bool a_or_b, std::vector<PkHero*>& target,
                  int32_t pos) {
  if (!pos) return;
  PkObject& o = a_or_b ? this->a : this->b;
  for (std::vector<PkHero>::iterator iter = o.heros.begin();
       iter != o.heros.end(); ++iter) {
    if (pos == iter->pos() && iter->hp() > 0) {
      target.push_back(&*iter);
    }
  }
}

inline void UniqueAdd(std::vector<PkHero*>& target, PkHero* hero) {
  if (std::find(target.begin(), target.end(), hero) == target.end()) {
    target.push_back(hero);
  }
}

void PK::UniqueAddSpecialTarget(bool a_or_b, std::vector<PkHero*>& target,
                                int32_t job) {
  PkObject& o = a_or_b ? this->a : this->b;
  for (std::vector<PkHero>::iterator iter = o.heros.begin();
       iter != o.heros.end(); ++iter) {
    if (iter->hp() > 0 && iter->job == job) {
      UniqueAdd(target, &*iter);
    }
  }
}

struct CompareAngerGreater {
  bool operator()(const PkHero* a, const PkHero* b) const {
    return a->anger() > b->anger();
  }
};

struct CompareHpLess {
  bool operator()(const PkHero* a, const PkHero* b) const {
    return a->hp() < b->hp();
  }
};

struct CompareHPGreater {
  bool operator()(const PkHero* a, const PkHero* b) const {
    return a->hp() > b->hp();
  }
};

struct CompareHpPerLess {
  bool operator()(const PkHero* a, const PkHero* b) const {
    return a->hp() * 100 / a->max_hp() < b->hp() * 100 / b->max_hp();
  }
};

struct CompareAPGreater {
  bool operator()(const PkHero* a, const PkHero* b) const {
    return std::max(a->ap(), a->sp()) > std::max(b->ap(), b->sp());
  }
};

void RandomInPool(std::vector<PkHero*>& in, std::vector<PkHero*>& out,
                  int32_t count, RandomNubmer& rand) {
  for (int32_t i = 0; i < count; ++i) {
    if (in.empty()) return;
    int32_t index = rand() % in.size();
    out.push_back(in[index]);
    in.erase(in.begin() + index);
  }
}

//|---+---+------+---+---|
//| 4 | 1 |      | 1 | 4 |
//|---+---|      |---+---|
//| 5 | 2 |  VS  | 2 | 5 |
//|---+---|      |---+---|
//| 6 | 3 |      | 3 | 6 |
//|---+---+------+---+---|
void PK::GetTarget(IPkObject* attack, bool is_friend,
                   std::vector<PkHero*>& target, int32_t type) {
  static std::pair<int32_t, int32_t> kColumnMap[] = {
      std::pair<int32_t, int32_t>(0, 0), std::pair<int32_t, int32_t>(1, 4),
      std::pair<int32_t, int32_t>(2, 5), std::pair<int32_t, int32_t>(3, 6),
      std::pair<int32_t, int32_t>(1, 4), std::pair<int32_t, int32_t>(2, 5),
      std::pair<int32_t, int32_t>(3, 6),
  };
  std::vector<PkHero*> pool;
  target.clear();
  bool a_or_b = attack->uid() > 0 ? true : false;
  a_or_b = is_friend ? a_or_b : !a_or_b;
  switch (type) {
    // 1   自身
    case 1: {
      target.push_back(static_cast<PkHero*>(attack));
    } break;
    // 2   前排（前排没有选后排）
    case 2: {
      TryGetTarget(a_or_b, target, 1);
      TryGetTarget(a_or_b, target, 2);
      TryGetTarget(a_or_b, target, 3);
      if (target.empty()) {
        TryGetTarget(a_or_b, target, 4);
        TryGetTarget(a_or_b, target, 5);
        TryGetTarget(a_or_b, target, 6);
      }
    } break;
    // 3   后排（后排没有选前排）
    case 3: {
      TryGetTarget(a_or_b, target, 4);
      TryGetTarget(a_or_b, target, 5);
      TryGetTarget(a_or_b, target, 6);
      if (target.empty()) {
        TryGetTarget(a_or_b, target, 1);
        TryGetTarget(a_or_b, target, 2);
        TryGetTarget(a_or_b, target, 3);
      }
    } break;
    // 4   一列（正对位置上的一列）
    case 4: {
      this->GetTarget(attack, is_friend, target, 6);
      if (!target.empty()) {
        int32_t p = target[0]->pos();
        if (p <= 3) {
          TryGetTarget(a_or_b, target, kColumnMap[(p - 1) % 3 + 1].second);
        }
      }
    } break;
    // 5   全体
    case 5: {
      TryGetTarget(a_or_b, target, 1);
      TryGetTarget(a_or_b, target, 2);
      TryGetTarget(a_or_b, target, 3);
      TryGetTarget(a_or_b, target, 4);
      TryGetTarget(a_or_b, target, 5);
      TryGetTarget(a_or_b, target, 6);
    } break;
    // 6   前排单个敌人
    case 6: {
      static int32_t kSingle[][3] = {
          {0}, {1, 2, 3}, {2, 1, 3}, {3, 1, 2}, {4, 5, 6}, {5, 4, 6}, {6, 4, 5},
      };
      //(x - 1) % 3 + 1 => {1,2,3}
      TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 1][0]);
      TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 1][1]);
      TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 1][2]);
      if (target.empty()) {
        //(x - 1) % 3 + 1 => {4,5,6}
        TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 4][0]);
        TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 4][1]);
        TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 4][2]);
      }
      if (!target.empty()) {
        target.resize(1);
      }
    } break;
    // 7   后排单个敌人
    case 7: {
      static int32_t kSingle[][3] = {
          {0}, {4, 5, 6}, {5, 4, 6}, {6, 4, 5}, {1, 2, 3}, {2, 1, 3}, {3, 1, 2},
      };
      //(x - 1) % 3 + 1 => {4,5,6}
      TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 1][0]);
      TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 1][1]);
      TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 1][2]);
      if (target.empty()) {
        //(x - 1) % 3 + 1 => {1,2,3}
        TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 4][0]);
        TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 4][1]);
        TryGetTarget(a_or_b, target, kSingle[(attack->pos() - 1) % 3 + 4][2]);
      }
      if (!target.empty()) {
        target.resize(1);
      }
    } break;
    //8   随机3目标
    //9   随机2目标
    //10  随机1目标
    case 8:
    case 9:
    case 10: {
      TryGetTarget(a_or_b, pool, 1);
      TryGetTarget(a_or_b, pool, 2);
      TryGetTarget(a_or_b, pool, 3);
      TryGetTarget(a_or_b, pool, 4);
      TryGetTarget(a_or_b, pool, 5);
      TryGetTarget(a_or_b, pool, 6);
      RandomInPool(pool, target, 11 - type, this->rand);
    } break;
    //11  当前生命百分比最低
    case 11: {
      TryGetTarget(a_or_b, target, 1);
      TryGetTarget(a_or_b, target, 2);
      TryGetTarget(a_or_b, target, 3);
      TryGetTarget(a_or_b, target, 4);
      TryGetTarget(a_or_b, target, 5);
      TryGetTarget(a_or_b, target, 6);
      std::sort(target.begin(), target.end(), CompareHpPerLess());
      if (!target.empty()) {
        target.resize(1);
      }
    } break;
    //12  当前生命值最低
    case 12: {
      TryGetTarget(a_or_b, target, 1);
      TryGetTarget(a_or_b, target, 2);
      TryGetTarget(a_or_b, target, 3);
      TryGetTarget(a_or_b, target, 4);
      TryGetTarget(a_or_b, target, 5);
      TryGetTarget(a_or_b, target, 6);
      std::sort(target.begin(), target.end(), CompareHpLess());
      if (!target.empty()) {
        target.resize(1);
      }
    } break;
    //13  当前攻击最高（2种攻击中最高）
    case 13: {
      TryGetTarget(a_or_b, target, 1);
      TryGetTarget(a_or_b, target, 2);
      TryGetTarget(a_or_b, target, 3);
      TryGetTarget(a_or_b, target, 4);
      TryGetTarget(a_or_b, target, 5);
      TryGetTarget(a_or_b, target, 6);
      std::sort(target.begin(), target.end(), CompareAPGreater());
      if (!target.empty()) {
        target.resize(1);
      }
    } break;
    //14  单个敌人及其相邻单位
    case 14: {
      static int32_t KNeighborMap[][3] = {
        {0},
        {2, 4},
        {1, 3, 5},
        {2, 6},
        {5},
        {4, 6},
        {5}
      };
      this->GetTarget(attack, is_friend, target, 6);
      if (!target.empty()) {
        int32_t p = target[0]->pos();
        TryGetTarget(a_or_b, target, KNeighborMap[p][0]);
        TryGetTarget(a_or_b, target, KNeighborMap[p][1]);
        TryGetTarget(a_or_b, target, KNeighborMap[p][2]);
      }
    } break;
    //15  单个敌人及生命值最少的单体（目标可以重复）
    case 15: {
      this->GetTarget(attack, is_friend, target, 6);

      TryGetTarget(a_or_b, pool, 1);
      TryGetTarget(a_or_b, pool, 2);
      TryGetTarget(a_or_b, pool, 3);
      TryGetTarget(a_or_b, pool, 4);
      TryGetTarget(a_or_b, pool, 5);
      TryGetTarget(a_or_b, pool, 6);
      std::sort(pool.begin(), pool.end(), CompareHpLess());
      if (!pool.empty()) {
        target.push_back(pool.front());
      }
    } break;
    //16  单个敌人及所有战列舰+巡洋舰（目标不重复）
    case 16: {
      this->GetTarget(attack, is_friend, target, 6);

      UniqueAddSpecialTarget(a_or_b, target, sy::JOB_BATTLESHIP);
      UniqueAddSpecialTarget(a_or_b, target, sy::JOB_CRUISER);
    } break;
    //17  单个敌人及所有驱逐舰+潜艇（目标不重复）
    case 17: {
      this->GetTarget(attack, is_friend, target, 6);

      UniqueAddSpecialTarget(a_or_b, target, sy::JOB_DESTROYER);
      UniqueAddSpecialTarget(a_or_b, target, sy::JOB_SUBMARINE);
    } break;
    //18  怒气最高的3个敌人
    //19  怒气最高的2个敌人
    //20  怒气最高的1个敌人
    case 18:
    case 19:
    case 20: {
      TryGetTarget(a_or_b, target, 1);
      TryGetTarget(a_or_b, target, 2);
      TryGetTarget(a_or_b, target, 3);
      TryGetTarget(a_or_b, target, 4);
      TryGetTarget(a_or_b, target, 5);
      TryGetTarget(a_or_b, target, 6);
      std::sort(target.begin(), target.end(), CompareAngerGreater());
      if (int32_t(target.size()) > 21 - type) {
        target.resize(21 - type);
      }
    } break;
    //混乱(不分敌我任选一个)
    case 21: {
      TryGetTarget(a_or_b, pool, 1);
      TryGetTarget(a_or_b, pool, 2);
      TryGetTarget(a_or_b, pool, 3);
      TryGetTarget(a_or_b, pool, 4);
      TryGetTarget(a_or_b, pool, 5);
      TryGetTarget(a_or_b, pool, 6);
      TryGetTarget(!a_or_b, pool, 1);
      TryGetTarget(!a_or_b, pool, 2);
      TryGetTarget(!a_or_b, pool, 3);
      TryGetTarget(!a_or_b, pool, 4);
      TryGetTarget(!a_or_b, pool, 5);
      TryGetTarget(!a_or_b, pool, 6);
      std::vector<PkHero*>::iterator erase_iter = std::remove(
          pool.begin(), pool.end(), static_cast<PkHero*>(attack));
      if (erase_iter != pool.end()) pool.erase(erase_iter);
      RandomInPool(pool, target, 1, this->rand);
    } break;
    //血量最高的3个人
    //血量最高的2个人
    //血量最高的1个人
    case 22:
    case 23:
    case 24: {
      TryGetTarget(a_or_b, target, 1);
      TryGetTarget(a_or_b, target, 2);
      TryGetTarget(a_or_b, target, 3);
      TryGetTarget(a_or_b, target, 4);
      TryGetTarget(a_or_b, target, 5);
      TryGetTarget(a_or_b, target, 6);
      std::sort(target.begin(), target.end(), CompareHPGreater());
      if (int32_t(target.size()) > 25 - type) {
        target.resize(25 - type);
      }
    } break;
  }
}

void PK::AddReportContent(ReportLine& r) {
  const std::vector<int64_t>& line = r.Line();
  sy::FightReportInfo* info = this->report.add_report_info();
  info->mutable_content()->Reserve(line.size());
  for (size_t i = 0; i < line.size(); ++i) {
    info->add_content(line[i]);
  }
}

PkHero* PK::GetHeroByUID(int64_t uid) {
  PkObject& o = uid > 0 ? this->a : this->b;
  for (std::vector<PkHero>::iterator iter = o.heros.begin();
       iter != o.heros.end(); ++iter) {
    if (iter->uid() == uid) {
      return &*iter;
    }
  }
  return NULL;
}

//生成PVE战报
void PK::GeneratePVEReport(LogicPlayer* player) {
  int32_t result = 0;
  int32_t group = 0;
  //PVE
  for (group = 1; group <= (int32_t)copy->monster.size(); ++group) {
    this->InitPlayer(player, true);
    const MonsterGroupBase* monster_group =
        MONSTER_GROUP_BASE.GetEntryByID(copy->monster[group - 1]).get();
    if (!monster_group) break;
    this->InitMonsterGroup(monster_group, false, false);
    if (this->use_helpers) {
      //上援军
      const MonsterGroupBase* monster_help =
          MONSTER_GROUP_BASE.GetEntryByID(copy->reinforcement[group - 1]).get();
      if (monster_help) this->InitMonsterGroup(monster_help, true, true);
    }
    result = this->GenerateGroupReport(group);
    //打断
    if (!result) break;
  }
}

void PK::GeneratePVPReport() {
  int32_t result = 0;
  int32_t group = 1;
  result = this->GenerateGroupReport(group);
  (void)result;
}

int32_t PK::GenerateGroupReport(int32_t group) {
  if (group >= this->wave) {
    this->wave = group;
    this->report.set_group_count(group);
  }
  attack_order.clear();
  //波次开始
  this->AddReportContent(line.Clear().Add(kReportTag_Group, group));
  //使用战斗力出手顺序
  bool b_first = false;
  if (this->copy->type == sy::COPY_TYPE_PK ||
      this->copy->type == sy::COPY_TYPE_COMPARE) {
    if (this->a.fight < this->b.fight) b_first = true;
  }

  for (int32_t i = 1; i <= 6; ++i) {
    PkHero* hero_a = this->a.GetHeroByPos(i);
    PkHero* hero_b = this->b.GetHeroByPos(i);
    if (b_first) {
      if (hero_b) attack_order.push_back(hero_b);
      if (hero_a) attack_order.push_back(hero_a);
    } else {
      if (hero_a) attack_order.push_back(hero_a);
      if (hero_b) attack_order.push_back(hero_b);
    }
  }

  //航母出手(按照船只出手顺序)
  if (b_first) {
    if (this->b.carrier.uid()) attack_order.push_back(&this->b.carrier);
    if (this->a.carrier.uid()) attack_order.push_back(&this->a.carrier);
  } else {
    if (this->a.carrier.uid()) attack_order.push_back(&this->a.carrier);
    if (this->b.carrier.uid()) attack_order.push_back(&this->b.carrier);
  }

  //站位信息
  line.Clear().Add(kReportTag_GroupInfo);
  this->a.FillGroupInfo(line);
  this->b.FillGroupInfo(line);
  this->a.FillHelpersInfo(line);
  this->AddReportContent(line);

  //给攻守双方上Buff
  this->AutoAddBuff();

  const int32_t kMaxRound = 20;
  for (this->round = 1; round <= kMaxRound; ++round) {
    //轮回
    this->AddReportContent(line.Clear().Add(kReportTag_Round, round));
    for (std::vector<IPkObject*>::iterator iter = attack_order.begin();
         iter != attack_order.end(); ++iter) {
      IPkObject* attacker = *iter;
      if (attacker->CheckPass()) continue;
      this->TryEraseLastAttackReport();
      this->AddReportContent(
          line.Clear().Add(kReportTag_Attack, attacker->uid()));
      if (attacker->State())
        this->AddReportContent(line.Clear().Add(
            kReportTag_HeroState, attacker->uid(), attacker->State()));

      attacker->BuffTick(*this, line);
      attacker->Attack(*this, line);
      attacker->AutoUnloadBuff(*this, line);
      //每次出手判断是否可以结束战斗
      if (this->CheckAllDead()) break;
    }
    if (this->CheckAllDead()) break;
  }

  star = this->CheckSuccess();
  star = this->CheckStar();
  return star;
}

void PK::TryEraseLastAttackReport() {
  if (this->report.report_info_size() &&
      (this->report.report_info().end() - 1)->content(0) == kReportTag_Attack) {
    this->report.mutable_report_info()->RemoveLast();
  }
}

int32_t PK::CheckAllDead() {
  PkObject* array[] = {&this->a, &this->b};
  for (int32_t i = 0; i < ArraySize(array); ++i) {
    PkObject& o = *array[i];
    if (o.AliveHero() <= 0) return i + 1;
  }
  return 0;
}

//攻守双方上战前Buff
void PK::AutoAddBuff() {
  this->a.AutoAddBuff(*this, this->line);
  this->b.AutoAddBuff(*this, this->line);
}

void PK::AddReportCarrierInfo(PkCarrier& c) {
  for (int32_t index = 0; index < this->report.carrier_info_size(); ++index) {
    if (this->report.carrier_info(index).carrier().carrier_id() ==
        c.id()) {
      return;
    }
  }

  sy::FightCarrierInfo* carrier = this->report.add_carrier_info();
  carrier->mutable_carrier()->CopyFrom(c.carrier);
  carrier->set_carrier_id(c.id());
  carrier->set_anger(c.anger());
}

void PK::AddReportHeroInfo(PkHero& hero) {
  for (int32_t i = 0; i < this->report.ship_info_size(); ++i) {
    if (this->report.ship_info(i).hero_info().uid() == hero.uid()) {
      return;
    }
  }

  sy::FightShipInfo* ship = this->report.add_ship_info();
  ship->mutable_hero_info()->CopyFrom(hero.hero);
  ship->set_position(hero.pos());
  ship->set_current_hp(hero.hp());
  ship->set_anger(hero.anger());

  ship->mutable_hero_info()->clear_exp();
  ship->mutable_hero_info()->clear_rand_attr();
  ship->mutable_hero_info()->clear_rand_attr_1();
  ship->mutable_hero_info()->clear_fate_exp();
  ship->mutable_hero_info()->clear_fate_cost();
  ship->mutable_hero_info()->clear_fate_seed();
}

//胜利判断
//回合数在20回合以内
int32_t PK::CheckSuccess() {
  //敌方全死光
  for (std::vector<PkHero>::iterator iter = this->b.heros.begin();
       iter != this->b.heros.end(); ++iter) {
    if (iter->hp() > 0) return 0;
  }
  switch (copy->condition.v1) {
    //不超过X回合数
    case 1: {
      if (this->round > copy->condition.v2) return 0;
    } break;
    //剩余血量不少于X万分比
    case 2: {
      int64_t current_hp = 0;
      int64_t max_hp = 0;
      for (std::vector<PkHero>::iterator iter = this->a.heros.begin();
           iter != this->a.heros.end(); ++iter) {
        current_hp += std::max(iter->hp(), 0l);
        max_hp += iter->max_hp();
      }
      int32_t percent = current_hp * 10000 / max_hp / 100;
      this->report.set_hp_percent(percent);
      if (current_hp * 10000 / max_hp < copy->condition.v2) return 0;
    } break;
    //我方死亡人数不超过X人
    case 3: {
      int32_t count = 0;
      for (std::vector<PkHero>::iterator iter = this->a.heros.begin();
           iter != this->a.heros.end(); ++iter) {
        if (iter->hp() <= 0) ++count;
      }
      if (count > copy->condition.v2) return 0;
    } break;
    //不处理
    case 4: {
    } break;
  }
  return 3;
}

//星级判断
int32_t PK::CheckStar() {
  if (star) {
    if (copy->star) {
      int32_t count = 0;
      for (std::vector<PkHero>::iterator iter = this->a.heros.begin();
           iter != this->a.heros.end(); ++iter) {
        if (iter->hp() <= 0) ++count;
      }
      this->star = std::max(3 - count, 1);
    }
    if (copy->type == sy::COPY_TYPE_TOWER) {
      this->star = copy->id() % 10;
    }
    if (copy->type == sy::COPY_TYPE_HARD) {
      if (this->round <= 4) this->star = 3;
      else if (this->round <= 6) this->star = 2;
      else this->star = 1;
    }
  }
  this->report.set_win(star);
  this->AddReportContent(line.Clear().Add(kReportTag_Star, star));

  return this->star;
}

void PK::InitSceneAttr(const RepeatedInt64* attr) {
  if (!attr) return;
  this->a.InitSceneAttr(attr);
  line.Clear();
  line.Add(kReportTag_SceneAttr);
  for (int32_t i = 0; i < attr->size(); ++i) {
    if (attr->Get(i)) line.Add(i, attr->Get(i));
  }
  if (line.Line().size() <= 1) return;
  this->AddReportContent(line);
}

bool PK::CheckHeroAlive(bool attacker, int32_t hero_id) {
  PkObject& o = attacker? this->a : this->b;
  PkHero* hero = o.GetHeroByHeroID(hero_id);
  return hero && hero->hp();
}
