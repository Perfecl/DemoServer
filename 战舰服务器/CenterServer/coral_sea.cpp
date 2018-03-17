#include "coral_sea.h"
#include "config.h"
#include <cpp/server_message.pb.h>
#include <array_stream.h>
#include <myrandom.h>
#include "pk.h"

CenterCoralSeaTeam::CenterCoralSeaTeam() : id_(server->GetTID()) {
  this->data_.set_team_id(this->id());
  for (int32_t i = 0; i < 5; ++i) {
    this->data_.add_player()->set_player_id(0);
  }
  DEBUG_LOG(logger)("Team Create, ID:%ld", this->id());
}

CenterCoralSeaTeam::~CenterCoralSeaTeam() {
  DEBUG_LOG(logger)("Team Dispose, ID:%ld", this->id());
}

std::string CenterCoralSeaTeam::DebugStr() const {
  ArrayStream<1024*16> stream;
  stream.Append("TeamID:%ld, LeadID:%ld", this->id(), this->data_.leader_id());
  for (int32_t i = 0; i < this->data_.player_size(); ++i) {
    stream.Append(", P%d:(%ld-%d)", i, this->data_.player(i).player_id(),
                  bool(this->data_.ready_status() & (1 << i)));
  }
  return stream.str();
}

std::pair<int32_t, sy::OtherPlayerInfo*> CenterCoralSeaTeam::GetPlayer(
    int64_t player_id) {
  for (int32_t i = 0; i < this->data_.player_size(); ++i) {
    if (this->data_.player(i).player_id() == player_id) {
      return std::make_pair(i, this->data_.mutable_player(i));
    }
  }
  return std::make_pair<int32_t, sy::OtherPlayerInfo*>(0, NULL);
}

bool CenterCoralSeaTeam::AddPlayer(const sy::OtherPlayerInfo& player,
                                   bool is_dead, bool notify) {
  //第一个人是队长
  //每个进来的人, 都把自己的状态设置对
  if (this->player_count() >= 5) return false;
  std::pair<int32_t, sy::OtherPlayerInfo*> pair = this->GetPlayer(player.player_id());
  if (pair.second) return true;
  for (int32_t i = 0; i < this->data_.player_size(); ++i) {
    if (!this->data_.player(i).player_id()) {
      this->data_.mutable_player(i)->CopyFrom(player);
      this->SetPlayerStatusWithoutNotify(i, true);
      this->SetPlayerDeadmanWithoutNoitfy(i, is_dead);
      break;
    }
  }
  if (this->player_count() == 1) {
    this->data_.set_leader_id(player.player_id());
  }
  if (notify) this->SendMeToEveryOne(0);
  return true;
}

void CenterCoralSeaTeam::SendMeToEveryOne(int64_t player_id_leave) {
  intranet::MessageSSNotifyCoralSeaTeam notify;
  notify.set_player_leave(player_id_leave);
  notify.mutable_team()->CopyFrom(this->data_);
  VectorSet<uint32_t> server_ids;
  for (int32_t i = 0; i < this->data_.player_size(); ++i) {
    if (this->data_.player(i).server_id() &&
        !(this->data_.dead_man() & (1 << i)))
      server_ids.insert(this->data_.player(i).server_id());
  }
  for (VectorSet<uint32_t>::iterator iter = server_ids.begin();
       iter != server_ids.end(); ++iter) {
    server->SendServerMessage(
        *iter, intranet::MSG_SS_NOTIFY_CORAL_SEA_TEAM, &notify);
  }
}

void CenterCoralSeaTeam::SetPlayerStatusWithoutNotify(int32_t index,
                                                      int32_t status) {
  int32_t s = this->data_.ready_status();
  if (status) {
    s |= (1u << index);
  } else {
    s &= ~(1u << index);
  }
  this->data_.set_ready_status(s);
}

