#include "config.h"
#include "test.h"
#include <myrandom.h>
#include <fstream>
#include <algorithm>
#include <boost/make_shared.hpp>
#include <google/protobuf/text_format.h>
#include <logger.h>
#include <mutex.h>
#include "logic_player.h"
#include "item.h"
#include "server.h"
#include "dirty_searcher/dirty_searcher.h"
#include <boost/random.hpp>
#include <array_stream.h>
#include <str_util.h>
#include "time_activity.h"

#define __ADD_CONFIG(T, F) ConfigFileManager::Instance().AddConfigFile(new T(F))

#define __CHECK(NAME) if(!GetXmlAttr(NAME, node, #NAME)) { ERROR_LOG(logger)("%s, attribute:%s, value:%s, not correct", __PRETTY_FUNCTION__, #NAME, node.attribute(#NAME).as_string()); return false; }
#define __IGNORE(NAME) GetXmlAttr(NAME, node, #NAME)

template <typename T>
boost::shared_ptr<T> CheckParam(int64_t param, const char* pattern, ...) {
  boost::shared_ptr<T> ptr =
      ConfigEntryManager<T>::Instance().GetEntryByID(param);

  if (!ptr) {
    va_list arg_ptr;
    va_start(arg_ptr, pattern);
    ERROR_VLOG(logger)(NULL, pattern, arg_ptr);
    va_end(arg_ptr);
  }

  return ptr;
}

void AddConfigFile(ConfigFile* file) {
  ConfigFileManager::Instance().AddConfigFile(file);
  __ADD_CONFIG(DirtyWordsConfigFile, "./config/脏词过滤.txt");
  __ADD_CONFIG(VersionConfigFile, "./config/gameconfig.xml");
  __ADD_CONFIG(SkillConfigFile, "./config/技能.xml");
  __ADD_CONFIG(HeroConfigFile, "./config/hero.xml");
  __ADD_CONFIG(SettingConfigFile, "./config/setting.xml");
  __ADD_CONFIG(ExpConfigFile, "./config/等级经验.xml");
  __ADD_CONFIG(ItemConfigFile, "./config/道具.xml");
  __ADD_CONFIG(CarrierConfigFile, "./config/航母.xml");
  __ADD_CONFIG(EquipConfigFile, "./config/装备.xml");
  __ADD_CONFIG(TalentConfigFile, "./config/突破.xml");
  __ADD_CONFIG(BuildShipAndPlaneConfigFile, "./config/军舰制造和抽奖.xml");
  __ADD_CONFIG(ArmyConfigFile, "./config/部队.xml");
  __ADD_CONFIG(LootConfigFile, "./config/掉落.xml");
  __ADD_CONFIG(MonsterConfigFile, "./config/怪物.xml");
  __ADD_CONFIG(CopyConfigFile, "./config/副本.xml");
  __ADD_CONFIG(TrainConfigFile, "./config/培养.xml");
  __ADD_CONFIG(ShopConfigFile,"./config/商店商城.xml");
  __ADD_CONFIG(FateConfigFile,"./config/天命.xml");
  __ADD_CONFIG(PatrolConfigFile,"./config/深海探秘.xml");
  __ADD_CONFIG(ArenaConfigFile, "./config/竞技场.xml");
  __ADD_CONFIG(BuyCountConfigFile, "./config/购买次数价格.xml");
  __ADD_CONFIG(TowerConfigFile, "./config/突袭油田.xml");
  __ADD_CONFIG(RecoverConfigFile, "./config/回收.xml");
  __ADD_CONFIG(DstrikeConfigFile, "./config/围剿叛军.xml");
  __ADD_CONFIG(SignConfigFile, "./config/签到奖励.xml");
  __ADD_CONFIG(ActivityConfigFile, "./config/日常任务与成就.xml");
  __ADD_CONFIG(RankConfigFile, "./config/军衔.xml");
  __ADD_CONFIG(SevenDaysConfigFile, "./config/七日活动.xml");
  __ADD_CONFIG(SupportConfigFile, "./config/援军助威.xml");
  __ADD_CONFIG(CarrierCopyFile, "./config/航母副本.xml");
  __ADD_CONFIG(ChartConfigFile, "./config/图鉴.xml");
  __ADD_CONFIG(ServerOpenConfigFile, "./config/活动.xml");
  __ADD_CONFIG(LeagueConfigFile, "./config/军团.xml");
  __ADD_CONFIG(WorldBossConfigFile, "./config/围剿叛军boss.xml");
  __ADD_CONFIG(WakeConfigFile, "./config/z武将觉醒.xml");
  __ADD_CONFIG(EliteCopyConfigFile, "./config/z精英副本外敌入侵.xml");
  __ADD_CONFIG(RedGoldConfigFile, "./config/z神兵与金装.xml");
  __ADD_CONFIG(LegionConfigFile, "./config/y制霸全球.xml");
  __ADD_CONFIG(CrossServerConfigFile, "./config/y跨服演武.xml");
  __ADD_CONFIG(SweepStakeConfigFile,"./config/y富甲天下.xml");
  __ADD_CONFIG(UpdateVersionConfigFile, "./config/强更版本号.xml");
  __ADD_CONFIG(MedalConfigFile, "./config/x将灵.xml");
  __ADD_CONFIG(RedEquipConfigFile, "./config/x红装升星.xml");
  __ADD_CONFIG(HeroComeBackConfigFile, "./config/w英雄回归.xml");
  __ADD_CONFIG(PearlHarborConfigFile, "./config/x捍卫珍珠港.xml");
  __ADD_CONFIG(Server70001ConfigFile,"./70001.csv");
  __ADD_CONFIG(FileActivityConfigFile,"./config/配置类活动.xml");

  logger->SetPrintScreen(true);
}

void InitRobotConfig();

void AfterLoadConfig() {
  //这边增加校验什么的

  int32_t ft = GetSettingValue(festival_time1);
  Setting::kFestivalStartTime =
      MakeDateTime(ft / 10000, ft / 100 % 100, ft % 100, 0, 0, 0);
  ft = GetSettingValue(festival_time2);
  Setting::kFestivalEndTime =
      MakeDateTime(ft / 10000, ft / 100 % 100, ft % 100, 23, 59, 59);
  TRACE_LOG(logger)("Festival StartTime:%ld, EndTime:%ld", Setting::kFestivalStartTime, Setting::kFestivalEndTime);

  ft = GetSettingValue(limited_recruit_starttime);
  Setting::kLimitedRecruitStarttime =
      MakeDateTime(ft / 10000, ft / 100 % 100, ft % 100, 0, 0, 0);
  ft = GetSettingValue(limited_recruit_endtime);
  Setting::kLimitedRecruitEndtime =
      MakeDateTime(ft / 10000, ft / 100 % 100, ft % 100, 23, 59, 59);

  Setting::kArenaAwardTime = GetSettingValue(arena_award_time);

  Setting::kRecoverOilTime = std::max<int32_t>(GetSettingValue(oil_recover_time), 1);
  Setting::kOilMax = GetSettingValue(oil_max_limit);
  Setting::kOilRecoverMax = GetSettingValue(oil_recover_limit);

  Setting::kRecoverEnergyTime = std::max<int32_t>(GetSettingValue(energy_recover_time), 1);
  Setting::kEnergyMax = GetSettingValue(energy_max_limit);
  Setting::kEnergyRecoverMax = GetSettingValue(energy_recover_limit);

  Setting::kDstrikeTokenID = GetSettingValue(dstrike_token_id);
  Setting::kDstrikeTokenNum = GetSettingValue(dstrike_token_num);
  Setting::kDstrikeTokenTime = GetSettingValue(dstrike_token_time);


  Setting::kNameMinLen = std::min(int64_t(4), Setting::GetValue(Setting::name_min_byte));
  Setting::kNameMaxLen = std::max(int64_t(32), Setting::GetValue(Setting::name_max_byte));
  Setting::kShipStartID = GetSettingValue(ship_start_id);
  Setting::kShipEndID = GetSettingValue(ship_end_id);
  Setting::kSeatCount[2] = GetSettingValue(seat2_open_lv);
  Setting::kSeatCount[3] = GetSettingValue(seat3_open_lv);
  Setting::kSeatCount[4] = GetSettingValue(seat4_open_lv);
  Setting::kSeatCount[5] = GetSettingValue(seat5_open_lv);
  Setting::kSeatCount[6] = GetSettingValue(seat6_open_lv);
  const ValuePair2Vec& clear_cd_need = Setting::GetValue2(Setting::cre_clearcd_need);
  if (clear_cd_need.empty()) {
    Setting::kClearCDResearchHero.v1 = 1;
    Setting::kClearCDResearchHero.v2 = 1;
  } else {
    Setting::kClearCDResearchHero = clear_cd_need[0];
  }
  Setting::kCrit = GetSettingValue(crit_o);

  CheckParam<HeroBase>(Setting::GetValue(Setting::born_ship_id),
                       "setting:born_ship_id:%ld not found",
                       Setting::GetValue(Setting::born_ship_id));

  Setting::kRefreshSeconds = GetSettingValue(daily_refresh_time) * 3600;
  TRACE_LOG(logger)("RefreshHour:%ld", GetSettingValue(daily_refresh_time));
  InitRobotConfig();

  logger->SetPrintScreen(false);
  //Tester t;
  //t();
}

bool CheckItemParam(int32_t item_id) {
  if (item_id > sy::MoneyKind_ARRAYSIZE) return LogicItem::CheckItem(item_id);
  return true;
}

static boost::unordered_map<std::string, int64_t> setting_container_;
static boost::unordered_map<std::string, std::vector<int32_t> > setting_container_1_;
static boost::unordered_map<std::string, ValuePair2Vec> setting_container_2_;

namespace Setting {
int32_t kDstrikeTokenID = 1;
int32_t kDstrikeTokenNum = 10;
int32_t kDstrikeTokenTime = 3600;

int32_t kArenaAwardTime = 22;
time_t kFestivalStartTime = 0;
time_t kFestivalEndTime = 0;

int32_t kRecoverOilTime = 0;
int32_t kOilMax = 0;
int32_t kOilRecoverMax = 0;
int32_t kRecoverEnergyTime = 0;
int32_t kEnergyMax = 0;
int32_t kEnergyRecoverMax = 0;

int32_t kMaxLevel = 60;

int32_t kNameMinLen = 4;
int32_t kNameMaxLen = 32;
int32_t kEquipStartID = 20000000;
int32_t kEuipEndID = 21999999;
int32_t kItemStartID = 22000000;
int32_t kItemEndID = 23999999;
int32_t KArmyStartID = 24000000;
int32_t KArmyEndID = 25999999;
int32_t kPlaneStartID = 10200000;
int32_t kPlaneEndID = 10299999;
int32_t kShipStartID = 10000000;
int32_t kShipEndID = 10099999;
int32_t kCrit = 150;
time_t kLimitedRecruitStarttime = 0;
time_t kLimitedRecruitEndtime = 0;

int32_t kSeatCount[7] = {0, 0, 0, 3, 4, 5, 6};
ValuePair2<int32_t, int32_t> kClearCDResearchHero;
int32_t kRefreshSeconds = 0;

std::pair<int32_t, int32_t> kEquipRefineExp[5] = {
    std::make_pair(0, 0), std::make_pair(0, 0), std::make_pair(0, 0),
    std::make_pair(0, 0), std::make_pair(0, 0),
};

std::vector<int32_t> kGeneralCopyID;

int64_t GetValue(const std::string& key) {
  boost::unordered_map<std::string, int64_t>::const_iterator iter =
      setting_container_.find(key);
  return iter != setting_container_.end() ? iter->second : 0;
}

const std::vector<int32_t>& GetValue1(const std::string& key) {
  static std::vector<int32_t> empty;
  boost::unordered_map<std::string, std::vector<int32_t> >::const_iterator iter = setting_container_1_.find(key);
  return iter != setting_container_1_.end() ? iter->second : empty;
}

const ValuePair2Vec& GetValue2(const std::string& key) {
  static ValuePair2Vec empty;
  boost::unordered_map<std::string, ValuePair2Vec>::const_iterator iter = setting_container_2_.find(key);
  return iter != setting_container_2_.end() ? iter->second : empty;
}

void AddValue(const std::string& key, int64_t value) {
  setting_container_[key] = value;
}

void AddValue(const std::string& key, std::vector<int32_t>& value) {
  setting_container_1_[key].swap(value);
}

void AddValue(const std::string& key, ValuePair2Vec& value) {
  setting_container_2_[key].swap(value);
}

int32_t GetValueInVec2(const std::string& key, int32_t first) {
  const ValuePair2Vec& value = GetValue2(key);
  for (ValuePair2Vec::const_iterator iter = value.begin(); iter != value.end();
       ++iter) {
    if (iter->v1 == first) return iter->v2;
  }
  return 0;
}

int32_t GetMaxValueInVec2(const std::string& key, int32_t first) {
  const ValuePair2Vec& value = GetValue2(key);
  for (ValuePair2Vec::const_reverse_iterator iter = value.rbegin();
       iter != value.rend(); ++iter) {
    if (first >= iter->v1) return iter->v2;
  }
  return 0;
}

bool IsInFestival() {
  if (GetVirtualSeconds() >= Setting::kFestivalStartTime &&
      GetVirtualSeconds() <= Setting::kFestivalEndTime) {
    return true;
  }
  return false;
}

int32_t GetGeneralCopyCount(LogicPlayer* player) {
  int32_t count = 0;
  for (std::vector<int32_t>::const_iterator iter = kGeneralCopyID.begin();
       iter != kGeneralCopyID.end(); ++iter) {
    count += bool(player->GetCopyCount(*iter));
  }
  return count;
}
}

int32_t ServerConfig::GetAttrAsInt(const char* xpath, const char* attr) {
  pugi::xpath_node_set nodes = this->doc().select_nodes(xpath);
  if (nodes.begin() != nodes.end()) {
    return nodes.begin()->node().attribute(attr).as_int();
  }
  return 0;
}

std::string ServerConfig::GetAttrAsString(const char* xpath, const char* attr) {
  pugi::xpath_node_set nodes = this->doc().select_nodes(xpath);
  if (nodes.begin() != nodes.end()) {
    return nodes.begin()->node().attribute(attr).as_string();
  }
  return "";
}

bool ServerConfig::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  {
    pugi::xpath_node_set nodes = doc.select_nodes("/root/listen");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      ListenInfo info;
      info.first = node.attribute("port").as_int();
      std::string type = node.attribute("type").as_string();
      if (type == "client") info.second = ENTRY_TYPE_PLAYER;
      if (type == "php") info.second = ENTRY_TYPE_GM;

      if (info.second != 0) this->listen_ports_.push_back(info);
    }
  }

  this->log_level_ = (LoggerLevel)this->GetAttrAsInt("/root/log_level", "level");
  logger->log_level(this->log_level_);

  this->recharge_platform_ = this->GetAttrAsInt("/root/recharge", "platform");
  if (this->recharge_platform_ <= 0) this->recharge_platform_ = 1;
  TRACE_LOG(logger)("RechargePlatform, %d", this->recharge_platform_);

  this->enable_report_ = this->GetAttrAsInt("/root/report", "enable");
  this->enable_report_attr_ = this->GetAttrAsInt("/root/report", "enable_attr");
  this->enable_dirtywords_ = this->GetAttrAsInt("/root/dirtywords", "enable");

  this->record_server_addr_.first = this->GetAttrAsString("/root/record_server", "ip");
  this->record_server_addr_.second = this->GetAttrAsString("/root/record_server", "port");

  this->auth_server_addr_.first = this->GetAttrAsString("/root/auth_server", "ip");
  this->auth_server_addr_.second = this->GetAttrAsString("/root/auth_server", "port");

  this->center_server_addr_.first = this->GetAttrAsString("/root/center_server", "ip");
  this->center_server_addr_.second = this->GetAttrAsString("/root/center_server", "port");

  this->original_server_id_ = this->GetAttrAsInt("/root/server_id", "id");
  this->test_gm_ = this->GetAttrAsInt("/root/test_gm", "enable");

  {
    this->server_ids_.clear();
    pugi::xpath_node_set nodes = doc.select_nodes("/root/server_id/server");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      uint32_t server_id = node.attribute("id").as_int();
      this->server_ids_.push_back(server_id);
    }
    std::vector<uint32_t>::iterator iter =
        std::find(this->server_ids_.begin(), this->server_ids_.end(),
                  original_server_id_);
    if (iter != this->server_ids_.end()) {
      this->server_ids_.erase(iter);
    }
    this->server_ids_.insert(this->server_ids_.begin(), original_server_id_);
  }

  return true;
}

void Accumulation(VectorMap<int32_t, AttackAttrArray>& attr) {
  AttackAttrArray temp_array;
  for (VectorMap<int32_t, AttackAttrArray>::iterator it = attr.begin();
       it != attr.end(); ++it) {
    for (size_t i = 0; i < temp_array.size(); i++)
      it->second[i] += temp_array[i];
    temp_array = it->second;
  }
}

void AddExtraAttr(
    int32_t group_id, int32_t level,
    VectorMap<int32_t, VectorMap<int32_t, AttackAttrArray> >& srcMap,
    Array<int64_t, sy::AttackAttr_ARRAYSIZE>& destAttr) {
  VectorMap<int32_t, VectorMap<int32_t, AttackAttrArray> >::iterator it_group =
      srcMap.find(group_id);
  if (it_group != srcMap.end()) {
    if (it_group->second.empty()) return;
    VectorMap<int32_t, AttackAttrArray>::iterator it =
        it_group->second.upper_bound(level);
    if (it == it_group->second.begin()) return;
    --it;
    for (size_t i = 0; i < it->second.size(); i++) destAttr[i] += it->second[i];
  }
}

std::string PrefixBase::MakeName(uint32_t server_id, const std::string& name) {
  const PrefixBase* base = PREFIX_BASE.GetEntryByID(server_id / 10000).get();
  if (!base) {
    ERROR_LOG(logger)("ServerID:%u cannot find PrefixBase", server_id);
    return name;
  }

  DefaultArrayStream stream;
  stream.Append("%s%d.%s", base->prefix.c_str(), server_id % 10000, name.c_str());
  return stream.str();
}

bool PrefixBase::Fill(pugi::xml_node node) {
  this->prefix = node.attribute("prefix").as_string("");

  TRACE_LOG(logger)("PlatformID:%ld, Prefix:%s", this->id(), this->prefix.c_str());
  return true;
}

bool SettingConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  {
    pugi::xpath_node_set nodes = doc.select_nodes("/config/setting/item");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      const char* key = node.attribute("param").as_string();
      int64_t value = node.attribute("value").as_llong();
      if (key && *key) {
        Setting::AddValue(key, value);
        DEBUG_LOG(logger)("load setting, %s => %ld", key, value);
      }
    }
  }

  {
    std::vector<int32_t> value;
    pugi::xpath_node_set nodes = doc.select_nodes("/config/setting1/item");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      const char* key = node.attribute("param").as_string();
      value.clear();
      GetXmlAttr(value, node, "value");

      if (key && *key) {
        Setting::AddValue(key, value);
        DEBUG_LOG(logger)("load setting1, %s => %ld", key, value.size());
      }
    }
  }

  {
    ValuePair2Vec value;
    pugi::xpath_node_set nodes = doc.select_nodes("/config/setting2/item");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      const char* key = node.attribute("param").as_string();
      value.clear();
      GetXmlAttr(value, node, "value");

      if (key && *key) {
        DEBUG_LOG(logger)("load setting2, %s => %ld", key, value.size());
        Setting::AddValue(key, value);
      }
    }
  }

  if (!GetSettingValue(oil_recover_time)) {
    ERROR_LOG(logger)("setting oil_recover_time is 0");
    return false;
  }
  if (!GetSettingValue(energy_recover_time)) {
    ERROR_LOG(logger)("setting energy_recover_time is 0");
    return false;
  }


  this->ParseTable<PrefixBase, ConfigEntryManager<PrefixBase> >("/config/prefix/item", "id");

  return true;
}

