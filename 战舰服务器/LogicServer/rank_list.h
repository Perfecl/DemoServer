#ifndef __RANK_LIST_H__
#define __RANK_LIST_H__
#include <noncopyable.h>
#include <vector>
#include <cpp/message.pb.h>
#include <singleton.h>

typedef sy::RankItemInfo RankItem;
typedef sy::RankList RankItemContainer;
class Army;

struct CompareRankLevel {
  bool operator()(const RankItem& v1, const RankItem& v2) const { return v1.level() > v2.level(); }
};
struct CompareRankStar {
  bool operator()(const RankItem& v1, const RankItem& v2) const { return v1.star() > v2.star(); }
};
struct CompareRankExploit {
  bool operator()(const RankItem& v1, const RankItem& v2) const { return v1.exploit() > v2.exploit(); }
};
struct CompareRankDamage {
  bool operator()(const RankItem& v1, const RankItem& v2) const { return v1.damage() > v2.damage(); }
};
struct CompareRankFight {
  bool operator()(const RankItem& v1, const RankItem& v2) const { return v1.fight_attr() > v2.fight_attr(); }
};

class RankListBase : NonCopyable {
 public:
  virtual ~RankListBase() {}
  virtual bool Update(RankItem& item) = 0;
  virtual void Remove(int64_t uid) = 0;
  virtual void Clear() = 0;
  virtual int32_t GetRankByUID(int64_t uid) const = 0;
  virtual const RankItem* GetRankInfoByUID(int64_t uid) const = 0;
  virtual const RankItem* GetLast() const = 0;
  virtual const RankItemContainer& data() const = 0;
  virtual void Update(const RankItemContainer& c) = 0;
  virtual int32_t RankType() const = 0;
  virtual bool Full() const = 0;
  static std::string Print(const RankItemContainer& data);
};

template <typename Compare, int Count, int Type>
class RankList : public RankListBase {
 public:
  // true表示在排行榜里面
  // false表示不在排行榜里面
  bool Update(RankItem& item) {
    int32_t old_index = GetRankByUID(item.uid()) - 1;
    bool up_or_down = true;
    //已经存在
    if (old_index >= 0) {
      up_or_down = !fn_(list_.items(old_index), item);
      list_.mutable_items(old_index)->CopyFrom(item);
    } else {
      //向上冒泡
      up_or_down = true;
      list_.add_items()->CopyFrom(item);
      old_index = list_.items_size() - 1;
    }
    this->BubbleSort(old_index, up_or_down);

    if (list_.items_size() > Count) {
      bool is_self =
          list_.items(list_.items_size() - 1).uid() == item.uid();
      list_.mutable_items()->RemoveLast();
      return !is_self;
    }
    return true;
  }

  int32_t RankType() const { return Type; }

  void Remove(int64_t uid) {
    int32_t index = GetRankByUID(uid) - 1;
    if (index >= 0) this->list_.mutable_items()->DeleteSubrange(index, 1);
  }

  bool Full() const { return this->list_.items_size() >= Count; }

  // 0表示没找到
  // 大于0表示排行
  int32_t GetRankByUID(int64_t uid) const {
    for (int32_t index = 0; index < list_.items_size(); ++index) {
      if (list_.items(index).uid() == uid) return index + 1;
    }
    return 0;
  }

  const RankItem* GetRankInfoByUID(int64_t uid) const {
    int32_t rank = this->GetRankByUID(uid);
    if (!rank) return NULL;
    return &this->list_.items(rank - 1);
  }

  const RankItem* GetLast() const {
    if (this->list_.items_size())
      return &this->list_.items(this->list_.items_size() - 1);
    return NULL;
  }

  void Clear() {
    this->list_.mutable_items()->Clear();
  }

  const RankItemContainer& data() const { return this->list_; }

  void Update(const RankItemContainer& c) {
    this->list_.Clear();
    this->list_.CopyFrom(c);
  }

 private:
  int32_t BubbleSort(int32_t index, bool up_or_down) {
    int32_t ret = 0;
    if (up_or_down) {
      for (int32_t i = index; i > 0; --i) {
        ret = i;
        if (fn_(list_.items(i), list_.items(i - 1))) {
          list_.mutable_items()->SwapElements(i, i - 1);
        } else {
          break;
        }
      }
    } else {
      for (int32_t i = index; i < list_.items_size() - 1; ++i) {
        ret = i;
        if (fn_(list_.items(i + 1), list_.items(i))) {
          list_.mutable_items()->SwapElements(i, i + 1);
        } else {
          break;
        }
      }
    }
    return ret;
  }

 private:
  Compare fn_;
  RankItemContainer list_;
};

class LogicPlayer;

class ServerRankList : NonCopyable {
 public:
  ~ServerRankList();
  void InitRankList();

  RankListBase& GetByType(int32_t type) const;
  void ClearRank(int32_t type);

  //主线副本
  void OnNormalCopyPassed(LogicPlayer* player);
  //精英副本
  void OnHardCopyPassed(LogicPlayer* player);
  //爬塔副本
  void OnTowerCopyPassed(LogicPlayer* player);
  //玩家等级提升
  void OnPlayerLevelUp(LogicPlayer* player);
  //战斗力
  void OnPlayerFightingUp(LogicPlayer* player);
  //伤害
  void OnDstrikeDamage(LogicPlayer* player);
  //功勋
  void OnDstrikeExploit(LogicPlayer* player);
  //航母副本合金数
  void OnCarrierCopy(LogicPlayer* player);
  //公会荣耀
  void OnArmyAddMerit(Army* army);
  //世界boss
  void OnWorldBossDamage(LogicPlayer* player);
  void OnWorldBossMerit(LogicPlayer* player);

  //道具积分榜
  void OnResearchItem(LogicPlayer* player);
  //跨服积分排行榜
  void OnCrossServerFight(RankItem& item);

  //制霸全球排行榜
  void OnLegionWar(int32_t type, RankItem& item);
  void OnLegionWar1(LogicPlayer* player, int32_t score, sy::RankItemInfo& info);
  void OnLegionForeplay(LogicPlayer* player, int32_t damage);

  //航母转盘活动
  void OnSweepStakeCarrier(LogicPlayer* player, int32_t score);

  void RemoveAll(int64_t uid);

  //拉取排行榜
  void SendLoadRankList();
  void OnRecvRankListData(int32_t type, const RankItemContainer& c);

  //勋章排行
  void OnMedalRank(LogicPlayer* player);
  void OnMedalCrossServerRank(RankItem& item);

  //捍卫珍珠港
  void OnPearlHarborPlayer(LogicPlayer* player, int64_t score);
  void OnPearlHarborArmy(Army* army);

 private:
  void Register(RankListBase*);
  void OnUpdateRankList(RankListBase& list, const RankItem& item);
  void OnUpdateRankItem(int32_t type, const RankItem& item);
 private:
   std::vector<RankListBase*> ranks_;
};

#define RANK_LIST Singleton<ServerRankList>::Instance()

#endif
