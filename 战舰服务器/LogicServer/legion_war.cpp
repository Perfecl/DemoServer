#include <storage_ext.h>
#include <str_util.h>
#include <storage.h>
#include <array_stream.h>
#include "legion_war.h"
#include "logic_player.h"
#include "server.h"
#include "rank_list.h"
#include "army.h"
#include "myrandom.h"

using namespace sy;

std::pair<int64_t, int32_t> LegionCity::GetArmy() {
  VectorMap<int64_t, int32_t> score_map;
  const LegionCityBase* base= LEGION_CITY_BASE.GetEntryByID(this->city_id_).get();
  if (!base) return std::make_pair(0l, 0);
  const std::vector<int32_t>& config = GetLegionArmyScore(base->quality);

  for (size_t index = 1; index < this->players_.size(); ++index) {
    if (!this->players_[index]) continue;
    sy::OtherPlayerInfo* player = LegionWar::Instance().GetPlayerByID(this->players_[index]);
    Army* army = server->GetArmyByID(player ? player->army_id() : 0);
    if (army) score_map[army->army_id()] += index <= config.size() ? config[index - 1] : 0;
  }
  if (score_map.empty()) return std::make_pair(0l, 0);

  int64_t army_id = 0;
  int32_t score = 0;
  for (VectorMap<int64_t, int32_t>::const_iterator iter = score_map.begin();
       iter != score_map.end(); ++iter) {
    if (iter->second > score) {
      army_id = iter->first;
      score = iter->second;
    }
  }
  return std::make_pair(army_id, base->quality);
}

LegionWar::LegionWar() : last_update_time_(0) {}

void LegionWar::NewPlayer(sy::OtherPlayerInfo& player,
                          std::pair<int32_t, int32_t> pos) {
  MessageResponseLegionWarRegister response;
  response.set_city(pos.first);
  response.set_position(pos.second);
  LogicPlayer* logic_player = server->GetPlayerByID(player.player_id());
  if (logic_player) {
    logic_player->SendMessageToClient(MSG_CS_RESPONSE_LEGION_WAR_REGISTER,
                                      &response);
  }

  const LegionCityPtr& city = this->GetCity(pos.first);
  if (!city) return;

  city->set(pos.second, player.player_id());
  this->players_[player.player_id()].reset(new OtherPlayerInfo(player));
  DEBUG_LOG(logger)("LegionWarNewPlayer, City:(%d-%d), Player:%ld"
      , pos.first, pos.second, player.player_id());

  this->SavePositionToLocal();
  this->SavePlayer(player);

  MessageNotifyLegionWarPlayer notify;
  notify.mutable_player()->CopyFrom(player);
  notify.set_position(pos.second);
  notify.set_city_id(pos.first);
  this->SendMessageToAll(MSG_CS_NOTIFY_LEGION_WAR_PLAYER, &notify);

  const std::string& value = this->dump();
  DEBUG_LOG(logger)("\n%s", value.c_str());
}

void LegionWar::Subscribe(LogicPlayer* player) {
  if (player) this->notify_player_.insert(player->session());
}

void LegionWar::SwapPos(std::pair<int32_t, int32_t> pos1,
                        std::pair<int32_t, int32_t> pos2) {

  const LegionCityPtr& city_a = this->GetCity(pos1.first);
  const LegionCityPtr& city_b = this->GetCity(pos2.first);
  if (!city_a || !city_b) {
    ERROR_LOG(logger)("LegionWar::SwapPos, CityA:%d, CityB:%d, One Not Found"
        , pos1.first, pos2.first);
    return;
  }
  int64_t player_a = city_a->get(pos1.second);
  int64_t player_b = city_b->get(pos2.second);

  city_a->set(pos1.second, player_b);
  city_b->set(pos2.second, player_a);

  DEBUG_LOG(logger)("LegionWarSwapPos, CityA:(%d-%d), PlayerA:%ld, CityB:(%d-%d), PlayerB:%ld"
      , pos1.first, pos1.second, player_a
      , pos2.first, pos2.second, player_b);
  this->SavePositionToLocal();

  MessageNotifyLegionWarPos notify;
  notify.set_city_1(pos1.first);
  notify.set_position_1(pos1.second);
  notify.set_city_2(pos2.first);
  notify.set_position_2(pos2.second);
  this->SendMessageToAll(MSG_CS_NOTIFY_LEGION_WAR_POS, &notify);

  sy::OtherPlayerInfo* pa = this->GetPlayerByID(player_a);
  sy::OtherPlayerInfo* pb = this->GetPlayerByID(player_b);

  {
    const std::string& value = this->dump();
    DEBUG_LOG(logger)("\n%s", value.c_str());
  }
  if (!(pa && pb)) return;
  const std::string& name_1 = pa ? pa->name() : "";
  const std::string& name_2 = pb ? pb->name() : "";
  DefaultArrayStream stream;
  stream.Append("%ld,%d,%d,%ld,%s,%d,%d,%ld,%s", GetSeconds(), pos1.first,
                pos1.second, player_a, name_1.c_str(), pos2.first, pos2.second,
                player_b, name_2.c_str());
  const std::string& log = stream.str();
  int64_t player_id = pb->player_id();
  const std::string& key = MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                            KVStorage::kKVTypeLOG, player_id);
  const std::string& value = storage::Get(key);
  MessageResponseGetLegionWarLog msg;
  msg.ParseFromString(value);
  if (msg.info_size() >= 20) {
    msg.mutable_info()->DeleteSubrange(0, 1);
  }
  *msg.add_info() = log;
  storage::Set(key, msg.SerializeAsString());
}

