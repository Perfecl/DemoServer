#ifndef __TESTER_H__
#define __TESTER_H__
#include <myrandom.h>
#include "rank_list.h"
#include "logic_player.h"
#include "config.h"
#include <fstream>

class Tester {
 public:
  void operator()() {
    TestItem();
    TestCopyStar(1001, 1155);
    TestAddExp(10000);
    TestPkRankReward();

    TestRandomPkTarget(2500);
    TestRandomPkTarget(3000);
    TestRandomPkTarget(100);
    TestRandomPkTarget(20);
    TestRandomPkTarget(99);

    TestTowerRandomBuff();

    //TestPatrol();

    const DstrikeBossBase* boss = DstrikeConfigFile::RandomBoss(50, 1);
    if (boss) INFO_LOG(logger)("BOSS ID:%ld", boss->id());
    boss = DstrikeConfigFile::RandomBoss(50, 1);
    if (boss) INFO_LOG(logger)("BOSS ID:%ld", boss->id());
    boss = DstrikeConfigFile::RandomBoss(50, 1);
    if (boss) INFO_LOG(logger)("BOSS ID:%ld", boss->id());
    boss = DstrikeConfigFile::RandomBoss(50, 1);
    if (boss) INFO_LOG(logger)("BOSS ID:%ld", boss->id());
    boss = DstrikeConfigFile::RandomBoss(50, 1);
    if (boss) INFO_LOG(logger)("BOSS ID:%ld", boss->id());

    TestTower();
    //TestShipLoot();
    //TestLoot();
    //TestResearchHero();
  }


 private:
  static void TestResearchHero() {
    int a[] = {1, 10, 20, 21};
    for (int kk = 0; kk < 4; ++kk) {
      for (int i = 0; i < 1000; ++i) {
        const ShipRaffleBasePtr& base = GetShipRaffleByCount(i, a[kk]);
        INFO_LOG(logger)("FindHeroTest type:%d,count:%d, %ld", a[kk], i, base->id());
      }
    }
  }

  void TestShipLoot() {
    const ShipPackBasePtr& ptr = SHIP_PACK_BASE.GetEntryByID(44000100);
    if (!ptr) return;
    std::map<int32_t, int32_t> count_map;
    for (int32_t i = 0; i < 100000; ++i) {
      int32_t index = ShipPackBase::Random(RandomBetween(0, ptr->sum - 1), ptr->heroid);
      count_map[index]++;
    }
    for (std::map<int32_t, int32_t>::const_iterator iter = count_map.begin();
         iter != count_map.end(); ++iter) {
      std::cout << iter->first << " => " << iter->second << std::endl;
    }
  }

  void TestItem() {
    LogicPlayer player(1);
    std::pair<int32_t, int32_t> item_count[] = {
      std::make_pair(22011301, 100001),
      std::make_pair(22011302, 100002),
      std::make_pair(22011303, 100003),
      std::make_pair(22011304, 100004),
    };
    LogicItem logic_item;
    for (unsigned i = 0; i < sizeof(item_count) / sizeof(item_count[0]); ++i) {
      sy::Item item;
      item.set_uid(i);
      item.set_item_id(item_count[i].first);
      item.set_count(item_count[i].second);
      if (LogicItem::CreateItem(item, logic_item)) {
        player.items_.AddItem(logic_item);
      }
    }

    AddSubItemSet modify;
    modify.push_back(ItemParam(22011301, -1 - 100));
    modify.push_back(ItemParam(22011302, -2 - 100));
    modify.push_back(ItemParam(22011303, -3 - 100));
    modify.push_back(ItemParam(22011304, -4 - 100));

    player.ObtainItem(&modify, NULL, NULL, 0, 0);

    AssertItemCount(&player, 22011301, 100000 - 100);
    AssertItemCount(&player, 22011302, 100000 - 100);
    AssertItemCount(&player, 22011303, 100000 - 100);
    AssertItemCount(&player, 22011304, 100000 - 100);
  }

