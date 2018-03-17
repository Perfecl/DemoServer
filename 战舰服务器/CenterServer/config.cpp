#include "config.h"
#include <pugixml.hpp>
#include <logger.h>
#include "server.h"
#include <vector_map.h>

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

void AddConfigFile() {
  ConfigFileManager::Instance().AddConfigFile(new SettingConfigFile("./config/setting.xml"));
  ConfigFileManager::Instance().AddConfigFile(new NameConfigFile("./config/hero.xml"));
  ConfigFileManager::Instance().AddConfigFile(new NameConfigFile("./config/道具.xml"));
  ConfigFileManager::Instance().AddConfigFile(new NameConfigFile("./config/航母.xml"));
  ConfigFileManager::Instance().AddConfigFile(new NameConfigFile("./config/部队.xml"));
  ConfigFileManager::Instance().AddConfigFile(new NameConfigFile("./config/装备.xml"));
  ConfigFileManager::Instance().AddConfigFile(new LegionConfigFile("./config/y制霸全球.xml"));
  __ADD_CONFIG(SkillConfigFile, "./config/技能.xml");
  __ADD_CONFIG(HeroConfigFile, "./config/hero.xml");
  __ADD_CONFIG(CarrierConfigFile, "./config/航母.xml");
}

void AfterLoadConfig() {
  Setting::kCrit = GetSettingValue(crit_o);
}

static boost::unordered_map<std::string, int64_t> setting_container_;
static boost::unordered_map<std::string, std::vector<int32_t> > setting_container_1_;
static boost::unordered_map<std::string, ValuePair2Vec> setting_container_2_;

namespace Setting {

int32_t kCrit = 150;

int64_t GetValue(const std::string& key) {
  boost::unordered_map<std::string, int64_t>::const_iterator iter =
      setting_container_.find(key);
  return iter != setting_container_.end() ? iter->second : 0;
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

};

bool ServerConfig::Parse() {
 pugi::xml_document& doc = this->doc();
 if (!doc) return false;

 http_port_ = "18800";

 {
   pugi::xpath_node_set nodes = doc.select_nodes("/root/listen");
   for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
        iter != nodes.end(); ++iter) {
     pugi::xml_node node = iter->node();
     ListenInfo info;
     info.first = node.attribute("port").as_int();
     std::string type = node.attribute("type").as_string();
     if (type == "server") info.second = ENTRY_TYPE_LOGIC_SERVER;
     if (type == "http") {
       http_port_ = node.attribute("port").as_string();
     }

     if (info.second != 0) this->listen_ports_.push_back(info);
   }
 }

 {
   pugi::xpath_node_set nodes = doc.select_nodes("/root/log_level");
   if (nodes.begin() != nodes.end()) {
    pugi::xml_node node = nodes.begin()->node();
    log_level_ = (LoggerLevel)node.attribute("level").as_int();
    logger->log_level(log_level_);
   }
 }

 if (!this->ParseMySQLParam("/root/auth_mysql", this->mysql_)) {
   ERROR_LOG(logger)("Load /root/auth_mysql fail");
   return false;
 }

 if (!this->ParseMySQLParam("/root/gm_mysql", this->gm_mysql_)) {
   ERROR_LOG(logger)("Load /root/gm_mysql fail");
   return false;
 }

 return true;
}

bool NameConfigFile::Parse() {
 pugi::xml_document& doc = this->doc();
 if (!doc) return false;
 {
   pugi::xpath_node_set nodes = doc.select_nodes("//item[@name]");
   for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
        iter != nodes.end(); ++iter) {
     pugi::xml_node node = iter->node();
     int64_t id = node.attribute("id").as_llong();
     const std::string& name = node.attribute("name").as_string();
     if (!name.empty()) {
       DEBUG_LOG(logger)("ItemName:%ld, %s", id, name.c_str());
       server->AddItemName(id, name);
     }
   }
 }
 return true;
}

//品质 => 个数
VectorMap<int32_t, int32_t> kLegionCount;
//Quality => CityID
std::vector<std::pair<int32_t, int32_t> > kCity;
//位置池(CityID, Position)
std::vector<std::pair<int32_t, int32_t> > kPositionPool;

static inline void ShufflePosition(
    std::vector<std::pair<int32_t, int32_t> >& position) {
  static VectorMap<int32_t, int32_t> CityIndex;
  std::random_shuffle(position.begin(), position.end());
  for (std::vector<std::pair<int32_t, int32_t> >::iterator iter =
           position.begin();
       iter != position.end(); ++iter) {
    iter->second = ++CityIndex[iter->first];
  }
  std::reverse(position.begin(), position.end());
}

