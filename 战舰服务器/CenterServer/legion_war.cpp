#include <str_util.h>
#include <storage.h>
#include <system.h>
#include "server.h"
#include <array_stream.h>
#include <storage_ext.h>
#include "legion_war.h"

const std::string& kKVPrefix = "LegionWar";

const std::string& kKVTypeUpdateTime  = "LegionWarUpdateTime";  //制霸全球更新时间
const std::string& kKVTypePlayers     = "LegionWarPlayers";     //制霸全球参与的玩家
const std::string& kKVTypeWarMap      = "LegionWarMap";         //制霸全球的地图信息

LegionWar::LegionWar(int32_t region)
    : region_(region), last_update_time_(0) {}

LegionWar::~LegionWar() {
  DEBUG_LOG(logger)("LegionWar Dispose, Region:%d", this->region_);
}

void LegionWar::LoadFromLocal() {
  this->players_.clear();
  this->cities_.clear();
  {
    const std::string& key =
        MakeKVStorageKey(kKVPrefix, kKVTypeUpdateTime, region_);
    storage_ext::Load(key, this->last_update_time_);
    DEBUG_LOG(logger)("LegionWarUpdateTime Region:%d, Time:%ld"
        , this->region_, this->last_update_time_);
  }
  {
    const std::string& key = MakeKVStorageKey(kKVPrefix, kKVTypePlayers, region_);
    storage_ext::Load(key, this->players_);
    for (VectorSet<int64_t>::iterator iter = this->players_.begin();
         iter != this->players_.end(); ++iter) {
      DEBUG_LOG(logger)("LegionWar Region:%d, Players:%ld"
          , this->region_, *iter);
    }
  }
  {
    const std::string& key = MakeKVStorageKey(kKVPrefix, kKVTypeWarMap, region_);
    const std::string& value = storage::Get(key);
    std::vector<std::string> values;
    SplitString(value, values, "\n");
    for (std::vector<std::string>::iterator iter = values.begin();
         iter != values.end(); ++iter) {
      int32_t city_id = 0, pos = 0;
      int64_t player_id = 0;
      if (sscanf(iter->c_str(), "%d-%d,%ld", &city_id, &pos, &player_id) >= 3) {
        const CityPtr& ptr = this->GetCity(city_id);
        ptr->set(pos, player_id);
      }
    }
  }

  if (GetSecondsDiffDays(this->last_update_time_, GetSeconds())) {
    this->last_update_time_ = GetSeconds();
    this->players_.clear();
    this->cities_.clear();
    this->SaveToLocal();
  }
}

std::string LegionWar::Dump() const {
  ArrayStream<1024 * 128> stream;
  for (VectorMap<int32_t, CityPtr>::const_iterator iter = this->cities_.begin();
       iter != this->cities_.end(); ++iter) {
    int32_t city_id = iter->first;
    const std::vector<int64_t>& players = iter->second->players();
    for (size_t pos = 1; pos < players.size(); ++pos) {
      if (players[pos]) {
        if (stream.size()) stream.Append("\n");
        stream.Append("%d-%lu,%ld", city_id, pos, players[pos]);
      }
    }
  }

  return stream.str();
}

void LegionWar::SaveToLocal() {
  this->last_update_time_ = GetSeconds();
  {
    const std::string& key =
        MakeKVStorageKey(kKVPrefix, kKVTypeUpdateTime, region_);
    storage_ext::Save(key, this->last_update_time_);
  }
  {
    const std::string& key = MakeKVStorageKey(kKVPrefix, kKVTypePlayers, region_);
    storage_ext::Save(key, this->players_);
  }
  {
    const std::string& key = MakeKVStorageKey(kKVPrefix, kKVTypeWarMap, region_);
    const std::string& value = this->Dump();
    storage::Set(key, value);
  }
}

LegionWar::CityPtr LegionWar::GetCity(int32_t city) {
  CityPtr& ptr = this->cities_[city];
  if (!ptr) ptr.reset(new City(city));
  return ptr;
}