bool VersionConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  std::string version;
  pugi::xpath_node_set nodes = doc.select_nodes("/gameconfig/game");
  for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
       iter != nodes.end(); ++iter) {
    version = iter->node().attribute("version").as_string("");
    break;
  }
  server_config->version(version);
  TRACE_LOG(logger)("VERSION:%s", version.c_str());
  return true;
}

VectorMap<std::string, FirstServerRewardAccount> kFirstServerReward;

const FirstServerRewardAccount* GetFirstServerReward(
    const std::string& openid) {
  VectorMap<std::string, FirstServerRewardAccount>::const_iterator iter = kFirstServerReward.find(openid);
  return iter != kFirstServerReward.end() ? &iter->second : NULL;
}

bool Server70001ConfigFile::Parse() {
  std::ifstream file(this->file_name().c_str());
  if (!file) {
    DEBUG_LOG(logger)("config file:%s not found", this->file_name().c_str());
    return true;
  }

  kFirstServerReward.clear();

  std::string line;
  while (std::getline(file, line, '\n')) {
    FirstServerRewardAccount account;
    char openid[128] = {0};
    if (sscanf(line.c_str(), "%[^,],%d,%d,%d,%d,%d,%d", openid, &account.money,
               &account.month_card, &account.life_card, &account.weekly_card,
               &account.month_card_1, &account.month_card_2) >= 7) {
      account.openid = openid;
      kFirstServerReward[account.openid] = account;
    } else {
      DEBUG_LOG(logger)("%s", line.c_str());
    }
  }

  return true;
}

bool DirtyWordsConfigFile::Parse() {
  std::ifstream file(this->file_name().c_str());
  if (!file) {
    ERROR_LOG(logger)("config file not found:%s", this->file_name().c_str());
    return false;
  }
  std::string line;
  while (std::getline(file, line, '\n')) {
    AddDirtyWord(line);
  }
  return true;
}

bool HeroBase::Fill(pugi::xml_node node) {
  __CHECK(quality);
  __CHECK(job);
  __CHECK(country);
  __CHECK(skill);
  __IGNORE(combo);
  __IGNORE(super_combo);
  __CHECK(make_cd);
  __CHECK(fury);
  __CHECK(breakadvancedid);
  __CHECK(talent1);
  __CHECK(train_id);
  __IGNORE(ship_piece);
  __IGNORE(sell);
  __CHECK(karma1);
  __IGNORE(quality);
  __IGNORE(wake_itemtree);

  //直接清掉不是主船的合体技能
  if (!combo.empty() && combo[0] == 0) {
    combo.clear();
  }
  if (!combo.empty() && combo.size() <= 2) {
    ERROR_LOG(logger)("HeroID:%ld, combo size is %lu", this->id(), combo.size());
  }
  DEBUG_LOG(logger)("load hero, id:%ld", this->id());
  return true;
}

int32_t HeroBase::GetLevelUpExp(int32_t current_level) const {
  const ExpBasePtr& exp_base = EXP_BASE.GetEntryByID(current_level);
  if (!exp_base) return 0;
  return exp_base->GetShipExp(this->quality);
}

bool RelationBase::Fill(pugi::xml_node node) {
  __CHECK(hero1_id);
  __IGNORE(equip);
  __CHECK(target);
  __CHECK(pro);

  if (hero1_id.empty() && !equip) {
    ERROR_LOG(logger)("HeroRelation, ID:%ld, hero1_id and equip are empty", this->id());
  }

  DEBUG_LOG(logger)("load relation, id:%ld, target:%d", this->id(), this->target);
  return true;
}

bool HeroConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<HeroBase, ConfigEntryManager<HeroBase> >("/config/hero/item", "id");
  this->ParseTable<RelationBase, ConfigEntryManager<RelationBase> >("/config/relation/item", "id");

  return true;
}

VectorSet<int32_t> ItemBase::navy_item;

bool ItemBase::Fill(pugi::xml_node node) {
  __CHECK(quality);
  __CHECK(can_lost);
  __CHECK(max_stack);
  __IGNORE(sell);
  __IGNORE(effect);
  __IGNORE(need_lv);

  if (this->id() >= GetSettingValue(armyparts_start_id) &&
      this->id() < GetSettingValue(armyparts_end_id))
    navy_item.insert(this->id());

  DEBUG_LOG(logger)("load item, item id:%ld", this->id());
  return true;
}

bool ItemConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<ItemBase, ConfigEntryManager<ItemBase> >("/config/item/item", "id");

  return true;
}

bool BreakTalentBase::Fill(pugi::xml_node node) {
  __CHECK(target);
  __CHECK(buff_type);

  DEBUG_LOG(logger)("load break talent, id:%ld", this->id());
  return true;
}

bool BreakAdvancedBase::Fill(pugi::xml_node node) {
  __CHECK(need_hero_lv);
  __CHECK(coin_cost);
  __CHECK(item_cost);
  __CHECK(hero_cost);
  __CHECK(base_attribute);
  __CHECK(grow_attribute);

  DEBUG_LOG(logger)("load break advanced, id:%ld", this->id());
  return true;
}

bool TalentConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<BreakTalentBase, ConfigEntryManager<BreakTalentBase> >("/config/break_talent/item", "id");
  this->ParseTable<BreakAdvancedBase, ConfigEntryManager<BreakAdvancedBase> >("/config/break_advanced/item", "id");

  return true;
}

bool EquipBase::Fill(pugi::xml_node node) {
  __CHECK(quality);
  __CHECK(type);
  __CHECK(suit);
  __CHECK(baseproperty);
  __CHECK(upproperty);
  __CHECK(refproperty);
  __IGNORE(equip_piece);
  __IGNORE(sell);
  __IGNORE(gaizao_tainfu);
  __IGNORE(shengjin_tainfu);

  DEBUG_LOG(logger)("load equip base, item id:%ld", this->id());
  return true;
}

bool SuitBase::Fill(pugi::xml_node node) {
  __CHECK(suit)
  __CHECK(num);
  __CHECK(property);
  return true;
}

bool EquipLevelUpBase::Fill(pugi::xml_node node) {
  __CHECK(green);
  __CHECK(blue);
  __CHECK(puple);
  __CHECK(orange);
  __CHECK(red);
  __CHECK(gold);

  DEBUG_LOG(logger)("load equip level up, item id:%ld", this->id());
  return true;
}

ValuePair2<int32_t, int32_t> EquipLevelUpBase::GetMoneyByTyEquipQuality(
    int32_t quality) const {
  switch (quality) {
    case sy::QUALITY_GREEN: return this->green; break;
    case sy::QUALITY_BLUE: return this->blue; break;
    case sy::QUALITY_PURPLE: return this->puple; break;
    case sy::QUALITY_ORANGE: return this->orange; break;
    case sy::QUALITY_RED: return this->red; break;
    case sy::QUALITY_UNIQUE: return this->gold; break;
  }
  return ValuePair2<int32_t, int32_t>();
}

bool EquipRefineBase::Fill(pugi::xml_node node) {
  __CHECK(green);
  __CHECK(blue);
  __CHECK(puple);
  __CHECK(orange);
  __CHECK(red);
  __CHECK(gold);

  DEBUG_LOG(logger)("load equip refine, item id:%ld", this->id());
  return true;
}

int32_t EquipRefineBase::GetExpByEquipQuality(int32_t quality) const {
  switch (quality) {
    case sy::QUALITY_GREEN: return this->green; break;
    case sy::QUALITY_BLUE: return this->blue; break;
    case sy::QUALITY_PURPLE: return this->puple; break;
    case sy::QUALITY_ORANGE: return this->orange; break;
    case sy::QUALITY_RED: return this->red; break;
    case sy::QUALITY_UNIQUE: return this->gold; break;
  }
  return 0;
}

static VectorMap<int32_t, EquipMasterBasePtr> equip_master_guide;

const EquipMasterBase* EquipMasterBase::GetEquipMasterBase(int32_t group_id,
                                                           int32_t level) {
  int32_t key = group_id * 100 + level;

  VectorMap<int32_t, EquipMasterBasePtr>::const_iterator it =
      equip_master_guide.upper_bound(key);

  if (it == equip_master_guide.begin() ||
      (it - 1)->second->id() / 100 != group_id)
    return NULL;

  return (it - 1)->second.get();
}

bool EquipMasterBase::Fill(pugi::xml_node node) {
  __CHECK(need_lv);
  __CHECK(property);

  if (id() > 10000 && id() < 20000)
    equip_master_guide[10000 + need_lv] =
        boost::static_pointer_cast<EquipMasterBase>(this->shared_from_this());
  if (id() > 20000 && id() < 30000)
    equip_master_guide[20000 + need_lv] =
        boost::static_pointer_cast<EquipMasterBase>(this->shared_from_this());
  if (id() > 30000 && id() < 40000)
    equip_master_guide[30000 + need_lv] =
        boost::static_pointer_cast<EquipMasterBase>(this->shared_from_this());
  if (id() > 40000 && id() < 50000)
    equip_master_guide[40000 + need_lv] =
        boost::static_pointer_cast<EquipMasterBase>(this->shared_from_this());
  if (id() > 50000 && id() < 60000)
    equip_master_guide[50000 + need_lv] =
        boost::static_pointer_cast<EquipMasterBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load equip master base, item id:%ld", this->id());
  return true;
}

bool EquipConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;
  equip_master_guide.clear();

  this->ParseTable<EquipBase, ConfigEntryManager<EquipBase> >("/config/equip/item", "id");
  this->ParseTable<SuitBase, ConfigEntryManager<SuitBase> >("/config/equip_suit/item", "id");
  this->ParseTable<EquipLevelUpBase, ConfigEntryManager<EquipLevelUpBase> >("/config/equip_lvup/item", "id");
  this->ParseTable<EquipRefineBase, ConfigEntryManager<EquipRefineBase> >("/config/equip_reform/item", "id");
  this->ParseTable<EquipMasterBase, ConfigEntryManager<EquipMasterBase> >("/config/equip_master/item", "id");
  return true;
}

bool ExpBase::Fill(pugi::xml_node node) {
  __CHECK(role_exp);

  __IGNORE(ship1_exp);
  __CHECK(ship2_exp);
  __CHECK(ship3_exp);
  __CHECK(ship4_exp);
  __CHECK(ship5_exp);
  __IGNORE(ship6_exp);

  __IGNORE(carrier1_exp);
  __IGNORE(carrier2_exp);
  __CHECK(carrier3_exp);
  __CHECK(carrier4_exp);
  __CHECK(carrier5_exp);
  __CHECK(carrier6_exp);

  __CHECK(add_power);
  __CHECK(add_energy);

  __CHECK(power_exp);
  __CHECK(power_money);
  __CHECK(energy_exp);
  __CHECK(energy_money);

  if (this->id() > Setting::kMaxLevel) {
    Setting::kMaxLevel = this->id();
  }

  DEBUG_LOG(logger)("load exp, Level:%ld, ship2_exp:%d", this->id(), this->ship2_exp);
  return true;
}

bool VipExpBase::Fill(pugi::xml_node node) {
  __CHECK(vip_exp);

  DEBUG_LOG(logger)("load vip exp, Level:%ld, exp:%d", this->id(), this->vip_exp);
  return true;
}

bool ExpConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<ExpBase, ConfigEntryManager<ExpBase> >("/config/level_exp/item", "id");
  this->ParseTable<VipExpBase, ConfigEntryManager<VipExpBase> >("/config/vip_exp/item", "id");

  return true;
}

bool CarrierBase::Fill(pugi::xml_node node) {
  __CHECK(quality);
  __CHECK(country);
  __CHECK(hit);
  __CHECK(crit);
  __CHECK(skill);
  __CHECK(item);
  __CHECK(slot);
  __CHECK(plane_type);
  __CHECK(breakadvancedid)

  DEBUG_LOG(logger)("load carrier, id:%ld", this->id());
  return true;
}

bool CarrierSlotBase::Fill(pugi::xml_node node) {
  __CHECK(item);
  __CHECK(money);

  DEBUG_LOG(logger)("load carrier slot, id:%ld", this->id());
  return true;
}

VectorMap<std::pair<int32_t, int32_t>, CarrierPlaneBasePtr> kPlaneBaseMap;

bool CarrierPlaneBase::Fill(pugi::xml_node node) {
  __CHECK(type);
  __CHECK(plane_lv);
  __CHECK(next_id);
  __CHECK(cost);
  __CHECK(extra_carrier);
  __CHECK(extra_ship);

  kPlaneBaseMap[std::make_pair(type, plane_lv)] =
      boost::static_pointer_cast<CarrierPlaneBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load carrier plane, id:%ld", this->id());
  return true;
}

bool CarrierConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kPlaneBaseMap.clear();

  this->ParseTable<CarrierBase, ConfigEntryManager<CarrierBase> >("/config/carrier_info/item", "id");
  this->ParseTable<CarrierPlaneBase, ConfigEntryManager<CarrierPlaneBase> >("/config/carrier_plane/item", "id");
  this->ParseTable<CarrierSlotBase, ConfigEntryManager<CarrierSlotBase> >("/config/carrier_position/item", "id");

  return true;
}

const CarrierPlaneBase* CarrierPlaneBase::GetPlaneBase(int32_t type,
                                                       int32_t plane_level) {
  VectorMap<std::pair<int32_t, int32_t>, CarrierPlaneBasePtr>::iterator it =
      kPlaneBaseMap.find(std::make_pair(type, plane_level));
  if (it == kPlaneBaseMap.end()) return NULL;
  return it->second.get();
}

std::vector<int32_t> date;
bool CopyBase::Fill(pugi::xml_node node) {
  __CHECK(type);
  __CHECK(monster);
  __CHECK(reinforcement);

  __CHECK(award);
  __CHECK(lose_award);
  __CHECK(first_award);
  __IGNORE(award_hero);
  __IGNORE(copy_box);

  __CHECK(normal_card);
  __CHECK(special_card);

  __CHECK(power);
  __CHECK(energy);
  __CHECK(change_exp);
  __CHECK(change_money);

  __CHECK(front_id);
  __CHECK(enter_limit);
  __CHECK(times_day);
  __CHECK(times_one);
  __CHECK(star);
  __IGNORE(condition);

  if (this->monster.size() != this->reinforcement.size()) {
    ERROR_LOG(logger)("CopyID:%ld monster size:%lu, reinforcement size:%lu"
        , this->id(), this->monster.size(), this->reinforcement.size());
  }

  //禁入日期
  date.clear();
  int32_t forbid_date = 0;
  __CHECK(date);
  for (std::vector<int32_t>::const_iterator iter = date.begin();
       iter != date.end(); ++iter) {
    forbid_date |= (1 << *iter);
  }
  this->forbid_day = forbid_date;
  //副本归类ID
  int32_t mapped_id = 0;
  __CHECK(mapped_id);
  this->mapped_copy_id = mapped_id;
  if (mapped_id && mapped_id != this->id()) {
    const CopyBasePtr ptr =
        CheckParam<CopyBase>(mapped_id, "CopyID:%ld, mapped_id:%d, not found",
                             this->id(), mapped_id);
    if (ptr) {
      this->mapped_copy_id = ptr->mapped_copy_id;
      this->forbid_day = ptr->forbid_day;
    }
  }
  this->mapped_copy_id = this->mapped_copy_id ? this->mapped_copy_id : this->id();

  if (this->forbid_day) {
    DEBUG_LOG(logger)("CopyID:%ld, MappedCopy:%d, ForbidDay:%d", this->id(), this->mapped_copy_id, this->forbid_day);
  }

  for (size_t i = 0; i < monster.size(); ++i) {
    CheckParam<MonsterGroupBase>(monster[i],
                                    "CopyID:%ld, MonsterGroupID:%d, not found",
                                    this->id(), monster[i]);
  }

  if (this->type == sy::COPY_TYPE_GENERAL) {
    Setting::kGeneralCopyID.push_back(this->id());
  }
  DEBUG_LOG(logger)("load copy, copy id:%ld", this->id());
  return true;
}

int32_t CopyBase::GetOrder() const {
  if (this->type == sy::COPY_TYPE_TOWER && this->order <= 0) {
    while (true) {
      const CopyBase* base = COPY_BASE.GetEntryByID(this->id() - 1).get();
      if (!base) break;
      return base->GetOrder();
    }
  }
  return this->order;
}

bool CopyBase::IsTowerShowBox() const {
  if (this->type != sy::COPY_TYPE_TOWER) return false;
  const CopyBase* ptr = COPY_BASE.GetEntryByID(this->id() / 10 * 10 + 1).get();
  if (!ptr) return false;
  const CopyChapterBase* base = COPY_CHAPTER_BASE.GetEntryByID(ptr->chapter).get();
  if (!base) return false;
  return base->box;
}

static std::vector<int32_t> part;

bool CopyGateBase::Fill(pugi::xml_node node) {
  int32_t hang = 0;
  part.clear();
  __CHECK(part);
  __CHECK(hang);

  this->copys.clear();
  for (std::vector<int32_t>::const_iterator iter = part.begin();
       iter != part.end(); ++iter) {
    CopyBasePtr copy =
        CheckParam<CopyBase>(*iter, "load config, %s, copy:%d not found",
                             __PRETTY_FUNCTION__, *iter);
    if (!copy) return false;
    this->copys.push_back(copy);
  }
  if (this->copys.empty()) {
    ERROR_LOG(logger)("load copy gate, attribute[part] is empty");
    return false;
  }

  this->hang_ptr = COPY_BASE.GetEntryByID(hang);
  if (hang && !this->hang_ptr) {
    ERROR_LOG(logger)("load copy gate, CopyID:%d not found", hang);
    return false;
  }

  DEBUG_LOG(logger)("load copy gate, ID:%ld", this->id());
  return true;
}

static boost::unordered_map<int64_t, CopyChapterBasePtr> follow_chapter;
static inline int64_t FollowChapterID(int32_t copy_type, int32_t chapter) {
  return int64_t(copy_type) << 32 | chapter;
}

bool CopyChapterBase::Fill(pugi::xml_node node) {
  std::vector<int32_t> gate;
  __CHECK(open_limit);
  __CHECK(last_chapter);
  __CHECK(type);
  __CHECK(gate);
  __CHECK(star1);
  __CHECK(box);

  this->levels.clear();
  for (std::vector<int32_t>::const_iterator iter = gate.begin();
       iter != gate.end(); ++iter) {
    CopyGateBasePtr level = CheckParam<CopyGateBase>(
        *iter, "load chapter, %s, Chapter:%ld, copy gate:%d not found",
        __PRETTY_FUNCTION__, this->id(), *iter);

    if (!level) return false;

    this->levels.push_back(level);
  }
  if (this->levels.empty()) {
    ERROR_LOG(logger)("load copy chapter, attribute[gate] is empty");
  }

  follow_chapter[FollowChapterID(type, last_chapter)] = boost::static_pointer_cast<CopyChapterBase>(this->shared_from_this());
  DEBUG_LOG(logger)("load copy chapter, id:%ld", this->id());
  return true;
}

const CopyBasePtr& CopyChapterBase::GetFirstCopy() const {
  static CopyBasePtr empty;
  if (this->levels.empty()) return empty;
  if ((*this->levels.begin())->copys.empty()) return empty;
  return *(*this->levels.begin())->copys.begin();
}

CopyChapterBasePtr CopyChapterBase::GetFollowChapter(int32_t copy_type,
                                                     int32_t chapter) {
  static CopyChapterBasePtr empty;
  boost::unordered_map<int64_t, CopyChapterBasePtr>::iterator iter =
      follow_chapter.find(FollowChapterID(copy_type, chapter));
  return iter != follow_chapter.end() ? iter->second : empty;
}

