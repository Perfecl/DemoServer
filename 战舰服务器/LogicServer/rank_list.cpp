#include "rank_list.h"
#include "server.h"
#include <array_stream.h>
#include "logic_player.h"
#include "army.h"
#include <cpp/server_message.pb.h>
#include <sstream>

using namespace intranet;

static RankList<CompareRankLevel, 1, sy::RANK_TYPE_LEVEL> kEmptyRankList;

enum RankFieldTag {
  RANK_FIELD_LEVEL      = 1 << 0,
  RANK_FIELD_FIGHT_ATTR = 1 << 2,
  RANK_FIELD_VIP_LEVEL  = 1 << 3,
  RANK_FIELD_ARMY_NAME  = 1 << 4,
  RANK_FIELD_EXPLOIT    = 1 << 5,
  RANK_FIELD_DAMAGE     = 1 << 6,
  RANK_FIELD_STAR       = 1 << 7,
  RANK_FIELD_AVATAR     = 1 << 8,
  RANK_FIELD_HARD_STAR  = 1 << 9,
  RANK_FIELD_TOWER_STAR = 1 << 10,
  RANK_FIELD_SERVER_ID  = 1 << 11,
};

// 先填充必须的三个字段(uid, name)
// 再填充可选字段
const int32_t kRankNormalCopyFields = RANK_FIELD_STAR;
const int32_t kRankHardCopyFields = RANK_FIELD_HARD_STAR;
const int32_t kRankTowerCopyFields = RANK_FIELD_TOWER_STAR;
const int32_t kRankLevelFields = RANK_FIELD_ARMY_NAME | RANK_FIELD_LEVEL | RANK_FIELD_FIGHT_ATTR | RANK_FIELD_AVATAR;
const int32_t kRankFightingFields = RANK_FIELD_ARMY_NAME | RANK_FIELD_LEVEL | RANK_FIELD_FIGHT_ATTR | RANK_FIELD_AVATAR;
const int32_t kRankDamageFields = RANK_FIELD_LEVEL | RANK_FIELD_FIGHT_ATTR | RANK_FIELD_AVATAR | RANK_FIELD_DAMAGE;
const int32_t kRankExploitFields = RANK_FIELD_LEVEL | RANK_FIELD_FIGHT_ATTR | RANK_FIELD_AVATAR | RANK_FIELD_EXPLOIT;
const int32_t kRankCarrierCopyFields = RANK_FIELD_LEVEL | RANK_FIELD_AVATAR | RANK_FIELD_STAR | RANK_FIELD_VIP_LEVEL;
const int32_t kRankWorldBossFields = RANK_FIELD_LEVEL | RANK_FIELD_AVATAR | RANK_FIELD_STAR | RANK_FIELD_VIP_LEVEL | RANK_FIELD_ARMY_NAME | RANK_FIELD_FIGHT_ATTR;
const int32_t kRankMedal =  RANK_FIELD_AVATAR | RANK_FIELD_VIP_LEVEL | RANK_FIELD_LEVEL;
const int32_t kRankPearlHarborPlayer = RANK_FIELD_AVATAR | RANK_FIELD_FIGHT_ATTR | RANK_FIELD_LEVEL | RANK_FIELD_VIP_LEVEL;

template <int32_t Fields>
void inline FillRankItem(LogicPlayer* player, sy::RankItemInfo& info) {
  if (!player) return;

  info.set_uid(player->uid());
  info.set_name(player->name());

  if (Fields & RANK_FIELD_ARMY_NAME) {
    Army* army = server->GetArmyByID(player->army_id());
    info.set_army_name(army ? army->info().army_name() : "");
  }
  if (Fields & RANK_FIELD_LEVEL) info.set_level(player->level());
  if (Fields & RANK_FIELD_FIGHT_ATTR) info.set_fight_attr(player->fight_attr());
  if (Fields & RANK_FIELD_VIP_LEVEL) info.set_vip_level(player->vip_level());
  if (Fields & RANK_FIELD_EXPLOIT) info.set_exploit(player->dstrike_exploit());
  if (Fields & RANK_FIELD_DAMAGE) info.set_damage(player->dstrike_damage());
  if (Fields & RANK_FIELD_AVATAR) info.set_avatar(player->avatar());
  if (Fields & RANK_FIELD_STAR)
    info.set_star(player->GetCopyStarByType(sy::COPY_TYPE_NORMAL));
  if (Fields & RANK_FIELD_HARD_STAR)
    info.set_star(player->GetCopyStarByType(sy::COPY_TYPE_HARD));
  if (Fields & RANK_FIELD_TOWER_STAR)
    info.set_star(player->GetCopyStarByType(sy::COPY_TYPE_TOWER));
}