void CenterCoralSeaTeam::SetPlayerDeadmanWithoutNoitfy(int32_t index,
                                                       int32_t status) {
  int32_t s = this->data_.dead_man();
  if (status) {
    s |= (1u << index);
  } else {
    s &= ~(1u << index);
  }
  this->data_.set_dead_man(s);
}

void CenterCoralSeaTeam::RemovePlayer(int64_t player_id) {
  std::pair<int32_t, sy::OtherPlayerInfo*> pair = this->GetPlayer(player_id);
  if (!pair.second) return;
  pair.second->Clear();
  pair.second->set_player_id(0);
  this->SetPlayerStatusWithoutNotify(pair.first, false);
  this->SetPlayerDeadmanWithoutNoitfy(pair.first, false);
  if (this->data_.leader_id() == player_id) {
    this->data_.set_leader_id(0);
  }
  if (!this->data_.leader_id()) {
    for (int32_t i = 0; i < this->data_.player_size(); ++i) {
      if (this->data_.player(i).player_id()) {
        this->data_.set_leader_id(this->data_.player(i).player_id());
        break;
      }
    }
  }
  this->SendMeToEveryOne(player_id);
}

void CenterCoralSeaTeam::SetPlayerStatus(int64_t player_id, int32_t status) {
  std::pair<int32_t, sy::OtherPlayerInfo*> pair = this->GetPlayer(player_id);
  if (pair.second) {
    int32_t s = this->data_.ready_status();
    if (status) {
      s |= (1u << pair.first);
    } else {
      s &= ~(1u << pair.first);
    }
    this->data_.set_ready_status(s);
  }
  this->SendMeToEveryOne(0);
}

int32_t CenterCoralSeaTeam::player_count() const {
  int32_t count = 0;
  for (int32_t i = 0; i < this->data_.player_size(); ++i) {
    count += this->data_.player(i).player_id() ? 1 : 0;
  }
  return count;
}

int32_t CenterCoralSeaTeam::dead_man_count() const {
  int32_t count = 0;
  for (int32_t i = 0; i < this->data_.player_size(); ++i) {
    int64_t player_id = this->data_.player(i).player_id() ? 1 : 0;
    if (!player_id) continue;
    if (this->data_.dead_man() & (1 << i)) count += 1;
  }
  return count;
}

int32_t CenterCoralSeaTeam::ready_count() const {
  int32_t count = 0;
  for (int32_t i = 0; i < this->data_.player_size(); ++i) {
    if (this->data_.leader_id() &&
        this->data_.player(i).player_id() == this->data_.leader_id()) {
      count += 1;
      continue;
    }
    count += (this->data_.ready_status() & (1 << i)) ? 1 : 0;
  }
  return count;
}

void CenterCoralSeaTeam::MakeOneBattle(
    const std::vector<sy::OtherPlayerInfo>& player_b) {
  VectorSet<int64_t> win_count;
  std::vector<int64_t> hp_a;
  std::vector<int64_t> hp_b;
  int32_t index_a = 0, index_b = 0;
  while (index_a < 5 && index_b < 5) {
    
  }
  //TODO:egmkang
}

CenterCoralSeaTeamManager::CenterCoralSeaTeamManager()
    : matching_index_(0), ready_index_(0) {
  this->matching_player_.resize(5);
  this->ready_team_.resize(10);
}

void CenterCoralSeaTeamManager::GetUnFullTeam(
    VectorSet<CoralSeaTeamPtr>& teams) {
  for (boost::unordered_map<int64_t, CoralSeaTeamPtr>::iterator iter =
           this->teams_.begin();
       iter != this->teams_.end(); ++iter) {
    if (iter->second) {
      int32_t player_count = iter->second->player_count();
      if (player_count && player_count < 5) teams.insert(iter->second);
    }
  }
}

