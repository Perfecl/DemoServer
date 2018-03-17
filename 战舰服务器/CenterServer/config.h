#pragma once
#include <vector>
#include <common_define.h>
#include <config_file.h>
#include <logger.h>
#include <vector_map.h>
#include <pugixml.hpp>
#include <config_entry.h>

#define DEF_ENTRY(NAME1)                                \
struct NAME1;                                           \
typedef boost::shared_ptr<NAME1> NAME1##Ptr;            \
struct NAME1 : public ConfigEntry

#define DEF_CONFIG_FILE(NAME)                           \
class NAME : public XmlConfigFile {                     \
  public:                                               \
    NAME(const std::string& file_name) : XmlConfigFile(file_name) {}  \
  protected:                                            \
    virtual bool Parse();                               \
}


typedef std::vector<ValuePair2<int32_t, int32_t> > ValuePair2Vec;
typedef std::vector<ValuePair2<int64_t, int64_t> > ValuePair2VecInt64;
typedef std::vector<ValuePair3<int32_t, int32_t, int32_t> > ValuePair3Vec;


#define SETTING_KEY(k) const std::string k = #k
namespace Setting {
  //航母攻击加成系数
  SETTING_KEY(carrier_x);
  //怒气上限
  SETTING_KEY(fury_max);
  SETTING_KEY(crit_o);

  //暴击系数
  extern int32_t kCrit;

  int64_t GetValue(const std::string& key);
  void AddValue(const std::string& key, int64_t value);
  void AddValue(const std::string& key, std::vector<int32_t>& value);
  void AddValue(const std::string& key, ValuePair2Vec& value);
};

#define GetSettingValue(NAME) Setting::GetValue(Setting::NAME)

void AddConfigFile();
void AfterLoadConfig();

class ServerConfig : public XmlConfigFile {
 public:
  ServerConfig(const std::string& file_name) : XmlConfigFile(file_name) {}

  //first 是port
  //second是监听端口的类型(Player, GM之类的)
  typedef std::pair<uint16_t, uint16_t> ListenInfo;
  //监听的端口
  const std::vector<ListenInfo>& ports() const { return listen_ports_; }

  const std::string& http_port() const { return http_port_; }
  //日志等级
  LoggerLevel log_level() const { return log_level_; }
  //认证数据库的地址
  const MySQLParams& auth_mysql() const { return mysql_; }
  //GM工具数据库地址
  const MySQLParams& gm_mysql() const { return gm_mysql_; }

 protected:
  virtual bool Parse();
 private:
  std::vector<ListenInfo> listen_ports_;
  std::string http_port_;
  LoggerLevel log_level_;
  MySQLParams mysql_;
  MySQLParams gm_mysql_;
};

class SettingConfigFile : public XmlConfigFile {
 public:
  SettingConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}

  static int32_t GetRegionSize();
  //num是全局的计数器
  static void GenerateRegion(VectorMap<uint32_t, int32_t>& servers, int32_t& num);

 protected:
  virtual bool Parse();
};

class NameConfigFile : public XmlConfigFile {
 public:
  NameConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}
 protected:
  virtual bool Parse();
};

class LegionConfigFile : public XmlConfigFile {
 public:
  LegionConfigFile(const std::string& file_name) : XmlConfigFile(file_name) {}

   static const std::vector<std::pair<int32_t, int32_t> >& GetPositions();
 protected:
  virtual bool Parse();
};

DEF_ENTRY(BuffBase) {
  bool Fill(pugi::xml_node node);
  int32_t guorp_id;
  int32_t bufftimes;
  int32_t target_type;
  int32_t target;
  int32_t buff_value_type;
  int32_t buff_type;
  int32_t last;
  int32_t pro;  //单位是百万分之一
  ValuePair2<int32_t, int32_t> pro_type1;
  ValuePair2<int32_t, int32_t> pro_type2;
  ValuePair2<int32_t, int32_t> pro_type3;
};
#define BUFF_BASE ConfigEntryManager<BuffBase>::Instance()