ServerRankList::~ServerRankList() {
  for (std::vector<RankListBase*>::iterator iter = ranks_.begin();
       iter != ranks_.end(); ++iter) {
    if (*iter) delete *iter;
  }
  this->ranks_.clear();
  this->ranks_.resize(sy::RankType_ARRAYSIZE, NULL);
}

void ServerRankList::InitRankList() {
  this->ranks_.resize(sy::RankType_ARRAYSIZE, NULL);
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_STAR_1>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_STAR_2>());
  Register(new RankList<CompareRankStar, 20, sy::RANK_TYPE_TOWER>());
  Register(new RankList<CompareRankExploit, 200, sy::RANK_TYPE_DSTRIKE_EXPLOIT>());
  Register(new RankList<CompareRankDamage, 200, sy::RANK_TYPE_DSTRIKE_DAMAGE>());
  Register(new RankList<CompareRankLevel, 50, sy::RANK_TYPE_LEVEL>());
  Register(new RankList<CompareRankFight, 50, sy::RANK_TYPE_FIGHT>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_CARRIER_COPY>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_ARMY>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_DAMAGE_UK>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_DAMAGE_US>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_DAMAGE_GE>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_DAMAGE_JP>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_MERIT_UK>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_MERIT_US>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_MERIT_GE>());
  Register(new RankList<CompareRankStar, 50, sy::RANK_TYPE_WORLD_BOSS_MERIT_JP>());
  Register(new RankList<CompareRankStar, 10, sy::RANK_TYPE_RESEARCH_ITEM>());
  Register(new RankList<CompareRankStar, 100, sy::RANK_TYPE_CROSS_SERVER_UK>());
  Register(new RankList<CompareRankStar, 100, sy::RANK_TYPE_CROSS_SERVER_US>());
  Register(new RankList<CompareRankStar, 100, sy::RANK_TYPE_CROSS_SERVER_GE>());
  Register(new RankList<CompareRankStar, 100, sy::RANK_TYPE_CROSS_SERVER_JP>());
  Register(new RankList<CompareRankStar, 20, sy::RANK_TYPE_LEGION_WAR_1>());
  Register(new RankList<CompareRankStar, 20, sy::RANK_TYPE_LEGION_WAR_2>());
  Register(new RankList<CompareRankStar, 30, sy::RANK_TYPE_LEGION_WAR_3>());
  Register(new RankList<CompareRankStar, 20, sy::RANK_TYPE_LEGION_FOREPLAY>());
  Register(new RankList<CompareRankStar, 100, sy::RANK_TYPE_SWEEP_STAKE_CARRIER>());
  Register(new RankList<CompareRankStar, 100, sy::RANK_TYPE_MEDAL>());
  Register(new RankList<CompareRankStar, 100, sy::RANK_TYPE_MEDAL_CROSS_SERVER>());
  Register(new RankList<CompareRankStar, 5, sy::RANK_TYPE_PEARL_HARBOR_PLAYER>());
  Register(new RankList<CompareRankStar, 20, sy::RANK_TYPE_PEARL_HARBOR_ARMY>());
}

void ServerRankList::Register(RankListBase* rank) {
  if (this->ranks_[rank->RankType()]) {
    delete rank;
    return;
  }
  this->ranks_[rank->RankType()] = rank;
}

void ServerRankList::SendLoadRankList() {
  MessageSSRequestLoadRankList request;

  request.set_server_id(server_config->server_id());
  for (int32_t rank_type = sy::RANK_TYPE_STAR_1; rank_type < sy::RankType_ARRAYSIZE;
       ++rank_type) {
    request.add_type(rank_type);
  }

  server->SendServerMessageToDB(MSG_SS_REQUEST_LOAD_RANK_LIST, &request);
}

void ServerRankList::OnRecvRankListData(int32_t type, const RankItemContainer& c) {
  this->GetByType(type).Update(c);
  kEmptyRankList.Clear();
}

void ServerRankList::OnUpdateRankItem(int32_t type, const RankItem& item) {
  //更新排行榜单行记录
  intranet::MessageSSUpdateRankListPlayer message;
  message.set_type(type);
  message.mutable_info()->CopyFrom(item);
  server->SendServerMessageToDB(intranet::MSG_SS_UPDATE_RANK_LIST_PLAYER, &message);
}