void LegionWar::ClearWithoutScore() {
  DEBUG_LOG(logger)("LegionWarClearAllWithoutScore, Now:%ld, Time:%ld"
      , GetSeconds(), this->last_update_time_);

  storage_ext::DeleteItemByPrefixFn fn(KVStorage::kKVPrefixLegionWar);
  storage::ForEach(fn);

  this->last_update_time_ = GetSeconds();
  this->cities_.clear();
  this->players_.clear();
  this->notify_player_.clear();

  storage_ext::Load(MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                     KVStorage::kKVTypeUpdateTime),
                    this->last_update_time_);
}

void LegionWar::ClearAll() {
  this->ClearWithoutScore();
  DEBUG_LOG(logger)("LegionWarClearAll, Now:%ld"
      , GetSeconds());
  this->player_score_.clear();
  storage_ext::DeleteItemByPrefixFn fn(KVStorage::kKVPrefixLegionWarScore);
  storage::ForEach(fn);

  MessageRespopnseGetLegionWarTargetAward response;
  response.set_score(0);
  server->SendMessageToAllClient(MSG_CS_RESPONSE_GET_LEGION_WAR_TARGET_AWARD, &response);
}

void LegionWar::LoadFromLocal() {
  DEBUG_LOG(logger)("LegionWarLoadFromLocal");
  if (!this->last_update_time_) {
    storage_ext::Load(MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                       KVStorage::kKVTypeUpdateTime),
                      this->last_update_time_);
    const std::string& value = storage::Get(
        MakeKVStorageKey(KVStorage::kKVPrefixLegionWar, KVStorage::kKVTypeMap));
    DEBUG_LOG(logger)("%s", value.c_str());
    std::vector<std::string> values;
    SplitString(value, values, "\n");
    for (std::vector<std::string>::iterator iter = values.begin();
         iter != values.end(); ++iter) {
      int32_t city_id = 0, pos = 0;
      int64_t player_id = 0;
      if (sscanf(iter->c_str(), "%d-%d,%ld", &city_id, &pos, &player_id) >= 3) {
        if (!player_id) continue;
        const LegionCityPtr& ptr = this->GetCity(city_id);
        ptr->set(pos, player_id);

        const std::string& value = storage::Get(
            MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                             KVStorage::kKVTypeOtherPlayer, player_id));
        sy::OtherPlayerInfo player;
        player.ParseFromString(value);
        this->players_[player_id].reset(new sy::OtherPlayerInfo(player));
      }
    }
  }

  if (GetSecondsDiffDays(this->last_update_time_, GetSeconds())) {
    this->ClearWithoutScore();
  }
}

void LegionWar::SetArmyFocusCity(int64_t army_id, int32_t city_id) {
  const std::string& key = MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                            KVStorage::kKVTypeFocus, army_id);
  storage_ext::Save(key, city_id);
}

int32_t LegionWar::GetArmyFocusCity(int64_t army_id) {
  const std::string& key = MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                            KVStorage::kKVTypeFocus, army_id);
  int32_t city_id = 0;
  storage_ext::Load(key, city_id);
  return city_id;
}

void LegionWar::SavePositionToLocal() {
  this->last_update_time_ = GetSeconds();
  storage_ext::Save(MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                     KVStorage::kKVTypeUpdateTime),
                    this->last_update_time_);
  const std::string& value = this->dump();
  storage::Set(
      MakeKVStorageKey(KVStorage::kKVPrefixLegionWar, KVStorage::kKVTypeMap),
      value);
}