bool CopyConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  Setting::kGeneralCopyID.clear();

  this->ParseTable<CopyBase, ConfigEntryManager<CopyBase> >("/config/copy_info/item", "id");
  this->ParseTable<CopyGateBase, ConfigEntryManager<CopyGateBase> >("/config/copy_gate/item", "id");
  this->ParseTable<CopyChapterBase, ConfigEntryManager<CopyChapterBase> >("/config/copy_chapter/item", "id");

  //计算副本的顺序
  for (int32_t copy_type = sy::CopyType_MIN; copy_type <= sy::CopyType_MAX; ++copy_type) {
    int32_t order = 1;
    CopyChapterBasePtr chapter = CopyChapterBase::GetFollowChapter(copy_type, 0);
    while (chapter) {
      for (std::vector<CopyGateBasePtr>::const_iterator iter_gate =
               chapter->levels.begin();
           iter_gate != chapter->levels.end(); ++iter_gate) {
        for (std::vector<CopyBasePtr>::const_iterator iter_copy =
                 (*iter_gate)->copys.begin();
             iter_copy != (*iter_gate)->copys.end(); ++iter_copy) {
          (*iter_copy)->SetOrder(order);
          (*iter_copy)->SetChapter(chapter->id());
          ++order;
          TRACE_LOG(logger)("CopyType:%d, Chapter:%ld, CopyID:%ld, Order:%d",
              copy_type, chapter->id(), (*iter_copy)->id(), (*iter_copy)->order);
        }
      }
      chapter = CopyChapterBase::GetFollowChapter(copy_type, chapter->id());
    }
  }
  return true;
}

bool ShipPackBase::Fill(pugi::xml_node node) {
  __CHECK(heroid);

  int32_t distribution = 0;
  for (std::vector<ValuePair2<int32_t, int32_t> >::const_iterator iter =
           this->heroid.begin();
       iter != this->heroid.end(); ++iter) {
    if (!CheckParam<HeroBase>(iter->v1, "%s, ShipPack:%ld, HeroID:%d not found",
                              __PRETTY_FUNCTION__, this->id(), iter->v1))
      return false;
    distribution += iter->v2;
  }
  this->sum = distribution;

  return true;
}

int32_t ShipPackBase::Random(
    int32_t rand, const std::vector<ValuePair2<int32_t, int32_t> >& vec) {
  if (rand < 0) rand = 0;

  int32_t count = 0;
  for (std::vector<ValuePair2<int32_t, int32_t> >::const_iterator iter =
           vec.begin();
       iter != vec.end(); ++iter) {
    count += iter->v2;
    if (rand < count) return iter->v1;
  }
  return 0;
}

bool ShipRaffleBase::Fill(pugi::xml_node node) {
  __CHECK(packs);

  this->packs_ptr.clear();
  for (std::vector<int32_t>::const_iterator iter = this->packs.begin();
       iter != this->packs.end(); ++iter) {
    const ShipPackBasePtr& base = CheckParam<ShipPackBase>(
        *iter, "%s, ShipPack:%d not found", __PRETTY_FUNCTION__, *iter);
    if (!base) continue;
    this->packs_ptr.push_back(base);
  }
  INFO_LOG(logger)("ShipPack:%ld, HeroCount:%lu", this->id(), this->packs_ptr.size());
  return true;
}

const ShipRaffleBasePtr& GetShipRaffleByCount(int32_t count, int32_t money) {
  int32_t base = 0;
  int32_t pack = -1;

  int32_t lucky = 0;
  int32_t loop = 0;

  switch (money) {
    case 1:
      base = 0;
      lucky = 100;
      loop = GetSettingValue(exh_adv_times);
      break;
    case 10:
      base = 200;
      lucky = 300;
      loop = GetSettingValue(exh_adv10_times);
      break;
    case 20:
      base = 400;
      break;
    case 21:
      base = 500;
      break;
  }

  if (loop && !(count % loop)) {
    pack = lucky;
  } else {
    if (count < 100) pack = count % 100 + base;
  }

  const ShipRaffleBasePtr& result =
      ConfigEntryManager<ShipRaffleBase>::Instance().GetEntryByID(pack);
  if (result) return result;
  return ConfigEntryManager<ShipRaffleBase>::Instance().GetEntryByID(base);
}

bool BuildShipAndPlaneConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<ShipPackBase, ConfigEntryManager<ShipPackBase> >("/config/ship_pack/item", "id");
  this->ParseTable<ShipRaffleBase, ConfigEntryManager<ShipRaffleBase> >("/config/ship_raffle/item", "id");

  return true;
}

static boost::unordered_map<int32_t, VectorSet<int32_t> > kLootMap;

bool LootBase::Fill(pugi::xml_node node) {
  __CHECK(box_id);
  __CHECK(level);
  __CHECK(type);
  __CHECK(value);
  __CHECK(info);
  kLootMap[box_id].insert(level);
  for (ValuePair3Vec::const_iterator iter = this->info.begin();
       iter != this->info.end(); ++iter) {
    if (iter->v1 >= GetSettingValue(ship_start_id) &&
        iter->v1 <= GetSettingValue(ship_end_id)) {
      if (!CheckParam<HeroBase>(iter->v1, "%s HeroID:%d not found",
                                __PRETTY_FUNCTION__, iter->v1))
        return false;
    } else {
      if (!CheckItemParam(iter->v1)) {
        ERROR_LOG(logger)("%s ItemID:%d not found", __PRETTY_FUNCTION__, iter->v1);
        return false;
      }
    }
  }

  TRACE_LOG(logger)("load drop, id:%ld", this->id());
  return true;
}

template <typename T>
struct RandomTrait;

template <>
struct RandomTrait<ValuePair3Vec> {
  typedef ValuePair3Vec::const_iterator iterator;
  static iterator begin(const ValuePair3Vec& v) { return v.begin(); }
  static iterator end(const ValuePair3Vec& v) { return v.end(); }
  static int32_t weight(iterator iter) { return iter->v3; }
};

template <>
struct RandomTrait<ValuePair2Vec> {
  typedef ValuePair2Vec::const_iterator iterator;
  static iterator begin(const ValuePair2Vec& v) { return v.begin(); }
  static iterator end(const ValuePair2Vec& v) { return v.end(); }
  static int32_t weight(iterator iter) { return iter->v2; }
};

template <typename T>
static inline int32_t RandomInContainer(const T& container) {
  typedef typename RandomTrait<T>::iterator iterator;
  int32_t max_random = 0;
  for (iterator iter = RandomTrait<T>::begin(container);
       iter != RandomTrait<T>::end(container); ++iter) {
    max_random += RandomTrait<T>::weight(iter);
  }
  if (max_random < 1) return -1;
  int32_t random = RandomBetween(0, max_random - 1);
  int32_t index = 0;
  for (iterator iter = RandomTrait<T>::begin(container);
       iter != RandomTrait<T>::end(container); ++iter, ++index) {
    random -= RandomTrait<T>::weight(iter);
    if (random <= 0) return index;
  }
  return -1;
}

inline void AddLootItem(ModifyCurrency& modify, AddSubItemSet& item,
                        int32_t key, int32_t value) {
  if (!key) return;
  if (key >= sy::MONEY_KIND_COIN && key < sy::MoneyKind_ARRAYSIZE) {
    modify[key] += value;
  } else {
    bool found = false;
    //装备
    if (LogicItem::GetMaxCount(key) == 1) {
      switch (value) {
        case 6: if (!item.full()) item.push_back(ItemParam(key, 1));
        case 5: if (!item.full()) item.push_back(ItemParam(key, 1));
        case 4: if (!item.full()) item.push_back(ItemParam(key, 1));
        case 3: if (!item.full()) item.push_back(ItemParam(key, 1));
        case 2: if (!item.full()) item.push_back(ItemParam(key, 1));
        case 1: if (!item.full()) item.push_back(ItemParam(key, 1));
          break;
        default: {
          for (int32_t i = 0; i < value; ++i) {
            if (!item.full()) item.push_back(ItemParam(key, 1));
          }
          break;
        }
      }
    } else {
      for (AddSubItemSet::iterator iter = item.begin(); iter != item.end();
           ++iter) {
        if (iter->item_id == key) {
          iter->item_count += value;
          found = true;
          break;
        }
      }
      if (!found && !item.full()) item.push_back(ItemParam(key, value));
    }
  }
}

//1. 独立随机
inline void LootByRandom_1(ModifyCurrency& modify, AddSubItemSet& item,
                           const ValuePair3Vec& random, int32_t random_count,
                           std::vector<int32_t>& out) {
  ValuePair3Vec v = random;
  while (random_count > 0) {
    int32_t index = RandomInContainer(v);
    if (index < 0 || index > int32_t(v.size())) break;
    AddLootItem(modify, item, v[index].v1, v[index].v2);
    --random_count;
    out.push_back(index);
  }
}

//4. 独立随机(带剔除)
inline void LootByRandom_4(ModifyCurrency& modify, AddSubItemSet& item,
                           const ValuePair3Vec& random, int32_t random_count,
                           std::vector<int32_t>& out) {
  ValuePair3Vec v = random;
  while (random_count > 0) {
    int32_t index = RandomInContainer(v);
    if (index < 0 || index > int32_t(v.size())) break;
    AddLootItem(modify, item, v[index].v1, v[index].v2);
    v.erase(v.begin() + index);
    out.push_back(index);
    --random_count;
  }
}
//2. 全部获得
inline void LootByRandom_2(ModifyCurrency& modify, AddSubItemSet& item,
                           const ValuePair3Vec& random, int32_t random_count,
                           std::vector<int32_t>& out) {
  (void)random_count;
  int32_t index = 0;
  for (ValuePair3Vec::const_iterator iter = random.begin();
       iter != random.end(); ++iter, ++index) {
      AddLootItem(modify, item, iter->v1, iter->v2);
      out.push_back(index);
  }
}
//3. 独立随机
inline void LootByRandom_3(ModifyCurrency& modify, AddSubItemSet& item,
                           const ValuePair3Vec& random, int32_t random_count,
                           std::vector<int32_t>& out) {
  (void)random_count;
  int32_t index = 0;
  for (ValuePair3Vec::const_iterator iter = random.begin();
       iter != random.end(); ++iter, ++index) {
    if (RandomIn10000() <= iter->v3) {
      AddLootItem(modify, item, iter->v1, iter->v2);
      out.push_back(index);
    }
  }
}

void LootByChoose(ModifyCurrency& modify, AddSubItemSet& item,
                  const ValuePair3Vec& info, std::vector<int32_t>& out) {
  if (out.empty()) return;

  for (ValuePair3Vec::const_iterator it = info.begin(); it != info.end();
       ++it) {
    if (it->v1 == out[0]) {
      AddLootItem(modify, item, it->v1, it->v2);
      break;
    }
  }
}

void LootByUID(ModifyCurrency& modify, AddSubItemSet& item,
               const ValuePair3Vec& info, std::vector<int32_t>& out) {
  if (out.empty()) return;
  int32_t index = out[0] % info.size();
  AddLootItem(modify, item, info[index].v1, info[index].v2);
}

void LootBase::Loot(ModifyCurrency& modify, AddSubItemSet& item,
                    __OUT__ std::vector<int32_t>* out) const {
  static std::vector<int32_t> empty;
  empty.clear();
  std::vector<int32_t>& output = out ? *out : empty;
  //掉落
  switch (this->type) {
    case 1:
      LootByRandom_1(modify, item, this->info, this->value, output);
      break;
    case 2:
      LootByRandom_2(modify, item, this->info, this->value, output);
      break;
    case 3:
      LootByRandom_3(modify, item, this->info, this->value, output);
      break;
    case 4:
      LootByChoose(modify, item, this->info, output);
      break;
    case 5:
      LootByUID(modify, item, this->info, output);
      break;
  }
}

static inline int32_t GenLootID(int32_t box_id, int32_t level) {
  return box_id * 1000 + level;
}

LootBasePtr LootConfigFile::Get(int32_t box_id, int32_t level) {
  static LootBasePtr empty;

  VectorSet<int32_t>::iterator iter = kLootMap[box_id].upper_bound(level);
  if (!kLootMap.empty() && iter != kLootMap[box_id].begin()) {
    return LOOT_BASE.GetEntryByID(GenLootID(box_id, *(--iter)));
  }
  TRACE_LOG(logger)("LootBase not found box_id:%d, level:%d", box_id, level);
  return empty;
}

bool LootConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;
  kLootMap.clear();
  this->ParseTable<LootBase, ConfigEntryManager<LootBase> >("/config/drop/item", "id");
  return true;
}

bool TrainBase::Fill(pugi::xml_node node) {
  __CHECK(cost_item);
  __CHECK(cost_money);
  __CHECK(train);
  __CHECK(grow);

  DEBUG_LOG(logger)("load train config, id:%ld", this->id());
  return true;
}

bool TrainLimitBase::Fill(pugi::xml_node node) {
  __CHECK(hp_max);
  __CHECK(attack_max);
  __CHECK(wf_max);
  __CHECK(ff_max);

  DEBUG_LOG(logger)("load train limit config, id:%ld", this->id());
  return true;
}

//固定写死四个属性
void TrainLimitBase::GetLimit(int32_t level, __OUT__ int32_t* attr_max) const {
  const ValuePair2Vec* array[] = {&this->hp_max, &this->attack_max,
                                  &this->wf_max, &this->ff_max};
  for (int32_t index = 0; index < ArraySize(array); ++index) {
    attr_max[index] = 0;
    for (ValuePair2Vec::const_reverse_iterator iter = array[index]->rbegin();
         iter != array[index]->rend(); ++iter) {
      if (level >= iter->v1) {
        attr_max[index] = iter->v2;
        break;
      }
    }
  }
}

bool TrainConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<TrainBase, ConfigEntryManager<TrainBase> >("/config/train/item", "id");
  this->ParseTable<TrainLimitBase, ConfigEntryManager<TrainLimitBase> >("/config/train_limit/item", "id");
  return true;
}

int32_t ArmyBase::GetLevelUpExp(int32_t current_level) const {
  const ArmyExpBasePtr& exp_base = ARMY_EXP_BASE.GetEntryByID(current_level);
  if (!exp_base) return 0;
  return exp_base->GetExp(this->quality);
}

int32_t CarrierBase::GetLevelUpExp(int32_t current_level) const {
  const ExpBasePtr& exp_base = EXP_BASE.GetEntryByID(current_level);
  if (!exp_base) return 0;
  return exp_base->GetCarrierExp(this->quality);
}

static VectorMap<int32_t, ArmyBasePtr> army_items;

ArmyBase* ArmyBase::GetArmyBaseByArmyItem(int32_t item_id) {
  VectorMap<int32_t, ArmyBasePtr>::iterator it = army_items.find(item_id);
  if (it == army_items.end())
    return NULL;
  else
    return it->second.get();
}

bool ArmyBase::Fill(pugi::xml_node node) {
  __CHECK(quality);
  __CHECK(type);
  __CHECK(exp);
  __CHECK(can_up);
  __CHECK(can_equip);
  __CHECK(soldier);
  __CHECK(baseproperty);
  __CHECK(refproperty);
  __IGNORE(sell);
  __CHECK(probability_player);
  __CHECK(probability_computer);
  __IGNORE(gaizao_tainfu);
  __IGNORE(shengjin_tainfu);

  for (ValuePair2Vec::const_iterator iter = this->soldier.begin();
       iter != this->soldier.end(); ++iter) {
    if (!CheckItemParam(iter->v1)) {
      ERROR_LOG(logger)("%s, ItemID:%d not found", __PRETTY_FUNCTION__, iter->v1);
      return false;
    }

    army_items[iter->v1] =
        boost::static_pointer_cast<ArmyBase>(this->shared_from_this());
  }

  DEBUG_LOG(logger)("load army config, id:%ld", this->id());
  return true;
}

bool ArmyExpBase::Fill(pugi::xml_node node) {
  __CHECK(green);
  __CHECK(blue);
  __CHECK(puple);
  __CHECK(orange);
  __CHECK(red);
  __CHECK(gold);

  DEBUG_LOG(logger)("load army exp config, id:%ld", this->id());
  return true;
}

bool ArmyRefineBase::Fill(pugi::xml_node node) {
  __CHECK(cost);
  __CHECK(self_cost);

  DEBUG_LOG(logger)("load army gaizao config, id:%ld", this->id());
  return true;
}

bool ArmyConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<ArmyBase, ConfigEntryManager<ArmyBase> >("/config/army/item", "id");
  this->ParseTable<ArmyExpBase, ConfigEntryManager<ArmyExpBase> >("/config/army_level_exp/item", "id");
  this->ParseTable<ArmyRefineBase, ConfigEntryManager<ArmyRefineBase> >("/config/army_gaizao_exp/item", "id");
  return true;
}

const int32_t kFeatsSoltNum = 32;
VectorMap<int32_t, VectorMap<int32_t, ValuePair2Vec>* > fresh_shop_map;

bool ShopConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  fresh_shop_map.clear();

  this->ParseTable<ShopBase, ConfigEntryManager<ShopBase> >("/config/shop/item",
                                                            "id");
  for (VectorMap<int32_t, VectorMap<int32_t, ValuePair2Vec>*>::iterator ic =
           fresh_shop_map.begin();
       ic != fresh_shop_map.end(); ++ic) {
    for (int32_t i = 0; i < kFeatsSoltNum; i++) {
      VectorMap<int32_t, ValuePair2Vec>& vct = ic->second[i];
      for (VectorMap<int32_t, ValuePair2Vec>::iterator it = vct.begin();
           it != vct.end(); ++it) {
        if ((it + 1) == vct.end()) break;
        std::copy(it->second.begin(), it->second.end(),
                  std::back_inserter((it + 1)->second));
      }
    }
  }

  return true;
}

bool ShopBase::Fill(pugi::xml_node node) {
  __CHECK(shoptype);
  __CHECK(item);
  __CHECK(moneytype);
  __IGNORE(price);
  __IGNORE(vip);
  __IGNORE(star);
  __CHECK(lv_min);
  __CHECK(lv_max);
  __IGNORE(weight);
  __IGNORE(lattice_position);
  __CHECK(onsale);
  __IGNORE(exchange);
  __IGNORE(startday);

  ValuePair2<int32_t, int32_t> pair;
  if (lattice_position >= 1 && lattice_position <= kFeatsSoltNum && onsale &&
      IsRefreshShop(shoptype)) {
    if (!fresh_shop_map[shoptype])
      fresh_shop_map[shoptype] =
          new VectorMap<int32_t, ValuePair2Vec>[kFeatsSoltNum];
    VectorMap<int32_t, ValuePair2Vec>* temp = fresh_shop_map[shoptype];
    pair.v1 = this->id();
    pair.v2 = this->weight;
    temp[lattice_position - 1][lv_min].push_back(pair);
  }

  if (!price.empty() && price[0].v1 != 1) {
    ERROR_LOG(logger)("load shop config, id:%ld, price[0]:%d is not 1"
        , this->id(), price[0].v1);
  }
  if (price.empty() && exchange.empty())
    price.push_back(ValuePair2<int32_t, int32_t>(1, -1));

  DEBUG_LOG(logger)("load shop config, id:%ld", this->id());
  return true;
}

void ShopBase::RandomFeatsCommodity(
    int32_t shop_id, int32_t level,
    ::google::protobuf::RepeatedPtrField< ::sy::ShopCommodityInfo>* out,
    int32_t server_start_day) {
  out->Clear();

  VectorMap<int32_t, VectorMap<int32_t, ValuePair2Vec>*>::iterator it =
      fresh_shop_map.find(shop_id);
  if (it == fresh_shop_map.end()) return;
  if (!it->second) return;
  VectorMap<int32_t, ValuePair2Vec>* temp = it->second;
  for (int32_t i = 0; i < kFeatsSoltNum; i++) {
    if (temp[i].empty()) continue;

    VectorMap<int32_t, ValuePair2Vec>::iterator it = temp[i].upper_bound(level);
    if (it == temp[i].begin())
      continue;
    else
      --it;

    ValuePair2Vec tem_vct;
    for (ValuePair2Vec::iterator it_t = it->second.begin();
         it_t != it->second.end(); ++it_t) {
      ShopBase* base = SHOP_BASE.GetEntryByID(it_t->v1).get();
      if (base && level <= base->lv_max && server_start_day >= base->startday)
        tem_vct.push_back(ValuePair2<int32_t, int32_t>(it_t->v1, it_t->v2));
    }

    int32_t index = RandomInContainer(tem_vct);
    if (index >= 0 && index < int32_t(tem_vct.size())) {
      sy::ShopCommodityInfo* info = out->Add();
      info->set_commodity_id(tem_vct[index].v1);
      info->set_bought_count(0);
    }
  }
}

