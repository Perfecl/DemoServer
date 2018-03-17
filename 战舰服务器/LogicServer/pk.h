#ifndef __PK_H__
#define __PK_H__
#include <vector>
#include <myrandom.h>
#include <vector_map.h>
#include "config.h"
#include "logic_player.h"
#include <algorithm>

struct ReportLine {
  ReportLine() {}

  ReportLine& Clear() {
    line.clear();
    return *this;
  }

  std::vector<int64_t>& Line() const {
    return this->line;
  }

  ReportLine& Add(int64_t p1) {
    line.push_back(p1);
    return *this;
  }

  ReportLine& Add(int64_t p1, int64_t p2) {
    line.push_back(p1);
    line.push_back(p2);
    return *this;
  }

  ReportLine& Add(int64_t p1, int64_t p2, int64_t p3) {
    line.push_back(p1);
    line.push_back(p2);
    line.push_back(p3);
    return *this;
  }

  ReportLine& Add(int64_t p1, int64_t p2, int64_t p3, int64_t p4) {
    line.push_back(p1);
    line.push_back(p2);
    line.push_back(p3);
    line.push_back(p4);
    return *this;
  }

  ReportLine& Add(int64_t p1, int64_t p2, int64_t p3, int64_t p4, int64_t p5) {
    line.push_back(p1);
    line.push_back(p2);
    line.push_back(p3);
    line.push_back(p4);
    line.push_back(p5);
    return *this;
  }

  ReportLine& Add(int64_t p1, int64_t p2, int64_t p3, int64_t p4, int64_t p5, int64_t p6) {
    line.push_back(p1);
    line.push_back(p2);
    line.push_back(p3);
    line.push_back(p4);
    line.push_back(p5);
    line.push_back(p6);
    return *this;
  }

 private:
  mutable std::vector<int64_t> line;
};

void LogReportToFile(const sy::ReportRecord& report);
std::string ToString(const sy::ReportRecord& report);

struct IPkObject;
struct PkHero;
struct PK;
struct BuffState;

typedef std::vector<BuffState> BuffType;
typedef std::vector<std::pair<int64_t, int32_t> > BuffIndexType;
typedef google::protobuf::RepeatedField<google::protobuf::int64> RepeatedInt64;

struct BuffState {
  BuffState();

  int32_t uid;
  IPkObject* attacker;    //释放者
  PkHero* target;         //遍历用字段
  bool a_side;            //遍历用字段

  boost::shared_ptr<int32_t> r;   //A回合数
  boost::shared_ptr<int32_t> r1;  //B回合数
  int32_t& roundA() { return *r; }
  int32_t& roundB() { return *r1; }
  SkillBasePtr skill;     //技能配置
  BuffBasePtr base;       //Buff配置
};

enum BuffAction {
  kBuff_Load,    //加Buff
  kBuff_Tick_A,  //Buff跳动(在被施法者这边)
  kBuff_Tick_B,  //Buff跳动(在施法者这边)
  kBuff_UnLoad,  //卸载Buff
};

struct BuffParam {
  BuffParam(BuffState& buff_state, IPkObject* target, PK& pk, ReportLine& line)
      : buff_state(buff_state), target(target), pk(pk), line(line) {}
  BuffState& buff_state;
  IPkObject* target;
  PK& pk;
  ReportLine& line;
  int32_t action;
};

enum PkHeroState {
  PkState_God     = 1 << 0,   //无敌
  PkState_Stun    = 1 << 1,   //眩晕
  PkState_Immuno  = 1 << 2,   //免疫
  PkState_Confuse = 1 << 3,   //混乱
  PkState_Slicent = 1 << 4,   //沉默
  PkState_Boss    = 1 << 5,   //BOSS免疫
};

struct HeroState {
  HeroState() { bzero(this, sizeof(*this)); }
  int32_t state;
  void SetState(uint32_t mask, bool is_set) {
    state = (state & ~mask) | (is_set ? mask : 0);
  }
  bool IsSetState(uint32_t mask) const {
    return state & mask;
  }

  int32_t ap;         //火炮攻击
  int32_t ap_ratio;   //火炮攻击千分比
  int32_t sp;         //导弹攻击
  int32_t sp_ratio;   //导弹攻击千分比
  int32_t wf;         //火炮防御
  int32_t wf_ratio;   //火炮防御千分比
  int32_t ff;         //导弹防御
  int32_t ff_ratio;   //导弹防御千分比
  int32_t hit;        //命中率
  int32_t miss;       //闪避率
  int32_t crit;       //暴击率
  int32_t resist;     //爆抗率
  int32_t damage;     //伤害加成
  int32_t damage_dec; //伤害减免