void LegionWar::SavePlayer(const sy::OtherPlayerInfo& player) {
  this->last_update_time_ = GetSeconds();
  storage_ext::Save(MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                     KVStorage::kKVTypeUpdateTime),
                    this->last_update_time_);
  const std::string& value = player.SerializeAsString();
  server->SetKeyValue(
      ::MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                         KVStorage::kKVTypeOtherPlayer, player.player_id()),
      value);
  this->players_[player.player_id()].reset(new OtherPlayerInfo(player));
}

LegionCityPtr LegionWar::GetCity(int32_t city) {
  static LegionCityPtr kEmpty;
  const LegionCityBasePtr& base = LEGION_CITY_BASE.GetEntryByID(city);
  if (!base) return kEmpty;
  if (this->cities_.find(city) == this->cities_.end()) {
    this->cities_[city].reset(new LegionCity(city));
  }
  return this->cities_[city];
}

void LegionWar::SendMessageToAll(uint16_t msgid,
                                 google::protobuf::Message* message) {
  for (VectorSet<boost::weak_ptr<TcpSession> >::iterator iter =
           notify_player_.begin();
       iter != notify_player_.end();) {
    const boost::shared_ptr<TcpSession>& session = iter->lock();
    if (session) {
      server->SendMessageToClient(session.get(), msgid, message);
      ++iter;
    } else {
      iter = notify_player_.erase(iter);
    }
  }
}

void LegionWar::SendCitiesToPlayer(LogicPlayer* player) {
  MessageResponseGetLegionWarInfo response;
  for (VectorMap<int32_t, LegionCityPtr>::iterator iter = this->cities_.begin();
       iter != this->cities_.end(); ++iter) {
    sy::LegionWarCity* city = response.add_city();
    city->set_city_id(iter->first);
    for (std::vector<int64_t>::iterator iter_player =
             iter->second->players_.begin();
         iter_player != iter->second->players_.end(); ++iter_player) {
      city->add_players(*iter_player);
    }
  }
  response.set_target(this->GetPlayerTarget(player->uid(), false));
  response.set_score(this->GetPlayerScore(player->uid()));
  response.set_army_score(this->GetArmyScore(player->army_id()));
  response.set_focus_city(this->GetArmyFocusCity(player->army_id()));
  player->SendMessageToClient(MSG_CS_RESPONSE_GET_LEGION_WAR_INFO, &response);
}

void LegionWar::SendPlayersToPlayer(LogicPlayer* player) {
  MessageResponseGetLegionWarPlayer response;
  for (VectorMap<int64_t, OtherPlayerPtr>::iterator iter =
           this->players_.begin();
       iter != this->players_.end(); ++iter) {
    response.add_player()->CopyFrom(*iter->second);
  }
  player->SendMessageToClient(MSG_CS_RESPONSE_GET_LEGION_WAR_PLAYER, &response);
}

std::pair<int32_t, int32_t> LegionWar::GetPlayerPos(int64_t player) {
  for (VectorMap<int32_t, LegionCityPtr>::iterator iter = this->cities_.begin();
       iter != this->cities_.end(); ++iter) {
    int32_t result = iter->second->GetPlayerPosition(player);
    if (result) return std::make_pair(iter->first, result);
  }
  return std::make_pair(0, 0);
}

sy::OtherPlayerInfo* LegionWar::GetPlayerByID(int64_t player_id) {
  if (!player_id) return NULL;
  VectorMap<int64_t, OtherPlayerPtr>::iterator iter =
      this->players_.find(player_id);
  if (iter != this->players_.end()) return iter->second.get();
  const std::string& value = storage::Get(MakeKVStorageKey(
      KVStorage::kKVPrefixLegionWar, KVStorage::kKVTypeOtherPlayer, player_id));
  if (value.empty()) return NULL;
  sy::OtherPlayerInfo player;
  if (!player.ParseFromString(value)) return NULL;
  this->players_[player_id].reset(new OtherPlayerInfo(player));
  return this->players_[player_id].get();
}

sy::OtherPlayerInfo* LegionWar::GetOtherPlayerByPos(
    std::pair<int32_t, int32_t> pos) {
  const LegionCityPtr& city = this->GetCity(pos.first);
  if (!city) return NULL;
  int64_t player = city->get(pos.second);
  if (!player) return NULL;
  return this->GetPlayerByID(player);
}