void ServerRankList::OnUpdateRankList(RankListBase& list, const RankItem& item) {
  int32_t rank_type = list.RankType();
  //更新玩家的索引
  intranet::MessageSSUpdateRankListDetails message;
  message.set_type(rank_type);
  message.set_server_id(server_config->server_id());
  for (int32_t i = 0; i < list.data().items_size(); ++i) {
    message.add_players(list.data().items(i).uid());
  }
  server->SendServerMessageToDB(intranet::MSG_SS_UPDATE_RANK_LIST_DETAILS, &message);

  if (item.has_uid()) this->OnUpdateRankItem(rank_type, item);
}

void ServerRankList::ClearRank(int32_t type) {
  this->GetByType(type).Clear();
  intranet::MessageSSUpdateRankListDetails message;
  message.set_type(type);
  message.set_server_id(server_config->server_id());
  server->SendServerMessageToDB(intranet::MSG_SS_UPDATE_RANK_LIST_DETAILS, &message);
}

RankListBase& ServerRankList::GetByType(int32_t type) const {
  if (type >= 0 && type < sy::RankType_ARRAYSIZE) {
    return this->ranks_[type] ? *this->ranks_[type] : kEmptyRankList;
  }
  return kEmptyRankList;
}

void ServerRankList::OnNormalCopyPassed(LogicPlayer* player) {
  if (player->level() < 20) return;
  RankItem item;
  FillRankItem<kRankNormalCopyFields>(player, item);
  RankListBase& list = this->GetByType(sy::RANK_TYPE_STAR_1);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
}

void ServerRankList::OnHardCopyPassed(LogicPlayer* player) {
  if (player->level() < 20) return;
  RankItem item;
  FillRankItem<kRankHardCopyFields>(player, item);
  RankListBase& list = this->GetByType(sy::RANK_TYPE_STAR_2);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
}

void ServerRankList::OnTowerCopyPassed(LogicPlayer* player) {
  if (player->level() < 20) return;
  RankItem item;
  FillRankItem<kRankTowerCopyFields>(player, item);
  RankListBase& list = this->GetByType(sy::RANK_TYPE_TOWER);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
}

void ServerRankList::OnPlayerLevelUp(LogicPlayer* player) {
  if (player->level() < 20) return;
  RankItem item;
  RankListBase& list = this->GetByType(sy::RANK_TYPE_LEVEL);
  if (list.Full() && list.GetLast() &&
      list.GetLast()->level() >= player->level())
    return;

  FillRankItem<kRankLevelFields>(player, item);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
  this->OnDstrikeDamage(player);
  this->OnDstrikeExploit(player);
}

void ServerRankList::OnPlayerFightingUp(LogicPlayer* player) {
  if (player->level() < 20) return;
  RankItem item;
  FillRankItem<kRankFightingFields>(player, item);
  RankListBase& list = this->GetByType(sy::RANK_TYPE_FIGHT);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
  this->OnDstrikeDamage(player);
  this->OnDstrikeExploit(player);
}

void ServerRankList::OnDstrikeDamage(LogicPlayer* player) {
  if (player->level() < 20) return;
  RankItem item;
  FillRankItem<kRankDamageFields>(player, item);
  if (!item.damage() || !item.fight_attr()) return;
  RankListBase& list = this->GetByType(sy::RANK_TYPE_DSTRIKE_DAMAGE);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
}

void ServerRankList::OnDstrikeExploit(LogicPlayer* player) {
  if (player->level() < 20) return;
  RankItem item;
  FillRankItem<kRankExploitFields>(player, item);
  if (!item.exploit() || !item.fight_attr()) return;
  RankListBase& list = this->GetByType(sy::RANK_TYPE_DSTRIKE_EXPLOIT);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
}