bool ShopBase::IsRefreshShop(int32_t shop_id) {
  const std::vector<int32_t>& vct =
      Setting::GetValue1(Setting::refresh_shop_type);
  for (size_t i = 0; i < vct.size(); i++) {
    if (vct[i] == shop_id) return true;
  }
  return false;
}

std::pair<int32_t, int32_t> ShopBase::GetBuyTypeAndCount(int32_t vip) const {
  int32_t type = 0;
  int32_t count = -1;
  for (ValuePair3Vec::const_iterator it = this->vip.begin();
       it != this->vip.end(); ++it) {
    type = it->v2;
    if (vip >= it->v1 && count < it->v3) count = it->v3;
  }

  return std::pair<int32_t, int32_t>(type, count);
}

int32_t ShopBase::GetPrice(int32_t bought_count, int32_t buy_count) const {
  int32_t total = 0;
  int32_t bought_count_t = bought_count + 1;
  for (int32_t i = 0; i < buy_count; ++i) {
    for (ValuePair2Vec::const_reverse_iterator rit = price.rbegin();
         rit != price.rend(); ++rit) {
      if (bought_count_t >= rit->v1) {
        total += rit->v2;
        break;
      }
    }
    bought_count_t++;
  }

  return total;
}

bool FateConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<FateBase, ConfigEntryManager<FateBase> >("/config/fate/item",
                                                            "id");

  return true;
}

bool FateBase::Fill(pugi::xml_node node) {
  __CHECK(cost);
  __CHECK(base_value);
  __CHECK(max_value);
  __CHECK(attack_value);
  __CHECK(hp_value);
  __CHECK(huopaodef_value);
  __CHECK(daodandef_value);

  DEBUG_LOG(logger)("load fate config, id:%ld", this->id());
  return true;
}

bool PatrolConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<PatrolBase, ConfigEntryManager<PatrolBase> >(
      "/config/patrol/item", "id");

  return true;
}

bool PatrolBase::Fill(pugi::xml_node node) {
  __CHECK(ship_award);
  __CHECK(item_award);
  __CHECK(mapped_id);
  __CHECK(skill_award);
  __CHECK(skill_time);
  __CHECK(skill_cost);

  DEBUG_LOG(logger)("load patrol config, id:%ld", this->id());
  return true;
}

static VectorMap<int32_t, ArenaRewardBase> arena_reward;
static VectorMap<int32_t, ArenaGoldRewardBase> arena_gold_reward;
static VectorMap<int32_t, ArenaRankBase> arena_rank_offset;

bool ArenaRewardBase::Fill(pugi::xml_node node) {
  __CHECK(id);
  __CHECK(daily_reward);

  DEBUG_LOG(logger)("load arena reward config, id:%d", this->id);
  return true;
}

bool ArenaGoldRewardBase::Fill(pugi::xml_node node) {
  __CHECK(id);
  __CHECK(gold);

  DEBUG_LOG(logger)("load arena reward config, id:%d", this->id);
  return true;
}

bool ArenaRankBase::Fill(pugi::xml_node node) {
  __CHECK(id);
  __CHECK(difference);

  DEBUG_LOG(logger)("load arena reward config, id:%d", this->id);
  return true;
}

bool ArenaConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  arena_reward.clear();
  arena_gold_reward.clear();
  arena_rank_offset.clear();

  {
    pugi::xpath_node_set nodes = doc.select_nodes("/config/arena_everyday_reward/item");
    for (pugi::xpath_node_set::iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      ArenaRewardBase base;
      base.Fill(node);
      arena_reward[base.id] = base;
    }
  }

  {
    pugi::xpath_node_set nodes = doc.select_nodes("/config/arena_gold_reward/item");
    for (pugi::xpath_node_set::iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      ArenaGoldRewardBase base;
      base.Fill(node);
      arena_gold_reward[base.id] = base;
    }
  }

  {
    pugi::xpath_node_set nodes = doc.select_nodes("/config/arena_rank/item");
    for (pugi::xpath_node_set::iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();
      ArenaRankBase base;
      base.Fill(node);
      arena_rank_offset[base.id] = base;
    }
  }

  return true;
}

ArenaRewardIter ArenaConfigFile::GetDailyAwardByRank(int32_t rank) {
  VectorMap<int32_t, ArenaRewardBase>::const_iterator iter =
      arena_reward.upper_bound(rank);
  return iter != arena_reward.begin() ? iter - 1 : iter;
}

ArenaRewardIter ArenaConfigFile::GetDailyAwardEnd() {
  return arena_reward.end();
}

ArenaGoldRewardIter ArenaConfigFile::GetGoldRewardByRank(int32_t rank) {
  VectorMap<int32_t, ArenaGoldRewardBase>::const_iterator iter =
      arena_gold_reward.lower_bound(rank);
  return iter;
}

ArenaGoldRewardIter ArenaConfigFile::GetGoldRewardEnd() {
  return arena_gold_reward.end();
}

int32_t ArenaConfigFile::GetArenaRankOffset(int32_t rank) {
  VectorMap<int32_t, ArenaRankBase>::const_iterator iter =
      arena_rank_offset.lower_bound(rank);
  iter = iter == arena_rank_offset.end() ? arena_rank_offset.end() - 1 : iter;
  return RandomBetween(iter->second.difference.v1, iter->second.difference.v2);
}

void LootByRandomCommon(const ValuePair2Vec& container, int32_t random_count,
                        std::vector<int32_t>& out) {
  out.clear();
  ValuePair2Vec v = container;
  while (random_count > 0) {
    int32_t index = RandomInContainer(v);
    if (index < 0 || index > int32_t(v.size())) break;
    --random_count;
    out.push_back(v[index].v1);
  }
}

bool BuyCountCostBase::Fill(pugi::xml_node node) {
  __CHECK(type);
  __CHECK(times);
  __CHECK(cost);

  return true;
}

bool BuyCountConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<BuyCountCostBase, ConfigEntryManager<BuyCountCostBase> >(
      "/config/cost/item", "id");
  this->ParseTable<VipFunctionBase, ConfigEntryManager<VipFunctionBase> >(
      "config/vip_function/item", "id");

  return true;
}

std::vector<int32_t> kTower3StarBuff;
std::vector<int32_t> kTower6StarBuff;
std::vector<int32_t> kTower9StarBuff;

bool TowerBuffBase::Fill(pugi::xml_node node) {
  __CHECK(effect_type);
  __CHECK(star_num);

  if (star_num == 3) kTower3StarBuff.push_back(this->id());
  if (star_num == 6) kTower6StarBuff.push_back(this->id());
  if (star_num == 9) kTower9StarBuff.push_back(this->id());

  DEBUG_LOG(logger)("load tower_buff config, id:%ld", this->id());
  return true;
}

bool TowerConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kTower3StarBuff.clear();
  kTower6StarBuff.clear();
  kTower9StarBuff.clear();
  this->ParseTable<TowerBuffBase, ConfigEntryManager<TowerBuffBase> >("/config/oil_star_buff/item", "id");

  return true;
}

void TowerConfigFile::GetTowerLootAward(AddSubItemSet& item_set,
                                        ModifyCurrency& modify,
                                        ValuePair2Vec& award, int32_t money,
                                        int32_t level) {
  std::vector<int32_t> index;
  const LootBasePtr& ptr1 = LootConfigFile::Get(
      Setting::GetMaxValueInVec2(Setting::server_tower_award_box_id1, money),
      level);
  const LootBasePtr& ptr2 = LootConfigFile::Get(
      Setting::GetMaxValueInVec2(Setting::server_tower_award_box_id2, money),
      level);

  if (ptr1) {
    index.clear();
    ptr1->Loot(modify, item_set, &index);
    for (size_t i = 0; i < index.size(); ++i) {
      ValuePair3<int32_t, int32_t, int32_t> a = ptr1->info[index[i]];
      award.push_back(ValuePair2<int32_t, int32_t>(a.v1, a.v2));
    }
  }
  if (ptr2) {
    index.clear();
    ptr2->Loot(modify, item_set, &index);
    for (size_t i = 0; i < index.size(); ++i) {
      ValuePair3<int32_t, int32_t, int32_t> a = ptr2->info[index[i]];
      award.push_back(ValuePair2<int32_t, int32_t>(a.v1, a.v2));
    }
  }
}

void TowerConfigFile::RandomBuff(int32_t& b3, int32_t& b6, int32_t& b9) {
  if (kTower3StarBuff.empty() || kTower6StarBuff.empty() ||
      kTower9StarBuff.empty())
    return;
  b3 = kTower3StarBuff[RandomBetween(0, kTower3StarBuff.size() - 1)];
  b6 = kTower6StarBuff[RandomBetween(0, kTower6StarBuff.size() - 1)];
  b9 = kTower9StarBuff[RandomBetween(0, kTower9StarBuff.size() - 1)];
}

bool RecoverConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<RecoverBase, ConfigEntryManager<RecoverBase> >(
      "/config/disassemble/item", "id");

  return true;
}

bool RecoverBase::Fill(pugi::xml_node node) {
  __CHECK(type);

  DEBUG_LOG(logger)("load recover config, id:%ld", this->id());
  return true;
}

static VectorMap<int32_t, DstrikeTriggerBasePtr> kDstrikeBossLevelMap;
static VectorMap<int64_t, DstrikeBossBasePtr> kDstrikeBossMap;
static VectorMap<int32_t, DstrikeRankAwardBasePtr> kDstrikeRankAwardMap;
static VectorSet<int32_t> kDstrikeMeritAwardLevel;

bool DstrikeConfigFile::Parse(){
  pugi::xml_document& doc = this->doc();
  if(!doc) return false;
  kDstrikeBossLevelMap.clear();
  kDstrikeBossMap.clear();
  kDstrikeRankAwardMap.clear();
  kDstrikeMeritAwardLevel.clear();

  this->ParseTable<DstrikeTriggerBase, ConfigEntryManager<DstrikeTriggerBase> >("config/dstrike_trigger/item", "id");
  this->ParseTable<DstrikeMeritAwardBase, ConfigEntryManager<DstrikeMeritAwardBase> >("config/dstrike_merit_award/item", "id");
  this->ParseTable<DstrikeBossBase, ConfigEntryManager<DstrikeBossBase> >("config/dstrike_boss/item", "id");
  this->ParseTable<DstrikeRankAwardBase, ConfigEntryManager<DstrikeRankAwardBase> >("config/dstrike_rank_award/item", "id");
  return true;
}

const DstrikeBossBase* DstrikeConfigFile::RandomBoss(int32_t level,
                                                     int32_t type) {
  DstrikeTriggerBasePtr boss_level;
  for (VectorMap<int32_t, DstrikeTriggerBasePtr>::const_reverse_iterator iter =
           kDstrikeBossLevelMap.rbegin();
       iter != kDstrikeBossLevelMap.rend(); ++iter) {
    if (level >= iter->first) {
      boss_level = iter->second;
      break;
    }
  }
  if (!boss_level) return NULL;
  const ValuePair2Vec* vec = type == sy::COPY_TYPE_NORMAL ? &boss_level->main : &boss_level->elite;
  if (!vec) return NULL;

  //随机
  int32_t boss_quality = 0;
  const int32_t rand = RandomIn10000();
  int32_t count = 0;
  for (ValuePair2Vec::const_iterator iter = vec->begin(); iter != vec->end();
       ++iter) {
    count += iter->v2;
    if (rand <= count) {
      boss_quality = iter->v1;
      break;
    }
  }
  if (!boss_quality) return NULL;
  const DstrikeBossBase* base = DSTRIKE_BOSS_BASE.GetEntryByID(boss_level->lv * 100 + boss_quality).get();
  return base;
}

const DstrikeBossBase* DstrikeConfigFile::GetBossByID(int64_t boss_id) {
  VectorMap<int64_t, DstrikeBossBasePtr>::const_iterator iter = kDstrikeBossMap.find(boss_id);
  return iter != kDstrikeBossMap.end() ? iter->second.get() : NULL;
}

const DstrikeRankAwardBase* DstrikeConfigFile::GetRewardByRank(int32_t rank) {
  VectorMap<int32_t, DstrikeRankAwardBasePtr>::iterator it =
      kDstrikeRankAwardMap.upper_bound(rank);
  return it != kDstrikeRankAwardMap.begin() ? (it - 1)->second.get() : NULL;
}

const DstrikeMeritAwardBase* DstrikeConfigFile::GetDailyAward(int32_t level,
                                                              int32_t index) {
  VectorSet<int32_t>::const_iterator iter = kDstrikeMeritAwardLevel.upper_bound(level);
  if (iter != kDstrikeMeritAwardLevel.begin()) {
    int32_t dest_level = *(iter - 1);
    return DSTRIKE_BOSS_DAILY_AWARD_BASE.GetEntryByID(dest_level * 100 + index).get();
  }
  return NULL;
}

bool DstrikeTriggerBase::Fill(pugi::xml_node node){
  __CHECK(lv);
  __CHECK(main);
  __CHECK(elite);

  kDstrikeBossLevelMap[lv] = boost::static_pointer_cast<DstrikeTriggerBase>(this->shared_from_this());
  return true;
}

bool DstrikeMeritAwardBase::Fill(pugi::xml_node node) {
  __CHECK(lv);
  __CHECK(index);
  __CHECK(merit_num);
  __CHECK(merit_award);
  kDstrikeMeritAwardLevel.insert(lv);

  if (index < 0 || index >= 64) {
    ERROR_LOG(logger)("dstrike merit award, id:%ld, index out of bound:%d", this->id(), index);
  }

  DEBUG_LOG(logger)("load dstrike merit award config, id:%ld", this->id());
  return true;
}

bool DstrikeBossBase::Fill(pugi::xml_node node) {
  __CHECK(lv);
  __CHECK(quality);
  int32_t monster_group;
  __CHECK(monster_group);
  __CHECK(grow_monster_value);

  const MonsterGroupBasePtr& base = MONSTER_GROUP_BASE.GetEntryByID(monster_group);
  if (!base) {
    ERROR_LOG(logger)("load dstrike boss config, monster_group:%d not found", monster_group);
  }
  this->monster = base;

  kDstrikeBossMap[monster_group] = boost::static_pointer_cast<DstrikeBossBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load dstrike boss config, id:%ld", this->id());
  return true;
}

bool DstrikeRankAwardBase::Fill(pugi::xml_node node) {
  __CHECK(range);
  __CHECK(damage_rankaward);
  __CHECK(merit_rankaward);

  kDstrikeRankAwardMap[range] = boost::static_pointer_cast<DstrikeRankAwardBase>(this->shared_from_this());

  return true;
}

int64_t MonsterGroupBase::GetAttr(const ValuePair2Vec& values, int32_t attr_index) const {
  for (ValuePair2Vec::const_iterator iter = values.begin();
       iter != values.end(); ++iter) {
    if (iter->v1 == attr_index) return iter->v2;
  }
  return 0;
}

int32_t DstrikeBossBase::GetAttrGrow(int32_t attr_index) const {
  for (ValuePair2Vec::const_iterator iter = this->grow_monster_value.begin();
       iter != this->grow_monster_value.end(); ++iter) {
    if (iter->v1 == attr_index) return iter->v2;
  }
  return 0;
}

void DstrikeBossBase::FillMonsterInfos(sy::CurrentCarrierInfo& carrier,
                                       std::vector<sy::HeroInfo>& heros,
                                       int32_t level) const {
  if (!this->monster) return;
  carrier = this->monster->carrier_info();
  heros = this->monster->hero_info();

  for (std::vector<sy::HeroInfo>::iterator iter = heros.begin();
       iter != heros.end(); ++iter) {
    sy::HeroInfo& hero = *iter;
    google::protobuf::RepeatedField<int64_t>* attr = hero.mutable_attr1();
    attr->Resize(sy::AttackAttr_ARRAYSIZE, 0);
    for (size_t i = 0; i < this->grow_monster_value.size(); ++i) {
      int32_t index = this->grow_monster_value[i].v1;
      int32_t add = this->grow_monster_value[i].v2 * (level - 1);
      if (index > 0 && index < sy::AttackAttr_ARRAYSIZE) {
        attr->Set(index, attr->Get(index) + add);
      }
    }
  }
}

void DstrikeBossBase::FillBlood(
    google::protobuf::RepeatedField< ::google::protobuf::int64>& blood,
    int32_t level) const {
  blood.Resize(6, 0);
  const MonsterGroupBase* base = this->monster.get();
  if (!base) return;

  int64_t add_hp = this->GetAttrGrow(sy::ATTACK_ATTR_HP) * (level - 1);

  const AttrVec* array[] = {
      &base->monster1_attr, &base->monster2_attr, &base->monster3_attr,
      &base->monster4_attr, &base->monster5_attr, &base->monster6_attr,
  };
  for (int32_t i = 0; i < ArraySize(array) && i < 6; ++i) {
    int64_t hp = array[i]->Get(sy::ATTACK_ATTR_HP);
    if (hp) blood.Set(i, hp ? hp + add_hp : 0);
  }
}

int64_t CalcFightScore(const AttrVec& vec, bool ignore_hit) {
  int64_t score = 0;

  //（面板攻击*3+面板生命*0.25+面板物防*5+面板法防*5）*（1+暴击率+抗暴率+命中率+闪避率+伤害加成+伤害减免）
  score += (vec.Get(sy::ATTACK_ATTR_AP) + vec.Get(sy::ATTACK_ATTR_SP)) / 2 * 3;
  score += vec.Get(sy::ATTACK_ATTR_HP) / 4;
  score += (vec.Get(sy::ATTACK_ATTR_WF) + vec.Get(sy::ATTACK_ATTR_FF)) * 5;
  if (!ignore_hit)
    score = score * (1000 + vec.Get(sy::ATTACK_ATTR_CRIT_PERCENT) +
                     vec.Get(sy::ATTACK_ATTR_RESIST_PERCENT) +
                     vec.Get(sy::ATTACK_ATTR_HIT_PERCENT) +
                     vec.Get(sy::ATTACK_ATTR_MISS_PERCENT) +
                     vec.Get(sy::ATTACK_ATTR_DAMAGE) +
                     vec.Get(sy::ATTACK_ATTR_DAMAGE_DECREASE)) /
            1000;
  return score;
}

inline void FillMonsterAttr(AttrVec& attr, const ValuePair2VecInt64& vec) {
  attr.Clear();
  attr.Resize(sy::AttackAttr_ARRAYSIZE, 0);
  for (ValuePair2VecInt64::const_iterator iter = vec.begin(); iter != vec.end();
       ++iter) {
    if (iter->v1 < sy::AttackAttr_ARRAYSIZE) attr.Set(iter->v1, iter->v2);
  }
  int64_t score = CalcFightScore(attr, false);
  attr.Set(0, score);
}

static ValuePair2VecInt64 carrier_value, monster1_value, monster2_value,
    monster3_value, monster4_value, monster5_value, monster6_value;