int32_t LegionWar::GetPlayerScore(int64_t player_id) {
  boost::unordered_map<int64_t, int32_t>::iterator iter = this->player_score_.find(player_id);
  if (iter != this->player_score_.end()) return iter->second;
  int32_t score = 0;
  storage_ext::Load(MakeKVStorageKey(KVStorage::kKVPrefixLegionWarScore,
                                     KVStorage::kKVTypeScore, player_id),
                    score);
  this->player_score_[player_id] = score;
  return score;
}

int32_t LegionWar::AddPlayerScore(int64_t player_id, int32_t add) {
  int32_t score = this->GetPlayerScore(player_id);
  this->player_score_[player_id] += add;
  storage_ext::Save(MakeKVStorageKey(KVStorage::kKVPrefixLegionWarScore,
                                     KVStorage::kKVTypeScore, player_id),
                    score + add);
  INFO_LOG(logger)("LegionWar::AddPlayerScore, PlayerID:%ld, AddScore:%d, Score:%d"
      , player_id, add, score + add);
  return score + add;
}

int32_t LegionWar::GetArmyScore(int64_t army_id) {
  int32_t score = 0;
  storage_ext::Load(
      MakeKVStorageKey(KVStorage::kKVPrefixLegionWarScore, army_id), score);
  return score;
}

void LegionWar::SetArmyScore(int64_t army_id, int32_t score) {
  storage_ext::Save(
      MakeKVStorageKey(KVStorage::kKVPrefixLegionWarScore, army_id), score);
}

//个人跨服排行
static std::vector<sy::RankItemInfo> UpdatePlayerScoreCache;

static inline void FillRankItemByOtherPlayer(sy::RankItemInfo& info,
                                             sy::OtherPlayerInfo& player,
                                             int32_t score) {
  info.set_uid(player.player_id());
  info.set_name(player.name());
  info.set_star(score);
  info.set_damage(server_config->server_id());
  info.set_level(player.level());
  info.set_vip_level(player.vip_level());
  info.set_avatar(player.avatar());
  info.set_army_name(player.army_name());
}

static inline void Push(std::vector<sy::RankItemInfo>& v,
                        sy::RankItemInfo& info) {
  for (std::vector<sy::RankItemInfo>::iterator iter = v.begin();
       iter != v.end(); ++iter) {
    if (iter->uid() == info.uid()) {
      v.erase(iter);
      break;
    }
  }
  v.push_back(info);
}

static inline void SendLegionWarPlayerScore(sy::OtherPlayerInfo& player,
                                            int32_t city, int32_t pos,
                                            VectorSet<int64_t>& set) {
  if (player.server() != server_config->server_id()) {
    INFO_LOG(logger)("PlayerID:%ld, ServerID:%u, Skipped"
        , player.player_id(), player.server());
    return;
  }
  const LegionCityBase* base = LEGION_CITY_BASE.GetEntryByID(city).get();
  if (!base) {
    ERROR_LOG(logger)("SendLegionWarPlayerScore, PlayerID:%ld, City:(%d-%d) not found"
        , player.player_id(), city, pos);
    return;
  }
  const std::vector<int32_t>& scores = GetLegionPersonalAward(base->quality);
  int32_t add_score = 0;
  if (pos >= 1 && pos <= (int32_t)scores.size()) {
    add_score = scores[pos - 1];
  }
  if (add_score <= 0) return;
  INFO_LOG(logger)("SendLegionWarPlayerScore, PlayerID:%ld, City:(%d-%d), AddScore:%d"
      , player.player_id(), city, pos, add_score);
  int32_t score = LegionWar::Instance().AddPlayerScore(player.player_id(), add_score);
  LogicPlayer* logic_player = server->GetPlayerByID(player.player_id());
  if (logic_player) {
    MessageRespopnseGetLegionWarTargetAward response;
    response.set_score(score);
    logic_player->SendMessageToClient(MSG_CS_RESPONSE_GET_LEGION_WAR_TARGET_AWARD, &response);
  }

  DefaultArrayStream stream;
  stream.Append("%d,%d,%d", city, pos, add_score);
  LogicPlayer::SendMail(player.player_id(), GetSeconds(),
                        sy::MAIL_TYPE_LEGION_WAR_SCORE, stream.str(), NULL);

  sy::RankItemInfo info;
  FillRankItemByOtherPlayer(info, player, score);
  if (player.army_id()) set.insert(player.army_id());
  RANK_LIST.OnLegionWar(sy::RANK_TYPE_LEGION_WAR_1, info);
  if (score >= GetSettingValue(legion_war_region_score1))
    Push(UpdatePlayerScoreCache, info);
}