  int32_t current_anger;  //当前的怒气
  int64_t current_hp;     //当前的血量
  int32_t current_shield; //当前护盾

  BuffType buffs; //套在头上的Buff
  BuffIndexType buffs2; //套在别人头上的buff(HeroUID, BuffUID)
  bool boss_buff;     //boss不能被击晕
};

struct IPkObject {
  enum {
    Country_Begin = 1,
    Country_End   = 4,
  };
  virtual ~IPkObject() {}
  virtual int64_t uid() const = 0;
  virtual int32_t pos() const = 0;
  virtual int32_t id() const = 0;

  //属性
  virtual int64_t hp() const = 0;
  virtual int64_t max_hp() const = 0;
  virtual int32_t anger() const = 0;
  virtual int32_t ap() const = 0;
  virtual int32_t sp() const = 0;
  virtual int32_t wf() const = 0;
  virtual int32_t ff() const = 0;
  virtual int32_t hit() const = 0;
  virtual int32_t miss() const = 0;
  virtual int32_t crit() const = 0;
  virtual int32_t resist() const = 0;
  virtual int32_t damage() const = 0;
  virtual int32_t damage_dec() const = 0;
  virtual int32_t country() const = 0;
  virtual int32_t star() { return 0; }
  virtual const RepeatedInt64& attr() const = 0;

  int32_t country_damage(int32_t country) const {
    if (!(country >= Country_Begin && country <= Country_End)) return 0;
    return this->attr().Get(sy::ATTACK_ATTR_UK_DAMAGE + (country - 1) * 2);
  }
  int32_t country_damage_dec(int32_t country) const {
    if (!(country >= Country_Begin && country <= Country_End)) return 0;
    return this->attr().Get(sy::ATTACK_ATTR_UK_DAMAGE_DEC + (country - 1) * 2);
  }
  virtual int32_t skill_damage_ratio(SkillBase* skill) const = 0;
  virtual int32_t skill_damage(SkillBase* skill) const = 0;

  inline int64_t attr(int32_t index) const {
    if (index >= 0 && index < attr().size()) return attr().Get(index);
    return 0;
  }

  //致命一击
  int64_t pk_hit() const { return attr(sy::ATTACK_ATTR_PK_HIT); }
  int64_t pk_hit_sub() const { return attr(sy::ATTACK_ATTR_PK_HIT_SUB); }
  //生命之光
  int64_t add_hp() const { return attr(sy::ATTACK_ATTR_PK_ADD_HP); }
  int64_t add_hp_sub() const { return attr(sy::ATTACK_ATTR_PK_ADD_HP_SUB); }
  //物理和法术伤害反弹
  int64_t ap_reback() const { return attr(sy::ATTACK_ATTR_PK_AP_DAMAGE_REBACK); }
  int64_t sp_reback() const { return attr(sy::ATTACK_ATTR_PK_SP_DAMAGE_REBACK); }
  //破势
  int64_t bullying() const { return attr(sy::ATTACK_ATTR_PK_BULLYING); }
  int64_t tenacity() const { return attr(sy::ATTACK_ATTR_PK_TENACITY); }
  int64_t ap_bullying_sub() const { return attr(sy::ATTACK_ATTR_PK_AP_BULLYING_SUB); }
  int64_t sp_bullying_sub() const { return attr(sy::ATTACK_ATTR_PK_SP_BULLYING_SUB); }
  //破防
  int64_t ignore_defence() const { return attr(sy::ATTACK_ATTR_PK_IGNORE_DEFENCE); }
  int64_t ignore_defence_inc() const { return attr(sy::ATTACK_ATTR_PK_IGNORE_DEFENCE_INC); }
  int64_t ignore_defence_sub() const { return attr(sy::ATTACK_ATTR_PK_IGNORE_DEFENCE_SUB); }
  //吸血
  int64_t vampiric() const { return attr(sy::ATTACK_ATTR_PK_VAMPIRIC); }

  //战斗有关的
  virtual bool CheckPass() { return false; }
  virtual int64_t AddHp(int64_t delta) { return 0; }
  virtual void AddAnger(int32_t anger, PK& pk, ReportLine& line) {};
  virtual void AddBuff(const BuffState& state) = 0;
  virtual void AddBuffIndex(int64_t hero_uid, int32_t buff_uid) = 0;