bool MonsterGroupBase::Fill(pugi::xml_node node) {
  __IGNORE(monster_group );
  __IGNORE(skill_lv);
  __IGNORE(monster_lv);
  skill_lv.resize(7, 0);
  monster_lv.resize(7, 0);
  this->carrier.Clear();
  this->heros.clear();

  carrier_value .clear();
  monster1_value.clear();
  monster2_value.clear();
  monster3_value.clear();
  monster4_value.clear();
  monster5_value.clear();
  monster6_value.clear();

  __IGNORE(quality);
  quality.resize(7, 0);
  __IGNORE(carrier_value );
  __IGNORE(monster1_value);
  __IGNORE(monster2_value);
  __IGNORE(monster3_value);
  __IGNORE(monster4_value);
  __IGNORE(monster5_value);
  __IGNORE(monster6_value);
  FillMonsterAttr(carrier_attr,  carrier_value );
  FillMonsterAttr(monster1_attr, monster1_value);
  FillMonsterAttr(monster2_attr, monster2_value);
  FillMonsterAttr(monster3_attr, monster3_value);
  FillMonsterAttr(monster4_attr, monster4_value);
  FillMonsterAttr(monster5_attr, monster5_value);
  FillMonsterAttr(monster6_attr, monster6_value);

  DEBUG_LOG(logger)("load monster group, id:%ld", this->id());
  return true;
}

bool MonsterBase::Fill(pugi::xml_node node) {
  __IGNORE(country);
  __IGNORE(job);
  __IGNORE(skill);
  __IGNORE(combo);
  __IGNORE(start_buff);

  if (!combo.empty() && combo[0] == 0) {
    combo.clear();
  }
  if (!combo.empty() && combo.size() <= 2) {
    ERROR_LOG(logger)("MonsterID:%ld, Combo size is %lu", this->id(), combo.size());
  }

  DEBUG_LOG(logger)("load monster, id:%ld", this->id());
  return true;
}

bool MonsterConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if(!doc) return false;

  this->ParseTable<MonsterBase, ConfigEntryManager<MonsterBase> >("config/monster/item", "id");
  this->ParseTable<MonsterGroupBase, ConfigEntryManager<MonsterGroupBase> >("config/monster_group/item", "id");
  return true;
}

int32_t SignBase::max_id = 0;

bool SignBase::Fill(pugi::xml_node node) {
  __CHECK(award);
  __CHECK(vipaward_lv);

  max_id = this->id() > max_id ? this->id() : max_id;

  DEBUG_LOG(logger)("load sign base, id:%ld", this->id());
  return true;
}

bool SignConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<SignBase, ConfigEntryManager<SignBase> >("config/sign/item",
                                                            "id");
  return true;
}

bool SkillBase::Fill(pugi::xml_node node) {
  ValuePair2Vec value_grow;
  ValuePair2Vec valuenum_grow;

  __CHECK(cd);
  __CHECK(times);
  __CHECK(fury);
  __CHECK(fight_type1);
  __CHECK(target_type);
  __CHECK(target);
  __IGNORE(carrier_attack_type);
  __IGNORE(value);
  __IGNORE(value_grow);
  __IGNORE(valuenum_grow);

  uint32_t max_index =
      std::max(value_grow.empty() ? 0 : value_grow.back().v1,
               valuenum_grow.empty() ? 0 : valuenum_grow.back().v1);
  this->skill_damage.resize(0);
  this->skill_damage.resize(max_index + 1);

  this->skill_damage[0].v1 = value.v1 / 10;
  this->skill_damage[0].v2 = value.v2;

  for (uint32_t i = 0; i < max_index; ++i) {
    this->skill_damage[i + 1].v1 =
        this->value.v1 / 10 +
        (i < value_grow.size() ? value_grow[i].v2 / 10 : 0);
    this->skill_damage[i + 1].v2 =
        this->value.v2 + (i < valuenum_grow.size() ? valuenum_grow[i].v2 : 0);
  }

  std::vector<int32_t> buff;
  __IGNORE(buff);
  this->buff_ptr.clear();
  for (std::vector<int32_t>::const_iterator iter = buff.begin();
       iter != buff.end(); ++iter) {
    BuffBasePtr ptr = CheckParam<BuffBase>(
        *iter, "SkillID:%ld, BuffID:%d not found", this->id(), *iter);
    if (!ptr) continue;
    this->buff_ptr.push_back(ptr);
  }

  DEBUG_LOG(logger)("load skill config, id:%ld", this->id());
  return true;
}

//返回伤害千分比和额外固定值
std::pair<int32_t, int32_t> SkillBase::GetSkillDamage(
    int32_t fate_level) const {
  if (fate_level < int32_t(this->skill_damage.size())) {
    return std::make_pair(this->skill_damage[fate_level].v1, this->skill_damage[fate_level].v2);
  }

  return std::make_pair(this->skill_damage.back().v1,
                        this->skill_damage.back().v2);
}

bool BuffBase::Fill(pugi::xml_node node) {
  __CHECK(guorp_id);
  __CHECK(bufftimes);
  __CHECK(target_type);
  __CHECK(target);
  __CHECK(buff_value_type);
  __CHECK(buff_type);
  __CHECK(last);
  __CHECK(pro);
  __IGNORE(pro_type1);
  __IGNORE(pro_type2);
  __IGNORE(pro_type3);

  DEBUG_LOG(logger)("load buff config, id:%ld", this->id());
  return true;
};

bool SkillConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if(!doc) return false;

  this->ParseTable<BuffBase, ConfigEntryManager<BuffBase> >("config/buff/item", "id");
  this->ParseTable<SkillBase, ConfigEntryManager<SkillBase> >("config/skill/item", "id");
  return true;
}

bool ActivityBase::Fill(pugi::xml_node node) {
  __CHECK(time);
  __CHECK(reward);
  __CHECK(points);

  DEBUG_LOG(logger)("load activity config, id:%ld", this->id());
  return true;
}

bool ActivityRewardBase::Fill(pugi::xml_node node) {
  __CHECK(reward);

  DEBUG_LOG(logger)("load activity_reward config, id:%ld", this->id());
  return true;
}

bool AchievementBase::Fill(pugi::xml_node node) {
  __CHECK(type);
  __CHECK(time);
  __CHECK(reward);

  DEBUG_LOG(logger)("load achievement config, id:%ld", this->id());
  return true;
}

bool ActivityConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<ActivityBase, ConfigEntryManager<ActivityBase> >(
      "config/activity/item", "id");
  this->ParseTable<ActivityRewardBase, ConfigEntryManager<ActivityRewardBase> >(
      "config/activity_reward/item", "id");
  this->ParseTable<AchievementBase, ConfigEntryManager<AchievementBase> >(
      "config/achievement/item", "id");
  return true;
}

bool RankBase::Fill(pugi::xml_node node) {
  __CHECK(rank);
  __CHECK(add);
  __CHECK(reward);
  __CHECK(cost);
  __CHECK(carrier_box);

  DEBUG_LOG(logger)("load rank config, id:%ld", this->id());
  return true;
};

bool RankConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<RankBase, ConfigEntryManager<RankBase> >("config/rank/item",
                                                            "id");
  return true;
}

bool SevenDaysBase::Fill(pugi::xml_node node) {
  __CHECK(day);
  __CHECK(index);
  __CHECK(type);
  __CHECK(condition);
  __IGNORE(reward);

  for (size_t i = 0; i < this->reward.size(); i++) {
    const LootBasePtr& loot_base = LootConfigFile::Get(this->reward[i], 1);
    if (!loot_base) {
      ERROR_LOG(logger)("loot base not found id: %d", this->reward[i]);
      return false;
    }
  }

  DEBUG_LOG(logger)("load sevendays config, id:%ld", this->id());
  return true;
};

bool SevenDays14Base::Fill(pugi::xml_node node) {
  __CHECK(day);
  __CHECK(index);
  __CHECK(type);
  __CHECK(condition);
  __IGNORE(reward);

  for (size_t i = 0; i < this->reward.size(); i++) {
    const LootBasePtr& loot_base = LootConfigFile::Get(this->reward[i], 1);
    if (!loot_base) {
      ERROR_LOG(logger)("loot base not found id: %d", this->reward[i]);
      return false;
    }
  }

  DEBUG_LOG(logger)("load sevendays 14 config, id:%ld", this->id());
  return true;
};

bool SevenDaysConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<SevenDaysBase, ConfigEntryManager<SevenDaysBase> >(
      "config/sevendays/item", "id");
  this->ParseTable<SevenDays14Base, ConfigEntryManager<SevenDays14Base> >(
      "config/sevendays_14/item", "id");
  return true;
}

//第一次10连抽选择的船
//后续有奖励是跟这个有关系的
//都是用mod算法, 策划必须得保证首次包和后续奖励的个数一样大, 否则会偏掉
int32_t ShipPackBase::GetInitialShip(int64_t uid, int32_t count) {
  const ShipPackBase* base =
      SHIP_PACK_BASE.GetEntryByID(
                        Setting::GetValueInVec2(Setting::exh_get_id, count))
          .get();
  if (!base) return 0;
  return base->heroid[uid % base->heroid.size()].v1;
}

static VectorMap<int32_t, SupportBasePtr> support_guide;

bool SupportBase::Fill(pugi::xml_node node) {
  __CHECK(need_lv);
  __CHECK(property);

  support_guide[need_lv] =
      boost::static_pointer_cast<SupportBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load support config, id:%ld", this->id());
  return true;
};

const SupportBasePtr SupportBase::GetSupportBase(int32_t level) {
  VectorMap<int32_t, SupportBasePtr>::iterator it =
      support_guide.upper_bound(level);
  return support_guide.begin() == it ? SupportBasePtr() : (it - 1)->second;
}

bool SupportConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  support_guide.clear();
  this->ParseTable<SupportBase, ConfigEntryManager<SupportBase> >(
      "config/support/item", "id");
  return true;
}

static std::vector<std::vector<MonsterGroupBasePtr> > RobotConfigPool;

void InitRobotConfig() {
  static const std::string kRobotGroup = "robot_groups";
  RobotConfigPool.clear();

  const std::vector<int32_t>& robot_groups = Setting::GetValue1(kRobotGroup);
  for (std::vector<int32_t>::const_iterator iter = robot_groups.begin();
       iter != robot_groups.end(); ++iter) {
    std::vector<MonsterGroupBasePtr> monster_group;

    char robot[256] = {0};
    snprintf(robot, sizeof robot, "robot_%d", *iter);
    const std::vector<int32_t>& group = Setting::GetValue1(robot);
    for (std::vector<int32_t>::const_iterator iter_robot = group.begin();
         iter_robot != group.end(); ++iter_robot) {
      const MonsterGroupBasePtr& base = MONSTER_GROUP_BASE.GetEntryByID(*iter_robot);
      if (base)
        monster_group.push_back(base);
      else
        ERROR_LOG(logger)("Robot_%d, MonsterGroupID:%d not found", *iter, *iter_robot);
    }

    if (!monster_group.empty()) RobotConfigPool.push_back(monster_group);
  }
}

const MonsterGroupBasePtr& GetRobotConfigByID(int32_t robot_id) {
  static MonsterGroupBasePtr empty;
  if (robot_id > sy::MAX_ROBOT_ID) return empty;
  if (RobotConfigPool.empty()) return empty;
  const std::vector<MonsterGroupBasePtr>& group =
      RobotConfigPool[(robot_id - 1) / RobotConfigPool.size()];
  if (group.empty()) return empty;
  return group[(robot_id - 1) % group.size()];
}

int32_t PatrolBase::GeneratePatrolAward(
    int32_t award_count, int32_t player_level, int32_t patrol_level,
    PatrolBase* base, VectorMap<int32_t, int32_t>& awards_temp) {

  if(!base) return sy::ERR_PARAM_INVALID;

  for (int32_t i = 0; i < award_count; ++i) {
    ModifyCurrency modify_temp(0, 0);
    AddSubItemSet item_set_temp;

    LootBase* loot = LootConfigFile::Get(base->item_award, player_level).get();
    if (!loot) return sy::ERR_PARAM_INVALID;
    loot->Loot(modify_temp, item_set_temp, NULL);

    if (RandomIn10000() < base->GetSkillAward(patrol_level)) {
      modify_temp *= 2;
      for (AddSubItemSet::iterator it = item_set_temp.begin();
           it != item_set_temp.end(); ++it)
        it->item_count *= 2;
    }
    for (AddSubItemSet::iterator it = item_set_temp.begin();
         it != item_set_temp.end(); ++it) {
      awards_temp[it->item_id] += it->item_count;
    }
    for (int32_t i = sy::MoneyKind_MIN; i < sy::MoneyKind_ARRAYSIZE; ++i) {
      if (modify_temp[i]) awards_temp[i] += modify_temp[i];
    }
  }
  return sy::ERR_OK;
}

int FateBase::RandomShipFateScale() {
  extern boost::mt19937 globalMt19937;
  boost::normal_distribution<> dis(GetSettingValue(fate_mean),
                                   GetSettingValue(fate_stddev));
  int32_t value = static_cast<int32_t>(dis(globalMt19937));
  if (value < GetSettingValue(fate_min)) value = GetSettingValue(fate_min);
  if (value > GetSettingValue(fate_max)) value = GetSettingValue(fate_max);
  return value;
}

bool VipFunctionBase::Fill(pugi::xml_node node) {
  __CHECK(viplv);
  __IGNORE(lv);
  DEBUG_LOG(logger)("load vip_function config, id:%ld", this->id());
  return true;
};

int32_t VipFunctionBase::GetValue(int32_t vip, int32_t level) const {
  if (this->lv.v2 && level >= this->lv.v1) return 1;
  if (viplv.empty()) return 0;
  int value = 0;
  for (ValuePair2Vec::const_iterator it = viplv.begin(); it != viplv.end();
       ++it) {
    if (vip < it->v1) break;
    value = it->v2;
  }
  return value;
}

VectorMap<int32_t, boost::normal_distribution<> > kCarrierCopyDis;

int32_t CarrierCopyBase::fight_attr(int32_t player_fight) const {
  if (kCarrierCopyDis.find(this->id()) == kCarrierCopyDis.end()) {
    return player_fight;
  }
  extern boost::mt19937 globalMt19937;
  int32_t percent = kCarrierCopyDis[this->id()](globalMt19937);
  int32_t count = 0;
  while ((percent < this->fc_min || percent > this->fc_max) && count < 10) {
    percent = kCarrierCopyDis[this->id()](globalMt19937);
    ++count;
  }
  percent = percent < this->fc_min ? this->fc_min : percent;
  percent = percent > this->fc_max ? this->fc_max : percent;
  return percent / 100.0 * player_fight;
}

int32_t CarrierCopyBase::level(int32_t player_level) const {
  return RandomBetween(-this->lvl_change, this->lvl_change) + player_level +
         lvl_base;
}

bool CarrierCopyBase::Fill(pugi::xml_node node) {
  __CHECK(fc_mean);
  __CHECK(fc_stddev);
  __CHECK(fc_min);
  __CHECK(fc_max);
  __CHECK(lvl_base);
  __CHECK(lvl_change);
  __CHECK(reward_a);
  __CHECK(reward_b);
  __CHECK(reward_fixed);

  if (kCarrierCopyDis.find(this->id()) == kCarrierCopyDis.end()) {
    kCarrierCopyDis[this->id()] = boost::normal_distribution<>(fc_mean, fc_stddev);
  }

  DEBUG_LOG(logger)("load carrier_copy, id:%ld", this->id());
  return true;
}

bool CarrierCopyAwardBase::Fill(pugi::xml_node node) {
  __CHECK(reward);

  DEBUG_LOG(logger)("load carrier_copy_award, id:%ld", this->id());
  return true;
}

bool CarrierCopyFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<CarrierCopyBase, ConfigEntryManager<CarrierCopyBase> >(
      "/config/carrier_fuben/item", "id");
  this->ParseTable<CarrierCopyAwardBase, ConfigEntryManager<CarrierCopyAwardBase> >(
      "/config/box/item", "id");
  return true;
}

std::vector<ChartBase*> ChartBase::charts;

bool ChartBase::Fill(pugi::xml_node node) {
  __CHECK(need);
  __CHECK(property);
  DEBUG_LOG(logger)("load chart config, id:%ld", this->id());

  charts.push_back(this);

  return true;
};

bool ChartConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;
  ChartBase::charts.clear();
  this->ParseTable<ChartBase, ConfigEntryManager<ChartBase> >(
      "/config/chart/item", "id");
  return true;
}

bool FundBase::Fill(pugi::xml_node node) {
  __CHECK(level);
  __CHECK(return1);

  DEBUG_LOG(logger)("load fund config, id:%ld", this->id());

  return true;
};

bool WelfareBase::Fill(pugi::xml_node node) {
  __CHECK(count);
  __CHECK(award);

  DEBUG_LOG(logger)("load welfare config, id:%ld", this->id());

  return true;
};

bool DailyAwardBase::Fill(pugi::xml_node node) {
  __CHECK(onsale);
  __CHECK(award);
  __CHECK(condition);
  __CHECK(drop_box);

  DEBUG_LOG(logger)("load daily_award config, id:%ld", this->id());

  return true;
};

bool VIPDailyAwardBase::Fill(pugi::xml_node node) {
  __CHECK(award);

  DEBUG_LOG(logger)("load vip_daily_award config, id:%ld", this->id());

  return true;
};

bool VIPWeeklyShopBase::Fill(pugi::xml_node node) {
  __CHECK(vip);
  __CHECK(lv);
  __CHECK(award);
  __CHECK(moneytype);
  __CHECK(price);
  __CHECK(available_times);

  DEBUG_LOG(logger)("load weekly_shop config, id:%ld", this->id());

  return true;
};

struct BreakAdvAttrKey {
  int32_t group_id;
  int32_t grade;
  int32_t level;
  BreakAdvAttrKey(int32_t group_id, int32_t grade, int32_t level)
      : group_id(group_id), grade(grade), level(level) {}
  bool operator == (const BreakAdvAttrKey& other) const{
    return this->group_id == other.group_id && this->grade == other.grade &&
           this->level == other.level;
  }
};

struct BreakAdvAttrKeyHash {
  size_t operator()(const BreakAdvAttrKey& key) const {
    size_t hash = 0;
    boost::hash_combine(hash, key.group_id);
    boost::hash_combine(hash, key.grade);
    boost::hash_combine(hash, key.level);
    return hash;
  }
};

typedef boost::unordered_map<BreakAdvAttrKey, BreakAdvAttr, BreakAdvAttrKeyHash> BreakAdvAttrCacheContianer;
static BreakAdvAttrCacheContianer gBreakAdvAttrCache;

static inline void CalcBreakAdvAttr(const BreakAdvAttrKey& key) {
  if (gBreakAdvAttrCache.find(key) != gBreakAdvAttrCache.end()) {
    return;
  }
  //这边计算一下属性,然后插入到gBreakAdvAttrCache里面去
  const BreakAdvancedBase* advanced_base =
      BREAK_ADVANCED.GetEntryByID(BreakAdvancedID(key.group_id, key.grade))
          .get();
  if (!advanced_base) {
    ERROR_LOG(logger)("%s BreakAdvanced:%d not found", __PRETTY_FUNCTION__, BreakAdvancedID(key.group_id, key.grade));
    return;
  }

  gBreakAdvAttrCache[key].attr[sy::ATTACK_ATTR_HP] = (advanced_base->base_attribute[0] + advanced_base->grow_attribute[0] * (key.level - 1) / 100);
  gBreakAdvAttrCache[key].attr[sy::ATTACK_ATTR_AP] = (advanced_base->base_attribute[1] + advanced_base->grow_attribute[1] * (key.level - 1) / 100);
  gBreakAdvAttrCache[key].attr[sy::ATTACK_ATTR_SP] = (advanced_base->base_attribute[1] + advanced_base->grow_attribute[1] * (key.level - 1) / 100);
  gBreakAdvAttrCache[key].attr[sy::ATTACK_ATTR_WF] = (advanced_base->base_attribute[2] + advanced_base->grow_attribute[2] * (key.level - 1) / 100);
  gBreakAdvAttrCache[key].attr[sy::ATTACK_ATTR_FF] = (advanced_base->base_attribute[3] + advanced_base->grow_attribute[3] * (key.level - 1) / 100);
}