void ServerRankList::OnCarrierCopy(LogicPlayer* player) {
  if (player->level() < 20) return;
  const RankItem* info = this->GetByType(sy::RANK_TYPE_CARRIER_COPY).GetRankInfoByUID(player->uid());
  if (!info || player->carrier_copy_info().item_count() > info->star()) {
    RankItem item;
    FillRankItem<kRankCarrierCopyFields>(player, item);
    item.set_star(player->carrier_copy_info().item_count());
    if (!item.star()) return;

    RankListBase& list = this->GetByType(sy::RANK_TYPE_CARRIER_COPY);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

std::string RankListBase::Print(const RankItemContainer& data){
  std::ostringstream oss;
  for (int32_t i = 0; i < data.items_size(); ++i) {
    const RankItem& info = data.items(i);
    if (i) oss << std::endl;
    oss << "Rank:" << i + 1 << " UID:" << info.uid();
    if (info.level()) oss << " Level:" << info.level();
    if (info.exploit()) oss << " Exploit:" << info.exploit();
    if (info.damage()) oss << " Damage:" << info.damage();
    if (info.fight_attr()) oss << " Fight:" << info.fight_attr();
    if (info.star()) oss << " Star:" << info.star();
  }
  return oss.str();
}

void ServerRankList::OnArmyAddMerit(Army* army) {
  if (!army) return;
  const RankItem* info = this->GetByType(sy::RANK_TYPE_WORLD_BOSS_ARMY)
                             .GetRankInfoByUID(army->army_id());
  if (!info || army->army_merit() > info->star()) {
    RankItem item;
    item.set_uid(army->army_id());
    item.set_name("");
    item.set_level(army->level());
    item.set_army_name(army->name());
    item.set_star(army->army_merit());
    item.set_avatar(army->info().avatar());
    if (!item.star()) return;
    RankListBase& list = this->GetByType(sy::RANK_TYPE_WORLD_BOSS_ARMY);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnWorldBossDamage(LogicPlayer* player) {
  if (!player || !player->world_boss_info().country()) return;
  int32_t type = 0;
  if (player->world_boss_info().country() == 1)
    type = sy::RANK_TYPE_WORLD_BOSS_DAMAGE_UK;
  if (player->world_boss_info().country() == 2)
    type = sy::RANK_TYPE_WORLD_BOSS_DAMAGE_US;
  if (player->world_boss_info().country() == 3)
    type = sy::RANK_TYPE_WORLD_BOSS_DAMAGE_GE;
  if (player->world_boss_info().country() == 4)
    type = sy::RANK_TYPE_WORLD_BOSS_DAMAGE_JP;
  if (!type) return;
  const RankItem* info = this->GetByType(type).GetRankInfoByUID(player->uid());
  if (!info || player->world_boss_info().damage() > info->star()) {
    RankItem item;
    FillRankItem<kRankWorldBossFields>(player, item);
    item.set_star(player->world_boss_info().damage());
    if (!item.star()) return;
    RankListBase& list = this->GetByType(type);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnWorldBossMerit(LogicPlayer* player) {
  if (!player || !player->world_boss_info().country()) return;
  int32_t type = 0;
  if (player->world_boss_info().country() == 1)
    type = sy::RANK_TYPE_WORLD_BOSS_MERIT_UK;
  if (player->world_boss_info().country() == 2)
    type = sy::RANK_TYPE_WORLD_BOSS_MERIT_US;
  if (player->world_boss_info().country() == 3)
    type = sy::RANK_TYPE_WORLD_BOSS_MERIT_GE;
  if (player->world_boss_info().country() == 4)
    type = sy::RANK_TYPE_WORLD_BOSS_MERIT_JP;
  if (!type) return;
  const RankItem* info = this->GetByType(type).GetRankInfoByUID(player->uid());
  if (!info || player->world_boss_info().merit() > info->star()) {
    RankItem item;
    FillRankItem<kRankWorldBossFields>(player, item);
    item.set_star(player->world_boss_info().merit());
    if (!item.star()) return;
    RankListBase& list = this->GetByType(type);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnResearchItem(LogicPlayer* player) {
  if (!player) return;
  if (player->research_item_point() < GetSettingValue(limited_recruit_0score)) return;

  const RankItem* info = this->GetByType(sy::RANK_TYPE_RESEARCH_ITEM)
                             .GetRankInfoByUID(player->uid());
  if (!info || player->research_item_point() > info->star()) {
    RankItem item;
    FillRankItem<kRankNormalCopyFields>(player, item);
    item.set_star(player->research_item_point());
    item.set_damage(player->rank_id());
    if (!item.star()) return;

    RankListBase& list = this->GetByType(sy::RANK_TYPE_RESEARCH_ITEM);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnCrossServerFight(RankItem& item) {
  if (!item.star()) return;
  const RankItem* info =
      this->GetByType(item.exploit()).GetRankInfoByUID(item.uid());
  if (!info || item.star() > info->star()) {
    RankListBase& list = this->GetByType(item.exploit());
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnLegionWar(int32_t type, RankItem& item) {
  if (!item.star()) return;
  if (type == sy::RANK_TYPE_LEGION_WAR_2 &&
      item.star() < GetSettingValue(legion_war_region_score1))
    return;
  if (type == sy::RANK_TYPE_LEGION_WAR_3 &&
      item.star() < GetSettingValue(legion_war_region_score2))
    return;
  RankListBase& list = this->GetByType(type);
  if (list.Update(item)) this->OnUpdateRankList(list, item);
}

void ServerRankList::RemoveAll(int64_t uid) {
  for (std::vector<RankListBase*>::iterator it = this->ranks_.begin();
       it != this->ranks_.end(); ++it) {
    RankListBase* base = *it;
    if (!base) continue;
    base->Remove(uid);
    OnUpdateRankList(*base, RankItem());
  }
}

void ServerRankList::OnLegionWar1(LogicPlayer* player, int32_t score, sy::RankItemInfo& info) {
  info.set_uid(player->uid());
  info.set_name(player->name());
  info.set_star(score);
  info.set_damage(server_config->server_id());
  info.set_level(player->level());
  info.set_vip_level(player->vip_level());
  info.set_avatar(player->avatar());
  Army* army = server->GetArmyByID(player->army_id());
  info.set_army_name(army ? army->name() : "");
  RankListBase& list = this->GetByType(sy::RANK_TYPE_LEGION_WAR_1);
  if (list.Update(info)) this->OnUpdateRankList(list, info);
}

void ServerRankList::OnLegionForeplay(LogicPlayer* player, int32_t damage) {
  if (!player || player->level() < 20) return;
  const RankItem* info = this->GetByType(sy::RANK_TYPE_LEGION_FOREPLAY)
                             .GetRankInfoByUID(player->uid());
  if (!info || damage > info->star()) {
    RankItem item;
    FillRankItem<kRankWorldBossFields>(player, item);
    item.set_star(damage);
    item.set_exploit(player->rank_id());
    if (!item.star()) return;
    RankListBase& list = this->GetByType(sy::RANK_TYPE_LEGION_FOREPLAY);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnSweepStakeCarrier(LogicPlayer* player, int32_t score) {
  if (!player || player->level() < 20) return;
  const RankItem* info = this->GetByType(sy::RANK_TYPE_SWEEP_STAKE_CARRIER)
                             .GetRankInfoByUID(player->uid());
  if (!info || score > info->star()) {
    RankItem item;
    FillRankItem<RANK_FIELD_LEVEL>(player, item);
    item.set_star(score);
    if (!item.star()) return;
    RankListBase& list = this->GetByType(sy::RANK_TYPE_SWEEP_STAKE_CARRIER);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnMedalRank(LogicPlayer* player) {
  if (!player) return;
  const RankItem* info =
      this->GetByType(sy::RANK_TYPE_MEDAL).GetRankInfoByUID(player->uid());
  if (!info || player->medal_star() > info->star()) {
    RankItem item;
    FillRankItem<kRankMedal>(player, item);
    item.set_star(player->medal_star());
    item.set_damage(player->medal_state_count());
    if (!item.star()) return;
    RankListBase& list = this->GetByType(sy::RANK_TYPE_MEDAL);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnMedalCrossServerRank(RankItem& item) {
  const RankItem* info = this->GetByType(sy::RANK_TYPE_MEDAL_CROSS_SERVER)
                             .GetRankInfoByUID(item.uid());
  if (!info || item.star() > info->star()) {
    RankListBase& list = this->GetByType(sy::RANK_TYPE_MEDAL_CROSS_SERVER);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnPearlHarborPlayer(LogicPlayer* player, int64_t score) {
  if (!player) return;
  const RankItem* info = this->GetByType(sy::RANK_TYPE_PEARL_HARBOR_PLAYER)
                             .GetRankInfoByUID(player->uid());
  if (!info || score > info->star()) {
    RankItem item;
    FillRankItem<kRankPearlHarborPlayer>(player, item);
    item.set_star(score);
    if (!item.star()) return;
    RankListBase& list = this->GetByType(sy::RANK_TYPE_PEARL_HARBOR_PLAYER);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

void ServerRankList::OnPearlHarborArmy(Army* army) {
  if (!army) return;
  sy::PearlHarborInfo& pearl_info = army->PearlHarborInfo();
  const RankItem* info = this->GetByType(sy::RANK_TYPE_PEARL_HARBOR_ARMY)
                             .GetRankInfoByUID(army->army_id());
  if (!info || pearl_info.score() > info->star()) {
    RankItem item;
    item.set_army_name(army->info().army_name());
    item.set_uid(army->army_id());
    item.set_star(pearl_info.score());
    item.set_level(army->level());
    item.set_avatar(army->info().avatar());
    if (!item.star()) return;
    RankListBase& list = this->GetByType(sy::RANK_TYPE_PEARL_HARBOR_ARMY);
    if (list.Update(item)) this->OnUpdateRankList(list, item);
  }
}