  static void TestTowerRandomBuff() {
    LogicPlayer player(1);
    player.PrepareTowerRandomBuff(3);
    player.PrepareTowerRandomBuff(6);
    player.PrepareTowerRandomBuff(9);
  }

  static void AssertItemCount(LogicPlayer* player, int32_t item_id,
                              int32_t count) {
    LogicItem *item = player->items().GetItemByItemID(item_id);
    if (!item) {
      ERROR_LOG(logger)("item id:%d not found", item_id);
      return;
    }
    if (item->count() != count) {
      ERROR_LOG(logger)("item id:%d, count:%d, not:%d", item_id, item->count(), count);
    }
  }

  static void TestCopyStar(int32_t chapter, int32_t copy_id) {
    LogicPlayer player(1);
    sy::CopyProgress progress;
    progress.set_copy_type(1);
    progress.set_chapter(chapter);
    progress.set_copy_id(copy_id);
    player.SetAllCopyStar(progress);
  }

  static void TestAddExp(int32_t exp) {
    LogicPlayer player(1);
    ModifyCurrency modify(0, 0);
    modify.exp += exp;

    player.player().set_level(1);
    player.UpdateCurrency(modify);

    player.player().set_level(100);
    player.UpdateCurrency(modify);
  }

  static void TestPkRankReward() {
    LogicPlayer player(1);
    player.SendPKRankReward(10000, 9955);
    player.SendPKRankReward(3000, 2800);
    player.SendPKRankReward(3000, 1400);
    player.SendPKRankReward(5, 1);
    player.SendPKRankReward(2, 1);
  }

  static void TestRandomPkTarget(int32_t rank) {
    DEBUG_LOG(logger)("TestRandomPkTarget:%d", rank);
    int32_t step = rank / 10;
    for (int32_t i = 0; i < 5; ++i) {
      DEBUG_LOG(logger)("RandomRank:%d", rank - (i + 1) * step + RandomBetween(0, step - 1) );
    }
  }

  static void TestTower() {
    CSMessageEntry entry;
    sy::MessageRequestFight *m = new sy::MessageRequestFight;
    entry.message.reset(m);

    {
    LogicPlayer player(0);
    m->set_copy_id(70111);
    player.ProcessRequestFight(entry);
    }

    {
    LogicPlayer player(0);
    m->set_copy_id(70112);
    player.ProcessRequestFight(entry);
    }
    {
    LogicPlayer player(0);
    m->set_copy_id(70113);
    player.ProcessRequestFight(entry);
    }
  }

  static void TestLoot() {
    ModifyCurrency modify(0, 0);
    AddSubItemSet item_set;
    std::vector<int32_t> index;
    for (int i = 0; i < 100; ++i) {
      index.clear();
      LootConfigFile::Get(400190, 50)->Loot(modify, item_set, &index);
      INFO_LOG(logger)("AutoFlop Index:%d", index[0]);
    }
  }

  static void TestPatrol() {
    int32_t max_patrol_level = 5;
    int32_t max_patrol_id = 6;
    int32_t award_min[3] = {4 * 60, 8 * 60, 12 * 60};
    int32_t award_min_interval[3] = {30, 20, 10};
    VectorMap<int32_t, int32_t> awards;
    std::ofstream fout("./patrol_test.txt", std::ios::out);
    for (int32_t i = 0; i <= max_patrol_level; i++) {
      for (int32_t j = 1; j <= max_patrol_id; j++) {
        for (int32_t k = 0; k < 3; k++) {
          for (int32_t l = 0; l < 3; l++) {
            int32_t count = award_min[k] / award_min_interval[l] - 1;
            if (!PatrolBase::GeneratePatrolAward(
                    count, 100, i, PATROL_BASE.GetEntryByID(j).get(), awards)) {
              fout << count << std::endl;
              for (VectorMap<int32_t, int32_t>::iterator it = awards.begin();
                   it != awards.end(); ++it) {
                fout << it->first << "-" << it->second << "|";
              }
              fout << std::endl;
              awards.clear();
            }
          }
        }
      }
    }

    fout.close();
  }
};

#endif