sy::OtherPlayerInfo* CenterCoralSeaTeamManager::GetPlayer(int64_t player_id) {
  boost::unordered_map<int64_t, sy::OtherPlayerInfo>::iterator iter = this->players_.find(player_id);
  return iter != this->players_.end() ? &iter->second : NULL;
}

void CenterCoralSeaTeamManager::OnSecondChanged() {
  int32_t count = this->matching_index_++;
  boost::unordered_map<int64_t, MatchingPlayerInfo>& players =
      this->matching_player_[count % matching_player_.size()];
  VectorSet<CoralSeaTeamPtr> teams;
  this->GetUnFullTeam(teams);
  this->TryMatchTeam(teams, players);

  VectorSet<int64_t>& ready_teams =
      this->ready_team_[this->ready_index_++ % this->ready_team_.size()];
  this->TryMakeBattle(ready_teams);
}

void CenterCoralSeaTeamManager::GetPlayerIDByServerIDAndLevel(
    const boost::unordered_map<int64_t, MatchingPlayerInfo>& players,
    uint32_t server_id, int32_t level_min, int32_t level_max,
    VectorSet<sy::OtherPlayerInfo*>& out) {
  for (boost::unordered_map<int64_t, MatchingPlayerInfo>::const_iterator iter =
           players.begin();
       iter != players.end(); ++iter) {
    const MatchingPlayerInfo& info = iter->second;
    if ((server_id - 1) / SettingConfigFile::GetRegionSize() !=
        (info.server_id - 1) / SettingConfigFile::GetRegionSize())
      continue;
    if (info.level < level_min || info.level > level_max) continue;
    sy::OtherPlayerInfo* p = this->GetPlayer(info.player_id);
    if (p) out.insert(p);
  }
}

void CenterCoralSeaTeamManager::TryMatchTeam(
    VectorSet<CoralSeaTeamPtr>& teams,
    boost::unordered_map<int64_t, MatchingPlayerInfo>& players) {
  VectorSet<sy::OtherPlayerInfo*> temp_players;
  for (VectorSet<CoralSeaTeamPtr>::iterator iter = teams.begin();
       iter != teams.end(); ++iter) {
    temp_players.clear();
    CoralSeaTeamPtr& ptr = *iter;
    if (!ptr->level()) continue;
    uint32_t server_id = ptr->server_id();
    int32_t empty_count = 5 - ptr->player_count();
    int32_t begin_level = ptr->level() - 5, end_level = ptr->level() + 5;
    this->GetPlayerIDByServerIDAndLevel(players, server_id, begin_level,
                                        end_level, temp_players);
    for (int32_t i = 0; i < empty_count && !temp_players.empty(); ++i) {
      ptr->AddPlayer(**temp_players.begin(), false, true);
      temp_players.erase(temp_players.begin());
    }
  }
}

void CenterCoralSeaTeamManager::TryMakeBattle(VectorSet<int64_t>& team) {
  //TODO:egmkang
}

CoralSeaTeamPtr CenterCoralSeaTeamManager::GetTeam(int64_t team_id) {
  static CoralSeaTeamPtr kEmpty;
  boost::unordered_map<int64_t, CoralSeaTeamPtr>::iterator iter =
      this->teams_.find(team_id);
  return iter != this->teams_.end() ? iter->second : kEmpty;
}

void CenterCoralSeaTeamManager::AutoFillDeadman(int64_t team_id) {
  const CoralSeaTeamPtr& team = this->GetTeam(team_id);
  if (team) {
    VectorSet<int64_t> except;
    if (team->data().leader_id()) except.insert(team->data().leader_id());
    for (int32_t i = 0; i < team->data().player_size(); ++i) {
      except.insert(team->data().player(i).player_id());
    }
    const std::vector<sy::OtherPlayerInfo>& players =
        server->SearchPlayerExcept(team->level(), 5 - team->player_count(),
                                   except);
    for (std::vector<sy::OtherPlayerInfo>::const_iterator iter =
             players.begin();
         iter != players.end(); ++iter) {
      team->AddPlayer(*iter, true, false);
    }
    team->SendMeToEveryOne(0);
  }
}