BreakAdvAttr BreakAdvancedBase::GetBreakAdvAttr(int32_t group, int32_t grade,
                                                int32_t level) {
  static BreakAdvAttr kEmpty;
  BreakAdvAttrKey key(group, grade, level);
  CalcBreakAdvAttr(key);
  BreakAdvAttrCacheContianer::const_iterator iter = gBreakAdvAttrCache.find(key);
  return iter != gBreakAdvAttrCache.end() ? iter->second : kEmpty;
}

bool LeagueConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<LeagueBase, ConfigEntryManager<LeagueBase> >("/config/league/item", "id");
  this->ParseTable<LeagueSkillBase, ConfigEntryManager<LeagueSkillBase> >("/config/league_skill/item", "id");
  this->ParseTable<LeagueSkillCostBase,ConfigEntryManager<LeagueSkillCostBase> >("/config/league_skill_cost/item", "id");
  this->ParseTable<LeagueSignBase, ConfigEntryManager<LeagueSignBase> >("/config/league_sign/item", "id");
  this->ParseTable<ArmyBossBase, ConfigEntryManager<ArmyBossBase> >("/config/league_copy/item", "id");
  this->ParseTable<LeagueLevelBase, ConfigEntryManager<LeagueLevelBase> >("/config/league_level/item", "id");
  this->ParseTable<LeagueSignRewardBase, ConfigEntryManager<LeagueSignRewardBase> >("/config/league_sign_reward/item", "id");
  return true;
}

bool LeagueSkillBase::Fill(pugi::xml_node node) {
  __CHECK(cost_type);
  __CHECK(skill_max);
  __CHECK(need_league);
  __CHECK(attr);
  DEBUG_LOG(logger)("load league skill config, id:%ld", this->id());

  return true;
};

bool LeagueSkillCostBase::Fill(pugi::xml_node node) {
  __CHECK(cost_type1);
  __CHECK(cost_type2);
  __CHECK(cost_type3);
  __CHECK(cost_type4);
  DEBUG_LOG(logger)("load league skill cost config, id:%ld", this->id());

  return true;
};

bool LeagueSignBase::Fill(pugi::xml_node node) {
  __CHECK(cost);
  __CHECK(sign_progress);
  __CHECK(league_exp);
  __CHECK(league_currency);
  __CHECK(vip);
  DEBUG_LOG(logger)("load league sign config, id:%ld", this->id());

  return true;
};

bool LeagueBase::Fill(pugi::xml_node node) {
  __CHECK(exp);
  __CHECK(limit);
  DEBUG_LOG(logger)("load league config, id:%ld", this->id());

  return true;
};

bool LeagueLevelBase::Fill(pugi::xml_node node) {
  __CHECK(exp);
  __CHECK(step);
  __CHECK(need_lvl);

  DEBUG_LOG(logger)("load league level config, id:%ld", this->id());

  return true;
};

bool LeagueSignRewardBase::Fill(pugi::xml_node node) {
  __CHECK(reward);

  DEBUG_LOG(logger)("load league sign reward config, id:%ld", this->id());

  return true;
}

bool ArmyBossBase::Fill(pugi::xml_node node) {
  __CHECK(chapter);
  __CHECK(monster);
  __CHECK(country);
  __CHECK(reward_kill);
  __CHECK(reward_kill2);
  __CHECK(reward_challenge);
  __CHECK(reward_choose);
  __CHECK(reward_once);

  this->monster_base = MONSTER_GROUP_BASE.GetEntryByID(this->monster);
  if (!monster_base) {
    ERROR_LOG(logger)("load army boss base, id:%ld, monster:%d not found", this->id(), this->monster);
  }
  static std::pair<int32_t, int32_t> kCountryMap[] = {
    std::make_pair(sy::ATTACK_ATTR_UK_DAMAGE_DEC, 0),
    std::make_pair(sy::ATTACK_ATTR_UK_DAMAGE_DEC, -500),
    std::make_pair(sy::ATTACK_ATTR_US_DAMAGE_DEC, -500),
    std::make_pair(sy::ATTACK_ATTR_GE_DAMAGE_DEC, -500),
    std::make_pair(sy::ATTACK_ATTR_JP_DAMAGE_DEC, -500),
  };
  for (int32_t pos = 1; pos <= 6; ++pos) {
    AttrVec& vec = const_cast<AttrVec&>(this->monster_base->GetMonsterAttr(pos));
    if (this->country >= 0 && this->country < ArraySize(kCountryMap)) {
      int32_t attr_index = kCountryMap[this->country].first;
      int32_t attr_value = kCountryMap[this->country].second;
      vec.Set(attr_index, attr_value + vec.Get(attr_index));
    }
  }
  if (this->id() / 10 >= 127) {
    ERROR_LOG(logger)("ArmyWarBoss Out of bound");
  }

  INFO_LOG(logger)("load army boss base, id:%ld, monster:%d", this->id(), this->monster);
  return true;
}

VectorMap<std::string, RechargeBasePtr> kGoods2Recharge;
VectorMap<std::string, std::string> kGoodIDMap;
VectorMap<int32_t, std::pair<std::string, int32_t> > kGoodItemMap;

RechargeBasePtr GetRechargeByGoodsID(const std::string& goods_id) {
  static RechargeBasePtr empty;
  VectorMap<std::string, RechargeBasePtr>::iterator iter = kGoods2Recharge.find(goods_id);
  return iter != kGoods2Recharge.end() ? iter->second : empty;
}

bool RechargeBase::Fill(pugi::xml_node node) {
  __CHECK(price);
  __CHECK(extra_gold);
  __CHECK(index);
  __CHECK(goods_id);
  if (!this->goods_id.empty()) {
    kGoods2Recharge[this->goods_id] = boost::static_pointer_cast<RechargeBase>(this->shared_from_this());
  }

  DEBUG_LOG(logger)("load recharge, id:%ld, price:%d", this->id(), this->price);
  return true;
}

VectorMap<int32_t, SevenDayRaceBasePtr> kSevenDaysRace;

bool SevenDayRaceBase::Fill(pugi::xml_node node) {
  __CHECK(reward);

  kSevenDaysRace[this->id()] = boost::static_pointer_cast<SevenDayRaceBase>(this->shared_from_this());
  DEBUG_LOG(logger)("load sevenday race, id:%ld", this->id());
  return true;
}

SevenDayRaceBase* GetSevenDaysRace(int32_t rank) {
  VectorMap<int32_t, SevenDayRaceBasePtr>::iterator iter = kSevenDaysRace.upper_bound(rank);
  return (!kSevenDaysRace.empty() && iter != kSevenDaysRace.begin()) ? (iter - 1)->second.get() : NULL;
}

VectorMap<int32_t, LimitedRecruitRankBasePtr> kLimitedRank;
VectorMap<std::string, DailyRechargeItemBasePtr> kDailyRechargItem;

bool DiamondFundBase::Fill(pugi::xml_node node) {
  __IGNORE(buy_count);
  __CHECK(type);
  __CHECK(day);
  __CHECK(award);

  DEBUG_LOG(logger)("load diamond fund, id:%ld", this->id());
  return true;
}

bool DailyRechargeItemBase::Fill(pugi::xml_node node) {

  this->goods_id = node.attribute("goods_id").as_string("");
  __CHECK(drop_box);
  if (!this->goods_id.empty())
    kDailyRechargItem[this->goods_id] =
        boost::static_pointer_cast<DailyRechargeItemBase>(
            this->shared_from_this());

  DEBUG_LOG(logger)("load daily recharge item, id:%ld, %s", this->id(), this->goods_id.c_str());
  return true;
}

const DailyRechargeItemBase* GetDailyRechargeItemByGoodID(
    const std::string& goodid) {
  VectorMap<std::string, DailyRechargeItemBasePtr>::const_iterator iter = kDailyRechargItem.find(goodid);
  if (iter != kDailyRechargItem.end()) return iter->second.get();
  return NULL;
}

bool ServerOpenConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kGoods2Recharge.clear();
  kLimitedRank.clear();
  kGoodIDMap.clear();
  kDailyRechargItem.clear();
  kGoodItemMap.clear();

  this->ParseTable<FundBase, ConfigEntryManager<FundBase> >("/config/fund/item",
                                                            "id");
  this->ParseTable<WelfareBase, ConfigEntryManager<WelfareBase> >(
      "/config/welfare/item", "id");
  this->ParseTable<DailyAwardBase, ConfigEntryManager<DailyAwardBase> >(
      "/config/daily_award/item", "id");
  this->ParseTable<RechargeBase, ConfigEntryManager<RechargeBase> >(
      "/config/recharge/item", "id");
  this->ParseTable<VIPDailyAwardBase, ConfigEntryManager<VIPDailyAwardBase> >(
      "/config/vip_daily_award/item", "id");
  this->ParseTable<VIPWeeklyShopBase, ConfigEntryManager<VIPWeeklyShopBase> >(
      "/config/vip_weekly_shop/item", "id");
  this->ParseTable<SevenDayRaceBase, ConfigEntryManager<SevenDayRaceBase> >(
      "/config/sevenday_fc_race/item", "id");
  this->ParseTable<LimitedRecruitBoxBase, ConfigEntryManager<LimitedRecruitBoxBase> >(
      "/config/limited_recruit_box/item", "id");
  this->ParseTable<LimitedRecruitRankBase, ConfigEntryManager<LimitedRecruitRankBase> >(
      "/config/limited_recruit_rank/item", "id");
  this->ParseTable<DiamondFundBase, ConfigEntryManager<DiamondFundBase> >(
      "/config/diamond_fund/item", "id");
  this->ParseTable<DailyRechargeItemBase, ConfigEntryManager<DailyRechargeItemBase> >(
      "/config/daily_recharge_item/item", "id");
  {
    pugi::xpath_node_set nodes = doc.select_nodes("//item[@goodid]");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
      pugi::xml_node node = iter->node();

      const std::string& goodid = node.attribute("goodid").as_string();
      int32_t item_id = node.attribute("item_id").as_int();
      if (item_id)
        kGoodItemMap[item_id] =
            std::make_pair(goodid, node.attribute("price").as_int() * 10);
      for (pugi::xml_attribute_iterator i = node.attributes().begin();
           i != node.attributes().end(); ++i) {
        if (i->empty()) continue;
        std::string attr = i->name();
        std::string xianyu_goodid = i->value();
        if (attr == "goodid") continue;
        if (attr.find("goodid") != std::string::npos) {
          TRACE_LOG(logger)("xianyu_goodid:%s, goodid:%s"
            , xianyu_goodid.c_str(), goodid.c_str());
        }
        kGoodIDMap[xianyu_goodid] = goodid;
      }
    }
  }
  return true;
}

const std::string& GetMappedGoodID(const std::string& goodid) {
  VectorMap<std::string, std::string>::const_iterator iter = kGoodIDMap.find(goodid);
  return iter != kGoodIDMap.end() ? iter->second : goodid;
}

std::pair<std::string, int32_t> GetGoodIDByItemID(int32_t item_id) {
  VectorMap<int32_t, std::pair<std::string, int32_t> >::iterator iter =
      kGoodItemMap.find(item_id);
  if (iter != kGoodItemMap.end())
    return iter->second;
  else
    return std::make_pair("", 0);
}

struct WorldBossAttr {
  sy::CurrentCarrierInfo carrier;
  std::vector<sy::HeroInfo> heros;
};
static VectorMap<int32_t, WorldBossAttr> kWorldBossAttrCache;
static VectorMap<int32_t, WorldBossAttrBasePtr> kWorldBossMapAttr;

void WorldBossAttrBase::Clear() { kWorldBossAttrCache.clear(); }

void WorldBossAttrBase::FillMonsterInfos(sy::CurrentCarrierInfo& carrier,
                                         std::vector<sy::HeroInfo>& heros,
                                         int32_t level, int32_t monster_group) {
  VectorMap<int32_t, WorldBossAttr>::iterator cache =
      kWorldBossAttrCache.find(level);
  if (cache != kWorldBossAttrCache.end()) {
    carrier = cache->second.carrier;
    heros = cache->second.heros;
    return;
  }

  MonsterGroupBase* group =
      MONSTER_GROUP_BASE.GetEntryByID(monster_group).get();
  if (!group) return;
  WorldBossAttr& boss_attr = kWorldBossAttrCache[level];

  boss_attr.carrier = group->carrier_info();
  boss_attr.heros = group->hero_info();

  VectorMap<int32_t, WorldBossAttrBasePtr>::iterator attr_base =
      kWorldBossMapAttr.upper_bound(level);
  if (kWorldBossMapAttr.empty()) return;
  if (attr_base != kWorldBossMapAttr.begin()) --attr_base;

  for (std::vector<sy::HeroInfo>::iterator iter = boss_attr.heros.begin();
       iter != boss_attr.heros.end(); ++iter) {
    sy::HeroInfo& hero = *iter;
    google::protobuf::RepeatedField<int64_t>* attr = hero.mutable_attr1();
    attr->Resize(sy::AttackAttr_ARRAYSIZE, 0);

    for (ValuePair2VecInt64::iterator it = attr_base->second->monster_group.begin();
         it != attr_base->second->monster_group.end(); ++it)
      attr->Set(it->v1, it->v2);

    for (ValuePair2VecInt64::iterator it =
             attr_base->second->grow_monster_value.begin();
         it != attr_base->second->grow_monster_value.end(); ++it)
      attr->Set(it->v1, attr->Get(it->v1) + it->v2 * (level - 1));
  }

  carrier = boss_attr.carrier;
  heros = boss_attr.heros;
}

bool WorldBossAttrBase::Fill(pugi::xml_node node) {
  __CHECK(monster_group);
  __CHECK(grow_monster_value);

  kWorldBossMapAttr[this->id()] =
      boost::static_pointer_cast<WorldBossAttrBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load world boss attr config, id:%ld", this->id());

  return true;
};

bool WorldBossMeritAwardBase::Fill(pugi::xml_node node) {
  __CHECK(lv);
  __CHECK(index);
  __CHECK(merit_num);
  __CHECK(merit_award);

  DEBUG_LOG(logger)("load world boss merit award config, id:%ld", this->id());

  return true;
};

static VectorMap<int32_t, WorldBossKillAwardBasePtr> kWorldBossKillAward;

const WorldBossKillAwardBase* WorldBossKillAwardBase::GetKillAward(
    int32_t level) {
  VectorMap<int32_t, WorldBossKillAwardBasePtr>::iterator it =
      kWorldBossKillAward.upper_bound(level);
  if (it != kWorldBossKillAward.begin()) --it;
  return it->second.get();
}


bool WorldBossKillAwardBase::Fill(pugi::xml_node node) {
  __CHECK(merit_award);

  kWorldBossKillAward[this->id()] =
      boost::static_pointer_cast<WorldBossKillAwardBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load world boss kill award config, id:%ld", this->id());

  return true;
};

bool WorldBossRankAwardBase::Fill(pugi::xml_node node) {
  __CHECK(merit_rankaward);
  __CHECK(damage_rankaward);

  DEBUG_LOG(logger)("load world boss rank award  config, id:%ld", this->id());

  return true;
};

bool WorldBossLeagueRankAwardBase::Fill(pugi::xml_node node) {
  __CHECK(merit_rankaward);

  DEBUG_LOG(logger)("load world boss league rank award  config, id:%ld", this->id());

  return true;
};

bool WorldBossConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;
  kWorldBossMapAttr.clear();
  kWorldBossKillAward.clear();

  this->ParseTable<WorldBossAttrBase, ConfigEntryManager<WorldBossAttrBase> >(
      "/config/pirate_boss_attr/item", "id");
  this->ParseTable<WorldBossMeritAwardBase,
                   ConfigEntryManager<WorldBossMeritAwardBase> >(
      "/config/pirate_boss_merit_award/item", "id");
  this->ParseTable<WorldBossKillAwardBase,
                   ConfigEntryManager<WorldBossKillAwardBase> >(
      "/config/pirate_boss_kill_award/item", "id");
  this->ParseTable<WorldBossRankAwardBase,
                   ConfigEntryManager<WorldBossRankAwardBase> >(
      "/config/pirate_boss_rank_award/item", "id");
  this->ParseTable<WorldBossLeagueRankAwardBase,
                   ConfigEntryManager<WorldBossLeagueRankAwardBase> >(
      "/config/pirate_boss_league_rank_award/item", "id");

  return true;
}

static VectorMap<std::pair<int32_t, int32_t>, AttackAttrArray> kWakeAttr;
static VectorMap<std::pair<int32_t, int32_t>, VectorMap<int32_t, int32_t> > kWakeItem;

bool WakeItemBase::Fill(pugi::xml_node node) {
  __CHECK(attr);
  __CHECK(cost);
  __CHECK(cost_money);

  DEBUG_LOG(logger)("load wake item config, id:%ld", this->id());

  return true;
};

bool WakeTalentBase::Fill(pugi::xml_node node) {
  __CHECK(attr);

  DEBUG_LOG(logger)("load wake talent config, id:%ld", this->id());

  return true;
};

bool WakeConditionBase::Fill(pugi::xml_node node) {
  __CHECK(need_lv);
  __CHECK(need_hero);
  __CHECK(need_item);
  __CHECK(need_coin);
  __CHECK(itemtree1);
  __CHECK(itemtree2);
  __CHECK(itemtree3);
  __CHECK(itemtree4);
  __CHECK(itemtree5);
  __CHECK(itemtree6);
  __CHECK(itemtree7);
  __CHECK(itemtree8);
  __CHECK(itemtree9);
  __CHECK(itemtree10);
  __CHECK(itemtree11);

  DEBUG_LOG(logger)("load wake condition config, id:%ld", this->id());

  return true;
};

const std::vector<int32_t>* GetWakeItemTree(int32_t type,
                                            WakeConditionBase* wake_condition) {
  if (!wake_condition) return NULL;
  const std::vector<int32_t>* item_tree = NULL;
  switch (type) {
    case 1:
      item_tree = &wake_condition->itemtree1;
      break;
    case 2:
      item_tree = &wake_condition->itemtree2;
      break;
    case 3:
      item_tree = &wake_condition->itemtree3;
      break;
    case 4:
      item_tree = &wake_condition->itemtree4;
      break;
    case 5:
      item_tree = &wake_condition->itemtree5;
      break;
    case 6:
      item_tree = &wake_condition->itemtree6;
      break;
    case 7:
      item_tree = &wake_condition->itemtree7;
      break;
    case 8:
      item_tree = &wake_condition->itemtree8;
      break;
    case 9:
      item_tree = &wake_condition->itemtree9;
      break;
    case 10:
      item_tree = &wake_condition->itemtree10;
      break;
    case 11:
      item_tree = &wake_condition->itemtree11;
      break;
  }
  return item_tree;
}