  virtual int32_t State() const { return 0; }
  virtual bool IsSetState(uint32_t mask) const = 0;
  virtual SkillBasePtr UseSkill(PK& pk) const = 0;
  virtual BuffType& Buffs() = 0;
  virtual BuffIndexType& Buffs2() = 0;


  //航母不需要这俩
  virtual void RemoveBuff(IPkObject* hero, int32_t buff_id, PK& pk,
                          ReportLine& line) {}
  virtual void AddBuff(IPkObject* attacker, SkillBase* skill, BuffBase* buff,
                       PK& pk, ReportLine& line) {}

  // Step 1
  void BuffTick(PK& pk, ReportLine& line);
  // Step 3
  void Attack(PK& pk, ReportLine& line);
  // Step 5
  void AutoUnloadBuff(PK& pk, ReportLine& line);

  template <typename Fn>
  void FindBuffs(std::vector<BuffState>& output, Fn equal);
};

struct PkHero : public IPkObject {
  PkHero() : job(0), hero_country(0), position(0), state() {}
  sy::HeroInfo hero;
  int32_t job;          //职业
  int32_t hero_country; //国家
  SkillBasePtr skill1;  //普通攻击
  SkillBasePtr skill2;  //技能攻击
  SkillBasePtr skill3;  //合体技能
  std::vector<int32_t> combo; //合体技能需要的船

  int32_t position;   //位置
  HeroState state;    //船只当前状态

  void AddStateReport(PK& pk, ReportLine& line);
  void AddDebugAttr(PK& pk);

  void RemoveBuff(IPkObject* hero, int32_t buff_id, PK& pk, ReportLine& line);
  void AddBuff(IPkObject* attacker, SkillBase* skill, BuffBase* buff, PK& pk, ReportLine& line);
  bool CheckPass();

  int64_t AddHp(int64_t delta);

  int64_t uid() const { return hero.uid(); }
  int32_t pos() const { return this->position; }
  int32_t id() const { return hero.hero_id(); }
  //攻击属性
  int64_t hp() const { return std::max(0L, state.current_hp); }
  int64_t max_hp() const { return hero.attr1(sy::ATTACK_ATTR_HP); }
  int32_t anger() const { return state.current_anger; }
  int32_t ap() const { return (int64_t)(hero.attr1(sy::ATTACK_ATTR_AP)) * (1000 + state.ap_ratio) / 1000 + state.ap; }
  int32_t sp() const { return (int64_t)(hero.attr1(sy::ATTACK_ATTR_SP)) * (1000 + state.sp_ratio) / 1000 + state.sp; }
  int32_t wf() const { return (int64_t)(hero.attr1(sy::ATTACK_ATTR_WF)) * (1000 + state.wf_ratio) / 1000 + state.wf; }
  int32_t ff() const { return (int64_t)(hero.attr1(sy::ATTACK_ATTR_FF)) * (1000 + state.ff_ratio) / 1000 + state.ff; }
  int32_t hit() const { return hero.attr1(sy::ATTACK_ATTR_HIT_PERCENT) + state.hit; }
  int32_t miss() const { return hero.attr1(sy::ATTACK_ATTR_MISS_PERCENT) + state.miss; }
  int32_t crit() const { return hero.attr1(sy::ATTACK_ATTR_CRIT_PERCENT) + state.crit; }
  int32_t resist() const { return hero.attr1(sy::ATTACK_ATTR_RESIST_PERCENT) + state.resist; }
  int32_t damage() const { return hero.attr1(sy::ATTACK_ATTR_DAMAGE) + state.damage; }
  int32_t damage_dec() const { return hero.attr1(sy::ATTACK_ATTR_DAMAGE_DECREASE) + state.damage_dec; }
  const RepeatedInt64& attr() const { return hero.attr1(); }
  int32_t country() const { return hero_country; }

  void AddAnger(int32_t anger, PK& pk, ReportLine& line);
  void AddBuff(const BuffState& state);
  void AddBuffIndex(int64_t hero_uid, int32_t buff_uid);

  int32_t skill_damage_ratio(SkillBase* skill) const {
    return skill->GetSkillDamage(this->hero.fate_level()).first;
  }

  int32_t skill_damage(SkillBase* skill) const {
    return skill->GetSkillDamage(this->hero.fate_level()).second;
  }

  int32_t State() const { return this->state.state; }
  bool IsSetState(uint32_t mask) const { return this->state.IsSetState(mask); }

  SkillBasePtr UseSkill(PK& pk) const;

