#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <boost/unordered_map.hpp>
#include <storage.h>
#include <array_stream.h>
#include "server.h"
#include <vector_map.h>
#include <myrandom.h>
#include <map>
#include <vector_set.h>
#include "legion_war.h"

using namespace sy;
using namespace intranet;

#ifdef IOS_PLATFORM
const int32_t EXCEPT_PLATFORM = 4;
#else
const int32_t EXCEPT_PLATFORM = 3;
#endif

struct OtherPlayerSimpleInfo
{
  uint32_t server_id;
  int32_t level;
  int64_t fight;
};

static VectorMap<int64_t, int64_t> FightMap;
static std::map<int32_t, VectorSet<int64_t> > LevelMap;
static boost::unordered_map<int64_t, OtherPlayerSimpleInfo> PlayerMap;

const uint32_t kMinPlayerSetCount = 50;

struct ForeachOtherPlayer : public storage::ForEachCallback {
  ForeachOtherPlayer() : prefix_("SummaryOtherPlayer:") {}

  const std::string& prefix() const { return prefix_; }

  bool every(const storage::Slice& key, const storage::Slice& value) const {
    //Key是SummaryOtherPlayer:%ld
    //Value是ServerID:%u,Level:%d,Fight:%ld
    int64_t player_id = 0;
    uint32_t server_id = 0;
    int32_t level = 0;
    int64_t fight = 0;
    if (sscanf(key.data() + prefix_.size(), "%ld", &player_id) >= 1 &&
        sscanf(value.data(), "ServerID:%u,Level:%d,Fight:%ld", &server_id,
               &level, &fight) >= 3) {
      FightMap.insert(std::make_pair(fight, player_id));
      LevelMap[level].insert(player_id);
      OtherPlayerSimpleInfo info = {server_id, level, fight};
      PlayerMap[player_id] = info;
      INFO_LOG(logger)("ForeachOtherPlayer, PlayerID:%ld, ServerID:%u, Level:%d, Fight:%ld",
          player_id, server_id, level, fight);
    }

    return true;
  }

  std::string prefix_;
};

void Server::SaveOtherPlayer(const sy::OtherPlayerInfo& player) {
  {
    boost::unordered_map<int64_t, OtherPlayerSimpleInfo>::iterator iter = PlayerMap.find(player.player_id());
    if (iter != PlayerMap.end()) {
      FightMap.erase(iter->second.fight);
    }
  }
  {
    DefaultArrayStream stream;
    stream.Append("SummaryOtherPlayer:%ld", player.player_id());
    const std::string& key = stream.str();
    stream.clear();
    int64_t fight = 0;
    for (int32_t i = 0; i < player.heros_size(); ++i) {
      fight += player.heros(i).attr1(0);
    }
    stream.Append("ServerID:%u,Level:%d,Fight:%ld", player.server_id(), player.level(), fight);
    const std::string& value = stream.str();
    storage::Set(key, value);
    //更新内存数据
    OtherPlayerSimpleInfo info = {player.server_id(), player.level(), fight};
    PlayerMap[player.player_id()] = info;
    FightMap[fight] = player.player_id();
    LevelMap[player.level() - 1].erase(player.player_id());
    LevelMap[player.level()].insert(player.player_id());
    DEBUG_LOG(logger)("SaveOtherPlayer, PlayerID:%ld, ServerID:%u, Level:%d, Fight:%ld"
        , player.player_id(), player.server_id(), player.level(), fight);
  }
  {
    DefaultArrayStream stream;
    stream.Append("DetailsOtherPlayer:%ld", player.player_id());
    const std::string& key = stream.str();
    const std::string& value = player.SerializeAsString();
    storage::Set(key, value);
  }
}

static inline int64_t SearchOnePlayerByFight(int64_t fight) {
  if (FightMap.size() <= kMinPlayerSetCount) return 0;
  int64_t distance = std::max(labs(FightMap.begin()->first - fight),
                              labs(FightMap.rbegin()->first - fight));
  double ratio = std::pow(distance, 0.1);
  if (ratio <= 1) ratio = 1.1;
  int32_t round = 1;

  while (round++ < 100) {
    int64_t fight_max = fight * std::pow(ratio, round) + round * 10000;
    int64_t fight_min = 2 * fight - fight_max;
    VectorMap<int64_t, int64_t>::iterator lower = FightMap.lower_bound(fight_min);
    VectorMap<int64_t, int64_t>::iterator upper = FightMap.lower_bound(fight_max);
    if (lower == FightMap.end()) continue;
    if (upper == FightMap.end()) upper = FightMap.end() - 1;
    uint32_t count = labs(std::distance(lower, upper));
    if (count <= kMinPlayerSetCount) continue;
    int32_t random = RandomBetween(0, count - 1);
    return (lower + random)->second;
  }

  return 0;
}