void CalcWakeAttr() {
  int level = 0;
  while (true) {
    WakeConditionBase* wake_condition =
        WAKE_CONDITION_BASE.GetEntryByID(level - 1).get();
    if (level > 0 && !wake_condition) break;

    for (int32_t i = 1; i <= 11; i++) {
      kWakeAttr[std::make_pair(level, i)].resize(sy::AttackAttr_ARRAYSIZE, 0);
      WakeTalentBase* wake_talent = WAKE_TALENT_BASE.GetEntryByID(level).get();
      if (wake_talent) {
        for (ValuePair2Vec::iterator it = wake_talent->attr.begin();
             it != wake_talent->attr.end(); ++it)
          kWakeAttr[std::make_pair(level, i)][it->v1] += it->v2;
      }
      if (!wake_condition) continue;

      const std::vector<int32_t>* item_tree =
          GetWakeItemTree(i, wake_condition);
      if (!item_tree) continue;

      AttackAttrArray& prev_attr = kWakeAttr[std::make_pair(level - 1, i)];
      for (size_t j = 0; j < prev_attr.size(); j++)
        kWakeAttr[std::make_pair(level, i)][j] += prev_attr[j];

      kWakeItem[std::make_pair(level, i)];
      VectorMap<int32_t, int32_t>& prev_items =
          kWakeItem[std::make_pair(level - 1, i)];
      for (VectorMap<int32_t, int32_t>::iterator it = prev_items.begin();
           it != prev_items.end(); ++it)
        kWakeItem[std::make_pair(level, i)][it->first] += it->second;

      for (size_t j = 0; j < item_tree->size(); j++) {
        WakeItemBase* item_base =
            WAKE_ITEM_BASE.GetEntryByID(item_tree->at(j)).get();
        if (!item_base) {
          ERROR_LOG(logger)("wake item not found id:%d", item_tree->at(j));
          continue;
        }
        for (ValuePair2Vec::iterator it = item_base->attr.begin();
             it != item_base->attr.end(); ++it)
          kWakeAttr[std::make_pair(level, i)][it->v1] += it->v2;
        kWakeItem[std::make_pair(level, i)][item_base->id()]++;
      }
    }
    level++;
  }
}

bool WakeConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kWakeAttr.clear();
  kWakeItem.clear();

  this->ParseTable<WakeItemBase, ConfigEntryManager<WakeItemBase> >(
      "/config/wake_item/item", "id");

  this->ParseTable<WakeTalentBase, ConfigEntryManager<WakeTalentBase> >(
      "/config/wake_talent/item", "id");

  this->ParseTable<WakeConditionBase, ConfigEntryManager<WakeConditionBase> >(
      "/config/wake_condition/item", "id");

  CalcWakeAttr();

  return true;
}

VectorMap<int32_t, std::vector<int32_t> > kLegionPersonalAward;
VectorMap<int32_t, std::vector<int32_t> > kLegionArmyScore;

bool LegionCityBase::Fill(pugi::xml_node node) {
  __CHECK(nearby);
  __CHECK(quality);

  DEBUG_LOG(logger)("load legion city, id:%ld", this->id());
  return true;
}

bool LegionScoreBase::Fill(pugi::xml_node node) {
  int32_t quality6, quality5, quality4, quality3, quality2;
  int32_t award6, award5, award4, award3, award2;
  __CHECK(quality6);
  __CHECK(quality5);
  __CHECK(quality4);
  __CHECK(quality3);
  __CHECK(quality2);
  __CHECK(award6);
  __CHECK(award5);
  __CHECK(award4);
  __CHECK(award3);
  __CHECK(award2);
  std::pair<int32_t, int32_t> army_score[] = {
    std::make_pair(quality6, 6),
    std::make_pair(quality5, 5),
    std::make_pair(quality4, 4),
    std::make_pair(quality3, 3),
    std::make_pair(quality2, 2),
  };
  std::pair<int32_t, int32_t> personal_award[] = {
    std::make_pair(award6, 6),
    std::make_pair(award5, 5),
    std::make_pair(award4, 4),
    std::make_pair(award3, 3),
    std::make_pair(award2, 2),
  };
  for (int32_t i = 0; i < ArraySize(army_score); ++i) {
    if (!army_score[i].first) continue;
    std::vector<int32_t>& array = kLegionArmyScore[army_score[i].second];
    if (array.size() < (size_t)this->id()) array.resize(this->id(), 0);
    array[this->id() - 1] = army_score[i].first;
  }

  for (int32_t i = 0; i < ArraySize(personal_award); ++i) {
    if (!personal_award[i].first) continue;
    std::vector<int32_t>& array = kLegionPersonalAward[personal_award[i].second];
    if (array.size() < (size_t)this->id()) array.resize(this->id(), 0);
    array[this->id() - 1] = personal_award[i].first;
  }

  DEBUG_LOG(logger)("load legion map score, id:%ld", this->id());
  return true;
}

const std::vector<int32_t>& GetLegionPersonalAward(int32_t quality) {
  static const std::vector<int32_t> kEmpty;
  VectorMap<int32_t, std::vector<int32_t> >::const_iterator iter =
      kLegionPersonalAward.find(quality);
  return iter != kLegionPersonalAward.end() ? iter->second : kEmpty;
}

const std::vector<int32_t>& GetLegionArmyScore(int32_t quality) {
  static const std::vector<int32_t> kEmpty;
  VectorMap<int32_t, std::vector<int32_t> >::const_iterator iter =
      kLegionArmyScore.find(quality);
  return iter != kLegionArmyScore.end() ? iter->second : kEmpty;
}

bool LegionPlayerRewardBase::Fill(pugi::xml_node node) {
  __CHECK(award);
  DEBUG_LOG(logger)("load legion player reward, id:%ld", this->id());
  return true;
}

bool LegionArmyRewardBase::Fill(pugi::xml_node node) {
  __CHECK(award);
  DEBUG_LOG(logger)("load legion army reward, id:%ld", this->id());
  return true;
}

bool LegionAwardBase::Fill(pugi::xml_node node) {
  __CHECK(money);
  __CHECK(rank);

  DEBUG_LOG(logger)("load legion award config, id:%ld", this->id());
  return true;
}

bool LegionDailyRewardBase::Fill(pugi::xml_node node) {
  __CHECK(times);
  __CHECK(money);

  DEBUG_LOG(logger)("load legion daily award config, id:%ld", this->id());
  return true;
}

bool LegionForeplayCopyBase::Fill(pugi::xml_node node) {
  __CHECK(monster);
  __CHECK(merit_award);
  __CHECK(server_award);

  DEBUG_LOG(logger)("load legion foreplay copy config, id:%ld", this->id());
  return true;
}


bool LegionForeplayAwardBase::Fill(pugi::xml_node node) {
  __CHECK(merit_award);
  __CHECK(merit_num);

  DEBUG_LOG(logger)("load legion foreplay award config, id:%ld", this->id());
  return true;
}

VectorMap<int32_t, int32_t> kLegionForeplayAward;
int32_t LegionForeplayAward1Base::GetAward(int32_t day) {
  if (kLegionForeplayAward.empty()) return 0;
  VectorMap<int32_t, int32_t>::iterator it =
      kLegionForeplayAward.upper_bound(day);
  if (it == kLegionForeplayAward.begin()) return 0;
  --it;
  return it->second;
}

bool LegionForeplayAward1Base::Fill(pugi::xml_node node) {
  __CHECK(money);

  kLegionForeplayAward[this->id()] = money;

  DEBUG_LOG(logger)("load legion foreplay award1 config, id:%ld", this->id());
  return true;
}

bool LegionForeplayRankBase::Fill(pugi::xml_node node) {
  __CHECK(damage_rankaward);

  DEBUG_LOG(logger)("load legion foreplay rank config, id:%ld", this->id());
  return true;
}

VectorMap<int32_t, LegionWarTargetBasePtr> kLegionWarTargetLevelMap;

LegionWarTargetBasePtr GetLegionWarTargetBaseByLevel(int32_t level) {
  static LegionWarTargetBasePtr empty;
  if (kLegionWarTargetLevelMap.empty()) return empty;
  VectorMap<int32_t, LegionWarTargetBasePtr>::iterator iter = kLegionWarTargetLevelMap.upper_bound(level);
  return !kLegionWarTargetLevelMap.empty() &&
                 iter != kLegionWarTargetLevelMap.begin()
             ? (iter - 1)->second
             : empty;
}

bool LegionWarTargetBase::Fill(pugi::xml_node node) {
  __CHECK(lvl_base);
  __CHECK(lvl_change);
  __CHECK(reward_a);
  __CHECK(reward_b);
  __CHECK(reward_c);
  __CHECK(reward_d);
  kLegionWarTargetLevelMap[this->id()] = boost::static_pointer_cast<LegionWarTargetBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load legion war target base, id:%ld", this->id());
  return true;
}

bool LegionConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kLegionPersonalAward.clear();
  kLegionArmyScore.clear();
  kLegionWarTargetLevelMap.clear();
  kLegionForeplayAward.clear();

  this->ParseTable<LegionCityBase, ConfigEntryManager<LegionCityBase> >(
      "/config/legion_map/item", "id");
  this->ParseTable<LegionScoreBase, ConfigEntryManager<LegionScoreBase> >(
      "/config/legion_map_score/item", "id");
  this->ParseTable<LegionAwardBase, ConfigEntryManager<LegionAwardBase> >(
      "/config/legion_award_all/item", "id");
  this->ParseTable<LegionPlayerRewardBase, ConfigEntryManager<LegionPlayerRewardBase> >(
      "/config/legion_rank_award1/item", "id");
  this->ParseTable<LegionArmyRewardBase, ConfigEntryManager<LegionArmyRewardBase> >(
      "/config/legion_rank_award2/item", "id");
  this->ParseTable<LegionDailyRewardBase, ConfigEntryManager<LegionDailyRewardBase> >(
      "/config/legion_fight_reward/item", "id");
  this->ParseTable<LegionWarTargetBase, ConfigEntryManager<LegionWarTargetBase> >(
      "/config/legion_outlaw_reward/item", "id");
  this->ParseTable<LegionForeplayCopyBase,
                   ConfigEntryManager<LegionForeplayCopyBase> >(
      "/config/legion_foreplay_copy/item", "id");
  this->ParseTable<LegionForeplayAwardBase,
                   ConfigEntryManager<LegionForeplayAwardBase> >(
      "/config/legion_foreplay_award/item", "id");
  this->ParseTable<LegionForeplayAward1Base,
                   ConfigEntryManager<LegionForeplayAward1Base> >(
      "/config/legion_foreplay_award1/item", "id");
  this->ParseTable<LegionForeplayRankBase,
                   ConfigEntryManager<LegionForeplayRankBase> >(
      "/config/legion_foreplay_rank/item", "id");

  return true;
}

bool AddWakeAttr(int32_t level, int32_t type,
                 Array<int64_t, sy::AttackAttr_ARRAYSIZE>& attr) {
  VectorMap<std::pair<int32_t, int32_t>, AttackAttrArray>::iterator it =
      kWakeAttr.find(std::make_pair(level, type));
  if (it == kWakeAttr.end()) return false;

  for (size_t i = 0; i < it->second.size(); i++) attr[i] += it->second[i];

  return true;
}

const VectorMap<int32_t, int32_t>* GetUsedWakeItem(int32_t level,
                                                   int32_t type) {
  VectorMap<std::pair<int32_t, int32_t>, VectorMap<int32_t, int32_t> >::iterator
      it = kWakeItem.find(std::make_pair(level, type));
  if (it == kWakeItem.end()) return NULL;
  return &it->second;
}

VectorMap<int32_t, boost::shared_ptr<EliteCopyRefreshBase> > kEliteCopyRefreshLimit;

bool EliteCopyRefreshBase::Fill(pugi::xml_node node) {
  int32_t refresh1, refresh2, refresh3;
  __CHECK(refresh1);
  __CHECK(refresh2);
  __CHECK(refresh3);
  this->hour6 = ValuePair2<int32_t, int32_t>(refresh1, refresh1);
  this->hour12 = ValuePair2<int32_t, int32_t>(refresh2, refresh1 + refresh2);
  this->hour18 = ValuePair2<int32_t, int32_t>(refresh3, refresh1 + refresh2 + refresh3);
  kEliteCopyRefreshLimit[this->id()] = boost::static_pointer_cast<EliteCopyRefreshBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load elite copy refresh, id:%ld", this->id());
  return true;
}

bool EliteCopyBase::Fill(pugi::xml_node node) {
  __CHECK(power);
  __CHECK(item1);
  __CHECK(item2);
  __CHECK(monster);
  if (!MONSTER_GROUP_BASE.GetEntryByID(this->monster)) {
    ERROR_LOG(logger)("load elite copy, monster group:%d not found", this->monster);
  }

  DEBUG_LOG(logger)("load elite copy, id:%ld", this->id());
  return true;
}

const EliteCopyRefreshBase* GetEliteCopyRefreshByLevel(int32_t level) {
  VectorMap<int32_t, boost::shared_ptr<EliteCopyRefreshBase> >::const_iterator
      iter = kEliteCopyRefreshLimit.upper_bound(level);
  return iter != kEliteCopyRefreshLimit.begin() ? (iter - 1)->second.get() : NULL;
}

bool EliteCopyConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;
  kEliteCopyRefreshLimit.clear();

  this->ParseTable<EliteCopyRefreshBase, ConfigEntryManager<EliteCopyRefreshBase> >(
      "/config/elitecopy_refresh/item", "id");
  this->ParseTable<EliteCopyBase, ConfigEntryManager<EliteCopyBase> >(
      "/config/elitecopy_copy/item", "id");

  return true;
}

static VectorMap<int32_t, VectorMap<int32_t, AttackAttrArray> >
    kRedGoldAttrGroup;

bool RedGoldSkillBase::Fill(pugi::xml_node node) {
  __CHECK(attr);
  __CHECK(equip_id);
  __CHECK(grade_level);
  __CHECK(gold_level);

  DEBUG_LOG(logger)("load red gold skill, id:%ld", this->id());

  int32_t group_id = equip_id * 10 + (grade_level ? 0 : 1);
  int32_t level = grade_level ? grade_level : gold_level;
  VectorMap<int32_t, AttackAttrArray>& it_attr = kRedGoldAttrGroup[group_id];
  for (ValuePair2Vec::iterator it = attr.begin(); it != attr.end(); ++it) {
    it_attr[level].resize(sy::AttackAttr_ARRAYSIZE, 0);
    it_attr[level][it->v1] += it->v2;
  }
  return true;
}

bool RedGoldConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kRedGoldAttrGroup.clear();
  this->ParseTable<RedGoldSkillBase, ConfigEntryManager<RedGoldSkillBase> >(
      "/config/red_gold_skill/item", "id");

  for (VectorMap<int32_t, VectorMap<int32_t, AttackAttrArray> >::iterator
           it_group = kRedGoldAttrGroup.begin();
       it_group != kRedGoldAttrGroup.end(); ++it_group)
    Accumulation(it_group->second);

  return true;
}

void RedGoldSkillBase::AddRedGoldAttr(
    int32_t equip_id, int32_t refine_level, int32_t gold_level,
    Array<int64_t, sy::AttackAttr_ARRAYSIZE>& attr) {
  if (refine_level)
    AddExtraAttr(equip_id * 10, refine_level, kRedGoldAttrGroup, attr);
  if (gold_level)
    AddExtraAttr(equip_id * 10 + 1, refine_level, kRedGoldAttrGroup, attr);
}


bool LimitedRecruitRankBase::Fill(pugi::xml_node node) {
  __CHECK(range1);
  __CHECK(range2);
  __CHECK(award);

  kLimitedRank[range1] = boost::static_pointer_cast<LimitedRecruitRankBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load recruit rank config, id:%ld", this->id());

  return true;
};

const ValuePair2Vec* LimitedRecruitRankBase::GetAwardByRank(int32_t rank) {
  if (kLimitedRank.empty()) return NULL;
  VectorMap<int32_t, LimitedRecruitRankBasePtr>::iterator it =
      kLimitedRank.upper_bound(rank);
  if (it == kLimitedRank.begin()) return NULL;
  --it;
  return &it->second->award;
}

bool LimitedRecruitBoxBase::Fill(pugi::xml_node node) {
  __CHECK(score);
  __CHECK(award);

  DEBUG_LOG(logger)("load recruit box item config, id:%ld", this->id());

  return true;
};

bool CrossServerPK1WinAwardBase::Fill(pugi::xml_node node) {
  __CHECK(win);
  __CHECK(award);

  DEBUG_LOG(logger)("load cross server pk1 win award config, id:%ld", this->id());

  return true;
};

static VectorMap<int32_t, CrossServerPK1RankAwardBasePtr> kCrossServerRank;
bool CrossServerPK1RankAwardBase::Fill(pugi::xml_node node) {
  __CHECK(range1);
  __CHECK(range2);
  __CHECK(rankaward);

  kCrossServerRank[range1] =
      boost::static_pointer_cast<CrossServerPK1RankAwardBase>(
          this->shared_from_this());
  DEBUG_LOG(logger)("load cross server pk1 rank award config, id:%ld", this->id());

  return true;
};

bool CrossServerPK1RollPlayerBase::Fill(pugi::xml_node node) {
  __CHECK(fc_mean);
  __CHECK(fc_stddev);
  __CHECK(fc_min);
  __CHECK(fc_max);
  __CHECK(lvl_base);
  __CHECK(lvl_change);
  __CHECK(reward_a);
  __CHECK(reward_b);

  DEBUG_LOG(logger)("load cross server pk1 roll player config, id:%ld", this->id());

  return true;
};

bool CrossServerConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kCrossServerRank.clear();
  this->ParseTable<CrossServerPK1WinAwardBase,
                   ConfigEntryManager<CrossServerPK1WinAwardBase> >(
      "/config/crossserverpk1_win_award/item", "id");
  this->ParseTable<CrossServerPK1RankAwardBase,
                   ConfigEntryManager<CrossServerPK1RankAwardBase> >(
      "/config/crossserverpk1_rank_award/item", "id");
  this->ParseTable<CrossServerPK1RollPlayerBase,
                   ConfigEntryManager<CrossServerPK1RollPlayerBase> >(
      "/config/crossserverpk1_roll_player/item", "id");

  return true;
}

int32_t CrossServerPK1RankAwardBase::GetAwardBaseByRank(int32_t rank) {
  if (kCrossServerRank.empty()) return 0;
  VectorMap<int32_t, CrossServerPK1RankAwardBasePtr>::iterator it =
      kCrossServerRank.upper_bound(rank);
  if (it == kCrossServerRank.begin()) return 0;
  --it;
  return it->second->rankaward;
}

bool ActivityInfoBase::Fill(pugi::xml_node node) {
  __CHECK(time);
  __CHECK(cost1);
  __CHECK(cost10);
  __CHECK(item);

  DEBUG_LOG(logger)("load activity info base, id:%ld", this->id());
  return true;
}

ValuePair2<int32_t, int32_t> ActivityInfoBase::IsTodayInActivty(
    int32_t delta_days) const {
  int32_t server_start_days = server->GetServerStartDays() + delta_days;
  for (size_t index = 0; index < this->time.size(); index++) {
    if (server_start_days >= this->time[index].v1 &&
        server_start_days < this->time[index].v1 + this->time[index].v2) {
      return this->time[index];
    }
  }
  return ValuePair2<int32_t, int32_t>(0, 0);
}

VectorMap<int32_t, std::vector<int32_t> > kActivityCountMap;

int32_t GetActivityNextCount(int32_t type, int32_t current_count) {
  std::vector<int32_t>& vec = kActivityCountMap[type];
  for (std::vector<int32_t>::const_iterator iter = vec.begin();
       iter != vec.end(); ++iter) {
    if (current_count < *iter) return *iter;
  }
  return 0;
}

bool ActivityCountBase::Fill(pugi::xml_node node) {
  __CHECK(activity_type)
  __CHECK(count);
  __CHECK(award);
  kActivityCountMap[activity_type].push_back(count);

  DEBUG_LOG(logger)("load activity count base, id:%ld", this->id());
  return true;
}

bool ActivityEquipBase::Fill(pugi::xml_node node) {
  __CHECK(score);
  __CHECK(award);

  DEBUG_LOG(logger)("load activity equip base, id:%ld", this->id());
  return true;
}

VectorMap<int32_t, ActivityCarrierBasePtr> kActivityCarrier;

bool ActivityCarrierBase::Fill(pugi::xml_node node) {
  __CHECK(reward1);
  __CHECK(reward2);

  kActivityCarrier[this->id()] =
      boost::static_pointer_cast<ActivityCarrierBase>(this->shared_from_this());
  DEBUG_LOG(logger)("load activity carrier base, id:%ld", this->id());
  return true;
}

std::vector<VectorSet<int32_t> > kSweepStakeRandomPool;