  std::vector<BuffState>& Buffs();
  BuffIndexType& Buffs2() {
    return this->state.buffs2;
  }

  bool CanAttack() const {
    if (!this->skill1) return false;
    if (this->hp() <= 0) return false;
    //眩晕状态不能普通攻击
    if (this->state.IsSetState(PkState_Stun)) return false;
    return true;
  }

  bool CanUseSkill() const {
    if (!this->skill2) return false;
    if (this->hp() <= 0) return false;
    //眩晕, 混乱, 沉默状态不能使用技能
    if (this->state.IsSetState(PkState_Stun | PkState_Confuse | PkState_Slicent))
      return false;
    if (skill2 && this->anger() < skill2->fury) return false;
    return true;
  }

  bool CanUseCombo(PK& pk) const;
};

struct PkCarrier : public IPkObject {
  PkCarrier() : hero_add_ap(0), hero_add_sp(0), current_anger(0), ap_mean(0), sp_mean(0) {}
  sy::CurrentCarrierInfo carrier;

  SkillBasePtr skill1;
  SkillBasePtr skill2;

  BuffIndexType buffs2;

  int8_t carrier_country;
  int32_t hero_add_ap;
  int32_t hero_add_sp;
  int32_t current_anger;
  mutable int32_t ap_mean;     //ap平均值
  mutable int32_t sp_mean;     //sp平均值
  int32_t AddAp() const { if (!ap_mean) ap_mean = hero_add_ap / 6 * GetSettingValue(carrier_x) / 100; return ap_mean; }
  int32_t AddSp() const { if (!sp_mean) sp_mean = hero_add_sp / 6 * GetSettingValue(carrier_x) / 100; return sp_mean; }

  int64_t uid() const { return carrier.carrier_id(); }
  int32_t pos() const { return 0; }
  int32_t id() const { return carrier.carrier_id(); }
  int64_t hp() const { return 1; }
  int64_t max_hp() const { return carrier.attr1(sy::ATTACK_ATTR_HP); }
  int32_t anger() const { return current_anger; }
  int32_t ap() const { return carrier.attr1(sy::ATTACK_ATTR_AP) + AddAp(); }
  int32_t sp() const { return carrier.attr1(sy::ATTACK_ATTR_SP) + AddSp(); }
  int32_t wf() const { return carrier.attr1(sy::ATTACK_ATTR_WF); }
  int32_t ff() const { return carrier.attr1(sy::ATTACK_ATTR_FF); }
  int32_t hit() const { return carrier.attr1(sy::ATTACK_ATTR_HIT_PERCENT); }
  int32_t miss() const { return carrier.attr1(sy::ATTACK_ATTR_MISS_PERCENT); }
  int32_t crit() const { return carrier.attr1(sy::ATTACK_ATTR_CRIT_PERCENT); }
  int32_t resist() const { return carrier.attr1(sy::ATTACK_ATTR_RESIST_PERCENT); }
  int32_t damage() const { return carrier.attr1(sy::ATTACK_ATTR_DAMAGE); }
  int32_t damage_dec() const { return carrier.attr1(sy::ATTACK_ATTR_DAMAGE_DECREASE); }
  const RepeatedInt64& attr() const { return carrier.attr1(); }
  int32_t country() const { return carrier_country; }

  void AddBuff(const BuffState& state);
  void AddBuffIndex(int64_t hero_uid, int32_t buff_uid) {
    this->buffs2.push_back(std::make_pair(hero_uid, buff_uid));
  }

  int32_t skill_damage_ratio(SkillBase* skill) const;
  int32_t skill_damage(SkillBase* skill) const;

  bool IsSetState(uint32_t mask) const { return false; }
  SkillBasePtr UseSkill(PK& pk) const;
  std::vector<BuffState>& Buffs();
  BuffIndexType& Buffs2() {
    return this->buffs2;
  }
};

struct PkObject : NonCopyable {
  PkObject() : fill_player(false), fight(0) {
    this->heros.reserve(6);
    scene_attr.resize(sy::AttackAttr_ARRAYSIZE, 0);
  }
  bool fill_player;
  int64_t fight;
  sy::PlayerSimpleInfo player;
  PkCarrier carrier;
  std::vector<PkHero> heros;
  std::vector<int64_t> helpers;
  Array<int64_t, sy::AttackAttr_ARRAYSIZE> scene_attr;  //场景增加的属性