DEF_ENTRY(SkillBase) {
  bool Fill(pugi::xml_node node);
  int32_t cd;
  int32_t times;
  int32_t fury;
  int32_t fight_type1;
  int32_t target_type;
  int32_t target;
  ValuePair2Vec carrier_attack_type;
  ValuePair2<int32_t, int32_t> value;
  std::vector<BuffBasePtr> buff_ptr;

  ValuePair2Vec skill_damage;

  std::pair<int32_t, int32_t> GetSkillDamage(int32_t fate_level) const;
};

#define SKILL_BASE ConfigEntryManager<SkillBase>::Instance()

DEF_CONFIG_FILE(SkillConfigFile);

DEF_ENTRY(HeroBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetLevelUpExp(int32_t current_level) const;

  int8_t quality;
  int8_t job;
  int8_t country;
  ValuePair2<int32_t, int32_t> skill;
  std::vector<int32_t> combo; //合体技能
  int32_t super_combo;        //超合体技
  int32_t make_cd;          //建造CD
  int32_t breakadvancedid;  //突破组ID
  ValuePair2Vec talent1;    //突破天赋
  std::vector<int32_t> karma1;  //缘分

  int16_t fury;             //怒气
  int32_t train_id;         //培养组ID
  ValuePair2<int32_t, int32_t> ship_piece; //船只合成
  ValuePair2<int32_t, int32_t> sell;

  int32_t wake_itemtree;
};

#define HERO_BASE ConfigEntryManager<HeroBase>::Instance()

DEF_CONFIG_FILE(HeroConfigFile);

DEF_ENTRY(CarrierBase) {
  bool Fill(pugi::xml_node node);

  int32_t GetLevelUpExp(int32_t current_level) const;
  std::pair<int32_t, int32_t> GetSkillByGrade(int32_t grade) const {
    for (ValuePair3Vec::const_reverse_iterator iter = this->skill.rbegin();
         iter != this->skill.rend(); ++iter) {
      if (grade >= iter->v1) {
        return std::pair<int32_t, int32_t>(iter->v2, iter->v3);
      }
    }
    return std::pair<int32_t, int32_t>(0 , 0);
  }

  int8_t quality;
  int8_t country;

  int32_t hit;
  int32_t crit;

  int32_t breakadvancedid;

  ValuePair3Vec skill;
  std::vector<int32_t> slot;
  std::vector<int32_t> plane_type;

  ValuePair2Vec item;
};
#define CARRIER_BASE  ConfigEntryManager<CarrierBase>::Instance()

DEF_CONFIG_FILE(CarrierConfigFile);
//副本
DEF_ENTRY(CopyBase) {
  bool Fill(pugi::xml_node node);

  std::vector<int32_t> award;
  std::vector<int32_t> lose_award;
  std::vector<int32_t> first_award; //首胜奖励
  int32_t award_hero; //N通关必掉
  int32_t copy_box; //手动领取的宝箱

  int32_t normal_card;
  int32_t special_card;

  int8_t type;        //类型
  std::vector<int32_t> monster; //怪物ID
  std::vector<int32_t> reinforcement; //我方援军
  int32_t power;      //消耗体力
  int32_t energy;     //消耗精力
  int32_t change_exp; //是否增加经验
  int32_t change_money;//是否增加金币

  int32_t lv;         //进入等级
  int32_t front_id;   //依赖
  int32_t enter_limit;//逻辑顺序依赖
  int32_t times_day;  //每天通关副本的次数
  int32_t times_one;  //是否只能通关一次
  int32_t star;       //是否需要评星
  int32_t forbid_day; //禁入日期
  int32_t mapped_copy_id; //副本归类的ID
  ValuePair2<int32_t, int32_t> condition; //胜利判断
};
#define COPY_BASE ConfigEntryManager<CopyBase>::Instance()

DEF_CONFIG_FILE(CopyConfigFile);