std::pair<int32_t, int32_t> LegionWar::Register(int64_t player) {
  DEBUG_LOG(logger)("LegionWar::Register PlayerID:%ld, CityCount:%lu"
      , player, this->cities_.size());
  //已经注册过了
  if (this->players_.find(player) != this->players_.end()) {
    const std::string& value = this->Dump();
    DEBUG_LOG(logger)("LegionWar Player Existed, Region:%d\n%s", this->region_, value.c_str());
    for (VectorMap<int32_t, CityPtr>::iterator iter = this->cities_.begin();
         iter != this->cities_.end(); ++iter) {
      const City* ptr = iter->second.get();
      int32_t pos = ptr->GetPlayerPosition(player);
      if (pos) {
        DEBUG_LOG(logger)("LegionWar Register New Player, Region:%d, Player:%ld, City:(%d-%d), Existed"
            , this->region_, player, iter->first, pos);
        return std::make_pair(ptr->city_id(), pos);
      }
    }
  }
  const std::vector<std::pair<int32_t, int32_t> >& positions =
      LegionConfigFile::GetPositions();
  //没有空位
  if (this->players_.size() >= positions.size()) {
    return std::make_pair(0, 0);
  }

  for (std::vector<std::pair<int32_t, int32_t> >::const_iterator iter =
           positions.begin();
       iter != positions.end(); ++iter) {
    const CityPtr& ptr = this->GetCity(iter->first);
    int32_t p = ptr->get(iter->second);
    if (!p) {
      ptr->set(iter->second, player);
      this->players_.insert(player);
      TRACE_LOG(logger)("LegionWar Register New Player, Region:%d, Player:%ld, City:(%d-%d)"
          , this->region_, player, iter->first, iter->second);
      this->SaveToLocal();
      return std::make_pair(iter->first, iter->second);
    }
  }

  return std::make_pair(0, 0);
}

void LegionWar::SwapPosition(std::pair<int32_t, int32_t> a,
                             std::pair<int32_t, int32_t> b) {
  const CityPtr& city_a = this->GetCity(a.first);
  const CityPtr& city_b = this->GetCity(b.first);
  int64_t player_a = city_a->get(a.second);
  int64_t player_b = city_b->get(b.second);

  city_a->set(a.second, player_b);
  city_b->set(b.second, player_a);

  TRACE_LOG(logger)("LegionWar SwapPosition, Region:%d, CityA:%d-%d, PlayerA:%ld, CityB:%d-%d, PlayerB:%ld"
      , this->region_
      , a.first, a.second, player_a
      , b.first, b.second, player_b);
  this->SaveToLocal();
}

void LegionWar::Clear() {
  DEBUG_LOG(logger)("LegionWar Clear, Region:%d", this->region_);
  this->players_.clear();
  this->cities_.clear();
  this->SaveToLocal();
}

static VectorMap<int32_t, LegionWarPtr> kGlobalLegionWar;

LegionWarPtr LegionWar::GetLegionWar(int32_t region) {
  VectorMap<int32_t, LegionWarPtr>::iterator iter = kGlobalLegionWar.find(region);
  if (iter == kGlobalLegionWar.end()) {
    LegionWarPtr ptr = LegionWarPtr(new LegionWar(region));
    ptr->LoadFromLocal();
    kGlobalLegionWar[region] = ptr;
    return ptr;
  }
  return iter->second;
}

void LegionWar::ClearAll() {
  DEBUG_LOG(logger)("LegionWar::ClearAll");
  for (VectorMap<int32_t, LegionWarPtr>::iterator iter =
           kGlobalLegionWar.begin();
       iter != kGlobalLegionWar.end(); ++iter) {
    iter->second->Clear();
  }
  //kGlobalLegionWar.clear();
}

bool LegionWar::HasPlayer(int64_t player_id) {
  VectorSet<int64_t>::const_iterator iter = this->players_.find(player_id);
  return iter != this->players_.end();
}