bool IsSweepStakeCountLucky(int64_t player_id, int32_t count) {
  if (kSweepStakeRandomPool.empty()) {
    for (int32_t index = 0; index < 10; ++index) {
      VectorSet<int32_t> set;
      for (int32_t count = 11; count < 300; count += 10) {
        set.insert(count + RandomBetween(0, 9));
      }
      set.insert(2);
      set.insert(7);
      kSweepStakeRandomPool.push_back(set);
    }
  }
  int32_t index = player_id % kSweepStakeRandomPool.size();
  return kSweepStakeRandomPool[index].find(count) !=
         kSweepStakeRandomPool[index].end();
}

bool SweepStakeConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kActivityCarrier.clear();
  kActivityCountMap.clear();
  this->ParseTable<ActivityInfoBase, ConfigEntryManager<ActivityInfoBase> >(
      "/config/activity_info/item", "id");
  this->ParseTable<ActivityEquipBase, ConfigEntryManager<ActivityEquipBase> >(
      "/config/activity_equip/item", "id");
  this->ParseTable<ActivityCountBase, ConfigEntryManager<ActivityCountBase> >(
      "/config/activity_count/item", "id");
  this->ParseTable<ActivityCarrierBase, ConfigEntryManager<ActivityCarrierBase> >(
      "/config/activity_carrier/item", "id");

  return true;
}

const ActivityCarrierBase* GetSweepStakeCarrier(int32_t rank) {
  VectorMap<int32_t, ActivityCarrierBasePtr>::const_iterator it =
      kActivityCarrier.upper_bound(rank);
  if (it == kActivityCarrier.begin()) return NULL;
  return (it - 1)->second.get();
}

VectorMap<int32_t, std::vector<std::string> > kUpdateVersion;
bool UpdateVersionConfigFile::Parse() {
  kUpdateVersion.clear();
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;
  pugi::xpath_node_set nodes = doc.select_nodes("/config/version/item");
  for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
       iter != nodes.end(); ++iter) {
    pugi::xml_node node = iter->node();
    for (pugi::xml_attribute_iterator i = node.attributes().begin();
         i != node.attributes().end(); ++i) {
      if (i->empty()) continue;
      std::string attr = i->name();
      std::string val = i->value();
      if (attr.find("version") != std::string::npos) {
        int32_t key = atoi(attr.c_str() + 7);
        kUpdateVersion[key].push_back(val);
        TRACE_LOG(logger)("%d:%s", key, val.c_str());
      }
    }
  }
  return true;
}

bool CanGetUpdateVersionAward(int32_t package_id,
                              const std::string& cur_version,
                              const std::string& got_version) {
  VectorMap<int32_t, std::vector<std::string> >::iterator it =
      kUpdateVersion.find(package_id);
  if (it == kUpdateVersion.end()) return false;

  int32_t cur_index = -1;
  int32_t got_index = -1;

  for (size_t i = 0; i < it->second.size(); i++) {
    if (cur_version == it->second[i]) cur_index = i;
    if (got_version == it->second[i]) got_index = i;
  }
  return cur_index > got_index;
}

bool MedalOpenBase::Fill(pugi::xml_node node) {
  __CHECK(level);
  __CHECK(open_limit);

  DEBUG_LOG(logger)("load MedalOpen base, id:%ld", this->id());
  return true;
}

bool MedalChartBase::Fill(pugi::xml_node node) {
  __CHECK(chapter);
  __CHECK(cost);

  DEBUG_LOG(logger)("load MedalChart base, id:%ld", this->id());
  return true;
}

bool MedalChart2Base::Fill(pugi::xml_node node) {
  __CHECK(star);
  __CHECK(attr);

  DEBUG_LOG(logger)("load MedalChart2 base, id:%ld", this->id());
  return true;
}

static VectorMap<int32_t, MedalFormationBasePtr> kFormationMap;

bool MedalFormationBase::Fill(pugi::xml_node node) {
  __CHECK(attr);

  kFormationMap[this->id()] =
      boost::static_pointer_cast<MedalFormationBase>(this->shared_from_this());

  DEBUG_LOG(logger)("load MedalFormation base, id:%ld", this->id());
  return true;
}

const MedalFormationBase* MedalFormationBase::GetAttrByStar(int32_t star) {
  if (kFormationMap.empty()) return NULL;
  VectorMap<int32_t, MedalFormationBasePtr>::iterator it =
      kFormationMap.upper_bound(star);
  if (it == kFormationMap.begin()) return NULL;
  return (--it)->second.get();
}

bool MedalCopyBase::Fill(pugi::xml_node node) {
  __CHECK(quality);
  __CHECK(monster);
  __CHECK(reward);

  DEBUG_LOG(logger)("load MedalCopy base, id:%ld", this->id());
  return true;
}

bool MedalConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kFormationMap.clear();

  this->ParseTable<MedalOpenBase, ConfigEntryManager<MedalOpenBase> >(
      "/config/medal_open/item", "id");
  this->ParseTable<MedalChartBase, ConfigEntryManager<MedalChartBase> >(
      "/config/medal_chart/item", "id");
  this->ParseTable<MedalChart2Base, ConfigEntryManager<MedalChart2Base> >(
      "/config/medal_chart2/item", "id");
  this->ParseTable<MedalFormationBase, ConfigEntryManager<MedalFormationBase> >(
      "/config/medal_formation/item", "id");
  this->ParseTable<MedalCopyBase, ConfigEntryManager<MedalCopyBase> >(
      "/config/medal_copy/item", "id");

  return true;
}

bool RedequipRisestarBase::Fill(pugi::xml_node node) {
  __CHECK(equip_id);
  __CHECK(star);
  __CHECK(attr_type);
  __CHECK(attr_max);
  __CHECK(attr1);

  DEBUG_LOG(logger)("load RedequipRisestar base, id:%ld", this->id());
  return true;
}

bool RedequipCostBase::Fill(pugi::xml_node node) {
  __CHECK(exp);
  __CHECK(exp1);
  __CHECK(exp2);
  __CHECK(exp3);
  __CHECK(cost1);
  __CHECK(cost2);
  __CHECK(cost3);
  __CHECK(rate1);
  __CHECK(rate2);

  DEBUG_LOG(logger)("load RedequipCost base, id:%ld", this->id());
  return true;
}

bool RedEquipConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<RedequipRisestarBase,
                   ConfigEntryManager<RedequipRisestarBase> >(
      "/config/redequip_risestar/item", "id");
  this->ParseTable<RedequipCostBase, ConfigEntryManager<RedequipCostBase> >(
      "/config/redequip_cost/item", "id");

  return true;
}


bool IsInLegionWarOpenDays() {
  const std::vector<int32_t>& open_days =
      Setting::GetValue1(Setting::legion_war_region_time1);
  const struct tm& t = GetTime();
  if (std::find(open_days.begin(), open_days.end(), t.tm_wday) !=
      open_days.end())
    return true;
  return false;
}

bool HeroreturnBase::Fill(pugi::xml_node node) {
  __CHECK(award);

  DEBUG_LOG(logger)("load Heroreturn base, id:%ld", this->id());
  return true;
}

bool HeroreturnRechargeBase::Fill(pugi::xml_node node) {
  __CHECK(type);
  __CHECK(condition);
  __CHECK(award);

  DEBUG_LOG(logger)("load HeroreturnRecharge base, id:%ld", this->id());
  return true;
}

bool HeroComeBackConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<HeroreturnBase, ConfigEntryManager<HeroreturnBase> >(
      "/config/heroreturn/item", "id");
  this->ParseTable<HeroreturnRechargeBase,
                   ConfigEntryManager<HeroreturnRechargeBase> >(
      "/config/heroreturn_recharge/item", "id");

  return true;
}

bool PearlharborCopyBase::Fill(pugi::xml_node node) {
  __CHECK(chapter);
  __CHECK(reward_challenge);
  __CHECK(reward_kill2);
  __CHECK(monster_num);
  __CHECK(monster_image);

  DEBUG_LOG(logger)("load PearlharborCopy base, id:%ld", this->id());
  return true;
}

std::vector<int32_t> PearlharborCopyBase::RandomGroupMonster() {
  std::vector<int32_t> out;
  std::vector<int32_t> v = monster_image;
  int32_t random_count = monster_num;
  while (random_count > 0) {
    int32_t index = RandomBetween(0, v.size() - 1);
    if (index < 0 || index >= int32_t(v.size())) break;
    out.push_back(monster_image[index]);
    v.erase(v.begin() + index);
    --random_count;
  }
  return out;
}

bool PearlharborBreakRewardBase::Fill(pugi::xml_node node) {
  __CHECK(chapter);
  __CHECK(quality);
  __CHECK(reward);

  DEBUG_LOG(logger)("load PearlharborBreakReward base, id:%ld", this->id());
  return true;
}

bool PearlharborRankPersonalawardBase::Fill(pugi::xml_node node) {
  __CHECK(award);

  DEBUG_LOG(logger)("load PearlharborRankPersonalaward base, id:%ld", this->id());
  return true;
}

bool PearlharborRankLeagueawardBase::Fill(pugi::xml_node node) {
  __CHECK(award);

  DEBUG_LOG(logger)("load PearlharborRankLeagueaward base, id:%ld", this->id());
  return true;
}

bool PearlHarborConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<PearlharborCopyBase,
                   ConfigEntryManager<PearlharborCopyBase> >(
      "/config/pearlharbor_copy/item", "id");
  this->ParseTable<PearlharborBreakRewardBase,
                   ConfigEntryManager<PearlharborBreakRewardBase> >(
      "/config/pearlharbor_break_reward/item", "id");
  this->ParseTable<PearlharborRankPersonalawardBase,
                   ConfigEntryManager<PearlharborRankPersonalawardBase> >(
      "/config/pearlharbor_rank_personalaward/item", "id");
  this->ParseTable<PearlharborRankLeagueawardBase,
                   ConfigEntryManager<PearlharborRankLeagueawardBase> >(
      "/config/pearlharbor_rank_leagueaward/item", "id");

  return true;
}

static VectorMap<int32_t, AcitivityContentBase> kFileActivity;

const AcitivityContentBase* GetActivityByID(int32_t id) {
  VectorMap<int32_t, AcitivityContentBase>::iterator iter = kFileActivity.find(id);
  return iter != kFileActivity.end() ? &iter->second : NULL;
}

static inline void ConvertRawStringToInteralFormat() {
  for (VectorMap<int32_t, AcitivityContentBase>::iterator iter =
           kFileActivity.begin();
       iter != kFileActivity.end(); ++iter) {
    AcitivityContentBase* ptr = &iter->second;

    int32_t index = 1;
    ArrayStream<1024 * 128> stream;
    switch (ptr->activity_type % 100) {
      case 1: {
        ptr->field= "id@*goodid@*money_count@*name@*content";

        for (std::vector<std::vector<std::string> >::const_iterator iter =
                 ptr->raw_str.begin();
             iter != ptr->raw_str.end(); ++iter, ++index) {
          if (index == 2) stream.Append("#$");
          if (iter->size() < 4u) {
            ERROR_LOG(logger)("ActivityID:%d, Type%d, LineSize:%d"
                , ptr->activity_id, ptr->activity_type, (int32_t)iter->size());
            continue;
          }
          stream.Append("%s@*%s@*%s@*%s@*%s", iter->at(0).c_str(), iter->at(1).c_str(),
                        iter->at(2).c_str(), iter->at(3).c_str(),
                        iter->at(4).c_str());
        }
      } break;
      case 2 ... 21:
      case 51 ... 61: {
        ptr->field = "id@*count@*name@*content";
        for (std::vector<std::vector<std::string> >::const_iterator iter =
                 ptr->raw_str.begin();
             iter != ptr->raw_str.end(); ++iter, ++index) {
          if (index == 2) stream.Append("#$");
          if (iter->size() < 3u) {
            ERROR_LOG(logger)("ActivityID:%d, Type%d, LineSize:%d"
                , ptr->activity_id, ptr->activity_type, (int32_t)iter->size());
            continue;
          }
          stream.Append("%s@*%s@*%s@*%s", iter->at(0).c_str(),
                        iter->at(1).c_str(), iter->at(2).c_str(),
                        iter->at(3).c_str());
        }
      } break;
      case 50: {
        ptr->field = "id@*money@*count@*name@*content";
        for (std::vector<std::vector<std::string> >::const_iterator iter =
                 ptr->raw_str.begin();
             iter != ptr->raw_str.end(); ++iter, ++index) {
          if (index == 2) stream.Append("#$");
          if (iter->size() < 4u) {
            ERROR_LOG(logger)("ActivityID:%d, Type%d, LineSize:%d"
                , ptr->activity_id, ptr->activity_type, (int32_t)iter->size());
            continue;
          }
          stream.Append("%s@*%s@*%s@*%s@*%s", iter->at(0).c_str(),
                        iter->at(1).c_str(), iter->at(2).c_str(),
                        iter->at(3).c_str(), iter->at(4).c_str());
        }
      } break;
      default:
        ERROR_LOG(logger)("Unknown ActivityType:%d", ptr->activity_type);
        break;
    }
    SplitString(ptr->field, ptr->raw_field, "@*");
    ptr->content = stream.str();
  }
}

//每周活动的快表
//1本周的活动
static VectorMap<int32_t, std::vector<WeeklyActivityBase*> > kWeeklyActivityMap;
//开服活动的快表
static VectorMap<int32_t, std::vector<ServerStartActivityBase*> > kServerStartActivityMap;

const std::vector<WeeklyActivityBase*>& GetWeeklyActivityByIndex(
    int32_t week_index) {
  static std::vector<WeeklyActivityBase*> kEmpty;
  if (week_index <= 0) return kEmpty;
  for (VectorMap<int32_t, std::vector<WeeklyActivityBase*> >::reverse_iterator
           iter = kWeeklyActivityMap.rbegin();
       iter != kWeeklyActivityMap.rend(); ++iter) {
    if (week_index >= iter->first) return iter->second;
  }
  return kEmpty;
}


bool WeeklyActivityBase::Fill(pugi::xml_node node) {
  this->duration = 0;
  __CHECK(count);
  __CHECK(weekday);
  __CHECK(content_id);

  do {
    if (weekday < 0 || weekday >= 7) break;
    const AcitivityContentBase* ptr = GetActivityByID(this->content_id);
    if (!ptr) break;
    if (ptr->duration > 7) break;
    if (this->count <= 0) break;
    this->duration = ptr->duration;
    kWeeklyActivityMap[count].push_back(this);
  } while (false);

  DEBUG_LOG(logger)("load weekly activity, id:%ld", this->id());
  return true;
}

bool ServerStartActivityBase::Fill(pugi::xml_node node) {
  this->duration = 0;
  __CHECK(count);
  __CHECK(content_id);

  do {
    if (count <= 0) break;
    if (count >= 999) break;
    const AcitivityContentBase* ptr = GetActivityByID(this->content_id);
    if (!ptr) break;
    this->duration = ptr->duration;
    for (int32_t d = this->count; d < this->count + ptr->duration; ++d) {
      kServerStartActivityMap[d].push_back(this);
    }
  } while (false);

  DEBUG_LOG(logger)("load server start activity, id:%ld", this->id());
  return true;
}

bool FileActivityConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  kFileActivity.clear();
  kWeeklyActivityMap.clear();
  kServerStartActivityMap.clear();

  pugi::xpath_node_set nodes =
      doc.select_nodes("/config/fixed_activity_content/item");
  for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
       iter != nodes.end(); ++iter) {
    pugi::xml_node node = iter->node();

    int32_t activity_id = node.attribute("activity_id").as_int();
    int32_t activity_type = node.attribute("activity_type").as_int();
    int32_t duration = node.attribute("duration").as_int();
    const std::string& col1 = node.attribute("col1").as_string();
    const std::string& col2 = node.attribute("col2").as_string();
    const std::string& col3 = node.attribute("col3").as_string();
    const std::string& col4 = node.attribute("col4").as_string();
    const std::string& col5 = node.attribute("col5").as_string();
    const std::string& desc = node.attribute("desc").as_string();

    AcitivityContentBase& config = kFileActivity[activity_id];
    config.activity_id = config.activity_id ? config.activity_id : activity_id;
    config.activity_type = config.activity_type ? config.activity_type : activity_type;
    config.duration = config.duration ? config.duration : duration;
    config.desc = config.desc.empty() ? desc : config.desc;
    std::vector<std::string> v;
    v.push_back(col1);
    v.push_back(col2);
    v.push_back(col3);
    v.push_back(col4);
    v.push_back(col5);
    config.raw_str.push_back(v);
  }
  ConvertRawStringToInteralFormat();

  this->ParseTable<WeeklyActivityBase, ConfigEntryManager<WeeklyActivityBase> >(
      "/config/fixed_weekly_activity/item", "id");
  this->ParseTable<ServerStartActivityBase,
                   ConfigEntryManager<ServerStartActivityBase> >(
      "/config/fixed_server_start_activity/item", "id");
  return true;
}

void AddWeeklyActivity(time_t time_begin, int32_t weeks,
                       const std::vector<WeeklyActivityBase*>& config,
                       std::vector<ActivityInfoConfig>& activity) {
  for (std::vector<WeeklyActivityBase*>::const_iterator iter = config.begin();
       iter != config.end(); ++iter) {
    WeeklyActivityBase* ptr = *iter;
    if (!ptr) continue;
    const AcitivityContentBase* p = GetActivityByID(ptr->content_id);
    if (!p) continue;

    ActivityInfoConfig v;
    v.activity_id = time_begin + 24 * 3600 * ptr->weekday;
    v.begin_time = time_begin + 24 * 3600 * ptr->weekday;
    v.end_time = v.begin_time + ptr->duration * 24 * 3600;
    v.activity_type = p->activity_type;
    v.raw_field = p->raw_field;
    v.raw_content = p->raw_str;
    v.desc = p->desc;
    if (v.end_time < server->server_start_time()) continue;
    activity.push_back(v);
  }
}

std::vector<ActivityInfoConfig> GetWeeklyActivityByWeeks(int32_t week_index) {
  std::vector<ActivityInfoConfig> ret;
  int32_t current_week = server->GetServerStartWeeks();
  time_t sunday_time = MakeSundayTime(server->server_start_time()) +
                       (current_week - 1) * 7 * 24 * 3600;

  const std::vector<WeeklyActivityBase*>& current_config =
      GetWeeklyActivityByIndex(week_index);
  AddWeeklyActivity(sunday_time, current_week, current_config, ret);
  const std::vector<WeeklyActivityBase*>& last_config =
      GetWeeklyActivityByIndex(week_index - 1);
  AddWeeklyActivity(sunday_time - 7 * 24 * 3600, current_week - 1, last_config,
                    ret);
  return ret;
}

std::vector<ActivityInfoConfig> GetServerStartActivityByDay(
    int32_t start_days) {
  std::vector<ActivityInfoConfig> ret;

  if (server->GetServerStartDays() <= 0) return ret;
  int32_t server_start_time = GetZeroClock(server->server_start_time());
  const std::vector<ServerStartActivityBase*>& vec = kServerStartActivityMap[start_days];
  for (std::vector<ServerStartActivityBase*>::const_iterator iter = vec.begin();
       iter != vec.end(); ++iter) {
    ServerStartActivityBase* ptr = *iter;
    if (!ptr) continue;
    const AcitivityContentBase* p = GetActivityByID(ptr->content_id);
    if (!p) continue;

    ActivityInfoConfig v;
    v.activity_id = server_start_time + (ptr->count - 1) * 24 * 3600;
    v.begin_time = server_start_time + (ptr->count - 1) * 24 * 3600;
    v.end_time = v.begin_time + ptr->duration * 24 * 3600;
    v.activity_type = p->activity_type;
    v.raw_field = p->raw_field;
    v.raw_content = p->raw_str;
    v.desc = p->desc;
    ret.push_back(v);
  }

  return ret;
}
