#ifndef __CORAL_SEA_H__
#define __CORAL_SEA_H__
#include <noncopyable.h>
#include <common_define.h>
#include <cpp/message.pb.h>
#include <singleton.h>
#include "server.h"

class CenterCoralSeaTeam : NonCopyable {
 public:
  CenterCoralSeaTeam();
  ~CenterCoralSeaTeam();

  int64_t id() const { return this->id_; }

  //true是成功
  //is_dead是true,就是死人,否则就是活人
  bool AddPlayer(const sy::OtherPlayerInfo& player, bool is_dead, bool notify);
  void RemovePlayer(int64_t player_id);
  void SetPlayerStatus(int64_t player_id, int32_t status);
  void SetPlayerStatusWithoutNotify(int32_t index, int32_t status);
  void SetPlayerDeadmanWithoutNoitfy(int32_t index, int32_t status);
  void SendMeToEveryOne(int64_t player_id_leave);

  void MakeOneBattle(const std::vector<sy::OtherPlayerInfo>& player_b);

  std::pair<int32_t, sy::OtherPlayerInfo*> GetPlayer(int64_t player_id);

  int32_t level() const { return this->level_; }
  uint32_t server_id() const { return this->server_id_; }
  int32_t player_count() const;
  int32_t dead_man_count() const;
  int32_t ready_count() const;

  const sy::CoralSeaTeam& data() const { return this->data_; }
  std::string DebugStr() const;
 private:
  int64_t id_;
  uint32_t server_id_;
  int32_t level_;
  sy::CoralSeaTeam data_;
};

typedef boost::shared_ptr<CenterCoralSeaTeam> CoralSeaTeamPtr;

struct MatchingPlayerInfo {
  int64_t player_id;
  uint32_t server_id;
  int32_t level;
  int32_t update_time;
};

class CenterCoralSeaTeamManager : NonCopyable {
 public:
  CenterCoralSeaTeamManager();

  void OnSecondChanged();

  void AutoFillDeadman(int64_t team_id);
  CoralSeaTeamPtr GetTeam(int64_t team_id);

  void RemovePlayer(int64_t player);
  CoralSeaTeamPtr CreateTeam(const sy::OtherPlayerInfo& player_info);
  void JoinTeam(int64_t team_id, const sy::OtherPlayerInfo& player_info);
  void SearchTeam(const sy::OtherPlayerInfo& player_info);

  void Dump() const;
 private:
  void GetUnFullTeam(VectorSet<CoralSeaTeamPtr>& teams);
  void TryMatchTeam(VectorSet<CoralSeaTeamPtr>& teams,
                    boost::unordered_map<int64_t, MatchingPlayerInfo>& players);
  void TryMakeBattle(VectorSet<int64_t>& team);

  sy::OtherPlayerInfo* GetPlayer(int64_t player_id);
  void GetPlayerIDByServerIDAndLevel(
      const boost::unordered_map<int64_t, MatchingPlayerInfo>& players,
      uint32_t server_id, int32_t level_min, int32_t level_max,
      VectorSet<sy::OtherPlayerInfo*>& out);

 private:
  boost::unordered_map<int64_t, CoralSeaTeamPtr> teams_;
  boost::unordered_map<int64_t, sy::OtherPlayerInfo> players_;
  uint32_t matching_index_;
  std::vector<boost::unordered_map<int64_t, MatchingPlayerInfo> > matching_player_;
  uint32_t ready_index_;
  std::vector<VectorSet<int64_t> > ready_team_;
};

#define TEAM_MANAGER Singleton<CenterCoralSeaTeamManager>::Instance()

#endif