  void SetCarrier(const sy::CurrentCarrierInfo& info);
  //overwrite是true会覆盖相同位置的船
  void AddHero(PK& pk, const sy::HeroInfo& info, int32_t pos, bool minus,
               bool overwrite, int64_t hp);
  PkHero* GetHeroByPos(int32_t pos);
  PkHero* GetHeroByHeroID(int32_t hero_id);
  int32_t AliveHero() const;
  void InitSceneAttr(const RepeatedInt64* attr);
  void CalcHeroAttr(PkHero& hero, PK& pk);
  void Clear();

  void FillGroupInfo(ReportLine& line);
  void AutoAddBuff(PK& pk, ReportLine& line);
  void FillHelpersInfo(ReportLine& line);
};

struct PK : NonCopyable {
  PK(const CopyBase* copy);

  //////////////////////////////////////////////////////////////////////
  //初始化场景增加的属性(这个属性只给玩家增加)
  void InitSceneAttr(const RepeatedInt64* attr);
  //玩家只有一波
  void InitPlayer(LogicPlayer* player, bool attacker,
                  const int32_t* hp_percent = NULL, float atk_percent = 1.0f);
  void InitPlayer(const sy::OtherPlayerInfo* other_player, bool attacker,
                  float uk_atk, float us_atk, float ge_atk, float jp_atk);

  //生成某一个group的战报
  //返回值0, 失败; 1,2,3成功
  int32_t GenerateGroupReport(int32_t group);
  //NPC可能有多波
  //overwrite为true的话, 那么会强制覆盖相同位置的船
  void InitMonsterGroup(const MonsterGroupBase* group, bool attacker, bool overwrite);
  //设置PVP NPC信息
  void InitMonsterGroup(int64_t player_id, const MonsterGroupBase* group, bool attacker);
  //设置BOSS信息(永远是被攻击者)
  void InitMonsterInfo(sy::CurrentCarrierInfo& carrier,
                       std::vector<sy::HeroInfo>& monster,
                       const int64_t* hp);
  //给攻守双方上战前buff
  void AutoAddBuff();
  //生成PVE战报
  void GeneratePVEReport(LogicPlayer* player);
  //生成不使用地图怪物的战报
  //需要自己手动设置攻守双方
  //包括PVP, 切磋, 竞技场PVE, 围剿BOSS
  void GeneratePVPReport();
  //////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////
  //胜利判断
  int32_t CheckSuccess();
  //星级判断
  int32_t CheckStar();
  //判断船是否活着(上阵活着,返回true; 其他返回false)
  bool CheckHeroAlive(bool attacker, int32_t hero);
  //////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////
  //获取目标
  //获取目标船只
  void GetTarget(IPkObject* attack, bool is_friend,
                 std::vector<PkHero*>& target, int32_t type);
  //获取特殊职业的船只对象
  void UniqueAddSpecialTarget(bool a_or_b, std::vector<PkHero*>& target,
                              int32_t job);
  //获取某个位置上的船只
  void TryGetTarget(bool a_or_b, std::vector<PkHero*>& target, int32_t pos);
  ////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////
  //一些帮助函数
  //0表示没有胜利者
  //1表示A胜利, B全死了
  //2表示B胜利, A全死了
  int32_t CheckAllDead();
  //通过UID获取船只
  PkHero* GetHeroByUID(int64_t uid);
  //试图删除上一个无内容的攻击战报
  void TryEraseLastAttackReport();
  //增加一行战报条目
  void AddReportContent(ReportLine& line);
  //增加航母信息
  void AddReportCarrierInfo(PkCarrier& carrier);
  //增加船只信息
  void AddReportHeroInfo(PkHero& hero);
  //关闭援军
  void DisableHelpers() { this->use_helpers = false; }
  ////////////////////////////////////////////////////////////////////////

  const CopyBase* copy;
  int64_t hero_uid;
  PkObject a;
  PkObject b;
  int32_t seed;
  RandomNubmer rand;
  sy::MessageResponseFightReport& response;
  sy::ReportRecord& report;
  int32_t wave;           //波次
  int32_t round;          //回合
  int32_t star;
  bool use_helpers;       //是否使用援军

  std::vector<IPkObject*> attack_order;
  ReportLine line;
};

template <typename Fn>
inline void IPkObject::FindBuffs(std::vector<BuffState>& output, Fn equal) {
  for (std::vector<BuffState>::iterator iter = this->Buffs().begin();
       iter != this->Buffs().end(); ++iter) {
    if (equal(*iter)) {
      output.push_back(*iter);
      output.back().target = static_cast<PkHero*>(this);
    }
  }
}

#endif
