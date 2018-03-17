#ifndef __LEGION_WAR_H__
#define __LEGION_WAR_H__
#include <singleton.h>
#include <noncopyable.h>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <vector_map.h>
#include <vector_set.h>
#include <cpp/message.pb.h>
#include <boost/unordered_map.hpp>

class LegionCity {
 public:
  LegionCity(int32_t city_id) : city_id_(city_id) {}

  int32_t city_id() const { return this->city_id_; }
  std::vector<int64_t>& players() { return this->players_; }

  int64_t get(int32_t pos) const {
    if (pos <= 0) return 0;
    return pos < (int32_t) this->players_.size() ? this->players_[pos] : 0;
  }

  int32_t GetPlayerPosition(int64_t player) const {
    std::vector<int64_t>::const_iterator iter =
        std::find(this->players_.begin(), this->players_.end(), player);
    return iter != this->players_.end()
               ? std::distance(this->players_.begin(), iter)
               : 0;
  }

  //ArmyID => Quality
  std::pair<int64_t, int32_t> GetArmy();

  //从1开始
  void set(int32_t position, int64_t player) {
    if (position >= (int32_t) this->players_.size()) {
      this->players_.resize(position + 1, 0);
    }
    this->players_[position] = player;
  }

 private:
  friend class LegionWar;
  friend class Server;
  int32_t city_id_;
  std::vector<int64_t> players_;
};

typedef boost::shared_ptr<LegionCity> LegionCityPtr;
class TcpSession;
class LogicPlayer;

class LegionWar : NonCopyable, public Singleton<LegionWar> {
 public:
   LegionWar();

  void NewPlayer(sy::OtherPlayerInfo& player, std::pair<int32_t, int32_t> pos);

  void SwapPos(std::pair<int32_t, int32_t> pos1,
               std::pair<int32_t, int32_t> pos2);

  void ClearWithoutScore();
  void ClearAll();

  void LoadFromLocal();
  void Subscribe(LogicPlayer* player);
  void SetArmyFocusCity(int64_t army_id, int32_t city_id);
  int32_t GetArmyFocusCity(int64_t army_id);

  std::pair<int32_t, int32_t> GetPlayerPos(int64_t player);

  sy::OtherPlayerInfo* GetOtherPlayerByPos(std::pair<int32_t, int32_t> pos);

  void SavePlayer(const sy::OtherPlayerInfo& player);

  int32_t GetPlayerScore(int64_t player_id);
  int32_t AddPlayerScore(int64_t player_id, int32_t add);

  int64_t GetPlayerTarget(int64_t player_id, bool refresh);

  int32_t GetArmyScore(int64_t army_id);
  void SetArmyScore(int64_t army_id, int32_t score);

 public:
  void SendCitiesToPlayer(LogicPlayer* player);
  void SendPlayersToPlayer(LogicPlayer* player);
  void SendMessageToAll(uint16_t msgid, google::protobuf::Message* message);
  int32_t CalcArmyScore(VectorSet<int64_t>& set);
  sy::OtherPlayerInfo* GetPlayerByID(int64_t player);

  std::string dump() const;
 private:
  friend class Server;
  LegionCityPtr GetCity(int32_t city);
  void SavePositionToLocal();
 private:
  int64_t last_update_time_;
  VectorMap<int32_t, LegionCityPtr> cities_;
  typedef boost::shared_ptr<sy::OtherPlayerInfo> OtherPlayerPtr;
  VectorMap<int64_t, OtherPlayerPtr> players_;
  boost::unordered_map<int64_t, int32_t> player_score_;
 private:
  //需要通知的玩家
  VectorSet<boost::weak_ptr<TcpSession> > notify_player_;
};

#endif