void CenterCoralSeaTeamManager::RemovePlayer(int64_t player) {
  for (boost::unordered_map<int64_t, CoralSeaTeamPtr>::iterator iter =
           this->teams_.begin();
       iter != this->teams_.end(); ++iter) {
    const CoralSeaTeamPtr& ptr = iter->second;
    if (ptr) ptr->RemovePlayer(player);
  }
  for (size_t index = 0; index < this->matching_player_.size(); ++index) {
    this->matching_player_[index].erase(player);
  }
  this->players_.erase(player);
}

CoralSeaTeamPtr CenterCoralSeaTeamManager::CreateTeam(
    const sy::OtherPlayerInfo& player_info) {
  this->RemovePlayer(player_info.player_id());
  int64_t team_id = server->GetTID();
  CoralSeaTeamPtr ptr(new CenterCoralSeaTeam());
  this->teams_[team_id] = ptr;
  ptr->AddPlayer(player_info, false, true);
  return ptr;
}

void CenterCoralSeaTeamManager::JoinTeam(
    int64_t team_id, const sy::OtherPlayerInfo& player_info) {
  this->RemovePlayer(player_info.player_id());
  const CoralSeaTeamPtr& ptr = this->GetTeam(team_id);
  if (ptr && ptr->player_count() < 5) {
    ptr->AddPlayer(player_info, false, true);
  }
}

void CenterCoralSeaTeamManager::SearchTeam(
    const sy::OtherPlayerInfo& player_info) {
  this->RemovePlayer(player_info.player_id());
  int32_t index = RandomBetween(0, this->matching_player_.size() - 1);
  MatchingPlayerInfo info = {player_info.player_id(), player_info.server(),
                             player_info.level(), GetSeconds()};
  this->matching_player_[index][player_info.player_id()] = info;
  this->players_[player_info.player_id()] = player_info;
}

void CenterCoralSeaTeamManager::Dump() const {
  for (boost::unordered_map<int64_t, CoralSeaTeamPtr>::const_iterator iter =
           this->teams_.begin();
       iter != this->teams_.end(); ++iter) {
    const std::string& s = iter->second->DebugStr();
    DEBUG_LOG(logger)("%s", s.c_str());
  }
  for (boost::unordered_map<int64_t, sy::OtherPlayerInfo>::const_iterator iter =
           this->players_.begin();
       iter != this->players_.end(); ++iter) {
    DEBUG_LOG(logger)("Players:%ld", iter->first);
  }
  for (std::vector<boost::unordered_map<int64_t, MatchingPlayerInfo> >::
           const_iterator iter = this->matching_player_.begin();
       iter != this->matching_player_.end(); ++iter) {
    for (boost::unordered_map<int64_t, MatchingPlayerInfo>::const_iterator
             iter_a = iter->begin();
         iter_a != iter->end(); ++iter_a) {
      DEBUG_LOG(logger)("MatchingPlayer[%ld], PlayerID:%ld, Server:%u, Level:%d"
          , iter - this->matching_player_.begin()
          , iter_a->second.player_id
          , iter_a->second.server_id
          , iter_a->second.level);
    }
  }
  for (std::vector<VectorSet<int64_t> >::const_iterator iter =
           this->ready_team_.begin();
       iter != this->ready_team_.end(); ++iter) {
    for (VectorSet<int64_t>::const_iterator iter_a = iter->begin();
         iter_a != iter->end(); ++iter_a) {
      DEBUG_LOG(logger)("ReadyTeam[%ld], TeamID:%ld"
          , iter - this->ready_team_.begin()
          , *iter_a);
    }
  }
  DEBUG_LOG(logger)("MatchingIndex:%u, ReadyIndex:%u"
      , this->matching_index_, this->ready_index_);
}