const std::vector<std::pair<int32_t, int32_t> >&
LegionConfigFile::GetPositions() {
  return kPositionPool;
}

bool LegionConfigFile::Parse() {
 pugi::xml_document& doc = this->doc();
 if (!doc) return false;

  kLegionCount.clear();
  kCity.clear();
  {
    pugi::xpath_node_set nodes =
        doc.select_nodes("/config/legion_map_score/item");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
     pugi::xml_node node = iter->node();
     int32_t id = node.attribute("id").as_int();
#define GET_VALUE(NUM) { int32_t value = node.attribute("award" #NUM).as_int(); if (value) kLegionCount[NUM] = id; }
     GET_VALUE(6)
     GET_VALUE(5)
     GET_VALUE(4)
     GET_VALUE(3)
     GET_VALUE(2)
#undef GET_VALUE
   }
  }

  {
    pugi::xpath_node_set nodes =
        doc.select_nodes("/config/legion_map/item");
    for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
         iter != nodes.end(); ++iter) {
     pugi::xml_node node = iter->node();
     int32_t city_id = node.attribute("id").as_int();
     int32_t quality = node.attribute("quality").as_int();
     kCity.push_back(std::make_pair(quality, city_id));
    }
  }

  std::sort(kCity.begin(), kCity.end());

  //(CityID, Position)[]
  std::vector<std::pair<int32_t, int32_t> > position;
  int32_t quality = 0;
  for (std::vector<std::pair<int32_t, int32_t> >::const_iterator iter =
           kCity.begin();
       iter != kCity.end(); ++iter) {
    if (quality != iter->first) {
      ShufflePosition(position);
      kPositionPool.insert(kPositionPool.end(), position.begin(),
                           position.end());
      position.clear();
    }
    std::vector<std::pair<int32_t, int32_t> > pos;
    pos.resize(kLegionCount[iter->first], std::make_pair(iter->second, 0));
    position.insert(position.end(), pos.begin(), pos.end());
    quality = iter->first;
  }

  ShufflePosition(position);
  kPositionPool.insert(kPositionPool.end(), position.begin(), position.end());

  return true;
}

static int32_t kServerGroup = 10;
static int32_t kServerGroupMinSize = 5;

int32_t SettingConfigFile::GetRegionSize() { return kServerGroup; }

bool SettingConfigFile::Parse() {
 pugi::xml_document& doc = this->doc();
 if (!doc) return false;

 std::string GroupSize("server_partition_unit");
 std::string GroupMinSize("server_partition_min_sever");

 pugi::xpath_node_set nodes = doc.select_nodes("/config/setting/item");
 for (pugi::xpath_node_set::const_iterator iter = nodes.begin();
      iter != nodes.end(); ++iter) {
   pugi::xml_node node = iter->node();
   if (node.attribute("param").as_string() == GroupSize) {
     GetXmlAttr(kServerGroup, node, "value");
     break;
   }
   if (node.attribute("param").as_string() == GroupMinSize) {
     GetXmlAttr(kServerGroupMinSize, node, "value");
     break;
   }
 }
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

 if (kServerGroup <= 0) kServerGroup = 10;
 if (kServerGroupMinSize <= 0) kServerGroupMinSize = 5;
 return true;
}

void SettingConfigFile::GenerateRegion(VectorMap<uint32_t, int32_t>& servers,
                                       int32_t& kRegionNum) {
  int32_t count = 0;
  for (VectorMap<uint32_t, int32_t>::iterator iter = servers.begin();
       iter != servers.end(); ++iter, ++count) {
    if (count % kServerGroup == 0) {
      kRegionNum++;
    }
    iter->second = kRegionNum;
  }
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

bool HeroConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<HeroBase, ConfigEntryManager<HeroBase> >("/config/hero/item", "id");

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

bool CarrierConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<CarrierBase, ConfigEntryManager<CarrierBase> >("/config/carrier_info/item", "id");

  return true;
}

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

  DEBUG_LOG(logger)("load copy, copy id:%ld", this->id());
  return true;
}

bool CopyConfigFile::Parse() {
  pugi::xml_document& doc = this->doc();
  if (!doc) return false;

  this->ParseTable<CopyBase, ConfigEntryManager<CopyBase> >("/config/copy_info/item", "id");

  return true;
}