static inline int64_t SearchOnePlayerByLevel(int64_t level) {
  if (LevelMap.empty()) return 0;
  int32_t step = 2;
  int32_t round = 1;
  std::vector<std::pair<int32_t, int32_t> > level_2_count;

  while (round++ < 1000) {
    level_2_count.clear();

    int32_t min_level = level - round * step;
    int32_t max_level = level + round * step;
    std::map<int32_t, VectorSet<int64_t> >::iterator lower = LevelMap.lower_bound(min_level);
    std::map<int32_t, VectorSet<int64_t> >::iterator upper = LevelMap.lower_bound(max_level);
    uint32_t count = 0;
    for (std::map<int32_t, VectorSet<int64_t> >::iterator iter = lower;
         iter != upper; ++iter) {
      if (iter->second.empty()) continue;
      level_2_count.push_back(std::make_pair(iter->first, iter->second.size()));
      count += level_2_count.back().second;
    }
    if (count <= kMinPlayerSetCount) continue;
    int32_t random = RandomBetween(0, count - 1);
    for (std::vector<std::pair<int32_t, int32_t> >::iterator iter =
             level_2_count.begin();
         iter != level_2_count.end(); ++iter) {
      random -= iter->second;
      if (random > 0) continue;
      VectorSet<int64_t>& set = LevelMap[iter->first];
      int32_t rand = RandomBetween(0, set.size() - 1);
      return *(set.begin() + rand);
    }
  }
  return 0;
}

static inline bool FetchOtherPlayer(int64_t player_id,
                                    sy::OtherPlayerInfo* info) {
  DefaultArrayStream stream;
  stream.Append("DetailsOtherPlayer:%ld", player_id);
  const std::string& value = storage::Get(stream.str());
  info->Clear();
  if (!info->ParseFromString(value)) return false;
  return true;
}

void Server::LoadOtherPlayer() {
  ForeachOtherPlayer fn;
  storage::ForEach(fn);
}

int32_t ProcessUpdateOtherPlayerInfo(SSMessageEntry& entry) {
  MessageSSUpdateOtherPlayerInfo* message =
      static_cast<MessageSSUpdateOtherPlayerInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  server->SaveOtherPlayer(message->info());
  return ERR_OK;
}

int32_t ProcessRequestQueryOtherPlayer(SSMessageEntry& entry) {
  MessageSSRequestQueryOtherPlayer* message =
      static_cast<MessageSSRequestQueryOtherPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSResponseQueryOtherPlayer response;
  for (int32_t i = 0; i < message->query_size(); ++i) {
    const QueryOtherPlayer& query = message->query(i);
    sy::OtherPlayerInfo* info = response.add_player();
    int32_t count = 0;
    while (count++ <= 3) {
      info->set_player_id(0);
      int64_t player_id = 0;
      if (query.has_level()) player_id = SearchOnePlayerByLevel(query.level());
      if (query.has_fight()) player_id = SearchOnePlayerByFight(query.fight());
      TRACE_LOG(logger)("QueryOtherPlayer, level:%d, fight:%ld, PlayerID:%ld"
        , query.level(), query.fight()
        , player_id);
      if (!player_id) continue;
      if (!FetchOtherPlayer(player_id, info)) info->set_player_id(0);

      int32_t platform = info->server_id() / 10000;
      if (platform == EXCEPT_PLATFORM) info->set_player_id(0);

      if (info->player_id()) break;
    }
  }

  response.set_request_player_id(message->request_player_id());
  response.set_msg_id(message->msg_id());
  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_QUERY_OTHER_PLAYER, &response);
  return ERR_OK;
}

int32_t ProcessUpdateLegionWarOtherPlayer(SSMessageEntry& entry) {
  MessageSSUpdateLegionWarPlayer* message = static_cast<MessageSSUpdateLegionWarPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  sy::OtherPlayerInfo info;
  if (FetchOtherPlayer(message->player_id(), &info)) {
    if (message->army_id()) {
      info.set_army_id(message->army_id());
      info.set_army_avatar(message->army_avatar());
      info.set_army_name(message->army_name());
    } else {
      message->clear_army_avatar();
      message->clear_army_id();
      message->clear_army_name();
    }
    server->SaveOtherPlayer(info);
  }

  int32_t region = server->GetLegionRegionByServerID(message->server_id());
  const LegionWarPtr& war = LegionWar::GetLegionWar(region);
  if (war && war->HasPlayer(message->player_id())) {
    server->SendMessageToLegionWarRegion(message->server_id(), entry.head.msgid,
                                         message);
  }
  return ERR_OK;
}

std::vector<sy::OtherPlayerInfo> Server::SearchPlayerExcept(
    int32_t level, int32_t count, const VectorSet<int64_t>& e) {
  std::vector<sy::OtherPlayerInfo> result;

  assert(count <= 10 && "玩家数量不能太多");
  if (count > 10) return result;

  VectorSet<int64_t> except = e;
  sy::OtherPlayerInfo player;
  while (result.size() < (size_t)count) {
    player.Clear();
    int64_t player_id = SearchOnePlayerByLevel(level);
    if (!player_id) break;
    if (except.find(player_id) != except.end()) continue;
    if (FetchOtherPlayer(player_id, &player)) {
      if (player.server_id() / 10000 == EXCEPT_PLATFORM) continue;
      except.insert(player_id);
      result.push_back(player);
    }
  }

  return result;
}
