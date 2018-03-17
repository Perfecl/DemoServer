#ifndef __LEGION_WAR_H__
#define __LEGION_WAR_H__
#include "config.h"
#include <singleton.h>
#include <noncopyable.h>
#include <vector_set.h>
#include <vector_map.h>
#include <boost/shared_ptr.hpp>

class City {
 public:
  City(int32_t city_id) : city_id_(city_id) {
    this->players_.reserve(100);
  }

  int32_t city_id() const { return this->city_id_; }

  std::vector<int64_t>& players() { return this->players_; }
  const std::vector<int64_t>& players() const { return this->players_; }

  int64_t get(int32_t pos) const {
    if (pos <= 0) return 0;
    return pos < (int32_t)this->players_.size() ? this->players_[pos] : 0;
  }

  int32_t GetPlayerPosition(int64_t player) const {
    std::vector<int64_t>::const_iterator iter =
        std::find(this->players_.begin(), this->players_.end(), player);
    return iter != this->players_.end()
               ? std::distance(this->players_.begin(), iter)
               : 0;
  }

  //position从1开始
  void set(int32_t position, int64_t player) {
    if (position >= (int32_t)this->players_.size()) {
      this->players_.resize(position + 1, 0);
    }
    this->players_[position] = player;
  }

 private:
  friend class LegionWar;
  int32_t city_id_;
  std::vector<int64_t> players_;
};

class LegionWar;
typedef boost::shared_ptr<LegionWar> LegionWarPtr;
//暂定10个区映射到一个战区上
//server_id / 10 => LegionWar
class LegionWar : NonCopyable {
 public:
   LegionWar(int32_t region);
   ~LegionWar();
   typedef boost::shared_ptr<City> CityPtr;

   //注册新玩家
   //CityID是0, 就是满了
   std::pair<int32_t, int32_t> Register(int64_t player);
   //交换位置
   void SwapPosition(std::pair<int32_t, int32_t> a,
                     std::pair<int32_t, int32_t> b);

   static LegionWarPtr GetLegionWar(int32_t region);
   static void ClearAll();

   bool HasPlayer(int64_t player_id);

  std::string Dump() const;

 private:
   void SaveToLocal();
   void LoadFromLocal();
   void Clear();
   CityPtr GetCity(int32_t city);

 private:
  int32_t region_;
  int64_t last_update_time_;
  VectorSet<int64_t> players_;
  VectorMap<int32_t, CityPtr> cities_;
};

#endif