//army id => quality
static inline void SendArmyOccupyAward(std::pair<int64_t, int32_t> army_info,
                                       int32_t city_id,
                                       std::vector<sy::RankItemInfo>& cache) {
  //发送占领城池奖励
  Army* army = server->GetArmyByID(army_info.first);
  if (!army) {
    ERROR_LOG(logger)("SendArmyOccupyAward, ArmyID:%ld not found", army_info.first);
    return;
  }
  const LegionAwardBase* base = LEGION_AWARD_BASE.GetEntryByID(army_info.second).get();
  if (!base) {
    ERROR_LOG(logger)("SendArmyOccupyAward, quality:%d not found", army_info.second);
    return;
  }
  std::vector<std::pair<int32_t, int32_t> > award;
  award.push_back(std::make_pair(23900063, base->money));
  DefaultArrayStream stream;
  stream.Append("%d", city_id);
  army->SendMailToArmyMember(MAIL_TYPE_LEGION_WAR_3, stream.str(), &award);

  TRACE_LOG(logger)("SendArmyOccupyAward, ArmyID:%ld, City:%d, quality:%d, Money:%d, Score:%d"
      , army->army_id(), city_id, army_info.second, base->money, base->rank);
  //发送占领城池积分
  for (std::vector<ArmyMemberInfo>::iterator iter = army->members().begin();
       iter != army->members().end(); ++iter) {
    sy::RankItemInfo info;
    sy::OtherPlayerInfo* player = LegionWar::Instance().GetPlayerByID(iter->player_id());
    if (!player) continue;
    int32_t score = LegionWar::Instance().AddPlayerScore(iter->player_id(), base->rank);
    LogicPlayer* logic_player = server->GetPlayerByID(iter->player_id());
    if (logic_player) {
      MessageRespopnseGetLegionWarTargetAward response;
      response.set_score(score);
      logic_player->SendMessageToClient(
          MSG_CS_RESPONSE_GET_LEGION_WAR_TARGET_AWARD, &response);
    }
    FillRankItemByOtherPlayer(info, *player, score);
    RANK_LIST.OnLegionWar(sy::RANK_TYPE_LEGION_WAR_1, info);
    Push(cache, info);
  }
}

//发送制霸全球占领积分
void Server::SendLegionWarScore() {
  UpdatePlayerScoreCache.clear();
  VectorSet<int64_t> army_set;
  TRACE_LOG(logger)("SendLegionWarScore");
  LegionWar& war = LegionWar::Instance();
  for (VectorMap<int32_t, LegionCityPtr>::iterator iter = war.cities_.begin();
       iter != war.cities_.end(); ++iter) {
    const LegionCityPtr& ptr = iter->second;
    for (size_t index = 1; index < ptr->players_.size(); ++index) {
      int64_t player_id = ptr->players_[index];
      if (!player_id) continue;
      sy::OtherPlayerInfo* player = war.GetPlayerByID(player_id);
      if (!player) {
        ERROR_LOG(logger)("SendLegionWarScore, PlayerID:%ld, City:(%d-%lu)"
            , player_id, iter->first, index);
        continue;
      }
      SendLegionWarPlayerScore(*player, iter->first, index, army_set);
    }
    std::pair<int64_t, int32_t> result = ptr->GetArmy();
    DEBUG_LOG(logger)("SendLegionWarScore, CityID:%d(Quality:%d), ArmyID:%ld"
          , ptr->city_id(), result.second, result.first);
    if (result.first)
      SendArmyOccupyAward(result, ptr->city_id(), UpdatePlayerScoreCache);
  }
  if (!UpdatePlayerScoreCache.empty())
    server->SendCrossServerRankItem(UpdatePlayerScoreCache,
                                    sy::RANK_TYPE_LEGION_WAR_2);
  //统计军团的积分变化
  war.CalcArmyScore(army_set);
}

int32_t LegionWar::CalcArmyScore(VectorSet<int64_t>& s) {
  static std::vector<sy::RankItemInfo> update_army_score;
  update_army_score.clear();

  int32_t score = 0;
  for (VectorSet<int64_t>::iterator iter = s.begin(); iter != s.end(); ++iter) {
    Army* army = server->GetArmyByID(*iter);
    if (!army) {
      INFO_LOG(logger)("CalcArmyScore, ArmyID:%ld not found", *iter);
      continue;
    }
    score = 0;
    for (std::vector<sy::ArmyMemberInfo>::const_iterator iter_member =
             army->members().begin();
         iter_member != army->members().end(); ++iter_member) {
      score += this->GetPlayerScore(iter_member->player_id());
    }
    INFO_LOG(logger)("CalcArmyScore, ArmyID:%ld, Score:%d", army->army_id(), score);

    sy::RankItemInfo info;
    info.set_uid(army->army_id());
    info.set_name(army->name());
    info.set_star(score);
    info.set_army_name(army->name());
    info.set_avatar(army->info().avatar());
    info.set_damage(server_config->server_id());
    this->SetArmyScore(army->army_id(), score);
    if (score >= GetSettingValue(legion_war_region_score2))
      update_army_score.push_back(info);
  }
  if (!update_army_score.empty())
    server->SendCrossServerRankItem(update_army_score, RANK_TYPE_LEGION_WAR_3);
  return score;
}

void Server::SendLegionWarAward() {
  //发送制霸全球排名奖励:
  //个人排行榜奖励
  //军团排行榜奖励
  RankListBase& rank1 = RANK_LIST.GetByType(sy::RANK_TYPE_LEGION_WAR_1);
  for (int32_t i = 0; i < rank1.data().items_size(); ++i) {
    sy::OtherPlayerInfo* player = LegionWar::Instance().GetPlayerByID(rank1.data().items(i).uid());
    if (!player) continue;
    if (player->server() == server_config->server_id()) {
      const LegionPlayerRewardBase* base = LEGION_PLAYER_REWARD_BASE.GetEntryByID(i + 1).get();
      if (!base) continue;
      DEBUG_LOG(logger)("SendLegionWarAward, PlayerID:%ld, Rank:%d", player->player_id(), i + 1);
      DefaultArrayStream stream;
      stream.Append("%d", i + 1);
      LogicPlayer::SendMail(player->player_id(), GetSeconds(),
                            sy::MAIL_TYPE_LEGION_WAR_1, stream.str(),
                            &base->award);
    }
  }

  RankListBase& rank3 = RANK_LIST.GetByType(sy::RANK_TYPE_LEGION_WAR_3);
  for (int32_t i = 0; i < rank3.data().items_size(); ++i) {
    int64_t army_id = rank3.data().items(i).uid();
    Army* army = server->GetArmyByID(army_id);
    if (!army) continue;
    const LegionArmyRewardBase* base = LEGION_ARMY_REWARD_BASE.GetEntryByID(i + 1).get();
    if (!base) continue;
    DEBUG_LOG(logger)("SendLegionWarAward, ArmyID:%ld, Rank:%d", army_id, i + 1);
    DefaultArrayStream stream;
    stream.Append("%d", i + 1);
    army->SendMailToArmyMember(sy::MAIL_TYPE_LEGION_WAR_2, stream.str(),
                               &base->award);
  }
}

int64_t LegionWar::GetPlayerTarget(int64_t player_id, bool refresh) {
  if (GetTime().tm_hour < 12) return 0;

  const std::string& key = MakeKVStorageKey(
      KVStorage::kKVPrefixLegionWar, KVStorage::kKVTypeTarget, player_id);
  int64_t target = 0;
  if (!refresh) {
    storage_ext::Load(key, target);
  }
  if (target && this->GetPlayerByID(target)) {
    return target;
  }
  if (this->players_.size() <= 2lu) {
    ERROR_LOG(logger)("LegionWar PlayerCount:%lu, Cannot refresh a target", this->players_.size());
    return 0;
  }

  do {
    int32_t rand = RandomBetween(0, this->players_.size() - 1);
    OtherPlayerPtr& ptr = (this->players_.begin() + rand)->second;
    target = ptr->player_id();
  } while (target == player_id || !target);

  DEBUG_LOG(logger)("GetPlayerTarget, PlayerID:%ld, Target:%ld", player_id, target);
  storage_ext::Save(key, target);
  return target;
}

std::string LegionWar::dump() const {
  ArrayStream<1024 * 128> stream;
  for (VectorMap<int32_t, LegionCityPtr>::const_iterator iter = this->cities_.begin();
       iter != this->cities_.end(); ++iter) {
    const std::vector<int64_t>& players = iter->second->players();
    for (size_t index = 1; index < players.size(); ++index) {
      if (players[index]) {
        if (stream.size()) stream.Append("\n");
        stream.Append("%d-%lu,%ld", iter->first, index, players[index]);
      }
    }
  }

  return stream.str();
}
