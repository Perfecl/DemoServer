#include "logic_player.h"
#include <myrandom.h>
#include <boost/random.hpp>
#include <net/MessageCache.h>
#include "server.h"
#include "rank_list.h"
#include <array_stream.h>
#include <google/protobuf/text_format.h>
#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <vector_set.h>
#include <lz4/lz4.h>
#include <str_util.h>
#include "pk.h"
#include "dirty_searcher/dirty_searcher.h"
#include "message_handler.h"
#include "army.h"
#include "legion_war.h"
#include "time_activity.h"
#include "storage.h"

using namespace sy;
using namespace intranet;
using google::protobuf::RepeatedField;
using google::protobuf::RepeatedPtrField;
using namespace KVStorage;

//航母副本通关掩码
const int32_t kCarrierCopyLevel4 = (1 << 10) | (1 << 11) | (1 << 12);

template <typename T>
inline void ProtobufUnique(google::protobuf::RepeatedField<T>* arr) {
  if (!arr) return;

  std::sort(arr->begin(), arr->end());
  arr->Resize(
      std::distance(arr->begin(), std::unique(arr->begin(), arr->end())), T());
}

//+= A * iter->v2 / B
template <int B>
static void inline AddConfigAttr(AttackAttrArray& attr,
                                 const ValuePair2Vec& vec, int A) {
  ValuePair2Vec::const_iterator iter = vec.begin();
  switch (vec.size()) {
    case 4: attr[iter->v1] += A * iter->v2 / B; ++iter;
    case 3: attr[iter->v1] += A * iter->v2 / B; ++iter;
    case 2: attr[iter->v1] += A * iter->v2 / B; ++iter;
    case 1: attr[iter->v1] += A * iter->v2 / B; ++iter;
    case 0: break;
    default:
      for (; iter != vec.end(); ++iter) {
        attr[iter->v1] += A * iter->v2 / B;
      }
  }
}

template <int B>
static void inline AddConfigAttr(AttackAttrArray& attr,
                                 const ValuePair2Vec& vec) {
  ValuePair2Vec::const_iterator iter = vec.begin();
  switch (vec.size()) {
    case 4: attr[iter->v1] += iter->v2 / B; ++iter;
    case 3: attr[iter->v1] += iter->v2 / B; ++iter;
    case 2: attr[iter->v1] += iter->v2 / B; ++iter;
    case 1: attr[iter->v1] += iter->v2 / B; ++iter;
    case 0: break;
    default:
      for (; iter != vec.end(); ++iter) {
        attr[iter->v1] += iter->v2 / B;
      }
  }
}

static inline void AddConfigAttr(AttackAttrArray& attr,
                                 const ValuePair2Vec& vec) {
  ValuePair2Vec::const_iterator iter = vec.begin();
  switch (vec.size()) {
    case 4: attr[iter->v1] += iter->v2; ++iter;
    case 3: attr[iter->v1] += iter->v2; ++iter;
    case 2: attr[iter->v1] += iter->v2; ++iter;
    case 1: attr[iter->v1] += iter->v2; ++iter;
    case 0: break;
    default:
      for (; iter != vec.end(); ++iter) {
        attr[iter->v1] += iter->v2;
      }
  }
}

//模板参数为1, 表示增加
enum { kAdd = 1, kSub = 0 };
template <int ADD>
inline void FillCurrencyAndItem(ModifyCurrency& modify, AddSubItemSet& item_set,
                                const ValuePair2<int32_t, int32_t>& config) {
  if (item_set.full()) return;
  if (config.v1 < 100) {
    modify[config.v1] += ADD ? config.v2 : -config.v2;
  } else {
    int32_t max_count = LogicItem::GetMaxCount(config.v1);
    if (max_count == 1 && config.v2 > 1) {
      for (int32_t i = 0; i < config.v2; ++i)
        item_set.push_back(ItemParam(config.v1, ADD ? 1 : -1));
    } else {
      for (size_t i = 0; i < item_set.size(); i++) {
        if (item_set[i].item_id == config.v1) {
          item_set[i].item_count += (ADD ? config.v2 : -config.v2);
          return;
        }
      }
      item_set.push_back(ItemParam(config.v1, ADD ? config.v2 : -config.v2));
    }
  }
}

template <int ADD>
inline void FillCurrencyAndItem(ModifyCurrency& modify, AddSubItemSet& item_set,
                                const ValuePair2Vec& config) {
  for (ValuePair2Vec::const_iterator iter = config.begin();
       iter != config.end(); ++iter) {
    FillCurrencyAndItem<ADD>(modify, item_set, *iter);
  }
}

void FillToKVPair2L(
    LootBase* loot, const std::vector<int32_t>& vct_index,
    __OUT__ google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items) {
  if (!out_items || !loot) return;

  const ValuePair3Vec& award = loot->info;
  for (std::vector<int32_t>::const_iterator it = vct_index.begin();
       it != vct_index.end(); ++it) {
    if ((*it) >= (int32_t)award.size() || (*it) < 0) continue;
    KVPair2* pair = out_items->Add();
    pair->set_key(award[*it].v1);
    pair->set_value(award[*it].v2);
  }
}

void FillToKVPair2(
    ModifyCurrency* modify, AddSubItemSet* item_set,
    __OUT__ google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items,
    bool split = false) {
  if (!out_items) return;

  if (modify) {
    for (int32_t i = MoneyKind_MIN; i < MoneyKind_ARRAYSIZE; ++i) {
      if ((*modify)[i]) {
        KVPair2* pair = out_items->Add();
        pair->set_key(i);
        pair->set_value((*modify)[i]);
      }
    }
  }

  if (item_set) {
    for (AddSubItemSet::const_iterator it = item_set->begin();
         it != item_set->end(); ++it) {
      if (split) {
        for (int32_t i = 0; i < it->item_count; i++) {
          KVPair2* pair = out_items->Add();
          pair->set_key(it->item_id);
          pair->set_value(1);
        }
      } else {
        KVPair2* pair = out_items->Add();
        pair->set_key(it->item_id);
        pair->set_value(it->item_count);
      }
    }
  }
}

static const boost::shared_ptr<Message> kEmptyMessage;

LogicPlayer::LogicPlayer(int64_t uid)
    : Player(uid),
      status_(0),
      last_update_time_(0),
      online_time_(0),
      last_heart_beat_time_(0),
      item_seed_(1),
      ship_seed_(1),
      flop_2_count_(0),
      pk_rank_reward_time_(0),
      pk_rank_reward_rank_(0),
      pk_max_rank_(-1),
      last_pk_time_(0),
      pk_player_id_(0),
      last_server_mail_id_(0),
      patrol_total_time_(0),
      max_fight_attr_(0),
      fight_attr_(0),
      chat_time_(0),
      army_id_(0),
      army_leave_time_(0),
      month_card_(0),
      big_month_card_(0),
      life_card_(0),
      festival_shop_refresh_time_(0),
      medal_copy_id_(0),
      medal_achi_(0) {
  (void)item_seed_;
  (void)ship_seed_;
  bzero(&this->month_card_1_, sizeof(this->month_card_1_));
  bzero(&this->month_card_2_, sizeof(this->month_card_2_));
  bzero(&this->weekly_card_, sizeof(this->weekly_card_));
  this->tower_state_.set_random_buff("");
}

LogicPlayer::~LogicPlayer() {
  DEBUG_LOG(logger)("Player:%ld deleted", this->uid());
}

int32_t LogicPlayer::create_days() const {
  if (this->player_.create_time() == 0) return 1;
  return GetSecondsDiffDays(this->player_.create_time(), GetSeconds()) + 1;
}

int32_t LogicPlayer::dstrike_exploit() const {
  return this->dstrike_info_.merit();
}

int64_t LogicPlayer::dstrike_damage() const {
  return this->dstrike_info_.damage();
}

int32_t LogicPlayer::avatar() const { return this->player_.avatar(); }

//没有创建玩家, 1分钟以上就干掉玩家对象
//1级,没抽过船,离线10分钟以上
//或者超时60分钟以上
bool LogicPlayer::can_be_delete() {
  if (!this->session().lock() && this->create_player() &&
      GetSeconds() - this->last_active_time() >= 60)
    return true;
  if (GetSeconds() - this->last_active_time() < 10 * 60) return false;
  if (this->level() <= 1 && this->ships_.size() <= 1) return true;
  if (GetSeconds() - this->last_active_time() >= 60 * 60) return true;
  return false;
}

int32_t LogicPlayer::army_war_count() const {
  VectorMap<int32_t, int32_t>::const_iterator iter = this->daily_counter_.find(COUNT_TYPE_ARMY_WAR_COUNT);
  return iter != this->daily_counter_.end() ? iter->second : 0;
}

void LogicPlayer::SendMessageToFriends(uint16_t msgid, Message* pMsg) {
  for (int32_t i = 0; i < this->friends_.infos_size(); ++i) {
    const sy::FriendInfo& info = this->friends_.infos(i);
    LogicPlayer* player = server->GetPlayerByID(info.friend_id());
    if (player && player->is_online() && info.type() == FRIEND_STATUS_FRIEND) {
      player->SendMessageToClient(msgid, pMsg);
    }
  }
}

void LogicPlayer::SendMessageToClient(uint16_t msgid, Message* pMsg) {
  const TcpSessionPtr& session = this->session().lock();
  if (session) {
    server->SendMessageToClient(session.get(), msgid, pMsg);
  }
}

void LogicPlayer::SendMessageToDB(uint16_t msgid, Message* pMsg) {
 server->SendPlayerMessageToDB(this->uid(), msgid, pMsg);
}

void LogicPlayer::SendErrorCodeToClient(int32_t error_code, uint16_t msgid) {
  MessageErrorCode message;
  message.set_err_code(ResultID(error_code));
  message.set_msg_id(msgid);
  this->SendMessageToClient(MSG_CS_ERROR_CODE, &message);
}

void LogicPlayer::SendLoadPlayer(int64_t msgid) {
  MessageSSRequestGetPlayerInfo request;
  request.set_uid(this->uid());
  request.set_msgid(msgid);
  server->SendServerMessageToDB(MSG_SS_REQUEST_GET_PLAYER_INFO, &request);
}

int32_t LogicPlayer::ProcessPlayerNotExist(SSMessageEntry& entry) {
  MessageSSResponsePlayerNotExist *message = static_cast<MessageSSResponsePlayerNotExist*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->status_ |= PLAYER_LOADING;
  this->status_ |= PLAYER_CREATE_PLAYER;
  if (message->msgid()) {
    this->SendErrorCodeToClient(ERR_PLAYER_NEED_CREATE, MSG_CS_REQUEST_LOAD_PLAYER);
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestHeartBeat(CSMessageEntry& entry) {
  MessageRequestHeartBeat* message =
      static_cast<MessageRequestHeartBeat*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->active();
  this->FetchServerMail();

  MessageResponseHeartBeat response;
  response.set_sec(GetMilliSeconds() + time_offset * 1000);
  response.set_millisec(message->millisec());
  response.set_version(server_config->version());

  this->SendMessageToClient(MSG_CS_RESPONSE_HEART_BEAT, &response);
  return ERR_OK;
}

const std::string kDirtyWords[] = {",", "|", " or ", " where", "\"", "\'", " ", ".", ";", "?", "%"};

static inline bool ContainsInjection(const std::string& name) {
  for (int32_t i = 0; i < ArraySize(kDirtyWords); ++i) {
    if (name.find(kDirtyWords[i]) != std::string::npos) return true;
  }
  return false;
}

int32_t LogicPlayer::ProcessRequestCreatePlayer(CSMessageEntry& entry) {
  MessageRequestCreatePlayer* message =
      static_cast<MessageRequestCreatePlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->name().size() >= 64) return ERR_PARAM_INVALID;

  if (ContainsInjection(message->name())) return ERR_PLAYER_ILLEGAL_CHARACTER;
  if (server_config->dirtywords() && ContainsDirtyWords(message->name()))
    return ERR_PLAYER_ILLEGAL_CHARACTER;

  if (this->load() && this->create_player()) {
    const std::string& player_name = PrefixBase::MakeName(this->player_.server(), message->name());

    if (int32_t(player_name.length()) < Setting::kNameMinLen) return ERR_PLAYER_NAME_LEN_MIN;
    if (int32_t(player_name.length()) > Setting::kNameMaxLen) return ERR_PLAYER_NAME_LEN_MAX;
    if (server->GetUIDByName(player_name)) return ERR_PLAYER_NAME_EXIST;

    MessageSSRequestCreatePlayer request;
    PlayerInfo* info = request.mutable_info();
    info->set_openid(this->account());
    info->set_uid(this->uid());
    info->set_name(player_name);
    info->set_server(this->player_.server());
    info->set_avatar(message->avatar());
    info->set_create_time(GetSeconds());
    info->set_channel(this->channel_);
    info->set_device_id(this->idfa_);

    info->set_vip_level(0);
    info->set_vip_exp(0);
    info->set_money(0);

    info->set_login_days(1);
    info->set_level(1);
    info->set_exp(0);
    info->set_coin(0);
    info->set_oil(GetSettingValue(born_oil));
    info->set_last_oil_time(GetSeconds());
    info->set_energy(GetSettingValue(born_engry));
    info->set_last_energy_time(GetSeconds());
    info->set_prestige(0);
    info->set_plane(0);
    info->set_hero(0);
    info->set_muscle(0);
    info->set_exploit(0);
    info->set_union_(0);

    //舰船
    HeroInfo* hero = request.add_ships();
    hero->set_hero_id(Setting::GetValue(Setting::born_ship_id));
    this->UpdateHeroInfo(*hero, kNotifyNone, SYSTEM_ID_BASE,
                         MSG_CS_REQUEST_CREATE_PLAYER);

    //阵型
    TacticInfo *tactic = request.mutable_tactic();
    PositionInfo* pos = tactic->add_infos();
    pos->set_position(1);
    pos->set_hero_uid(hero->uid());
    PositionInfo* battle_pos = tactic->add_battle_pos();
    battle_pos->set_position(1);
    battle_pos->set_hero_uid(hero->uid());

    //航母
    if (Setting::GetValue(Setting::carrier_born_id)) {
      CarrierInfo* carrier = request.mutable_carrier();
      carrier->set_carrier_id(Setting::GetValue(Setting::carrier_born_id));
    }

    //初始化道具
    const ValuePair2Vec& add_items = Setting::GetValue2(Setting::initial_carry_id);
    for (size_t i = 0; i < add_items.size(); ++i) {
      if (add_items[i].v1 >= Setting::kShipStartID &&
          add_items[i].v1 <= Setting::kShipEndID) {
        HeroInfo* info = request.add_ships();
        info->set_hero_id(add_items[i].v1);
        this->UpdateHeroInfo(*info, kNotifyNone, SYSTEM_ID_BASE,
                             MSG_CS_REQUEST_CREATE_PLAYER);
      } else {
        sy::Item* item = request.add_items();
        item->set_uid(i + 1);
        item->set_item_id(add_items[i].v1);
        item->set_count(add_items[i].v2);
      }
    }

    this->last_server_mail_id_ = server->GetNewMailID();
    request.set_last_server_mail_id(this->last_server_mail_id_);

    TRACE_LOG(logger)("BeginCreatePlayer:%ld, %s", this->uid(), request.info().name().c_str());
    request.set_tid(server->GetTID());
    request.set_msgid(entry.head.msgid);
    request.set_system(SYSTEM_ID_TIME);
    this->SendMessageToDB(MSG_SS_REQUEST_CREATE_PLAYER, &request);
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseCreatePlayer(SSMessageEntry& entry) {
  MessageSSResponseCreatePlayer* message =
      static_cast<MessageSSResponseCreatePlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->status_ &= ~PLAYER_CREATE_PLAYER;
  //this->SendLoadPlayer(0);
  server->AddNewName(message->name(), this->uid());

  //创建角色成功
  TRACE_LOG(logger)("EndCreatePlayer:%ld, %s", this->uid(),
                    message->name().c_str());
  //这边记录一下登录
  {
    const TcpSessionPtr& session = this->session().lock();
    MessageSSUpdateLoginInfo msg;
    msg.set_tid(server->GetTID());
    msg.set_online_time(0);
    msg.set_login(1);
    msg.set_ipaddr(session ? session->IpAddr() : "");
    this->SendMessageToDB(MSG_SS_UPDATE_LOGIN_INFO, &msg);
  }

  MessageResponseCreatePlayer msg;
  msg.set_name(message->name());
  this->SendMessageToClient(MSG_CS_RESPONSE_CREATE_PLAYER, &msg);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestChangeName(CSMessageEntry& entry) {
  MessageRequestChangeName *message = static_cast<MessageRequestChangeName*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->has_name()) {
    if (ContainsInjection(message->name())) return ERR_PLAYER_ILLEGAL_CHARACTER;
    if (server_config->dirtywords() && ContainsDirtyWords(message->name()))
      return ERR_PLAYER_ILLEGAL_CHARACTER;
    if (int32_t(message->name().length()) < Setting::kNameMinLen) return ERR_PLAYER_NAME_LEN_MIN;
    if (int32_t(message->name().length()) > Setting::kNameMaxLen) return ERR_PLAYER_NAME_LEN_MAX;

    ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_BASE);
    modify[MONEY_KIND_MONEY] -= GetSettingValue(change_name_cost);
    CHECK_RET(this->CheckCurrency(modify));

    const std::string& player_name =
        PrefixBase::MakeName(this->player_.server(), message->name());
    if (server->GetUIDByName(player_name)) return ERR_PLAYER_NAME_EXIST;

    MessageSSRequestChangeName request;
    request.set_name(player_name);
    this->SendMessageToDB(MSG_SS_REQUEST_CHANGE_NAME, &request);
    return ERR_OK;
  }

  if (message->has_avatar()) {
    this->player_.set_avatar(message->avatar());
    MessageSSChangeAvatar request;
    request.set_avatar(message->avatar());
    this->SendMessageToDB(MSG_SS_UPDATE_CHANGE_AVATAR, &request);

    MessageResponseChangeName response;
    response.set_name(this->name());
    response.set_avatar(message->avatar());
    this->SendMessageToClient(MSG_CS_RESPONSE_CHANGE_NAME, &response);
    return ERR_OK;
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseChangeName(SSMessageEntry& entry) {
  MessageSSResponseChangeName* message =
      static_cast<MessageSSResponseChangeName*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->name().empty()) return ERR_INTERNAL;
  if (!message->name().empty()) {
    server->RemoveName(this->player_.name());
  }
  server->AddNewName(message->name(), this->uid());
  this->player_.set_name(message->name());

  ModifyCurrency modify(MSG_CS_REQUEST_CHANGE_NAME, SYSTEM_ID_BASE);
  modify[MONEY_KIND_MONEY] -= GetSettingValue(change_name_cost);
  if (this->CheckCurrency(modify)) {
    ERROR_LOG(logger)("ChangeName, PlayerID:%ld, Money not enough", this->uid());
  }
  this->UpdateCurrency(modify);

  MessageResponseChangeName response;
  response.set_name(message->name());
  response.set_avatar(this->player_.avatar());
  this->SendMessageToClient(MSG_CS_RESPONSE_CHANGE_NAME, &response);

  Army* army = server->GetArmyByID(this->army_id());
  if (army) {
    sy::ArmyMemberInfo* member_info = army->GetMember(this->uid());
    if (member_info) {
      member_info->set_name(message->name());
      army->UpdateMember(this->uid());
    }
    if(army->info().master_id() == this->uid())
      army->SetMaster(this->uid(), this->name());
  }

  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetPlayerInfo(SSMessageEntry& entry) {
  MessageSSResponsePlayerInfo *message = static_cast<MessageSSResponsePlayerInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->player_.CopyFrom(message->player());
  server->AddNewName(this->player_.name(), this->uid());
  this->fresh_time_ = message->fresh_time();
  this->last_update_time_ = this->is_online() ? GetVirtualSeconds() : message->last_login_time();

  this->daily_counter_.clear();
  for (int32_t i = 0; i < message->buy_count_size(); ++i) {
    this->daily_counter_[message->buy_count(i).key()] = message->buy_count(i).value();
  }
  this->dstrike_info_.CopyFrom(message->dstrike());

  //矫正汽油信息
  if (!this->player_.last_oil_time()) {
    this->player_.set_last_oil_time(GetSeconds());
    this->player_.set_oil(Setting::GetValue(Setting::born_oil));
  }

  achievements_.clear();
  for (int32_t i = 0; i < message->achievements_size(); ++i) {
    this->achievements_[message->achievements(i).key()] =
        message->achievements(i).value();
  }
  this->recharge_.clear();
  for (int32_t i = 0; i < message->recharge_size(); ++i) {
    this->recharge_.push_back(message->recharge(i));
  }
  activity_record_new_.clear();
  for (int32_t i = 0; i < message->records_new_size(); ++i) {
    int32_t atype = message->records_new(i).type();
    int32_t aid = message->records_new(i).id();
    TRACE_LOG(logger)("LoadActivityRecord: player_id:%ld, type:%d, id:%d", this->uid(),atype, aid);
    activity_record_new_[std::make_pair(atype, aid)].CopyFrom(
        message->records_new(i));
  }

  //矫正外网玩家的登录天数
  if (this->player_.login_days() < this->player_.sign_id()) {
    this->player_.set_login_days(this->player_.sign_id());
  }
  TRACE_LOG(logger)("PlayerID:%ld, GetPlayerInfo, openid:%s, LoginDays:%d, fresh_time:%d",
                    this->uid(), this->player_.openid().c_str(),
                    this->player_.login_days(), this->fresh_time_);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetItemInfo(SSMessageEntry& entry) {
  MessageSSResponseGetItemInfo* message =
      static_cast<MessageSSResponseGetItemInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->items_.clear();
  for (int32_t i = 0; i < message->items_size(); ++i) {
    Item* item = message->mutable_items(i);
    LogicItem logic_item;
    if (LogicItem::CreateItem(*item, logic_item)) {
      this->items().AddItem(logic_item);
    } else {
      ERROR_LOG(logger)("Player:%ld, ItemID:%d not found, Item UID:%ld",
                        this->uid(), item->item_id(), item->uid());
    }
    this->item_seed_ = std::max(item->uid(), this->item_seed_);
  }
  this->equips_.CopyFrom(message->equips());

  RepeatedField<int64_t>* array[] = {
      this->equips_.mutable_equips_1(), this->equips_.mutable_equips_2(),
      this->equips_.mutable_equips_3(), this->equips_.mutable_equips_4(),
      this->equips_.mutable_equips_5(), this->equips_.mutable_equips_6(),
  };
  for (int32_t i = 0; i < ArraySize(array); ++i) {
    array[i]->Resize(sy::MAX_EQUIPED_ITEM_COUNT, 0);
  }
  TRACE_LOG(logger)("PlayerID:%ld, GetItemInfo, item count:%d", this->uid(), message->items_size());
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetTacticInfo(SSMessageEntry& entry) {
  MessageSSResponseGetTacticInfo* message =
      static_cast<MessageSSResponseGetTacticInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  sy::TacticInfo correct;
  correct.Clear();
  sy::TacticInfo& tactic_info = *message->mutable_info();
  for (int32_t j = 0; j < tactic_info.infos_size(); ++j) {
    if (this->GetHeroByUID(tactic_info.infos(j).hero_uid())) {
      correct.add_infos()->CopyFrom(tactic_info.infos(j));
    }
  }
  this->tactic_.CopyFrom(correct);

  for (int32_t i = 0; i < tactic_info.battle_pos_size(); ++i) {
    if (this->IsInTactic(tactic_info.battle_pos(i).hero_uid())) {
      sy::PositionInfo* info = correct.mutable_battle_pos()->Add();
      info->CopyFrom(tactic_info.battle_pos(i));
    }
  }
  this->tactic_.CopyFrom(correct);
  if (this->tactic_.battle_pos_size() < this->tactic_.infos_size()) {
    correct.mutable_battle_pos()->CopyFrom(tactic_info.infos());
  }

  for (int32_t i = 0; i < tactic_info.support_pos_size(); ++i) {
    if (this->GetHeroByUID(tactic_info.support_pos(i).hero_uid())) {
      correct.add_support_pos()->CopyFrom(tactic_info.support_pos(i));
    }
  }
  this->tactic_.CopyFrom(correct);

  obtained_carriers_.clear();
  for (int32_t i = 0; i < message->obtained_carriers_size(); i++) {
    obtained_carriers_.insert(message->obtained_carriers(i));
  }
  this->CalcChart();
  this->max_fight_attr_ = message->max_fight_attr();
  this->army_id_ = message->army_id();
  this->army_skill_.resize(sy::ArmySkill_ARRAYSIZE, 0);
  for (int32_t i = 0; i < message->army_skill_size(); ++i) {
    this->army_skill_[i] = message->army_skill(i);
  }
  this->army_leave_time_ = message->leave_time();

  return ERR_OK;
}

LogicHero* LogicPlayer::GetHeroByPos(int32_t pos) {
  for (int32_t index = 0; index < this->tactic_.battle_pos_size(); ++index) {
    if (this->tactic_.battle_pos(index).position() == pos) {
      return this->GetHeroByUID(this->tactic_.battle_pos(index).hero_uid());
    }
  }
  return NULL;
}

LogicHero* LogicPlayer::GetHeroByUID(int64_t uid) {
  if (!uid) return NULL;
  for (std::vector<LogicHero>::iterator iter = this->ships_.begin();
       iter != this->ships_.end(); ++iter) {
    if (iter->first.uid() == uid) {
      return &*iter;
    }
  }
  return NULL;
}

void LogicPlayer::DeleteHeroByUID(int64_t uid) {
  for (std::vector<LogicHero>::iterator iter = this->ships_.begin();
       iter != this->ships_.end(); ++iter) {
    if (iter->first.uid() == uid) {
      std::iter_swap(this->ships_.end() - 1, iter);
      this->ships_.pop_back();
      break;
    }
  }
}

sy::CarrierInfo* LogicPlayer::GetCarrierByID(int32_t carrier_id) {
  for (std::vector<sy::CarrierInfo>::iterator iter = this->carriers_.begin();
       iter != this->carriers_.end(); ++iter) {
    if (iter->carrier_id() == carrier_id) {
      return &*iter;
    }
  }
  return NULL;
}

void LogicPlayer::UpdateCarrier(sy::CarrierInfo* info) {
  MessageNotifyCarrierInfo notify;
  MessageSSUpdateCarrierInfo request;
  notify.mutable_info()->CopyFrom(*info);
  request.mutable_info()->CopyFrom(*info);
  request.set_tid(server->GetTID());

  this->SendMessageToDB(MSG_SS_UPDATE_CARRIER_INFO, &request);
  this->SendMessageToClient(MSG_CS_NOTIFY_CARRIER_INFO, &notify);
}

void LogicPlayer::AddCarrier(int32_t carrier_id) {
  CarrierBase* base = CARRIER_BASE.GetEntryByID(carrier_id).get();
  if (!base) return;
  if (this->GetCarrierByID(carrier_id)) return;

  sy::CarrierInfo info;
  info.set_carrier_id(carrier_id);
  this->carriers_.push_back(info);

  this->UpdateCarrier(&this->carriers_.back());
  if (obtained_carriers_.insert(carrier_id).second) {
    MessageSSUpdateObtainedCarriers msg;
    MessageNotifyObtainedCarriers notify;

    for (VectorSet<int32_t>::iterator it = this->obtained_carriers_.begin();
         it != this->obtained_carriers_.end(); ++it) {
      msg.add_carrier_id(*it);
      notify.add_carrier_id(*it);
    }
    SendMessageToDB(MSG_SS_UPDATE_OBTAINED_CARRIERS, &msg);
    SendMessageToClient(MSG_CS_NOTIFY_OBTAINED_CARRIERS, &notify);
  }
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_CARRIER_BUILD,
                       MSG_CS_REQUEST_ACTIVE_CARRIER);
}

int32_t LogicPlayer::ProcessResponseGetHeroInfo(SSMessageEntry& entry) {
  MessageSSResponseGetHeroInfo* message =
      static_cast<MessageSSResponseGetHeroInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->ships_.clear();
  for (int32_t i = 0; i < message->ships_size(); ++i) {
    sy::HeroInfo* from = message->mutable_ships(i);
    this->ship_seed_ = std::max(from->uid(), this->ship_seed_);
    this->UpdateHeroInfo(*from, kNotifyNone, 0, MSG_SS_GET_HERO_INFO);
  }
  this->hero_research_info_.CopyFrom(message->research());

  TRACE_LOG(logger)("PlayerID:%ld, GetHeroInfo, ship count:%d", this->uid(), message->ships_size());
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetReportAbstract(SSMessageEntry& entry) {
  MessageSSGetReportAbstract* message =
      static_cast<MessageSSGetReportAbstract*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->report_abstract_.clear();
  for (int32_t i = 0; i < message->report_abstract_size(); ++i) {
    const std::string& str = message->report_abstract(i);
    this->report_abstract_[atoi(str.c_str())].push_back(str);
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetCopyInfo(SSMessageEntry& entry) {
  MessageSSResponseGetCopyInfo* message =
      static_cast<MessageSSResponseGetCopyInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->copy_progress_.clear();
  for (int32_t i = 0; i < message->progress_size(); ++i) {
    this->copy_progress_.push_back(message->progress(i));
  }

  this->copy_star_.clear();
  for (int32_t index = 0; index < message->copy_star_size(); ++index) {
    const sy::CopyStarInfo &info = message->copy_star(index);
    this->copy_star_.insert(std::make_pair(info.copy_id(), info.star()));
  }

  this->copy_count_.clear();
  for (int32_t index = 0; index < message->copy_count_size(); ++index) {
    const sy::CopyCount& info = message->copy_count(index);
    this->copy_count_.insert(std::make_pair(info.copy_id(), info.count()));
  }

  this->passed_copy_.clear();
  for (int32_t index = 0; index < message->passed_copy_size(); ++index) {
    this->passed_copy_.push_back(message->passed_copy(index));
  }

  this->chapter_award_.clear();
  for (int32_t index = 0; index < message->chapter_award_size(); ++index) {
    const sy::ChapterAwardInfo& info = message->chapter_award(index);
    this->chapter_award_[info.chapter()] = info.mask();
  }

  this->gate_award_.clear();
  for (int32_t index = 0; index < message->gate_award_size(); ++index) {
    this->gate_award_.push_back(message->gate_award(index));
  }

  this->tower_state_ = message->tower_stat();
  if (this->tower_state_.random_buff().length() <= 3u) {
    this->tower_state_.set_random_buff("");
  }
  INFO_LOG(logger)("PlayerID:%ld, max_star_order:%d", this->uid(), this->tower_state_.max_star_order());

  this->tower_buff_.clear();
  for (int32_t i = 0; i < message->tower_buff_size(); ++i) {
    this->tower_buff_[message->tower_buff(i).key()] = message->tower_buff(i).value();
  }

  this->carrier_copy_info_.CopyFrom(message->carrier_copy_info());
  this->carrier_copy_.clear();
  for (int32_t i = 0; i < message->carrier_copy_size(); ++i) {
    if (this->carrier_copy_.full()) break;
    this->carrier_copy_.push_back(message->carrier_copy(i));
  }

  this->medal_copy_id_ = message->medal_copy_id();
  if (!this->medal_copy_id_) {
    this->medal_copy_id_ = this->RefreshMedalCopyID();
    MessageUpdateMedalCopyID upmcid;
    upmcid.set_medal_copy_id(this->medal_copy_id_);
    this->SendMessageToDB(MSG_SS_UPDATE_MEDAL_COPY_ID, &upmcid);
  }

  this->medal_star_ = message->medal_star();
  this->medal_state_ = message->medal_state();
  this->medal_achi_ = message->medal_achi();

  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetShopInfo(SSMessageEntry& entry) {
  MessageSSResponseGetShopInfo* message =
      static_cast<MessageSSResponseGetShopInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  refresh_shop_info_.clear();
  for (int32_t i = 0; i < message->refresh_commodity_size(); ++i) {
    const RefreshShopInfo& info = message->refresh_commodity(i);
    refresh_shop_info_[info.shop_id()].CopyFrom(info);
  }

  this->normal_shop_info_.clear();
  for (int32_t i = 0; i < message->normal_commodity_size(); ++i) {
    const sy::ShopCommodityInfo* info = message->mutable_normal_commodity(i);
    this->normal_shop_info_[info->commodity_id()] = info->bought_count();
  }
  this->life_shop_info_.clear();
  for (int32_t i = 0; i < message->life_commodity_size(); ++i) {
    const sy::ShopCommodityInfo* info = message->mutable_life_commodity(i);
    this->life_shop_info_[info->commodity_id()] = info->bought_count();
  }

  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetCarrierInfo(SSMessageEntry& entry) {
  MessageSSResponseGetCarrierInfo* message =
      static_cast<MessageSSResponseGetCarrierInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  //航母信息
  this->carriers_.clear();
  for (int32_t i = 0; i < message->carrier_size(); ++i) {
    sy::CarrierInfo& from = *message->mutable_carrier(i);
    sy::CarrierInfo* info = this->GetCarrierByID(from.carrier_id());
    const CarrierBase* base = CARRIER_BASE.GetEntryByID(from.carrier_id()).get();
    if (!base) {
      ERROR_LOG(logger)("%s, PlayerID:%ld, CarrierID:%d not found", __PRETTY_FUNCTION__, this->uid(), from.carrier_id());
      continue;
    }

    if (info) {
      info->CopyFrom(from);
    } else {
      this->carriers_.push_back(from);
    }
  }
  //当前装备的航母
  this->current_carrier_.CopyFrom(message->current_carrier());
  TRACE_LOG(logger)("PlayerID:%ld, GetCarrierInfo, carrier count:%d",
                    this->uid(), message->carrier_size());
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetPatrolInfo(SSMessageEntry& entry) {
  MessageSSGetPatrolInfo* message =
      static_cast<MessageSSGetPatrolInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->patrol_total_time_ = message->total_time();
  this->patrol_infos_.clear();
  for (int32_t i = 0; i < message->infos_size(); ++i) {
    const sy::PatrolInfo& info = message->infos(i);
    if (HERO_BASE.GetEntryByID(info.ship_uid()).get()) {
      this->patrol_infos_[info.patrol_id()].CopyFrom(info);
    }
  }
  return ERR_OK;
}

int32_t LogicPlayer::CheckItem(AddSubItemSet* update_items,
                               DeleteItemSet* delete_items,
                               NotifyItemSet* notify_set) {
  if (update_items) {
    AddSubItemSet temp_items;
    DetachShipItem(*update_items, &temp_items, NULL);
    update_items = &temp_items;

    for (AddSubItemSet::iterator iter = update_items->begin();
         iter != update_items->end(); ++iter) {
      LogicItem* item = this->items().GetItemByItemID(iter->item_id);
      //增加
      if (iter->item_count > 0) {
        if (item && item->count() + iter->item_count >= item->max_count()) {
          iter->item_count = item->max_count() - item->count();
        } else {
          const std::pair<int32_t, boost::shared_ptr<ConfigEntry> >& pair =
              LogicItem::GetItemParam(iter->item_id);
          if (!pair.second) return ERR_ITEM_INVALID_PARAM;
          if (pair.first == sy::ITEM_TYPE_ITEM) {
            const ItemBasePtr& base = boost::static_pointer_cast<ItemBase>(pair.second);
            if (base->max_stack < iter->item_count)
              iter->item_count = base->max_stack;
          }
        }
      } else {
        if (!item || item->count() + iter->item_count < 0) {
          ERROR_LOG(logger)("Item need more error id:%d, count:%d", iter->item_id, iter->item_count);
          return ERR_ITEM_NEED_MORE;
        }
      }
    }
  }
  if (delete_items) {
    VectorSet<int64_t> set;
    for (DeleteItemSet::iterator iter = delete_items->begin();
         iter != delete_items->end(); ++iter) {
      if (!this->items().GetItemByUniqueID(*iter)) return ERR_ITEM_NEED_MORE;
      if (!set.insert(*iter).second) return ERR_PARAM_INVALID;
    }
  }
  if (notify_set) {
    for (NotifyItemSet::iterator iter = notify_set->begin();
         iter != notify_set->end(); ++iter) {
      if (!this->items().GetItemByUniqueID(*iter)) return ERR_ITEM_NOT_FOUND;
    }
  }
  return ERR_OK;
}

LogicItem* LogicPlayer::AddItem(int32_t item_id, int32_t item_count, sy::Item* attr) {
  LogicItem item;
  ItemParam param(item_id, item_count);
  if (LogicItem::CreateItem(param, item)) {
    if (attr) item.CopyAttr(*attr);
    item.item().set_uid(++this->item_seed_);
    this->items_.AddItem(item);
    return this->items_.GetItemByUniqueID(item.uid());
  }
  return NULL;
}

void LogicPlayer::OnItemCountChanged(int32_t item_id, int32_t count) {
  INFO_LOG(logger)("PlayerID:%ld, ItemID:%d, Count:%d", this->uid(), item_id, count);
  if (item_id >= GetSettingValue(armyparts_start_id) &&
      item_id < GetSettingValue(armyparts_end_id)) {
    if (count)
      server->InsertIntoRobList(item_id, this->uid());
    else
      server->EraseFromRobList(item_id, this->uid());
  }
}

int32_t LogicPlayer::ObtainItem(const AddSubItemSet* update_items,
                                const DeleteItemSet* delete_set,
                                const NotifyItemSet* notify_set, int32_t msg_id,
                                int32_t sys_id) {
  MessageNotifyItemInfo notify;
  notify.set_message_id(msg_id);
  MessageSSUpdateItemInfo msg;
  msg.set_tid(server->GetTID());
  msg.set_msgid(msg_id);
  msg.set_system(sys_id);
  msg.set_tid(server->GetTID());
  LogicItem logic_item;

  if (update_items) {
    AddSubItemSet temp_items;
    AddSubItemSet temp_ships;
    DetachShipItem(*update_items, &temp_items, &temp_ships);
    if (!AddShipItem(temp_ships, sys_id, msg_id)) return ERR_HERO_NOT_FOUND;
    update_items = &temp_items;

    for (AddSubItemSet::const_iterator iter = update_items->begin();
         iter != update_items->end(); ++iter) {
#ifdef DEBUG
      if (iter->item_id < 100) assert(false && "item_id");
#endif
      if (!iter->item_count) continue;
      int32_t max_count = LogicItem::GetMaxCount(iter->item_id);
      //不能叠加(要减少不能叠加的物品,只能通过UID删除)
      if (max_count == 1) {
        if (iter->item_count <= 0) return ERR_ITEM_INVALID_PARAM;
        //增加多个装备
        for (int32_t i = 0; i < iter->item_count && i < 256; ++i) {
          if (LogicItem::CreateItem(*iter, logic_item)) {
            logic_item.count(1);
            logic_item.item().set_uid(++this->item_seed_);
            this->items_.AddItem(logic_item);

            this->OnItemCountChanged(iter->item_id, logic_item.count());
            msg.add_update_items()->CopyFrom(logic_item.item());
            notify.add_update_items()->CopyFrom(logic_item.item());
          } else {
            return ERR_ITEM_CREATE_FAIL;
          }
        }
      } else {
        LogicItem* item = this->items().GetItemByItemID(iter->item_id);
        if (iter->item_count < 0 &&
            (!item || item->count() + iter->item_count < 0))
          return ERR_ITEM_NEED_MORE;

        int32_t current_count = 0;
        if (item && item->count() + iter->item_count > max_count) {
          current_count = item->count() + iter->item_count;
        } else {
          current_count = std::min(
              (item ? item->count() : 0) + iter->item_count, max_count);
        }

        this->OnItemCountChanged(iter->item_id, current_count);
        //个数减到0了
        if (current_count == 0) {
          if (item) {
            msg.add_delete_items(item->uid());
            notify.add_delete_items(item->uid());
            this->items_.RemoveItem(item->uid());
          }
          continue;
        }

        if (item) {  //更新
          item->count(current_count);
          msg.add_update_items()->CopyFrom(item->item());
          notify.add_update_items()->CopyFrom(item->item());
        } else {  //插入
          if (LogicItem::CreateItem(*iter, logic_item)) {
            logic_item.item().set_uid(++this->item_seed_);
            this->items_.AddItem(logic_item);

            msg.add_update_items()->CopyFrom(logic_item.item());
            notify.add_update_items()->CopyFrom(logic_item.item());
            continue;
          }
          return ERR_ITEM_CREATE_FAIL;
        }
      }
    }
  }

  if (delete_set) {
    for (DeleteItemSet::const_iterator iter = delete_set->begin();
         iter != delete_set->end(); ++iter) {
      this->items_.RemoveItem(*iter);
      msg.add_delete_items(*iter);
      notify.add_delete_items(*iter);
    }
  }

  if (notify_set) {
    for (NotifyItemSet::const_iterator iter = notify_set->begin();
         iter != notify_set->end(); ++iter) {
      LogicItem* item = this->items().GetItemByUniqueID(*iter);
      if (!item) return ERR_ITEM_NOT_FOUND;
      msg.add_update_items()->CopyFrom(item->data());
      notify.add_update_items()->CopyFrom(item->data());
    }
  }

  if (notify.delete_items_size() + notify.update_items_size()) {
    for (int32_t i = 0; i < msg.update_items_size(); ++i) {
      DEBUG_LOG(logger)(
          "player:%ld, update item:%ld, item_id:%d, item_count:%d", this->uid(),
          msg.update_items(i).uid(), msg.update_items(i).item_id(),
          msg.update_items(i).count());
    }
    for (int32_t i = 0; i < msg.delete_items_size(); ++i) {
      DEBUG_LOG(logger)("player:%ld, delete item:%ld", this->uid(),
                        msg.delete_items(i));
    }
    this->SendMessageToDB(MSG_SS_UPDATE_ITEM_INFO, &msg);

    this->SendMessageToClient(MSG_CS_NOTIFY_ITEM_INFO, &notify);
  }
  return ERR_OK;
}

int32_t LogicPlayer::CheckItem2(UpdateItemSet* update_set,
                                DeleteItemSet* delete_set,
                                NotifyItemSet* notify_set) {
  AddSubItemSet add_sub_set;
  NotifyItemSet notify_set_2;
  if (notify_set) {
    for (NotifyItemSet::const_iterator iter = notify_set->begin();
         iter != notify_set->end(); ++iter) {
      notify_set_2.push_back(*iter);
    }
  }

  if (update_set) {
    for (UpdateItemSet::iterator iter = update_set->begin();
         iter != update_set->end(); ++iter) {
      if (iter->uid())
      {
        if (!this->items().GetItemByUniqueID(iter->uid())) return ERR_ITEM_NOT_FOUND;
        notify_set_2.push_back(iter->uid());
      } else {
        ItemParam param(iter->item_id(), iter->count());
        add_sub_set.push_back(param);
      }
    }
  }

  return this->CheckItem(add_sub_set.size() ? &add_sub_set : NULL, delete_set,
                         notify_set_2.size() ? &notify_set_2 : NULL);
}

int32_t LogicPlayer::ObtainItem2(UpdateItemSet* update_set,
                                 const DeleteItemSet* delete_set,
                                 NotifyItemSet* notify_set, int32_t msg_id,
                                 int32_t sys_id) {
  AddSubItemSet add_sub_set;
  NotifyItemSet notify_set_2;
  if (notify_set) {
    for (NotifyItemSet::const_iterator iter = notify_set->begin();
         iter != notify_set->end(); ++iter) {
      notify_set_2.push_back(*iter);
    }
  }

  if (update_set) {
    for (UpdateItemSet::iterator iter = update_set->begin();
         iter != update_set->end(); ++iter) {
      if (!iter->count()) continue;
      if (iter->uid())
      {
        LogicItem *item = this->items().GetItemByUniqueID(iter->uid());
        if (!item) return ERR_ITEM_NOT_FOUND;
        item->CopyAttr(*iter);
        notify_set_2.push_back(iter->uid());
      } else {
        if (LogicItem::GetMaxCount(iter->item_id()) > 1) {
          ItemParam param(iter->item_id(), iter->count());
          add_sub_set.push_back(param);
        } else {
          LogicItem* item = this->AddItem(iter->item_id(), 1, iter);
          if (!item) return ERR_ITEM_CREATE_FAIL;
          notify_set_2.push_back(item->uid());
        }
      }
    }
  }
  return this->ObtainItem(add_sub_set.size() ? &add_sub_set : NULL, delete_set,
                          notify_set_2.size() ? &notify_set_2 : NULL, msg_id,
                          sys_id);
}

int32_t LogicPlayer::ProcessResponseGetMailInfo(SSMessageEntry& entry) {
  MessageSSResponseGetMailInfo *message = static_cast<MessageSSResponseGetMailInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  INFO_LOG(logger)("PlayerID:%ld, MailCount:%d", this->uid(), message->mails_size());

  this->last_server_mail_id_ = message->last_server_mail_id();
  MessageResponseGetMail response;
  int32_t count = message->mails_size();
  for (int32_t i = 0; i < count; ++i) {
    response.add_mails()->CopyFrom(message->mails(i));
  }
  response.set_read_mail_id(message->last_mail_id());

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_MAIL, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetRewardInfo(SSMessageEntry& entry) {
  MessageSSGetRewardInfo* message =
      static_cast<MessageSSGetRewardInfo*>(entry.get());
  this->pk_rank_reward_rank_ = message->pk_rank_reward_rank();
  this->pk_rank_reward_time_ = message->pk_rank_reward_time();
  this->last_pk_time_ = message->last_pk_time();
  this->pk_targets_.clear();
  for (int32_t i = 0; i < message->rank_info_size(); ++i) {
    int32_t rank = message->rank_info(i).rank();
    this->pk_targets_[rank] = server->GetUIDByRand(rank);
  }
  this->pk_max_rank_ = message->pk_rank_max();
  this->pk_max_rank_ = this->pk_max_rank_ <= 0 ? -1 : this->pk_max_rank_;
  this->pk_current_rank_ = server->GetRank(this->uid());
  this->daily_sign_.clear();
  for (int32_t i = 0; i < message->daily_sign_size(); ++i) {
    this->daily_sign_.push_back(message->daily_sign(i));
  }
  this->month_card_ = message->month_card();
  this->big_month_card_ = message->big_month_card();
  this->life_card_ = message->life_card();
  this->month_card_1_[0] = message->month_card_1();
  this->month_card_1_[1] = message->month_card_1_login();
  this->month_card_1_[2] = message->month_card_1_status();
  this->month_card_2_[0] = message->month_card_2();
  this->month_card_2_[1] = message->month_card_2_login();
  this->month_card_2_[2] = message->month_card_2_status();
  this->weekly_card_[0] = message->weekly_card();
  this->weekly_card_[1] = message->weekly_card_login();
  this->weekly_card_[2] = message->weekly_card_status();

  this->vip_weekly_.clear();
  for (int32_t i = 0; i < message->vip_weekly_size(); i++) {
    const KVPair2& data = message->vip_weekly(i);
    this->vip_weekly_[data.key()] = data.value();
  }
  return ERR_OK;
}

void LogicPlayer::SendMail(int64_t player_id, int64_t mail_time,
                           int32_t mail_type, const std::string& mail_content,
                           const std::vector<std::pair<int32_t,int32_t> >* reward) {
  MessageSSSendMail request;
  request.set_player_id(player_id);
  request.set_tid(server->GetTID());
  request.set_server_id(server_config->server_id());
  MailInfo* info = request.mutable_mail();
  info->set_mail_id(server->GetNewMailID());
  info->set_mail_time(mail_time);
  info->set_mail_type(mail_type);
  info->set_mail_content(mail_content);
  if (reward) {
    for (std::vector<std::pair<int32_t, int32_t> >::const_iterator it =
             reward->begin();
         it != reward->end(); ++it) {
      sy::KVPair2* pair = info->add_mail_attachment();
      pair->set_key(it->first);
      pair->set_value(it->second);
    }
  }
  server->SendServerMessageToDB(MSG_SS_SEND_MAIL, &request);
}

int32_t LogicPlayer::ProcessIgnorePlayerMessage(CSMessageEntry& entry) {
  return ERR_OK;
}

int32_t LogicPlayer::ProcessLoadPlayerBegin(CSMessageEntry& entry) {
  if (this->create_player()) {
    this->SendErrorCodeToClient(ERR_PLAYER_NEED_CREATE, MSG_CS_REQUEST_LOAD_PLAYER);
    return ERR_OK;
  }

  const int32_t msgid = entry.head.msgid;

  if (this->load_complete()) {
    static boost::shared_ptr<MessageSSResponseGetPlayerInfoEnd> kLoadEnd(new MessageSSResponseGetPlayerInfoEnd);
    SSMessageEntry e;
    e.message = kLoadEnd;
    kLoadEnd->set_msgid(msgid);
    return this->ProcessLoadPlayerEnd(e);
  }
  this->SendLoadPlayer(msgid);
  return ERR_OK;
}

sy::ActivitySweepStake* LogicPlayer::GetSweepStake(int32_t type) {
  for (int32_t index = 0; index < this->sweep_stake_.activity_size(); ++index) {
    sy::ActivitySweepStake* info = this->sweep_stake_.mutable_activity(index);
    if (info->type() == type) return info;
  }
  sy::ActivitySweepStake* info = this->sweep_stake_.add_activity();
  info->set_type(type);
  info->set_update_time(GetSeconds());
  return info;
}

int32_t LogicPlayer::ProcessRequestGetSweepStakeCountAward(
    CSMessageEntry& entry) {
  MessageRequestGetSweepStakeCountAward* message =
      static_cast<MessageRequestGetSweepStakeCountAward*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->type() < ACTIVITY_SWEEP_STAKE_EQUIP)
    return ERR_ACTIVITY_NOT_FOUND;
  if (message->type() >= sy::ActivitySweepStakeType_ARRAYSIZE)
    return ERR_ACTIVITY_NOT_FOUND;

  sy::ActivitySweepStake* info = this->GetSweepStake(message->type());
  int32_t next_count = GetActivityNextCount(message->type(), info->award_n_count());
  if (!next_count) return ERR_PARAM_INVALID;
  if (next_count > info->total_count()) return ERR_ACTIVITY_AWARD_COUNT;

  const ActivityCountBase* base =
      ACTIVITY_COUNT_BASE.GetEntryByID(message->type() * 1000 + next_count)
          .get();
  if (!base) return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_SWEEP_STAKE);
  AddSubItemSet item_set;

  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  CHECK_RET(this->UpdateCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_SWEEP_STAKE);
  info->set_award_n_count(next_count);
  this->UpdateSweepStake(info);

  MessageResponseGetSweepStakeCountAward response;
  response.set_type(message->type());
  sy::KVPair2* pair = response.mutable_award_2();
  pair->set_key(base->award.v1);
  pair->set_value(base->award.v2);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_SWEEP_STAKE_COUNT_AWARD, &response);
  return ERR_OK;
}

void LogicPlayer::UpdateSweepStake(sy::ActivitySweepStake* info) {
  server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypeSweepStake,
                            this->sweep_stake_.SerializeAsString());
  if (info) {
    MessageNotifyActivitySweepStake notify;
    notify.add_activity()->CopyFrom(*info);
    this->SendMessageToClient(MSG_CS_NOTIFY_ACTIVITY_SWEEP_STAKE, &notify);
  }
}

template <int32_t SWEEP_STAKE_TYPE>
struct SweepStakeTrait;

template <>
struct SweepStakeTrait<ACTIVITY_SWEEP_STAKE_CARRIER> {
  static int32_t acitivity_type() { return ACTIVITY_SWEEP_STAKE_CARRIER; }
  static int32_t loot_id() { return GetSettingValue(activity_carrier_1drop); }
  static int32_t free_count() { return COUNT_TYPE_ACTIVITY_CARRIER_COUNT; }
  static void UpdateRank(LogicPlayer* player, int32_t total_count, int32_t& new_rank) {
    new_rank = 0;
    RANK_LIST.OnSweepStakeCarrier(
        player, total_count * GetSettingValue(activity_carrier_1score));
    new_rank = RANK_LIST.GetByType(sy::RANK_TYPE_SWEEP_STAKE_CARRIER)
                   .GetRankByUID(player->uid());
  }
};

template <>
struct SweepStakeTrait<ACTIVITY_SWEEP_STAKE_EQUIP> {
  static int32_t acitivity_type() { return ACTIVITY_SWEEP_STAKE_EQUIP; }
  static int32_t loot_id() { return GetSettingValue(activity_equip_1drop); }
  static int32_t free_count() { return COUNT_TYPE_ACTIVITY_EQUIP_COUNT; }
  static void UpdateRank(LogicPlayer* player, int32_t total_count, int32_t& new_rank) {
    new_rank = 0;
  }
};

template <int32_t SWEEP_STAKE_TYPE>
static int32_t ProcessRequestSweepStake(LogicPlayer* player, int32_t count, uint16_t msgid) {
  typedef SweepStakeTrait<SWEEP_STAKE_TYPE> Trait;

  const ActivityInfoBasePtr& info_base = ACTIVITY_INFO_BASE.GetEntryByID(Trait::acitivity_type());
  if (!info_base) return ERR_INTERNAL;
  ValuePair2<int32_t, int32_t> param = info_base->IsTodayInActivty(0);
  if (!param.v1) return ERR_ACTIVITY_NOT_OPEN;

  const LootBasePtr& base =
      LootConfigFile::Get(Trait::loot_id(), player->level());
  if (!base) return ERR_PARAM_INVALID;
  count = count == 10 ? 10 : 1;
  bool is_free = false;
  if (count == 1 && !player->GetDailyCount(Trait::free_count()))
    is_free = true;

  sy::ActivitySweepStake* info = player->GetSweepStake(Trait::acitivity_type());
  if (!info) return ERR_INTERNAL;

  ModifyCurrency modify(msgid, SYSTEM_ID_SWEEP_STAKE);
  AddSubItemSet item_set;
  if (!is_free) {
    FillCurrencyAndItem<kSub>(
        modify, item_set, count == 10 ? info_base->cost10 : info_base->cost1);
  }
  std::vector<int32_t> loot_index;
  if (!info_base->item.v1) {
    for (int32_t i = 0; i < count; ++i) {
      base->Loot(modify, item_set, &loot_index);
    }
  } else {
    int32_t item_index = 0;
    if (count == 1) {
      if (IsSweepStakeCountLucky(player->uid(), info->total_count() + count)) {
        loot_index.push_back(item_index);
        item_set.push_back(ItemParam(info_base->item.v1, info_base->item.v2));
      } else {
        base->Loot(modify, item_set, &loot_index);
      }
    } else if (info->total_count() <= 2) {
      for (int32_t i = 0; i < count - 3; ++i) {
        base->Loot(modify, item_set, &loot_index);
      }
      loot_index.push_back(item_index);
      item_set.push_back(ItemParam(info_base->item.v1, info_base->item.v2));
      loot_index.push_back(item_index);
      item_set.push_back(ItemParam(info_base->item.v1, info_base->item.v2));
      loot_index.push_back(item_index);
      item_set.push_back(ItemParam(info_base->item.v1, info_base->item.v2));
    } else if (info->total_count() <= 7) {
      for (int32_t i = 0; i < count - 2; ++i) {
        base->Loot(modify, item_set, &loot_index);
      }
      loot_index.push_back(item_index);
      item_set.push_back(ItemParam(info_base->item.v1, info_base->item.v2));
      loot_index.push_back(item_index);
      item_set.push_back(ItemParam(info_base->item.v1, info_base->item.v2));
    } else {
      for (int32_t i = 0; i < count - 1; ++i) {
        base->Loot(modify, item_set, &loot_index);
      }
      loot_index.push_back(item_index);
      item_set.push_back(ItemParam(info_base->item.v1, info_base->item.v2));
    }
  }
  std::random_shuffle(loot_index.begin(), loot_index.end());

  CHECK_RET(player->CheckCurrency(modify));
  CHECK_RET(player->CheckItem(&item_set, NULL, NULL));

  if (is_free) {
    player->UpdateBuyCount(Trait::free_count(), 1);
  }

  info->set_total_count(info->total_count() + count);
  player->UpdateSweepStake(info);
  int32_t new_rank = 0;
  Trait::UpdateRank(player, info->total_count(), new_rank);

  player->UpdateCurrency(modify);
  player->ObtainItem(&item_set, NULL, NULL, msgid, SYSTEM_ID_SWEEP_STAKE);

  MessageResponseSweepStake response;
  response.set_type(Trait::acitivity_type());
  response.set_new_rank(new_rank);
  for (std::vector<int32_t>::const_iterator iter = loot_index.begin();
       iter != loot_index.end(); ++iter) {
    if (*iter >= (int32_t)base->info.size()) continue;
    sy::KVPair2* pair = response.add_award_1();
    pair->set_key(base->info[*iter].v1);
    pair->set_value(base->info[*iter].v2);
  }
  player->SendMessageToClient(MSG_CS_RESPONSE_SWEEP_STAKE, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSweepStakeCarrier(CSMessageEntry& entry) {
  MessageRequestSweepStakeCarrier* message =
      static_cast<MessageRequestSweepStakeCarrier*>(entry.get());
  if (!message) return ERR_INTERNAL;

  return ProcessRequestSweepStake<ACTIVITY_SWEEP_STAKE_CARRIER>(this, message->count(), entry.head.msgid);
}

int32_t LogicPlayer::ProcessRequestSweepStakeEquip(CSMessageEntry& entry) {
  MessageRequestSweepStakeEquip* message =
      static_cast<MessageRequestSweepStakeEquip*>(entry.get());
  if (!message) return ERR_INTERNAL;

  return ProcessRequestSweepStake<ACTIVITY_SWEEP_STAKE_EQUIP>(this, message->count(), entry.head.msgid);
}

int32_t LogicPlayer::ProcessRequestGetSweepStakeEquipAward(
    CSMessageEntry& entry) {
  MessageRequestGetSweepStakeEquipAward* message =
      static_cast<MessageRequestGetSweepStakeEquipAward*>(entry.get());
  if (!message) return ERR_INTERNAL;
  const ActivityEquipBase* base =
      ACTIVITY_EQUIP_BASE.GetEntryByID(message->award_index()).get();
  if (!base) return ERR_PARAM_INVALID;
  sy::ActivitySweepStake* info = this->GetSweepStake(sy::ACTIVITY_SWEEP_STAKE_EQUIP);

  if (info->total_count() <
      base->score / GetSettingValue(activity_equip_1score)) {
    return ERR_ACTIVITY_EQUIO_SCORE;
  }
  if (info->award_status() & (1 << base->id())) {
    return ERR_ACTIVITY_AWARDED;
  }

  //积分奖励
  info->set_award_status(info->award_status() | (1 << base->id()));
  this->UpdateSweepStake(info);

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_SWEEP_STAKE);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid,
                   SYSTEM_ID_SWEEP_STAKE);
  info->set_award_status(info->award_status() | (1 << base->id()));
  this->UpdateSweepStake(info);

  MessageResponseGetSweepStakeEquipAward response;
  response.set_award_index(message->award_index());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_SWEEP_STAKE_EQUIP_AWARD, &response);
  return ERR_OK;
}

void LogicPlayer::RefreshSweepStake() {
  MessageNotifyActivitySweepStake notify;
  const sy::ActivitySweepStake kEmpty;

  for (int32_t i = 1; i < sy::ActivitySweepStakeType_ARRAYSIZE; ++i) {
    const ActivityInfoBasePtr& base = ACTIVITY_INFO_BASE.GetEntryByID(i);
    if (!base) continue;
    ValuePair2<int32_t, int32_t> param = base->IsTodayInActivty(0);
    sy::ActivitySweepStake* info = this->GetSweepStake(i);
    if (!param.v1) continue;
    int32_t update_days =
        GetSecondsDiffDays(info->update_time(), server->server_start_time());
    update_days = abs(update_days) + 1;
    if (update_days >= param.v1 && update_days < param.v1 + param.v2) continue;

    *info = kEmpty;
    info->set_type(i);
    info->set_update_time(GetSeconds());
    notify.add_activity()->CopyFrom(*info);
  }

  if (notify.activity_size()) {
    server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypeSweepStake,
                              this->sweep_stake_.SerializeAsString());
    this->SendMessageToClient(MSG_CS_NOTIFY_ACTIVITY_SWEEP_STAKE, &notify);
  }
}

void LogicPlayer::SendSweepStakeData() {
  this->SendMessageToClient(MSG_CS_RESPONSE_ACTIVITY_SWEEP_STAKE,
                            &this->sweep_stake_);
}

void LogicPlayer::SendAllDataToClient() {
  //禁止登陆
  if (this->CheckBanToLogin()) {
    return;
  }

  this->RefreshTimeActivity();
  this->HeroComeBackLogin();
  this->max_fight_attr_ = std::max(this->max_fight_attr_, this->fight_attr_);
  this->player_.set_max_rank(this->pk_max_rank_ > 0 ? this->pk_max_rank_ : sy::MAX_ROBOT_ID);
  this->SendPlayerData();
  this->SendHeroData();
  this->SendCopyData();
  this->SendTacticData();
  this->SendItemData();
  this->SendShopData();
  this->SendSweepStakeData();
  server->SendNotices(*this);

  if (!achievements_[ACHIEVEMENT_TYPE_COUNT_JOIN_ARMY] && this->army_id_)
    UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_JOIN_ARMY, 1);

  this->SendMessageToClient(MSG_CS_NOTIFY_ELITE_RANDOM_COPY, &this->elite_random_copy_);
  this->SendMessageToClient(MSG_CS_RESPONSE_TIME_ACTIVITY, ACTIVITY.GetMessage());
  this->ProcessRequestGetFriendInfo(*(CSMessageEntry*)(NULL));
  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_PLAYER_END, NULL);

  this->SendFirstServerReward();
}

void LogicPlayer::SendFirstServerReward() {
  const FirstServerRewardAccount* config = GetFirstServerReward(this->account());
  if (!config) return;
  int32_t rewarded = this->achievements_[ACHIEVEMENT_TYPE_FIRST_SERVER_REWARD];
  if (rewarded) return;
  DEBUG_LOG(logger)("SendFirstServerReward, OpenID:%s, PlayerID:%ld, Money:%d"
      ", MonthCard:%d, LifeCard:%d, WeeklyCard:%d, MonthCard1:%d, MonthCard2:%d"
      , config->openid.c_str(), this->uid()
      , config->money, config->month_card
      , config->life_card, config->weekly_card, config->month_card_1
      , config->month_card_2);

  std::vector<std::pair<int32_t, int32_t> > reward;
  reward.push_back(std::pair<int32_t, int32_t>(sy::MONEY_KIND_MONEY, config->money * 20));
  reward.push_back(std::pair<int32_t, int32_t>(sy::MONEY_KIND_VIPEXP, config->money * 10));
  if (config->month_card) reward.push_back(std::pair<int32_t, int32_t>(config->month_card, 1));
  if (config->life_card) reward.push_back(std::pair<int32_t, int32_t>(config->life_card, 1));
  if (config->weekly_card) reward.push_back(std::pair<int32_t, int32_t>(config->weekly_card, 1));
  if (config->month_card_1) reward.push_back(std::pair<int32_t, int32_t>(config->month_card_1, 1));
  if (config->month_card_2) reward.push_back(std::pair<int32_t, int32_t>(config->month_card_2, 1));

  LogicPlayer::SendMail(this->uid(), GetSeconds(), MAIL_TYPE_FIRST_SERVER, "",
                        &reward);
  this->UpdateAchievement(ACHIEVEMENT_TYPE_FIRST_SERVER_REWARD, 1);
}

void LogicPlayer::SendMessageToSelf(uint16_t msgid,
                                    const boost::shared_ptr<Message>& msg) {
  CSHead head = {0, 0, 0};
  head.msgid = msgid;
  ::PushCSMessage(head, this->session_.lock(), msg);
}

void LogicPlayer::SendPlayerData() {
  MessageResponseLoadPlayer msg;
  msg.mutable_info()->CopyFrom(this->player());
  msg.set_server_id(server_config->server_id());
  msg.set_pk_rank_reward_rank(this->pk_rank_reward_rank_);
  msg.set_pk_rank_reward_time(this->pk_rank_reward_time_);
  for (VectorMap<int32_t, int32_t>::const_iterator iter =
           this->daily_counter_.begin();
       iter != this->daily_counter_.end(); ++iter) {
    sy::KVPair2* info = msg.add_buy_count();
    info->set_key(iter->first);
    info->set_value(iter->second);
  }
  msg.set_patrol_total_time(this->patrol_total_time_);
  for (VectorMap<int32_t, sy::PatrolInfo>::const_iterator iter =
           this->patrol_infos_.begin();
       iter != this->patrol_infos_.end(); ++iter) {
    sy::PatrolInfo* info = msg.add_patrol_info();
    info->CopyFrom(iter->second);
  }
  for (VectorMap<int32_t, std::deque<std::string> >::const_iterator iter =
           this->report_abstract_.begin();
       iter != this->report_abstract_.end(); ++iter) {
    for (std::deque<std::string>::const_iterator iter_content =
             iter->second.begin();
         iter_content != iter->second.end(); ++iter_content) {
      msg.add_report_abstract(*iter_content);
    }
  }
  msg.mutable_tower_state()->CopyFrom(this->tower_state_);
  for (VectorMap<int16_t, int16_t>::const_iterator iter =
           this->tower_buff_.begin();
       iter != this->tower_buff_.end(); ++iter) {
    sy::KVPair2* info = msg.add_tower_buff();
    info->set_key(iter->first);
    info->set_value(iter->second);
  }
  msg.mutable_dstrike_info()->CopyFrom(this->dstrike_info_);
  for (VectorMap<int32_t, int32_t>::const_iterator it = achievements_.begin();
       it != achievements_.end(); ++it) {
    KVPair2* pair = msg.add_achievements();
    pair->set_key(it->first);
    pair->set_value(it->second);
  }
  for (std::vector<sy::RechargeInfo>::iterator iter = this->recharge_.begin();
       iter != this->recharge_.end(); ++iter) {
    msg.add_recharge()->CopyFrom(*iter);
  }
  for (size_t i = 0; i < this->daily_sign_.size(); i++)
    msg.add_daily_sign(this->daily_sign_[i]);
  msg.set_month_card(this->month_card_);
  msg.set_big_month_card(this->big_month_card_);
  msg.set_life_card(this->life_card_);
  msg.set_weekly_card(this->weekly_card_[0]);
  msg.set_weekly_card_login(this->weekly_card_[1]);
  msg.set_weekly_card_status(this->weekly_card_[2]);
  msg.set_month_card_1(this->month_card_1_[0]);
  msg.set_month_card_1_login(this->month_card_1_[1]);
  msg.set_month_card_1_status(this->month_card_1_[2]);
  msg.set_month_card_2(this->month_card_2_[0]);
  msg.set_month_card_2_login(this->month_card_2_[1]);
  msg.set_month_card_2_status(this->month_card_2_[2]);

  for (VectorMap<int32_t, int32_t>::iterator it = this->vip_weekly_.begin();
       it != this->vip_weekly_.end(); ++it) {
    KVPair2* data = msg.add_vip_weekly();
    data->set_key(it->first);
    data->set_value(it->second);
  }
  for (size_t i = 0; i < this->army_skill_.size(); ++i) {
    msg.add_army_skill(this->army_skill_[i]);
  }

  for (VectorMap<std::pair<int32_t, int64_t>, sy::ActivityRecord>::iterator it =
           activity_record_new_.begin();
       it != activity_record_new_.end(); ++it) {
    if (ACTIVITY.GetActivityID((sy::TimeActivityType)it->first.first) ==
        it->first.second) {
      TRACE_LOG(logger)("PlayerGetActivityRecord player_id:%ld, record_id:%ld",  this->uid(), it->first.second);
      msg.add_activity_record()->CopyFrom(it->second);
    }
  }
  for (std::vector<uint32_t>::const_iterator iter =
           server_config->server_ids().begin();
       iter != server_config->server_ids().end(); ++iter) {
    msg.add_servers(*iter);
  }

  msg.set_army_id(this->army_id_);
  msg.set_army_leave_time(this->army_leave_time_);
  msg.set_user_defined(this->user_defined_);
  msg.mutable_cross_info()->CopyFrom(this->cross_server_info_);
  msg.set_max_fight_attr(this->max_fight_attr());
  msg.set_got_award_version(this->got_award_version_);
  msg.set_medal_copy_id(this->medal_copy_id_);
  msg.set_medal_star(this->medal_star_);
  msg.set_medal_state(this->medal_state_);
  msg.set_medal_achi(this->medal_achi_);

  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_PLAYER, &msg);
}

void LogicPlayer::SendHeroData() {
  MessageResponseLoadShip msg;
  for (std::vector<LogicHero>::iterator iter = this->ships_.begin();
       iter != this->ships_.end(); ++iter) {
    sy::HeroInfo* info = msg.add_ships();
    info->CopyFrom(iter->first);
  }
  for (std::vector<sy::CarrierInfo>::iterator iter = this->carriers_.begin();
       iter != this->carriers_.end(); ++iter) {
    sy::CarrierInfo* info = msg.add_carrier();
    info->CopyFrom(*iter);
  }
  msg.mutable_current_carrier()->CopyFrom(this->current_carrier_);
  msg.mutable_hero_research()->CopyFrom(this->hero_research_info_);
  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_SHIP, &msg);
}

void LogicPlayer::SendCopyData() {
  MessageResponseLoadCopy msg;
  for (std::vector<sy::CopyProgress>::const_iterator iter =
           this->copy_progress_.begin();
       iter != this->copy_progress_.end(); ++iter) {
    msg.add_progress()->CopyFrom(*iter);
  }
  for (VectorMap<int32_t, int32_t>::const_iterator iter =
           this->copy_star_.begin();
       iter != this->copy_star_.end(); ++iter) {
    sy::CopyStarInfo* info = msg.add_copy_star();
    info->set_copy_id(iter->first);
    info->set_star(iter->second);
  }
  for (VectorMap<int32_t, int32_t>::const_iterator iter =
           this->copy_count_.begin();
       iter != this->copy_count_.end(); ++iter) {
    sy::CopyCount* info = msg.add_copy_count();
    info->set_copy_id(iter->first);
    info->set_count(iter->second);
  }
  for (std::vector<int32_t>::const_iterator iter = this->passed_copy_.begin();
       iter != this->passed_copy_.end(); ++iter) {
    msg.add_passed_copy(*iter);
  }
  for (VectorMap<int32_t, int32_t>::const_iterator iter =
           this->chapter_award_.begin();
       iter != this->chapter_award_.end(); ++iter) {
    sy::ChapterAwardInfo* info = msg.add_chapter_award();
    info->set_chapter(iter->first);
    info->set_mask(iter->second);
  }
  for (std::vector<int32_t>::const_iterator iter = this->gate_award_.begin();
       iter != this->gate_award_.end(); ++iter) {
    msg.add_gate_award(*iter);
  }
  msg.mutable_carrier_copy_info()->CopyFrom(this->carrier_copy_info_);
  for (size_t i = 0; i < this->carrier_copy_.size(); ++i) {
    msg.add_carrier_copy()->CopyFrom(this->carrier_copy_[i]);
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_COPY, &msg);
}

void LogicPlayer::SendTacticData() {
  MessageResponseLoadTactic msg;
  msg.mutable_info()->CopyFrom(this->tactic_);
  for (VectorSet<int32_t>::iterator it = this->obtained_carriers_.begin();
       it != this->obtained_carriers_.end(); ++it) {
    msg.add_obtained_carriers(*it);
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_TACTIC, &msg);
}

void LogicPlayer::SendItemData() {
  MessageResponseLoadItem msg;
  for (ItemManager<LogicItem>::const_iterator iter = this->items().begin();
       iter != this->items().end(); ++iter) {
    msg.add_items()->CopyFrom(iter->second.data());
  }
  msg.mutable_equips()->CopyFrom(this->equips_);
  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_ITEM, &msg);
}

void LogicPlayer::SendShopData() {
  MessageResponseLoadShop msg;

  const std::vector<int32_t>& fresh_type =
      Setting::GetValue1(Setting::refresh_shop_type);
  for (size_t i = 0; i < fresh_type.size(); i++) {
    RefreshShopInfo& shop_info = refresh_shop_info_[fresh_type[i]];
    if (!shop_info.shop_id()) {
      shop_info.set_shop_id(fresh_type[i]);
      ShopBase::RandomFeatsCommodity(shop_info.shop_id(), this->level(),
                                     shop_info.mutable_feats_commodity(),
                                     server->GetServerStartDays());
      MessageSSUpdateShopInfo db_msg;
      db_msg.add_refresh_commodity()->CopyFrom(shop_info);
      this->SendMessageToDB(MSG_SS_UPDATE_SHOP_INFO, &db_msg);
    }
  }
  for (VectorMap<int32_t, sy::RefreshShopInfo>::iterator it =
           refresh_shop_info_.begin();
       it != refresh_shop_info_.end(); ++it) {
    if (it->first == 3) {
      msg.set_last_time(it->second.last_time());
      msg.set_used_count(it->second.used_count());
      msg.mutable_feats_commodity()->CopyFrom(it->second.feats_commodity());
    }
    msg.add_refresh_commodity()->CopyFrom(it->second);
  }

  for (VectorMap<int32_t, int32_t>::const_iterator iter =
           this->normal_shop_info_.begin();
       iter != this->normal_shop_info_.end(); ++iter) {
    sy::ShopCommodityInfo* info = msg.add_normal_commodity();
    info->set_commodity_id(iter->first);
    info->set_bought_count(iter->second);
  }
  for (VectorMap<int32_t, int32_t>::const_iterator iter =
           this->life_shop_info_.begin();
       iter != this->life_shop_info_.end(); ++iter) {
    sy::ShopCommodityInfo* info = msg.add_life_commodity();
    info->set_commodity_id(iter->first);
    info->set_bought_count(iter->second);
  }

  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_SHOP, &msg);
}

int32_t LogicPlayer::ProcessLoadPlayerEnd(SSMessageEntry& entry) {
  MessageSSResponseGetPlayerInfoEnd* message = static_cast<MessageSSResponseGetPlayerInfoEnd*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->LoadLocalStorage();
  this->status_ = 0;
  this->status_ = PLAYER_LOAD_COMPLETE;
  int32_t msgid = message->msgid();
  if (!msgid) return ERR_OK;

  switch (msgid) {
    case MSG_CS_REQUEST_LOAD_PLAYER: {
      this->OnPlayerLogIn();
      this->SendAllDataToClient();
    }
    break;
    case MSG_CS_REQUEST_GET_OTHER_PLAYER: {
      this->CalcTacticAttr(kNotifyNone, 0, 0);
      TRACE_LOG(logger)("LoadOtherPlayer:%ld", this->uid());
    } break;
    default: {
      ERROR_LOG(logger)("ProcessLoadPlayerEnd Unkown msgid:0x%04X", msgid);
    }
  }
  server->UpdateTactic(this);
  return ERR_OK;
}

void LogicPlayer::LoadLocalStorage() {
  this->LoadEliteRandomCopy();
  this->world_boss_info_.ParseFromString(
      server->GetPlayerValue(this->uid(), KVStorage::kKVTypeServerBoss));
  this->user_defined_ =
      server->GetPlayerValue(this->uid(), KVStorage::kKVTypeUserDefined);
  this->cross_server_info_.ParseFromString(
      server->GetPlayerValue(this->uid(), KVStorage::kKVTypeCrossServer));
  this->sweep_stake_.ParseFromString(
      server->GetPlayerValue(this->uid(), KVStorage::kKVTypeSweepStake));
  this->festival_shop_refresh_time_ = atol(
      server->GetPlayerValue(this->uid(), KVStorage::kKVTypeFestivalRefreshTime)
          .c_str());
  this->got_award_version_ =
      server->GetPlayerValue(this->uid(), KVStorage::kKVTypeGotVersion);
  this->come_back_info_.ParseFromString(
      server->GetPlayerValue(this->uid(), KVStorage::kKVTypePlayerComeBack));
}

int32_t LogicPlayer::ProcessRequestSetTacticInfo(CSMessageEntry& entry) {
  MessageRequestSetTactic *message = static_cast<MessageRequestSetTactic*>(entry.get());
  if (!message) return ERR_INTERNAL;

  //检测等级
  int32_t count = message->info().infos_size();
  if (count <= 0 || count > 6) return ERR_PARAM_INVALID;
  if (this->player().level() < Setting::kSeatCount[count]) return ERR_HERO_COUNT_OF;

  //阵型检测(相同的船不能上阵)
  VectorSet<int32_t> heros;
  for (int32_t i = 0; i < message->info().infos_size(); ++i) {
    const sy::PositionInfo& info = message->info().infos(i);
    if (info.position() < 1 || info.position() > 6) return ERR_POS_OUT_OF_BOUND;
    LogicHero *hero_info = this->GetHeroByUID(info.hero_uid());
    if (!hero_info) return ERR_HERO_NOT_FOUND;
    if (!heros.insert(hero_info->first.hero_id()).second) return ERR_HERO_SAME;
  }
  for (int32_t i = 0; i < message->info().support_pos_size(); ++i) {
    const sy::PositionInfo& info = message->info().support_pos(i);
    const std::vector<int32_t>& lv_condition =
        Setting::GetValue1(Setting::rescuearmy_open_lv);
    if (info.position() < 1 || info.position() > (int32_t)lv_condition.size())
      return ERR_POS_OUT_OF_BOUND;
    if (lv_condition[info.position() - 1] > this->level())
      return ERR_PARAM_INVALID;
    LogicHero* hero_info = this->GetHeroByUID(info.hero_uid());
    if (!hero_info) return ERR_HERO_NOT_FOUND;
    if (!heros.insert(hero_info->first.hero_id()).second) return ERR_HERO_SAME;
  }

  this->tactic_.CopyFrom(message->info());
  {
    int64_t info[7] = {0};
    for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
      info[this->tactic_.infos(i).position()] = this->tactic_.infos(i).hero_uid();
    }
    DEBUG_LOG(logger)("SetTactic, PlayerID:%ld, %ld,%ld,%ld,%ld,%ld,%ld", this->uid(),
        info[1], info[2], info[3], info[4], info[5], info[6]);
  }
  this->ReCalcRelation();
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_TACTIC, entry.head.msgid);

  MessageResponseSetTactic response;
  response.mutable_info()->CopyFrom(message->info());
  this->SendMessageToClient(MSG_CS_RESPONSE_SET_TACTIC, &response);

  MessageSSRequestUpdateTacticInfo request;
  request.mutable_infos()->CopyFrom(message->info().infos());
  request.mutable_battle_pos()->CopyFrom(message->info().battle_pos());
  request.mutable_support_pos()->CopyFrom(message->info().support_pos());
  this->SendMessageToDB(MSG_SS_UPDATE_TACTIC_INFO, &request);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestEquipPlaneNew(CSMessageEntry& entry) {
  MessageRequestEquipPlaneNew* message =
      static_cast<MessageRequestEquipPlaneNew*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t index = message->index();

  sy::CarrierInfo* carrier_info =
      this->GetCarrierByID(current_carrier_.carrier_id());
  if (!carrier_info) return ERR_CARRIER_NOT_FOUND;
  const CarrierBasePtr& carrier_base =
      CARRIER_BASE.GetEntryByID(current_carrier_.carrier_id());
  if (!carrier_base) return ERR_CARRIER_NOT_FOUND;

  int32_t carrier_max_box = 0;

  RankBase* rank_base = RANK_BASE.GetEntryByID(this->player_.rank_id()).get();
  if (rank_base)
    carrier_max_box = rank_base->carrier_box;
  else
    carrier_max_box = carrier_base->slot[0];

  if (index >= carrier_max_box ||
      index >= (int32_t)carrier_base->plane_type.size() || index < 0)
    return ERR_PARAM_INVALID;

  if (this->current_carrier_.plane_id_size() <= index)
    this->current_carrier_.mutable_plane_id()->Resize(index + 1, 0);

  AddSubItemSet item_set;

  int32_t plane_id = message->plane_id();
  int32_t change_count = 0;
  if (plane_id) {
    int64_t old_plane = current_carrier_.plane_id(index);
    if (old_plane != plane_id) {
      if (old_plane) item_set.push_back(ItemParam(old_plane, 1));

      const CarrierPlaneBase* plane_base =
          CARRIER_PLANE_BASE.GetEntryByID(message->plane_id()).get();
      if (!plane_base) return ERR_PLANE_NOT_FOUND;

      if (plane_base->type != carrier_base->plane_type[index])
        return ERR_PLANE_TOO_MANY;

      change_count = -1;
    }
  } else {
    plane_id = current_carrier_.plane_id(index);
    if (!plane_id) return ERR_PARAM_INVALID;
    change_count = 1;
  }

  //设置当前航母,并计算属性
  if (change_count) item_set.push_back(ItemParam(plane_id, change_count));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_EQUIP_PLANE_NEW,
             SYSTEM_ID_BASE);
  this->current_carrier_.set_plane_id(index, message->plane_id());

  DEBUG_LOG(logger)("PlayerID:%ld, CarrierPlaneCount:%d", this->uid(), this->current_carrier_.plane_id_size());

  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_CARRIER_BUILD, entry.head.msgid);
  MessageResponseEquipPlaneNew response;
  response.set_plane_id(message->plane_id());
  response.set_index(message->index());
  this->SendMessageToClient(MSG_CS_RESPONSE_EQUIP_PLANE_NEW, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPlaneLevelUpNew(CSMessageEntry& entry) {
  MessageRequestPlaneLevelUpNew* message =
      static_cast<MessageRequestPlaneLevelUpNew*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t index = message->index();
  int32_t plane_id = 0;

  if(index >= this->current_carrier_.plane_id_size())
    return ERR_PARAM_INVALID;

  if (index >= 0)
    plane_id = this->current_carrier_.plane_id(index);
  else
    plane_id = message->plane_id();

  if (!plane_id) return ERR_PARAM_INVALID;

  const CarrierPlaneBase* plane_base =
      CARRIER_PLANE_BASE.GetEntryByID(plane_id).get();
  if (!plane_base) return ERR_PLANE_NOT_FOUND;

  const CarrierPlaneBase* next_plane_base =
      CARRIER_PLANE_BASE.GetEntryByID(plane_base->next_id).get();
  if (!next_plane_base) return ERR_ITEM_LEVEL_MAX;

  ModifyCurrency modify(MSG_CS_REQUEST_PLANE_LEVEL_UP_NEW,
                        SYSTEM_ID_PLANE_SYNTHESIS);
  AddSubItemSet item_set;

  int32_t do_count = 1;

  if (index >= 0) {
    FillCurrencyAndItem<kSub>(modify, item_set, next_plane_base->cost);
    for (size_t i = 0; i < item_set.size(); i++) {
      if (item_set[i].item_id == plane_base->id()) {
        item_set[i].item_count += 1;
        break;
      }
    }
  } else {
    if (message->is_all()) {
      do_count = 99999;
      int temp = 0;

      for (ValuePair2Vec::const_iterator it = next_plane_base->cost.begin();
           it != next_plane_base->cost.end(); ++it) {
        if (it->v1 < 100)
          temp = this->GetCurrency(it->v1) / it->v2;
        else
          temp = this->GetItemCountByItemID(it->v1) / it->v2;
        do_count = do_count < temp ? do_count : temp;
      }

      if (do_count < 1) return ERR_ITEM_NEED_MORE;
    }

    for (int32_t i = 0; i < do_count; i++)
      FillCurrencyAndItem<kSub>(modify, item_set, next_plane_base->cost);
    item_set.push_back(ItemParam(plane_base->next_id, do_count));
  }

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_PLANE_LEVEL_UP_NEW,
             SYSTEM_ID_PLANE_SYNTHESIS);

  if (index >= 0) {
    this->current_carrier_.set_plane_id(index, plane_base->next_id);
    this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_PLANE_SYNTHESIS,
                         entry.head.msgid);
  }

  MessageResponsePlaneLevelUpNew response;
  response.set_count(do_count);
  response.set_plane_id(plane_base->id());
  response.set_index(index);
  this->SendMessageToClient(MSG_CS_RESPONSE_PLANE_LEVEL_UP_NEW, &response);

  return ERR_OK;
}

//oil_recover_time
//energy_recover_time
void LogicPlayer::UpdateOilInfo(bool force, int32_t sys_id, int32_t msgid) {
  int32_t oil_time = GetSeconds() - this->player_.last_oil_time();
  int32_t energy_time = GetSeconds() - this->player_.last_energy_time();

  int32_t add_oil = oil_time > Setting::kRecoverOilTime ? oil_time/ Setting::kRecoverOilTime : 0;
  int32_t add_energy = energy_time > Setting::kRecoverEnergyTime ? energy_time / Setting::kRecoverEnergyTime : 0;

  int32_t current_oil = this->player_.oil();
  int32_t current_energy = this->player_.energy();

  if (current_oil > Setting::kOilRecoverMax) {
    current_oil = std::min(current_oil, Setting::kOilMax);
  } else {
    current_oil = std::min(current_oil + add_oil, Setting::kOilRecoverMax);
  }

  if (current_energy > Setting::kEnergyRecoverMax) {
    current_energy = std::min(current_energy, Setting::kEnergyMax);
  } else {
    current_energy = std::min(current_energy + add_energy, Setting::kEnergyRecoverMax);
  }

  if ((add_oil + add_energy) || force) {
    if (add_oil) this->player_.set_last_oil_time(GetSeconds());
    if (add_energy) this->player_.set_last_energy_time(GetSeconds());

    this->player_.set_oil(current_oil);
    this->player_.set_energy(current_energy);

    MessageNotifyOilInfo message;
    message.set_oil(this->player_.oil());
    message.set_last_oil_time(this->player_.last_oil_time());
    message.set_energy(this->player_.energy());
    message.set_last_energy_time(this->player_.last_energy_time());
    message.set_sysid(sys_id);
    message.set_msgid(msgid);
    this->SendMessageToClient(MSG_CS_NOTIFY_OIL_INFO, &message);

    MessageSSUpdateOilInfo msg;
    msg.set_tid(server->GetTID());
    msg.set_msgid(msgid);
    msg.set_system(sys_id);
    msg.set_oil(this->player_.oil());
    msg.set_last_oil_time(this->player_.last_oil_time());
    msg.set_energy(this->player_.energy());
    msg.set_last_energy_time(this->player_.last_energy_time());
    this->SendMessageToDB(MSG_SS_UPDATE_OIL_INFO, &msg);
  }
}

void LogicPlayer::Update() {
  if (!this->load_complete() || this->last_update_time_ == GetVirtualSeconds()) return;
  if (!this->last_update_time_) this->last_update_time_ = GetVirtualSeconds();
  this->CheckBanToLogin();
  this->online_time_ += GetVirtualSeconds() - this->last_update_time_;
  this->last_update_time_ = GetVirtualSeconds();
  this->UpdateOilInfo(false, SYSTEM_ID_TIME, 0);
  this->UpdateDstrikeInfo();
  this->RefreshEliteRandomCopy();

  int32_t day_diff =
      GetSecondsDiffDays(this->fresh_time_ - Setting::kRefreshSeconds,
                         GetVirtualSeconds() - Setting::kRefreshSeconds);
  //重置各个系统
  if (day_diff) {
    TRACE_LOG(logger)("PlayerID:%ld, RefreshAllSystem:%d, CreateDays:%d"
        , this->uid(), day_diff, this->create_days());
    this->RefreshAllSystem();
    this->RefreshWeeklySystem(day_diff);
  }
}

int32_t LogicPlayer::GetItemCountByItemID(int32_t item_id) {
  LogicItem* item = this->items_.GetItemByItemID(item_id);
  return item ? item->count() : 0;
}

void LogicPlayer::AddItemCount(int32_t item_id, int32_t count,int32_t msg_id,int32_t sys_id) {
  AddSubItemSet set;
  set.push_back(ItemParam(item_id, count));
  int32_t result = this->ObtainItem(&set, NULL, NULL, msg_id, sys_id);
  if (result)
    ERROR_LOG(logger)("PlayerID:%ld, AddItemCount Fail, ItemID:%d, Count:%d"
        , this->uid(), item_id, count);
}

void LogicPlayer::UpdateDstrikeInfo(bool force) {
  int32_t last_time = this->dstrike_info_.update_time();
  int32_t add = (GetSeconds() - last_time) / Setting::kDstrikeTokenTime;
  int32_t change = add + force;

  int32_t current_count = this->GetItemCountByItemID(Setting::kDstrikeTokenID);

  do {
    if (current_count < Setting::kDstrikeTokenNum && !add) break;
    if (current_count >= Setting::kDstrikeTokenNum &&
        GetSeconds() - last_time < 120)
      break;
    if (current_count < Setting::kDstrikeTokenNum) {
      //增加道具
      int32_t real_add =
          std::min(Setting::kDstrikeTokenNum - current_count, add);
      this->AddItemCount(Setting::kDstrikeTokenID, real_add, 0,
                         SYSTEM_ID_DSTRIKE_MAIN);
    }
  } while (false);

  if (add) last_time += add * Setting::kDstrikeTokenTime;
  if (add + current_count >= Setting::kDstrikeTokenNum)
    last_time = GetSeconds();
  this->dstrike_info_.set_update_time(last_time);

  if (change) {
    MessageNotifyDstrikeInfo notify;
    notify.mutable_info()->CopyFrom(this->dstrike_info_);
    this->SendMessageToClient(MSG_CS_NOTIFY_DSTRIKE_INFO, &notify);

    MessageSSUpdateDstrikeInfo update;
    update.mutable_info()->CopyFrom(this->dstrike_info_);
    this->SendMessageToDB(MSG_SS_UPDATE_DSTRIKE_INFO, &update);
  }
}

struct RedEquipReset {
  LogicPlayer* player_;
  RedEquipReset(LogicPlayer* player) : player_(player) {}
  void operator()(const LogicItem& item) {
    if (!player_) return;
    const EquipBase* base = item.equip_base();
    if (!base || base->quality != QUALITY_RED) return;
    if (!item.GetAttr(ITEM_ATTR_STAR_LUCKY)) return;
    each_items_.push_back(const_cast<LogicItem*>(&item));
  }
  void Reset() {
    NotifyItemSet notify_set;
    for (size_t i = 0; i < each_items_.size(); ++i) {
      each_items_[i]->SetAttr(ITEM_ATTR_STAR_LUCKY, 0);
      notify_set.push_back(each_items_[i]->uid());
    }
    player_->ObtainItem(NULL, NULL, &notify_set, SYSTEM_ID_RED_EQUIP_STAR,
                        SYSTEM_ID_RED_EQUIP_STAR);
    each_items_.clear();
  }
  std::vector<LogicItem*> each_items_;
};

void LogicPlayer::RefreshAllSystem() {
  this->fresh_time_ = GetVirtualSeconds();

  //重置各个系统

  //重置副本次数
  MessageNotifyCopyInfo notify_copy;
  MessageSSUpdateCopyInfo update_copy;
  for (VectorMap<int32_t, int32_t>::iterator iter = this->copy_count_.begin();
       iter != this->copy_count_.end(); ++iter) {
    iter->second = 0;
    sy::CopyCount* info = notify_copy.add_copy_count();
    info->set_count(iter->second);
    info->set_copy_id(iter->first);
    update_copy.add_copy_count()->CopyFrom(*info);
  }
  this->SendMessageToClient(MSG_CS_NOTIFY_COPY_INFO, &notify_copy);
  this->SendMessageToDB(MSG_SS_UPDATE_COPY_INFO, &update_copy);

  //重置商店物品
  MessageNotifyShopInfo notify_shop;
  MessageSSUpdateShopInfo update_shop;
  for (VectorMap<int32_t, sy::RefreshShopInfo>::iterator iter =
           this->refresh_shop_info_.begin();
       iter != this->refresh_shop_info_.end(); ++iter) {
    iter->second.set_used_count(0);
    if (iter->first == 3) {
      notify_shop.set_last_time(iter->second.last_time());
      notify_shop.set_used_count(iter->second.used_count());
      notify_shop.mutable_feats_commodity()->CopyFrom(
          iter->second.feats_commodity());
    }
    notify_shop.add_refresh_commodity()->CopyFrom(iter->second);
    update_shop.add_refresh_commodity()->CopyFrom(iter->second);
  }
  for (VectorMap<int32_t, int32_t>::iterator iter =
           this->normal_shop_info_.begin();
       iter != this->normal_shop_info_.end(); ++iter) {
    iter->second = 0;
    sy::ShopCommodityInfo* info = notify_shop.add_normal_commodity();
    info->set_commodity_id(iter->first);
    info->set_bought_count(iter->second);
    update_shop.add_normal_commodity()->CopyFrom(*info);
  }
  this->SendMessageToClient(MSG_CS_NOTIFY_SHOP_INFO, &notify_shop);
  this->SendMessageToDB(MSG_SS_UPDATE_SHOP_INFO, &update_shop);

  //更新登录天数
  this->player_.set_login_days(this->player_.login_days() + 1);
  MessageNotifyLoginDays notify_login_days;
  notify_login_days.set_login_days(this->player_.login_days());
  this->SendMessageToClient(MSG_CS_NOTIFY_LOGIN_DAYS, &notify_login_days);
  MessageSSUpdateLoginDays update_login_days;
  update_login_days.set_login_days(this->player_.login_days());
  this->SendMessageToDB(MSG_SS_UPDATE_LOGIN_DAYS, &update_login_days);

  this->dstrike_info_.set_merit(0);
  this->dstrike_info_.set_damage(0);
  this->dstrike_info_.set_daily_award(0);
  this->UpdateDstrikeInfo(true);

  //重置购买次数
  MessageNotifyBuyCount notify_buy_count;
  MessageSSUpdateBuyCount update_buy_count;
  update_buy_count.set_is_clear(true);
  for (VectorMap<int32_t, int32_t>::iterator iter = this->daily_counter_.begin();
       iter != this->daily_counter_.end(); ++iter) {
    iter->second = 0;
    sy::KVPair2* info = notify_buy_count.add_infos();
    info->set_key(iter->first);
    info->set_value(iter->second);
  }
  this->SendMessageToDB(MSG_SS_UPDATE_BUY_COUNT, &update_buy_count);
  this->SendMessageToClient(MSG_CS_NOTIFY_BUY_COUNT, &notify_buy_count);

  //重置抽船
  this->hero_research_info_.set_day_free_rd_count(0);
  UpdateHeroResearchInfo();

  //天命经验重置
  std::vector<HeroInfo> info;
  for (std::vector<LogicHero>::iterator iter = this->ships_.begin();
       iter != this->ships_.end(); ++iter) {
    if (iter->first.fate_exp()) {
      iter->first.set_fate_exp(0);
      info.push_back(iter->first);
    }
  }
  UpdateHeroInfo(info, SYSTEM_ID_BASE, 0);

  //重置日常标记
  this->ResetDailySign();

  //重置跨服积分战胜场
  this->cross_server_info_.set_win_count(0);
  this->cross_server_info_.set_win_get_award(0);
  server->SetPlayerKeyValue(this->uid(), kKVTypeCrossServer,
                            cross_server_info_.SerializeAsString());
  MessageNotifyCrossServer cross_server_notify;
  cross_server_notify.mutable_info()->CopyFrom(this->cross_server_info_);
  this->SendMessageToClient(MSG_CS_NOTIFY_CROSS_SERVER, &cross_server_notify);

  //重置限时特惠
  int32_t ac_id = ACTIVITY.GetActivityID(TIME_ACTIVITY_RECHARGE);
  if (ac_id) {
    sy::ActivityRecord* act_record =
        this->GetActivityRecordX(TIME_ACTIVITY_RECHARGE, ac_id);
    if (act_record) {
      act_record->clear_record();
      act_record->set_refresh_time(GetVirtualSeconds());
      UpdateActivityRecordNew(TIME_ACTIVITY_RECHARGE, ac_id);
    }
  }

  //周卡, 小月基金, 大月基金增加登录天数
  int32_t month_card_change = 0;
  for (int32_t type = 0; type < 3; ++type) {
    int32_t valid_days = 0;
    int32_t buy_count = 0;
    int32_t* card = this->GetWeeklyCardByType(type, valid_days, buy_count);
    if (!card || !card[0]) continue;
    if (GetSecondsDiffDays(card[0], GetSeconds()) + 1 <= valid_days) {
      card[1]++;
      if (card[1] >= valid_days) card[1] = valid_days;
      month_card_change++;
    }
  }
  if (month_card_change) this->UpdateMonthCard(true);

  this->SendMessageToClient(MSG_CS_RESPONSE_TIME_ACTIVITY,
                            ACTIVITY.GetMessage());
  for (int32_t type = TimeActivityType_MIN; type < TimeActivityType_ARRAYSIZE;
       ++type) {
    int64_t id = ACTIVITY.GetActivityID((TimeActivityType)type);
    if (!id) continue;
    sy::ActivityRecord* act_record =
        this->GetActivityRecordX((TimeActivityType)type, id);
    if (act_record) {
      MessageNotifyActivityRecord notify;
      notify.mutable_records()->CopyFrom(*act_record);
      this->SendMessageToClient(MSG_CS_NOTIFY_ACTIVITY_RECORD, &notify);
    } else {
      sy::ActivityRecord act;
      act.set_type(type);
      act.set_id(id);
      MessageNotifyActivityRecord notify;
      notify.mutable_records()->CopyFrom(act);
      this->SendMessageToClient(MSG_CS_NOTIFY_ACTIVITY_RECORD, &notify);
    }
  }
  this->RefreshTimeActivity();
  //这边刷新转盘活动的数据
  this->RefreshSweepStake();

  this->RefreshFestivalShop();

  RedEquipReset red_equip_reset(this);
  this->items_.ForEach(red_equip_reset);
  red_equip_reset.Reset();

  this->HeroComeBackLogin();

  //!!!不要在下面几行代码下面添加代码!!!
  //重新设置刷新时间
  MessageSSUpdateFreshTime update_fresh_time;
  update_fresh_time.set_fresh_time(this->fresh_time_);
  this->SendMessageToDB(MSG_SS_UPDATE_FRESH_TIME, &update_fresh_time);
  //!!!不要在下面几行代码下面添加代码!!!
}

void LogicPlayer::OnLevelChanged(int32_t from_level, int32_t dest_level) {
  ModifyCurrency money(0, SYSTEM_ID_BASE);
  for (int32_t i = from_level; i < dest_level; ++i) {
    const ExpBasePtr& base = EXP_BASE.GetEntryByID(i);
    if (base) {
      money.oil += base->add_power;
      money.energy += base->add_energy;
    }
  }
  if (this->player_.oil() + money.oil >= Setting::kOilMax) {
    money.oil = Setting::kOilMax - this->player_.oil();
  }
  if (this->player_.energy() + money.energy >= Setting::kEnergyMax) {
    money.energy = Setting::kEnergyMax - this->player_.energy();
  }

  INFO_LOG(logger)("PlayerID:%ld, FromLevel:%d, DestLevel:%d, Oil:%d, Energy:%d"
      , this->uid(), from_level, dest_level, money.oil, money.energy);
  UpdateAchievement(SEVEN_DAY_TYPE_LEVEL, dest_level);
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_BASE, 0);
  this->UpdateCurrency(money);
  RANK_LIST.OnPlayerLevelUp(this);
}

void LogicPlayer::OnVipLevelChanged(int32_t from_level, int32_t dest_level) {
  this->SetDailySign(VIP_DAILY_AWARD, false);

  //签到补奖励
  if (IsSameDay(GetSeconds(), this->player_.sign_time())) {
    SignBase* base = SIGN_BASE.GetEntryByID(this->player_.sign_id()).get();
    if (!base) return;

    if (dest_level >= base->vipaward_lv && from_level < base->vipaward_lv) {
      ModifyCurrency modify(0, SYSTEM_ID_VIP);
      AddSubItemSet item_set;
      FillCurrencyAndItem<kAdd>(modify, item_set, base->award);
      if (this->CheckCurrency(modify)) return;
      if (this->CheckItem(&item_set, NULL, NULL)) return;
      this->UpdateCurrency(modify);
      this->ObtainItem(&item_set, NULL, NULL, 0, SYSTEM_ID_VIP);
    }
  }
}

void LogicPlayer::OnExpChanged() {
  if (!this->load_complete()) {
    ERROR_LOG(logger)("%s, Player:%ld not loaded", __PRETTY_FUNCTION__, this->uid());
    return;
  }

  if (this->player_.level() <= 0) this->player_.set_level(1);
  //达到最高等级,就不再增加经验
  if (this->player_.level() >= Setting::kMaxLevel) {
    this->player_.set_level(Setting::kMaxLevel);
    this->player_.set_exp(0);
    return;
  }

  int64_t current_exp = this->player_.exp();
  int32_t current_level = this->player_.level();
  int32_t level_changed = 0;
  while (true) {
    const ExpBasePtr& base = EXP_BASE.GetEntryByID(current_level);
    //经验不够
    if (!base || base->role_exp <= 0 || current_exp < base->role_exp) {
      break;
    }
    current_exp -= base->role_exp;
    current_level += 1;
    level_changed += 1;
  }
  this->player_.set_level(current_level);
  this->player_.set_exp(current_exp);

  if (level_changed) this->OnLevelChanged(current_level - level_changed, current_level);
}

void LogicPlayer::OnVipExpChanged() {
  int32_t orgin_level = this->player_.vip_level();
  int32_t current_exp = this->player_.vip_exp();
  int32_t current_level = this->player_.vip_level();
  bool vip_level_changed = false;
  while (true) {
    const VipExpBasePtr& base = VIP_EXP_BASE.GetEntryByID(current_level);
    if (!base) return;
    if (base->vip_exp <= 0 || current_exp < base->vip_exp) {
      break;
    }
    current_exp -= base->vip_exp;
    current_level += 1;
    vip_level_changed = true;
  }
  this->player_.set_vip_exp(current_exp);
  this->player_.set_vip_level(current_level);

  if (vip_level_changed) this->OnVipLevelChanged(orgin_level, current_level);
}

//计算航母飞机提供的属性
void LogicPlayer::CalcCarrier(int32_t notify) {
  const CarrierBasePtr& carrier_base = CARRIER_BASE.GetEntryByID(this->carrier_id());
  if (!carrier_base) return;

  Array<int64_t, AttackAttr_ARRAYSIZE> carrier_attr;
  carrier_attr.resize(AttackAttr_ARRAYSIZE, 0);

  Array<int64_t, AttackAttr_ARRAYSIZE> ship_attr;
  ship_attr.resize(AttackAttr_ARRAYSIZE, 0);

  //航母基础属性
  CarrierInfo* carrier_info = this->GetCarrierByID(this->carrier_id());
  if (!carrier_info) {
    this->current_carrier_.set_carrier_id(0);
    return;
  }
  int32_t group_id = carrier_base->breakadvancedid;
  int32_t level = carrier_info->level() + 1;
  int32_t grade = carrier_info->reform_level();
  BreakAdvAttr attr =
      BreakAdvancedBase::GetBreakAdvAttr(group_id, grade, level);
  carrier_attr[ATTACK_ATTR_HP] += attr[ATTACK_ATTR_HP];
  carrier_attr[ATTACK_ATTR_AP] += attr[ATTACK_ATTR_AP];
  carrier_attr[ATTACK_ATTR_SP] += attr[ATTACK_ATTR_SP];
  carrier_attr[ATTACK_ATTR_WF] += attr[ATTACK_ATTR_WF];
  carrier_attr[ATTACK_ATTR_FF] += attr[ATTACK_ATTR_FF];

  this->current_carrier_.set_grade(grade);

  for (const int64_t* iter = this->current_carrier_.plane_id().begin();
       iter != this->current_carrier_.plane_id().end(); ++iter) {
    if (!*iter) continue;
    const CarrierPlaneBase* base = CARRIER_PLANE_BASE.GetEntryByID(*iter).get();
    if (!base) continue;
    //增加航母的属性
    AddConfigAttr<100>(carrier_attr, base->extra_carrier);
    //增加船的属性
    AddConfigAttr<100>(ship_attr, base->extra_ship);
  }

  //增加船的属性
  this->current_carrier_.mutable_extra_damage1()->Resize(AttackAttr_ARRAYSIZE, 0);
  for (size_t i = 0; i < ship_attr.size(); ++i) {
    this->current_carrier_.mutable_extra_damage1()->Set(i, ship_attr[i]);
  }

  int32_t carrier_hit = carrier_base->hit;
  int32_t carrier_crit = carrier_base->crit;
  carrier_attr[ATTACK_ATTR_HIT_PERCENT] += carrier_hit;
  carrier_attr[ATTACK_ATTR_CRIT_PERCENT] += carrier_crit;

  //计算百分比增加
  std::pair<int32_t, int32_t> percent[] = {
    std::make_pair(ATTACK_ATTR_HP, ATTACK_ATTR_HP_PERCENT),
    std::make_pair(ATTACK_ATTR_AP, ATTACK_ATTR_AP_PERCENT),
    std::make_pair(ATTACK_ATTR_SP, ATTACK_ATTR_SP_PERCENT),
    std::make_pair(ATTACK_ATTR_WF, ATTACK_ATTR_WF_PERCENT),
    std::make_pair(ATTACK_ATTR_FF, ATTACK_ATTR_FF_PERCENT),
  };
  for (int32_t i = 0; i < ArraySize(percent); ++i) {
    int32_t old_value = carrier_attr[percent[i].first];
    int32_t new_value = old_value * (carrier_attr[percent[i].second] + 1000) / 1000;
    carrier_attr[percent[i].first] = new_value;
  }

  this->current_carrier_.mutable_attr1()->Resize(AttackAttr_ARRAYSIZE, 0);
  for (size_t i = 0; i < carrier_attr.size(); ++i) {
    this->current_carrier_.mutable_attr1()->Set(i, carrier_attr[i]);
  }

  int64_t score = CalcFightScore(this->current_carrier_.attr1(), true);
  this->current_carrier_.mutable_attr1()->Set(0, score);

  if (notify & kNotifyClient) {
    MessageNotifyCurrentCarrierInfo msg;
    msg.mutable_info()->CopyFrom(this->current_carrier_);
    this->SendMessageToClient(MSG_CS_NOTIFY_CURRENT_CARRIER_INFO, &msg);
  }
  if (notify & kNotifyServer) {
    MessageSSUpdateCurrentCarrierInfo msg;
    msg.mutable_info()->CopyFrom(this->current_carrier_);
    this->SendMessageToDB(MSG_SS_UPDATE_CURRENT_CARRIER_INFO, &msg);
  }
}

void LogicPlayer::CalcTacticAttr(int32_t notify, int32_t sys_id, int32_t msg_id) {
  TacticHeroSet hero_set;
  for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
    int64_t uid = this->tactic_.infos(i).hero_uid();
    LogicHero* hero = this->GetHeroByUID(uid);
    if (hero) hero_set.push_back(hero);
  }
  this->fight_attr_ = 0;
  this->CalcCarrier(notify);
  //清空属性
  for (size_t i = 0; i < hero_set.size(); ++i) {
    LogicHero* hero = hero_set[i];
    if (hero) {
      hero->first.mutable_attr1()->Resize(0, 0);
      hero->first.mutable_attr1()->Resize(AttackAttr_ARRAYSIZE, 0);
    }
  }

  for (size_t i = 0; i < hero_set.size(); ++i) {
    LogicHero* hero = hero_set[i];
    if (hero) this->CalcHeroAttr(hero, &hero_set, kNotifyNone);
  }

  for (size_t i = 0; i < hero_set.size(); ++i) {
    LogicHero *hero = hero_set[i];
    if (hero) {
      this->CalcHeroPercent(&hero->first);
      this->fight_attr_ += hero->first.attr1(0);
    }
  }

  if (this->fight_attr_ / 10000 > this->achievements_[SEVEN_DAY_TYPE_FIGHTING])
    UpdateAchievement(SEVEN_DAY_TYPE_FIGHTING, this->fight_attr_ / 10000);

  if (notify & kNotifyClient) {
    MessageNotifyHeroInfo msg;
    for (size_t i = 0; i < hero_set.size(); ++i) {
      msg.add_info()->CopyFrom(hero_set[i]->first);
    }
    this->SendMessageToClient(MSG_CS_NOTIFY_HERO_INFO, &msg);
    RANK_LIST.OnPlayerFightingUp(this);
  }
  if (notify & kNotifyServer) {
    MessageSSUpdateHeroInfo msg;
    for (size_t i = 0; i < hero_set.size(); ++i) {
      msg.add_info()->CopyFrom(hero_set[i]->first);
    }
    msg.set_tid(server->GetTID());
    msg.set_system(sys_id);
    msg.set_msgid(msg_id);
    this->SendMessageToDB(MSG_SS_UPDATE_HERO_INFO, &msg);
  }
  if (this->fight_attr_ > this->max_fight_attr_) {
    this->max_fight_attr_ = this->fight_attr_;
    MessageSSUpdateMaxFightAttr msg;
    msg.set_attr(this->max_fight_attr_);
    this->SendMessageToDB(MSG_SS_UPDATE_MAX_FIGHT_ATTR, &msg);

    MessageNotifyMaxFightAttr notify;
    notify.set_fight_attr(this->max_fight_attr_);
    this->SendMessageToClient(MSG_CS_NOTIFY_MAX_FIGHT_ATTR, &notify);
  }
  if (notify & kNotifyClient) {
    bool ship_vacancy = false;
    int32_t max_fate_level = 0;
    int32_t min_fate_level = 999999;
    int32_t min_wake_level = 999999;
    for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
      LogicHero* ship = this->GetHeroByUID(this->tactic_.infos(i).hero_uid());
      if (ship) {
        max_fate_level = max_fate_level > ship->first.fate_level()
                             ? max_fate_level
                             : ship->first.fate_level();
        min_fate_level = min_fate_level < ship->first.fate_level()
                             ? min_fate_level
                             : ship->first.fate_level();
        min_wake_level = min_wake_level < ship->first.wake_level()
                             ? min_wake_level
                             : ship->first.wake_level();
      } else {
        ship_vacancy = true;
      }
    }
    if (this->tactic_.infos_size() < 6) ship_vacancy = true;

    if (max_fate_level > this->achievements_[SEVEN_DAY_TYPE_MAX_SHIP])
      UpdateAchievement(SEVEN_DAY_TYPE_MAX_SHIP, max_fate_level);
    ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_SHIP_FATE_MAX_LEVEL,
                                    max_fate_level, this);

    if (!ship_vacancy) {
      if (min_fate_level > this->achievements_[SEVEN_DAY_TYPE_ALL_SHIP])
        UpdateAchievement(SEVEN_DAY_TYPE_ALL_SHIP, min_fate_level);
      if (min_wake_level > this->achievements_[ACHIEVEMENT_TYPE_MIN_WAKE_LEVEL])
        UpdateAchievement(ACHIEVEMENT_TYPE_MIN_WAKE_LEVEL, min_wake_level);
      ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_SHIP_FATE_MIN_LEVEL,
                                      min_fate_level, this);
    }
  }
  this->SendOtherPlayerToCenter(false);
}

int64_t kLastUpdateOtherPlayer = 0;
VectorMap<int64_t, int64_t> kUpdateOtherPlayerMap;

void LogicPlayer::SendOtherPlayerToCenter(bool force) {
  if (GetSecondsDiffDays(kLastUpdateOtherPlayer, GetSeconds())) {
    kLastUpdateOtherPlayer = GetSeconds();
    kUpdateOtherPlayerMap.clear();
  }
  if (this->level() < 45) return;

  VectorMap<int64_t, int64_t>::iterator player_fight =
      kUpdateOtherPlayerMap.find(this->uid());
  if (!force) {
    if (this->fight_attr_ < 10000 ||
        this->fight_attr_ < this->max_fight_attr_ ||
        (player_fight != kUpdateOtherPlayerMap.end() &&
         this->fight_attr_ <= player_fight->second))
      return;
  }

  MessageSSUpdateOtherPlayerInfo request;
  this->FillOtherPlayerInfo(request.mutable_info());
  server->SendMessageToCenter(MSG_SS_UPDATE_OTHER_PLAYER_INFO, &request);
  kUpdateOtherPlayerMap[this->uid()] = this->fight_attr_;
}

void LogicPlayer::CalcHeroPercent(sy::HeroInfo* info) {
  //计算百分比
  this->current_carrier_.mutable_extra_damage1()->Resize(AttackAttr_ARRAYSIZE, 0);
  RepeatedField<int64_t>& attack = *info->mutable_attr1();
  for (int32_t i = AttackAttr_MIN; i < AttackAttr_ARRAYSIZE; ++i) {
    attack.Set(i, attack.Get(i) + this->current_carrier_.extra_damage1(i));
  }

  std::pair<int32_t, int32_t> percent[] = {
    std::make_pair(ATTACK_ATTR_HP, ATTACK_ATTR_HP_PERCENT),
    std::make_pair(ATTACK_ATTR_AP, ATTACK_ATTR_AP_PERCENT),
    std::make_pair(ATTACK_ATTR_SP, ATTACK_ATTR_SP_PERCENT),
    std::make_pair(ATTACK_ATTR_WF, ATTACK_ATTR_WF_PERCENT),
    std::make_pair(ATTACK_ATTR_FF, ATTACK_ATTR_FF_PERCENT),
  };
  for (int32_t i = 0; i < ArraySize(percent); ++i) {
    int64_t old_value = attack.Get(percent[i].first);
    int32_t new_value = old_value * (attack.Get(percent[i].second) + 1000) / 1000;
    attack.Set(percent[i].first, new_value);
  }

  //计算战斗力
  int64_t score = CalcFightScore(attack, false);
  attack.Set(0, score);
}

void LogicPlayer::CalcHeroAtTacticAttr(
    LogicHero* ship, google::protobuf::RepeatedField<int64_t>* equips,
    TacticHeroSet& ships_set, AttackAttrArray& attr) {
  sy::HeroInfo* info = &ship->first;
  const HeroBase* base = ship->second.get();

  //装备的属性
  //前四个是装备, 后面两个是宝物
  int32_t equip_min_level = 999999;
  int32_t equip_refine_min_level = 999999;
  VectorMap<int32_t, int32_t> suits_effect;
  for (int32_t i = 0; equips && i < equips->size() && i < 4; ++i) {
    int64_t equip_uid = equips->Get(i);
    if (!equip_uid) equip_min_level = equip_refine_min_level = 0;
    if (equip_uid) {
      LogicItem* equip = this->items_.GetItemByUniqueID(equip_uid);
      if (!equip) {
        equips->Set(i, 0);
        equip_min_level = equip_refine_min_level = 0;
        continue;
      }

      const EquipBase* equip_base = equip->equip_base();
      if (!equip_base) {
        ERROR_LOG(logger)("%s PlayerID:%ld, ItemUID:%ld, ItemID:%d equip base is null",
                                                                      __FUNCTION__, this->uid(), equip->uid(), equip->item_id() );
        continue;
      }
      int32_t refine_level = equip->GetAttr(ITEM_ATTR_REFINE_LEVEL);
      int32_t equip_level = equip->GetAttr(ITEM_ATTR_LEVEL);
      equip_min_level =
          equip_min_level < equip_level ? equip_min_level : equip_level;
      equip_refine_min_level = equip_refine_min_level < refine_level
                                   ? equip_refine_min_level
                                   : refine_level;

      //基础属性
      AddConfigAttr(attr, equip_base->baseproperty);
      //强化属性
      AddConfigAttr<100>(attr, equip_base->upproperty, equip_level);
      //精炼属性
      AddConfigAttr<100>(attr, equip_base->refproperty, refine_level);
      suits_effect[equip_base->suit]++;

      //装备表:改造激活, 升金增加属性
      int32_t gold_level = equip->GetAttr(ITEM_ATTR_GOLD_LEVEL);
      if (!equip_base->gaizao_tainfu.empty() &&
          !equip_base->shengjin_tainfu.empty()) {
        RedGoldSkillBase::AddRedGoldAttr(
            equip_base->id(), equip_base->GetGaiZaoTianFuLevel(refine_level),
            equip_base->GetShenJinTainFuLevel(gold_level), attr);
      }

      //红装升星
      if (equip_base->quality == 6) {
        int32_t red_equip_level = equip->GetAttr(ITEM_ATTR_STAR_LEVEL);
        int32_t red_id = equip_base->id() % 100 * 10 + red_equip_level;
        RedequipRisestarBase* rdr_base =
            REDEQUIP_RISESTAR_BASE.GetEntryByID(red_id).get();
        RedequipCostBase* rds_base =
            REDEQUIP_COST_BASE.GetEntryByID(red_equip_level).get();
        if (rdr_base && rds_base) {
          float rate = 1;
          if (rds_base->exp)
            rate = equip->GetAttr(ITEM_ATTR_STAR_EXP) / (float)rds_base->exp;
          attr[rdr_base->attr_type] +=
              rdr_base->attr1 + rdr_base->attr_max * rate;
          if (rdr_base->attr_type == ATTACK_ATTR_AP)
            attr[ATTACK_ATTR_SP] += rdr_base->attr1 + rdr_base->attr_max * rate;
        }
      }
    }
  }

  for (VectorMap<int32_t, int32_t>::const_iterator it = suits_effect.begin();
       it != suits_effect.end(); ++it) {
    for (int32_t i = 2; i <= it->second; i++) {
      const SuitBase* suit = SUIT_BASE.GetEntryByID(it->first * 100 + i).get();
      if (suit) {
        AddConfigAttr(attr, suit->property);
      }
    }
  }
  //装备强化大师
  const EquipMasterBase* equip_master_base =
      EquipMasterBase::GetEquipMasterBase(100, equip_min_level);
  if (equip_master_base) {
    AddConfigAttr(attr, equip_master_base->property);
  }
  equip_master_base =
      EquipMasterBase::GetEquipMasterBase(200, equip_refine_min_level);
  if (equip_master_base) {
    AddConfigAttr(attr, equip_master_base->property);
  }

  int32_t navy_min_level = 999999;
  int32_t navy_refine_min_level = 999999;
  for (int32_t i = 4; equips && i < equips->size() && i < 6; ++i) {
    int64_t navy_uid = equips->Get(i);
    if (!navy_uid) navy_min_level = navy_refine_min_level = 0;
    if (navy_uid) {
      LogicItem* navy = this->items_.GetItemByUniqueID(navy_uid);
      if (!navy) {
        equips->Set(i, 0);
        navy_min_level = navy_refine_min_level = 0;
        continue;
      }

      const ArmyBase* army_base = navy->army_base();
      if (!army_base) {
        ERROR_LOG(logger)("%s PlayerID:%ld, ItemUID:%ld, ItemID:%d equip base is null", __PRETTY_FUNCTION__, this->uid(), navy->uid(), navy->item_id());
        continue;
      }
      int32_t navy_level = navy->GetAttr(ITEM_ATTR_NAVY_LEVEL);
      int32_t navy_refine_level = navy->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL);
      navy_min_level =
          navy_min_level < navy_level ? navy_min_level : navy_level;
      navy_refine_min_level = navy_refine_min_level < navy_refine_level
                                  ? navy_refine_min_level
                                  : navy_refine_level;

      //强化属性
      for (ValuePair3Vec::const_iterator iter = army_base->baseproperty.begin();
           iter != army_base->baseproperty.end(); ++iter) {
        attr[iter->v1] += iter->v2 / 100 + (iter->v3 * navy_level / 100);
      }
      //改造属性
      AddConfigAttr<100>(attr, army_base->refproperty, navy_refine_level);
      //部队表:改造激活属性,升金激活属性
      int32_t gold_level = navy->GetAttr(ITEM_ATTR_NVAY_GOLD_LEVEL);
      if (!army_base->shengjin_tainfu.empty() &&
          !army_base->gaizao_tainfu.empty()) {
        RedGoldSkillBase::AddRedGoldAttr(army_base->id(), navy_refine_level,
                                         gold_level, attr);
      }
    }
  }
  //宝物(海军)强化大师
  const EquipMasterBase* navy_master_base =
      EquipMasterBase::GetEquipMasterBase(300, navy_min_level);
  if (navy_master_base) {
    AddConfigAttr(attr, navy_master_base->property);
  }
  navy_master_base = EquipMasterBase::GetEquipMasterBase(400, navy_refine_min_level);
  if (navy_master_base) {
    AddConfigAttr(attr, navy_master_base->property);
  }

  //飞机强化大师
  int32_t plane_all_level = 0;
  for (int32_t i = 0; i < this->current_carrier_.plane_id_size(); i++) {
    int32_t pid = this->current_carrier_.plane_id(i);
    const CarrierPlaneBase* plane_base =
        CARRIER_PLANE_BASE.GetEntryByID(pid).get();
    if (plane_base) plane_all_level += plane_base->plane_lv;
  }
  const EquipMasterBase* plane_master_base =
      EquipMasterBase::GetEquipMasterBase(500, plane_all_level);
  if (plane_master_base) AddConfigAttr(attr, plane_master_base->property);

  //航母支援
  do {
    if (equips->size() < 7) break;
    CarrierInfo* carrier_info = this->GetCarrierByID(equips->Get(6));
    if (!carrier_info) break;
    CarrierBase* carrier_base =
        CARRIER_BASE.GetEntryByID(carrier_info->carrier_id()).get();
    if (!carrier_base) break;

    int32_t group_id = carrier_base->breakadvancedid;
    int32_t level = carrier_info->level() + 1;
    int32_t grade = carrier_info->reform_level();

    BreakAdvAttr carrier_attr =
        BreakAdvancedBase::GetBreakAdvAttr(group_id, grade, level);
    int32_t per =
        Setting::GetValueInVec2("carrier_mount", carrier_base->quality);

    attr[ATTACK_ATTR_HP] += carrier_attr[ATTACK_ATTR_HP] * per / 100;
    attr[ATTACK_ATTR_AP] += carrier_attr[ATTACK_ATTR_AP] * per / 100;
    attr[ATTACK_ATTR_SP] += carrier_attr[ATTACK_ATTR_SP] * per / 100;
    attr[ATTACK_ATTR_WF] += carrier_attr[ATTACK_ATTR_WF] * per / 100;
    attr[ATTACK_ATTR_FF] += carrier_attr[ATTACK_ATTR_FF] * per / 100;
  } while (false);

  // 天赋
  // 不上阵的船不能给其他船加属性(给自己也不行)
  for (ValuePair2Vec::const_iterator iter = base->talent1.begin();
       iter != base->talent1.end(); ++iter) {
    if (info->grade() < iter->v1) break;
    const BreakTalentBase* talent_base = BREAK_TALENT.GetEntryByID(iter->v2).get();
    if (!talent_base) continue;

    Array<sy::HeroInfo*, 6> targets;
    switch (talent_base->target) {
      case 1: {//自己
        targets.push_back(info);
      } break;
      case 2: //全部友军
      case 3: //英国
      case 4: //美国
      case 5: //德国
      case 6: //日本
      {
        for (TacticHeroSet::iterator iter = ships_set.begin();
             iter != ships_set.end(); ++iter) {
          if ((talent_base->target - 2) == 0 ||
              (*iter)->second->country == (talent_base->target - 2))
            targets.push_back(&(*iter)->first);
        }
      } break;
    }
    for (size_t i = 0; i < targets.size(); ++i) {
      for (ValuePair2Vec::const_iterator add_iter = talent_base->buff_type.begin();
           add_iter != talent_base->buff_type.end(); ++add_iter) {
        int32_t old = targets[i]->mutable_attr1()->Get(add_iter->v1);
        targets[i]->mutable_attr1()->Set(add_iter->v1, old + add_iter->v2);
      }
    }
  }
  //军衔
  for (size_t i = 0; i < rank_attr_.size(); ++i) {
    if (rank_attr_[i]) attr[i] += rank_attr_[i];
  }
  //图鉴
  this->CalcChart();
  for (size_t i = 0; i < chart_attr_.size(); ++i) {
    if (chart_attr_[i]) attr[i] += chart_attr_[i];
  }
  //军团
  this->AddLeagueAttr(attr);
  //缘分
  for (int32_t i = 0; i < ship->first.relation_size(); ++i) {
    const RelationBase* base = RELATION_BASE.GetEntryByID(ship->first.relation(i)).get();
    if (!base) continue;

    Array<sy::HeroInfo*, 6> targets;
    switch (base->target) {
      case 1: {//自己
        targets.push_back(info);
      } break;
      case 2:  //全部友军
      case 3:  //英国
      case 4:  //美国
      case 5:  //德国
      case 6:  //日本
      {
        for (TacticHeroSet::iterator iter = ships_set.begin();
             iter != ships_set.end(); ++iter) {
          if ((base->target - 2) == 0 ||
              (*iter)->second->country == (base->target - 2))
            targets.push_back(&(*iter)->first);
        }
      } break;
    }
    for (size_t j = 0; j < targets.size(); ++j) {
      for (ValuePair2Vec::const_iterator add_iter = base->pro.begin();
           add_iter != base->pro.end(); ++add_iter) {
        int32_t old = targets[j]->mutable_attr1()->Get(add_iter->v1);
        targets[j]->mutable_attr1()->Set(add_iter->v1, old + add_iter->v2);
      }
    }
  }
  //援军
  if (this->tactic_.support_pos_size() == 6) {
    int32_t min_level = 9999999;
    for (int32_t i = 0; i < this->tactic_.support_pos_size(); i++) {
      const sy::PositionInfo& info = this->tactic_.support_pos(i);
      LogicHero* hero = GetHeroByUID(info.hero_uid());
      if (hero) {
        min_level =
            min_level < hero->first.level() ? min_level : hero->first.level();
      } else {
        min_level = 0;
        break;
      }
    }
    const SupportBasePtr& support_base = SupportBase::GetSupportBase(min_level);
    if (support_base) AddConfigAttr(attr, support_base->property);
  }
  //勋章
  this->AddMedalAttr(attr);
}

void LogicPlayer::CalcHeroAttr(LogicHero* ship, TacticHeroSet* set,
                               int32_t notify) {
  static TacticHeroSet empty;

  sy::HeroInfo* info = &ship->first;
  TacticHeroSet& ships_set = set ? *set : empty;
  //没有上阵
  if (!set) {
    info->mutable_attr1()->Resize(0, 0);
  }
  info->mutable_attr1()->Resize(AttackAttr_ARRAYSIZE, 0);

  Array<int64_t, AttackAttr_ARRAYSIZE> attr;
  attr.resize(AttackAttr_ARRAYSIZE, 0);

  const HeroBase *base = ship->second.get();
  if (!base) {
    ERROR_LOG(logger)("%s HeroID:%d not found", __FUNCTION__,
                      info->hero_id());
    return;
  }

  int32_t group_id = base->breakadvancedid;
  int32_t level = info->level();
  int32_t grade = info->grade();
  RepeatedField<int64_t>* equips = NULL;
  int32_t ship_pos = 1;
  for (TacticHeroSet::const_iterator iter = ships_set.begin();
       iter != ships_set.end(); ++iter, ++ship_pos) {
    if (*iter == ship) {
      equips = this->GetEquipsInfo(ship_pos);
      break;
    }
  }

  BreakAdvAttr temp_attr =
      BreakAdvancedBase::GetBreakAdvAttr(group_id, grade, level);
  attr[ATTACK_ATTR_HP] = temp_attr[ATTACK_ATTR_HP];
  attr[ATTACK_ATTR_AP] = temp_attr[ATTACK_ATTR_AP];
  attr[ATTACK_ATTR_SP] = temp_attr[ATTACK_ATTR_SP];
  attr[ATTACK_ATTR_WF] = temp_attr[ATTACK_ATTR_WF];
  attr[ATTACK_ATTR_FF] = temp_attr[ATTACK_ATTR_FF];

  //天命
  FateBase* fate_base = FATE_BASE.GetEntryByID(ship->first.fate_level()).get();
  if (fate_base) {
    AddConfigAttr(attr, fate_base->attack_value);
    attr[fate_base->hp_value.v1] += fate_base->hp_value.v2;
    attr[fate_base->huopaodef_value.v1] += fate_base->huopaodef_value.v2;
    attr[fate_base->daodandef_value.v1] += fate_base->daodandef_value.v2;
  }
  //初始怒气
  attr[sy::ATTACK_ATTR_ANGER] += base->fury;
  //加固(船随机属性)
  {
    const int32_t kAttrCount = 4;
    info->mutable_rand_attr()->Resize(kAttrCount, 0);
    attr[sy::ATTACK_ATTR_HP] += info->rand_attr(0);
    attr[sy::ATTACK_ATTR_AP] += info->rand_attr(1);
    attr[sy::ATTACK_ATTR_SP] += info->rand_attr(1);
    attr[sy::ATTACK_ATTR_WF] += info->rand_attr(2);
    attr[sy::ATTACK_ATTR_FF] += info->rand_attr(3);
  }
  //觉醒
  AddWakeAttr(info->wake_level(), base->wake_itemtree, attr);
  for (int32_t i = 0; i < info->wake_item_size(); i++) {
    WakeItemBase* wake_base =
        WAKE_ITEM_BASE.GetEntryByID(info->wake_item(i)).get();
    if (wake_base) {
      for (ValuePair2Vec::iterator it = wake_base->attr.begin();
           it != wake_base->attr.end(); ++it)
        attr[it->v1] += it->v2;
    }
  }

  if (equips && equips->size() < 6) equips->Resize(6, 0);
  if (equips)
    DEBUG_LOG(logger)("CalcHeroAttr, HeroUID:%ld, ShipPos:%d, Item:%ld,%ld,%ld,%ld,%ld,%ld",
        info->uid(), ship_pos,
        equips->Get(0), equips->Get(1), equips->Get(2), equips->Get(3), equips->Get(4), equips->Get(5));

  //计算船只上阵属性
  if (set) {
    this->CalcHeroAtTacticAttr(ship, equips, ships_set, attr);
  }

  for (int32_t i = 0; i < AttackAttr_ARRAYSIZE; ++i) {
    info->mutable_attr1()->Set(i, attr[i] + info->attr1(i));
  }
  //没有上阵, 就需要计算百分比加成
  //如果上阵了, 最后才需要计算
  if (!set) this->CalcHeroPercent(info);

  if (notify & kNotifyClient) {
    MessageNotifyHeroInfo msg;
    msg.add_info()->CopyFrom(*info);
    this->SendMessageToClient(MSG_CS_NOTIFY_HERO_INFO, &msg);
  }
  if (notify & kNotifyServer) {
    MessageSSUpdateHeroInfo msg;
    msg.add_info()->CopyFrom(*info);
    msg.set_tid(server->GetTID());
    msg.set_system(0);
    msg.set_msgid(0);
    this->SendMessageToDB(MSG_SS_UPDATE_HERO_INFO, &msg);
  }
}

void LogicPlayer::AddHeroExp(LogicHero* info, int32_t exp) {
  sy::HeroInfo *ship = &info->first;
  if (ship->level() <= 0) ship->set_level(1);
  const HeroBase* hero_base = info->second.get();
  if (!hero_base) return;

  int64_t current_exp = ship->exp() + exp;
  int32_t current_level = ship->level();
  int32_t level_changed = 0;
  while (true) {
    int32_t need_exp = hero_base->GetLevelUpExp(current_level);
    if (need_exp <= 0 || current_exp < need_exp) {
      break;
    }
    current_exp -= need_exp;
    current_level += 1;
    level_changed += 1;
  }
  ship->set_level(current_level);
  ship->set_exp(current_exp);

  bool in_tactic = this->IsInTactic(ship->uid());

  if (level_changed) {
    if (in_tactic)
      this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_WAR_SHIP_UPGRADE,
                           MSG_CS_REQUEST_HERO_LEVEL_UP);
    else
      this->CalcHeroAttr(info, NULL, kNotifyAll);
  } else {
    MessageSSUpdateHeroInfo message;
    message.add_info()->CopyFrom(*ship);
    message.set_tid(server->GetTID());
    message.set_system(SYSTEM_ID_WAR_SHIP_UPGRADE);
    message.set_msgid(MSG_CS_REQUEST_HERO_LEVEL_UP);
    this->SendMessageToDB(MSG_SS_UPDATE_HERO_INFO, &message);

    MessageNotifyHeroInfo msg;
    msg.add_info()->CopyFrom(*ship);
    this->SendMessageToClient(MSG_CS_NOTIFY_HERO_INFO, &msg);
  }
}

void LogicPlayer::UpdateHeroResearchInfo() {
  MessageSSUpdateResearchHeroInfo message;
  message.mutable_info()->CopyFrom(this->hero_research_info_);
  this->SendMessageToDB(MSG_SS_UPDATE_RESEARCH_HERO_INFO, &message);

  MessageNotifyHeroResearchInfo notify;
  notify.mutable_info()->CopyFrom(this->hero_research_info_);
  this->SendMessageToClient(MSG_CS_NOTIFY_HERO_RESEARCH_INFO, &notify);
}

bool LogicPlayer::UpdateHeroInfo(std::vector<sy::HeroInfo>& infos,
                                 int32_t sys_id, int32_t msg_id) {
  if (infos.empty()) return true;
  for (std::vector<sy::HeroInfo>::iterator iter = infos.begin();
       iter != infos.end(); ++iter) {
    if (!this->UpdateHeroInfo(*iter, kNotifyNone, sys_id, msg_id)) return false;
  }

  MessageSSUpdateHeroInfo message;
  MessageNotifyHeroInfo notify;
  for (std::vector<sy::HeroInfo>::iterator iter = infos.begin();
       iter != infos.end(); ++iter) {
    message.add_info()->CopyFrom(*iter);
    notify.add_info()->CopyFrom(*iter);
  }
  message.set_tid(server->GetTID());
  message.set_msgid(msg_id);
  message.set_system(sys_id);
  this->SendMessageToDB(MSG_SS_UPDATE_HERO_INFO, &message);
  this->SendMessageToClient(MSG_CS_NOTIFY_HERO_INFO, &notify);

  return true;
}

bool LogicPlayer::UpdateHeroInfo(sy::HeroInfo& info, int32_t notify,int32_t sys_id,int32_t msg_id) {
  const HeroBasePtr& base = HERO_BASE.GetEntryByID(info.hero_id());
  if (!base) {
    ERROR_LOG(logger)("%s PlayerID:%ld, HeroUID:%ld, HeroID:%d not found", __FUNCTION__, this->uid(), info.uid(), info.hero_id());
    return false;
  }
  if (!info.has_uid() || info.uid() < 0) {
    info.set_uid(++this->ship_seed_);
  }
  if (info.level() <= 0) {
    info.set_level(1);
    info.set_exp(0);
  }
  if (info.fate_level() <= 0) {
    info.set_fate_level(1);
    info.set_fate_exp(0);
    info.set_fate_seed(FateBase::RandomShipFateScale());
  }

  LogicHero *ship = this->GetHeroByUID(info.uid());
  if (!ship) {
    this->ships_.push_back(std::make_pair(info, base));
    ship = &this->ships_.back();
  } else
    ship->first.CopyFrom(info);

  this->CalcHeroAttr(ship, NULL, kNotifyNone);

  if (notify & kNotifyServer) {
    MessageSSUpdateHeroInfo message;
    message.add_info()->CopyFrom(info);
    message.set_tid(server->GetTID());
    message.set_system(sys_id);
    message.set_msgid(msg_id);
    this->SendMessageToDB(MSG_SS_UPDATE_HERO_INFO, &message);
  }

  if (notify & kNotifyClient) {
    MessageNotifyHeroInfo msg;
    msg.add_info()->CopyFrom(info);
    this->SendMessageToClient(MSG_CS_NOTIFY_HERO_INFO, &msg);
  }

  return true;
}

void LogicPlayer::UpdateMoneyInfo(/*const*/ ModifyCurrency& modify) {
  MessageNotifyMoneyInfo notify;
  notify.set_exp(this->player_.exp());
  notify.set_vip_exp(this->player_.vip_exp());
  notify.set_level(this->player_.level());
  notify.set_vip_level(this->player_.vip_level());
  notify.set_coin(this->player_.coin());
  notify.set_money(this->player_.money());
  notify.set_prestige(this->player_.prestige());
  notify.set_hero(this->player_.hero());
  notify.set_plane(this->player_.plane());
  notify.set_exploit(this->player_.exploit());
  notify.set_muscle(this->player_.muscle());
  notify.set_union_(this->player_.union_());
  notify.set_sysid(modify.system);
  notify.set_msgid(modify.msgid);

  this->SendMessageToClient(MSG_CS_NOTIFY_MONEY_INFO, &notify);

  MessageSSUpdateMoneyInfo message;
  message.set_system(modify.system);
  message.set_msgid(modify.msgid);
  message.set_tid(server->GetTID());

  message.set_exp(this->player_.exp());
  message.set_vip_exp(this->player_.vip_exp());
  message.set_coin(this->player_.coin());
  message.set_level(this->player_.level());
  message.set_money(this->player_.money());
  message.set_vip_level(this->player_.vip_level());
  message.set_prestige(this->player_.prestige());
  message.set_hero(this->player_.hero());
  message.set_plane(this->player_.plane());
  message.set_exploit(this->player_.exploit());
  message.set_muscle(this->player_.muscle());
  message.set_union_(this->player_.union_());
  message.add_delta(0);
  for (int32_t i = MONEY_KIND_COIN; i <= MONEY_KIND_UNION; ++i) {
    message.add_delta(modify[i]);
  }
  message.set_delta_vip_level(modify.delta_vip_level);
  message.set_delta_level(modify.delta_level);

  this->SendMessageToDB(MSG_SS_UPDATE_MONEY_INFO, &message);
}

const int32_t MSG_MAP[][2] = {
    {MSG_SS_REQUEST_CREATE_PLAYER, MSG_CS_REQUEST_CREATE_PLAYER},
    {MSG_SS_RESPONSE_GET_MAIL_REWARD, MSG_CS_REQUEST_GET_MAIL_REWARD},
};

int32_t LogicPlayer::ProcessServerErrorMessage(SSMessageEntry& entry) {
  MessageSSServerMessageError *message = static_cast<MessageSSServerMessageError*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t msgid = message->msg_id();
  for (uint32_t i = 0; i < sizeof(MSG_MAP) / sizeof(MSG_MAP[0]); ++i) {
    if (MSG_MAP[i][0] == msgid) {
      this->SendErrorCodeToClient(message->error_code(), MSG_MAP[i][1]);
      return ERR_OK;
    }
  }
  this->SendErrorCodeToClient(message->error_code(), 0);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestActiveCarrier(CSMessageEntry& entry) {
  MessageRequestActiveCarrier* message =
      static_cast<MessageRequestActiveCarrier*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (this->GetCarrierByID(message->carrier_id())) return ERR_CARRIER_EXISTED;
  const CarrierBasePtr& carrier_base = CARRIER_BASE.GetEntryByID(message->carrier_id());
  if (!carrier_base) return ERR_CARRIER_NOT_FOUND;
  AddSubItemSet update_items;

  for (std::vector<ValuePair2<int32_t, int32_t> >::const_iterator iter =
           carrier_base->item.begin();
       iter != carrier_base->item.end(); ++iter) {
    ItemParam item;
    item.item_id = iter->v1;
    item.item_count = -iter->v2;
    update_items.push_back(item);
  }

  CHECK_RET(this->CheckItem(&update_items, NULL, NULL));
  this->ObtainItem(&update_items, NULL, NULL, MSG_CS_REQUEST_ACTIVE_CARRIER,
                   SYSTEM_ID_CARRIER_BUILD);

  this->AddCarrier(message->carrier_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_ACTIVE_CARRIER, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::CheckCurrency(const ModifyCurrency& modify) {
  if (modify.empty) return ERR_CURRENCY_UNKNOW;
  int64_t coin = int64_t(this->player_.coin()) + int64_t(modify.coin);
  int64_t money = int64_t(this->player_.money()) + int64_t(modify.money);
  int64_t exp = int64_t(this->player_.exp()) + int64_t(modify.exp);
  int64_t vip_exp = int64_t(this->player_.vip_exp()) + int64_t(modify.vip_exp);
  int64_t oil = int64_t(this->player_.oil()) + int64_t(modify.oil);
  int64_t energy = int64_t(this->player_.energy()) + int64_t(modify.energy);
  int64_t prestige = int64_t(this->player_.prestige()) + int64_t(modify.prestige);
  int64_t hero = int64_t(this->player_.hero()) + int64_t(modify.hero);
  int64_t plane = int64_t(this->player_.plane()) + int64_t(modify.plane);
  int64_t muscle = int64_t(this->player_.muscle()) + int64_t(modify.muscle);
  int64_t exploit = int64_t(this->player_.exploit()) + int64_t(modify.exploit);
  int64_t union_ = int64_t(this->player_.union_()) + int64_t(modify._union);

  //两种基本货币都是int64
  if (coin < 0 || coin >= LONG_MAX) return ERR_CURRENCY_COIN;
  if (money < 0 || money >= LONG_MAX) return ERR_CURRENCY_MONEY;

  if (oil < 0) return ERR_CURRENCY_OIL;
  if (energy < 0) return ERR_CURRENCY_ENERGY;
  if (oil > Setting::kOilMax) return ERR_CURRENCY_OIL_OF;
  if (energy > Setting::kEnergyMax) return ERR_CURRENCY_ENERGY_OF;

  if (prestige < 0 || prestige > INT_MAX) return ERR_CURRENCY_PRESTIGE;
  if (hero < 0 || hero > INT_MAX) return ERR_CURRENCY_HERO;
  if (plane < 0 || plane > INT_MAX) return ERR_CURRENCY_PLANE;
  if (muscle < 0 || muscle > INT_MAX) return ERR_CURRENCY_MUSCLE;
  if (exploit < 0 || exploit > INT_MAX) return ERR_CURRENCY_EXPLOIT;
  if (union_ < 0 || union_ > INT_MAX) return ERR_CURRENCY_UNION;

  if (exp < 0) return ERR_CURRENCY_EXP;
  if (vip_exp < 0) return ERR_CURRENCY_VIP_EXP;

  return ERR_OK;
}

int32_t LogicPlayer::UpdateCurrency(/*const*/ ModifyCurrency& modify) {
  CHECK_RET(this->CheckCurrency(modify));
  if (!modify) return ERR_OK;

  if (modify.money < 0)
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_CONSUMER_FEEDBACK,
                                    -modify.money, this);

  int64_t coin = int64_t(this->player_.coin()) + int64_t(modify.coin);
  int64_t money = int64_t(this->player_.money()) + int64_t(modify.money);
  int64_t oil = int64_t(this->player_.oil()) + int64_t(modify.oil);
  int64_t energy = int64_t(this->player_.energy()) + int64_t(modify.energy);
  int64_t exp = int64_t(this->player_.exp()) + int64_t(modify.exp);
  int64_t vip_exp = int64_t(this->player_.vip_exp()) + int64_t(modify.vip_exp);
  int32_t prestige = int64_t(this->player_.prestige()) + int64_t(modify.prestige);
  int32_t hero = int64_t(this->player_.hero()) + int64_t(modify.hero);
  int32_t plane = int64_t(this->player_.plane()) + int64_t(modify.plane);
  int64_t muscle = int64_t(this->player_.muscle()) + int64_t(modify.muscle);
  int64_t exploit = int64_t(this->player_.exploit()) + int64_t(modify.exploit);
  int64_t union_ = int64_t(this->player_.union_()) + int64_t(modify._union);

  this->player_.set_coin(coin);
  this->player_.set_money(money);
  this->player_.set_oil(oil);
  this->player_.set_energy(energy);
  this->player_.set_exp(exp);
  this->player_.set_vip_exp(vip_exp);
  this->player_.set_prestige(prestige);
  this->player_.set_hero(hero);
  this->player_.set_plane(plane);
  this->player_.set_muscle(muscle);
  this->player_.set_exploit(exploit);
  this->player_.set_union_(union_);

  int32_t old_level = this->level();
  int32_t old_vip_level = this->vip_level();
  if (modify.exp) this->OnExpChanged();
  if (modify.vip_exp) this->OnVipExpChanged();

  if (!modify.delta_level) modify.delta_level = this->level() - old_level;
  if (!modify.delta_vip_level)
    modify.delta_vip_level = this->vip_level() - old_vip_level;
  //更新
  this->UpdateMoneyInfo(modify);
  if (modify.oil || modify.energy)
    this->UpdateOilInfo(true, modify.system, modify.msgid);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestTest(CSMessageEntry& entry) {
  MessageRequestTest* message =
      static_cast<MessageRequestTest*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (server_config->test_gm()) {
    this->ProcessGM(message->mutable_info());
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_TEST, NULL);
  return ERR_OK;
}

void LogicPlayer::SetAllCopyStar(const sy::CopyProgress& progress) {
  MessageNotifyCopyInfo notify;
  MessageSSUpdateCopyInfo update;
  const CopyChapterBase* chapter = COPY_CHAPTER_BASE.GetEntryByID(progress.chapter()).get();
  if (!chapter) return;

  std::vector<const CopyChapterBase*> queue;
  while (chapter) {
    queue.push_back(chapter);

    chapter = COPY_CHAPTER_BASE.GetEntryByID(chapter->last_chapter).get();
  }

  bool is_break = false;
  for (std::vector<const CopyChapterBase*>::const_reverse_iterator
           chapter_iter = queue.rbegin();
       chapter_iter != queue.rend() && !is_break; ++chapter_iter) {
    const CopyChapterBase* chapter = *chapter_iter;
    for (std::vector<CopyGateBasePtr>::const_iterator iter =
             chapter->levels.begin();
         iter != chapter->levels.end() && !is_break; ++iter) {
      int32_t copy_gate = (*iter)->id();
      (void)copy_gate;
      for (std::vector<CopyBasePtr>::const_iterator copy_iter =
               (*iter)->copys.begin();
           copy_iter != (*iter)->copys.end(); ++copy_iter) {
        int32_t copy_id = (*copy_iter)->id();
        this->copy_star_[copy_id] = 3;

        sy::CopyStarInfo star;
        star.set_star(3);
        star.set_copy_id(copy_id);
        notify.mutable_copy_star()->CopyFrom(star);
        update.add_copy_star()->CopyFrom(star);
        this->SendMessageToClient(MSG_CS_NOTIFY_COPY_INFO, &notify);
        DEBUG_LOG(logger)("Chapter:%ld, CopyID:%d", chapter->id(), copy_id);

        if (copy_id == progress.copy_id()) {
          is_break = true;
          break;
        }
      }
    }
  }

  notify.Clear();
  this->UpdateCopyProgress(progress, NULL, NULL);
  notify.mutable_progress()->CopyFrom(progress);
  update.mutable_copy()->CopyFrom(progress);
  this->SendMessageToClient(MSG_CS_NOTIFY_COPY_INFO, &notify);
  this->SendMessageToDB(MSG_SS_UPDATE_COPY_INFO, &update);
}

int32_t LogicPlayer::ProcessGM(sy::GMContent* info) {
  if (!info) return ERR_INTERNAL;

  ModifyCurrency modify(MSG_CS_REQUEST_TEST, SYSTEM_ID_GM);
  //判断Level和VIPLevel(优先级比较高)
  if (info->has_level()) {
    info->clear_exp();
    const ExpBasePtr& base = EXP_BASE.GetEntryByID(info->level());
    if (!base) return ERR_PARAM_OVERFLOW;
    modify.delta_level = info->level() - this->level();
    this->SetLevel(info->level());
    modify.force_update = true;
  }
  if (info->has_vip_level()) {
    info->clear_vip_exp();
    const VipExpBasePtr& base = VIP_EXP_BASE.GetEntryByID(info->vip_level());
    if (!base) return ERR_PARAM_OVERFLOW;
    modify.delta_vip_level = info->vip_level() - this->vip_level();
    this->SetVipLevel(info->vip_level());
    modify.force_update = true;
  }
  if (info->has_time_delta_sec()) {
    time_offset += info->time_delta_sec();
  }

  if (info->has_coin()) modify.coin = info->coin();
  if (info->has_money()) modify.money = info->money();
  if (info->has_oil()) modify.oil = info->oil();
  if (info->has_exp()) modify.exp = info->exp();
  if (info->has_vip_exp()) modify.vip_exp = info->vip_exp();
  if (info->has_prestige()) modify.prestige = info->prestige();
  if (info->has_hero()) modify.hero = info->hero();
  if (info->has_plane()) modify.plane = info->plane();
  if (info->has_muscle()) modify.muscle = info->muscle();
  if (info->has_exploit()) modify.exploit = info->exploit();
  if (info->has_union_()) modify._union = info->union_();
  if (info->has_energy()) modify.energy = info->energy();
  if (modify) CHECK_RET(this->CheckCurrency(modify));

  if (info->item_size() > ITEM_COUNT_ONE_TIME) return ERR_PARAM_ARRAY_BOUND;
  if (info->delete_items_size() > ITEM_COUNT_ONE_TIME) return ERR_PARAM_ARRAY_BOUND;

  UpdateItemSet update_items;
  DeleteItemSet delete_items;
  for (int32_t i = 0; i < info->delete_items_size(); ++i) {
    delete_items.push_back(info->delete_items(i));
  }
  for (int32_t i = 0; i < info->item_size(); ++i) {
    const sy::Item& item = info->item(i);
    update_items.push_back(item);
  }
  //道具
  if (update_items.size() || delete_items.size()) {
    CHECK_RET(this->CheckItem2(&update_items, &delete_items, NULL));
  }

  //检查航母
  for (int32_t i = 0; i < info->carrier_size(); ++i) {
    if (!CARRIER_BASE.GetEntryByID(info->carrier(i).carrier_id())) return ERR_CARRIER_NOT_FOUND;
  }
  //检查船只
  for (int32_t i = 0; i < info->ship_size(); ++i) {
    if (!HERO_BASE.GetEntryByID(info->ship(i).hero_id())) return ERR_HERO_NOT_FOUND;
  }

  this->ObtainItem2(&update_items, &delete_items, NULL, MSG_CS_REQUEST_TEST,
                    SYSTEM_ID_GM);
  if (modify) this->UpdateCurrency(modify);
  //添加航母
  for (int32_t i = 0; i < info->carrier_size(); ++i) {
    this->AddCarrier(info->carrier(i).carrier_id());
  }
  //添加船
  for (int32_t i = 0; i < info->ship_size(); ++i) {
    this->UpdateHeroInfo(*info->mutable_ship(i), kNotifyAll, SYSTEM_ID_GM,
                         MSG_CS_REQUEST_TEST);
  }
  //副本相关
  {
    //副本进度
    MessageNotifyCopyInfo notify;
    MessageSSUpdateCopyInfo update;
    for (int32_t i = 0; i < info->progress_size(); ++i) {
      notify.Clear();
      update.Clear();
      this->UpdateCopyProgress(info->progress(i), &notify, &update);
      this->SendMessageToClient(MSG_CS_NOTIFY_COPY_INFO, &notify);
      this->SendMessageToDB(MSG_SS_UPDATE_COPY_INFO, &update);
    }
    //副本星数
    for (int32_t i = 0; i < info->copy_star_size(); ++i) {
      notify.Clear();
      update.Clear();
      this->copy_star_[info->copy_star(i).copy_id()] = info->copy_star(i).star();
      notify.mutable_copy_star()->CopyFrom(info->copy_star(i));
      update.add_copy_star()->CopyFrom(info->copy_star(i));
      this->SendMessageToClient(MSG_CS_NOTIFY_COPY_INFO, &notify);
      this->SendMessageToDB(MSG_SS_UPDATE_COPY_INFO, &update);
    }
    if (info->has_all_star()) this->SetAllCopyStar(info->all_star());
  }

  if (info->has_create_time()) {
    MessageSSUpdateCreateTime update;
    update.set_create_time(info->create_time());
    this->player_.set_create_time(info->create_time());
    this->SendMessageToDB(MSG_SS_UPDATE_CREATE_TIME, &update);
  }
  if (info->has_sign_id()) {
    this->player_.set_sign_id(info->sign_id());
    this->player_.set_sign_time(0);
    MessageSSUpdateSignIn db_msg;
    db_msg.set_sign_id(this->player_.sign_id());
    db_msg.set_sign_time(this->player_.sign_time());
    this->SendMessageToDB(MSG_SS_UPDATE_SIGN_IN, &db_msg);
    MessageResponseSignIn response;
    response.set_sign_id(this->player_.sign_id());
    response.set_sign_time(this->player_.sign_time());
    this->SendMessageToClient(MSG_CS_RESPONSE_SIGN_IN, &response);
  }

  return ERR_OK;
}

int32_t LogicPlayer::GetCopyStar(int32_t copy_id) const {
  const CopyBase* base = COPY_BASE.GetEntryByID(copy_id).get();
  if (!base) return 0;
  //抢夺海军一直是3星
  if (base->type == sy::COPY_TYPE_ARMY) {
    return 3;
  }
  //如果是爬塔
  if (base->type == sy::COPY_TYPE_TOWER) {
    if (base->GetOrder() <= this->tower_state_.max_star_order()) return 3;
    return 0;
  }
  VectorMap<int32_t, int32_t>::const_iterator iter = this->copy_star_.find(copy_id);
  return iter != this->copy_star_.end() ? iter->second : 0;
}

int32_t LogicPlayer::ProcessRequestResearchHero(CSMessageEntry& entry) {
  MessageRequestResearchHero* message =
      static_cast<MessageRequestResearchHero*>(entry.get());
  if (!message) return ERR_INTERNAL;

  // 使用金钱 （1-1次采购 10-10次采购 20-1次研发）
  int32_t use_money = message->use_money();

  int32_t use_item_count =
      this->hero_research_info_.item_count();  //用材料研发次数(客户端不用)
  int32_t use_money_count =
      this->hero_research_info_.money_count();  //用钱研发次数(客户端不用)
  int32_t use_money_count2 =
      this->hero_research_info_.money_count2();  //十连抽次数(客户端不用)
  int32_t rd_count = this->hero_research_info_.rd_count();  //金币研发次数
  INFO_LOG(logger)("PlayerID:%ld, ResearchHero, UseMoney:%d, ItemCount:%d,UseMoneyCount:%d,UseMoneyCount2:%d,RDCount:%d"
      , this->uid(), use_money
      , use_item_count, use_money_count, use_money_count2, rd_count);

  AddSubItemSet item;
  ModifyCurrency money(MSG_CS_REQUEST_RESEARCH_HERO, SYSTEM_ID_BUILD_SHIPS);

  bool ten_half = false;
  bool one_half = false;
  bool is_free = true;

  do {
    if (use_money == 1) {
      //免费时间
      if (this->hero_research_info_.last_free_time() >=
          GetVirtualSeconds() - GetSettingValue(creboat_free_time)) {
        is_free = false;
        if (this->GetItemCountByItemID(GetSettingValue(exh_item_id)) >=
            GetSettingValue(exh_item_num)) {
          item.push_back(ItemParam(GetSettingValue(exh_item_id),
                                   -GetSettingValue(exh_item_num)));
        } else {
          int32_t ratio = 1;
          if (!GetDailySign(DAILY_RESEARCH_HERO)) {
            ratio = 2;
            one_half = true;
          }
          item.push_back(
              ItemParam(23900160, -(GetSettingValue(exh_gem_need) / ratio)));
        }
      }
      money.coin += GetSettingValue(recruit2_reward1);
    } else if (use_money == 10) {
      is_free = false;
      int32_t ratio = 1;
      if (achievements_[OTHER_TEN_RESEARCH_HERO_HALF] == 0 &&
          this->player_.total_recharge() > 0) {
        ratio = 2;
        ten_half = true;
      }
      item.push_back(
          ItemParam(23900160, -(GetSettingValue(exh_gem10_need) / ratio)));
      money.coin += GetSettingValue(recruit2_reward1) * 10;

    } else if (use_money == 20) {
      if ((this->hero_research_info_.day_free_rd_count() >=
           GetSettingValue(cre_free_num)) ||
          (this->hero_research_info_.last_free_rd_time() >=
           GetVirtualSeconds() - GetSettingValue(cre_free_cd))) {
        item.push_back(ItemParam(GetSettingValue(cre_item_id),
                                 -GetSettingValue(cre_item_num)));
        is_free = false;
      }
      money.coin += GetSettingValue(recruit1_reward1);
    } else if (use_money == 21) {
      is_free = false;
      item.push_back(ItemParam(GetSettingValue(cre_item_id),
                               -GetSettingValue(cre_item_num) * 10));
      money.coin += GetSettingValue(recruit1_reward1) * 10;
    } else {
      return ERR_PARAM_INVALID;
    }

    //校验参数
    CHECK_RET(this->CheckItem(&item, NULL, NULL));
    CHECK_RET(this->CheckCurrency(money));

  } while (false);

  MessageResponseGetResearchHero new_heros;

  int raffle_num = 0;
  if (use_money == 1)
    raffle_num = ++use_money_count;
  else if (use_money == 10)
    raffle_num = ++use_money_count2;
  else if (use_money == 20)
    raffle_num = ++rd_count;
  else if (use_money == 21)
    raffle_num = ++use_item_count;

  const std::vector<ShipPackBasePtr>* packs = NULL;

  const ShipRaffleBasePtr& base = GetShipRaffleByCount(raffle_num, use_money);
  if (base) packs = &base->packs_ptr;
  if (!packs) {
    ERROR_LOG(logger)("%s +%d", __PRETTY_FUNCTION__, __LINE__);
    return ERR_INTERNAL;
  }
  INFO_LOG(logger)("PlayerID:%ld, UseShipRaffleID:%ld, RaffleNum:%d, UseMoney:%d"
      , this->uid(), base->id(), raffle_num, use_money);

  std::vector<sy::HeroInfo> heros;
  for (std::vector<ShipPackBasePtr>::const_iterator iter = packs->begin();
       iter != packs->end(); ++iter) {
    int32_t hero_id = ShipPackBase::Random(RandomBetween(0, (*iter)->sum - 1),
                                           (*iter)->heroid);
    if (hero_id) {
      sy::HeroInfo info;
      info.set_hero_id(hero_id);
      heros.push_back(info);
    } else {
      ERROR_LOG(logger)("%s %d HeroNotFound", __PRETTY_FUNCTION__, __LINE__);
    }
  }
  if (use_money == 10) {
    int32_t hero_id =
        ShipPackBase::GetInitialShip(this->uid(), use_money_count2);
    if (hero_id) {
      sy::HeroInfo info;
      info.set_hero_id(hero_id);
      heros.push_back(info);
    }
  }

  std::random_shuffle(heros.begin(), heros.end());
  this->UpdateHeroInfo(heros, SYSTEM_ID_BUILD_SHIPS,
                       MSG_CS_REQUEST_RESEARCH_HERO);
  for (std::vector<sy::HeroInfo>::const_iterator iter = heros.begin();
       iter != heros.end(); ++iter) {
    new_heros.add_hero_uid(iter->uid());
    HeroBase* hero_base = HERO_BASE.GetEntryByID(iter->hero_id()).get();
    if (hero_base && hero_base->quality >= sy::QUALITY_ORANGE) {
      server->AddPrizeChche(this, iter->hero_id());
    }
  }

  if (use_money == 20)
    UpdateBuyCount(COUNT_TYPE_DAILY_MAKE_SHIP, 1);
  else if (use_money == 21)
    UpdateBuyCount(COUNT_TYPE_DAILY_MAKE_SHIP, 10);
  else
    UpdateBuyCount(COUNT_TYPE_DAILY_BUY_SHIP, 1);

  if (ten_half) UpdateAchievement(OTHER_TEN_RESEARCH_HERO_HALF, 1);
  if (one_half) SetDailySign(DAILY_RESEARCH_HERO, true);

  this->ObtainItem(&item, NULL, NULL, MSG_CS_REQUEST_RESEARCH_HERO,
                   SYSTEM_ID_BUILD_SHIPS);
  this->UpdateCurrency(money);

  this->hero_research_info_.set_money_count(use_money_count);
  this->hero_research_info_.set_money_count2(use_money_count2);
  this->hero_research_info_.set_item_count(use_item_count);
  this->hero_research_info_.set_rd_count(rd_count);

  if (is_free) {
    if (use_money == 1) {
      this->hero_research_info_.set_last_free_time(GetVirtualSeconds());
    }
    if (use_money == 20) {
      this->hero_research_info_.set_last_free_rd_time(GetVirtualSeconds());
      this->hero_research_info_.set_day_free_rd_count(
          this->hero_research_info_.day_free_rd_count() + 1);
    }
  }

  this->UpdateHeroResearchInfo();

  if (new_heros.hero_uid_size())
    this->SendMessageToClient(MSG_CS_RESPONSE_GET_RESEARCH_HERO, &new_heros);

  if (use_money == 10) {
    UpdateAchievement(SEVEN_DAY_TYPE_SHIPBUILD,
                      this->achievements_[SEVEN_DAY_TYPE_SHIPBUILD] + 1);
  }

  if (use_money == 10 || use_money == 1)
  {
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_BUY_SHIP, use_money, this);
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_BUY_SHIP, use_money,
                                    this);
  }

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetResearchHero(CSMessageEntry& entry) {
  const int32_t hero_id = this->hero_research_info_.hero_id();

  if (!hero_id) return ERR_PARAM_INVALID;
  if (GetVirtualSeconds() < this->hero_research_info_.last_time())
    return ERR_COLD_DOWN;

  sy::HeroInfo info;
  info.set_hero_id(hero_id);
  this->UpdateHeroInfo(info, kNotifyAll, SYSTEM_ID_WAR_SHIP_RESEARCH,
                       MSG_CS_REQUEST_GET_RESEARCH_HERO);
  int64_t uid = info.uid();

  this->hero_research_info_.set_hero_id(0);
  this->UpdateHeroResearchInfo();

  MessageResponseGetResearchHero message;
  message.add_hero_uid(uid);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_RESEARCH_HERO, &message);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestEquipRefine(CSMessageEntry& entry) {
  MessageRequestEquipRefine* message =
      static_cast<MessageRequestEquipRefine*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicItem* equip = this->items().GetItemByUniqueID(message->equip_uid());
  if (!equip) return ERR_ITEM_NOT_FOUND;

  AddSubItemSet sub_set;
  int32_t exp = 0;
  for (int32_t i = 0; i < message->items_size(); i++) {
    const KVPair2& item = message->items(i);
    sub_set.push_back(ItemParam(item.key(), -item.value()));
    exp += Setting::GetValueInVec2(Setting::equipreform_exp, item.key()) *
           item.value();
  }
  if (!exp) return ERR_ITEM_REFINE_NOT_FOUND;
  CHECK_RET(CheckItem(&sub_set, NULL, NULL));

  int32_t quality = equip->equip_base()->quality;
  int32_t current_refine_level = equip->GetAttr(ITEM_ATTR_REFINE_LEVEL);
  int32_t orgin_level = current_refine_level;
  int32_t current_refine_exp = equip->GetAttr(ITEM_ATTR_REFINE_EXP);

  const EquipRefineBase* refine_base =
      EQUIP_REFINE_BASE.GetEntryByID(current_refine_level).get();
  if (!refine_base) return ERR_ITEM_LEVEL_MAX;

  //升级
  current_refine_exp += exp;
  while (refine_base &&
         refine_base->GetExpByEquipQuality(quality) <= current_refine_exp) {
    current_refine_exp -= refine_base->GetExpByEquipQuality(quality);
    current_refine_level += 1;
    refine_base = EQUIP_REFINE_BASE.GetEntryByID(current_refine_level).get();
  }

  equip->SetAttr(ITEM_ATTR_REFINE_LEVEL, current_refine_level);
  equip->SetAttr(ITEM_ATTR_REFINE_EXP, current_refine_exp);
  UpdateBuyCount(COUNT_TYPE_DAILY_REFINE_EQUIP, 1);

  //通知道具
  NotifyItemSet notify_set;
  notify_set.push_back(equip->uid());
  this->ObtainItem(&sub_set, NULL, &notify_set, MSG_CS_REQUEST_EQUIP_REFINE,
                   SYSTEM_ID_EQUIP_REFORM);
  this->OnEquipChanged(equip->GetAttr(sy::ITEM_ATTR_EQUIPED_HERO));

  MessageResponseEquipRefine response;
  response.set_change_level(current_refine_level - orgin_level);
  response.set_equip_uid(equip->uid());
  this->SendMessageToClient(MSG_CS_RESPONSE_EQUIP_REFINE, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestEquipLevelUp(CSMessageEntry& entry) {
  MessageRequestEquipLevelUp* message =
      static_cast<MessageRequestEquipLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (!message->has_count() || message->count() < 0) message->set_count(1);

  LogicItem *equip = this->items().GetItemByUniqueID(message->equip_uid());
  if (!equip || !equip->equip_base()) return ERR_ITEM_NOT_FOUND;
  int32_t count = message->count();
  int32_t quality = equip->equip_base()->quality;

  ModifyCurrency modify(MSG_CS_REQUEST_EQUIP_LEVEL_UP,
                        SYSTEM_ID_EQUIP_LEVEL_UP);
  int32_t level_up_count = 0;
  int32_t level_up_count2 = 0, level_up_count3 = 0;
  int32_t current_level = equip->GetAttr(ITEM_ATTR_LEVEL);
  if (current_level >= this->level() * 2 - 1) return ERR_ITEM_LEVEL_MAX;

  for (int32_t i = 0; i < count; ++i) {
    if (current_level >= this->level() * 2) break;
    //升级10次, 最多升10级(不算暴击)
    if (level_up_count >= count) break;
    if (message->max_level() && current_level >= message->max_level()) break;

    const EquipLevelUpBase* level_base = EQUIP_LEVEL_UP_BASE.GetEntryByID(current_level).get();
    const VipFunctionBase* vip_2 = VIP_FUNCTION_BASE.GetEntryByID(260).get();
    const VipFunctionBase* vip_3 = VIP_FUNCTION_BASE.GetEntryByID(261).get();
    if (!level_base || !vip_2 || !vip_3) break;
    ValuePair2<int32_t, int32_t> cost = level_base->GetMoneyByTyEquipQuality(quality);
    if (!cost.v2) break;
    int32_t level_up = 1;
    int32_t random = RandomIn10000();
    if (random < vip_3->GetValue(this->vip_level(), this->level())) {
      level_up = 3;
      level_up_count3++;
    } else if (random < vip_2->GetValue(this->vip_level(), this->level())) {
      level_up = 2;
      level_up_count2++;
    }

    modify[cost.v1] -= cost.v2;
    //钱不够了, 就按照这么多钱升级
    if (this->CheckCurrency(modify)) {
      modify[cost.v1] += cost.v2;
      break;
    }
    current_level += level_up;
    level_up_count += 1;
  }
  if (!modify || modify.empty) return ERR_CURRENCY_COIN;
  int32_t error_code = this->CheckCurrency(modify);
  if (error_code) return error_code;

  //装备等级发生变化,才会真的升级,计算属性
  if (current_level != equip->GetAttr(ITEM_ATTR_LEVEL)) {
    equip->SetAttr(ITEM_ATTR_LEVEL, current_level);
    UpdateBuyCount(COUNT_TYPE_DAILY_EQUIP_UP, 1);

    NotifyItemSet notify_set;
    notify_set.push_back(equip->uid());
    this->ObtainItem(NULL, NULL, &notify_set, MSG_CS_REQUEST_EQUIP_LEVEL_UP,
                     SYSTEM_ID_EQUIP_LEVEL_UP);
    this->OnEquipChanged(equip->GetAttr(sy::ITEM_ATTR_EQUIPED_HERO));
    this->UpdateCurrency(modify);
  }

  MessageResponseEquipLevelUp response;
  response.set_equip_uid(equip->uid());
  response.set_count1(level_up_count);
  response.set_count2(level_up_count2);
  response.set_count3(level_up_count3);
  this->SendMessageToClient(MSG_CS_RESPONSE_EQUIP_LEVEL_UP, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestHeroLevelUp(CSMessageEntry& entry) {
  MessageRequestHeroLevelUp* message =
      static_cast<MessageRequestHeroLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicHero* hero = this->GetHeroByUID(message->hero_uid());
  if (!hero) return ERR_HERO_NOT_FOUND;
  HeroBase* base = HERO_BASE.GetEntryByID(hero->first.hero_id()).get();
  if (!base) return ERR_HERO_NOT_FOUND;

  int32_t max_exp = 0;
  for (int32_t i = hero->first.level(); i <= this->level(); ++i) {
    max_exp += base->GetLevelUpExp(i);
  }
  max_exp -= (hero->first.exp() + 1);

  if (max_exp <= 0) return ERR_INTERNAL;

  AddSubItemSet sub_set;
  int32_t exp = 0;
  for (int32_t i = 0; i < message->item_list_size(); ++i) {
    int64_t item_uid = message->item_list(i).key();
    int32_t item_count = message->item_list(i).value();
    LogicItem* item = this->items().GetItemByUniqueID(item_uid);
    if (!item) return ERR_ITEM_NOT_FOUND;
    sub_set.push_back(ItemParam(item->item_id(), -item_count));
    if (item_count < 0) return ERR_PARAM_INVALID;
    exp += Setting::GetValueInVec2(Setting::shiplvup_exp, item->item_id()) *
           item_count;
  }

  if (exp <= 0) return ERR_INTERNAL;
  if (exp > max_exp) exp = max_exp;

  ModifyCurrency modify(MSG_CS_REQUEST_HERO_LEVEL_UP,
                        SYSTEM_ID_WAR_SHIP_UPGRADE);
  modify[MONEY_KIND_COIN] = -exp;
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&sub_set, NULL, NULL));

  UpdateCurrency(modify);
  this->ObtainItem(&sub_set, NULL, NULL, MSG_CS_REQUEST_HERO_LEVEL_UP,
                   SYSTEM_ID_WAR_SHIP_UPGRADE);
  this->AddHeroExp(hero, exp);

  MessageResponseHeroLevelUp response;
  response.set_hero_uid(hero->first.uid());
  this->SendMessageToClient(MSG_CS_RESPONSE_HERO_LEVEL_UP, &response);
  return ERR_OK;
}

void LogicPlayer::OnEquipChanged(int64_t hero_uid) {
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_TACTIC, 0);
  CalcEquipsCondition();
}

void LogicPlayer::CalcEquipsCondition() {
  bool equip_vacancy = false;
  bool navy_vacancy = false;
  int32_t equip_min_quality = 999999;
  int32_t equip_min_level = 999999;
  int32_t equip_min_refine_level = 999999;
  int32_t equip_max_refine_level = 0;
  int32_t navy_min_refine_level = 999999;
  int32_t navy_max_refine_level = 0;

  bool equip_once_falg = false;
  bool navy_once_flag = false;
  int32_t once_equip_min_level = 0;
  int32_t once_equip_min_refine_level = 0;
  int32_t once_navy_min_level = 0;
  int32_t once_navy_min_refine_level = 0;

  for (int32_t i = 1; i <= 6; ++i) {
    RepeatedField<int64_t>* equips = GetEquipsInfo(i);
    if (equips) {
      bool t_equip_once_falg = true;
      bool t_navy_once_flag = true;
      int32_t t_once_equip_min_level = 999999;
      int32_t t_once_equip_min_refine_level = 999999;
      int32_t t_once_navy_min_level = 999999;
      int32_t t_once_navy_min_refine_level = 999999;

      for (int32_t i = 0; i < equips->size() && i < 4; ++i) {
        int64_t equip_uid = equips->Get(i);
        LogicItem* item = this->items_.GetItemByUniqueID(equip_uid);
        if (item && item->equip_base()) {
          equip_min_quality = equip_min_quality < item->equip_base()->quality
                                  ? equip_min_quality
                                  : item->equip_base()->quality;
          equip_min_level = equip_min_level < item->GetAttr(ITEM_ATTR_LEVEL)
                                ? equip_min_level
                                : item->GetAttr(ITEM_ATTR_LEVEL);
          equip_min_refine_level =
              equip_min_refine_level < item->GetAttr(ITEM_ATTR_REFINE_LEVEL)
                  ? equip_min_refine_level
                  : item->GetAttr(ITEM_ATTR_REFINE_LEVEL);
          equip_max_refine_level =
              equip_max_refine_level > item->GetAttr(ITEM_ATTR_REFINE_LEVEL)
                  ? equip_max_refine_level
                  : item->GetAttr(ITEM_ATTR_REFINE_LEVEL);

          t_once_equip_min_level =
              t_once_equip_min_level < item->GetAttr(ITEM_ATTR_LEVEL)
                  ? t_once_equip_min_level
                  : item->GetAttr(ITEM_ATTR_LEVEL);
          t_once_equip_min_refine_level =
              t_once_equip_min_refine_level <
                      item->GetAttr(ITEM_ATTR_REFINE_LEVEL)
                  ? t_once_equip_min_refine_level
                  : item->GetAttr(ITEM_ATTR_REFINE_LEVEL);
        } else {
          equip_vacancy = true;
          t_equip_once_falg = false;
        }
      }
      for (int32_t i = 4; i < equips->size() && i < 6; ++i) {
        int64_t navy_uid = equips->Get(i);
        LogicItem* item = this->items_.GetItemByUniqueID(navy_uid);
        if (item) {
          navy_min_refine_level =
              navy_min_refine_level < item->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL)
                  ? navy_min_refine_level
                  : item->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL);
          navy_max_refine_level =
              navy_max_refine_level > item->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL)
                  ? navy_max_refine_level
                  : item->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL);

          t_once_navy_min_level =
              t_once_navy_min_level < item->GetAttr(ITEM_ATTR_NAVY_LEVEL)
                  ? t_once_navy_min_level
                  : item->GetAttr(ITEM_ATTR_NAVY_LEVEL);
          t_once_navy_min_refine_level =
              t_once_navy_min_refine_level <
                      item->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL)
                  ? t_once_navy_min_refine_level
                  : item->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL);
        } else {
          navy_vacancy = true;
          t_navy_once_flag = false;
        }
      }
      if (t_equip_once_falg) {
        equip_once_falg = true;
        once_equip_min_level = once_equip_min_level > t_once_equip_min_level
                                   ? once_equip_min_level
                                   : t_once_equip_min_level;
        once_equip_min_refine_level =
            once_equip_min_refine_level > t_once_equip_min_refine_level
                ? once_equip_min_refine_level
                : t_once_equip_min_refine_level;
      }
      if (t_navy_once_flag) {
        navy_once_flag = true;
        once_navy_min_level = once_navy_min_level > t_once_navy_min_level
                                  ? once_navy_min_level
                                  : t_once_navy_min_level;
        once_navy_min_refine_level =
            once_navy_min_refine_level > t_once_navy_min_refine_level
                ? once_navy_min_refine_level
                : t_once_navy_min_refine_level;
      }
    } else {
      navy_vacancy = true;
      equip_vacancy = true;
    }
  }

  ++equip_min_level;
  ++once_equip_min_level;
  ++once_navy_min_level;

  if (equip_max_refine_level >
      this->achievements_[SEVEN_DAY_TYPE_EQUIP_MAX_REFINE])
    UpdateAchievement(SEVEN_DAY_TYPE_EQUIP_MAX_REFINE, equip_max_refine_level);
  if (navy_max_refine_level >
      this->achievements_[SEVEN_DAY_TYPE_NAVY_MAX_REFINE])
    UpdateAchievement(SEVEN_DAY_TYPE_NAVY_MAX_REFINE, navy_max_refine_level);
  if (!equip_vacancy) {
    if (equip_min_quality > this->achievements_[SEVEN_DAY_TYPE_EQUIP_QUALITY])
      UpdateAchievement(SEVEN_DAY_TYPE_EQUIP_QUALITY, equip_min_quality);
    if (equip_min_level > this->achievements_[SEVEN_DAY_TYPE_EQUIP_LEVEL])
      UpdateAchievement(SEVEN_DAY_TYPE_EQUIP_LEVEL, equip_min_level);
    if (equip_min_refine_level >
        this->achievements_[SEVEN_DAY_TYPE_EQUIP_ALL_REFINE])
      UpdateAchievement(SEVEN_DAY_TYPE_EQUIP_ALL_REFINE,
                        equip_min_refine_level);

    ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_EQUIP_REFINE_MIN_LEVEL,
                                    equip_min_refine_level, this);
  }
  ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_EQUIP_REFINE_MAX_LEVEL,
                                  equip_max_refine_level, this);
  if (!navy_vacancy) {
    if (navy_min_refine_level >
        this->achievements_[SEVEN_DAY_TYPE_NAVY_ALL_REFINE])
      UpdateAchievement(SEVEN_DAY_TYPE_NAVY_ALL_REFINE, navy_min_refine_level);
    ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_NAVY_REFINE_MIN_LEVEL,
                                    navy_min_refine_level, this);
  }

  if (equip_once_falg) {
    if (once_equip_min_level >
        this->achievements_[ACHIEVEMENT_TYPE_COUNT_EQUIP_LEVEL])
      UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_EQUIP_LEVEL,
                        once_equip_min_level);
    if (once_equip_min_refine_level >
        this->achievements_[ACHIEVEMENT_TYPE_COUNT_EQUIP_REFINE_LEVEL])
      UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_EQUIP_REFINE_LEVEL,
                        once_equip_min_refine_level);
  }
  if (navy_once_flag) {
    if (once_navy_min_level >
        this->achievements_[ACHIEVEMENT_TYPE_COUNT_AEMY_LEVEL])
      UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_AEMY_LEVEL, once_navy_min_level);
    if (once_navy_min_refine_level >
        this->achievements_[ACHIEVEMENT_TYPE_COUNT_AEMY_REFINE_LEVEL])
      UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_AEMY_REFINE_LEVEL,
                        once_navy_min_refine_level);
  }
  ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_NAVY_REFINE_MAX_LEVEL,
                                  navy_max_refine_level, this);
}

//// CDType_ARRAYSIZE
//struct CallBackClearCD {
//  virtual int32_t Check(LogicPlayer* player, ModifyCurrency& money) const = 0;
//  virtual void ResetCD(LogicPlayer* player) const = 0;
//};
//
//CallBackClearCD* kClearCDCallBack[CDType_ARRAYSIZE] = {NULL, NULL};
//
//int32_t LogicPlayer::ProcessRequestClearCD(CSMessageEntry& entry) {
//  MessageRequestClearCD* message =
//      static_cast<MessageRequestClearCD*>(entry.get());
//  if (!message) return ERR_INTERNAL;
//  if (message->cd_type() >= ArraySize(kClearCDCallBack) ||
//      message->cd_type() < 0)
//    return ERR_PARAM_INVALID;
//  if (!kClearCDCallBack[message->cd_type()]) return ERR_INTERNAL;
//
//  ModifyCurrency modify(MSG_CS_REQUEST_CLEAR_CD, SYSTEM_ID_BASE);
//  int32_t result = kClearCDCallBack[message->cd_type()]->Check(this, modify);
//  if (result || modify.money >= 0) return result;
//  result = this->CheckCurrency(modify);
//  if (result) return result;
//
//  kClearCDCallBack[message->cd_type()]->ResetCD(this);
//  this->UpdateCurrency(modify);
//
//  MessageResponseClearCD response;
//  response.set_cd_type(message->cd_type());
//  this->SendMessageToClient(MSG_CS_RESPONSE_CLEAR_CD, &response);
//  return ERR_OK;
//}

int32_t LogicPlayer::CheckHero(const DeleteHeroSet* delete_set,
                               const NotifyHeroSet* notify_set) {
  if (delete_set) {
    VectorSet<int64_t> set;
    for (const int64_t* iter = delete_set->begin(); iter != delete_set->end();
         ++iter) {
      if (!this->GetHeroByUID(*iter)) return ERR_HERO_NOT_FOUND;
      if (!set.insert(*iter).second) return ERR_PARAM_INVALID;
      if (this->IsInTactic(*iter) || this->IsInSupportTactic(*iter))
        return ERR_HERO_ON_BATTLE;
    }
  }
  if (notify_set) {
    for (const int64_t* iter = notify_set->begin(); iter != notify_set->end();
         ++iter) {
      if (!this->GetHeroByUID(*iter)) return ERR_HERO_NOT_FOUND;
    }
  }
  return ERR_OK;
}

int32_t LogicPlayer::ObtainHero(const DeleteHeroSet* delete_set,
                                const NotifyHeroSet* notify_set, int32_t sys_id,
                                int32_t msg_id) {
  int32_t result = this->CheckHero(delete_set, notify_set);
  if (result) return result;

  MessageSSUpdateHeroInfo msg;
  MessageNotifyHeroInfo notify;
  msg.set_tid(server->GetTID());
  msg.set_msgid(msg_id);
  msg.set_system(sys_id);
  if (delete_set) {
    for (const int64_t* iter = delete_set->begin(); iter != delete_set->end();
         ++iter) {
      this->DeleteHeroByUID(*iter);
      msg.add_delete_items(*iter);
      notify.add_delete_hero(*iter);
    }
  }
  if (notify_set) {
    for (const int64_t* iter = notify_set->begin(); iter != notify_set->end();
         ++iter) {
      LogicHero* info = this->GetHeroByUID(*iter);
      if (!info) return ERR_HERO_NOT_FOUND;
      msg.add_info()->CopyFrom(info->first);
      notify.add_info()->CopyFrom(info->first);
    }
  }

  this->SendMessageToClient(MSG_CS_NOTIFY_HERO_INFO, &notify);
  this->SendMessageToDB(MSG_SS_UPDATE_HERO_INFO, &msg);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestHeroGradeLevelUp(CSMessageEntry& entry) {
  MessageRequestHeroGradeLevelUp* message =
      static_cast<MessageRequestHeroGradeLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicHero *hero = this->GetHeroByUID(message->hero_uid());
  if (!hero) return ERR_HERO_NOT_FOUND;
  const HeroBase* hero_base = HERO_BASE.GetEntryByID(hero->first.hero_id()).get();
  if (!hero_base) return ERR_HERO_NOT_FOUND;
  const BreakAdvancedBase* grade_base = BREAK_ADVANCED.GetEntryByID(
      BreakAdvancedID(hero_base->breakadvancedid, hero->first.grade())).get();
  if (!grade_base || grade_base->coin_cost <= 0) return ERR_ITEM_LEVEL_MAX;
  if (hero->first.level() < grade_base->need_hero_lv) return ERR_HERO_NEED_LEVEL_UP;

  AddSubItemSet sub_item;
  sub_item.push_back(ItemParam(GetSettingValue(shipreform_id), -grade_base->item_cost));

  ModifyCurrency modify(MSG_CS_REQUEST_HERO_GRADE_LEVEL_UP,
                        SYSTEM_ID_WAR_SHIP_REFORM);
  modify.coin = -grade_base->coin_cost;

  NotifyHeroSet notify_set;
  notify_set.push_back(hero->first.uid());
  DeleteHeroSet delete_heros;

  std::sort(message->mutable_other_uid()->begin(),
            message->mutable_other_uid()->end());
  int64_t* end = std::unique(message->mutable_other_uid()->begin(),
                             message->mutable_other_uid()->end());

  for (int64_t* iter = message->mutable_other_uid()->begin(); iter != end;
       ++iter) {
    LogicHero* other = this->GetHeroByUID(*iter);
    if (!other) return ERR_HERO_NOT_FOUND;
    if (other == hero) return ERR_PARAM_INVALID;
    if (other->first.hero_id() != hero_base->id()) return ERR_PARAM_INVALID;

    if (other->first.level() > 1) return ERR_PARAM_INVALID;
    if (other->first.fate_level() > 1) return ERR_PARAM_INVALID;
    if (other->first.train_cost() > 0) return ERR_PARAM_INVALID;
    if (other->first.grade() > 0) return ERR_PARAM_INVALID;
    if (other->first.wake_level() > 0) return ERR_PARAM_INVALID;

    if (delete_heros.full()) break;
    delete_heros.push_back(*iter);
  }
  if (int32_t(delete_heros.size()) != grade_base->hero_cost)
    return ERR_HERO_NEED_MORE;

  int32_t result = this->CheckItem(&sub_item, NULL, NULL);
  if (result) return result;
  result = this->CheckCurrency(modify);
  if (result) return result;
  result = this->CheckHero(&delete_heros, &notify_set);
  if (result) return result;

  hero->first.set_grade(hero->first.grade() + 1);
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_WAR_SHIP_REFORM, entry.head.msgid);
  this->ObtainItem(&sub_item, NULL, NULL, MSG_CS_REQUEST_HERO_GRADE_LEVEL_UP,
                   SYSTEM_ID_WAR_SHIP_REFORM);
  this->UpdateCurrency(modify);
  if (this->IsInTactic(hero->first.uid())) {
    this->ObtainHero(&delete_heros, NULL, SYSTEM_ID_WAR_SHIP_REFORM,
                     MSG_CS_REQUEST_HERO_GRADE_LEVEL_UP);
  } else {
    this->ObtainHero(&delete_heros, &notify_set, SYSTEM_ID_WAR_SHIP_REFORM,
                     MSG_CS_REQUEST_HERO_GRADE_LEVEL_UP);
  }

  MessageResponseHeroGradeLevelUp response;
  response.set_hero_uid(hero->first.uid());
  this->SendMessageToClient(MSG_CS_RESPONSE_HERO_GRADE_LEVEL_UP, &response);
  return ERR_OK;
}

int32_t LogicPlayer::IsInTactic(int64_t hero_uid) {
  for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
    if (this->tactic_.infos(i).hero_uid() == hero_uid) return i + 1;
  }
  return 0;
}

int32_t LogicPlayer::IsInSupportTactic(int64_t hero_uid) {
  for (int32_t i = 0; i < this->tactic_.support_pos_size(); ++i) {
    if (this->tactic_.support_pos(i).hero_uid() == hero_uid) return i + 1;
  }
  return 0;
}

void LogicPlayer::UpdateCopyInfo(
    int32_t copy_id, int32_t star, __OUT__ sy::MessageNotifyCopyInfo* notify,
    __OUT__ intranet::MessageSSUpdateCopyInfo* msg) {
  VectorMap<int32_t, int32_t>::const_iterator iter = this->copy_star_.find(copy_id);
  if (iter != this->copy_star_.end() && iter->second >= star) {
    return;
  }
  this->copy_star_[copy_id] = star;

  sy::CopyStarInfo info;
  info.set_star(star);
  info.set_copy_id(copy_id);
  if (notify) {
    notify->mutable_copy_star()->CopyFrom(info);
  }
  if (msg) {
    msg->add_copy_star()->CopyFrom(info);
  }
}

void LogicPlayer::UpdateCopyProgress(
    const sy::CopyProgress& progress, __OUT__ sy::MessageNotifyCopyInfo* notify,
    __OUT__ intranet::MessageSSUpdateCopyInfo* msg) {
  sy::CopyProgress *info = this->GetCopyProgress(progress.copy_type());
  if (info) {
    info->CopyFrom(progress);
  } else {
    this->copy_progress_.push_back(progress);
  }
  if (notify) {
    notify->mutable_progress()->CopyFrom(progress);
  }
  if (msg) {
    msg->mutable_copy()->CopyFrom(progress);
  }
}

void LogicPlayer::UpdatePassedCopy(
    int32_t copy_id, __OUT__ sy::MessageNotifyCopyInfo* notify,
    __OUT__ intranet::MessageSSUpdateCopyInfo* msg) {
  if (std::find(this->passed_copy_.begin(), this->passed_copy_.end(),
                copy_id) != this->passed_copy_.end()) {
    return;
  }
  this->passed_copy_.push_back(copy_id);
  if (notify) {
    notify->add_passed_copy(copy_id);
  }
  if (msg) {
    msg->add_passed_copy(copy_id);
  }
}

int32_t LogicPlayer::GetCopyCount(int32_t copy_id) const {
  VectorMap<int32_t, int32_t>::const_iterator iter = this->copy_count_.find(copy_id);
  return iter != this->copy_count_.end() ? iter->second : 0;
}

void LogicPlayer::UpdateCopyCount(
    int32_t copy_id, __OUT__ sy::MessageNotifyCopyInfo* notify,
    __OUT__ intranet::MessageSSUpdateCopyInfo* msg) {
  int32_t count = ++this->copy_count_[copy_id];
  if (notify) {
    sy::CopyCount* info = notify->add_copy_count();
    info->set_copy_id(copy_id);
    info->set_count(count);
  }
  if (msg) {
    sy::CopyCount* info = msg->add_copy_count();
    info->set_copy_id(copy_id);
    info->set_count(count);
  }
}

int32_t LogicPlayer::GetCurrentCopyProgress(int32_t copy_type) const {
  if (sy::COPY_TYPE_TOWER == copy_type) return this->tower_state_.max_order();
  sy::CopyProgress* progress = this->GetCopyProgress(copy_type);
  if (!progress) return 0;
  const CopyBase* ptr = COPY_BASE.GetEntryByID(progress->copy_id()).get();
  if (progress->copy_id() && !ptr) return -1;
  return ptr->GetOrder();
}

void FillFightResultMessage(MessageNotifyFightResult& notify,
                            AddSubItemSet& item_set, ModifyCurrency& modify) {
  notify.mutable_money()->Reserve(16);
  notify.mutable_item()->Reserve(16);
  for (int32_t i = MONEY_KIND_COIN; i <= MoneyKind_MAX; ++i) {
    int32_t value = modify[i];
    if (value > 0) {
      sy::MoneyType* info = notify.add_money();
      info->set_kind(i);
      info->set_value(value);
    }
  }
  for (int32_t i = 0; i < int32_t(item_set.size()); ++i) {
    if (item_set[i].item_count > 0) {
      sy::ItemSimpleInfo* info = notify.add_item();
      info->set_item_id(item_set[i].item_id);
      info->set_count(item_set[i].item_count);
    }
  }
}

int32_t LogicPlayer::OnDstrikeCopy(sy::DstrikeBoss& boss_info, const CopyBase* copy_base,
                                   std::vector<int64_t>& current_hp) {
  MessageNotifyDstrikeBossFightResult notify;

  //BOSS扣血
  int64_t total_damage = 0;
  std::vector<int64_t> sub_hp;
  sub_hp.resize(6, 0);
  for (int32_t i = 0; i < 6; ++i) {
    sub_hp[i] = boss_info.mutable_boss_blood()->Get(i) - current_hp[i];
    if (sub_hp[i] > 0) total_damage += sub_hp[i];
  }

  const ValuePair2Vec& config = Setting::GetValue2(Setting::dstrike_activity_time);
  bool cost_by_half = false;
  int32_t income_double = 1;
  //消耗减半
  if (config.size() >= 1u && server->active_tm().tm_hour >= config[0].v1 &&
      server->active_tm().tm_hour < config[0].v2) {
    cost_by_half = true;
  }
  //收益翻倍
  if (config.size() >= 2u && server->active_tm().tm_hour >= config[1].v1 &&
      server->active_tm().tm_hour < config[1].v2) {
    income_double = 2;
  }
  INFO_LOG(logger)("PlayerID:%ld, Damage:%ld, HalfCost:%d, DoubleIncome:%d" ,
      this->uid(), total_damage, cost_by_half, income_double);
  ModifyCurrency modify(MSG_CS_REQUEST_FIGHT,
                        SYSTEM_ID_DSTRIKE_MAIN);
  AddSubItemSet item_set;

  //扣道具
  int32_t sub_item_count =
      GetSettingValue(dstrike_special_id) == copy_base->id() ? 2 : 1;
  sub_item_count = cost_by_half ? sub_item_count / 2 : sub_item_count;
  if (!sub_item_count) sub_item_count = 1;
  item_set.push_back(ItemParam(Setting::kDstrikeTokenID, -sub_item_count));
  //加功勋
  modify[MONEY_KIND_EXPLOIT] = GetSettingValue(dstrike_token_reward) * abs(sub_item_count);
  notify.set_exploit(modify[MONEY_KIND_EXPLOIT]);
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_FIGHT,
                   SYSTEM_ID_DSTRIKE_MAIN);
  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_DESTRIKE, 1, this);

  //最后一击
  if (1 == server->SubDstrikeBossHP(boss_info.player_id(), sub_hp)) {
    int32_t kill_money = Setting::GetValueInVec2(Setting::dstrike_kill_award, boss_info.boss_quality());
    int32_t find_money = Setting::GetValueInVec2(Setting::dstrike_find_award, boss_info.boss_quality());
    //发现BOSS奖励
    DefaultArrayStream stream;
    std::vector<std::pair<int32_t, int32_t> > award;
    award.push_back(std::pair<int32_t, int32_t>(MONEY_KIND_MONEY, find_money));
    stream.Append("%d,%ld,%s", boss_info.boss_id(), this->uid(), this->name().c_str());
    this->SendMail(boss_info.player_id(), GetSeconds(),
                   MAIL_TYPE_DESTRIKE_DISCOVERY, stream.str(), &award);
    //最后一击奖励
    stream.clear();
    award.clear();
    award.push_back(std::pair<int32_t, int32_t>(MONEY_KIND_MONEY, kill_money));
    stream.Append("%d", boss_info.boss_id());
    this->SendMail(this->uid(), GetSeconds(),
                   MAIL_TYPE_DESTRIKE, stream.str(), &award);
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_DESTRIKE, 1, this);
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_KILL_DESTRIKE, 1,
                                    this);

    //增加boss的等级
    LogicPlayer* player = server->GetPlayerByID(boss_info.player_id());
    if (player && player->load_complete()) {
      player->dstrike_info_.set_level(player->dstrike_info_.level() + 1);
      player->UpdateDstrikeInfo(true);
    } else {
      MessageSSUpdateOtherDstrikeBoss request;
      request.set_player_id(boss_info.player_id());
      this->SendMessageToDB(MSG_SS_UPDATE_OTHER_DSTRIKE_BOSS, &request);
    }
  }

  UpdateBuyCount(COUNT_TYPE_DAILY_DSTRIKE, 1);
  UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_DSTRIKE, this->achievements_[ACHIEVEMENT_TYPE_COUNT_DSTRIKE] + 1);

  notify.set_damage(total_damage);
  notify.set_merit(total_damage / 1000 * income_double);
  DEBUG_LOG(logger)("PlayerID:%ld, DstrikeBoss Damage:%ld, Merit:%ld"
      , this->uid(), notify.damage(), notify.merit());

  //伤害提升
  if (this->dstrike_info_.damage() < notify.damage()) {
    this->dstrike_info_.set_damage(notify.damage());
    if (this->dstrike_info_.damage() / 100000 >
        this->achievements_[SEVEN_DAY_TYPE_DSTRIKE_DAMAGE])
      UpdateAchievement(SEVEN_DAY_TYPE_DSTRIKE_DAMAGE,
                        this->dstrike_info_.damage() / 100000);
  }
  //伤害排行榜
  notify.set_damage_rank(RANK_LIST.GetByType(sy::RANK_TYPE_DSTRIKE_DAMAGE).GetRankByUID(this->uid()));
  RANK_LIST.OnDstrikeDamage(this);
  notify.set_damage_new_rank(RANK_LIST.GetByType(sy::RANK_TYPE_DSTRIKE_DAMAGE).GetRankByUID(this->uid()));
  //功勋排行榜
  notify.set_exploit_rank(RANK_LIST.GetByType(sy::RANK_TYPE_DSTRIKE_EXPLOIT).GetRankByUID(this->uid()));
  this->dstrike_info_.set_merit(notify.damage() / 1000 * income_double + this->dstrike_info_.merit());
  RANK_LIST.OnDstrikeExploit(this);
  notify.set_exploit_new_rank(RANK_LIST.GetByType(sy::RANK_TYPE_DSTRIKE_EXPLOIT).GetRankByUID(this->uid()));
  this->UpdateDstrikeInfo(true);

  if (this->dstrike_info_.merit() / 10000 >
      this->achievements_[SEVEN_DAY_TYPE_DSTRIKE_EXPLOIT])
    UpdateAchievement(SEVEN_DAY_TYPE_DSTRIKE_EXPLOIT,
                      this->dstrike_info_.merit() / 10000);
  this->SendMessageToClient(MSG_CS_NOTIFY_DSTRIKE_FIGHT_RESULT, &notify);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestFight(CSMessageEntry& entry) {
  MessageRequestFight* message = static_cast<MessageRequestFight*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->has_copy_id()) {
    DEBUG_LOG(logger)("PlayerID:%ld, RequestFight %d", this->uid(), message->copy_id());

    const CopyBase* copy_base = COPY_BASE.GetEntryByID(message->copy_id()).get();
    if (!copy_base) return ERR_COPY_INVALID;
    int32_t result = this->CheckEnterCopy(copy_base);
    if (result) return result;
    PK pk(copy_base);

    //爬塔属性
    if (copy_base->type == COPY_TYPE_TOWER) {
      pk.InitSceneAttr(this->current_carrier_.mutable_tower_attr1());
    }
    int32_t order = this->GetCurrentCopyProgress(copy_base->type);
    if (copy_base->GetOrder() <= order) {
      pk.DisableHelpers();
    }
    pk.GeneratePVEReport(this);
    if (server_config->report()) {
      LogReportToFile(pk.report);
    }

    int32_t star = pk.star;
    int32_t win = star > 0;
    int32_t round = pk.round;
    int32_t first_blood = 0;
    MessageNotifyFightResult notify;

    ModifyCurrency modify(MSG_CS_REQUEST_FIGHT, SYSTEM_ID_COPY);
    AddSubItemSet item_set;
    this->FillCopyAward(win, copy_base, modify, item_set, first_blood, notify,
                        false);
    result = this->CheckCurrency(modify);
    if (result) return result;
    result = this->CheckItem(&item_set, NULL, NULL);
    if (result) return result;

    notify.set_win(win);
    notify.set_round(round);
    notify.set_star(star);
    notify.set_copy_id(copy_base->id());
    //奖励
    this->UpdateCurrency(modify);
    this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_FIGHT,
                     SYSTEM_ID_COPY);
    //副本次数等
    this->OnCopyPassed(star, first_blood, copy_base);
    //战斗结算信息
    this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
    //发送战报
    this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
    //发送统计信息
    this->SendCopyStatus(copy_base, star);
    server->UpdateCopyStatistics(copy_base->type, copy_base->id(), this->uid());
    return ERR_OK;
  }

  if (message->player_id() && message->item_id()) {
    return OnRobProcess(message->player_id(), message->item_id());
  }
  return ERR_OK;
}

int32_t LogicPlayer::OnRobProcess(int64_t player_id, int32_t item_id) {
  const CopyBase* copy_base = COPY_BASE.GetEntryByID(62600).get();
  if (!copy_base) return ERR_COPY_INVALID;
  ItemBase* item_base = ITEM_BASE.GetEntryByID(item_id).get();
  if (!item_base) return ERR_ITEM_NOT_FOUND;
  ArmyBase* army_base = ArmyBase::GetArmyBaseByArmyItem(item_id);
  if (!army_base) return ERR_ITEM_NOT_FOUND;

  bool is_rob_success = false;

  if(this->player_.energy() < GetSettingValue(army_grab_expend))
    return ERR_CURRENCY_ENERGY;

  AddSubItemSet item_set;

  PK rob_pk(copy_base);
  rob_pk.InitPlayer(this, true);

  if (player_id > sy::MAX_ROBOT_ID) {
    if (!item_base->can_lost) return ERR_ITEM_INVALID_PARAM;
    LogicPlayer* other_player = server->GetPlayerByID(player_id);
    if (!other_player) return ERR_PLAYER_NOT_EXIST;
    if (abs(other_player->level() - this->level()) > 5)
      return ERR_PARAM_INVALID;
    if (other_player->player_.truce_time() > GetVirtualSeconds())
      return ERR_PARAM_INVALID;

    SetTruceTime(0);

    rob_pk.InitPlayer(other_player, false);
    rob_pk.GeneratePVPReport();

    int32_t rob_result = IsRobSuccess(army_base, true);
    if (rob_pk.star > 0 && rob_result) {
      is_rob_success = true;
      if (rob_result == 1) {
        item_set.push_back(ItemParam(item_id, -1));
        CHECK_RET(other_player->CheckItem(&item_set, NULL, NULL));
        other_player->ObtainItem(&item_set, NULL, NULL,
                                 MSG_CS_REQUEST_FIGHT_REPORT, SYSTEM_ID_ROB);
        DefaultArrayStream stream;
        stream.Append("%d,%ld,%s", item_id, this->uid(),
                      this->player_.name().c_str());
        this->SendMail(other_player->uid(), GetSeconds(), sy::MAIL_TYPE_NAVY,
                       stream.str(), NULL);
      }
    }
  } else {
    const std::vector<int32_t>& monsters =
        Setting::GetValue1(Setting::duobao_monstergroup);
    MonsterGroupBase* monster =
        MONSTER_GROUP_BASE.GetEntryByID(
                              monsters[RandomIn10000() % monsters.size()])
            .get();
    if (!monster) return ERR_INTERNAL;

    rob_pk.InitMonsterGroup(player_id, monster, false);
    rob_pk.GeneratePVPReport();

    if (rob_pk.star > 0 && IsRobSuccess(army_base, false)) {
      is_rob_success = true;
    }
  }

  if (is_rob_success) {
    item_set.clear();
    item_set.push_back(ItemParam(item_id, 1));
  }

  this->UpdateAchievement(
      ACHIEVEMENT_TYPE_COUNT_ROB_NAVY,
      this->achievements_[ACHIEVEMENT_TYPE_COUNT_ROB_NAVY] + 1);
  this->UpdateAchievement(FOURTEEN_DAY_TYPE_ROB_ARMY1,
                          this->achievements_[FOURTEEN_DAY_TYPE_ROB_ARMY1] + 1);
  this->UpdateAchievement(FOURTEEN_DAY_TYPE_ROB_ARMY2,
                          this->achievements_[FOURTEEN_DAY_TYPE_ROB_ARMY2] + 1);
  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_ROB, 1, this);

  int32_t win = rob_pk.star ? 1 : 0;
  int32_t first_blood = 0;
  MessageNotifyFightResult notify;
  ModifyCurrency modify(MSG_CS_REQUEST_FIGHT, SYSTEM_ID_ROB);
  this->FillCopyAward(win, copy_base, modify, item_set, first_blood, notify,
                      true);

  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  CHECK_RET(this->CheckCurrency(modify));
  UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_FIGHT_REPORT,
                   SYSTEM_ID_ROB);

  notify.set_win(win);
  notify.set_round(rob_pk.round);
  notify.set_star(rob_pk.star);
  notify.set_copy_id(62600);
  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &rob_pk.response);

  return ERR_OK;
}

void LogicPlayer::SetTruceTime(int64_t time) {
  this->player_.set_truce_time(time);
  MessageSSUpdateTruceTime db_msg;
  db_msg.set_truce_time(this->player_.truce_time());
  SendMessageToDB(MSG_SS_UPDATE_TRUCE_TIME, &db_msg);
  MessageResponseUseTruce response;
  response.set_truce_until_time(this->player_.truce_time());
  SendMessageToClient(MSG_CS_RESPONSE_USE_TRUCE, &response);
}

int32_t LogicPlayer::ProcessRequestFlop(CSMessageEntry& entry) {
  MessageRequestFlop* message = static_cast<MessageRequestFlop*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->flop_type() != 1 && message->flop_type() != 2)
    return ERR_PARAM_INVALID;

  if (message->flop_type() == 1 && !this->flop_1_) return ERR_PARAM_INVALID;
  if (message->flop_type() == 2 && (!this->flop_2_)) return ERR_PARAM_INVALID;

  MessageResponseFlop response;

  std::vector<int32_t> index;
  ModifyCurrency modify(MSG_CS_REQUEST_FLOP, SYSTEM_ID_BASE);
  AddSubItemSet item_set;

  if (message->flop_type() == 1) {
    this->flop_1_->Loot(modify, item_set, &index);
    response.set_flop_id(this->flop_1_->id());
  }

  if (message->flop_type() == 2) {
    int32_t count = message->has_count() ? message->count() : 1;
    for (int32_t i = 0; i < count; ++i) {
      this->flop_2_->Loot(modify, item_set, &index);
      this->flop_2_count_++;
      //抽卡花费
      modify.money += -GetSettingValue(card_special_cost);
      if (this->flop_2_count_ >= GetSettingValue(card_special_limit)) {
        break;
      }
    }
    response.set_flop_id(this->flop_2_->id());
  }
  const ValuePair3Vec& loot =
      message->flop_type() == 1 ? this->flop_1_->info : this->flop_2_->info;
  for (size_t i = 0; i < index.size(); ++i) {
    if (index[i] < 0 || index[i] >= (int32_t)loot.size()) return ERR_PARAM_INVALID;
    response.add_index(index[i]);
    sy::ItemSimpleInfo* info = response.add_items();
    info->set_item_id(loot[index[i]].v1);
    info->set_count(loot[index[i]].v2);
  }

  int32_t result = this->CheckCurrency(modify);
  if (result) return result;
  result = this->CheckItem(&item_set, NULL, NULL);
  if (result) return result;

  if (message->flop_type() == 1) this->flop_1_.reset();
  if (message->flop_type() == 2 &&
      this->flop_2_count_ >= GetSettingValue(card_special_limit)) {
    this->flop_2_.reset();
  }
  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_FLOP, SYSTEM_ID_BASE);
  this->SendMessageToClient(MSG_CS_RESPONSE_FLOP, &response);
  return ERR_OK;
}

bool UnSetEquipedItem(RepeatedField<int64_t>* equips, int64_t item_uid) {
  for (int i = 0; i < equips->size(); ++i) {
    if (equips->Get(i) == item_uid) {
      equips->Set(i, 0);
      return true;
    }
  }
  return false;
}

int32_t LogicPlayer::ProcessRequestEquipItem(CSMessageEntry& entry) {
  MessageRequestEquipItem* message =
      static_cast<MessageRequestEquipItem*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->position() < 0 || message->position() > 6) return ERR_PARAM_INVALID;

  int32_t is_wear = message->has_is_wear() ? message->is_wear() : 0;
  LogicItem* item = this->items().GetItemByUniqueID(message->item_uid());
  if (!item) return ERR_ITEM_NOT_FOUND;

  //老道具如果装在其他船上,必须先卸载下来
  int32_t equiped_pos = this->GetItemEquipedPos(item);
  if (is_wear && equiped_pos) return ERR_PARAM_INVALID;

  RepeatedField<int64_t>* equips_info = this->GetEquipsInfo(message->position());
  if (!equips_info) return ERR_INTERNAL;
  equips_info->Resize(MAX_EQUIPED_ITEM_COUNT, 0);

  NotifyItemSet notify_set;
  if (is_wear) {
    //[1, 6)
    int32_t slot = Setting::GetValueInVec2(Setting::equip_position, item->equip_type()) - 1;
    if (slot < 0 || slot > 5) return ERR_PARAM_INVALID;
    LogicItem* old_equip = this->items().GetItemByUniqueID(equips_info->Get(slot));
    if (old_equip) {
      old_equip->SetAttr(sy::ITEM_ATTR_EQUIPED_HERO, 0);
      old_equip->SetAttr(sy::ITEM_ATTR_EQUIPED_POS, 0);
      notify_set.push_back(old_equip->uid());
    }
    equips_info->Set(slot, item->uid());
    item->SetAttr(sy::ITEM_ATTR_EQUIPED_POS, message->position());
  } else {
    if (!UnSetEquipedItem(equips_info, item->uid())) return ERR_PARAM_INVALID;
    item->SetAttr(sy::ITEM_ATTR_EQUIPED_POS, 0);
  }
  notify_set.push_back(item->uid());

  this->ReCalcRelation();
  this->OnEquipChanged(0);
  this->ObtainItem(NULL, NULL, &notify_set, MSG_CS_REQUEST_EQUIP_ITEM,
                   SYSTEM_ID_BASE);
  this->UpdateEquipsInfo(message->position());

  MessageResponseEquipItem response;
  response.set_position(message->position());
  this->SendMessageToClient(MSG_CS_RESPONSE_EQUIP_ITEM, &response);
  return ERR_OK;
}

void LogicPlayer::UpdateEquipsInfo(int32_t pos) {
  RepeatedField<int64_t>* equips_info = this->GetEquipsInfo(pos);

  MessageSSUpdateEquipInfo update;
  update.mutable_equips()->CopyFrom(*equips_info);
  update.set_position(pos);
  this->SendMessageToDB(MSG_SS_UPDATE_EQUIP_INFO, &update);

  MessageNotifyEquipInfo notify;
  notify.set_position(pos);
  notify.mutable_equips()->CopyFrom(*equips_info);
  this->SendMessageToClient(MSG_CS_NOTIFY_EQUIP_INFO, &notify);
}

sy::CopyProgress* LogicPlayer::GetCopyProgress(int32_t type) const {
  for (std::vector<sy::CopyProgress>::const_iterator iter =
           this->copy_progress_.begin();
       iter != this->copy_progress_.end(); ++iter) {
    if (iter->copy_type() == type) {
      return const_cast<sy::CopyProgress*>(&*iter);
    }
  }
  return NULL;
}

RepeatedField<int64_t>* LogicPlayer::GetEquipsInfo(int32_t pos) {
  switch (pos) {
    case 1: return this->equips_.mutable_equips_1();
    case 2: return this->equips_.mutable_equips_2();
    case 3: return this->equips_.mutable_equips_3();
    case 4: return this->equips_.mutable_equips_4();
    case 5: return this->equips_.mutable_equips_5();
    case 6: return this->equips_.mutable_equips_6();
  }
  return NULL;
}

int32_t LogicPlayer::GetItemEquipedPos(LogicItem* item) {
  if (!item) return 0;

  for (int pos = 1; pos <= 6; ++pos) {
    RepeatedField<int64_t>* info = this->GetEquipsInfo(pos);
    if (!info) return 0;
    for (int32_t i = 0; i < info->size(); ++i) {
      if (info->Get(i) == item->uid()) return pos;
    }
  }
  return 0;
}

int32_t LogicPlayer::GetCopyChapterStarCount(int32_t chapter) {
  const CopyChapterBase* chapter_base = COPY_CHAPTER_BASE.GetEntryByID(chapter).get();
  int32_t star = 0;
  for (std::vector<CopyGateBasePtr>::const_iterator iter =
           chapter_base->levels.begin();
       iter != chapter_base->levels.end(); ++iter) {
    for (std::vector<CopyBasePtr>::const_iterator iter_copy =
             (*iter)->copys.begin();
         iter_copy != (*iter)->copys.end(); ++iter_copy) {
      int32_t copy_star = this->GetCopyStar((*iter_copy)->id());
      star += copy_star;
    }
  }
  return star;
}

int32_t LogicPlayer::ProcessRequestGetCopyAward(CSMessageEntry& entry) {
  MessageRequestGetCopyAward* message =
      static_cast<MessageRequestGetCopyAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t chapter = message->has_chapter() ? message->chapter() : 0;
  int32_t chapter_index = message->has_chapter_index() ? message->chapter_index() : 0;
  int32_t copy_id = message->has_gate_id() ? message->gate_id() : 0;

  AddSubItemSet item_set;
  ModifyCurrency modify(MSG_CS_REQUEST_GET_COPY_AWARD, SYSTEM_ID_COPY);
  if (copy_id) {
    if (std::find(this->gate_award_.begin(), this->gate_award_.end(), copy_id) !=
        this->gate_award_.end())
      return ERR_COPY_AWARDED;

    const CopyBase* base = COPY_BASE.GetEntryByID(copy_id).get();
    if (!base) return ERR_PARAM_INVALID;
    int32_t order = this->GetCurrentCopyProgress(base->type);
    if (base->order > order) return ERR_PARAM_INVALID;
    const LootBasePtr& loot = LootConfigFile::Get(base->copy_box, this->level());
    if (loot) loot->Loot(modify, item_set, NULL);
    else return ERR_PARAM_INVALID;
  } else {
    if (this->chapter_award_[chapter] & (1 << chapter_index))
      return ERR_COPY_AWARDED;

    const CopyChapterBase* chapter_base = COPY_CHAPTER_BASE.GetEntryByID(chapter).get();
    if (!chapter_base) return ERR_PARAM_INVALID;
    if (chapter_index < 0 || chapter_index >= int32_t(chapter_base->star1.size()))
      return ERR_PARAM_INVALID;
    int32_t need_star = (chapter_base->star1.begin() + chapter_index)->v1;
    int32_t box_id = (chapter_base->star1.begin() + chapter_index)->v2;
    const LootBasePtr& loot_base = LootConfigFile::Get(box_id, this->level());
    int32_t star = this->GetCopyChapterStarCount(chapter);;

    if (star < need_star) return ERR_PARAM_INVALID;
    if (loot_base) loot_base->Loot(modify, item_set, NULL);
  }

  int32_t result = this->CheckCurrency(modify);
  if (result) return result;
  result = this->CheckItem(&item_set, NULL, NULL);
  if (result) return result;

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_COPY_AWARD,
                   SYSTEM_ID_COPY);

  MessageResponseGetCopyAward response;
  MessageSSUpdateCopyInfo update;
  if (copy_id) {
    this->gate_award_.push_back(copy_id);
    response.set_gate(copy_id);
    update.set_gate_award(copy_id);
  } else {
    this->chapter_award_[chapter] |= (1 << chapter_index);
    sy::ChapterAwardInfo* info = update.mutable_chapter_award();
    info->set_chapter(chapter);
    info->set_mask(this->chapter_award_[chapter]);
    response.mutable_chapter()->CopyFrom(*info);
  }
  this->SendMessageToDB(MSG_SS_UPDATE_COPY_INFO, &update);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_COPY_AWARD, &response);
  return ERR_OK;
}

static inline int32_t HeroRandomAttr(const TrainBase* train_base, int32_t attr_max, int32_t attr) {
  if (attr_max == attr) return 0;
  int32_t random_1 = RandomIn10000();
  int32_t random_2 = RandomIn10000();
  int32_t num = double(attr_max) * train_base->grow / 10000 * random_1 / 10000;
  if (!attr) return num;
  float success = float(random_2) / 100 / 100 - float(attr) / attr_max / 2 -
                  0.5 + float(train_base->train) / 10000;
  return success > 0 ? num : -num;
}

int32_t LogicPlayer::ProcessRequestHeroRandomAttr(CSMessageEntry& entry) {
  MessageRequestHeroRandomAttr* message =
      static_cast<MessageRequestHeroRandomAttr*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t train_type = message->type();
  int32_t train_count = message->count();
  if (train_count <= 0 || train_count > 10) return ERR_PARAM_INVALID;
  if (train_type < 1 || train_type > 3) return ERR_PARAM_INVALID;

  LogicHero* logic_hero = this->GetHeroByUID(message->hero_uid());
  if (!logic_hero) return ERR_HERO_NOT_FOUND;
  sy::HeroInfo *hero = &logic_hero->first;
  const HeroBase* base = logic_hero->second.get();
  if (!base) return ERR_HERO_NOT_FOUND;

  const int32_t kAttrCount = 4;
  int32_t attr[kAttrCount] = {0};
  int32_t attr_back[kAttrCount] = {0};
  if (hero->rand_attr_size() < kAttrCount) hero->mutable_rand_attr()->Resize(kAttrCount, 0);
  if (hero->rand_attr_1_size() < kAttrCount) hero->mutable_rand_attr_1()->Resize(kAttrCount, 0);
  for (int32_t i = 0; i < kAttrCount; ++i) {
    attr[i] = hero->rand_attr(i);
  }

  AddSubItemSet item_set;
  ModifyCurrency modify(MSG_CS_REQUEST_HERO_RANDOM_ATTR,
                        SYSTEM_ID_WAR_SHIP_FIRM);
  const TrainBase* train_base = TRAIN_BASE.GetEntryByID(train_type).get();
  if (!train_base) return ERR_PARAM_INVALID;
  modify[train_base->cost_money.v1] += -train_base->cost_money.v2 * train_count;
  item_set.push_back(ItemParam(GetSettingValue(train_id),
                               -train_base->cost_item * train_count));

  int32_t result = this->CheckCurrency(modify);
  if (result) return result;
  result = this->CheckItem(&item_set, NULL, NULL);
  if (result) return result;
  const TrainLimitBase* limit_base =
      TRAIN_LIMIT_BASE.GetEntryByID(base->train_id * 100 + base->quality).get();
  if (!limit_base) return ERR_PARAM_INVALID;

  int32_t attr_max[kAttrCount] = {0};
  limit_base->GetLimit(hero->level(), attr_max);
  for (int32_t i = 0; i < train_count; ++i) {
    //每次洗出来的属性
    int32_t attr_temp[kAttrCount] = {0};
    attr_temp[0] = HeroRandomAttr(train_base, attr_max[0], attr[0] + attr_back[0]);
    attr_temp[1] = HeroRandomAttr(train_base, attr_max[1], attr[1] + attr_back[1]);
    attr_temp[2] = HeroRandomAttr(train_base, attr_max[2], attr[2] + attr_back[2]);
    attr_temp[3] = HeroRandomAttr(train_base, attr_max[3], attr[3] + attr_back[3]);
    DEBUG_LOG(logger)("PlayerID:%ld, HeroID:%ld, RandomHeroAttr, %d,%d,%d,%d"
        , this->uid(), hero->uid()
        , attr_temp[0], attr_temp[1], attr_temp[2], attr_temp[3]);
    //计算战斗力大小, 战斗力大于0的, 才累加上去
    if (train_count > 1) {
      //增加的战斗力(战斗力的100倍)
      int32_t score = attr_temp[0] / 4 +
                      attr_temp[1] * 3 +
                      attr_temp[2] * 5 +
                      attr_temp[3] * 5;
      //战斗力是负数
      if (score <= 0) continue;
    }

    attr_back[0] += attr_temp[0];
    attr_back[1] += attr_temp[1];
    attr_back[2] += attr_temp[2];
    attr_back[3] += attr_temp[3];
  }

  for (int32_t i = 0; i < kAttrCount; ++i) {
    int32_t dest = attr[i] + attr_back[i];
    if (dest < 0) attr_back[i] = -attr[i];
    if (dest > attr_max[i]) attr_back[i] = attr_max[i] - attr[i];
    hero->mutable_rand_attr_1()->Set(i, attr_back[i]);
  }

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_HERO_RANDOM_ATTR,
                   SYSTEM_ID_WAR_SHIP_FIRM);
  hero->set_train_cost(hero->train_cost() +
                       train_base->cost_item * train_count);
  this->AddHeroExp(logic_hero, 0);

  UpdateAchievement(
      SEVEN_DAY_TYPE_TRAIN_GOLD,
      this->achievements_[SEVEN_DAY_TYPE_TRAIN_GOLD] + train_count);
  UpdateBuyCount(COUNT_TYPE_DAILY_SHIP_UP, train_count);

  MessageResponseHeroRandomAttr response;
  response.set_hero_uid(logic_hero->first.uid());
  this->SendMessageToClient(MSG_CS_RESPONSE_HERO_RANDOM_ATTR, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSaveRandomAttr(CSMessageEntry& entry) {
  MessageRequestSaveRandomAttr* message =
      static_cast<MessageRequestSaveRandomAttr*>(entry.get());
  if (!message) return ERR_INTERNAL;
  LogicHero* logic_hero = this->GetHeroByUID(message->hero_uid());
  if (!logic_hero) return ERR_HERO_NOT_FOUND;
  sy::HeroInfo* hero = &logic_hero->first;

  const int32_t kAttrCount = 4;
  if (hero->rand_attr_size() < kAttrCount) hero->mutable_rand_attr()->Resize(kAttrCount, 0);
  if (hero->rand_attr_1_size() < kAttrCount) hero->mutable_rand_attr_1()->Resize(kAttrCount, 0);
  for (int32_t i = 0; i < kAttrCount; ++i) {
    hero->mutable_rand_attr()->Set(i, hero->rand_attr(i) + hero->rand_attr_1(i));
    hero->mutable_rand_attr_1()->Set(i, 0);
  }

  this->AddHeroExp(logic_hero, 0);
  if (this->IsInTactic(hero->uid())) {
    this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_WAR_SHIP_FIRM, entry.head.msgid);
  }
  MessageResponseSaveRandomAttr response;
  response.set_hero_uid(hero->uid());
  this->SendMessageToClient(MSG_CS_RESPONSE_SAVE_RANDOM_ATTR, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestComposeNavy(CSMessageEntry& entry) {
  MessageRequestComposeNavy* message =
      static_cast<MessageRequestComposeNavy*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageResponseComposeNavy response;
  const ArmyBase* base = ARMY_BASE.GetEntryByID(message->navy_id()).get();
  if (!base) return ERR_ITEM_NOT_FOUND;

  ModifyCurrency modify(MSG_CS_REQUEST_COMPOSE_NAVY, SYSTEM_ID_COMPOUND);
  AddSubItemSet item_set;
  NotifyItemSet notify_set;

  int32_t count = 1;
  if (message->count() > 0) count = message->count();
  if (message->count() > 100) count = 100;

  for (int32_t i = 0; i < count; i++)
    FillCurrencyAndItem<kSub>(modify, item_set, base->soldier);

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, &notify_set));

  for (int32_t i = 0; i < count; i++) {
    LogicItem* navy = this->AddItem(message->navy_id(), 1, NULL);
    if (!navy) return ERR_INTERNAL;
    notify_set.push_back(navy->uid());
    response.add_navy_uid(navy->uid());
  }

  this->ObtainItem(&item_set, NULL, &notify_set, MSG_CS_REQUEST_COMPOSE_NAVY,
                   SYSTEM_ID_COMPOUND);
  this->UpdateCurrency(modify);

  UpdateBuyCount(COUNT_TYPE_DAILY_COMPOSE_NAVY, count);

  if (base->quality == 4) {
    UpdateAchievement(
        SEVEN_DAY_TYPE_NAVY_PURPLE_COUNT,
        this->achievements_[SEVEN_DAY_TYPE_NAVY_PURPLE_COUNT] + count);
    UpdateAchievement(
        ACHIEVEMENT_TYPE_NAVY_PURPLE_COUNT,
        this->achievements_[ACHIEVEMENT_TYPE_NAVY_PURPLE_COUNT] + count);
  } else if (base->quality == 5) {
    UpdateAchievement(
        SEVEN_DAY_TYPE_NAVY_ORANGE_COUNT,
        this->achievements_[SEVEN_DAY_TYPE_NAVY_ORANGE_COUNT] + count);
    UpdateAchievement(
        ACHIEVEMENT_TYPE_NAVY_ORANGE_COUNT,
        this->achievements_[ACHIEVEMENT_TYPE_NAVY_ORANGE_COUNT] + count);
  }
  UpdateAchievement(SEVEN_DAY_TYPE_NAVY_COUNT,
                    this->achievements_[SEVEN_DAY_TYPE_NAVY_COUNT] + count);
  UpdateAchievement(ACHIEVEMENT_TYPE_NAVY_COUNT,
                    this->achievements_[ACHIEVEMENT_TYPE_NAVY_COUNT] + count);

  this->SendMessageToClient(MSG_CS_RESPONSE_COMPOSE_NAVY, &response);
  return ERR_OK;
}

int32_t LogicPlayer::AddNavyExp(LogicItem* navy, int32_t exp) {
  if (!navy || !navy->army_base()) return 0;
  const ArmyBase* base = navy->army_base();

  int64_t current_exp = navy->GetAttr(sy::ITEM_ATTR_NAVY_EXP) + exp;
  int32_t current_level = navy->GetAttr(sy::ITEM_ATTR_NAVY_LEVEL);
  int32_t level_changed = 0;
  while (true) {
    int32_t need_exp = base->GetLevelUpExp(current_level);
    if (need_exp <= 0 || current_exp < need_exp) {
      break;
    }
    current_exp -= need_exp;
    current_level += 1;
    level_changed += 1;
  }
  navy->SetAttr(sy::ITEM_ATTR_NAVY_EXP, current_exp);
  navy->SetAttr(sy::ITEM_ATTR_NAVY_LEVEL, current_level);
  return level_changed;
}

int32_t LogicPlayer::ProcessRequestNavyLevelUp(CSMessageEntry& entry) {
  MessageRequestNavyLevelUp* message =
      static_cast<MessageRequestNavyLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicItem* navy = this->items_.GetItemByUniqueID(message->navy_uid());
  if (!navy) return ERR_ITEM_NOT_FOUND;

  NotifyItemSet notify_set;
  notify_set.push_back(navy->uid());
  DeleteItemSet delete_set;
  int32_t add_exp = 0;

  std::sort(message->mutable_other_navy()->begin(), message->mutable_other_navy()->end());
  int64_t* end = std::unique(message->mutable_other_navy()->begin(),
                             message->mutable_other_navy()->end());
  for (int64_t* iter = message->mutable_other_navy()->begin(); iter != end;
       ++iter) {
    LogicItem* info = this->items().GetItemByUniqueID(*iter);
    if (GetItemEquipedPos(info)) return ERR_PARAM_INVALID;
    if (!info || !info->army_base()) return ERR_ITEM_NOT_FOUND;
    if (delete_set.full()) break;
    if (navy == info) continue;

    add_exp += info->army_base()->exp;
    add_exp += info->GetAttr(sy::ITEM_ATTR_NAVY_EXP);
    delete_set.push_back(info->uid());
  }

  ModifyCurrency modify(MSG_CS_REQUEST_NAVY_LEVEL_UP, SYSTEM_ID_ARMY_UP);
  modify.coin += -add_exp;

  int32_t result = this->CheckCurrency(modify);
  if (result) return result;
  result = this->CheckItem(NULL, &delete_set, &notify_set);
  if (result) return result;

  int32_t level_changed = this->AddNavyExp(navy, add_exp);
  if (level_changed) {
    this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_ARMY_UP, entry.head.msgid);
  }

  this->UpdateCurrency(modify);
  this->ObtainItem(NULL, &delete_set, &notify_set, MSG_CS_REQUEST_NAVY_LEVEL_UP,
                   SYSTEM_ID_ARMY_UP);

  this->OnEquipChanged(0);

  UpdateBuyCount(sy::COUNT_TYPE_DAILY_NAVY_UP, 1);

  MessageResponseNavyLevelUp response;
  response.set_navy_uid(navy->uid());
  response.set_level_changed(level_changed);
  this->SendMessageToClient(MSG_CS_RESPONSE_NAVY_LEVEL_UP, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestNavyRefine(CSMessageEntry& entry) {
  MessageRequestNavyRefine* message =
      static_cast<MessageRequestNavyRefine*>(entry.get());
  if (!message) return ERR_INTERNAL;

  // 1.该宝物是否存在
  LogicItem* navy = this->items_.GetItemByUniqueID(message->item_uid());
  if (!navy || !navy->army_base()) return ERR_ITEM_NOT_FOUND;

  // 2.该宝物是否可以升级
  int32_t current_refine_level = navy->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL);
  const ArmyRefineBase* refine_data =
      ARMY_REFINE_BASE.GetEntryByID(current_refine_level).get();
  if (!refine_data) return ERR_ITEM_REFINE_NOT_FOUND;

  // 3.需要的金钱和材料是否满足
  ModifyCurrency modify(MSG_CS_REQUEST_NAVY_REFINE, SYSTEM_ID_ARMY_REFORM);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kSub>(modify, item_set, refine_data->cost);

  int32_t result = CheckCurrency(modify);
  if (result) return result;

  // 4.其他宝物数量和种类是否正确
  DeleteItemSet delete_set;

  size_t need_self_num = refine_data->self_cost;
  if (need_self_num) {
    std::sort(message->mutable_other_uid()->begin(),
              message->mutable_other_uid()->end());
    int64_t* end = std::unique(message->mutable_other_uid()->begin(),
                               message->mutable_other_uid()->end());
    for (int64_t* iter = message->mutable_other_uid()->begin(); iter != end;
         ++iter) {
      LogicItem* other_item = this->items_.GetItemByUniqueID(*iter);
      if (!other_item || !other_item->army_base()) return ERR_PARAM_INVALID;
      if (navy->item_id() != other_item->item_id()) return ERR_PARAM_INVALID;
      if (other_item->GetAttr(ITEM_ATTR_NAVY_LEVEL)) return ERR_PARAM_INVALID;
      if (other_item->GetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL))
        return ERR_PARAM_INVALID;
      if (other_item->GetAttr(ITEM_ATTR_EQUIPED_POS)) return ERR_PARAM_INVALID;

      if (delete_set.size() >= need_self_num) break;

      delete_set.push_back(*iter);
    }

    if (delete_set.size() < need_self_num) return ERR_ITEM_NEED_MORE;
  }

  // 5.其他宝物是否存在
  result = CheckItem(&item_set, &delete_set, NULL);
  if (result) return result;

  // 6.宝物升级
  navy->SetAttr(ITEM_ATTR_NAVY_REFINE_LEVEL, current_refine_level + 1);

  // 7.扣除金钱材料
  this->UpdateCurrency(modify);

  // 8.删除其他宝物通知宝物升级
  NotifyItemSet notify;
  notify.push_back(navy->uid());
  this->ObtainItem(&item_set, &delete_set, &notify, MSG_CS_REQUEST_NAVY_REFINE,
                   SYSTEM_ID_ARMY_REFORM);


  this->OnEquipChanged(0);

  // 9.回发消息
  MessageResponseNavyRefine response;
  response.set_item_uid(navy->uid());
  this->SendMessageToClient(MSG_CS_RESPONSE_NAVY_REFINE, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestComposeEquip(CSMessageEntry& entry) {
  MessageRequestComposeEquip* message =
      static_cast<MessageRequestComposeEquip*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t item_id = message->item_id();

  int32_t count = message->item_count();
  if (count < 1) count = 1;
  if (count > 100) count = 100;

  MessageResponseComposeEquip response;
  AddSubItemSet item_set;

  // 1.判断合成类型
  if (item_id >= Setting::kShipStartID && item_id <= Setting::kShipEndID) {
    // 2.1是否可以合成
    const HeroBase* hero_base = HERO_BASE.GetEntryByID(item_id).get();
    if (!hero_base) return ERR_HERO_NOT_FOUND;
    if (hero_base->ship_piece.v2 <= 0) return ERR_ITEM_NEED_MORE;

    // 2.2检查材料和金钱
    item_set.push_back(ItemParam(hero_base->ship_piece.v1,
                                 -(hero_base->ship_piece.v2 * count)));
    int32_t result = this->CheckItem(&item_set, NULL, NULL);
    if (result) return result;

    // 2.3 合成船只
    std::vector<sy::HeroInfo> heroes;
    for (int32_t i = 0; i < count; ++i) {
      sy::HeroInfo info;
      info.set_hero_id(item_id);
      heroes.push_back(info);
    }
    this->UpdateHeroInfo(heroes, SYSTEM_ID_EQUIP_REFORM,
                         MSG_CS_REQUEST_COMPOSE_EQUIP);

    // 2.4 扣除材料
    this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_COMPOSE_EQUIP,
                     SYSTEM_ID_EQUIP_REFORM);

    for (std::vector<sy::HeroInfo>::const_iterator iter = heroes.begin();
         iter != heroes.end(); ++iter) {
      response.add_ship_uid(iter->uid());
    }

  } else {
    // 3.1是否可以合成
    const EquipBase* equip_base = EQUIP_BASE.GetEntryByID(item_id).get();
    if (!equip_base) return ERR_ITEM_NOT_FOUND;
    if (equip_base->equip_piece.v2 <= 0) return ERR_ITEM_NEED_MORE;

    // 3.2检查材料和金
    item_set.push_back(ItemParam(equip_base->equip_piece.v1,
                                 -(equip_base->equip_piece.v2 * count)));
    int32_t result = this->CheckItem(&item_set, NULL, NULL);
    if (result) return result;

    // 3.3 合成物品
    NotifyItemSet notify_set;
    for (int32_t i = 0; i < count; i++) {
      LogicItem* equip = this->AddItem(item_id, 1, NULL);
      if (!equip) return ERR_INTERNAL;
      notify_set.push_back(equip->uid());
    }

    // 3.4扣除材料
    this->ObtainItem(&item_set, NULL, &notify_set, MSG_CS_REQUEST_COMPOSE_EQUIP,
                     SYSTEM_ID_EQUIP_REFORM);

    for (NotifyItemSet::iterator iter = notify_set.begin();
         iter != notify_set.end(); ++iter) {
      response.add_item_uid(*iter);
    }
  }

  // 4.回发消息
  this->SendMessageToClient(MSG_CS_RESPONSE_COMPOSE_EQUIP, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSellItem(CSMessageEntry& entry) {
  MessageRequestSellItem* message =
      static_cast<MessageRequestSellItem*>(entry.get());
  if (!message) return ERR_INTERNAL;

  AddSubItemSet item_set;
  DeleteItemSet delete_set;
  ModifyCurrency modify(MSG_CS_REQUEST_SELL_ITEM, SYSTEM_ID_SHOP_LIST);

  ProtobufUnique(message->mutable_item_uid());

  for (int32_t i = 0; i < message->item_uid_size(); ++i) {
    LogicItem* item = this->items_.GetItemByUniqueID(message->item_uid(i));
    if (!item) continue;
    if (item_set.full()) continue;
    if (delete_set.full()) continue;

    ValuePair2<int32_t, int32_t> sell_price = item->sell_price();
    if (sell_price.v1 <= 0 || sell_price.v2 <= 0)
      continue;

    delete_set.push_back(item->uid());

    modify[sell_price.v1] += (sell_price.v2 * item->count());
  }

  int32_t result = CheckItem(&item_set, &delete_set, NULL);
  if (result) return result;

  result = CheckCurrency(modify);
  if (result) return result;

  this->ObtainItem(NULL, &delete_set, NULL, MSG_CS_REQUEST_SELL_ITEM,
                   SYSTEM_ID_SHOP_LIST);
  this->UpdateCurrency(modify);

  MessageResponseSellItem response;
  for (int32_t i = MONEY_KIND_COIN; i <= MoneyKind_MAX; ++i) {
    int32_t value = modify[i];
    if (value) {
      sy::MoneyType* info = response.add_money();
      info->set_kind(i);
      info->set_value(value);
    }
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_SELL_ITEM, &response);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSellShip(CSMessageEntry& entry) {
  MessageRequestSellShip* message =
      static_cast<MessageRequestSellShip*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ModifyCurrency modify(MSG_CS_REQUEST_SELL_SHIP, SYSTEM_ID_SHOP_LIST);
  DeleteHeroSet delete_heros;

  for (int32_t i = 0; i < message->ship_uid_size(); ++i) {
    LogicHero* ship = this->GetHeroByUID(message->ship_uid(i));
    if (!ship || !ship->second) continue;

    if(delete_heros.full()) break;

    ValuePair2<int32_t, int32_t> sell_price = ship->second->sell;
    if (sell_price.v1 <= 0 || sell_price.v2 <= 0)
      continue;

    delete_heros.push_back(ship->first.uid());

    modify[sell_price.v1] += sell_price.v2;
  }

  int32_t result = CheckHero(&delete_heros, NULL);
  if (result) return result;

  result = CheckCurrency(modify);
  if (result) return result;

  this->ObtainHero(&delete_heros, NULL, SYSTEM_ID_SHOP_LIST,
                   MSG_CS_REQUEST_SELL_SHIP);
  this->UpdateCurrency(modify);

  MessageResponseSellShip response;
  for (int32_t i = MONEY_KIND_COIN; i <= MoneyKind_MAX; ++i) {
    int32_t value = modify[i];
    if (value) {
      sy::MoneyType* info = response.add_money();
      info->set_kind(i);
      info->set_value(value);
    }
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_SELL_SHIP, &response);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestBuyItem(CSMessageEntry& entry) {
  MessageRequestBuyItem* message =
      static_cast<MessageRequestBuyItem*>(entry.get());
  if (!message) return ERR_INTERNAL;

  // 1.指定商品是否存在
  const ShopBase* shop_base =
      SHOP_BASE.GetEntryByID(message->commodity_id()).get();
  if (!shop_base) return ERR_PARAM_INVALID;

  if (message->commodity_id() == 40200)
    entry.head.msgid = MSG_CS_REQUEST_BUY_OPEN_FUND;

  //纪念币最高可以购买1W个
  int32_t kMaxCount = message->commodity_id() == 100000 ? 10000 : 100;

  int32_t buy_count = message->commodity_count();
  if (buy_count <= 0) buy_count = 1;
  if (buy_count >= kMaxCount) buy_count = kMaxCount;

  // 2.判断购买限制
  int32_t result = CheckBuyCondition(shop_base);
  if (result) return result;

  Army* army = server->GetArmyByID(this->army_id_);
  if (!shop_base->onsale) return ERR_PARAM_INVALID;
  if (shop_base->shoptype == 8 || shop_base->shoptype == 9) {
    if (!army || army->level() < shop_base->lv_min ||
        army->level() > shop_base->lv_max)
      return ERR_PARAM_INVALID;
  } else {
    if (this->level() < shop_base->lv_min)
      return ERR_PARAM_INVALID;
  }

  // 3.判断购买次数限制
  int32_t bought_count = 0;
  int32_t buy_type = 0;
  const std::pair<int32_t, int32_t> buying_restrictions =
      shop_base->GetBuyTypeAndCount(this->vip_level());
  if (0 == buying_restrictions.first) return ERR_PARAM_INVALID;
  if (ShopBase::IsRefreshShop(shop_base->shoptype)) {
    //刷新商店
    if (shop_base->shoptype == 9) {
      if (!army) return ERR_PARAM_INVALID;
      if (!army->CanCommodityBuy(shop_base->lattice_position, this->uid()))
        return ERR_PARAM_INVALID;
    } else {
      const RefreshShopInfo& refresh_info =
          refresh_shop_info_[shop_base->shoptype];
      bool flag = false;
      for (int32_t i = 0; i < refresh_info.feats_commodity_size(); i++) {
        if (shop_base->id() == refresh_info.feats_commodity(i).commodity_id()) {
          bought_count = refresh_info.feats_commodity(i).bought_count();
          flag = true;
          break;
        }
      }
      if (!flag) return ERR_PARAM_INVALID;
    }
    buy_type = 3;
  } else {
    buy_type = buying_restrictions.first;
    bought_count = GetBoughtCount(shop_base->id(), buy_type);
  }
  if (buying_restrictions.second != 0 &&
      (bought_count + buy_count) > buying_restrictions.second)
    return ERR_PARAM_INVALID;

  // 4.检查货币是否充足
  int32_t need_money = shop_base->GetPrice(bought_count, buy_count);
  if (need_money < 0) return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_SHOP_LIST);
  AddSubItemSet item_set;

  if (shop_base->moneytype < 100)
    modify[shop_base->moneytype] = -need_money;
  else
    item_set.push_back(ItemParam(shop_base->moneytype, -need_money));
  ValuePair2Vec temp_exchange = shop_base->exchange;
  for (ValuePair2Vec::iterator it = temp_exchange.begin();
       it != temp_exchange.end(); ++it)
    it->v2 *= buy_count;
  FillCurrencyAndItem<kSub>(modify, item_set, temp_exchange);

  // 5.添加物品
  ValuePair2<int32_t, int32_t> shop_item = shop_base->item;
  shop_item.v2 *= buy_count;
  FillCurrencyAndItem<kAdd>(modify, item_set, shop_item);

  // 6.扣除金钱并增加购买次数
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  CHECK_RET(this->CheckCurrency(modify));

  ObtainItem(&item_set, NULL, NULL,entry.head.msgid,
             SYSTEM_ID_SHOP_LIST);
  this->UpdateCurrency(modify);

  if (1 == buy_type || 2 == buy_type)
    UpdateShopInfo(shop_base->id(), buy_type, bought_count + buy_count);
  if (3 == buy_type) {
    if (shop_base->shoptype == 9) {
      if (army)
        army->AddArmyBuyRecord(shop_base->lattice_position, this->uid());
    } else {
      RefreshShopInfo& refresh_info = refresh_shop_info_[shop_base->shoptype];
      for (int32_t i = 0; i < refresh_info.feats_commodity_size(); i++) {
        if (shop_base->id() == refresh_info.feats_commodity(i).commodity_id()) {
          refresh_info.mutable_feats_commodity(i)->set_bought_count(
              refresh_info.feats_commodity(i).bought_count() + buy_count);
          break;
        }
      }
      UpdateFeatsShopInfo(shop_base->shoptype);
      if (shop_base->shoptype == 3)
        UpdateAchievement(
            SEVEN_DAY_TYPE_SHOP_BUY,
            this->achievements_[SEVEN_DAY_TYPE_SHOP_BUY] + buy_count);
    }
  }
  if (4 == shop_base->star.v1)
    server->UpdateServerShopCount(message->commodity_id(), buy_count);

  if (shop_base->item.v1 == GetSettingValue(oilr_id))
    UpdateBuyCount(sy::COUNT_TYPE_DAILY_BUY_OIL,
                   shop_base->item.v2 * buy_count);
  if (shop_base->item.v1 == GetSettingValue(energy_id))
    UpdateBuyCount(sy::COUNT_TYPE_DAILY_BUY_ENEGRY,
                   shop_base->item.v2 * buy_count);
  if (shop_base->item.v1 ==
      Setting::GetValue2(Setting::dstrike_item_mapped)[0].v1)
    UpdateAchievement(sy::SEVEN_DAY_TYPE_DSTRIKE_BUY,
                      this->achievements_[SEVEN_DAY_TYPE_DSTRIKE_BUY] +
                          shop_base->item.v2 * buy_count);

  if (shop_base->shoptype == 1 && modify[MONEY_KIND_MONEY] < 0) {
    UpdateAchievement(sy::SEVEN_DAY_TYPE_SHOPTYPE1_COST,
                      this->achievements_[SEVEN_DAY_TYPE_SHOPTYPE1_COST] -
                          modify[MONEY_KIND_MONEY]);
  }
  if (shop_base->shoptype == 2 && modify[MONEY_KIND_MUSCLE] < 0) {
    UpdateAchievement(sy::SEVEN_DAY_TYPE_SHOPTYPE2_COST,
                      this->achievements_[SEVEN_DAY_TYPE_SHOPTYPE2_COST] -
                          modify[MONEY_KIND_MUSCLE]);
  }
  if (shop_base->shoptype == 3 && modify[MONEY_KIND_HERO] < 0) {
    UpdateAchievement(sy::SEVEN_DAY_TYPE_SHOPTYPE3_COST,
                      this->achievements_[SEVEN_DAY_TYPE_SHOPTYPE3_COST] -
                          modify[MONEY_KIND_HERO]);
  }
  if (shop_base->shoptype == 4 && modify[MONEY_KIND_PRESTIGE] < 0) {
    UpdateAchievement(sy::SEVEN_DAY_TYPE_SHOPTYPE4_COST,
                      this->achievements_[SEVEN_DAY_TYPE_SHOPTYPE4_COST] -
                          modify[MONEY_KIND_PRESTIGE]);
  }
  if (shop_base->shoptype == 6 && modify[MONEY_KIND_EXPLOIT] < 0) {
    UpdateAchievement(sy::SEVEN_DAY_TYPE_SHOPTYPE6_COST,
                      this->achievements_[SEVEN_DAY_TYPE_SHOPTYPE6_COST] -
                          modify[MONEY_KIND_EXPLOIT]);
  }

  MessageSSAddBuyItemLog log;
  log.set_tid(server->GetTID());
  log.set_commodity_id(shop_base->id());
  log.set_shop_type(shop_base->shoptype);
  log.set_item_id(shop_base->item.v1);
  log.set_buy_count(buy_count);
  this->SendMessageToDB(MSG_SS_ADD_BUY_ITEM_LOG, &log);

  MessageResponseBuyItem response;
  response.set_commodity_id(message->commodity_id());
  response.set_commodity_count(buy_count);
  this->SendMessageToClient(MSG_CS_RESPONSE_BUY_ITEM, &response);
  return ERR_OK;
}

int32_t LogicPlayer::CheckBuyCondition(const ShopBase* base) {
  switch (base->star.v1) {
    case 1: {  //爬塔星数判断
      if (this->tower_state_.max_star() < base->star.v2)
        return ERR_PLAYER_NEED_STAR;
    } break;
    case 2: {  //等级判断
      if (this->level() < base->star.v2) return ERR_CURRENCY_EXP;
    } break;
    case 3: {  //竞技场最高排名
      if (this->max_pk_rank() > base->star.v2) return ERR_PARAM_INVALID;
    } break;
    case 4: {  //商品限制总数量
      if (server->GetServerShopCount(base->id()) >= base->star.v2)
        return ERR_PARAM_INVALID;
    } break;
    case 5: {  //军团内总数限制
      Army* army = server->GetArmyByID(this->army_id_);
      if (!army) return ERR_PARAM_INVALID;
      if (army->GetArmyShopNum(base->id()) >= base->star.v2)
        return ERR_PARAM_INVALID;
    } break;
    case 6: {  //军团等级下限
      Army* army = server->GetArmyByID(this->army_id_);
      if (!army) return ERR_PARAM_INVALID;
      if (army->level() < base->star.v2) return ERR_PARAM_INVALID;
    } break;
    case 7: {  //制霸积分
      if (LegionWar::Instance().GetPlayerScore(this->uid()) < base->star.v2)
        return ERR_PARAM_INVALID;
    } break;
    case 8: {  //英雄回归限制
      if (base->star.v2 && !IsHeroComeBack()) return ERR_PARAM_INVALID;
    } break;
    case 0:break;
#ifdef DEBUG
    default: {
      TRACE_LOG(logger)("Unknown Condition: %d", base->star.v1);
      assert(false && "shop conditiom");
    } break;
#endif
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestRefreshFeats(CSMessageEntry& entry) {
  MessageRequestRefreshFeats* message =
      static_cast<MessageRequestRefreshFeats*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t shop_id = message->shop_id();
  if (!ShopBase::IsRefreshShop(shop_id)) return ERR_PARAM_INVALID;

  const time_t max_free_num =
      Setting::GetValueInVec2(Setting::shopX_free_num, shop_id);
  const time_t recover_time =
      Setting::GetValueInVec2(Setting::shopX_recover_time, shop_id);
  const time_t currect_time = GetVirtualSeconds();

  RefreshShopInfo& shop_info = refresh_shop_info_[shop_id];

  //计算出免费次数
  time_t free_num = 0;
  if (recover_time > 0)
    free_num = (currect_time - shop_info.last_time()) / recover_time;

  //免费次数修正
  if (free_num >= max_free_num) {
    free_num = max_free_num;
    shop_info.set_last_time((currect_time - max_free_num * recover_time));
  }

  if (free_num > 0) {
    //使用免费次数
    shop_info.set_last_time(shop_info.last_time() + recover_time);
  } else {
    //使用刷新令或这功勋
    int32_t vip_type = 0;
    switch (shop_id) {
      case 3:
        vip_type = 480;
        break;
      case 7:
        vip_type = 814;
        break;
      case 12:
        vip_type = 1401;
        break;
      case 25:
        vip_type = 1972;
        break;
    }
    VipFunctionBase* vip_base = VIP_FUNCTION_BASE.GetEntryByID(vip_type).get();
    if (!vip_base) return ERR_INTERNAL;
    const int32_t refresh_num =
        vip_base->GetValue(this->vip_level(), this->level());

    if (shop_info.used_count() < refresh_num) {
      AddSubItemSet item_set;
      item_set.push_back(ItemParam(
          Setting::GetValueInVec2(Setting::shopX_refresh_id, shop_id), -1));
      int32_t result = this->CheckItem(&item_set, NULL, NULL);

      if (result) {
        int32_t rc_id =
            Setting::GetValueInVec2(Setting::shopX_refresh_resource1, shop_id);
        int32_t rc_count =
            Setting::GetValueInVec2(Setting::shopX_refresh_resource2, shop_id);
        if (rc_id < 100) {
          ModifyCurrency modify(MSG_CS_REQUEST_REFRESH_FEATS,
                                SYSTEM_ID_EXPLOIT_SHOP);
          modify[rc_id] = -rc_count;
          result = CheckCurrency(modify);
          if (result) return result;
          this->UpdateCurrency(modify);
        } else {
          AddSubItemSet modify;
          modify.push_back(ItemParam(rc_id, -rc_count));
          result = CheckItem(&modify, NULL, NULL);
          if (result) return result;
          this->ObtainItem(&modify, NULL, NULL, MSG_CS_REQUEST_REFRESH_FEATS,
                           SYSTEM_ID_EXPLOIT_SHOP);
        }
      } else {
        this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_REFRESH_FEATS,
                         SYSTEM_ID_EXPLOIT_SHOP);
      }

      shop_info.set_used_count(shop_info.used_count() + 1);
    } else {
      return ERR_PARAM_INVALID;
    }
  }

  ShopBase::RandomFeatsCommodity(shop_id, this->level(),
                                 shop_info.mutable_feats_commodity(),
                                 server->GetServerStartDays());
  UpdateFeatsShopInfo(shop_id);
  if (3 == shop_id) {
    UpdateAchievement(SEVEN_DAY_TYPE_SHOP_REFRESH,
                      this->achievements_[SEVEN_DAY_TYPE_SHOP_REFRESH] + 1);
  }

  this->SendMessageToClient(MSG_CS_RESPONSE_REFRESH_FEATS, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::GetCopyStarByType(int32_t copy_type) const {
  int32_t count = 0;
  if (copy_type == sy::COPY_TYPE_TOWER) {
    count = this->tower_state_.max_star();
    return count;
  }
  for (VectorMap<int32_t, int32_t>::const_iterator iter =
           this->copy_star_.begin();
       iter != this->copy_star_.end(); ++iter) {
    const CopyBase* base = COPY_BASE.GetEntryByID(iter->first).get();
    if (base && base->type == copy_type) count += iter->second;
  }
  return count;
}

int32_t LogicPlayer::GetBoughtCount(int32_t item_id, int32_t type) const {
  if (1 == type) {
    VectorMap<int32_t, int32_t>::const_iterator it =
        this->normal_shop_info_.find(item_id);
    if (it != this->normal_shop_info_.end()) return it->second;
  }
  if (2 == type) {
    VectorMap<int32_t, int32_t>::const_iterator it =
        this->life_shop_info_.find(item_id);
    if (it != this->life_shop_info_.end()) return it->second;
  }

  return 0;
}

int32_t LogicPlayer::ProcessRequestUpdateMailID(CSMessageEntry& entry) {
  MessageSSUpdateMailID request;
  this->SendMessageToDB(MSG_SS_UPDATE_MAIL_ID, &request);
  return ERR_OK;
}

void LogicPlayer::FetchServerMail() {
  if (!this->load_complete()) return;
  if (!server->server_mail().mails_size()) return;
  if (!this->last_server_mail_id_) return;
  if (this->last_server_mail_id_ >=
      (server->server_mail().mails().end() - 1)->mail_id())
    return;
  std::vector<std::pair<int32_t, int32_t> > reward;
  for (int32_t i = 0; i < server->server_mail().mails_size(); ++i) {
    const sy::MailInfo& mail = server->server_mail().mails(i);
    reward.clear();
    if (mail.mail_id() <= this->last_server_mail_id_) continue;

    bool channel_match = true;
    bool level_match =
      (mail.level_min() <= 0 && mail.level_max() <= 0) || //不限制等级
      (mail.level_min() <=0 && this->level() <= mail.level_max()) || //限制最高等级
      (mail.level_max() <=0 && this->level() >= mail.level_min()) || //限制最低等级
      (this->level() >= mail.level_min() && this->level() <= mail.level_max());
    bool vip_match =
      (mail.vip_min() <= 0 && mail.vip_max() <= 0) || //不限制等级
      (mail.vip_min() <= 0 && this->vip_level() <= mail.vip_max()) || //限制最高等级
      (mail.vip_max() <= 0 && this->vip_level() >= mail.vip_min()) || //限制最低等级
      (this->vip_level() >= mail.vip_min() && this->vip_level() <= mail.vip_max());

    if (channel_match && level_match && vip_match) {
      for (int32_t reward_index = 0; reward_index < mail.mail_attachment_size();
           ++reward_index) {
        reward.push_back(
            std::make_pair(mail.mail_attachment(reward_index).key(),
                           mail.mail_attachment(reward_index).value()));
      }
      this->SendMail(this->uid(), mail.mail_time(), sy::MAIL_TYPE_SYS,
                     mail.mail_content(), &reward);
    }

    this->last_server_mail_id_ = mail.mail_id();
  }

  MessageSSUpdateMailID request;
  request.set_server_mail_id(this->last_server_mail_id_);
  this->SendMessageToDB(MSG_SS_UPDATE_MAIL_ID, &request);
}

int32_t LogicPlayer::ProcessRequestSendMail(CSMessageEntry& entry) {
  MessageRequestSendMail* message =
      static_cast<MessageRequestSendMail*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->content().length() > 256) return ERR_MAIL_TOO_LONG;
  ArrayStream<1024> stream;
  stream.Append("%ld,%s,%s", this->uid(), this->player().name().c_str(),
                message->content().c_str());
  this->SendMail(message->player_id(), GetSeconds(), MAIL_TYPE_PERSONAL_MAIL,
                 stream.str(), NULL);
  this->SendMessageToClient(MSG_CS_RESPONSE_SEND_MAIL, NULL);
  return ERR_OK;
}

void LogicPlayer::UpdateShopInfo(int32_t item_id, int32_t type, int32_t count) {
  MessageNotifyShopInfo msg;
  MessageSSUpdateShopInfo db_msg;

  if (1 == type) {
    this->normal_shop_info_[item_id] = count;

    for (VectorMap<int32_t, int32_t>::iterator it = normal_shop_info_.begin();
         it != normal_shop_info_.end(); ++it) {
      sy::ShopCommodityInfo* info = msg.add_normal_commodity();
      info->set_commodity_id(item_id);
      info->set_bought_count(count);

      info = db_msg.add_normal_commodity();
      info->set_commodity_id(item_id);
      info->set_bought_count(count);
    }
  }
  if (2 == type) {
    this->life_shop_info_[item_id] = count;

    sy::ShopCommodityInfo* info = msg.add_life_commodity();
    info->set_commodity_id(item_id);
    info->set_bought_count(count);

    info = db_msg.add_life_commodity();
    info->set_commodity_id(item_id);
    info->set_bought_count(count);
  }

  this->SendMessageToClient(MSG_CS_NOTIFY_SHOP_INFO, &msg);
  this->SendMessageToDB(MSG_SS_UPDATE_SHOP_INFO, &db_msg);
}

void LogicPlayer::UpdateFeatsShopInfo(int32_t shop_id) {
  MessageNotifyShopInfo msg;
  MessageSSUpdateShopInfo db_msg;

  RefreshShopInfo& shop_info = refresh_shop_info_[shop_id];
  msg.add_refresh_commodity()->CopyFrom(shop_info);
  db_msg.add_refresh_commodity()->CopyFrom(shop_info);

  if (shop_id == 3) {
    msg.set_last_time(shop_info.last_time());
    msg.set_used_count(shop_info.used_count());
    msg.mutable_feats_commodity()->CopyFrom(shop_info.feats_commodity());
  }

  this->SendMessageToClient(MSG_CS_NOTIFY_SHOP_INFO, &msg);
  this->SendMessageToDB(MSG_SS_UPDATE_SHOP_INFO, &db_msg);
}

int32_t LogicPlayer::ProcessRequestFateLevelUp(CSMessageEntry& entry) {
  MessageRequestFateLevelUp* message =
      static_cast<MessageRequestFateLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->count() < 0) return ERR_INTERNAL;

  LogicHero* hero = this->GetHeroByUID(message->hero_uid());
  if (!hero) return ERR_HERO_NOT_FOUND;
  FateBase* base = FATE_BASE.GetEntryByID(hero->first.fate_level()).get();
  if (!base) return ERR_HERO_NOT_FOUND;

  int32_t need_scale = hero->first.fate_seed();
  int32_t fate_exp = hero->first.fate_exp();
  int32_t fate_level = hero->first.fate_level();

  int32_t real_count = 0;

  for (int32_t i = 0; i < message->count(); ++i) {
    real_count++;
    fate_exp += base->cost;

    int32_t scale = static_cast<int32_t>((long)fate_exp * 10000 / base->max_value);

    if (scale >= need_scale) {
      ++fate_level;
      fate_exp = 0;
      need_scale = FateBase::RandomShipFateScale();
      break;
    }
  }

  AddSubItemSet item_set;
  item_set.push_back(
      ItemParam(GetSettingValue(ship_fate_id), -base->cost * real_count));
  int32_t result = CheckItem(&item_set, NULL, NULL);
  if (result) return result;

  hero->first.set_fate_cost(hero->first.fate_cost() + base->cost * real_count);
  hero->first.set_fate_exp(fate_exp);
  hero->first.set_fate_level(fate_level);
  hero->first.set_fate_seed(need_scale);

  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_FATE_LEVEL_UP,
             SYSTEM_ID_WAR_SHIP_RESEARCH);

  if (this->IsInTactic(hero->first.uid()))
    this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_WAR_SHIP_RESEARCH,
                         entry.head.msgid);
  else
    UpdateHeroInfo(hero->first, kNotifyAll, SYSTEM_ID_WAR_SHIP_RESEARCH,
                   MSG_CS_REQUEST_FATE_LEVEL_UP);

  MessageResponseFateLevelUp response;
  response.set_hero_uid(hero->first.uid());
  SendMessageToClient(MSG_CS_RESPONSE_FATE_LEVEL_UP, &response);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetPKRankList(CSMessageEntry& entry) {
  MessageResponseGetPkRankList response;
  for (int32_t i = 1; i <= ARENA_LIST_COUNT; ++i) {
    sy::PKRankInfo* info = response.add_infos();
    info->set_rank(i);
    info->set_player_id(server->GetUIDByRand(i));
  }

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_PK_RANK_LIST, &response);
  return ERR_OK;
}

void LogicPlayer::UpdatePKRankInfo() {
  if (this->pk_current_rank_ <= 0) return;

  this->pk_current_rank_ = server->GetRank(this->uid());
  MessageSSUpdatePKRankInfo message;
  message.set_rank(this->pk_current_rank_);
  message.set_player_id(this->uid());
  message.set_server(server_config->server_id());
  for (PkTargetType::const_iterator iter = this->pk_targets_.begin();
       iter != this->pk_targets_.end(); ++iter) {
    sy::PKRankInfo* info = message.add_rank_info();
    info->set_rank(iter->first);
    info->set_player_id(0);
  }
  message.set_rank_times(0);
  message.set_max_rank(this->pk_max_rank_);
  message.set_last_pk_time(this->last_pk_time_);

  this->SendMessageToDB(MSG_SS_UPDATE_PK_RANK_INFO, &message);
}

int32_t LogicPlayer::ProcessRequestGetMyPkRankInfo(CSMessageEntry& entry) {
  if (this->pk_current_rank_ <= 0 || this->pk_targets_.empty() ||
      this->pk_max_rank_ <= 0) {
    this->pk_current_rank_ = server->RefreshPKTargets(this->uid(), this->pk_targets_);
    this->pk_max_rank_ = this->pk_current_rank_;
    this->UpdatePKRankInfo();
  }

  MessageResponseGetMyPkRankInfo response;
  response.set_max_rank(this->pk_max_rank_);
  response.set_pk_times(0);
  response.set_rank(this->pk_current_rank_);
  response.set_last_pk_time(this->last_pk_time_);
  for (PkTargetType::iterator iter = this->pk_targets_.begin();
       iter != this->pk_targets_.end(); ++iter) {
    iter->second = server->GetUIDByRand(iter->first);
  }
  for(PkTargetType::const_iterator iter = this->pk_targets_.begin();
       iter != this->pk_targets_.end(); ++iter) {
    sy::PKRankInfo *info = response.add_infos();
    info->set_rank(iter->first);
    info->set_player_id(iter->second);
  }
  //竞技场前10
  for (int32_t i = 1; i <= 10; ++i) {
    sy::PKRankInfo* info = response.add_top10();
    info->set_rank(i);
    info->set_player_id(server->GetUIDByRand(i));
  }

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_MY_PK_RANK_INFO, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetOtherPlayerInfo(CSMessageEntry& entry) {
  MessageRequestGetOtherPlayer* message =
      static_cast<MessageRequestGetOtherPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSRequestLoadMultiPlayer request;
  for (int32_t i = 0; i < message->player_uids_size(); ++i) {
    int64_t player_id = message->player_uids(i);
    INFO_LOG(logger)("GetOtherPlayer:%ld", player_id);
    if (player_id <= sy::MAX_ROBOT_ID) continue;

    request.add_forward_ids(player_id);
    LogicPlayer *player = server->GetPlayerByID(player_id);
    if (!player) {
      request.add_player_ids(player_id);
      player = server->GetOrNewPlayer(player_id);
    }
    if (player) player->active();
  }

  request.set_forward_player(this->uid());
  request.set_msg_id(MSG_CS_REQUEST_GET_OTHER_PLAYER);
  this->SendMessageToDB(MSG_SS_REQUEST_LOAD_MULTI_PLAYER, &request);
  return ERR_OK;
}

void LogicPlayer::FillOtherPlayerInfo(sy::OtherPlayerInfo* info) {
  info->set_server(server_config->server_id());
  info->set_server_id(this->player_.server());
  info->set_player_id(this->uid());
  info->set_name(this->player_.name());
  info->set_level(this->level());
  info->set_vip_level(this->vip_level());
  info->set_avatar(this->avatar());
  info->set_rank_id(this->rank_id());
  info->mutable_carrier()->CopyFrom(this->current_carrier_);
  info->set_army_id(this->army_id());
  if (this->army_id()) {
    Army* army = server->GetArmyByID(this->army_id());
    if (army) info->set_army_name(army->name());
  }
  //船, 阵型
  for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
    info->add_battle_pos()->CopyFrom(this->tactic_.infos(i));
    int64_t hero_uid = this->tactic_.infos(i).hero_uid();
    LogicHero* hero = this->GetHeroByUID(hero_uid);
    if (!hero) continue;
    info->add_heros()->CopyFrom(hero->first);
  }
  //出站阵型
  for (int32_t i = 0; i < this->tactic_.battle_pos_size(); ++i) {
    info->add_battle_pos1()->CopyFrom(this->tactic_.battle_pos(i));
  }

  //装备
  info->mutable_equips()->CopyFrom(this->equips_);
  RepeatedField<int64_t>* array[] = {
      this->equips_.mutable_equips_1(), this->equips_.mutable_equips_2(),
      this->equips_.mutable_equips_3(), this->equips_.mutable_equips_4(),
      this->equips_.mutable_equips_5(), this->equips_.mutable_equips_6(),
  };
  for (int32_t i = 0; i < ArraySize(array); ++i) {
    for (int32_t item_size = 0; item_size < array[i]->size(); ++item_size) {
      int64_t item_uid = array[i]->Get(item_size);
      if (!item_uid) continue;
      LogicItem* item = this->items().GetItemByUniqueID(item_uid);
      if (!item) continue;
      info->add_items()->CopyFrom(item->data());
    }
  }
  for (std::vector<sy::CarrierInfo>::iterator it = carriers_.begin();
       it != carriers_.end(); ++it) {
    info->add_all_carriers()->CopyFrom(*it);
  }
  info->set_pvp_country(this->cross_server_info_.country());
}

int32_t LogicPlayer::ProcessResponseGetOtherPlayerInfo(SSMessageEntry& entry) {
  MessageSSResponseLoadMultiPlayer* message =
      static_cast<MessageSSResponseLoadMultiPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;
  MessageSSRequestLoadMultiPlayer* request = message->mutable_request();

  MessageResponseGetOtherPlayer response;
  for (int32_t i = 0; i < request->forward_ids_size(); ++i) {
    LogicPlayer *player = server->GetPlayerByID(request->forward_ids(i));
    if (player) {
      sy::OtherPlayerInfo* info = response.add_infos();
      player->FillOtherPlayerInfo(info);
    }
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_OTHER_PLAYER, &response);
  return ERR_OK;
}

int32_t LogicPlayer::OnPKFinished(PK& pk, bool sweep) {
  const CopyBase* base = pk.copy;
  sy::ReportRecord& record = pk.report;
  if (!base) return ERR_PARAM_INVALID;
  if (this->player_.oil() < base->power) return ERR_CURRENCY_OIL;
  if (this->player_.energy() < base->energy) return ERR_CURRENCY_ENERGY;

  int32_t success = record.win();
  const sy::PlayerSimpleInfo& p1 = record.players(0);
  const sy::PlayerSimpleInfo& p2 = record.players(1);
  int64_t p2_id = p2.player_id();

  int64_t report_id = server->GetNewReportID();
  int32_t p2_rank = server->GetRank(p2_id);
  int32_t rank_up = 0;

  if (success) {
    //P1排名尽量上升
    PkTargetType dest_player_targets;
    std::pair<int32_t, int32_t> result_rank = server->P1RankUp(this->uid(), p2_id);

    int32_t new_rank = result_rank.first;
    if (new_rank > 0 && new_rank < this->pk_current_rank_) {
      new_rank = server->RefreshPKTargets(this->uid(), this->pk_targets_);
      p2_rank = server->RefreshPKTargets(p2_id, dest_player_targets);
      //通知另外一个玩家刷新
      this->RefreshPlayerPKTargets(p2_id, p2_rank, dest_player_targets);
    }
    DEBUG_LOG(logger)("PlayerID:%ld Rank:%d, DestPlayerID:%ld, Rank:%d",
        this->uid(), new_rank, p2_id, p2_rank);

    DefaultArrayStream stream;
    //排名提升了
    if (new_rank > 0 && new_rank < this->pk_current_rank_) {
      rank_up = 1;
      this->pk_current_rank_ = new_rank;
      stream.Append("%ld,%ld,%s,%d", report_id, this->uid(),
                    this->player_.name().c_str(), p2_rank);
      this->SendMail(p2_id, GetSeconds(), sy::MAIL_TYPE_BATTLE_SUCCESS,
                     stream.str(), NULL);
      if (this->achievements_[SEVEN_DAY_TYPE_PK] == 0 ||
          (new_rank && new_rank < this->achievements_[SEVEN_DAY_TYPE_PK]))
        UpdateAchievement(SEVEN_DAY_TYPE_PK, new_rank);
      if (new_rank <= 5) {
        sy::MessageNotifyServerNotice notice;
        notice.set_type(2);
        char temp[20] = {0};
        sprintf(temp, "%d", new_rank);
        notice.add_param(this->name());
        notice.add_param(temp);
        server->SendMessageToAllClient(sy::MSG_CS_NOTIFY_SERVER_NOTICE,
                                       &notice);
      }

    } else {
      stream.Append("%ld,%ld,%s", 0l, this->uid(),
                    this->player_.name().c_str());
      //给不是机器人的人发邮件
      if (p2_id > sy::MAX_ROBOT_ID)
        this->SendMail(p2_id, GetSeconds(), sy::MAIL_TYPE_BATTLE_FAIL,
                       stream.str(), NULL);
    }
    //历史排名提升
    DEBUG_LOG(logger)("PlayerID:%ld, MaxRank:%d, NewMaxRank:%d", this->uid(), this->pk_max_rank_, new_rank);
    if (new_rank > 0 && new_rank < this->pk_max_rank_) {
      this->SendPKRankReward(this->pk_max_rank_, new_rank);
      this->pk_max_rank_ = new_rank;
    }
    UpdateAchievement(FOURTEEN_DAY_TYPE_PK6,
                      this->achievements_[FOURTEEN_DAY_TYPE_PK6] + 1);
    UpdateAchievement(FOURTEEN_DAY_TYPE_PK7,
                      this->achievements_[FOURTEEN_DAY_TYPE_PK7] + 1);
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_PK, 1, this);
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_PK, 1, this);
  }

  UpdateBuyCount(sy::COUNT_TYPE_DAILY_ARENA, 1);

  //名次发生变化, 发送战报
  if (rank_up) {
    this->SendReportToDB(report_id, &pk.response);
    DefaultArrayStream stream;
    stream.Append("%d,%ld,%ld,%ld,%s,%d,%ld,%s,%d,%d,%d,%d,%d,%d,%d",
                  REPORT_TYPE_ARENA, GetSeconds(), report_id, this->uid(),
                  this->player_.name().c_str(), this->pk_current_rank_, p2_id,
                  p2.name().c_str(), p2_rank, success, p1.avatar(), p2.avatar(),
                  p1.has_level() ? p1.level() : 1,
                  p2.has_level() ? p2.level() : 1, rank_up);
    this->SendArenaReportResult(this->uid(), p2_id, stream.str(),
                                REPORT_TYPE_ARENA);
  }

  this->last_pk_time_ = GetSeconds();
  this->UpdatePKRankInfo();

  this->SendPKSuccessFailReward(success, base->id(), success, sweep);
  if (!sweep) this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);

  if (rank_up) {
    CSMessageEntry empty;
    return this->ProcessRequestGetMyPkRankInfo(empty);
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestRefreshPKTargets(CSMessageEntry& entry) {
  this->pk_current_rank_ = server->RefreshPKTargets(this->uid(), this->pk_targets_);
  return this->ProcessRequestGetMyPkRankInfo(entry);
}

static inline int32_t InitArenaTarget(PK& pk, int64_t player_id,
                                      int32_t top_rank) {
  //从未打过排行榜, 第一次永远都是胜利
  if (top_rank == sy::MAX_ROBOT_ID) {
    const MonsterGroupBasePtr& monster = GetRobotConfigByID(sy::MAX_ROBOT_ID - 1);
    if (!monster) return ERR_INTERNAL;
    pk.InitMonsterGroup(player_id, monster.get(), false);
    pk.b.player.set_player_id(player_id);
  } else {
    if (player_id <= sy::MAX_ROBOT_ID) {
      const MonsterGroupBasePtr& monster = GetRobotConfigByID(player_id);
      if (!monster) return ERR_INTERNAL;
      pk.InitMonsterGroup(player_id, monster.get(), false);
    } else {
      LogicPlayer* player = server->GetPlayerByID(player_id);
      if (!player || !player->load_complete()) return ERR_PLAYER_NOT_EXIST;
      pk.InitPlayer(player, false);
    }
  }
  return ERR_OK;
}

int32_t LogicPlayer::ArenaPK(sy::MessageRequestTestPkTarget* message) {
  if (!message) return ERR_INTERNAL;

  int64_t player_id = server->GetUIDByRand(message->rank());
  if (player_id != message->player_id() ||
      this->pk_targets_.find(message->rank()) == this->pk_targets_.end() ||
      this->pk_targets_[message->rank()] != player_id) {
    this->SendMessageToClient(MSG_CS_RESPONSE_TEST_PK_TARGET, NULL);
    CSMessageEntry entry;
    return this->ProcessRequestGetMyPkRankInfo(entry);
  }

  bool sweep = message->has_count() && message->count() > 0;
  int32_t count = message->has_count() ? std::min(message->count(), 5) : 1;

  const CopyBasePtr& copy_base = COPY_BASE.GetEntryByID(62500);

  int32_t result = ERR_OK;
  int32_t success_count = 0;
  int32_t lose_count = 0;
  for (int32_t i = 0; i < count; ++i) {
    PK pk(copy_base.get());
    pk.InitPlayer(this, true);
    CHECK_RET(InitArenaTarget(pk, player_id, this->pk_max_rank_));
    pk.GeneratePVPReport();

    result = this->OnPKFinished(pk, sweep);
    if (result) break;
    if (server_config->report()) {
      LogReportToFile(pk.report);
    }
    TRACE_LOG(logger)("PlayerID:%ld, CopyID:%ld, Star:%d", this->uid(), copy_base->id(), pk.star);

    success_count += bool(pk.star);
    lose_count += !bool(pk.star);
  }

  return result;
}

int32_t LogicPlayer::ProcessRequestTestPkTarget(CSMessageEntry& entry) {
  MessageRequestTestPkTarget* message =
      static_cast<MessageRequestTestPkTarget*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->player_id() > sy::MAX_ROBOT_ID) {
    LogicPlayer* player = server->GetPlayerByID(message->player_id());
    if (!player || !player->load_complete()) {
      MessageSSRequestLoadMultiPlayer request;
      request.set_msg_id(entry.head.msgid);
      request.set_conn_id(0);
      request.set_session_id(entry.session_ptr->GetSessionID());
      request.set_forward_player(this->uid());
      request.set_pk_player(this->uid());
      request.add_player_ids(message->player_id());
      request.mutable_arena()->CopyFrom(*message);

      server->SendServerMessageToDB(MSG_SS_REQUEST_LOAD_MULTI_PLAYER, &request);
      return ERR_OK;
    }
  }

  return this->ArenaPK(message);
}

void LogicPlayer::SendPKSuccessFailReward(bool success, int32_t copy_id,
                                          int32_t star, bool auto_flop) {
  const CopyBasePtr& base = COPY_BASE.GetEntryByID(copy_id);
  if (!base) return;

  MessageNotifyFightResult notify;
  int32_t first_blood = 0;
  ModifyCurrency modify(MSG_CS_REQUEST_TEST_PK_TARGET, SYSTEM_ID_PKMAIN);
  AddSubItemSet item_set;
  this->FillCopyAward(success, base.get(), modify, item_set, first_blood,
                      notify, auto_flop);

  int32_t result = this->CheckCurrency(modify);
  if (result) {
    ERROR_LOG(logger)("PlayerID:%ld, SendPKSuccessFailReward, CheckCurrency fail", this->uid());
  }
  result = this->CheckItem(&item_set, NULL, NULL);
  if (result) {
    ERROR_LOG(logger)("PlayerID:%ld, SendPKSuccessFailReward, CheckItem fail", this->uid());
  }
  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_TEST_PK_TARGET,
                   SYSTEM_ID_PKMAIN);

  notify.set_win(success);
  notify.set_copy_id(copy_id);
  notify.set_star(star);
  server->UpdateCopyStatistics(base->type, base->id(), this->uid());
  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
}

void LogicPlayer::SendPKRankReward(int32_t old_rank, int32_t new_rank) {
  if (old_rank <= new_rank) return;
  int32_t rank = old_rank;
  //排名提升奖励
  ArenaGoldRewardIter old_base = ArenaConfigFile::GetGoldRewardByRank(old_rank);
  if (old_base == ArenaConfigFile::GetGoldRewardEnd()) {
    ERROR_LOG(logger)("SendPKRankReward Fail, OldRank:%d, NewRank:%d", old_rank, new_rank);
    return;
  }

  ModifyCurrency money(MSG_CS_REQUEST_TEST_PK_TARGET, SYSTEM_ID_PKMAIN);

  while (old_rank > new_rank) {
    if (new_rank >= (old_base - 1)->second.id) {
      int32_t delta = (old_rank - new_rank) * old_base->second.gold / 100;
      DEBUG_LOG(logger)("OldRank:%d => NewRank:%d, +%d", old_rank, new_rank, delta);
      money.money += delta;
      old_rank = new_rank;
    } else {
      int32_t delta =  (old_rank - (old_base - 1)->second.id) * old_base->second.gold / 100;
      DEBUG_LOG(logger)("OldRank:%d => NewRank:%d, +%d", old_rank, (old_base - 1)->second.id, delta);
      money.money += delta;
      old_rank = (old_base - 1)->second.id;
    }
    --old_base;
  }

  this->UpdateCurrency(money);
  MessageNotifyPkPerformance notify;
  notify.set_old_rank(rank);
  notify.set_new_rank(new_rank);
  notify.set_money(money.money);
  this->SendMessageToClient(MSG_CS_NOTIFY_PK_PERFORMANCE, &notify);

  TRACE_LOG(logger)("PlayerID:%ld, OldRank:%d NewRank:%d, Money:%d"
      , this->uid() , rank, new_rank, money.money);
}

void LogicPlayer::RefreshPlayerPKTargets(int64_t player, int32_t new_rank,
                                         VectorMap<int32_t, int64_t>& targets) {
  if (player < sy::MAX_ROBOT_ID) return;

  LogicPlayer *dest_player = server->GetPlayerByID(player);
  if (dest_player) {
    dest_player->pk_current_rank_ = new_rank;
    dest_player->pk_targets_ = targets;
    dest_player->SendMessageToClient(MSG_CS_NOTIFY_ARENA_RANK_CHANGED, NULL);
  }

  MessageSSUpdatePKRankInfo message;
  message.set_rank(new_rank);
  message.set_player_id(player);
  message.set_server(server_config->server_id());
  for (PkTargetType::const_iterator iter = targets.begin();
       iter != targets.end(); ++iter) {
    sy::PKRankInfo* info = message.add_rank_info();
    info->set_rank(iter->first);
    info->set_player_id(0);
  }

  this->SendMessageToDB(MSG_SS_UPDATE_PK_RANK_INFO, &message);
}

void LogicPlayer::Send21ClockPkReward(int32_t rank, int32_t time) {
  this->pk_rank_reward_rank_ = rank;
  this->pk_rank_reward_time_ = time;

  MessageNotifyDailyPKRankInfo notify;
  notify.set_pk_rank_reward_rank(rank);
  notify.set_pk_rank_reward_time(0);
  this->SendMessageToClient(MSG_CS_NOTIFY_DAILY_RANK_INFO, &notify);
}

void LogicPlayer::SendTowerState() {
  MessageNotifyTowerState notify;
  notify.mutable_state()->CopyFrom(this->tower_state_);
  for (VectorMap<int16_t, int16_t>::const_iterator iter =
           this->tower_buff_.begin();
       iter != this->tower_buff_.end(); ++iter) {
    sy::KVPair2* info = notify.add_tower_buff();
    info->set_key(iter->first);
    info->set_value(iter->second);
  }
  this->SendMessageToClient(MSG_CS_NOTIFY_TOWER_STATE, &notify);
  MessageSSUpdateTowerInfo message;
  message.mutable_state()->CopyFrom(this->tower_state_);
  message.mutable_buff()->CopyFrom(notify.tower_buff());
  this->SendMessageToDB(MSG_SS_UPDATE_TOWER_INFO, &message);
}

void LogicPlayer::OnBuyCount(int32_t count_type) {
  switch (count_type) {
    case sy::COUNT_TYPE_TOWER:
      this->OnBuyTowerCount();
      break;
    case sy::COUNT_TYPE_DAILY_CARRIER_COPY_RESET:
      this->OnBuyCarrierCopyCount();
      break;
    case sy::COUNT_TYPE_CROSS_BUY_SERVER_RANDOM:
      this->CrossServerQueryPlayers(MSG_CS_REQUEST_CROSS_SERVER_RANDOM_PLAYER);
      break;
  }
}

void LogicPlayer::OnBuyTowerCount() {
  INFO_LOG(logger)("Before Reset TowerState, PlayerID:%ld, max_star_order:%d"
        , this->uid(), this->tower_state_.max_star_order());
  this->tower_state_.set_award(0);
  this->tower_state_.set_buff_star(0);
  this->tower_state_.set_copy_star(0);
  this->tower_state_.set_current_star(0);
  this->tower_state_.set_current_order(0);
  this->tower_state_.set_random_buff("");
  this->tower_buff_.clear();
  this->current_carrier_.clear_tower_attr1();
  this->SendTowerState();
  INFO_LOG(logger)("After Reset TowerState, PlayerID:%ld, max_star_order:%d"
        , this->uid(), this->tower_state_.max_star_order());

  UpdateAchievement(SEVEN_DAY_TYPE_TOWER_RESET,
                    this->achievements_[SEVEN_DAY_TYPE_TOWER_RESET] + 1);
  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_TOWER, 1, this);

  MessageNotifyCurrentCarrierInfo msg;
  msg.mutable_info()->CopyFrom(this->current_carrier_);
  this->SendMessageToClient(MSG_CS_NOTIFY_CURRENT_CARRIER_INFO, &msg);
}

void LogicPlayer::OnBuyCarrierCopyCount() {
  int32_t item_max_count = this->carrier_copy_info_.item_max_count();
  this->carrier_player_.clear();
  server->RandomTactic(this, 1, this->carrier_copy_);
  this->carrier_copy_info_.Clear();
  this->carrier_copy_info_.set_layer(1);
  this->carrier_copy_info_.set_item_max_count(item_max_count);
  this->UpdateCarrierCopy(true, true);
}

int32_t LogicPlayer::ProcessRequestBuyCount(CSMessageEntry& entry) {
  MessageRequestBuyCount* message =
      static_cast<MessageRequestBuyCount*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const int32_t count_type = message->count_type();
  int32_t current_count = abs(this->daily_counter_[count_type]);
  int32_t mapped_count_type = count_type;
  int32_t buy_count = message->buy_count() ? message->buy_count() : 1;

  if (buy_count < 1 || buy_count > 100) return ERR_PARAM_INVALID;

  //是购买副本
  if (mapped_count_type >= sy::COUNT_TYPE_SPECIFIC_COPY_BEGIN) {
    const CopyBase* base = COPY_BASE.GetEntryByID(count_type).get();
    if (!base || (base->type != sy::COPY_TYPE_NORMAL &&
                  base->type != sy::COPY_TYPE_HARD)) {
      return ERR_PARAM_INVALID;
    }
    mapped_count_type = sy::COUNT_TYPE_SPECIFIC_COPY_BEGIN;
  }

  const BuyCountCostBase* base =
      BUY_COUNT_BASE.GetEntryByID(mapped_count_type).get();
  if (!base) return ERR_PARAM_INVALID;
  int32_t max_count = base->GetBuyCount(this->vip_level());
  if (max_count >= 0 && current_count + buy_count > max_count)
    return ERR_CURRENCY_VIP_EXP;

  int64_t cost = 0;
  for (int32_t i = current_count; i < current_count + buy_count; ++i)
    cost += base->GetBuyCost(i + 1);

  ModifyCurrency money(MSG_CS_REQUEST_BUY_COUNT, SYSTEM_ID_BASE);
  money[base->type] = -cost;

  CHECK_RET(this->CheckCurrency(money));
  this->UpdateCurrency(money);

  UpdateBuyCount(count_type, buy_count);
  this->OnBuyCount(count_type);

  MessageResponseBuyCount response;
  response.set_count_type(count_type);
  response.set_bought_count(this->daily_counter_[count_type]);
  this->SendMessageToClient(MSG_CS_RESPONSE_BUY_COUNT, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestTalk(CSMessageEntry& entry) {
  MessageRequestTalk* message = static_cast<MessageRequestTalk*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (this->level() < GetSettingValue(chat_min_lv))
    return ERR_PLAYER_LEVEL;

  if (message->content().empty() ||
      message->content().length() >= 256u)
    return ERR_PARAM_INVALID;

  if (this->player().status() == 1 &&
      GetSeconds() <= this->player().status_time())
    return ERR_BAN_TALK;

  if (server_config->dirtywords())
    ReplaceDirtyWords(*message->mutable_content());

  MessageNotifyTalk notify;
  notify.set_channel(message->channel());
  notify.set_content(message->content());

  this->MakePlayerSimpleInfo(notify.mutable_player());

  switch (message->channel()) {
    case TALK_CHANNEL_SERVER:
#ifdef __EGMKANG_DEBUG
      if (message->content() == "***1" ||
          message->content() == "***2" ||
          message->content() == "***3" ||
          message->content() == "***4") {

        ModifyCurrency modify(0, 0);
        modify[MONEY_KIND_MONEY] += 1000000;
        modify[MONEY_KIND_COIN] += 100000000;
        modify[MONEY_KIND_HERO] += 1000000;
        modify[MONEY_KIND_PLANE] += 1000000;
        modify[MONEY_KIND_PRESTIGE] += 1000000;
        modify[MONEY_KIND_MUSCLE] += 1000000;
        modify[MONEY_KIND_EXPLOIT] += 1000000;
        modify[MONEY_KIND_UNION] += 1000000;
        this->UpdateCurrency(modify);

        AddSubItemSet item_set;
        item_set.push_back(ItemParam(23900657, 1000));  //天命石
        item_set.push_back(ItemParam(23900539, 1000));  //飞机礼包
        item_set.push_back(ItemParam(23900033, 1000000));  //船经验
        item_set.push_back(ItemParam(23900034, 1000000));  //船突破
        item_set.push_back(ItemParam(23900446, 1000));  //红装礼包
        item_set.push_back(ItemParam(23900116, 100000));  //红船零件
        this->ObtainItem(&item_set, NULL, NULL, 0, 0);
      }
      if (message->content() == "next") {
        static CSMessageEntry e;
        this->carrier_copy_info_.set_passed_copy(kCarrierCopyLevel4);
        this->ProcessRequestCarrierCopyNextLevel(e);
        this->UpdateCarrierCopy(true, true);
        return ERR_OK;
      }
      if (message->content() == "buy") {
        this->OnBuyCarrierCopyCount();
        return ERR_OK;
      }
      if (message->content() == "L1") {
        this->SendMessageToSelf(MSG_CS_REQUEST_GET_LEGION_WAR_PLAYER, kEmptyMessage);
        return ERR_OK;
      }
      if (message->content() == "L2") {
        this->SendMessageToSelf(MSG_CS_REQUEST_LEGION_WAR_REGISTER, kEmptyMessage);
        return ERR_OK;
      }
      if (message->content() == "L3") {
        LegionWar::Instance().GetPlayerTarget(this->uid(), true);
        return ERR_OK;
      }
      if (message->content() == "rand") {
        this->CrossServerQueryPlayers(0);
        return ERR_OK;
      }
#endif
#ifdef DEBUG
      if (message->content() == "refresh_army_war") {
        Army* army = server->GetArmyByID(this->army_id());
        if (army) army->ResetArmyWarFreshTime();
        return ERR_OK;
      }
      if (message->content() == "legion_war_score") {
        server->SendLegionWarScore();
      }
      if (message->content() == "legion_war_award") {
        server->SendLegionWarAward();
      }
      if (message->content() == "refresh_come_back") {
        this->come_back_info_.set_login_record(0);
        this->come_back_info_.set_award_record(0);
        server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypePlayerComeBack,
                                  this->come_back_info_.SerializeAsString());
        MessageNotifyHeroComeBackInfo notify;
        notify.mutable_info()->CopyFrom(this->come_back_info_);
        this->SendMessageToClient(MSG_CS_NOYIFY_COME_BACK_INFO, &notify);
      }
#endif
      if (server_config->test_gm()) {
        if (message->content() == "player") {
          server->SendServerMessageToDB(MSG_SS_REQUEST_QUERY_LIVELY_ACCOUNT,
                                        NULL);
          return ERR_OK;
        }
      }
      if ((GetSeconds() - this->chat_time_) >
          GetSettingValue(chat_cd_time) - 1) {
        server->SendMessageToAllClient(MSG_CS_NOTIFY_TALK, &notify);
        this->chat_time_ = GetSeconds();
      }
      break;
    case TALK_CHANNEL_PRIVATE: {
      LogicPlayer* player = server->GetPlayerByID(message->player());
      if (!player) return ERR_PLAYER_OFFLINE;
      if (FRIEND_STATUS_BLACKMAIL != player->GetRelationship(this->uid()))
        player->SendMessageToClient(MSG_CS_NOTIFY_TALK, &notify);
    } break;
    case TALK_CHANNEL_ARMY: {
      Army* army = server->GetArmyByID(this->army_id_);
      if (!army) return ERR_ARMY_NOT_JOINED;
      army->SendMessageToArmy(MSG_CS_NOTIFY_TALK, &notify);
    } break;
    default:
      ERROR_LOG(logger)("%s Channel:%d not processed", __FUNCTION__, message->channel());
      break;
  }

  MessageResponseTalk response;
  response.set_channel(message->channel());
  if (message->has_player()) response.set_player(message->player());
  response.set_content(message->content());
  this->SendMessageToClient(MSG_CS_RESPONSE_TALK, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestStartPatrol(CSMessageEntry& entry) {
  MessageRequestStartPatrol* message =
      static_cast<MessageRequestStartPatrol*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const PatrolInfo& msg_info = message->patrol_info();

  PatrolBase* base = PATROL_BASE.GetEntryByID(msg_info.patrol_id()).get();
  if (!base) return ERR_PARAM_INVALID;
  LogicHero* ship = this->GetHeroByUID(msg_info.ship_uid());
  if (!ship) return ERR_HERO_NOT_FOUND;

  int32_t ship_id = ship->second->id();

  //巡逻点是否可开启
  if (!GetPassedCopy(base->mapped_id)) return ERR_COPY_COUNT;

  //巡逻点是否被占用
  for (VectorMap<int32_t, sy::PatrolInfo>::const_iterator it =
           patrol_infos_.begin();
       it != patrol_infos_.end(); ++it) {
    if (it->second.ship_uid() == ship_id) return ERR_PATROL_SHIP_PATROLLING;
    if (it->first == base->id() && it->second.ship_uid())
      return ERR_PATROL_TAKE_UP;
  }

  //巡逻类型(时间)
  const std::vector<int32_t>& patrol_time_vct =
      Setting::GetValue1(Setting::patrol_time);
  if (msg_info.patrol_type() >= static_cast<int32_t>(patrol_time_vct.size()))
    return ERR_PARAM_INVALID;
  int32_t patrol_time_hour = patrol_time_vct[msg_info.patrol_type()];

  //巡逻模式(等级)
  const std::vector<int32_t>& patrol_awardtime_vct =
      Setting::GetValue1(Setting::patrol_awardtime);
  if (msg_info.patrol_mode() >=
      static_cast<int32_t>(patrol_awardtime_vct.size()))
    return ERR_PARAM_INVALID;
  VipFunctionBase* vip_base1 = VIP_FUNCTION_BASE.GetEntryByID(360).get();
  VipFunctionBase* vip_base2 = VIP_FUNCTION_BASE.GetEntryByID(361).get();
  if (!vip_base1 || !vip_base2) return ERR_INTERNAL;
  if (msg_info.patrol_mode() == 1) {
    if (!vip_base1->GetValue(this->vip_level(), this->level()))
      return ERR_CURRENCY_VIP_EXP;
  }
  if (msg_info.patrol_mode() == 2) {
    if (!vip_base2->GetValue(this->vip_level(), this->level()))
      return ERR_CURRENCY_VIP_EXP;
  }
  int32_t patrol_awardtime = patrol_awardtime_vct[msg_info.patrol_mode()];
  const ValuePair2Vec* award_ptr = NULL;
  if (0 == msg_info.patrol_mode())
    award_ptr = &Setting::GetValue2(Setting::patrol_1_award);
  if (1 == msg_info.patrol_mode())
    award_ptr = &Setting::GetValue2(Setting::patrol_2_award);
  if (2 == msg_info.patrol_mode())
    award_ptr = &Setting::GetValue2(Setting::patrol_3_award);
  if (NULL == award_ptr) return ERR_PARAM_INVALID;

  //消耗判断
  ValuePair2Vec cost = Setting::GetValue2(Setting::patrol_cost);
  if (msg_info.patrol_mode() >= (int)cost.size()) return ERR_PARAM_INVALID;
  ModifyCurrency modify(MSG_CS_REQUEST_START_PATROL, SYSTEM_ID_HOOK);
  modify[cost[msg_info.patrol_mode()].v1] =
      -cost[msg_info.patrol_mode()].v2 * patrol_time_hour;
  int32_t result = CheckCurrency(modify);
  if (result) return result;

  //更新巡逻点信息
  PatrolInfo& info = patrol_infos_[msg_info.patrol_id()];
  info.set_patrol_id(msg_info.patrol_id());
  info.set_ship_uid(ship_id);
  info.set_patrol_type(msg_info.patrol_type());
  info.set_patrol_mode(msg_info.patrol_mode());
  info.set_patrol_start_time(GetVirtualSeconds());

  //固定碎片奖励
  std::vector<int32_t> out;
  LootByRandomCommon(*award_ptr, 1, out);
  if (out.empty()) return ERR_PARAM_INVALID;
  sy::KVPair2* pair = info.add_patrol_awards();
  pair->set_key(ship->second->ship_piece.v1);
  pair->set_value(patrol_time_hour / 4 * out[0]);

  //随机掉落物品
  VectorMap<int32_t, int32_t> awards_temp;
  int32_t award_count = patrol_time_hour * 60 / patrol_awardtime - 1;

  result = PatrolBase::GeneratePatrolAward(
      award_count, this->level(), info.patrol_level(), base, awards_temp);
  if (result) return result;

  ModifyCurrency modify_temp(0, 0);
  AddSubItemSet item_set_temp;
  LootBase* loot = LootConfigFile::Get(base->ship_award, this->level()).get();
  if (!loot) return ERR_PARAM_INVALID;
  loot->Loot(modify_temp, item_set_temp, NULL);
  for (AddSubItemSet::iterator it = item_set_temp.begin();
       it != item_set_temp.end(); ++it) {
    awards_temp[it->item_id] += it->item_count;
  }
  for (int32_t i = MoneyKind_MIN; i < MoneyKind_ARRAYSIZE; ++i) {
    if (modify_temp[i]) awards_temp[i] += modify_temp[i];
  }

  for (VectorMap<int32_t, int32_t>::iterator it = awards_temp.begin();
       it != awards_temp.end(); ++it) {
    sy::KVPair2* pair = info.add_patrol_awards();
    pair->set_key(it->first);
    pair->set_value(it->second);
  }

  //扣除精力
  UpdateCurrency(modify);

  UpdateAchievement(
      SEVEN_DAY_TYPE_PATROL_TOTLE,
      this->achievements_[SEVEN_DAY_TYPE_PATROL_TOTLE] + patrol_time_hour);
  if (2 == msg_info.patrol_mode())
    UpdateAchievement(
        SEVEN_DAY_TYPE_PATROL_ADVANCE,
        this->achievements_[SEVEN_DAY_TYPE_PATROL_ADVANCE] + patrol_time_hour);

  MessageResponseStartPatrol response;
  UpdatePatrolinfo(base->id(), response.mutable_patrol_info());
  SendMessageToClient(MSG_CS_RESPONSE_START_PATROL, &response);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetPatrolAwards(CSMessageEntry& entry) {
  MessageRequestGetPatrolAwards* message =
      static_cast<MessageRequestGetPatrolAwards*>(entry.get());
  if (!message) return ERR_INTERNAL;

  AddSubItemSet item_set;
  ModifyCurrency modify(MSG_CS_REQUEST_GET_PATROL_AWARDS, SYSTEM_ID_HOOK);

  MessageResponseGetPatrolAwards response;

  int32_t patrol_id = message->patrol_id();
  VectorMap<int32_t, sy::PatrolInfo>::iterator info_it =
      patrol_infos_.find(patrol_id);
  if (info_it == patrol_infos_.end()) return ERR_PARAM_INVALID;

  if (info_it->second.ship_uid() == 0) return ERR_PARAM_INVALID;

  int32_t patrol_time_hour = Setting::GetValue1(Setting::patrol_time)
                                 .at(info_it->second.patrol_type());

  if ((GetVirtualSeconds() - info_it->second.patrol_start_time()) <
      patrol_time_hour * 3600)
    return ERR_PARAM_INVALID;

  for (int32_t i = 0; i < info_it->second.patrol_awards_size(); ++i) {
    sy::KVPair2 pair = info_it->second.patrol_awards(i);
    if (pair.key() < 100)
      modify[pair.key()] += pair.value();
    else
      item_set.push_back(ItemParam(pair.key(), pair.value()));
  }

  patrol_total_time_ += patrol_time_hour;

  response.set_patrol_id(patrol_id);

  ResetPatrolInfo(patrol_id);

  response.set_total_time(patrol_total_time_);

  int32_t result = CheckItem(&item_set, NULL, NULL);
  if (result) return result;
  result = CheckCurrency(modify);
  if (result) return result;

  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_PATROL_AWARDS,
             SYSTEM_ID_HOOK);
  UpdateCurrency(modify);

  UpdateBuyCount(COUNT_TYPE_DAILY_PATROL, patrol_time_hour);

  SendMessageToClient(MSG_CS_RESPONSE_GET_PATROL_AWARDS, &response);
  UpdatePatrolinfo(0, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPatrolLevelUp(CSMessageEntry& entry) {
  MessageRequestPatrolLevelUp* message =
      static_cast<MessageRequestPatrolLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  PatrolBase* base = PATROL_BASE.GetEntryByID(message->patrol_id()).get();
  if (!base) return ERR_PARAM_INVALID;

  PatrolInfo& it = patrol_infos_[message->patrol_id()];
  if (!it.patrol_id()) it.set_patrol_id(message->patrol_id());

  int32_t need_time = base->GetSkillTime(it.patrol_level());
  if (!need_time || need_time > this->patrol_total_time_)
    return ERR_PATROL_NEED_TIME;

  int32_t cost = base->GetSkillCost(it.patrol_level());
  if (0 == cost) return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_PATROL_LEVEL_UP, SYSTEM_ID_HOOK);
  modify[MONEY_KIND_MONEY] = -cost;

  int32_t result = CheckCurrency(modify);
  if (result) return result;

  it.set_patrol_level(it.patrol_level() + 1);

  MessageResponsePatrolLevelUp response;
  UpdatePatrolinfo(it.patrol_id(), response.mutable_patrol_info());
  SendMessageToClient(MSG_CS_RESPONSE_PATROL_LEVEL_UP, &response);

  return ERR_OK;
}

void LogicPlayer::UpdatePatrolinfo(int32_t patrol_id, sy::PatrolInfo* out) {
  MessageSSUpdatePatrolInfo msg;
  msg.set_total_time(this->patrol_total_time_);

  for (VectorMap<int32_t, sy::PatrolInfo>::iterator it = patrol_infos_.begin();
       it != patrol_infos_.end(); ++it) {
    if (out && it->second.patrol_id() == patrol_id) out->CopyFrom(it->second);

    sy::PatrolInfo* info = msg.add_infos();
    info->CopyFrom(it->second);
  }

  this->SendMessageToDB(MSG_SS_UPDATE_PATROL_INFO, &msg);
}

int32_t LogicPlayer::AgreeAddFriend(sy::MessageRequestAgreeAddFriend* message) {
  int32_t kFriendMax = GetSettingValue(friend_max);
  sy::FriendInfo my_info;
  this->FillFriendInfo(my_info);
  my_info.set_type(FRIEND_STATUS_FRIEND);

  sy::FriendInfo *info = this->GetFriendInfoByID(message->friend_id());
  if (!info || info->type() != FRIEND_STATUS_UNAUDIT) return ERR_PARAM_INVALID;

  if (message->op_type() == 1) {
    //对方好友上限
    int32_t my_friend_count = this->GetFriendCountByType(FRIEND_STATUS_FRIEND);
    int32_t friend_count = server->GetFriendCount(message->friend_id());
    //同意添加好友
    if (my_friend_count >= kFriendMax) return ERR_PLAYER_FRIEND_MAX_1;
    if (friend_count >= kFriendMax) return ERR_PLAYER_FRIEND_MAX_2;
    info->set_type(FRIEND_STATUS_FRIEND);
    LogicPlayer *player = server->GetPlayerByID(message->friend_id());
    if (player) {
      sy::FriendInfo* friend_info = player->GetFriendInfoByID(this->uid());
      if (friend_info) friend_info->CopyFrom(my_info);
      else player->friends_.add_infos()->CopyFrom(my_info);

      MessageNotifyFriendInfo notify;
      notify.add_friend_()->CopyFrom(my_info);
      player->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);
    }

    MessageNotifyFriendInfo notify;
    notify.add_friend_()->CopyFrom(*info);
    this->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);

    MessageSSUpdateFriendInfo update;
    update.add_friends()->CopyFrom(*info);
    update.set_player_id(this->uid());
    this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);

    update.Clear();
    update.set_player_id(info->friend_id());
    update.add_friends()->CopyFrom(my_info);
    this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);

    DefaultArrayStream stream;
    stream.Append("%ld,%s", this->uid(), this->name().c_str());
    this->SendMail(info->friend_id(), GetSeconds(), sy::MAIL_TYPE_ADD_FRIEND,
                   stream.str(), NULL);
    server->SetFriendCount(this->uid(), my_friend_count + 1);
    server->SetFriendCount(message->friend_id(), friend_count + 1);
  } else {
    //不同意
    this->DeleteFriend(message->friend_id());
    MessageSSUpdateFriendInfo update;
    update.set_delete_friend(message->friend_id());
    update.set_player_id(this->uid());
    this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessAddFriend(int64_t uid) {
  if (this->GetFriendCountByType(FRIEND_STATUS_FRIEND) >= GetSettingValue(friend_max))
    return ERR_PLAYER_FRIEND_MAX_1;
  //不能自己加自己为好友
  if (this->uid() == uid) return ERR_PLAYER_NON_SELF;

  sy::FriendInfo* info = this->GetFriendInfoByID(uid);
  if (info) {
    if (info->type() != FRIEND_STATUS_UNAUDIT) return ERR_PLAYER_FRIEND_EXISTS;
    static MessageRequestAgreeAddFriend request;
    request.set_friend_id(uid);
    request.set_op_type(1);
    return this->AgreeAddFriend(&request);
  }
  sy::FriendInfo my_info;
  this->FillFriendInfo(my_info);
  my_info.set_type(FRIEND_STATUS_UNAUDIT);

  LogicPlayer *player = server->GetPlayerByID(uid);
  if (player) {
    sy::FriendInfo* info = player->GetFriendInfoByID(this->uid());
    //你被对面拉黑, 后续不需要操作
    if (info && info->type() == FRIEND_STATUS_BLACKMAIL) return ERR_OK;
    if (info) info->CopyFrom(my_info);
    else player->friends_.add_infos()->CopyFrom(my_info);

    MessageNotifyFriendInfo notify;
    notify.add_friend_()->CopyFrom(my_info);
    player->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);
  }

  MessageSSUpdateFriendInfo update;
  update.set_player_id(uid);
  update.add_friends()->CopyFrom(my_info);
  this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessDeleteFriend(int64_t uid) {
  //是好友才能双方都删除
  sy::FriendInfo* info = this->GetFriendInfoByID(uid);
  if (info && info->type() == FRIEND_STATUS_FRIEND) {
    LogicPlayer* player = server->GetPlayerByID(uid);
    if (player) player->DeleteFriend(this->uid());
  }

  this->DeleteFriend(uid);

  //通过this删除this和player的好友
  MessageSSUpdateFriendInfo update;
  update.set_player_id(this->uid());
  update.set_delete_friend(uid);
  this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);
  int32_t my_friend_count = this->GetFriendCountByType(FRIEND_STATUS_FRIEND);
  server->SetFriendCount(this->uid(), my_friend_count);
  int32_t friend_count = server->GetFriendCount(uid);
  server->SetFriendCount(uid, friend_count > 0 ? friend_count : 0);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessBanAFriend(sy::FriendInfo& info) {
  if (this->GetFriendCountByType(FRIEND_STATUS_BLACKMAIL) >=
      GetSettingValue(friend_black_max))
    return ERR_PLAYER_FRIEND_MAX_3;
  if (this->GetFriendInfoByID(info.friend_id()))
    ProcessDeleteFriend(info.friend_id());
  //然后添加拉黑
  info.set_type(FRIEND_STATUS_BLACKMAIL);
  this->friends_.add_infos()->CopyFrom(info);

  MessageNotifyFriendInfo notify;
  notify.add_friend_()->CopyFrom(info);
  this->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);

  MessageSSUpdateFriendInfo update;
  update.set_player_id(this->uid());
  update.add_friends()->CopyFrom(info);
  this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestAddFriend(CSMessageEntry& entry) {
  MessageRequestAddFriend* message = static_cast<MessageRequestAddFriend*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->has_add_friend()) CHECK_RET(this->ProcessAddFriend(message->add_friend()));
  if (message->has_delete_friend()) CHECK_RET(this->ProcessDeleteFriend(message->delete_friend()));
  if (message->has_blackmail_user()) CHECK_RET(this->ProcessBanAFriend(*message->mutable_blackmail_user()));

  MessageResponseAddFriend response;
  if (message->has_add_friend()) response.set_add_friend(message->add_friend());
  this->SendMessageToClient(MSG_CS_RESPONSE_ADD_FRIEND, &response);
  return ERR_OK;
}

int32_t LogicPlayer::GetFriendCountByType(int32_t type) {
  int32_t count  = 0;
  for (int32_t i = 0; i < this->friends_.infos_size(); ++i) {
    const sy::FriendInfo& info = this->friends_.infos(i);
    if (info.type() == type) ++count;
  }
  return count;
}

int32_t LogicPlayer::ProcessRequestAgreeAddFriend(CSMessageEntry& entry) {
  MessageRequestAgreeAddFriend* message =
      static_cast<MessageRequestAgreeAddFriend*>(entry.get());
  if (!message) return ERR_INTERNAL;

  return this->AgreeAddFriend(message);
}

int32_t LogicPlayer::FillFriendInfo(sy::FriendInfo& info) {
  if (!this->load_complete()) return 0;

  int32_t patrol_id = 0;
  sy::CopyProgress* progress = this->GetCopyProgress(COPY_TYPE_AUTO_FIGHT);
  if (progress) patrol_id = progress->copy_id();

  int32_t change =
      (this->level() != info.level()             )
    + (this->vip_level() != info.vip_level()     )
    + (this->avatar() != info.avatar()           )
    + (this->fight_attr() != info.score()        )
    + (this->name() != info.name()               )
    + (patrol_id != info.patrol_id()             )
    + (this->player_.rank_id() != info.rank_id() )
    ;
  if (change) {
    info.set_friend_id(this->uid());
    info.set_level(this->level());
    info.set_vip_level(this->vip_level());
    info.set_avatar(this->avatar());
    info.set_score(this->fight_attr());
    info.set_name(this->name());
    info.set_last_active_time(this->last_update_time_);
    info.set_patrol_id(patrol_id);
    info.set_rank_id(this->player_.rank_id());
  }
  return change;
}

int32_t LogicPlayer::ProcessRequestGetFriendInfo(CSMessageEntry& entry) {
  MessageSSUpdateFriendInfo update;
  update.set_player_id(this->uid());
  MessageResponseLoadFriend response;
  for (int32_t i = 0; i < this->friends_.infos_size(); ++i) {
    sy::FriendInfo& info = *this->friends_.mutable_infos(i);
    LogicPlayer *player = server->GetPlayerByID(info.friend_id());
    if (player && player->FillFriendInfo(info)) {
      update.add_friends()->CopyFrom(info);
    }
    response.add_infos()->CopyFrom(info);
    //如果好友在线, 那么设置活动时间为0
    if (player && player->is_online())
      response.mutable_infos()->rbegin()->set_last_active_time(0);
  }

  if (update.friends_size()) this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);
  this->SendMessageToClient(MSG_CS_RESPONSE_LOAD_FRIEND, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetFriendInfo(SSMessageEntry& entry) {
  MessageSSGetFriendInfo* message =
      static_cast<MessageSSGetFriendInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->friends_.Clear();

  for (int32_t i = 0; i < message->infos_size(); ++i) {
    sy::FriendInfo info = message->infos(i);
    this->friends_.add_infos()->CopyFrom(info);
  }
  return ERR_OK;
}

void LogicPlayer::ResetPatrolInfo(int32_t patrol_id)
{
  VectorMap<int32_t, sy::PatrolInfo>::iterator it =
      patrol_infos_.find(patrol_id);
  if (patrol_infos_.end() == it) return;

  it->second.set_ship_uid(0);
  it->second.set_patrol_type(0);
  it->second.set_patrol_mode(0);
  it->second.clear_patrol_awards();
  it->second.set_patrol_start_time(0);
}

int32_t LogicPlayer::ProcessRequestLogOut(CSMessageEntry& entry) {
  this->OnPlayerLogOut();
  return ERR_OK;
}

void LogicPlayer::OnPlayerLogIn() {
  this->last_update_time_ = GetVirtualSeconds();
  MessageCache::Instance().ClearRawMessage(this->uid());
  if (!this->channel_.empty()) {
    MessageSSUpdateLoginTime update;
    update.set_channel(this->channel_);
    this->SendMessageToDB(MSG_SS_UPDATE_LOGIN_TIME, &update);
  }

  this->CalcRank();
  this->CalcTacticAttr(kNotifyNone, 0, 0);
  this->CalcTowerAttr();

  DEBUG_LOG(logger)("PlayerID:%ld, DeviceID:%s, IDFA:%s"
      , this->uid(), this->device_id_.c_str(), this->idfa_.c_str());
  if (!this->device_id_.empty() && !this->idfa_.empty() &&
      this->player_.device_id() != this->idfa_) {
    this->player_.set_device_id(this->idfa_);

    MessageSSUpdateDeviceID update;
    update.set_device_id(this->device_id_);
    update.set_idfa(this->idfa_);
    this->SendMessageToDB(MSG_SS_UPDATE_DEVICE_ID, &update);
  }

  //玩家有可能是被其他人拉起来的
  if (this->is_online()) {
    const TcpSessionPtr& session = this->session().lock();
    MessageSSUpdateLoginInfo msg;
    msg.set_tid(server->GetTID());
    msg.set_online_time(0);
    msg.set_login(1);
    msg.set_ipaddr(session ? session->IpAddr() : "");
    this->SendMessageToDB(MSG_SS_UPDATE_LOGIN_INFO, &msg);
    this->UpdateArmyMember();
    RANK_LIST.OnPlayerLevelUp(this);

    MessageSSRequestPlayerLogin request;
    request.set_player_id(this->uid());
    request.set_server_id(this->player().server());
    server->SendMessageToCenter(MSG_SS_REQUEST_PLAYER_LOGIN, &request);

    do {
      if (!this->army_id_) break;
      Army* army = server->GetArmyByID(this->army_id_);
      if (!army) break;
      sy::ArmyMemberInfo* member = army->GetMember(this->uid());
      if (!member) break;
      member->set_update_time(GetVirtualSeconds());
      army->UpdateMember(this->uid());
    } while (false);
  }

  this->online_time_ = 0;
  this->SendMessageToDB(MSG_SS_REQUEST_GET_MAIL, NULL);
  if (this->session().expired()) return;
  DEBUG_LOG(logger)("PlayerLogIn, PlayerID:%ld", this->uid());

  for (VectorSet<int32_t>::const_iterator it = ItemBase::navy_item.begin();
       it != ItemBase::navy_item.end(); ++it) {
    if (this->items_.GetItemByItemID(*it))
      server->InsertIntoRobList(*it, this->uid());
  }
}

void LogicPlayer::OnPlayerLogOut() {
  const TcpSessionPtr& session = this->session().lock();
  if (session) session->SetUID(0);
  DEBUG_LOG(logger)("PlayerLogOut, PlayerID:%ld", this->uid());
  if (this->online_time_) {
    MessageSSUpdateLoginInfo msg;
    msg.set_tid(server->GetTID());
    msg.set_online_time(this->online_time_);
    msg.set_login(2);
    this->SendMessageToDB(MSG_SS_UPDATE_LOGIN_INFO, &msg);
    this->UpdateArmyMember();
  }

  foreplay_players_.erase(this->uid());
  world_boss_players_.erase(this->uid());

  server->EraseFromRobList(0, this->uid());
  this->online_time_ = 0;

  //没有创角的玩家, 立即删除玩家对象
  if (this->create_player()) {
    server->ErasePlayer(this->uid());
  }
}

void LogicPlayer::SendReportToDB(int64_t report_id,
                                 sy::MessageResponseFightReport* resp) {
  MessageResponseFightReport& response = *resp;

  char buffer[SS_MSG_MAX_LEN];
  CSHead* head = (CSHead*)(buffer);
  head->msgid = MSG_CS_RESPONSE_FIGHT_REPORT;
  head->length = response.ByteSize();
  int32_t msg_len = sizeof(*head) + head->real_length();
  response.SerializeToArray(&buffer[0] + sizeof(*head), msg_len);

  std::string zipped_buffer;
  zipped_buffer.assign(buffer, buffer + msg_len);

  DefaultArrayStream stream;
  stream.Append("ReportID:%ld", report_id);
  storage::Set(stream.str(), zipped_buffer);
  DEBUG_LOG(logger)("ProcessSaveReport %s", stream.c_str());
}

int32_t LogicPlayer::ProcessRequestGetReprot(CSMessageEntry& entry) {
  MessageRequestFightReport* message =
      static_cast<MessageRequestFightReport*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.Append("ReportID:%ld", message->report_id());
  const std::string& report = storage::Get(stream.str());
  DEBUG_LOG(logger)("ProcessGetReport %s", stream.c_str());

  entry.session_ptr->SendCompressedMessage(MSG_CS_RESPONSE_FIGHT_REPORT,
                                           const_cast<char*>(report.c_str()),
                                           report.length());

  return ERR_OK;
}

void LogicPlayer::SendArenaReportResult(int64_t player1, int64_t player2,
                                        const std::string& report,
                                        int32_t report_type) {
  MessageNotifyNewReport notify;
  notify.set_report_abstract(report);
  int64_t playerids[] = {player1, player2};

  for (int32_t i = 0; i < ArraySize(playerids); ++i) {
    int64_t player_id = playerids[i];
    LogicPlayer *player = player_id > MAX_ROBOT_ID ? server->GetPlayerByID(player_id) : NULL;
    if (player) {
      player->report_abstract_[sy::REPORT_TYPE_ARENA].push_back(report);
      if (player->report_abstract_[sy::REPORT_TYPE_ARENA].size() >
          sy::MAX_REPORT_ABSTRACT_COUNT) {
        player->report_abstract_[sy::REPORT_TYPE_ARENA].pop_front();
      }
      player->SendMessageToClient(MSG_CS_NOTIFY_NEW_REPORT, &notify);
    }
  }

  if (player1 > MAX_ROBOT_ID || player2 > MAX_ROBOT_ID) {
    server->IncTID();
    MessageSSUpdateReportAbstract request;
    request.set_report_uid(server->GetTID());
    request.set_player_1(player1 > MAX_ROBOT_ID ? player1 : 0);
    request.set_player_2(player2 > MAX_ROBOT_ID ? player2 : 0);
    request.set_report_type(report_type);
    request.set_report_content(report);
    this->SendMessageToDB(MSG_SS_UPDATE_REPORT_ABSTRACT, &request);
  }
}

int32_t LogicPlayer::ProcessRequestRecover(CSMessageEntry& entry) {
  MessageRequestRecover* message =
      static_cast<MessageRequestRecover*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->item_type() < 1 || message->item_type() > 4)
    return ERR_PARAM_INVALID;

  if (message->item_uid_size() > 5) return ERR_PARAM_INVALID;

  int32_t result = 0;

  ModifyCurrency modify(MSG_CS_REQUEST_RECOVER, SYSTEM_ID_RECOVER);
  if (!message->is_resolve()) {
    modify[MONEY_KIND_MONEY] -=
        GetSettingValue(recombine_cost) * message->item_uid_size();
    result = CheckCurrency(modify);
    if (result) return result;
  }

  MessageResponseRecover response;

  if (1 == message->item_type())
    result = RecoverEquip(message->item_uid(), message->is_resolve(),
                          response.mutable_recover_items());
  if (2 == message->item_type())
    result = RecoverNavy(message->item_uid(), response.mutable_recover_items());
  if (3 == message->item_type())
    result = RecoverShip(message->item_uid(), message->is_resolve(),
                         response.mutable_recover_items());
  if (4 == message->item_type())
    result = RecoverCarrier(message->item_uid(), message->is_resolve(),
                         response.mutable_recover_items());
  if (result) return result;

  UpdateCurrency(modify);

  response.set_item_type(message->item_type());
  SendMessageToClient(MSG_CS_RESPONSE_RECOVER, &response);

  return ERR_OK;
}

int32_t LogicPlayer::GetCopyBoughtCount(const CopyBase* copy_base) {
  if (copy_base->type == sy::COPY_TYPE_ELITE) {
    return this->daily_counter_[sy::COUNT_TYPE_ELITE_COPY];
  }
  if (copy_base->type == sy::COPY_TYPE_NORMAL ||
      copy_base->type == sy::COPY_TYPE_HARD) {
    return this->daily_counter_[copy_base->id()] * copy_base->times_day;
  }
  return 0;
}

int32_t LogicPlayer::CheckEnterCopy(const CopyBase* copy_base) {
  //名将副本总次数
  if (copy_base->type == COPY_TYPE_GENERAL &&
      Setting::GetGeneralCopyCount(this) >= GetSettingValue(general_day_times))
    return ERR_COPY_COUNT;
  if (this->level() < copy_base->lv) return ERR_CURRENCY_EXP;
  //副本禁入日期
  if ((1 << GetTime().tm_wday) & copy_base->forbid_day) return ERR_COPY_NOT_OPEN_DAY;
  //终生只能打一次的副本
  if (copy_base->times_one && this->GetPassedCopy(copy_base->id()))
    return ERR_COPY_PASSED;
  //判断体力
  if (this->player().oil() < copy_base->power) return ERR_CURRENCY_OIL;
  //次数
  int32_t mapped_copy_id = copy_base->mapped_copy_id;
  if (copy_base->id() != mapped_copy_id) {
    const CopyBase* base = COPY_BASE.GetEntryByID(mapped_copy_id).get();
    if (!base) {
      ERROR_LOG(logger)("CopyID:%ld MappedCopyID:%d is null", copy_base->id(), mapped_copy_id);
    } else {
      copy_base = base;
    }
  }
  if (copy_base->times_day &&
      this->GetCopyCount(mapped_copy_id) - this->GetCopyBoughtCount(copy_base) >= copy_base->times_day)
    return ERR_COPY_COUNT;

  //进度
  if (copy_base->type == sy::COPY_TYPE_TOWER) {
    if (this->tower_state_.current_order() < 0) return ERR_PARAM_INVALID;
    if (copy_base->GetOrder() != this->tower_state_.current_order() + 1) return ERR_COPY_INVALID;
  } else {
    const int32_t current_progress = this->GetCurrentCopyProgress(copy_base->type);
    if (current_progress < 0) return ERR_INTERNAL;
    if (copy_base->GetOrder() > current_progress + 1) return ERR_COPY_INVALID;
  }

  //前置副本判断
  const CopyBase* base_limit =
      COPY_BASE.GetEntryByID(copy_base->enter_limit).get();
  if (base_limit) {
    if (this->GetCurrentCopyProgress(base_limit->type) < base_limit->order)
      return ERR_COPY_INVALID;
  }

  return ERR_OK;
}

void LogicPlayer::FillCopyExpAndMoney(int32_t power, int32_t energy,
                                      int32_t& add_exp, int32_t& add_money) {
  const ExpBase* exp_base = EXP_BASE.GetEntryByID(this->level()).get();
  if (exp_base) {
    add_exp = power ? (power * exp_base->power_exp)
                    : 0 + energy ? (energy * exp_base->energy_exp) : 0;
    add_money = power ? (power * exp_base->power_money)
                      : 0 + energy ? (energy * exp_base->energy_money) : 0;
    float per = 1.0f;
    LeagueSkillBase* skill_base =
        LEAGUE_SKILL_BASE.GetEntryByID(ARMY_SKILL_EXP).get();
    if (this->army_id_ && skill_base) {
      per +=
          this->army_skill_[ARMY_SKILL_EXP] * skill_base->attr[0].v2 / 1000.0f;
    }
    add_exp = add_exp * per;
  }
}

void LogicPlayer::FillCopyAward(int32_t win, const CopyBase* copy_base,
                                ModifyCurrency& modify, AddSubItemSet& item_set,
                                int32_t& first_blood,
                                MessageNotifyFightResult& notify,
                                bool auto_flop) {
  const std::vector<LootBasePtr>& first_award_ptr= copy_base->FirstAward(this->level());
  const std::vector<LootBasePtr>& win_award_ptr = copy_base->AwardPtr(this->level());
  const std::vector<LootBasePtr>& lose_award_ptr= copy_base->LoseAward(this->level());
  static std::vector<LootBasePtr> empty;
  //首胜奖励
  first_blood = false;
  const std::vector<LootBasePtr>* award[2] = {&empty, &empty};
  if (!first_award_ptr.empty() && !this->GetPassedCopy(copy_base->id())) {
    first_blood = true;
    award[0] = win ? &first_award_ptr : &empty;
  }

  //胜利失败奖励
  award[1] = win ? &win_award_ptr : &lose_award_ptr;
  for (int32_t i = 0; i < ArraySize(award); ++i) {
    for (std::vector<LootBasePtr>::const_iterator iter = award[i]->begin();
         iter != award[i]->end(); ++iter) {
      if (*iter) (*iter)->Loot(modify, item_set, NULL);
    }
  }

  //胜利扣体力
  //如果有失败奖励也会扣体力
  if (win || (!win && !award[1]->empty())) {
    modify[MONEY_KIND_OIL] -= copy_base->power;
    modify[MONEY_KIND_ENERGY] -= copy_base->energy;
    int32_t add_exp = 0;
    int32_t add_money = 0;
    this->FillCopyExpAndMoney(copy_base->power, copy_base->energy, add_exp, add_money);
    modify[MONEY_KIND_EXP] += add_exp;
    modify[MONEY_KIND_COIN] += add_money;

    // N次奖励
    const LootBase* ext_award =
        copy_base->award_hero
            ? LootConfigFile::Get(copy_base->award_hero, this->level()).get()
            : NULL;
    if (ext_award) {
      int32_t mod = GetSettingValue(award_hero_drop_count);
      mod = mod > 1 ? mod : 5;
      int32_t count = this->GetCopyCount(copy_base->mapped_copy_id
                                             ? copy_base->mapped_copy_id
                                             : copy_base->id());
      if (count % mod == this->uid() % (mod - 1) + 1) {
        ext_award->Loot(modify, item_set, NULL);
      }
    }
  }

  if (win) {
    this->flop_1_ = copy_base->NormalCardPtr(this->level());
    this->flop_2_ = copy_base->SpecialCardPtr(this->level());
  }
  this->flop_2_count_ = 0;
  if (Setting::IsInFestival() &&
      server->GetServerStartDays() >= GetSettingValue(festival_time0)) {
    const LootBasePtr& ptr = LootConfigFile::Get(GetSettingValue(festival_drop), this->level());
    if (ptr) {
      ptr->Loot(modify, item_set, NULL);
    }
  }
  FillFightResultMessage(notify, item_set, modify);
  //自动翻拍逻辑
  if (auto_flop) {
    std::vector<int32_t> index;
    if (this->flop_1_) {
      INFO_LOG(logger)("PlayerID:%ld, LootID:%ld", this->uid(), this->flop_1_->id());
      this->flop_1_->Loot(modify, item_set, &index);
      if (!index.empty() && (int32_t) this->flop_1_->info.size() > index[0]) {
        ItemSimpleInfo* info = notify.mutable_flop();
        info->set_item_id(this->flop_1_->info[index[0]].v1);
        info->set_count(this->flop_1_->info[index[0]].v2);
        DEBUG_LOG(logger)("AutoFlop:%ld, ItemID:%d, Count:%d"
            , this->flop_1_->id(), info->item_id(), info->count());
      }
    }
    this->flop_1_.reset();
    this->flop_2_.reset();
    this->flop_2_count_ = 0;
  }
  if (this->flop_1_) notify.set_flop_1(this->flop_1_->id());
  if (this->flop_2_) notify.set_flop_2(this->flop_2_->id());

  if (IsHeroComeBack() && copy_base->type == COPY_TYPE_TOWER &&
      modify[MONEY_KIND_MUSCLE] > 0) {
    modify[MONEY_KIND_MUSCLE] *= 1.5f;
  }
}

void LogicPlayer::PrepareTowerRandomBuff(int32_t order) {
  if (order <= 0 || order % 3 != 0) {
    this->tower_state_.set_random_buff("");
    return;
  }
  int32_t b3 = 0, b6 = 0, b9 = 0;
  TowerConfigFile::RandomBuff(b3, b6, b9);
  DEBUG_LOG(logger)("PrepareTowerRandomBuff PlayerID:%ld, order:%d, BuffID:%d,%d,%d"
      , this->uid(), order, b3, b6, b9);
  DefaultArrayStream stream;
  stream.Append("%d;%d;%d", b3, b6, b9);
  this->tower_state_.set_random_buff(stream.str());
}

void LogicPlayer::OnCopyPassed(int32_t star, int32_t first_blood,
                               const CopyBase* copy_base) {
  const int32_t copy_id = copy_base->id();
  const int32_t mapped_copy_id = copy_base->mapped_copy_id;
  const int32_t current_progress = this->GetCurrentCopyProgress(copy_base->type);
  MessageNotifyCopyInfo notify_client;
  MessageSSUpdateCopyInfo update_server;

  //爬塔副本
  if (copy_base->type == sy::COPY_TYPE_TOWER) {
    int32_t win = star;
    //爬塔副本的难度决定了星数
    star = copy_base->id() % 10;
    DEBUG_LOG(logger)("OnTowerFinished, PlayerID:%ld, Order:%d, Win:%d, Star:%d",
        this->uid(), copy_base->GetOrder(), win, star);

    if (win) {
      this->tower_state_.set_current_order(this->tower_state_.current_order() + 1);
      this->tower_state_.set_current_star(this->tower_state_.current_star() + star);
      this->tower_state_.set_buff_star(this->tower_state_.buff_star() + star);
      int32_t copy_star = this->tower_state_.copy_star();
      copy_star |= star << ((copy_base->GetOrder() % 3) * 8);
      this->tower_state_.set_copy_star(copy_star);

      INFO_LOG(logger)("Before TowerState, Player:%ld, max_star_order:%d"
          , this->uid(), this->tower_state_.max_star_order());
      if (3 == star &&
          (copy_base->GetOrder() == this->tower_state_.max_star_order() + 1)) {
        this->tower_state_.set_max_star_order(this->tower_state_.max_star_order() + 1);
      }
      INFO_LOG(logger)("After TowerState, Player:%ld, max_star_order:%d"
          , this->uid(), this->tower_state_.max_star_order());

      //更新最大的星数和层数
      if (this->tower_state_.max_star() < this->tower_state_.current_star()) {
        this->tower_state_.set_max_star(this->tower_state_.current_star());
        RANK_LIST.OnTowerCopyPassed(this);
        int32_t rank =
            RANK_LIST.GetByType(RANK_TYPE_TOWER).GetRankByUID(this->uid());
        if (this->achievements_[SEVEN_DAY_TYPE_TOWER_RANK] == 0 ||
            (rank && rank < this->achievements_[SEVEN_DAY_TYPE_TOWER_RANK]))
          UpdateAchievement(SEVEN_DAY_TYPE_TOWER_RANK, rank);
      }
      if (this->tower_state_.max_star() >
          this->achievements_[SEVEN_DAY_TYPE_TOWER_STAR])
        UpdateAchievement(SEVEN_DAY_TYPE_TOWER_STAR,
                          this->tower_state_.max_star());
      if (this->tower_state_.max_star() >
          this->achievements_[FOURTEEN_DAY_TYPE_TOWER])
        UpdateAchievement(FOURTEEN_DAY_TYPE_TOWER,
                          this->tower_state_.max_star());
      if (this->tower_state_.max_order() < this->tower_state_.current_order())
        this->tower_state_.set_max_order(this->tower_state_.current_order());
      //随机buff
      this->PrepareTowerRandomBuff(this->tower_state_.current_order());
      //胜利重置抽箱子数
      if (copy_base->IsTowerShowBox()) this->tower_shop_ = 0;

      UpdateBuyCount(sy::COUNT_TYPE_DAILY_TOWER, 1);
      this->UpdateCopyInfo(copy_base->id(), star, &notify_client, &update_server);

    } else {
      this->tower_state_.set_current_order(-this->tower_state_.current_order() - 1);
    }

    this->SendTowerState();
  } else {
    if (star) {
      sy::CopyProgress progress;
      progress.set_copy_type(copy_base->type);
      progress.set_chapter(copy_base->chapter);
      progress.set_copy_id(copy_base->id());

      if (current_progress + 1 == copy_base->GetOrder())
        this->UpdateCopyProgress(progress, &notify_client, &update_server);
      bool has_passed = GetPassedCopy(copy_id);
      if (first_blood || (copy_base->type == COPY_TYPE_DAILY && !has_passed))
        this->UpdatePassedCopy(copy_id, &notify_client, &update_server);
      if (copy_base->star)
        this->UpdateCopyInfo(copy_id, star, &notify_client, &update_server);
      const CopyBase* mapped_copy = copy_base;
      //映射副本只是用来处理副本次数
      if (mapped_copy_id && mapped_copy_id != copy_id)
        mapped_copy = COPY_BASE.GetEntryByID(mapped_copy_id).get();
      if (mapped_copy->times_day) {
        if (mapped_copy->type == COPY_TYPE_DAILY) {
          if (has_passed)
            this->UpdateCopyCount(mapped_copy_id, &notify_client,
                                  &update_server);
        } else {
          this->UpdateCopyCount(mapped_copy_id, &notify_client, &update_server);
        }
      }

      //副本星级排行榜
      if (copy_base->type == sy::COPY_TYPE_NORMAL) RANK_LIST.OnNormalCopyPassed(this);
      if (copy_base->type == sy::COPY_TYPE_HARD) RANK_LIST.OnHardCopyPassed(this);
      //触发围剿BOSS
      if (copy_base->type == sy::COPY_TYPE_NORMAL ||
          copy_base->type == sy::COPY_TYPE_HARD) {
        this->CheckDstrikeBossTrigger(copy_base->type);
      }
      if (copy_base->type == sy::COPY_TYPE_NORMAL) {
        UpdateBuyCount(sy::COUNT_TYPE_DAILY_COPY, 1);
        const CopyBase* old_base =
            COPY_BASE.GetEntryByID(this->achievements_[SEVEN_DAY_TYPE_COPY])
                .get();

        int32_t old_order = old_base ? old_base->GetOrder() : 0;
        int32_t new_order = copy_base ? copy_base->GetOrder() : 0;
        if (new_order > old_order) {
          UpdateAchievement(SEVEN_DAY_TYPE_COPY, copy_base->id());
        }
        UpdateAchievement(
            FOURTEEN_DAY_TYPE_SWEEP_COPY3,
            this->achievements_[FOURTEEN_DAY_TYPE_SWEEP_COPY3] + 1);
        UpdateAchievement(
            FOURTEEN_DAY_TYPE_SWEEP_COPY4,
            this->achievements_[FOURTEEN_DAY_TYPE_SWEEP_COPY4] + 1);
        ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_COPY, 1, this);
        ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_COPY, 1, this);
      }
      if (copy_base->type == sy::COPY_TYPE_HARD) {
        ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_ELITE_COPY, 1, this);
        ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_HARD_COPY, 1,
                                        this);
        UpdateBuyCount(sy::COUNT_TYPE_DAILY_HARD_COPY, 1);
        UpdateAchievement(FOURTEEN_DAY_TYPE_ELITECOPY,
                          this->achievements_[FOURTEEN_DAY_TYPE_ELITECOPY] + 1);
      }
      if (copy_base->type == sy::COPY_TYPE_GENERAL) {
        UpdateBuyCount(sy::COUNT_TYPE_DAILY_GENERAL_COPY, 1);
        if (!this->achievements_[SEVEN_DAY_TYPE_GENERAL])
          UpdateAchievement(SEVEN_DAY_TYPE_GENERAL, copy_base->id());
        CopyBase* old_base =
            COPY_BASE.GetEntryByID(this->achievements_[SEVEN_DAY_TYPE_GENERAL])
                .get();
        if (old_base && copy_base->order > old_base->order)
          UpdateAchievement(SEVEN_DAY_TYPE_GENERAL, copy_base->id());
      }
      if (copy_base->type == sy::COPY_TYPE_GENERAL_HIDE) {
        if (!this->achievements_[SEVEN_DAY_TYPE_GENERAL_HIDE])
          UpdateAchievement(SEVEN_DAY_TYPE_GENERAL_HIDE, copy_base->id());
        CopyBase* old_base =
            COPY_BASE.GetEntryByID(
                         this->achievements_[SEVEN_DAY_TYPE_GENERAL_HIDE])
                .get();
        if (old_base && copy_base->order > old_base->order)
          UpdateAchievement(SEVEN_DAY_TYPE_GENERAL_HIDE, copy_base->id());
      }
      if (copy_base->type == sy::COPY_TYPE_DAILY) {
        UpdateBuyCount(sy::COUNT_TYPE_DAILY_DAILY_COPY, 1);
      }
      if (copy_base->type == sy::COPY_TYPE_ELITE) {
        ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_ELITE, 1, this);
      }
    }
  }

  if (notify_client.has_copy_star() + notify_client.has_progress() +
      notify_client.copy_count_size() + notify_client.passed_copy_size()) {
    this->SendMessageToClient(MSG_CS_NOTIFY_COPY_INFO, &notify_client);
    this->SendMessageToDB(MSG_SS_UPDATE_COPY_INFO, &update_server);
  }
}

int32_t LogicPlayer::ProcessRequestTowerBuyBuff(CSMessageEntry& entry) {
  MessageRequestTowerBuyBuff* message =
      static_cast<MessageRequestTowerBuyBuff*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t b3 = 0, b6 = 0, b9 = 0;
  int32_t copy_order = this->tower_state_.current_order() / 3 * 3;
  sscanf(this->tower_state_.random_buff().c_str(), "%d;%d;%d", &b3, &b6, &b9);
  if (message->buff_id() != b3 && message->buff_id() != b6 &&
      message->buff_id() != b9) {
    return ERR_PARAM_INVALID;
  }
  const TowerBuffBase* base = TOWER_BUFF_BASE.GetEntryByID(message->buff_id()).get();
  if (!base) return ERR_INTERNAL;
  if (this->tower_state_.buff_star() < base->star_num)
    return ERR_CURRENCY_STAR;
  if (this->tower_buff_[copy_order]) return ERR_PARAM_INVALID;

  this->tower_state_.set_random_buff("");
  this->tower_buff_[copy_order] = base->id();
  this->tower_state_.set_buff_star(this->tower_state_.buff_star() - base->star_num);

  //计算爬塔的属性
  this->CalcTowerAttr();

  MessageNotifyCurrentCarrierInfo msg;
  msg.mutable_info()->CopyFrom(this->current_carrier_);
  this->SendMessageToClient(MSG_CS_NOTIFY_CURRENT_CARRIER_INFO, &msg);

  this->SendTowerState();
  this->SendMessageToClient(MSG_CS_RESPONSE_TOWER_BUY_BUFF, NULL);
  return ERR_OK;
}

void LogicPlayer::CalcTowerAttr() {
  int64_t attr[sy::AttackAttr_ARRAYSIZE] = {0};
  for (VectorMap<int16_t, int16_t>::const_iterator iter =
           this->tower_buff_.begin();
       iter != this->tower_buff_.end(); ++iter) {
    const TowerBuffBase* base = TOWER_BUFF_BASE.GetEntryByID(iter->second).get();
    if (base) {
      attr[base->effect_type.v1] += base->effect_type.v2 / 100;
    }
  }
  this->current_carrier_.mutable_tower_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  for (int32_t i = 1; i < sy::AttackAttr_ARRAYSIZE; ++i) {
    this->current_carrier_.mutable_tower_attr1()->Set(i, attr[i]);
  }
}

int32_t LogicPlayer::ProcessRequestTowerAward(CSMessageEntry& entry) {
  MessageRequestTowerAward* message =
      static_cast<MessageRequestTowerAward*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t copy_star = this->tower_state_.copy_star();
  int32_t copy_star_count = 0;
  copy_star_count = (copy_star & 0xFF) + ((copy_star >> 8) & 0xFF) + ((copy_star >> 16) & 0xFF);
  if (message->star() > copy_star_count) return ERR_CURRENCY_STAR;
  const CopyChapterBase* base = COPY_CHAPTER_BASE.GetEntryByID(message->chapter()).get();
  if (!base) return ERR_INTERNAL;
  const CopyBase* copy_base = base->GetFirstCopy().get();
  if (!copy_base) return ERR_INTERNAL;
  if (this->tower_state_.current_order() / 3 != (copy_base->GetOrder() + 2) / 3)
    return ERR_PARAM_INVALID;
  if (this->tower_state_.current_order() < this->tower_state_.award())
    return ERR_PARAM_INVALID;

  AddSubItemSet item_set;
  ModifyCurrency modify(MSG_CS_REQUEST_TOWER_AWARD, SYSTEM_ID_CHALLENGE);
  for (size_t i = 0; i < base->star1.size(); ++i) {
    if (base->star1[i].v1 == message->star()) {
      const LootBasePtr& p = LootConfigFile::Get(base->star1[i].v2, this->level());
      if (p) {
        p->Loot(modify, item_set, NULL);
      }
      break;
    }
  }
  int32_t result = this->CheckCurrency(modify);
  if (result) return result;
  result = this->CheckItem(&item_set, NULL, NULL);
  if (result) return result;
  this->tower_state_.set_award(this->tower_state_.current_order() / 3 * 3);
  this->tower_state_.set_copy_star(0);

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_TOWER_AWARD,
                   SYSTEM_ID_CHALLENGE);
  this->SendTowerState();
  this->SendMessageToClient(MSG_CS_RESPONSE_TOWER_AWARD, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestTowerBuyBox(CSMessageEntry& entry) {
  int32_t& current_count = this->tower_shop_;

  const BuyCountCostBase* base = BUY_COUNT_BASE.GetEntryByID(COUNT_TYPE_TOWER_BOX).get();
  if (!base) return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_TOWER_BUY_BOX, SYSTEM_ID_CHALLENGE);
  AddSubItemSet item_set;
  modify[base->type] += -base->GetBuyCost(current_count + 1);
  int32_t result = this->CheckCurrency(modify);
  if (result) return result;

  ValuePair2Vec award;
  TowerConfigFile::GetTowerLootAward(item_set, modify, award,
                                     base->GetBuyCost(current_count + 1),
                                     this->level());

  result = this->CheckItem(&item_set, NULL, NULL);
  if (result) return result;

  ++current_count;

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_TOWER_BUY_BOX,
                   SYSTEM_ID_CHALLENGE);
  MessageResponseTowerBuyBox response;
  for (size_t i = 0; i < award.size(); ++i) {
    KVPair2* info = response.add_awards();
    info->set_key(award[i].v1);
    info->set_value(award[i].v2);
  }

  this->SendMessageToClient(MSG_CS_RESPONSE_TOWER_BUY_BOX, &response);
  return ERR_OK;
}

int32_t LogicPlayer::RecoverShip(
    const google::protobuf::RepeatedField<int64_t>& uids, bool is_resolve,
    google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items) {
  ModifyCurrency modify(MSG_CS_REQUEST_RECOVER, SYSTEM_ID_RECOVER);
  AddSubItemSet item_set;
  VectorMap<int32_t, int32_t> item_map;
  DeleteHeroSet delete_heros;

  for (int32_t i_ids = 0; i_ids < uids.size(); ++i_ids) {
    LogicHero* ship = this->GetHeroByUID(uids.Get(i_ids));
    if (!ship) return ERR_HERO_NOT_FOUND;

    HeroBase* hero_base = HERO_BASE.GetEntryByID(ship->second->id()).get();
    if (!hero_base) return ERR_HERO_NOT_FOUND;

    //战舰升级
    int64_t hero_exp = ship->first.exp();
    for (int32_t i = ship->first.level() - 1; i >= 1; --i)
      hero_exp += hero_base->GetLevelUpExp(i);
    const ValuePair2Vec& ship_lv_up_exp = Setting::GetValue2(Setting::shiplvup_exp);
    int64_t temp_exp = hero_exp;
    for (ValuePair2Vec::const_reverse_iterator it = ship_lv_up_exp.rbegin();
         it != ship_lv_up_exp.rend(); ++it) {
      int32_t item_count = temp_exp / it->v2;
      temp_exp %= it->v2;
      if (item_count) item_map[it->v1] += item_count;
      if (temp_exp < ship_lv_up_exp.begin()->v2) break;
    }
    modify[MONEY_KIND_COIN] += hero_exp;

    //战舰加固
    if (ship->first.train_cost())
      item_map[GetSettingValue(train_id)] += ship->first.train_cost();

    //战舰天命
    if (ship->first.fate_cost())
      item_map[GetSettingValue(ship_fate_id)] += ship->first.fate_cost();

    //战舰觉醒
    VectorMap<int32_t, int32_t> wake_temp_vct;
    for (int32_t k = 0; k < ship->first.wake_item_size(); k++) {
      if (ship->first.wake_item(k))
        wake_temp_vct[ship->first.wake_item(k)] += 1;
    }
    const VectorMap<int32_t, int32_t>* wake_items =
        GetUsedWakeItem(ship->first.wake_level(), ship->second->wake_itemtree);
    if (wake_items) {
      for (VectorMap<int32_t, int32_t>::const_iterator it = wake_items->begin();
           it != wake_items->end(); ++it)
        if (it->second) wake_temp_vct[it->first] += it->second;
    }
    for (VectorMap<int32_t, int32_t>::const_iterator it = wake_temp_vct.begin();
         it != wake_temp_vct.end(); ++it) {
      if (is_resolve) {
        WakeItemBase* base = WAKE_ITEM_BASE.GetEntryByID(it->first).get();
        ItemBase* item = ITEM_BASE.GetEntryByID(it->first).get();
        if (!item) return ERR_ITEM_NOT_FOUND;
        if (!base) return ERR_PARAM_INVALID;
        ModifyCurrency tmmd(MSG_CS_REQUEST_RECOVER_WAKE_ITEM, SYSTEM_ID_WAKE);
        AddSubItemSet tmset;
        for (int32_t j = 0; j < it->second; ++j)
          FillCurrencyAndItem<kAdd>(tmmd, tmset, item->sell);
        for (int32_t j = 0; j < MoneyKind_ARRAYSIZE; j++)
          if (tmmd[j]) modify[j] += tmmd[j];
        for (AddSubItemSet::iterator tit = tmset.begin(); tit != tmset.end();
             ++tit)
          if (tit->item_count) item_map[tit->item_id] += tit->item_count;
      } else {
        if (it->second) item_map[it->first] += it->second;
      }
    }
    for (int32_t j = ship->first.wake_level() - 1; j >= 0; --j) {
      WakeConditionBase* wake_base = WAKE_CONDITION_BASE.GetEntryByID(j).get();
      if (!wake_base) break;
      if (wake_base->need_coin) modify[MONEY_KIND_COIN] += wake_base->need_coin;
      if (wake_base->need_item) item_map[23900017] += wake_base->need_item;
      if (wake_base->need_hero) {
        if (is_resolve) {
          RecoverBase* recover_base =
              RECOVER_BASE.GetEntryByID(ship->second->quality + 10).get();
          if (!recover_base) return ERR_PARAM_INVALID;
          for (int32_t i = 0; i < wake_base->need_hero; ++i)
            FillCurrencyAndItem<kAdd>(modify, item_set, recover_base->type);
        } else {
          item_map[ship->second->ship_piece.v1] +=
              ship->second->ship_piece.v2 * wake_base->need_hero;
        }
      }
    }

    //战舰突破
    int32_t ship_count = 1;
    int32_t item_count = 0;
    for (int32_t j = ship->first.grade() - 1; j >= 0; --j) {
      const BreakAdvancedBase* grade_base =
          BREAK_ADVANCED.GetEntryByID(
                            BreakAdvancedID(hero_base->breakadvancedid, j))
              .get();
      if (!grade_base) return ERR_HERO_NOT_FOUND;

      ship_count += grade_base->hero_cost;
      item_count += grade_base->item_cost;
      modify[MONEY_KIND_COIN] += grade_base->coin_cost;
    }
    if (item_count) item_map[GetSettingValue(shipreform_id)] += item_count;

    if (is_resolve) {
      RecoverBase* recover_base =
          RECOVER_BASE.GetEntryByID(ship->second->quality + 10).get();
      if (!recover_base) return ERR_PARAM_INVALID;
      for (int32_t i = 0; i < ship_count; ++i)
        FillCurrencyAndItem<kAdd>(modify, item_set, recover_base->type);
    } else {
      if (ship->second->ship_piece.v2 * ship_count)
        item_map[ship->second->ship_piece.v1] +=
            ship->second->ship_piece.v2 * ship_count;
    }

    if (delete_heros.full()) break;
    delete_heros.push_back(ship->first.uid());
  }

  int32_t result = CheckCurrency(modify);
  if (result) return result;

  for (VectorMap<int32_t, int32_t>::const_iterator it = item_map.begin();
      it != item_map.end(); ++it){
    if(it->first > 100)
      item_set.push_back(ItemParam(it->first, it->second));
    else
      modify[it->first] += it->second;
  }
  result = CheckItem(&item_set, NULL, NULL);
  if (result) return result;

  result = CheckHero(&delete_heros, NULL);
  if (result) return result;

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_RECOVER, SYSTEM_ID_RECOVER);
  ObtainHero(&delete_heros, NULL, SYSTEM_ID_RECOVER, MSG_CS_REQUEST_RECOVER);

  FillToKVPair2(&modify, &item_set, out_items);

  return ERR_OK;
}

int32_t LogicPlayer::RecoverEquip(
    const google::protobuf::RepeatedField<int64_t>& uids, bool is_resolve,
    google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items) {
  ModifyCurrency modify(MSG_CS_REQUEST_RECOVER, SYSTEM_ID_RECOVER);
  AddSubItemSet item_set;
  VectorMap<int32_t, int32_t> item_map;
  DeleteItemSet delete_set;

  for (int32_t i = 0; i < uids.size(); ++i) {
    LogicItem* item = this->items_.GetItemByUniqueID(uids.Get(i));
    if (!item || !item->equip_base()) return ERR_ITEM_NOT_FOUND;
    if (GetItemEquipedPos(item)) return ERR_PARAM_INVALID;

    //装备强化返还
    ValuePair2<int32_t, int32_t> equip_pair;
    for (int32_t i = item->GetAttr(sy::ITEM_ATTR_LEVEL) - 1; i >= 0; --i) {
      EquipLevelUpBase* level_up_base =
          EQUIP_LEVEL_UP_BASE.GetEntryByID(i).get();
      if (!level_up_base) continue;
      equip_pair =
          level_up_base->GetMoneyByTyEquipQuality(item->equip_base()->quality);
      modify[equip_pair.v1] +=
          (int64_t)(equip_pair.v2 *
                    (Setting::GetMaxValueInVec2(Setting::equip_goldreturn_rate,
                                                this->vip_level()) /
                     10000.0));
    }
    //装备精炼返还
    int64_t refine_exp = item->GetAttr(sy::ITEM_ATTR_REFINE_EXP);
    for (int32_t i = item->GetAttr(sy::ITEM_ATTR_REFINE_LEVEL) - 1; i >= 0;
         --i) {
      EquipRefineBase* refine_base = EQUIP_REFINE_BASE.GetEntryByID(i).get();
      if (!refine_base) return ERR_ITEM_REFINE_NOT_FOUND;
      refine_exp +=
          refine_base->GetExpByEquipQuality(item->equip_base()->quality);
    }
    const ValuePair2Vec& equip_reform_exp =
        Setting::GetValue2(Setting::equipreform_exp);
    for (ValuePair2Vec::const_reverse_iterator it = equip_reform_exp.rbegin();
         it != equip_reform_exp.rend(); ++it) {
      int32_t item_count = refine_exp / it->v2;
      refine_exp %= it->v2;
      if (item_count > 0) item_map[it->v1] += item_count;
      if (refine_exp < equip_reform_exp.begin()->v2) break;
    }

    // TODO 装备升星返还

    //分解碎片或资源
    EquipBase* equip_base = EQUIP_BASE.GetEntryByID(item->item_id()).get();
    if (!equip_base) return ERR_ITEM_NOT_FOUND;

    if (is_resolve) {
      RecoverBase* recover_base =
          RECOVER_BASE.GetEntryByID(equip_base->quality).get();
      if (!recover_base) return ERR_PARAM_INVALID;
      FillCurrencyAndItem<kAdd>(modify, item_set, recover_base->type);
    } else {
      if (equip_base->equip_piece.v2)
        item_map[equip_base->equip_piece.v1] += equip_base->equip_piece.v2;
    }

    delete_set.push_back(item->uid());
  }

  int32_t result = CheckCurrency(modify);
  if (result) return result;

  for (VectorMap<int32_t, int32_t>::const_iterator it = item_map.begin();
       it != item_map.end(); ++it)
    item_set.push_back(ItemParam(it->first, it->second));

  result = CheckItem(&item_set, &delete_set, NULL);
  if (result) return result;

  UpdateCurrency(modify);
  ObtainItem(&item_set, &delete_set, NULL, MSG_CS_REQUEST_RECOVER,
             SYSTEM_ID_RECOVER);

  FillToKVPair2(&modify, &item_set, out_items);

  return ERR_OK;
}

int32_t LogicPlayer::RecoverNavy(
    const google::protobuf::RepeatedField<int64_t>& uids,
    google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items) {
  ModifyCurrency modify(MSG_CS_REQUEST_RECOVER, SYSTEM_ID_RECOVER);
  AddSubItemSet item_set;
  NotifyItemSet notify_set;
  VectorMap<int32_t, int32_t> item_map;

  VectorMap<int32_t, int32_t> temp_insert_item_map;

  for (int32_t i = 0; i < uids.size(); ++i) {
    LogicItem* navy = this->items_.GetItemByUniqueID(uids.Get(i));
    if (!navy || !navy->army_base()) return ERR_ITEM_NOT_FOUND;

    if (GetItemEquipedPos(navy)) return ERR_PARAM_INVALID;

    ArmyBase* navy_base = ARMY_BASE.GetEntryByID(navy->item_id()).get();
    if (!navy_base) return ERR_ITEM_NOT_FOUND;

    //宝物等级
    int64_t navy_exp = navy->GetAttr(sy::ITEM_ATTR_NAVY_EXP);
    for (int32_t i = navy->GetAttr(sy::ITEM_ATTR_NAVY_LEVEL) - 1; i >= 0; --i) {
      ArmyExpBase* navy_exp_base = ARMY_EXP_BASE.GetEntryByID(i).get();
      if (!navy_exp_base) return ERR_ITEM_NOT_FOUND;
      navy_exp += navy_exp_base->GetExp(navy->army_base()->quality);
    }
    modify.coin += navy_exp;

    const ValuePair2Vec& army_exp = Setting::GetValue2(Setting::army_exp);
    for (ValuePair2Vec::const_reverse_iterator it = army_exp.rbegin();
         it != army_exp.rend(); ++it) {
      int32_t item_count = navy_exp / it->v2;
      navy_exp %= it->v2;
      if (item_count > 0) item_map[it->v1] += item_count;
      if (navy_exp < army_exp.begin()->v2) break;
    }

    //宝物改造等级
    for (int32_t i = navy->GetAttr(sy::ITEM_ATTR_NAVY_REFINE_LEVEL) - 1; i >= 0;
         --i) {
      ArmyRefineBase* army_refine_base = ARMY_REFINE_BASE.GetEntryByID(i).get();
      if (!army_refine_base) return ERR_ITEM_NOT_FOUND;
      AddSubItemSet temp_item_set;
      FillCurrencyAndItem<kAdd>(modify, temp_item_set, army_refine_base->cost);
      for (AddSubItemSet::iterator it = temp_item_set.begin();
           it != temp_item_set.end(); ++it)
        item_map[it->item_id] += it->item_count;

      if (army_refine_base->self_cost)
        item_map[navy_base->id()] += army_refine_base->self_cost;
    }

    navy->SetAttr(sy::ITEM_ATTR_NAVY_LEVEL, 0);
    navy->SetAttr(sy::ITEM_ATTR_NAVY_EXP, 0);
    navy->SetAttr(sy::ITEM_ATTR_NAVY_REFINE_LEVEL, 0);

    notify_set.push_back(navy->uid());

    temp_insert_item_map[navy->item_id()] += 1;
  }

  int32_t result = CheckCurrency(modify);
  if (result) return result;

  for (VectorMap<int32_t, int32_t>::const_iterator it = item_map.begin();
       it != item_map.end(); ++it)
    item_set.push_back(ItemParam(it->first, it->second));
  result = CheckItem(&item_set, NULL, &notify_set);
  if (result) return result;

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, &notify_set, MSG_CS_REQUEST_RECOVER,
             SYSTEM_ID_RECOVER);

  for (VectorMap<int32_t, int32_t>::iterator it = temp_insert_item_map.begin();
       it != temp_insert_item_map.end(); ++it)
    item_map[it->first] += it->second;
  item_set.clear();
  for (VectorMap<int32_t, int32_t>::const_iterator it = item_map.begin();
       it != item_map.end(); ++it)
    item_set.push_back(ItemParam(it->first, it->second));

  FillToKVPair2(&modify, &item_set, out_items);

  return ERR_OK;
}

int32_t LogicPlayer::RecoverCarrier(
    const google::protobuf::RepeatedField<int64_t>& ids, bool is_resolve,
    google::protobuf::RepeatedPtrField< ::sy::KVPair2>* out_items) {
  ModifyCurrency modify(MSG_CS_REQUEST_RECOVER, SYSTEM_ID_RECOVER);
  AddSubItemSet item_set;
  VectorMap<int32_t, int32_t> item_map;

  VectorSet<int32_t> delete_set;

  for (int32_t j = 0; j < ids.size(); ++j) {
    CarrierInfo* carrier = this->GetCarrierByID(ids.Get(j));
    if (!carrier) return ERR_CARRIER_NOT_FOUND;

    CarrierBase* base = CARRIER_BASE.GetEntryByID(ids.Get(j)).get();
    if (!base) return ERR_CARRIER_NOT_FOUND;

    if (this->current_carrier_.carrier_id() == ids.Get(j))
      return ERR_PARAM_INVALID;

    for (size_t i = 1; i <= 6; ++i) {
      RepeatedField<int64_t>* equips = GetEquipsInfo(i);
      if (equips) {
        if (equips->Get(6) == base->id()) return ERR_PARAM_INVALID;
      }
    }

    int64_t carrier_exp = carrier->exp();
    for (int32_t i = carrier->level(); i >= 1; --i)
      carrier_exp += base->GetLevelUpExp(i);

    const ValuePair2Vec& carrier_lv_up_exp =
        Setting::GetValue2(Setting::carrierlvup_exp);

    int64_t temp_exp = carrier_exp;
    for (ValuePair2Vec::const_reverse_iterator it = carrier_lv_up_exp.rbegin();
         it != carrier_lv_up_exp.rend(); ++it) {
      int32_t item_count = temp_exp / it->v2;
      temp_exp %= it->v2;
      if (item_count) item_map[it->v1] += item_count;
      if (temp_exp < carrier_lv_up_exp.begin()->v2) break;
    }
    modify[MONEY_KIND_COIN] += carrier_exp;

    int32_t item_count = 0;
    int32_t hero_count = 0;
    for (int32_t i = carrier->reform_level() - 1; i >= 0; --i) {
      const BreakAdvancedBase* grade_base =
          BREAK_ADVANCED.GetEntryByID(BreakAdvancedID(base->breakadvancedid, i))
              .get();
      if (!grade_base) return ERR_CARRIER_NOT_FOUND;
      item_count += grade_base->item_cost;
      hero_count += grade_base->hero_cost;
      modify[MONEY_KIND_COIN] += grade_base->coin_cost;
    }
    if (item_count)
      item_map[GetSettingValue(carrier_advanceditem_id)] += item_count;

    if (is_resolve) {
      RecoverBase* recover_base =
          RECOVER_BASE.GetEntryByID(base->quality + 20).get();
      if (!recover_base) return ERR_PARAM_INVALID;
      FillCurrencyAndItem<kAdd>(modify, item_set, recover_base->type);
      if (hero_count) {
        int32_t per_count =
            Setting::GetValueInVec2("carrier_recover", base->quality);
        if (per_count) item_map[23900010] += hero_count * per_count;
      }
    } else {
      for (ValuePair2Vec::iterator it = base->item.begin();
           it != base->item.end(); ++it) {
        if (it->v1) item_map[it->v1] += it->v2;
      }

      if (hero_count) item_map[base->item[0].v1] += hero_count;
    }
    delete_set.insert(carrier->carrier_id());
  }

  for (VectorMap<int32_t, int32_t>::const_iterator it = item_map.begin();
       it != item_map.end(); ++it)
    item_set.push_back(ItemParam(it->first, it->second));

  int32_t result = CheckCurrency(modify);
  if (result) return result;
  result = CheckItem(&item_set, NULL, NULL);
  if (result) return result;

  for (VectorSet<int32_t>::iterator it = delete_set.begin();
       it != delete_set.end(); ++it) {
    for (std::vector<sy::CarrierInfo>::iterator it2 = this->carriers_.begin();
         it2 != this->carriers_.end(); ++it2) {
      if (it2->carrier_id() == *it) {
        this->carriers_.erase(it2);
        break;
      }
    }

    MessageSSUpdateCarrierInfo up_message;
    up_message.set_delete_id(*it);
    up_message.set_tid(server->GetTID());
    this->SendMessageToDB(MSG_SS_UPDATE_CARRIER_INFO, &up_message);

    MessageNotifyCarrierInfo msg;
    msg.set_delete_id(*it);
    this->SendMessageToClient(MSG_CS_NOTIFY_CARRIER_INFO, &msg);
  }

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_RECOVER, SYSTEM_ID_RECOVER);

  FillToKVPair2(&modify, &item_set, out_items);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetUIDByName(CSMessageEntry& entry) {
  MessageRequestGetUIDByName* message =
      static_cast<MessageRequestGetUIDByName*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->name().size() >= 64) return ERR_PARAM_INVALID;
  int64_t uid = server->GetUIDByName(message->name());
  if (uid) {
    MessageResponseGetUIDByName response;
    response.set_name(message->name());
    response.set_player_id(uid);
    this->SendMessageToClient(MSG_CS_RESPONSE_GET_UID_BY_NAME, &response);
  } else {
    MessageSSRequestGetUIDByName request;
    request.set_name(message->name());
    this->SendMessageToDB(MSG_SS_REQUEST_GET_UID_BY_NAME, &request);
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetUIDByName(SSMessageEntry& entry) {
  MessageSSResponseGetUIDByName *message = static_cast<MessageSSResponseGetUIDByName*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->player_id()) server->AddNewName(message->name(), message->player_id());

  MessageResponseGetUIDByName response;
  response.set_name(message->name());
  response.set_player_id(message->player_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_UID_BY_NAME, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestCopySweep(CSMessageEntry& entry) {
  MessageRequestCopySweep *message = static_cast<MessageRequestCopySweep*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const CopyBase *copy_base = COPY_BASE.GetEntryByID(message->copy_id()).get();
  if (!copy_base) return ERR_PARAM_INVALID;
  if (copy_base->type == sy::COPY_TYPE_TOWER &&
      Setting::GetValue1(Setting::oil_3stars).size() >= 2u) {
    if (this->vip_level() < Setting::GetValue1(Setting::oil_3stars)[0] &&
        this->level() < Setting::GetValue1(Setting::oil_3stars)[1]) {
      return ERR_CURRENCY_VIP_EXP;
    }
  }

  int32_t first_blood = 0, star = 3;
  AddSubItemSet item_set;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_COPY);
  MessageNotifyFightResult notify;
  MessageResponseCopySweep response;
  response.set_copy_id(message->copy_id());

  if (this->GetCopyStar(copy_base->id()) < 3) return ERR_COPY_CANNOT_SWEEP;
  CHECK_RET(this->CheckEnterCopy(copy_base));

  if (copy_base->type == sy::COPY_TYPE_ARMY) {
    int32_t army_id = message->param();
    if (this->items_.GetItemByItemID(army_id)) return ERR_ITEM_INVALID_PARAM;
    ArmyBase* army_base = ArmyBase::GetArmyBaseByArmyItem(army_id);
    if (!army_base) return ERR_ITEM_NOT_FOUND;

    if (IsRobSuccess(army_base, false)) {
      item_set.push_back(ItemParam(army_id, 1));
    }
    this->UpdateAchievement(
        ACHIEVEMENT_TYPE_COUNT_ROB_NAVY,
        this->achievements_[ACHIEVEMENT_TYPE_COUNT_ROB_NAVY] + 1);
    this->UpdateAchievement(
        FOURTEEN_DAY_TYPE_ROB_ARMY1,
        this->achievements_[FOURTEEN_DAY_TYPE_ROB_ARMY1] + 1);
    this->UpdateAchievement(
        FOURTEEN_DAY_TYPE_ROB_ARMY2,
        this->achievements_[FOURTEEN_DAY_TYPE_ROB_ARMY2] + 1);
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_ROB, 1, this);
  } else {
    this->UpdateAchievement(
        ACHIEVEMENT_TYPE_COUNT_SWEEP_COPY,
        this->achievements_[ACHIEVEMENT_TYPE_COUNT_SWEEP_COPY] + 1);
  }

  this->FillCopyAward(1, copy_base, modify, item_set, first_blood, notify,
                      true);
  first_blood = false;
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  this->OnCopyPassed(star, first_blood, copy_base);

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_COPY);

  notify.set_win(1);
  notify.set_round(0);
  notify.set_star(star);
  notify.set_copy_id(copy_base->id());

  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_COPY_SWEEP, &response);
  server->UpdateCopyStatistics(copy_base->type, copy_base->id(), this->uid());
  return ERR_OK;
}

sy::FriendInfo* LogicPlayer::GetFriendInfoByID(int64_t uid) {
  for (int32_t i = 0; i < this->friends_.infos_size(); ++i) {
    if (this->friends_.infos(i).friend_id() == uid)
      return this->friends_.mutable_infos(i);
  }
  return NULL;
}

void LogicPlayer::DeleteFriend(int64_t uid) {
  for (int32_t i = 0; i < this->friends_.infos_size(); /*++i*/) {
    if (this->friends_.infos(i).friend_id() == uid) {
      this->friends_.mutable_infos()->DeleteSubrange(i, 1);
      INFO_LOG(logger)("PlayerID:%ld DeleteFriend:%ld", this->uid(), uid);
    } else {
      ++i;
    }
  }

  MessageNotifyFriendInfo notify;
  notify.set_delete_friend(uid);
  this->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);
}

int32_t LogicPlayer::ProcessRequestDstrikeDailyAward(CSMessageEntry& entry) {
  MessageRequestDstrikeBossDailyAward* message =
      static_cast<MessageRequestDstrikeBossDailyAward*>(entry.get());
  if (!message) return ERR_INTERNAL;
  uint64_t status = this->dstrike_info_.daily_award();
  if (status & (1ul << message->index())) return ERR_DEBUG_BOSS_REWARDED;

  const DstrikeMeritAwardBase* base = DstrikeConfigFile::GetDailyAward(this->level(), message->index());
  if (!base) return ERR_DEBUG_BOSS_CONFIG;
  if (this->dstrike_info_.merit() < base->merit_num) return ERR_DEBUG_BOSS_MERIT_NEED_MORE;

  ModifyCurrency modify(MSG_CS_REQUEST_DSTRIKE_DAILY_AWARD,
                        SYSTEM_ID_DSTRIKE_MAIN);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->merit_award);

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  status |= 1lu << message->index();
  this->dstrike_info_.set_daily_award(status);
  this->UpdateDstrikeInfo(true);
  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_DAILY_AWARD,
                   SYSTEM_ID_DAILY_TASK);

  this->SendMessageToClient(MSG_CS_RESPONSE_DSTRIKE_DAILY_AWARD, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestDstrikePreFight(CSMessageEntry& entry) {
  MessageRequestDstrikeFight* message = static_cast<MessageRequestDstrikeFight*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageResponseDstrikeFight response;
  sy::DstrikeBoss* boss = server->GetDstrikeBossByPlayerId(message->player_id());
  if (!boss || !IsBossValid(*boss) ||
      (this->uid() != message->player_id() && !boss->boss_status())) {
    response.set_player_id(message->player_id());
    this->SendMessageToClient(MSG_CS_RESPONSE_DSTRIKE_FIGHT, &response);
    return ERR_DSTRIKE_NOT_EXISTS;
  }
  sy::DstrikeBoss boss_info = *boss;
  const DstrikeBossBase* boss_base = DstrikeConfigFile::GetBossByID(boss->boss_id());
  if (!boss_base) {
    boss->set_boss_expire_time(0);
    response.set_player_id(message->player_id());
    this->SendMessageToClient(MSG_CS_RESPONSE_DSTRIKE_FIGHT, &response);
    return ERR_DSTRIKE_ESCAPE;
  }
  const CopyBase* copy_base =
      COPY_BASE.GetEntryByID(message->token_count() == 1 ? 62550 : 62551).get();
  if (!copy_base) return ERR_COPY_INVALID;

  sy::CurrentCarrierInfo carrier;
  std::vector<sy::HeroInfo> monster;
  int32_t level = boss_info.boss_level();
  boss_base->FillMonsterInfos(carrier, monster, level);

  boss_info.mutable_boss_blood()->Resize(6, 0);
  PK pk(copy_base);
  pk.InitPlayer(this, true);
  pk.InitMonsterInfo(carrier, monster, boss_info.boss_blood().begin());
  pk.GeneratePVPReport();
  if (server_config->report()) {
    LogReportToFile(pk.report);
  }

  std::vector<int64_t> current_hp;
  current_hp.resize(6, 0);
  for (int32_t pos = 1; pos <= 6; ++pos) {
    PkHero* hero = pk.b.GetHeroByPos(pos);
    if (hero) {
      current_hp[pos - 1] = hero->hp();
    } else {
      INFO_LOG(logger)("Pos:%d, invalid", pos);
    }
  }

  INFO_LOG(logger)("PlayerID%ld, CurrentHP:%ld,%ld,%ld,%ld,%ld,%ld, TotalBlood:%ld"
      , this->uid()
     , current_hp[0], current_hp[1], current_hp[2], current_hp[3], current_hp[4], current_hp[5]
     , boss_info.total_blood());
  CHECK_RET(this->OnDstrikeCopy(boss_info, copy_base, current_hp));

  boss = server->GetDstrikeBossByPlayerId(message->player_id());
  if (boss && IsBossValid(*boss)) response.mutable_boss()->CopyFrom(*boss);
  else response.set_player_id(message->player_id());

  this->SendMessageToClient(MSG_CS_RESPONSE_DSTRIKE_FIGHT, &response);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestDstrikeShare(CSMessageEntry& entry){
  //获取自己的BOSS信息
  sy::DstrikeBoss* boss = server->GetDstrikeBossByPlayerId(this->uid());
  if (boss && IsBossValid(*boss)) {
    //设置boss的分享信息
    boss->set_boss_status(1);
    UpdateBuyCount(COUNT_TYPE_DAILY_SHARE_DSTRIKE, 1);
    ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_SHARE_DESTRIK, 1,
                                    this);
  }
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestUseTruce(CSMessageEntry& entry) {
  MessageRequestUseTruce* message =
      static_cast<MessageRequestUseTruce*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if(this->player_.truce_time() > GetVirtualSeconds())
    return ERR_PARAM_INVALID;

  int32_t item_id = message->truce_item_id();

  int32_t use_time =
      Setting::GetValueInVec2(Setting::army_armistice_time, item_id);

  if (0 == use_time) return ERR_PARAM_INVALID;

  LogicItem* item = this->items_.GetItemByItemID(item_id);
  if (!item || !item->item_base()) return ERR_ITEM_NOT_FOUND;

  AddSubItemSet item_set;
  item_set.push_back(ItemParam(item_id, -1));

  int32_t result = CheckItem(&item_set, NULL, NULL);
  if (result) return result;

  SetTruceTime(use_time * 60 + GetVirtualSeconds());

  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_USE_TRUCE, SYSTEM_ID_ROB);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetRobOpponent(CSMessageEntry& entry) {
  MessageRequestGetRobOpponent* message =
      static_cast<MessageRequestGetRobOpponent*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const ItemBase* item_base = ITEM_BASE.GetEntryByID(message->item_id()).get();
  if (!item_base || !item_base->can_lost) return ERR_PARAM_ARRAY_BOUND;

  MessageResponseGetRobOpponent response;
  std::vector<LogicPlayer*> temp_vct;

  const VectorSet<int64_t>* set_players =
      server->GetRobPlayers(message->item_id());
  if (set_players) {
    for (VectorSet<int64_t>::const_iterator it = set_players->begin();
         it != set_players->end(); ++it) {
      LogicPlayer* other_player = server->GetPlayerByID(*it);
      if (!other_player) return ERR_PLAYER_NOT_EXIST;
      if (other_player->player_.truce_time() > GetVirtualSeconds()) continue;
      if (abs(other_player->level() - this->level()) > 5) continue;
      if (!other_player->items_.GetItemByItemID(message->item_id())) continue;

      temp_vct.push_back(other_player);
    }
  }

  int32_t random_count = 2;

  while (random_count > 0) {
    if (temp_vct.empty()) break;

    int32_t index = RandomBetween(0, temp_vct.size() - 1);

    LogicPlayer* temp_player = temp_vct[index];
    sy::PlayerSimpleInfo* info = response.add_other_info();
    info->set_player_id(temp_player->uid());
    info->set_level(temp_player->level());
    info->set_name(temp_player->player_.name());

    temp_vct.erase(temp_vct.begin() + index);
    --random_count;
  }

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ROB_OPPONENT, &response);

  return ERR_OK;
}

//触发新的boss
bool LogicPlayer::CheckDstrikeBossTrigger(int32_t type) {
  {
    DstrikeBoss* boss = server->GetDstrikeBossByPlayerId(this->uid());
    if (boss && IsBossValid(*boss)) return false;
  }
  const DstrikeBossBase* base = DstrikeConfigFile::RandomBoss(this->level(), type);
  if (base) {
    TRACE_LOG(logger)("PlayerID:%ld, NewDstrikeBoss:%ld", this->uid(), base->monster->id());

    sy::MessageNotifyDstrikeShare notify;
    sy::DstrikeBoss* boss = notify.mutable_boss();
    boss->set_player_id(this->uid());
    boss->set_name(this->player_.name());
    boss->set_boss_id(base->monster->id());
    boss->set_boss_quality(base->quality);
    boss->set_boss_level(this->dstrike_info_.level() + 1);
    boss->set_boss_status(0);
    boss->set_boss_time(GetSeconds());
    boss->set_boss_expire_time(Setting::GetValueInVec2(Setting::dstrike_boss_time, base->quality));
    //计算血量
    base->FillBlood(*boss->mutable_boss_blood(), boss->boss_level());
    int64_t total_blood = 0;
    for (int32_t index = 0; index < boss->boss_blood_size(); ++index) {
      total_blood += boss->boss_blood(index);
    }
    boss->set_total_blood(total_blood);
    DEBUG_LOG(logger)("PlayerID:%ld NewDstrikeBoss:%d Blood:%ld,%ld,%ld,%ld,%ld,%ld, TotalBlood:%ld"
        , this->uid(), boss->boss_id()
        , boss->boss_blood(0), boss->boss_blood(1)
        , boss->boss_blood(2), boss->boss_blood(3)
        , boss->boss_blood(4), boss->boss_blood(5)
        , total_blood
        );

    server->AddDstrikeBoss(*boss);
    this->SendMessageToClient(MSG_CS_NOTIFY_DSTRIKE_SHARE, &notify);

    return true;
  }
  return false;
}

int32_t LogicPlayer::ProcessRequestDstrikeList(CSMessageEntry& entry) {
  sy::MessageResponseDstrikeList response;
  server->GetDstrikeBossList(this, response);
  this->SendMessageToClient(MSG_CS_RESPONSE_DSTRIKE_LIST, &response);
  return ERR_OK;
}

void LogicPlayer::GetAllFriends(std::vector<int64_t>& friends) {
  for (int32_t i = 0; i < this->friends_.infos_size(); ++i) {
    const sy::FriendInfo& info = this->friends_.infos(i);
    if (info.type() == FRIEND_STATUS_FRIEND) {
      friends.push_back(info.friend_id());
    }
  }
}

int32_t LogicPlayer::ProcessRequestFriendEnergy(CSMessageEntry& entry) {
  MessageRequestFriendEnergy* message = static_cast<MessageRequestFriendEnergy*>(entry.get());
  if (!message) return ERR_INTERNAL;
  sy::FriendInfo* info = this->GetFriendInfoByID(message->friend_id());
  if (!info ||
      !GetSecondsDiffDays(info->energy_time() - Setting::kRefreshSeconds,
                         GetVirtualSeconds() - Setting::kRefreshSeconds))
    return ERR_PARAM_INVALID;

  //更新好友的精力
  {
    LogicPlayer* player = server->GetPlayerByID(message->friend_id());
    if (player) {
      sy::FriendInfo* friend_info = player->GetFriendInfoByID(this->uid());
      if (friend_info) {
        friend_info->set_energy(1);
        MessageNotifyFriendInfo notify;
        notify.add_friend_()->CopyFrom(*friend_info);
        player->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);
      }
    }
    MessageSSUpdateFriendEnergy update;
    update.set_friend_id(message->friend_id());
    update.set_enery(1);
    this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_ENERGY, &update);
  }
  UpdateBuyCount(sy::COUNT_TYPE_DAILY_GIVE_ENEGRY, 1);
  //更新自己的时间戳
  info->set_energy_time(GetVirtualSeconds());

  MessageSSUpdateFriendInfo update;
  update.set_player_id(this->uid());
  update.add_friends()->CopyFrom(*info);
  this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);

  MessageNotifyFriendInfo notify;
  notify.add_friend_()->CopyFrom(*info);
  this->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_FRIEND_ENERGY, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetFriendEnergy(CSMessageEntry& entry) {
  MessageRequestGetFriendEnergy* message =
      static_cast<MessageRequestGetFriendEnergy*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t max_energy = GetSettingValue(friend_jingli_max);
  int32_t get_energy = 0;
  int32_t got_energy = this->daily_counter_[COUNT_TYPE_FRIEND_ENERGY];
  //计算最多可以领取多少个精力
  int32_t max_get_energy = Setting::kEnergyMax - this->player_.energy();
  if (max_get_energy <= 0) return ERR_CURRENCY_ENERGY_OF;

  MessageNotifyFriendInfo notify;
  MessageSSUpdateFriendInfo update;
  update.set_player_id(this->uid());

  if (0 == message->friend_id()) {
    for (int32_t i = 0; i < this->friends_.infos_size(); ++i) {
      sy::FriendInfo* info = this->friends_.mutable_infos(i);
      if (!info || !info->energy()) continue;
      if (got_energy >= max_get_energy) break;
      if (got_energy + get_energy >= max_energy) break;
      get_energy++;
      info->set_energy(0);
      notify.add_friend_()->CopyFrom(*info);
      update.add_friends()->CopyFrom(*info);
    }
  } else {
    sy::FriendInfo* info = this->GetFriendInfoByID(message->friend_id());
    if (!info || !info->energy()) return ERR_PARAM_INVALID;
    if (got_energy >= max_energy) return ERR_PLAYER_FRIEND_ENERGY;
    get_energy = 1;
    info->set_energy(0);
    notify.add_friend_()->CopyFrom(*info);
    update.add_friends()->CopyFrom(*info);
  }

  ModifyCurrency modify(MSG_CS_REQUEST_GET_FRIEND_ENERGY, SYSTEM_ID_BASE);
  modify.energy = get_energy;

  CHECK_RET(this->CheckCurrency(modify));
  UpdateBuyCount(COUNT_TYPE_FRIEND_ENERGY, get_energy);
  this->UpdateCurrency(modify);

  MessageResponseGetFriendEnergy response;
  response.set_get_energy(get_energy);

  this->SendMessageToClient(MSG_CS_NOTIFY_FRIEND_INFO, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_FRIEND_ENERGY, &response);
  this->SendMessageToDB(MSG_SS_UPDATE_FRIEND_INFO, &update);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetRankList(CSMessageEntry& entry) {
  MessageRequestGetRankList *message = static_cast<MessageRequestGetRankList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageResponseGetRankList response;
  response.set_rank_type(message->rank_type());
  if (message->rank_type() == RANK_TYPE_ARMY_WAR) {
    Army* army = server->GetArmyByID(this->army_id());
    if (army) {
      response.mutable_list()->CopyFrom(army->rank_list().data());
    }
  } else {
    response.mutable_list()->CopyFrom(
        RANK_LIST.GetByType(message->rank_type()).data());
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_RANK_LIST, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestUseItem(CSMessageEntry& entry) {
  MessageRequestUseItem* message =
      static_cast<MessageRequestUseItem*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t use_count = message->item_count();
  if (use_count > 100) use_count = 100;
  LogicItem* item = this->items_.GetItemByUniqueID(message->item_uid());
  if (!item) return ERR_ITEM_NOT_FOUND;
  if (use_count <= 0 || item->count() < use_count) return ERR_ITEM_NEED_MORE;
  const ItemBase* base = item->item_base();
  if (!base) return ERR_ITEM_NOT_FOUND;

  if(this->level() < base->need_lv)
    return ERR_PARAM_INVALID;

  MessageResponseUseItem response;
  ModifyCurrency modify(MSG_CS_REQUEST_USE_ITEM, SYSTEM_ID_BAG);
  AddSubItemSet item_set;

  item_set.push_back(ItemParam(item->item_id(), -use_count));

  std::vector<int32_t> choose_item;
  choose_item.push_back(message->choose_item());

  std::pair<std::string, int32_t> goodid_pair =
      GetGoodIDByItemID(item->item_id());
  if (!goodid_pair.first.empty()) use_count = 1;

  for (int32_t i = 0; i < use_count; ++i) {
    const LootBase* loot =
        LootConfigFile::Get(base->effect, this->level()).get();
    if (!loot) break;
    loot->Loot(modify, item_set, &choose_item);
  }

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  if (item->item_id() == 23900011) {
    Army* army = server->GetArmyByID(this->army_id_);
    if (army)
      army->AddArmyExp(1000000 * use_count, this->uid(), entry.head.msgid);
  }
  if (!goodid_pair.first.empty()) {
    AddRecharge(GetVirtualSeconds(), goodid_pair.first, goodid_pair.second,
                true, entry.head.msgid);
  }

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_USE_ITEM,
                   SYSTEM_ID_BAG);
  for (AddSubItemSet::const_iterator it = item_set.begin();
       it != item_set.end(); ++it) {
    KVPair2* pair = response.add_results();
    pair->set_key(it->item_id);
    pair->set_value(it->item_count);
  }
  if (modify) {
    for (int32_t i = MoneyKind_MIN; i < MoneyKind_ARRAYSIZE; ++i) {
      if (modify[i]) {
        KVPair2* pair = response.add_results();
        pair->set_key(i);
        pair->set_value(modify[i]);
      }
    }
  }

  this->SendMessageToClient(MSG_CS_RESPONSE_USE_ITEM, &response);
  return ERR_OK;
}

void LogicPlayer::UpdateBuyCount(int32_t type, int32_t count) {
  if (daily_counter_[type] < 0)
    daily_counter_[type] -= count;
  else
    daily_counter_[type] += count;

  MessageNotifyBuyCount notify;
  sy::KVPair2* info = notify.add_infos();
  info->set_key(type);
  info->set_value(daily_counter_[type]);
  this->SendMessageToClient(MSG_CS_NOTIFY_BUY_COUNT, &notify);

  MessageSSUpdateBuyCount request;
  request.add_infos()->CopyFrom(*info);
  this->SendMessageToDB(MSG_SS_UPDATE_BUY_COUNT, &request);
}

void LogicPlayer::MakePlayerSimpleInfo(sy::PlayerSimpleInfo* simple) {
  if (simple) {
    simple->set_avatar(this->avatar());
    simple->set_name(this->name());
    simple->set_player_id(this->uid());
    simple->set_vip_level(this->vip_level());
    simple->set_level(this->level());
    simple->set_rank_id(this->rank_id());
  }
}

int32_t LogicPlayer::ProcessRequestSignIn(CSMessageEntry& entry) {
  time_t now_time = GetVirtualSeconds();

  if (IsSameDay(now_time, this->player_.sign_time())) return ERR_PARAM_INVALID;

  int32_t next_id = this->player_.sign_id() + 1;
  while (next_id > SignBase::max_id)
    next_id -= (SignBase::max_id - GetSettingValue(sign_loop_id) + 1);

  SignBase* base = SIGN_BASE.GetEntryByID(next_id).get();
  if (!base) return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_SIGN_IN, SYSTEM_ID_ACTIVITY);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  if (base->vipaward_lv && this->player_.vip_level() >= base->vipaward_lv) {
    modify *= 2;
    for (AddSubItemSet::iterator it = item_set.begin(); it != item_set.end();
         ++it)
      it->item_count *= 2;
  }

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_SIGN_IN,
                   SYSTEM_ID_ACTIVITY);

  this->player_.set_sign_time(now_time);
  this->player_.set_sign_id(this->player_.sign_id() + 1);

  MessageResponseSignIn response;
  response.set_sign_id(this->player_.sign_id());
  response.set_sign_time(this->player_.sign_time());
  this->SendMessageToClient(MSG_CS_RESPONSE_SIGN_IN, &response);

  MessageSSUpdateSignIn db_msg;
  db_msg.set_sign_id(this->player_.sign_id());
  db_msg.set_sign_time(this->player_.sign_time());
  this->SendMessageToDB(MSG_SS_UPDATE_SIGN_IN, &db_msg);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetRankAward(CSMessageEntry& entry) {
  if (this->pk_rank_reward_time_) return ERR_PARAM_INVALID;

  ArenaRewardIter base = ArenaConfigFile::GetDailyAwardByRank(pk_rank_reward_rank_);
  if (base == ArenaConfigFile::GetDailyAwardEnd()) {
    ERROR_LOG(logger)("SendPKRankReward Fail, Rank:%d", pk_rank_reward_rank_);
    return ERR_PARAM_INVALID;
  }

  ModifyCurrency modify(MSG_CS_REQUEST_GET_RANK_AWARD, SYSTEM_ID_RANK);
  AddSubItemSet item_set;

  FillCurrencyAndItem<kAdd>(modify, item_set, base->second.daily_reward);

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_RANK_AWARD,
                   SYSTEM_ID_RANK);

  this->pk_rank_reward_time_ = GetSeconds();

  MessageSSUpdatePKRankReward update;
  update.set_pk_rank_reward_rank(this->pk_rank_reward_time_);
  update.set_pk_rank_reward_time(this->pk_rank_reward_time_);
  SendMessageToDB(MSG_SS_UPDATE_PK_RANK_REWARD, &update);

  SendMessageToClient(MSG_CS_REQUEST_GET_RANK_AWARD, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetDailyAward(CSMessageEntry& entry) {
  MessageRequestGetDailyAward* message =
      static_cast<MessageRequestGetDailyAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_DAILY_AWARD,
                        SYSTEM_ID_DAILY_TASK);
  AddSubItemSet item_set;

  //积分奖励
  if (message->daily_type() == sy::COUNT_TYPE_DAILY_ACTIVITY) {
    int32_t need_num = message->activity_type();

    int32_t bit_flag = 0;
    if (30 == need_num) bit_flag = 1;
    if (60 == need_num) bit_flag = 2;
    if (90 == need_num) bit_flag = 4;
    if (120 == need_num) bit_flag = 8;

    if (this->daily_counter_[COUNT_TYPE_DAILY_ACTIVITY] & bit_flag)
      return ERR_PARAM_INVALID;

    //现有的积分
    int32_t activity_num = 0;
    for (VectorMap<int32_t, int32_t>::iterator it =
             this->daily_counter_.begin();
         it != this->daily_counter_.end(); ++it) {
      if (it->second < 0) {
        ActivityBase* other_base = ACTIVITY_BASE.GetEntryByID(it->first).get();
        if (!other_base) continue;
        activity_num += other_base->points;
      }
    }

    //需要积分
    ActivityRewardBase* activity_reward_base =
        ACTIVITY_REWARD_BASE.GetEntryByID(message->activity_type()).get();
    if (!activity_reward_base) return ERR_PARAM_INVALID;
    if (need_num > activity_num) return ERR_PARAM_INVALID;

    FillCurrencyAndItem<kAdd>(modify, item_set, activity_reward_base->reward);

    CHECK_RET(this->CheckCurrency(modify));
    CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

    this->daily_counter_[COUNT_TYPE_DAILY_ACTIVITY] |= bit_flag;
  } else {  //其他日常奖励

    ActivityBase* base =
        ACTIVITY_BASE.GetEntryByID(message->daily_type()).get();
    if (!base) return ERR_PARAM_INVALID;

    if (this->daily_counter_[base->id()] < 0 ||
        this->daily_counter_[base->id()] < base->time)
      return ERR_PARAM_INVALID;

    FillCurrencyAndItem<kAdd>(modify, item_set, base->reward);

    CHECK_RET(this->CheckCurrency(modify));
    CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

    this->daily_counter_[base->id()] *= -1;
  }

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_DAILY_AWARD,
                   SYSTEM_ID_DAILY_TASK);

  MessageNotifyBuyCount notify;
  sy::KVPair2* info = notify.add_infos();
  info->set_key(message->daily_type());
  info->set_value(daily_counter_[message->daily_type()]);
  this->SendMessageToClient(MSG_CS_NOTIFY_BUY_COUNT, &notify);

  MessageSSUpdateBuyCount update;
  update.add_infos()->CopyFrom(*info);
  this->SendMessageToDB(MSG_SS_UPDATE_BUY_COUNT, &update);

  SendMessageToClient(MSG_CS_RESPONSE_GET_DAILY_AWARD, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetAchievement(CSMessageEntry& entry) {
  MessageRequestGetAchievement* message =
      static_cast<MessageRequestGetAchievement*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t acheivement_id = message->achievement_id();

  AchievementBase* base = ACHIEVEMENT_BASE.GetEntryByID(acheivement_id).get();
  if (!base) return ERR_PARAM_INVALID;

  if (this->achievements_[base->type] >= acheivement_id)
    return ERR_PARAM_INVALID;

  bool is_access = false;

  switch (base->type) {
    case ACHIEVEMENT_TYPE_LEVEL:
    case ACHIEVEMENT_TYPE_LEVEL_V:
      if (this->level() >= base->time) is_access = true;
      break;
    case ACHIEVEMENT_TYPE_COPY_STARS:
      if (this->GetCopyStarByType(COPY_TYPE_NORMAL) >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_FIGHTING:
      if (this->fight_attr() >= base->time) is_access = true;
      break;
    case ACHIEVEMENT_TYPE_SHIP_UP:
      is_access = true;
      for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
        LogicHero* ship = GetHeroByUID(this->tactic_.infos(i).hero_uid());
        if (!ship || ship->first.level() < base->time) {
          is_access = false;
          break;
        }
      }
      break;
    case ACHIEVEMENT_TYPE_MAKE_SHIP:
      is_access = true;
      for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
        LogicHero* ship = GetHeroByUID(this->tactic_.infos(i).hero_uid());
        if (!ship || ship->first.fate_level() < base->time) {
          is_access = false;
          break;
        }
      }
      break;
    case ACHIEVEMENT_TYPE_TOWER:
      if (this->GetCopyStarByType(COPY_TYPE_TOWER) >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_VIP:
      if (this->vip_level() >= base->time) is_access = true;
      break;
    case ACHIEVEMENT_TYPE_PATROL_RIOT:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_PATROL_RIOT] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_PATROL_TIME:
      if (this->patrol_total_time_ >= base->time) is_access = true;
      break;
    case ACHIEVEMENT_TYPE_DSTRIKE:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_DSTRIKE] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_HARD_COPY_STARS:
      if (this->GetCopyStarByType(COPY_TYPE_HARD) >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_SHIP_AWAKENING:
      if (this->achievements_[ACHIEVEMENT_TYPE_MIN_WAKE_LEVEL] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_HARD_COPY: {
      sy::CopyProgress* progress = this->GetCopyProgress(COPY_TYPE_HARD);
      if (progress && progress->chapter() >= base->time) is_access = true;
    } break;
    case ACHIEVEMENT_TYPE_HARD_ENEMY:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_HARD_ENEMY] >=
          base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_RECHARGE:
      break;
    case ACHIEVEMENT_TYPE_BATTLE_FORMATIONS:
      break;
    case ACHIEVEMENT_TYPE_EQUIP:
      is_access = false;
      {
        int32_t equiped_ship = 0;
        for (int32_t i = 1; i <= 6; ++i) {
          RepeatedField<int64_t>* info = GetEquipsInfo(i);
          if (!info) break;
          int32_t equip_num = 0;
          for (int32_t i = 0; i < info->size() && i < 4; i++)
            if (info->Get(i)) equip_num++;
          if (equip_num >= 4) equiped_ship++;
        }
        if (equiped_ship >= base->time) is_access = true;
      }
      break;
    case ACHIEVEMENT_TYPE_ARMY:
      is_access = false;
      {
        int32_t equiped_ship = 0;
        for (int32_t i = 1; i <= 6; ++i) {
          RepeatedField<int64_t>* info = GetEquipsInfo(i);
          if (!info) break;
          int32_t equip_num = 0;
          for (int32_t i = 4; i < info->size() && i < 6; i++)
            if (info->Get(i)) equip_num++;
          if (equip_num >= 2) equiped_ship++;
        }
        if (equiped_ship >= base->time) is_access = true;
      }
      break;
    case ACHIEVEMENT_TYPE_ROB_NAVY:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_ROB_NAVY] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_BLUE_NAVY:
      if (this->achievements_[ACHIEVEMENT_TYPE_NAVY_COUNT] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_PURPLE_NAVY:
      if (this->achievements_[ACHIEVEMENT_TYPE_NAVY_PURPLE_COUNT] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_ORANGE_NAVY:
      if (this->achievements_[ACHIEVEMENT_TYPE_NAVY_ORANGE_COUNT] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_EQUIP_LEVEL:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_EQUIP_LEVEL] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_EQUIP_REFINE_LEVEL:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_EQUIP_REFINE_LEVEL] >=
          base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_AEMY_LEVEL:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_AEMY_LEVEL] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_AEMY_REFINE_LEVEL:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_AEMY_REFINE_LEVEL] >=
          base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_SWEEP_COPY:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_SWEEP_COPY] >= base->time)
        is_access = true;
      break;
    case ACHIEVEMENT_TYPE_COPY_ID: {
      CopyProgress* progress = this->GetCopyProgress(COPY_TYPE_NORMAL);
      if (progress) {
        CopyBase* my_copy = COPY_BASE.GetEntryByID(progress->copy_id()).get();
        CopyBase* copy = COPY_BASE.GetEntryByID(base->time).get();
        if (my_copy && copy && my_copy->order >= copy->order) is_access = true;
      }
    } break;
    case ACHIEVEMENT_TYPE_JOIN_ARMY:
      if (this->achievements_[ACHIEVEMENT_TYPE_COUNT_JOIN_ARMY] >= base->time)
        is_access = true;
      break;
  }

  if (!is_access) return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_ACHIEVEMENT, SYSTEM_ID_ACHIEVEMENT);
  AddSubItemSet item_set;

  FillCurrencyAndItem<kAdd>(modify, item_set, base->reward);

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_ACHIEVEMENT,
                   SYSTEM_ID_ACHIEVEMENT);

  this->UpdateAchievement(base->type, acheivement_id);

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ACHIEVEMENT, NULL);

  return ERR_OK;
}

void LogicPlayer::UpdateAchievement(int32_t type, int32_t value) {
  if (type >= SEVEN_DAY_TYPE_BEGIN && type < SEVEN_DAY_TYPE_END) {
    if (GetZeroClock(this->player_.create_time()) + 7 * 24 * 3600 <
        GetVirtualSeconds())
      return;
  }
  if (type >= FOURTEEN_DAY_TYPE_BEGIN && type < FOURTEEN_DAY_TYPE_END) {
    SevenDays14Base* base =
        SEVEN_DAYS_14_BASE.GetEntryByID(type * 100 + 1).get();
    if (!base) return;
    if (!IsSameDay(this->player_.create_time() + (base->day + 6) * 24 * 3600,
                   GetVirtualSeconds()))
      return;
  }
  this->achievements_[type] = value;

  MessageNotifyAchievement notify;
  sy::KVPair2* pair = notify.add_achievements();
  pair->set_key(type);
  pair->set_value(value);
  this->SendMessageToClient(MSG_CS_NOTIFY_ACHIEVEMENT_COUNT, &notify);

  MessageSSUpdateAchievement update;
  pair = update.add_infos();
  pair->set_key(type);
  pair->set_value(value);
  this->SendMessageToDB(MSG_SS_UPDATE_ACHIEVEMENT, &update);
}

int32_t LogicPlayer::ProcessRequestGetMailReward(CSMessageEntry& entry) {
  MessageRequestGetMailReward* message =
      static_cast<MessageRequestGetMailReward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSRequestGetMailReward request;
  request.set_mail_id(message->mail_id());
  this->SendMessageToDB(MSG_SS_REQUEST_GET_MAIL_REWARD, &request);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseGetMailReward(SSMessageEntry& entry) {
  MessageSSResponseGetMailReward* message =
      static_cast<MessageSSResponseGetMailReward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->result_id()) return message->result_id();

  ModifyCurrency modify(MSG_CS_REQUEST_GET_MAIL_REWARD, SYSTEM_ID_MAIL);
  AddSubItemSet item_set;

  for (int32_t i = 0; i < message->rewards_size(); i++) {
    const KVPair2& pair = message->rewards(i);

    if (pair.key() < 100) {
      modify[pair.key()] += pair.value();
    } else {
      item_set.push_back(ItemParam(pair.key(), pair.value()));
    }
  }

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  if (mail_reward_temp_id_.insert(message->mail_id()).second == false)
    return ERR_PARAM_INVALID;

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_MAIL_REWARD,
                   SYSTEM_ID_MAIL);

  MessageResponseGetMailReward response;
  response.set_mail_id(message->mail_id());
  SendMessageToClient(MSG_CS_RESPONSE_GET_MAIL_REWARD, &response);

  MessageSSNotifyGetMailReward notify;
  notify.set_mail_id(message->mail_id());
  SendMessageToDB(MSG_SS_NOTIFY_GET_MAIL_REWARD, &notify);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestUpRankInfo(CSMessageEntry& entry) {
  int32_t next_rank = 0;

  if (this->player_.rank_id() >= GetSettingValue(rank_end_id))
    return ERR_PARAM_INVALID;

  if (this->player_.rank_id() == 0) {
    next_rank = GetSettingValue(rank_start_id);
  } else {
    next_rank = this->player_.rank_id() + 1;
  }

  RankBase* base = RANK_BASE.GetEntryByID(next_rank).get();
  if (!base) return ERR_INTERNAL;

  if (this->GetCopyStarByType(COPY_TYPE_NORMAL) < base->cost)
    return ERR_PARAM_INVALID;

  if (base->reward.size() > 0) {
    ModifyCurrency modify(MSG_CS_REQUEST_RANK_INFO, SYSTEM_ID_MILITARY_RANK);
    AddSubItemSet item_set;

    FillCurrencyAndItem<kAdd>(modify, item_set, base->reward);

    CHECK_RET(this->CheckCurrency(modify));
    CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

    this->UpdateCurrency(modify);
    this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_RANK_INFO,
                     SYSTEM_ID_MILITARY_RANK);
  }

  this->player_.set_rank_id(next_rank);

  this->CalcRank();
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_MILITARY_RANK, entry.head.msgid);

  MessageResponseGetRankInfo response;
  response.set_rank_id(next_rank);
  SendMessageToClient(MSG_CS_RESPONSE_RANK_INFO, &response);

  MessageSSUpdateRankID update;
  update.set_rank_id(next_rank);
  SendMessageToDB(MSG_SS_UPDATE_RANK_ID, &update);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPatrolHelp(CSMessageEntry& entry) {
  VipFunctionBase* vip_base = VIP_FUNCTION_BASE.GetEntryByID(362).get();
  if (!vip_base) return ERR_INTERNAL;

  int32_t max_count = vip_base->GetValue(this->vip_level(), this->level());
  if (abs(this->daily_counter_[sy::COUNT_TYPE_DAILY_PATROL_RIOT]) >= max_count)
    return ERR_CURRENCY_VIP_EXP;

  ModifyCurrency modify(MSG_CS_REQUEST_PATROL_HELP, SYSTEM_ID_HOOK);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, Setting::GetValue2(Setting::patrol_help_reward));

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_PATROL_HELP,
                   SYSTEM_ID_HOOK);

  this->UpdateBuyCount(COUNT_TYPE_DAILY_PATROL_RIOT, 1);
  this->UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_PATROL_RIOT, this->achievements_[ACHIEVEMENT_TYPE_COUNT_PATROL_RIOT] + 1);
  this->SendMessageToClient(MSG_CS_RESPONSE_PATROL_HELP, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetSevenDays(CSMessageEntry& entry) {
  MessageRequestGetSevenDays* message =
      static_cast<MessageRequestGetSevenDays*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (GetZeroClock(this->player_.create_time()) +
          GetSettingValue(sevendays_award_day) * 24 * 3600 <
      GetVirtualSeconds())
    return ERR_PARAM_INVALID;

  SevenDaysBase* base =
      SEVEN_DAYS_BASE.GetEntryByID(message->seven_day_id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (GetZeroClock(this->player_.create_time()) + (base->day - 1) * 24 * 3600 >
      GetVirtualSeconds())
    return ERR_PARAM_INVALID;

  if (base->reward.empty()) return ERR_PARAM_INVALID;

  if (achievements_[base->type + 100] & (1 << (base->index - 1)))
    return ERR_PARAM_INVALID;

  if (base->type == SEVEN_DAY_TYPE_RECHARGE) {
    time_t tm_d = this->player_.create_time() + (base->day - 1) * 3600 * 24;
    if (GetTodayRechargeNum(tm_d) < base->condition) return ERR_PARAM_INVALID;
  } else if (base->type == SEVEN_DAY_TYPE_PK ||
             base->type == SEVEN_DAY_TYPE_TOWER_RANK) {
    if (achievements_[base->type] == 0 ||
        achievements_[base->type] > base->condition)
      return ERR_PARAM_INVALID;
  } else {
    if (achievements_[base->type] < base->condition) return ERR_PARAM_INVALID;
  }

  ModifyCurrency modify(MSG_CS_REQUEST_GET_SEVEN_DAYS,
                        SYSTEM_ID_SEVEN_ACTIVITY);
  AddSubItemSet item_set;

  for (size_t i = 0; i < base->reward.size(); i++) {
    const LootBasePtr& loot_base =
        LootConfigFile::Get(base->reward[i], this->level());

    std::vector<int32_t> param;

    if (SEVEN_DAY_TYPE_LOGIN_SHIP == base->type)
      param.push_back(message->ship_id());
    else
      param.push_back((int32_t) this->uid());

    if (loot_base) loot_base->Loot(modify, item_set, &param);
  }

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_SEVEN_DAYS,
                   SYSTEM_ID_SEVEN_ACTIVITY);

  int32_t change_value =
      achievements_[base->type + 100] | (1 << (base->index - 1));
  UpdateAchievement(base->type + 100, change_value);
  SendMessageToClient(MSG_CS_RESPONSE_GET_SEVEN_DAYS, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetFourteenDays(CSMessageEntry& entry) {
  MessageRequestGetFourteenDays* message =
      static_cast<MessageRequestGetFourteenDays*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (GetZeroClock(this->player_.create_time()) +
          GetSettingValue(sevendays14_award_day) * 24 * 3600 <
      GetVirtualSeconds())
    return ERR_PARAM_INVALID;

  SevenDays14Base* base =
      SEVEN_DAYS_14_BASE.GetEntryByID(message->fourteen_day_id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (GetZeroClock(this->player_.create_time()) + (base->day + 6) * 24 * 3600 >
      GetVirtualSeconds())
    return ERR_PARAM_INVALID;

  if (base->reward.empty()) return ERR_PARAM_INVALID;

  if (achievements_[base->type + 100] & (1 << (base->index - 1)))
    return ERR_PARAM_INVALID;

  if (base->type >= FOURTEEN_DAY_TYPE_RECHARGE1 &&
      base->type <= FOURTEEN_DAY_TYPE_RECHARGE7) {
    if (!TodayHasRechargeOrder(base->condition*10)) return ERR_PARAM_INVALID;
  } else if (achievements_[base->type] < base->condition)
    return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_FOURTEEN_DAYS,
                        SYSTEM_ID_SEVEN_ACTIVITY);
  AddSubItemSet item_set;

  for (size_t i = 0; i < base->reward.size(); i++) {
    const LootBasePtr& loot_base =
        LootConfigFile::Get(base->reward[i], this->level());
    std::vector<int32_t> param;

    if (FOURTEEN_DAY_TYPE_LOGIN_SHIP == base->type)
      param.push_back(message->ship_id());
    else
      param.push_back((int32_t) this->uid());

    if (loot_base) loot_base->Loot(modify, item_set, &param);
  }

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_FOURTEEN_DAYS,
                   SYSTEM_ID_SEVEN_ACTIVITY);

  int32_t change_value =
      achievements_[base->type + 100] | (1 << (base->index - 1));
  UpdateAchievement(base->type + 100, change_value);
  SendMessageToClient(MSG_CS_RESPONSE_GET_FOURTEEN_DAYS, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetServerShopInfo(CSMessageEntry& entry) {
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_SERVER_SHOP_INFO, &server->server_shop());
  return ERR_OK;
}

bool LogicPlayer::CheckRelation(LogicHero* ship, int32_t relation_id) {
  int32_t equip_index = this->IsInTactic(ship->first.uid());
  if (!equip_index) return false;
  const RelationBase* base = RELATION_BASE.GetEntryByID(relation_id).get();
  if (!base) return false;

  if (base->hero1_id.size()) {
    Array<int32_t, 64> hero_set;
    for (int32_t j = 0; j < this->tactic_.infos_size(); ++j) {
      LogicHero *hero = this->GetHeroByUID(this->tactic_.infos(j).hero_uid());
      if (hero) hero_set.push_back(hero->second->id());
    }
    for (int32_t j = 0; j < this->tactic_.support_pos_size(); ++j) {
      LogicHero* hero = this->GetHeroByUID(this->tactic_.support_pos(j).hero_uid());
      if (hero) hero_set.push_back(hero->second->id());
    }
    for (size_t i = 0; i < base->hero1_id.size(); ++i) {
      if (std::find(hero_set.begin(), hero_set.end(), base->hero1_id[i]) == hero_set.end())
        return false;
    }
  }
  if (base->equip) {
    RepeatedField<int64_t>* equips = this->GetEquipsInfo(equip_index);
    bool found = false;
    if (!equips) return false;
    for (int32_t i = 0; i < equips->size(); ++i) {
      LogicItem* item = this->items_.GetItemByUniqueID(equips->Get(i));
      if (item && item->item_id() == base->equip) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }

  return true;
}

bool LogicPlayer::ReCalcRelation() {
  for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
    int64_t uid = this->tactic_.infos(i).hero_uid();
    if (!uid) continue;
    LogicHero* hero = this->GetHeroByUID(uid);
    if (!hero) continue;

    for (int32_t relation_index = 0;
         relation_index < hero->first.relation_size(); /*++relation_index*/) {
      if (this->CheckRelation(hero, hero->first.relation(relation_index))) {
        ++relation_index;
      } else {
        hero->first.mutable_relation()->SwapElements(
            relation_index, hero->first.relation_size() - 1);
        hero->first.mutable_relation()->Resize(hero->first.relation_size() - 1, 0);
      }
    }
  }
  return false;
}

int32_t LogicPlayer::ProcessRequestActiveRelation(CSMessageEntry& entry) {
  MessageRequestActiveRelation* message =
      static_cast<MessageRequestActiveRelation*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (!this->IsInTactic(message->hero_uid())) return ERR_INTERNAL;

  LogicHero *hero = this->GetHeroByUID(message->hero_uid());
  if (!hero) return ERR_HERO_NOT_FOUND;
  if (message->relations_size()) {
    hero->first.mutable_relation()->Clear();
    *hero->first.mutable_relation() = message->relations();
    this->ReCalcRelation();
  } else {
    //该船只没有这个缘分
    if (std::find(hero->second->karma1.begin(), hero->second->karma1.end(),
                  message->relation_id()) == hero->second->karma1.end()) {
      return ERR_PARAM_INVALID;
    }
    //已经激活了缘分
    if (std::find(hero->first.mutable_relation()->begin(),
                  hero->first.mutable_relation()->end(),
                  message->relation_id()) !=
        hero->first.mutable_relation()->end()) {
      return ERR_OK;
    }

    //条件不满足
    if (!this->CheckRelation(hero, message->relation_id()))
      return ERR_PARAM_INVALID;
    hero->first.mutable_relation()->Add(message->relation_id());
  }

  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_TACTIC, entry.head.msgid);
  this->SendMessageToClient(MSG_CS_RESPONSE_ACTIVE_RELATION, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestActiveRelationAll(CSMessageEntry& entry) {
  for (int32_t i = 0; i < this->tactic_.infos_size(); ++i) {
    int64_t uid = this->tactic_.infos(i).hero_uid();
    if (!uid) continue;
    LogicHero* hero = this->GetHeroByUID(uid);
    if (!hero) continue;
    HeroBase* base = HERO_BASE.GetEntryByID(hero->first.hero_id()).get();
    if (!base) continue;

    hero->first.mutable_relation()->Clear();
    for (size_t j = 0; j < base->karma1.size(); j++)
      hero->first.add_relation(base->karma1[j]);
  }
  this->ReCalcRelation();
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_TACTIC, entry.head.msgid);
  this->SendMessageToClient(MSG_CS_RESPONSE_ACTIVE_RELATION_ALL, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestChangeCarrier(CSMessageEntry& entry) {
  MessageRequestChangeCarrier* message =
      static_cast<MessageRequestChangeCarrier*>(entry.get());
  if (!message) return ERR_INTERNAL;

  sy::CarrierInfo* current_carrier =
      this->GetCarrierByID(message->carrier_id());
  if (!current_carrier) return ERR_CARRIER_NOT_FOUND;
  const CarrierBasePtr& carrier_base =
      CARRIER_BASE.GetEntryByID(message->carrier_id());
  if (!carrier_base) return ERR_CARRIER_NOT_FOUND;

  for (size_t i = 1; i <= 6; ++i) {
    RepeatedField<int64_t>* equips = GetEquipsInfo(i);
    if (equips) {
      if (equips->Get(6) == carrier_base->id()) return ERR_PARAM_INVALID;
    }
  }

  VectorMap<int32_t, int32_t> plane_map;
  AddSubItemSet item_set;

  for (int32_t i = 0; i < current_carrier_.plane_id_size(); i++) {
    int32_t plane_id = current_carrier_.plane_id(i);
    if (plane_id) plane_map[plane_id]++;
  }

  for (VectorMap<int32_t, int32_t>::const_iterator it = plane_map.begin();
       it != plane_map.end(); ++it)
    item_set.push_back(ItemParam(it->first, it->second));

  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_CHANGE_CARRIER,
             SYSTEM_ID_CARRIER_BUILD);

  this->current_carrier_.clear_plane_id();
  this->current_carrier_.set_carrier_id(message->carrier_id());
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_CARRIER_BUILD, entry.head.msgid);
  this->SendMessageToClient(MSG_CS_RESPONSE_CHANGE_CARRIER, NULL);

  return ERR_OK;
}

bool LogicPlayer::CheckBanToLogin() {
  if (this->player_.status() == 2 &&
      GetSeconds() <= this->player_.status_time()) {
    MessageNotifyBanToLogin notify;
    notify.set_status(this->player_.status());
    notify.set_status_time(this->player_.status_time());
    this->SendMessageToClient(MSG_CS_NOTIFY_BAN_TO_LOGIN, &notify);
    const TcpSessionPtr& ptr = this->session().lock();
    if (ptr) ptr->Shutdown();
    return true;
  }
  return false;
}

static inline bool IsShip(int ship_id) {
  return ship_id >= Setting::kShipStartID &&
         ship_id <= Setting::kShipEndID;
}

void LogicPlayer::DetachShipItem(const AddSubItemSet& input,
                                 AddSubItemSet* out_items,
                                 AddSubItemSet* out_ships) {
  for (size_t i = 0; i < input.size(); ++i) {
    if (IsShip(input[i].item_id)) {
      if (out_ships) out_ships->push_back(input[i]);
    } else {
      if (out_items) out_items->push_back(input[i]);
    }
  }
}

bool LogicPlayer::AddShipItem(const AddSubItemSet& ships, int32_t sys_id,
                              int32_t msg_id) {
  if (0 == ships.size()) return true;
  std::vector<sy::HeroInfo> hero_infos;
  for (size_t i = 0; i < ships.size(); i++) {
    if (IsShip(ships[i].item_id) && ships[i].item_count > 0) {
      sy::HeroInfo info;
      info.set_hero_id(ships[i].item_id);
      hero_infos.push_back(info);
    }
  }
  return this->UpdateHeroInfo(hero_infos, sys_id, msg_id);
}

int32_t LogicPlayer::ProcessRequestDialog(CSMessageEntry& entry) {
  MessageRequestDialog* message =
      static_cast<MessageRequestDialog*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->dialog_id().size() >= 256) return ERR_PARAM_INVALID;
  if (message->story_id().size() >= 256) return ERR_PARAM_INVALID;

  MessageSSUpdateDialog response;

  if (message->has_dialog_id()) {
    this->player_.set_dialog_id(message->dialog_id());
    response.set_dialog_id(this->player_.dialog_id());
    TRACE_LOG(logger)("PlayerID:%ld, DialogID:%s", this->uid(), message->dialog_id().c_str());
  }

  if (message->has_story_id()) {
    this->player_.set_story_id(message->story_id());
    response.set_story_id(this->player_.story_id());
    TRACE_LOG(logger)("PlayerID:%ld, StoryID:%s", this->uid(), message->story_id().c_str());
  }

  if (response.has_dialog_id() || response.has_story_id())
    this->SendMessageToDB(MSG_SS_UPDATE_DIALOG, &response);

  this->SendMessageToClient(MSG_CS_RESPONSE_DIALOG, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestClientFlag(CSMessageEntry& entry) {
  MessageRequestClientFlag* message =
      static_cast<MessageRequestClientFlag*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->client_flag().size() >= 1024) return ERR_PARAM_INVALID;

  this->player_.set_client_flag(message->client_flag());

  MessageSSUpdateClientFlag response;
  response.set_client_flag(this->player_.client_flag());
  this->SendMessageToDB(MSG_SS_UPDATE_CLIENT_FLAG, &response);

  this->SendMessageToClient(MSG_CS_RESPONSE_CLIENT_FLAG, NULL);
  return ERR_OK;
}

void LogicPlayer::SetLevel(int32_t level) {
  int32_t old_level = this->level();
  this->player_.set_level(level);
  this->player_.set_exp(0);
  this->OnLevelChanged(old_level, level);
}

void LogicPlayer::SetVipLevel(int32_t vip_level) {
  int32_t old_level = this->vip_level();
  this->player_.set_vip_level(vip_level);
  this->player_.set_vip_exp(0);
  this->OnVipLevelChanged(old_level, vip_level);
}

void LogicPlayer::CalcRank() {
  rank_attr_.resize(0);
  rank_attr_.resize(AttackAttr_ARRAYSIZE);
  for (int32_t i = GetSettingValue(rank_start_id); i <= this->player_.rank_id();
       i++) {
    RankBase* base = RANK_BASE.GetEntryByID(i).get();
    if (base) {
      for (ValuePair2Vec::iterator it = base->add.begin();
           it != base->add.end(); ++it) {
        if (it->v1) rank_attr_[it->v1] += it->v2;
      }
    }
  }
}

void LogicPlayer::CalcChart() {
  chart_attr_.resize(0);
  chart_attr_.resize(AttackAttr_ARRAYSIZE);

  for (std::vector<ChartBase*>::iterator it_base = ChartBase::charts.begin();
       it_base != ChartBase::charts.end(); ++it_base) {
    ChartBase* base = *it_base;
    if (!base) continue;

    bool sucess = true;
    for (std::vector<int32_t>::iterator it_carr = base->need.begin();
         it_carr != base->need.end(); ++it_carr) {
      if (obtained_carriers_.end() == obtained_carriers_.find(*it_carr)) {
        sucess = false;
        break;
      }
    }

    if (!sucess) continue;

    for (ValuePair2Vec::iterator it = base->property.begin();
         it != base->property.end(); ++it) {
      if (it->v1) chart_attr_[it->v1] += it->v2;
    }
  }
}

int32_t LogicPlayer::TestPK(int64_t player_id) {
  LogicPlayer* player = server->GetPlayerByID(player_id);
  if (!player || !player->load_complete()) return ERR_INTERNAL;
  const CopyBasePtr& base = COPY_BASE.GetEntryByID(62650);
  if (!base) return ERR_INTERNAL;
  PK pk(base.get());
  pk.InitPlayer(this, true);
  pk.InitPlayer(player, false);
  pk.GeneratePVPReport();

  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestTestPK(CSMessageEntry& entry) {
  MessageRequestTestPK* message =
      static_cast<MessageRequestTestPK*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicPlayer* player = server->GetPlayerByID(message->player_id());
  if (!player || !player->load_complete()) {
    MessageSSRequestLoadMultiPlayer request;
    request.set_msg_id(entry.head.msgid);
    request.set_conn_id(0);
    request.set_session_id(entry.session_ptr->GetSessionID());
    request.set_forward_player(this->uid());
    request.set_pk_player(this->uid());
    request.add_player_ids(message->player_id());
    request.add_forward_ids(message->player_id());

    server->SendServerMessageToDB(MSG_SS_REQUEST_LOAD_MULTI_PLAYER, &request);
    return ERR_OK;
  }

  return this->TestPK(message->player_id());
}

int32_t LogicPlayer::ProcessRequestGetRandomFriend(CSMessageEntry& entry) {
  server->RandomShuffleFriend(this);
  return ERR_OK;
}

int32_t LogicPlayer::GetRelationship(int32_t uid) {
  for (int32_t i = 0; i < this->friends_.infos_size(); ++i) {
    const FriendInfo& info = this->friends_.infos(i);
    if (info.friend_id() == uid) return info.type();
  }
  return 0;
}

int32_t LogicPlayer::GetCurrency(int kind) {
  switch (kind) {
    case sy::MONEY_KIND_COIN:
      return this->player_.coin();
    case sy::MONEY_KIND_MONEY:
      return this->player_.money();
    case sy::MONEY_KIND_OIL:
      return this->player_.oil();
    case sy::MONEY_KIND_ENERGY:
      return this->player_.energy();
    case sy::MONEY_KIND_EXP:
      return this->player_.exp();
    case sy::MONEY_KIND_VIPEXP:
      return this->player_.vip_exp();
    case sy::MONEY_KIND_HERO:
      return this->player_.hero();
    case sy::MONEY_KIND_PLANE:
      return this->player_.plane();
    case sy::MONEY_KIND_PRESTIGE:
      return this->player_.prestige();
    case sy::MONEY_KIND_MUSCLE:
      return this->player_.muscle();
    case sy::MONEY_KIND_EXPLOIT:
      return this->player_.exploit();
    case sy::MONEY_KIND_UNION:
      return this->player_.union_();
  }
  return 0;
}

void LogicPlayer::SendCopyStatus(const CopyBase* base, int32_t star) {
  MessageSSUpdateCopyStatus msg;
  msg.set_tid(server->GetTID());
  msg.set_copy_type(base->type);
  msg.set_copy_id(base->id());
  msg.set_copy_star(star);
  msg.set_player_id(this->uid());
  msg.set_copy_order(base->GetOrder());

  this->SendMessageToDB(MSG_SS_UPDATE_COPY_STATUS, &msg);
}

//RecordPlayer保活
void LogicPlayer::active() {
  if (!this->last_heart_beat_time_) {
    this->last_heart_beat_time_ = GetSeconds();
  }
  if (GetSeconds() - this->last_heart_beat_time_ >= 600) {
    this->last_heart_beat_time_ = GetSeconds();
    this->SendMessageToDB(MSG_SS_PLAYER_HEART_BEAT, NULL);
  }
  Player::active();
}

int32_t LogicPlayer::ProcessRequestEquipCarrier(CSMessageEntry& entry) {
  MessageRequestEquipCarrier* message =
      static_cast<MessageRequestEquipCarrier*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const int32_t solt = 6;

  if (message->carrier_id()) {
    sy::CarrierInfo* carrier = this->GetCarrierByID(message->carrier_id());
    if (!carrier) return ERR_CARRIER_NOT_FOUND;
  }

  if (this->current_carrier_.carrier_id() &&
      (this->current_carrier_.carrier_id() == message->carrier_id()))
    return ERR_PARAM_INVALID;

  int carrier_num = 0;
  for (size_t i = 1; i <= 6; ++i) {
    RepeatedField<int64_t>* info = this->GetEquipsInfo(i);
    if (!info) return ERR_INTERNAL;
    info->Resize(MAX_EQUIPED_ITEM_COUNT, 0);
    if (message->carrier_id() && (message->carrier_id() == info->Get(solt)))
      return ERR_PARAM_INVALID;
    if (info->Get(solt)) carrier_num++;
  }

  RepeatedField<int64_t>* equips_info =
      this->GetEquipsInfo(message->position());
  if (!equips_info) return ERR_PARAM_INVALID;
  if (!equips_info->Get(solt)) {
    if (this->level() < Setting::GetValueInVec2(Setting::carrier_RemoteSupport,
                                                carrier_num + 1))
      return ERR_PARAM_INVALID;
  }

  equips_info->Set(solt, message->carrier_id());

  this->OnEquipChanged(0);
  this->UpdateEquipsInfo(message->position());

  MessageResponseEquipCarrier response;
  response.set_position(message->position());
  this->SendMessageToClient(MSG_CS_RESPONSE_EQUIP_CARRIER, &response);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetArmyInfo(CSMessageEntry& entry) {
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;

  army->RefreshArmyShop();
  MessageResponseGetArmyInfo response;
  response.mutable_army()->CopyFrom(army->info());
  for (std::vector<sy::ArmyMemberInfo>::iterator iter = army->members().begin();
       iter != army->members().end(); ++iter) {
    sy::ArmyMemberInfo* info = response.add_member();
    info->CopyFrom(*iter);
    if (!IsSameDay(info->update_time(), GetVirtualSeconds()))
      info->set_today_exp(0);
    LogicPlayer *player = server->GetPlayerByID(info->player_id());
    if (player && player->is_online())
      info->set_is_online(true);
    TRACE_LOG(logger)("Player uid:%ld, update_time:%d", info->player_id(), info->update_time());
  }
  for (std::vector<sy::ArmyApplyInfo>::iterator iter = army->applies().begin();
       iter != army->applies().end(); ++iter) {
    sy::ArmyApplyInfo* info = response.add_apply();
    info->CopyFrom(*iter);
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ARMY_INFO, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestCreateArmy(CSMessageEntry& entry) {
  MessageRequestCreateArmy* message = static_cast<MessageRequestCreateArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (this->army_id()) return ERR_ARMY_JOINED;
  if (this->vip_level() < GetSettingValue(league_create_VIP_level))
    return ERR_CURRENCY_VIP_EXP;
  if ((GetVirtualSeconds() - this->army_leave_time_) <
      GetSettingValue(league_leave_time))
    return ERR_ARMY_LEAVE_CD;

  if (message->army_name().length() >= 63u) return ERR_ARMY_NAME_TOO_LONG;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_LEAGUE);
  modify.money -= GetSettingValue(league_create);
  CHECK_RET(this->CheckCurrency(modify));

  int64_t army_id = server->GetNewArmyID();
  if (ContainsInjection(message->army_name())) return ERR_PLAYER_ILLEGAL_CHARACTER;
  if (server_config->dirtywords() && ContainsDirtyWords(message->army_name()))
    return ERR_PLAYER_ILLEGAL_CHARACTER;

  MessageSSRequestCreateArmy request;
  request.set_server(server_config->server_id());

  sy::ArmyInfo* army = request.mutable_info();
  army->set_army_id(army_id);
  army->set_level(1);
  army->set_master_id(this->uid());
  army->set_master_name(this->name());
  army->set_army_name(message->army_name());
  army->set_avatar(message->avatar());

  sy::ArmyMemberInfo* member = request.mutable_master();
  this->MakeArmyMemberInfo(member);
  member->set_army_id(army_id);
  member->set_position(ARMY_POSITION_MASTER);

  this->DeleteApply(this->uid());

  server->SendServerMessageToDB(MSG_SS_REQUEST_CREATE_ARMY, &request);
  return ERR_OK;
}

void LogicPlayer::UpdateArmyMember() {
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return;
  ArmyMemberInfo* info = army->GetMember(this->uid());
  if (!info) return;
  MakeArmyMemberInfo(info);
  UpdateArmyMemberInfo(*info);
}

void LogicPlayer::MakeArmyMemberInfo(sy::ArmyMemberInfo* info) {
  info->set_player_id(this->uid());
  info->set_fight(this->fight_attr_);
  info->set_level(this->level());
  info->set_vip_level(this->vip_level());
  info->set_name(this->name());
  info->set_avatar(this->avatar());
  if (GetSecondsDiffDays(info->update_time() - Setting::kRefreshSeconds,
                         GetVirtualSeconds() - Setting::kRefreshSeconds)) {
    info->set_today_exp(0);
  }
  info->set_update_time(GetVirtualSeconds());
}

void LogicPlayer::DeleteArmyMember(int64_t army_id, int64_t member_id) {
  MessageSSUpdateArmyMember update;
  update.set_army_id(army_id);
  update.set_member_id(member_id);

  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_MEMBER, &update);
}

void LogicPlayer::UpdateArmyMemberInfo(const sy::ArmyMemberInfo& info) {
  MessageSSUpdateArmyMember update;
  update.mutable_member()->CopyFrom(info);
  update.set_server(server_config->server_id());

  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_MEMBER, &update);
}

int32_t LogicPlayer::ProcessResponseCreateArmy(sy::ArmyInfo& army) {
  //设置Player的军团信息
  MessageSSUpdateLeguageInfo update_player;
  this->army_id_ = army.army_id();
  update_player.set_army_id(this->army_id_);
  update_player.set_leave_time(this->army_leave_time_);
  this->SendMessageToDB(MSG_SS_UPDATE_LEGUAGE_INFO, &update_player);

  //扣钱
  ModifyCurrency modify(MSG_CS_REQUEST_CREATE_ARMY, SYSTEM_ID_LEAGUE);
  modify.money -= GetSettingValue(league_create);
  if (modify.money + this->player_.money() <= 0) {
    modify.money = -this->player_.money();
    INFO_LOG(logger)("CreateArmy, PlayerID:%ld, Money:%ld", this->uid(), this->player_.money());
  }
  this->UpdateCurrency(modify);

  MessageResponseCreateArmy response;
  response.set_army_name(army.army_name());
  this->SendMessageToClient(MSG_CS_RESPONSE_CREATE_ARMY, &response);

  CSMessageEntry empty;
  this->ProcessRequestGetArmyInfo(empty);
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_LEAGUE,
                       MSG_CS_REQUEST_CREATE_ARMY);
  UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_JOIN_ARMY, 1);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestDismissArmy(CSMessageEntry& entry) {
  int32_t army_id = this->army_id_;
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;
  if (army->members().size() > 1) return ERR_ARMY_ARMY_CANT_DISMISS;
  const ArmyMemberInfo* info = army->GetArmyMemberInfo(this->uid());
  if (!info) return ERR_ARMY_MEMBER_NOT_FOUND;
  if (info->position() != sy::ARMY_POSITION_MASTER) return ERR_ARMY_PERMISSION;
  server->DismissArmy(this->army_id_);
  this->SendMessageToClient(MSG_CS_RESPONSE_DISMISS_ARMY, NULL);
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_LEAGUE,
                       MSG_CS_REQUEST_DISMISS_ARMY);

  server->DeletePlayerKeyVlaue(army_id, KVStorage::kKVTypePearlHarbor);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestAgreeArmyApply(CSMessageEntry& entry) {
  MessageRequestAgreeArmyApply* message =
      static_cast<MessageRequestAgreeArmyApply*>(entry.get());
  if (!message) return ERR_INTERNAL;
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;
  const sy::ArmyMemberInfo* member = army->GetArmyMemberInfo(this->uid());
  if (!member) return ERR_INTERNAL;
  if (member->position() < sy::ARMY_POSITION_VP) return ERR_ARMY_PERMISSION;
  sy::ArmyApplyInfo* apply = army->GetApply(message->player_id());
  if (!apply) return ERR_PARAM_INVALID;
  const LeagueBase* base = LEAGUE_BASE.GetEntryByID(army->level()).get();
  if (!base) {
    ERROR_LOG(logger)("AgreeArmyApply, ArmyID:%ld, level:%d", army->army_id(), army->level());
    return ERR_INTERNAL;
  }

  MessageResponseAgreeArmyApply response;
  response.set_is_agree(message->is_agree());
  response.set_player_id(message->player_id());

  LogicPlayer* player = server->GetPlayerByID(apply->player_id());
  sy::ArmyMemberInfo info;

  if (message->is_agree()) {
    if ((int32_t)army->members().size() >= base->limit)
      return ERR_ARMY_MEMBER_COUNT;
    server->OnPlayerJoinArmy(apply->player_id(), army);

    info.set_position(sy::ARMY_POSITION_MEMBER);
    info.set_army_id(army->army_id());
    info.set_update_time(GetVirtualSeconds());
    info.set_player_id(apply->player_id());
    info.set_name(apply->name());
    info.set_level(apply->level());
    info.set_vip_level(apply->vip_level());
    info.set_fight(apply->fight());
    info.set_avatar(apply->avatar());
    if (player) player->MakeArmyMemberInfo(&info);
    response.mutable_info()->CopyFrom(info);
    army->AddMemeber(info, this);
    server->UpdatePlayerArmyStatus(apply->player_id(), army->army_id(), 0);
    if (player) {
      player->CalcTacticAttr(kNotifyAll, 0, 0);
      player->UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_JOIN_ARMY, 1);
    }
    this->DeleteApply(apply->player_id());
  }
  army->DeleteApply(message->player_id());
  server->DeleteArmyApply(apply->player_id(), army->army_id());

  this->SendMessageToClient(MSG_CS_RESPONSE_AGREE_ARMY_APPLY, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestKickArmyMember(CSMessageEntry& entry) {
  MessageRequestKickArmyMember* message =
      static_cast<MessageRequestKickArmyMember*>(entry.get());
  if (!message) return ERR_INTERNAL;
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;
  const ArmyMemberInfo* mine = army->GetArmyMemberInfo(this->uid());
  if (!mine) return ERR_INTERNAL;
  const ArmyMemberInfo* info = army->GetArmyMemberInfo(message->member_id());
  if (!info) return ERR_ARMY_MEMBER_NOT_FOUND;
  if (mine->position() <= info->position())
    return ERR_ARMY_PERMISSION;

  army->DeleteMemeber(message->member_id(), this);
  server->UpdatePlayerArmyStatus(message->member_id(), 0, 0);

  LogicPlayer* player = server->GetPlayerByID(message->member_id());
  if (player) player->CalcTacticAttr(kNotifyAll, 0, 0);

  server->OnPlayerJoinArmy(message->member_id(), NULL);

  MessageResponseKickArmyMember response;
  response.set_member_id(message->member_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_KICK_ARMY_MEMBER, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLeaveArmy(CSMessageEntry& entry) {
  Army* army = server->GetArmyByID(this->army_id());
  if (!army) return ERR_ARMY_NOT_JOINED;
  const ArmyMemberInfo* mine = army->GetArmyMemberInfo(this->uid());
  if (!mine) return ERR_INTERNAL;
  if (mine->position() == sy::ARMY_POSITION_MASTER)
    return ERR_ARMY_IS_MASTER;

  army->DeleteMemeber(this->uid(), this);
  server->UpdatePlayerArmyStatus(this->uid(), 0, GetVirtualSeconds());
  this->SendMessageToClient(MSG_CS_RESPONSE_LEAVE_ARMY, NULL);

  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_LEAGUE, MSG_CS_REQUEST_LEAVE_ARMY);

  server->OnPlayerJoinArmy(this->uid(), NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestChangeArmyAnnouncement(
    CSMessageEntry& entry) {
  MessageRequestChangeArmyAnnouncement* message =
      static_cast<MessageRequestChangeArmyAnnouncement*>(entry.get());
  if (!message) return ERR_INTERNAL;

  Army* army = server->GetArmyByID(this->army_id());
  if (!army) return ERR_ARMY_NOT_JOINED;
  const ArmyMemberInfo* info = army->GetArmyMemberInfo(this->uid());
  if (!info) return ERR_ARMY_MEMBER_NOT_FOUND;
  if (info->position() == sy::ARMY_POSITION_MEMBER)
    return ERR_ARMY_PERMISSION;

  if (message->announcement1().length() >= 256 ||
      message->announcement2().length() >= 256)
    return ERR_ARMY_ANNOUNCEMENT_LEN;
  if (message->has_announcement1()) {
    army->info().set_announcement1(message->announcement1());
  }
  if (message->has_announcement2()) {
    army->info().set_announcement2(message->announcement2());
  }
  if (message->has_avatar()) {
    army->info().set_avatar(message->avatar());
  }

  MessageSSUpdateArmyNotice update;
  update.set_notice1(army->info().announcement1());
  update.set_notice2(army->info().announcement2());
  update.set_avatar(army->info().avatar());
  update.set_army_id(this->army_id());
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_NOTICE, &update);

  MessageResponseChangeArmyAnnouncement response;
  response.set_announcement1(army->info().announcement1());
  response.set_announcement2(army->info().announcement2());
  response.set_avatar(army->info().avatar());
  this->SendMessageToClient(MSG_CS_RESPONSE_CHANGE_ARMY_ANNOUNCEMENT, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestArmyApply(CSMessageEntry& entry) {
  MessageRequestArmyApply* message =
      static_cast<MessageRequestArmyApply*>(entry.get());
  if (!message) return ERR_INTERNAL;
  Army* army = server->GetArmyByID(message->army_id());
  if (!army) return ERR_ARMY_NOT_FOUND;

  if (this->army_id()) return ERR_ARMY_JOINED;

  MessageSSUpdateArmyApply update;

  if (message->is_cancel()) {
    army->DeleteApply(this->uid());
    server->DeleteArmyApply(this->uid(), army->army_id());
    update.set_army_id(army->army_id());
    update.set_player_id(this->uid());
  } else {
    if ((int32_t)server->applies(this->uid()).size() >
        GetSettingValue(league_applicant_restriction1))
      return ERR_ARMY_APPLY_MAX;
    if ((GetVirtualSeconds() - this->army_leave_time_) <
        GetSettingValue(league_leave_time))
      return ERR_ARMY_LEAVE_CD;
    if ((int32_t)army->members().size() >= army->GetArmyMaxNum())
      return ERR_ARMY_MEMBER_COUNT;

    for (std::vector<sy::ArmyApplyInfo>::iterator iter =
             army->applies().begin();
         iter != army->applies().end(); ++iter) {
      if (iter->player_id() == this->uid()) {
        army->applies().erase(iter);
        break;
      }
    }

    sy::ArmyApplyInfo apply;
    apply.set_name(this->name());
    apply.set_fight(this->fight_attr());
    apply.set_level(this->level());
    apply.set_avatar(this->avatar());
    apply.set_army_id(army->army_id());
    apply.set_player_id(this->uid());
    apply.set_vip_level(this->vip_level());
    army->applies().push_back(apply);
    server->AddArmyApply(this->uid(), army->army_id());
    update.set_server(server_config->server_id());
    update.mutable_info()->CopyFrom(apply);
  }

  MessageResponseArmyApply response;
  response.set_army_id(army->army_id());
  response.set_is_cancel(message->is_cancel());
  this->SendMessageToClient(MSG_CS_RESPONSE_ARMY_APPLY, &response);
  server->SendServerMessageToDB(MSG_SS_UPDATE_ARMY_APPLY, &update);
  return ERR_OK;
}

int32_t LogicPlayer::FillCachedTactic(CachedTactic& tactic) {
  tactic.resize(0);
  tactic.resize(8, 0);
  tactic[0] = this->uid();
  tactic[1] = this->current_carrier_.carrier_id();
  int32_t count = 0;
  for (int32_t i = 0; i < this->tactic_.battle_pos_size(); ++i) {
    int32_t pos = this->tactic_.battle_pos(i).position();
    LogicHero* hero = this->GetHeroByUID(this->tactic_.battle_pos(i).hero_uid());
    if (!hero || !hero->second) continue;
    tactic[pos + 1] = hero->second->id();
    count++;
  }
  INFO_LOG(logger)("CachedTactic, %d,%d,%d,%d,%d,%d,%d,%d"
      , tactic[0], tactic[1], tactic[2], tactic[3], tactic[4]
      , tactic[5], tactic[6], tactic[7]);
  return count;
}

int32_t LogicPlayer::ProcessRequestCarrierLevelUp(CSMessageEntry& entry) {
  MessageRequestCarrierLevelUp* message =
      static_cast<MessageRequestCarrierLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  CarrierInfo* info = this->GetCarrierByID(message->carrier_id());
  if (!info) return ERR_CARRIER_NOT_FOUND;
  CarrierBase* base = CARRIER_BASE.GetEntryByID(info->carrier_id()).get();
  if (!base) return ERR_CARRIER_NOT_FOUND;

  int32_t max_exp = 0;
  for (int32_t i = info->level(); i < this->level(); ++i) {
    max_exp += base->GetLevelUpExp(i + 1);
  }
  max_exp -= (info->exp() + 1);
  if (max_exp <= 0) return ERR_INTERNAL;

  AddSubItemSet sub_set;
  int32_t exp = 0;
  for (int32_t i = 0; i < message->item_list_size(); ++i) {
    int64_t item_uid = message->item_list(i).key();
    int32_t item_count = message->item_list(i).value();
    LogicItem* item = this->items().GetItemByUniqueID(item_uid);
    if (!item) return ERR_ITEM_NOT_FOUND;
    sub_set.push_back(ItemParam(item->item_id(), -item_count));
    if (item_count < 0) return ERR_PARAM_INVALID;
    exp += Setting::GetValueInVec2(Setting::carrierlvup_exp, item->item_id()) *
           item_count;
  }

  if (exp <= 0) return ERR_INTERNAL;
  if (exp > max_exp) exp = max_exp;

  ModifyCurrency modify(MSG_CS_REQUEST_CARRIER_LEVEL_UP,
                        SYSTEM_ID_CARRIER_LEVEL_UP);
  modify[MONEY_KIND_COIN] = -exp;
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&sub_set, NULL, NULL));

  UpdateCurrency(modify);
  this->ObtainItem(&sub_set, NULL, NULL, MSG_CS_REQUEST_CARRIER_LEVEL_UP,
                   SYSTEM_ID_CARRIER_LEVEL_UP);
  this->AddCarrierExp(info, exp);

  MessageResponseCarrierLevelUp response;
  response.set_carrier_id(info->carrier_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_CARRIER_LEVEL_UP, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestCarrierReformUp(CSMessageEntry& entry) {
  MessageRequestCarrierReformUp* message =
      static_cast<MessageRequestCarrierReformUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  CarrierInfo* info = this->GetCarrierByID(message->carrier_id());
  if (!info) return ERR_CARRIER_NOT_FOUND;
  CarrierBase* base = CARRIER_BASE.GetEntryByID(info->carrier_id()).get();
  if (!base) return ERR_CARRIER_NOT_FOUND;

  const BreakAdvancedBase* grade_base =
      BREAK_ADVANCED.GetEntryByID(BreakAdvancedID(base->breakadvancedid,
                                                  info->reform_level()))
          .get();
  if (!grade_base || grade_base->coin_cost <= 0) return ERR_ITEM_LEVEL_MAX;
  if (info->level() + 1 < grade_base->need_hero_lv)
    return ERR_HERO_NEED_LEVEL_UP;

  AddSubItemSet sub_item;
  sub_item.push_back(ItemParam(GetSettingValue(carrier_advanceditem_id),
                               -grade_base->item_cost));
  sub_item.push_back(ItemParam(base->item[0].v1, -grade_base->hero_cost));

  ModifyCurrency modify(MSG_CS_REQUEST_CARRIER_REFORM_UP,
                        SYSTEM_ID_CARRIER_REFORM);
  modify.coin = -grade_base->coin_cost;

  int32_t result = this->CheckItem(&sub_item, NULL, NULL);
  if (result) return result;
  result = this->CheckCurrency(modify);
  if (result) return result;

  info->set_reform_level(info->reform_level() + 1);
  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_CARRIER_REFORM, entry.head.msgid);
  this->ObtainItem(&sub_item, NULL, NULL, MSG_CS_REQUEST_CARRIER_REFORM_UP,
                   SYSTEM_ID_CARRIER_REFORM);
  this->UpdateCurrency(modify);

  MessageSSUpdateCarrierInfo up_message;
  up_message.mutable_info()->CopyFrom(*info);
  up_message.set_tid(server->GetTID());
  this->SendMessageToDB(MSG_SS_UPDATE_CARRIER_INFO, &up_message);

  MessageNotifyCarrierInfo msg;
  msg.mutable_info()->CopyFrom(*info);
  this->SendMessageToClient(MSG_CS_NOTIFY_CARRIER_INFO, &msg);

  MessageResponseCarrierReformUp response;
  response.set_carrier_id(info->carrier_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_CARRIER_REFORM_UP, &response);
  return ERR_OK;
}

LogicPlayer* LogicPlayer::GetCarrierPlayer(int32_t id) {
  if (id < 1 || id > 12) return NULL;

  if (!this->carrier_player_.full()) {
    this->carrier_player_.resize(12);
  }
  const sy::CarrierCopy& copy = this->carrier_copy_[id - 1];
  double percent = copy.fight_attr() * 1.0 / this->max_fight_attr();

  if (!this->carrier_player_[id - 1]) {
    this->carrier_player_[id - 1] = boost::make_shared<LogicPlayer>(id);
    //初始化
    LogicPlayer* p = this->carrier_player_[id - 1].get();
    p->InitCarrierPlayerSimpleInfo(copy.level());
    p->InitCarrierPlayerCarrier(this, copy.carrier_id(), percent);
    for (int32_t i = 1; i <= 6; ++i) {
      p->InitCarrierPlayerHero(this, copy.heros(i - 1), i, percent);
    }
  }
  return this->carrier_player_[id - 1].get();
}

void LogicPlayer::InitCarrierPlayerSimpleInfo(int32_t level) {
  this->player_.set_level(level);
}

extern boost::mt19937 globalMt19937;
boost::uniform_real<> kCarrierAttrDis(-0.05, 0.05);

void LogicPlayer::InitCarrierPlayerCarrier(LogicPlayer* from,
                                           int32_t carrier_id, double percent) {
  this->current_carrier_.mutable_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  this->current_carrier_.mutable_tower_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  this->current_carrier_.mutable_extra_damage1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  if (!carrier_id) return;

  this->current_carrier_.CopyFrom(from->current_carrier_);
  this->current_carrier_.set_carrier_id(carrier_id);
  this->current_carrier_.mutable_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  this->current_carrier_.mutable_tower_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  this->current_carrier_.mutable_extra_damage1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
  for (int32_t index = ATTACK_ATTR_HP; index <= ATTACK_ATTR_FF; ++index) {
    int32_t attr = this->current_carrier_.attr1(index) *
                   (kCarrierAttrDis(globalMt19937) + percent);
    this->current_carrier_.set_attr1(index, attr);
  }
}

void LogicPlayer::InitCarrierPlayerHero(LogicPlayer* from, int32_t hero_id,
                                        int32_t pos, double percent) {
  if (!hero_id) return;
  LogicHero* from_hero = from->GetHeroByPos(pos);
  while (!from_hero) {
    pos = (pos + 1) % 6;
    pos = !pos ? 6 : pos;
    from_hero = from->GetHeroByPos(pos);
  }

  const HeroBasePtr& base = HERO_BASE.GetEntryByID(hero_id);
  if (!base) {
    ERROR_LOG(logger)("InitCarrierPlayerHero, HeroID:%d config not found", hero_id);
    return;
  }

  sy::HeroInfo hero;
  hero.CopyFrom(from_hero->first);
  hero.set_hero_id(hero_id);
  hero.set_uid(hero_id);

  //矫正船的属性
  hero.mutable_attr1()->Set(0, hero.attr1(0) * percent);
  //四种基础属性会有5%的振幅
  for (int32_t i = sy::ATTACK_ATTR_HP; i <= sy::ATTACK_ATTR_FF; ++i) {
    hero.mutable_attr1()->Set(
        i, hero.attr1(i) * (percent + kCarrierAttrDis(globalMt19937)));
  }

  this->ships_.push_back(std::make_pair(hero, base));
  sy::PositionInfo* pos_info = this->tactic_.add_battle_pos();
  pos_info->set_position(pos);
  pos_info->set_hero_uid(hero_id);
}

//第四层有一个通关,就算通关了
int32_t LogicPlayer::ProcessRequestCarrierCopyNextLevel(CSMessageEntry& entry) {
  int32_t current_layer = this->carrier_copy_info_.layer();
  int32_t count = this->carrier_copy_info_.count();
  INFO_LOG(logger)("PlayerID:%ld, CurrentLayer:%d, UseCount:%d", this->uid(), current_layer, count);

  //通关之后就只能重置
  if (current_layer >= 4) return ERR_PARAM_INVALID;
  //第四层没有通关
  if (current_layer >= 1 && current_layer <= 3 &&
      !(this->carrier_copy_info_.passed_copy() & kCarrierCopyLevel4)) {
    return ERR_CARRIER_COPY_NOT_PASSED;
  }

  this->carrier_player_.clear();
  server->RandomTactic(this, current_layer + 1, this->carrier_copy_);

  this->carrier_copy_info_.clear_current_index();
  this->carrier_copy_info_.clear_left_hp();
  this->carrier_copy_info_.clear_award_count();
  this->carrier_copy_info_.clear_passed_copy();
  this->carrier_copy_info_.set_layer(current_layer + 1);

  this->UpdateCarrierCopy(true, true);
  this->SendMessageToClient(MSG_CS_RESPONSE_CARRIER_COPY_NEXT_LEVEL, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetCarrierCopyAward(CSMessageEntry& entry) {
  if (!(this->carrier_copy_info_.passed_copy() & kCarrierCopyLevel4))
    return ERR_CARRIER_COPY_NOT_PASSED;

  int32_t award_id = this->carrier_copy_info_.layer() * 10 + this->carrier_copy_info_.award_count() + 1;
  const CarrierCopyAwardBase* base = CARRIER_COPY_AWARD_BASE.GetEntryByID(award_id).get();
  if (!base) return ERR_PARAM_INVALID;

  //扣钱
  const BuyCountCostBase* buy_base = BUY_COUNT_BASE.GetEntryByID(COUNT_TYPE_CARRIER_COPY_AWARD).get();
  if (!buy_base) return ERR_PARAM_INVALID;
  int32_t max_count = buy_base->GetBuyCount(this->vip_level());
  if (this->carrier_copy_info_.award_count() >= max_count) return ERR_CURRENCY_VIP_EXP;

  MessageResponseGetCarrierCopyAward response;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_CARRIER_COPY);
  AddSubItemSet item_set;
  for (std::vector<int32_t>::const_iterator iter = base->reward.begin();
       iter != base->reward.end(); ++iter) {
    const LootBasePtr& ptr = LootConfigFile::Get(*iter, this->level());
    if (ptr) {
      ptr->Loot(modify, item_set, NULL);
    }
  }

  for (int32_t i = MONEY_KIND_COIN; i <= MONEY_KIND_UNION; ++i) {
    if (modify[i]) {
      sy::KVPair2* pair = response.add_awards();
      pair->set_key(i);
      pair->set_value(modify[i]);
    }
  }
  for (AddSubItemSet::const_iterator iter = item_set.begin();
       iter != item_set.end(); ++iter) {
    sy::KVPair2* pair = response.add_awards();
    pair->set_key(iter->item_id);
    pair->set_value(iter->item_count);
  }

  modify[buy_base->type] += -buy_base->GetBuyCost(this->carrier_copy_info_.award_count() + 1);
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_CARRIER_COPY);

  this->carrier_copy_info_.set_award_count(this->carrier_copy_info_.award_count() + 1);
  this->UpdateCarrierCopy(false, true);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_CARRIER_COPY_AWARD, &response);
  return ERR_OK;
}

void LogicPlayer::UpdateCarrierCopy(bool copys, bool info) {
  MessageNotifyCarrierCopy notify;
  MessageSSUpdateCarrierCopyInfo msg;

  if (info) {
    notify.mutable_carrier_copy_info()->CopyFrom(this->carrier_copy_info_);
    msg.mutable_carrier_copy_info()->CopyFrom(this->carrier_copy_info_);
  }

  if (copys) {
    for (size_t i = 0; i < this->carrier_copy_.size(); ++i) {
      notify.add_carrier_copy()->CopyFrom(this->carrier_copy_[i]);
      msg.add_carrier_copy()->CopyFrom(this->carrier_copy_[i]);
    }
  }
  this->SendMessageToClient(MSG_CS_NOTIFY_CARRIER_COPY, &notify);
  this->SendMessageToDB(MSG_SS_UPDATE_CARRIER_COPY_INFO, &msg);
}

int32_t LogicPlayer::CarrierCopyPK(PK& pk) {
  int32_t map_index = pk.copy->id() % 100;
  pk.InitPlayer(this, true);
  LogicPlayer* player = this->GetCarrierPlayer(map_index);
  if (!player) {
    ERROR_LOG(logger)("CarrierCopyPK, PlayerID:%ld, MapID:%ld init fail", this->uid(), pk.copy->id());
    return 0;
  }

  this->carrier_copy_info_.mutable_left_hp()->Resize(6, 0);
  INFO_LOG(logger)("BeforeCarrierCopy PlayerID:%ld, Layer:%d, Index:%d, HP:%d,%d,%d,%d,%d,%d", this->uid()
        , this->carrier_copy_info_.layer(), this->carrier_copy_info_.current_index()
        , this->carrier_copy_info_.left_hp(0), this->carrier_copy_info_.left_hp(1)
        , this->carrier_copy_info_.left_hp(2), this->carrier_copy_info_.left_hp(3)
        , this->carrier_copy_info_.left_hp(4), this->carrier_copy_info_.left_hp(5)
        );
  pk.InitPlayer(player, false,
                this->carrier_copy_info_.current_index() == map_index
                    ? &*this->carrier_copy_info_.left_hp().begin()
                    : NULL);
  pk.GeneratePVPReport();

  this->carrier_copy_info_.mutable_left_hp()->Clear();
  this->carrier_copy_info_.set_current_index(0);
  if (!pk.star) {
    //目标和血量状况
    this->carrier_copy_info_.set_current_index(map_index);
    this->carrier_copy_info_.mutable_left_hp()->Resize(6, 0);
    for (int32_t pos = 1; pos <= 6; ++pos) {
      PkHero* hero = pk.b.GetHeroByPos(pos);
      if (hero) {
        int32_t percent = hero->hp() * 1.0 / hero->max_hp() * 10000;
        this->carrier_copy_info_.set_left_hp(pos - 1, percent <= 0 ? 0 : percent);
      }
    }
    INFO_LOG(logger)("AfterCarrierCopy PlayerID:%ld, Layer:%d, Index:%d, HP:%d,%d,%d,%d,%d,%d", this->uid()
        , this->carrier_copy_info_.layer(), this->carrier_copy_info_.current_index()
        , this->carrier_copy_info_.left_hp(0), this->carrier_copy_info_.left_hp(1)
        , this->carrier_copy_info_.left_hp(2), this->carrier_copy_info_.left_hp(3)
        , this->carrier_copy_info_.left_hp(4), this->carrier_copy_info_.left_hp(5)
        );
  }
  return pk.star;
}

//     Layer:N
//----+------+------+
// 10 |  11  |  12  |
//----+------+------+
// 07 |  08  |  09  |
//----+------+------+
// 04 |  05  |  06  |
//----+------+------+
// 01 |  02  |  03  |
//----+------+------+
const int32_t kCarrierCopyMask[] = {
  0, 0, 0, 0,
  /*04*/1 << 1 | 1 << 5 | 1 << 9,
  /*05*/1 << 2 | 1 << 4 | 1 << 6 | 1 << 8,
  /*06*/1 << 3 | 1 << 5 | 1 << 9,
  /*07*/1 << 4 | 1 << 8 | 1 << 10,
  /*08*/1 << 5 | 1 << 7 | 1 << 11 | 1 << 9,
  /*09*/1 << 6 | 1 << 8 | 1 << 12,
  /*10*/1 << 7 | 1 << 11,
  /*11*/1 << 10 | 1 << 12 | 1 << 8,
  /*12*/1 << 11 | 1 << 9
};

int32_t LogicPlayer::ProcessRequestFightCarrierCopy(CSMessageEntry& entry) {
  MessageRequestFightCarrierCopy *message = static_cast<MessageRequestFightCarrierCopy*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (this->carrier_copy_info_.layer() < 0 ||
      this->carrier_copy_info_.layer() > 4)
    return ERR_PARAM_INVALID;
  if (message->index() < 1 || message->index() > 12) return ERR_PARAM_INVALID;
  if (this->carrier_copy_info_.count() >= 16)
    return ERR_COPY_COUNT;
  //是否可以打判断
  int32_t mask = kCarrierCopyMask[message->index()];
  if (mask && !(this->carrier_copy_info_.passed_copy() & mask)) {
    return ERR_COPY_INVALID;
  }
  //副本已经打过了
  if ((1 << message->index()) & this->carrier_copy_info_.passed_copy())
    return ERR_COPY_PASSED;

  const CarrierCopyBase* base =
      CARRIER_COPY_BASE.GetEntryByID(this->carrier_copy_info_.layer() * 10 +
                                     (message->index() - 1) / 3 + 1).get();
  if (!base) return ERR_INTERNAL;
  if (message->index() > (int32_t)this->carrier_copy_.size()) return ERR_INTERNAL;

  ModifyCurrency money(entry.head.msgid, SYSTEM_ID_CARRIER_COPY);
  AddSubItemSet item_set;
  const sy::CarrierCopy& copy = this->carrier_copy_[message->index() - 1];

  const CopyBase* copy_base = COPY_BASE.GetEntryByID(62800 + message->index()).get();
  if (!copy_base) return ERR_COPY_INVALID;

  //战斗流程
  //胜利才有奖励
  PK pk(copy_base);
  if (this->CarrierCopyPK(pk)) {
    int64_t add_item = (int64_t)base->reward_a * copy.fight_attr() / this->max_fight_attr() + base->reward_b;
    add_item = this->level() * add_item / 100;
    if (IsHeroComeBack()) add_item *= 1.5f;
    item_set.push_back(ItemParam(23900010, add_item));
    const LootBasePtr& loot_ptr = LootConfigFile::Get(base->reward_fixed, this->level());
    if (loot_ptr) {
      loot_ptr->Loot(money, item_set, NULL);
    }
    CHECK_RET(this->CheckCurrency(money));
    CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
    //奖励状况
    this->UpdateCurrency(money);
    this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_CARRIER_COPY);
    //通关状况
    this->carrier_copy_info_.set_passed_copy(this->carrier_copy_info_.passed_copy() | (1 << message->index()));
    //通过获取合金数
    this->carrier_copy_info_.set_item_count(this->carrier_copy_info_.item_count() + add_item);
    if (this->carrier_copy_info_.item_max_count() <
        this->carrier_copy_info_.item_count()) {
      this->carrier_copy_info_.set_item_max_count(this->carrier_copy_info_.item_count());
      RANK_LIST.OnCarrierCopy(this);
    }
  }

  MessageNotifyFightResult notify;
  notify.set_win(pk.star);
  notify.set_star(pk.star);
  FillFightResultMessage(notify, item_set, money);
  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);

  this->carrier_copy_info_.set_count(this->carrier_copy_info_.count() + 1);
  this->UpdateCarrierCopy(false, true);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
  server->UpdateCopyStatistics(copy_base->type, copy_base->id(), this->uid());
  return ERR_OK;
}

void LogicPlayer::AddCarrierExp(CarrierInfo* carrier, int32_t exp) {
  if (!carrier) return;
  if (carrier->level() < 0) carrier->set_level(0);

  CarrierBase* base = CARRIER_BASE.GetEntryByID(carrier->carrier_id()).get();
  if(!base) return;

  int64_t current_exp = carrier->exp() + exp;
  int32_t current_level = carrier->level();
  int32_t level_changed = 0;

  while (true) {
    int32_t need_exp = base->GetLevelUpExp(current_level + 1);
    if (need_exp <= 0 || current_exp < need_exp) {
      break;
    }
    current_exp -= need_exp;
    current_level += 1;
    level_changed += 1;
  }

  carrier->set_level(current_level);
  carrier->set_exp(current_exp);

  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_CARRIER_BUILD,
                       MSG_CS_REQUEST_CARRIER_LEVEL_UP);

  MessageSSUpdateCarrierInfo message;
  message.mutable_info()->CopyFrom(*carrier);
  message.set_tid(server->GetTID());
  this->SendMessageToDB(MSG_SS_UPDATE_CARRIER_INFO, &message);

  MessageNotifyCarrierInfo msg;
  msg.mutable_info()->CopyFrom(*carrier);
  this->SendMessageToClient(MSG_CS_NOTIFY_CARRIER_INFO, &msg);
}

void LogicPlayer::OnRecharge(int32_t goodid, int32_t money) {
  TRACE_LOG(logger)("PlayerID:%ld, RechargeID:%d, Money:%d", this->uid(), goodid, money);
  if (goodid >= 60 && goodid <= 62) OnGetMonthCard(goodid);
  if (goodid >= 57 && goodid <= 59) OnWeeklyCard(goodid);
}

void LogicPlayer::OnGetMonthCard(int32_t id) {
  time_t ZeroClock = GetZeroClock(GetVirtualSeconds());
  int32_t add_month_card = this->achievements_[ACHIEVEMENT_TYPE_COUNT_MONTH_CARD];
  if (id == 60) {
    if (this->month_card_ < ZeroClock) this->month_card_ = ZeroClock;
    this->month_card_ += 30 * 60 * 60 * 24;
  }
  if (id == 61) {
    if (this->big_month_card_ < ZeroClock) this->big_month_card_ = ZeroClock;
    this->big_month_card_ += 30 * 60 * 60 * 24;
  }
  if (id == 62) {
    this->life_card_ = GetVirtualSeconds();
    add_month_card += 100;
  }
  this->UpdateAchievement(ACHIEVEMENT_TYPE_COUNT_MONTH_CARD, add_month_card);
  this->UpdateMonthCard(true);
}

void LogicPlayer::UpdateMonthCard(bool notify_client) {
  if (notify_client) {
    MessageNotifyMonthCard notify;
    notify.set_month_card(this->month_card_);
    notify.set_big_month_card(this->big_month_card_);
    notify.set_life_card(this->life_card_);
    notify.set_weekly_card(this->weekly_card_[0]);
    notify.set_weekly_card_login(this->weekly_card_[1]);
    notify.set_weekly_card_status(this->weekly_card_[2]);
    notify.set_month_card_1(this->month_card_1_[0]);
    notify.set_month_card_1_login(this->month_card_1_[1]);
    notify.set_month_card_1_status(this->month_card_1_[2]);
    notify.set_month_card_2(this->month_card_2_[0]);
    notify.set_month_card_2_login(this->month_card_2_[1]);
    notify.set_month_card_2_status(this->month_card_2_[2]);
    this->SendMessageToClient(MSG_CS_NOTIFY_MONTH_CARD, &notify);
  }

  MessageSSUpdateMonthCard update;
  update.set_month_card(this->month_card_);
  update.set_big_month_card(this->big_month_card_);
  update.set_life_card(this->life_card_);
  update.set_weekly_card(this->weekly_card_[0]);
  update.set_weekly_card_login(this->weekly_card_[1]);
  update.set_weekly_card_status(this->weekly_card_[2]);
  update.set_month_card_1(this->month_card_1_[0]);
  update.set_month_card_1_login(this->month_card_1_[1]);
  update.set_month_card_1_status(this->month_card_1_[2]);
  update.set_month_card_2(this->month_card_2_[0]);
  update.set_month_card_2_login(this->month_card_2_[1]);
  update.set_month_card_2_status(this->month_card_2_[2]);
  this->SendMessageToDB(MSG_SS_UPDATE_MONTH_CARD, &update);
}

int32_t LogicPlayer::OnRecharge1RMB(int32_t time, const std::string& goodid,
                                    int32_t money, int32_t msgid) {

  int32_t temp = this->achievements_[OTHER_FIRST_RECHARGE];
  if (!(temp & (1 << 3))) {
    ModifyCurrency modify(msgid, SYSTEM_ID_RECHARGE);
    AddSubItemSet item_set;
    FillCurrencyAndItem<kAdd>(
        modify, item_set, Setting::GetValue2(Setting::oneRMB_charge_reward));
    this->UpdateCurrency(modify);
    this->ObtainItem(&item_set, NULL, NULL, msgid, SYSTEM_ID_RECHARGE);
    temp |= (1 << 3);
    UpdateAchievement(OTHER_FIRST_RECHARGE, temp);
  }
  return ERR_OK;
}

void LogicPlayer::AddRechargeInfo(int32_t time, int32_t money, int32_t goodid) {
  sy::RechargeInfo info;
  info.set_recharge_time(time);
  info.set_money(money);
  info.set_goodid(goodid);
  this->recharge_.push_back(info);

  MessageSSUpdateRecharge update;
  update.set_recharge_time(time);
  update.set_money(money);
  update.set_goodid(goodid);
  this->SendMessageToDB(MSG_SS_UPDATE_RECHARGE, &update);

  MessageNotifyRecharge notify;
  notify.mutable_info()->CopyFrom(info);
  notify.set_total_recharge(this->player_.total_recharge());
  this->SendMessageToClient(MSG_CS_NOTIFY_RECHARGE, &notify);
}

//周卡类型0, 小月基金类型1, 大月基金类型2
int32_t LogicPlayer::OnWeeklyCard(int32_t goodid) {
  if (!this->life_card_) {
    ERROR_LOG(logger)("OnWeeklyCard, PlayerID:%ld not buy life_card", this->uid());
    return ERR_OK;
  }
  if (GetSeconds() >= this->month_card_ &&
      GetSeconds() >= this->big_month_card_) {
    ERROR_LOG(logger)("OnWeeklyCard, PlayerID:%ld MonthCard is not valid", this->uid());
    return ERR_OK;
  }
  //处理周卡和两种月卡
  int32_t type = -1;
  if (goodid == 57) type = 0;
  if (goodid == 58) type = 1;
  if (goodid == 59) type = 2;
  int32_t valid_days = 0;
  int32_t buy_count = 0;
  int32_t* card = this->GetWeeklyCardByType(type, valid_days, buy_count);
  if (!card) {
    ERROR_LOG(logger)("OnWeeklyCard PlayerID:%ld, GoodID:%d"
        , this->uid(), goodid);
    return ERR_OK;
  }
  int32_t diff_days = GetSecondsDiffDays(card[0], GetSeconds()) + 1;
  if (diff_days <= valid_days) {
    ERROR_LOG(logger)("OnWeeklyCard PlayerID:%ld, CardType:%d, Time:%d, LoginDays:%d, Status:%d"
        , this->uid(), type, card[0], card[1], card[2]);
  }
  card[0] = GetSeconds();
  card[1] = 1;
  card[2] = 0;
  TRACE_LOG(logger)("OnWeeklyCard PlayerID:%ld, CardType:%d, Time:%d, LoginDays:%d, Status:%d"
      , this->uid(), type, card[0], card[1], card[2]);
  this->UpdateMonthCard(true);
  switch (type) {
    case 0: {
      this->UpdateAchievement(sy::ACHIEVEMENT_TYPE_WEEKLY_CARD_COUNT, buy_count + 1);
    } break;
    case 1: {
      this->UpdateAchievement(sy::ACHIEVEMENT_TYPE_MONTH_CARD_128_COUNT, buy_count + 1);
    } break;
    case 2: {
      this->UpdateAchievement(sy::ACHIEVEMENT_TYPE_MONTH_CARD_328_COUNT, buy_count + 1);
    } break;
  }
  return ERR_OK;
}

int32_t* LogicPlayer::GetWeeklyCardByType(int32_t type, int32_t& valid_days,
                                          int32_t& buy_count) {
  valid_days = 0;
  switch (type) {
    case 0:
      valid_days = 7;
      buy_count = this->achievements_[sy::ACHIEVEMENT_TYPE_WEEKLY_CARD_COUNT];
      return &this->weekly_card_[0];
    case 1:
      valid_days = 30;
      buy_count = this->achievements_[sy::ACHIEVEMENT_TYPE_MONTH_CARD_128_COUNT];
      return &this->month_card_1_[0];
    case 2:
      valid_days = 30;
      buy_count = this->achievements_[sy::ACHIEVEMENT_TYPE_MONTH_CARD_328_COUNT];
      return &this->month_card_2_[0];
  }
  return NULL;
}

void LogicPlayer::DayRebate(sy::TimeActivityType type) {
  TimeActivityNew* activity_rebate = ACTIVITY.GetAs(type);
  if (activity_rebate) {
    int32_t days =
        GetSecondsDiffDays(activity_rebate->begin_time(), GetVirtualSeconds());
    if (days <= 30) {
      int32_t record =
          this->GetActivityRecordNew(type, activity_rebate->id(), -1);
      if (!(record & (1 << days))) {
        record |= (1 << days);
        this->SetActivityRecordNew(type, activity_rebate->id(), -1, record);
        ACTIVITY.AddActivityRecordCount(type, 1, this);
      }
    }
  }
}

int32_t LogicPlayer::AddRecharge(int32_t time, std::string& goodid,
                                 int32_t money, bool add_money, int32_t msgid) {
  const RechargeBasePtr& recharge_base = GetRechargeByGoodsID(goodid);
  TRACE_LOG(logger)("BeginRecharge PlayerID:%ld, Time:%d, GoodID:%s, Money:%d, AddMoney:%d, MSGID:0x%04X"
      , this->uid(), time, goodid.c_str(), money, add_money, msgid);
  if (recharge_base) {
    if (money != recharge_base->price * 10) {
      goodid = "test";
      money = 0;
    }
  }

  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_CUMLATIVE_RECHARGE, money * 10,
                                  this);
  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_RECHARGE, money * 10,
                                  this);

  if (money >= 60) DayRebate(TIME_ACTIVITY_DAY_REBATE6);
  if (money >= 300) DayRebate(TIME_ACTIVITY_DAY_REBATE30);

  //首冲状态
  {
    int32_t temp = this->achievements_[OTHER_FIRST_RECHARGE];
    if (!(temp & (1 << 4)) && money >= 60) {
      temp |= 1 << 4;
      UpdateAchievement(OTHER_FIRST_RECHARGE, temp);
    }
  }

  //二次充值
  if (this->player_.total_recharge() > 0 &&
      money >= GetSettingValue(second_charge_condition)) {
    int32_t temp = this->achievements_[OTHER_FIRST_RECHARGE];
    if ((!(temp & (1 << 2))) && (!(temp & (1 << 1)))) {
      temp |= 1 << 1;
      UpdateAchievement(OTHER_FIRST_RECHARGE, temp);
    }
  }

  if (goodid.find("item") == 0) {
    return this->OnRechargeItem(time, goodid, money, msgid);
  }

  ModifyCurrency modify(msgid, SYSTEM_ID_RECHARGE);
  if (add_money) {
    modify[sy::MONEY_KIND_MONEY] += money;
    modify[sy::MONEY_KIND_VIPEXP] += money;
  }
  int32_t month_card = 0;

  if (recharge_base) {
    month_card = recharge_base->index;
  }
  //各种额度首次充值
  if (server_config->recharge_platform() == 1){
      //各种额度首次充值
      if (recharge_base) {
        int32_t good_index = recharge_base->index;
        int32_t low = this->achievements_[sy::ACHIEVEMENT_TYPE_RECHARGE_1];
        int32_t high = this->achievements_[sy::ACHIEVEMENT_TYPE_RECHARGE_2];
        uint64_t bitset = uint64_t((uint32_t)high) << 32 | (uint32_t)low;
        if (!(bitset & (1 << good_index))) {
          modify[sy::MONEY_KIND_MONEY] += recharge_base->extra_gold;
          bitset |= 1 << good_index;
          if (good_index >= 32) {
            high = bitset >> 32;
            this->UpdateAchievement(sy::ACHIEVEMENT_TYPE_RECHARGE_2,
                                    bitset >> 32);
          } else {
            low = bitset << 32 >> 32;
            this->UpdateAchievement(sy::ACHIEVEMENT_TYPE_RECHARGE_1,
                                    bitset << 32 >> 32);
          }
        }
      }
  } else {
    if (!this->achievements_[sy::ACHIEVEMENT_TYPE_DOUBLE]) {
      modify[sy::MONEY_KIND_MONEY] += money;
      this->UpdateAchievement(sy::ACHIEVEMENT_TYPE_DOUBLE, GetVirtualSeconds());
    }
  }

  CHECK_RET(this->CheckCurrency(modify));
  this->UpdateCurrency(modify);
  this->player_.set_total_recharge(this->player_.total_recharge() + money * 10);
  MessageSSUpdateTotalRecharge ttup;
  ttup.set_total_recharge(player().total_recharge());
  this->SendMessageToDB(MSG_SS_UPDATE_TOTAL_RECHARGE, &ttup);

  this->OnRecharge(month_card, money);
  this->AddRechargeInfo(time, money * 10, month_card);
  TRACE_LOG(logger)("LogicServer Recharge Success, Player:%ld, Money:%d"
      , this->uid(), money);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestServerOpenFund(CSMessageEntry& entry) {
  MessageRequestGetOpenFund* message =
      static_cast<MessageRequestGetOpenFund*>(entry.get());
  if (!message) return ERR_INTERNAL;

  FundBase* base = FUND_BASE.GetEntryByID(message->fund_id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (life_shop_info_.find(40200) == life_shop_info_.end())
    return ERR_PARAM_INVALID;

  int32_t ache = this->achievements_[OTHER_SERVER_OPEN_FUND];
  int32_t flag = 1 << (message->fund_id() - 1);
  if (ache & flag) return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_OPEN_FUND, SYSTEM_ID_BASE);
  modify[MONEY_KIND_MONEY] += base->return1;
  CHECK_RET(CheckCurrency(modify));
  UpdateCurrency(modify);

  ache |= flag;
  UpdateAchievement(OTHER_SERVER_OPEN_FUND, ache);

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_OPEN_FUND, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestServerOpenWelfare(CSMessageEntry& entry) {
  MessageRequestGetOpenWelfare* message =
      static_cast<MessageRequestGetOpenWelfare*>(entry.get());
  if (!message) return ERR_INTERNAL;

  WelfareBase* base = WELFARE_BASE.GetEntryByID(message->welfare_id()).get();
  if (!base) return ERR_PARAM_INVALID;

  bool valid = false;
  for (int32_t i = 0; i < server->server_shop().items_size(); i++) {
    const KVPair2& pair = server->server_shop().items(i);
    if (pair.key() == 40200 && pair.value() >= base->count) {
      valid = true;
      break;
    }
  }
  if (valid == false) return ERR_ITEM_NEED_MORE;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_OPEN_WELFARE, SYSTEM_ID_BASE);
  AddSubItemSet item;
  FillCurrencyAndItem<kAdd>(modify, item, base->award);
  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item, NULL, NULL, MSG_CS_REQUEST_GET_OPEN_WELFARE,
             SYSTEM_ID_BASE);

  int32_t ache = this->achievements_[OTHER_SERVER_WELFARE];
  int32_t flag = 1 << (message->welfare_id() - 1);
  if (ache & flag) return ERR_PARAM_INVALID;

  ache |= flag;
  UpdateAchievement(OTHER_SERVER_WELFARE, ache);

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_OPEN_WELFARE, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestArmySkillUp(CSMessageEntry& entry) {
  MessageRequestArmySkillUp* message =
      static_cast<MessageRequestArmySkillUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->skill_id() >= (int32_t)army_skill_.size()) return ERR_PARAM_INVALID;

  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;

  LeagueSkillBase* skill_base =
      LEAGUE_SKILL_BASE.GetEntryByID(message->skill_id()).get();
  if (!skill_base) return ERR_PARAM_INVALID;

  int32_t skill_level = army_skill_[message->skill_id()];
  if (skill_level >= army->skill_level(message->skill_id()))
    return ERR_PARAM_INVALID;

  LeagueSkillCostBase* cost_base =
      LEAGUE_SKILL_COST_BASE.GetEntryByID(skill_level).get();
  if (!cost_base) return ERR_INTERNAL;

  int32_t cost_count = 0;
  switch (skill_base->cost_type) {
    case 1:
      cost_count = cost_base->cost_type1;
      break;
    case 2:
      cost_count = cost_base->cost_type2;
      break;
    case 3:
      cost_count = cost_base->cost_type3;
      break;
    case 4:
      cost_count = cost_base->cost_type4;
      break;
  }
  if (!cost_count) return ERR_INTERNAL;

  ModifyCurrency modify(MSG_CS_REQUEST_ARMY_SKILL_UP, SYSTEM_ID_LEAGUE);
  modify[MONEY_KIND_UNION] = -cost_count;

  CHECK_RET(this->CheckCurrency(modify));
  this->UpdateCurrency(modify);

  skill_level = ++army_skill_[message->skill_id()];

  MessageResponseArmySkillUp msg;
  msg.set_skill_id(message->skill_id());
  msg.set_skill_level(skill_level);
  this->SendMessageToClient(MSG_CS_RESPONSE_ARMY_SKILL_UP, &msg);

  MessageSSUpdateLeguageInfo update;
  for (size_t i = 0; i < army_skill_.size(); i++)
    update.add_army_skill(army_skill_[i]);
  this->SendMessageToDB(MSG_SS_UPDATE_LEGUAGE_INFO, &update);

  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_ARMY_UP,
                       MSG_CS_REQUEST_ARMY_SKILL_UP);

  return ERR_OK;
}

void LogicPlayer::AddLeagueAttr(AttackAttrArray& attr) {
  if (!this->army_id_) return;
  for (size_t i = 0; i < army_skill_.size(); i++) {
    LeagueSkillBase* skill_base = LEAGUE_SKILL_BASE.GetEntryByID(i).get();
    if (!skill_base) continue;

    for (ValuePair2Vec::iterator it = skill_base->attr.begin();
         it != skill_base->attr.end(); ++it) {
      if (it->v1 && it->v2) {
        if (it->v1 < (int32_t)attr.size() && it->v1)
          attr[it->v1] += (it->v2 * army_skill_[i]);
      }
    }
  }
}

int32_t LogicPlayer::ProcessRequestArmySign(CSMessageEntry& entry) {
  MessageRequestArmySign* message =
      static_cast<MessageRequestArmySign*>(entry.get());
  if (!message) return ERR_INTERNAL;

  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;

  LeagueSignBase* sign_base =
      LEAGUE_SIGN_BASE.GetEntryByID(message->sign_id()).get();
  if (!sign_base) return ERR_PARAM_INVALID;

  if (this->vip_level() < sign_base->vip) return ERR_PARAM_INVALID;

  if (this->GetDailySign(ARMY_SIGN_DONE1) &&
      this->GetDailySign(ARMY_SIGN_DONE2) &&
      this->GetDailySign(ARMY_SIGN_DONE3))
    return ERR_PARAM_INVALID;

  bool is_crital = false;
  if (RandomBetween(0, 100) < GetSettingValue(league_sign_reward))
    is_crital = true;

  int32_t union_num = sign_base->league_currency;
  if(is_crital) union_num *= 1.5f;

  ModifyCurrency modify(MSG_CS_REQUEST_ARMY_SIGN, SYSTEM_ID_LEAGUE);
  modify[sign_base->cost.v1] = -sign_base->cost.v2;
  modify[MONEY_KIND_UNION] += union_num;
  CHECK_RET(CheckCurrency(modify));
  int32_t result = army->ArmySign(sign_base, this, is_crital);
  if (result) return result;

  sy::ArmyMemberInfo* member_info = army->GetMember(this->uid());
  if(!member_info) return ERR_ARMY_NOT_JOINED;
  member_info->set_army_exp(member_info->army_exp() + sign_base->league_exp);
  if (IsSameDay(member_info->update_time(), GetVirtualSeconds()))
    member_info->set_today_exp(member_info->today_exp() +
                               sign_base->league_exp);
  else
    member_info->set_today_exp(sign_base->league_exp);
  member_info->set_update_time(GetVirtualSeconds());
  army->UpdateMember(this->uid());

  UpdateCurrency(modify);

  if (sign_base->id() == 1) this->SetDailySign(ARMY_SIGN_DONE1, 1);
  if (sign_base->id() == 2) this->SetDailySign(ARMY_SIGN_DONE2, 1);
  if (sign_base->id() == 3) this->SetDailySign(ARMY_SIGN_DONE3, 1);

  MessageResponseArmySign msg;
  msg.set_sign_id(message->sign_id());
  msg.set_sign_progress(army->sign_value());
  msg.set_sign_num(army->sign_count());
  msg.set_army_exp(army->exp());
  msg.set_army_union(sign_base->league_currency);
  this->SendMessageToClient(MSG_CS_RESPONSE_ARMY_SIGN, &msg);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestArmyLevelUp(CSMessageEntry& entry) {
  Army* army_info = server->GetArmyByID(this->army_id_);
  if (!army_info) return ERR_ARMY_NOT_JOINED;

  const ArmyMemberInfo* member = army_info->GetArmyMemberInfo(this->uid());
  if (!member) return ERR_ARMY_NOT_JOINED;

  if (!(member->position() == ARMY_POSITION_VP ||
        member->position() == ARMY_POSITION_MASTER))
    return ERR_ARMY_PERMISSION;

  int32_t result = army_info->ArmyLevelUp(this);
  if (result) return result;

  this->SendMessageToClient(MSG_CS_RESPONSE_ARMY_LEVEL_UP, NULL);

  return ERR_OK;
}

bool LogicPlayer::GetDailySign(sy::DailySign type) {
  if(type < 0) return false;
  int32_t index = type / 32;
  if (index >= (int32_t)this->daily_sign_.size()) return false;
  return daily_sign_[index] & (1 << (type % 32));
}

void LogicPlayer::SetDailySign(sy::DailySign type, bool flag) {
  if(type < 0) return;
  int32_t index = type / 32;
  if (index >= (int32_t)this->daily_sign_.size()) this->daily_sign_.resize(index + 1, 0);

  uint32_t temp = 1 << (type % 32);
  if (flag)
    daily_sign_[index] |= temp;
  else
    daily_sign_[index] &= ~temp;
  this->UpdateDailySign();
}

void LogicPlayer::ResetDailySign() {
  this->daily_sign_.clear();
  this->UpdateDailySign();
}

void LogicPlayer::UpdateDailySign() {
  MessageSSUpdateDailySign update;
  MessageNotifyDailySign notify;
  for (size_t i = 0; i < daily_sign_.size(); i++) {
    update.add_daily_sign(this->daily_sign_[i]);
    notify.add_daily_sign(this->daily_sign_[i]);
  }
  this->SendMessageToClient(MSG_CS_NOTIFY_DAILY_SIGN, &notify);
  this->SendMessageToDB(MSG_SS_UPDATE_DAILY_SIGN, &update);
}

int32_t LogicPlayer::ProcessRequestGetArmySignAward(CSMessageEntry& entry) {
  MessageRequestGetArmySignAward* message =
      static_cast<MessageRequestGetArmySignAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  Army* army_info = server->GetArmyByID(this->army_id_);
  if (!army_info) return ERR_ARMY_NOT_JOINED;

  const ArmyMemberInfo* member = army_info->GetArmyMemberInfo(this->uid());
  if (!member) return ERR_ARMY_NOT_JOINED;

  LeagueSignRewardBase* base =
      LEAGUE_SIGN_REWARD_BASE.GetEntryByID(message->sign_id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (army_info->info().donate_value() < message->sign_id())
    return ERR_PARAM_INVALID;

  DailySign type = (DailySign)-1;
  switch (message->sign_id()) {
    case 65:
      type = ARMY_SIGN_65;
      break;
    case 130:
      type = ARMY_SIGN_130;
      break;
    case 165:
      type = ARMY_SIGN_165;
      break;
    case 210:
      type = ARMY_SIGN_210;
      break;
  }
  if (type < 0) return ERR_PARAM_INVALID;

  if (this->GetDailySign(type)) return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_ARMY_SIGN_AWARD, SYSTEM_ID_LEAGUE);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->reward);

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_ARMY_SIGN_AWARD,
                   SYSTEM_ID_LEAGUE);

  this->SetDailySign(type, true);

  MessageResponseGetArmySignAward msg;
  msg.set_sign_id(message->sign_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ARMY_SIGN_AWARD, &msg);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequsetArmyMaxLevelUp(CSMessageEntry& entry) {
  MessageRequestArmyMaxLevelUp* message =
      static_cast<MessageRequestArmyMaxLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;

  const ArmyMemberInfo* member = army->GetArmyMemberInfo(this->uid());
  if (!member) return ERR_ARMY_NOT_JOINED;

  if (!(member->position() == ARMY_POSITION_VP ||
        member->position() == ARMY_POSITION_MASTER))
    return ERR_ARMY_PERMISSION;

  int32_t result = army->ArmySkillLevelUp(message->skill_id(), this);
  if (result) return result;

  MessageResponseArmyMaxLevelUp res;
  res.set_skill_id(message->skill_id());
  res.set_skill_lv(army->skill_level(message->skill_id()));
  res.set_army_exp(army->exp());
  this->SendMessageToClient(MSG_CS_RESPONSE_ARMY_MAX_LEVEL_UP, &res);

  return ERR_OK;
}

int32_t LogicPlayer::GetTodayRechargeNum(time_t day) {
  int32_t today_money = 0;
  for (size_t i = 0; i < recharge_.size(); i++) {
    if (IsSameDay(day, this->recharge_[i].recharge_time()))
      today_money += this->recharge_[i].money();
  }
  return today_money / 10;
}

bool LogicPlayer::TodayHasRechargeOrder(int32_t money) {
  for (size_t i = 0; i < recharge_.size(); i++) {
    if (IsSameDay(GetVirtualSeconds(), this->recharge_[i].recharge_time())) {
      if (this->recharge_[i].money() == money) return true;
    }
  }
  return false;
}

bool LogicPlayer::TodayHasRechargeOrderID(int32_t good_id) {
  for (size_t i = 0; i < recharge_.size(); i++) {
    if (IsSameDay(GetVirtualSeconds(), this->recharge_[i].recharge_time())) {
      if (this->recharge_[i].goodid() == good_id) return true;
    }
  }
  return false;
}

int32_t LogicPlayer::ProcessRequestGetDailyActivity(CSMessageEntry& entry) {
  MessageResquestGetDailyActivity* message =
      static_cast<MessageResquestGetDailyActivity*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const DailyAwardBase* base =
      DAILY_AWARD_BASE.GetEntryByID(message->activity_id()).get();
  if (!base || !base->onsale) return ERR_PARAM_INVALID;

  if (this->GetDailySign((DailySign)base->id())) return ERR_PARAM_INVALID;

  CHECK_RET(this->CheckDailyActivityCondition(base));

  ModifyCurrency modify(MSG_CS_REQUEST_GET_DAILY_ACTIVITY,
                        SYSTEM_ID_DAILY_ACTIVITY);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  for (size_t i = 0; i < base->drop_box.size(); i++) {
    LootBase* loot =
        LootConfigFile::Get(base->drop_box[i], this->create_days()).get();
    if (!loot) return sy::ERR_INTERNAL;
    loot->Loot(modify, item_set, NULL);
  }

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_DAILY_ACTIVITY,
                   SYSTEM_ID_DAILY_ACTIVITY);
  this->SetDailySign((DailySign)base->id(), true);

  MessageResponseGetDailyActivity msg;
  msg.set_activity_id(base->id());
  FillToKVPair2(&modify, &item_set, msg.mutable_award());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_DAILY_ACTIVITY, &msg);

  return ERR_OK;
}

int32_t LogicPlayer::CheckDailyActivityCondition(const DailyAwardBase* base) {
  int32_t result = 0;

  switch (base->id()) {
    case 10:
      if (GetVirtualSeconds() > this->month_card_) result = ERR_PARAM_INVALID;
      break;
    case 11:
      if (GetVirtualSeconds() > this->big_month_card_)
        result = ERR_PARAM_INVALID;
      break;
    case 12:
      if (!this->life_card_) result = ERR_PARAM_INVALID;
      break;
    case 13:
      if (GetTime().tm_hour < 12 || GetTime().tm_hour > 14)
        result = ERR_PARAM_INVALID;
      break;
    case 14:
      if (GetTime().tm_hour < 18 || GetTime().tm_hour > 20)
        result = ERR_PARAM_INVALID;
      break;
    default:
      if (1 == base->condition.v1) {
        if (base->condition.v2 / 10 >
            this->GetTodayRechargeNum(GetVirtualSeconds()))
          result = ERR_RECHARGE_MONEY;
      }
      //if (2 == base->condition.v1) {
      //  if (!TodayHasRechargeOrder(base->condition.v2))
      //    result = ERR_PARAM_INVALID;
      //}
      if (3 == base->condition.v1) {
        if (!TodayHasRechargeOrderID(base->condition.v2))
          result = ERR_PARAM_INVALID;
      }
      break;
  }
  return result;
}

int32_t LogicPlayer::ProcessRequestGetArmyWarInfo(CSMessageEntry& entry) {
  MessageResponseGetArmyWarInfo response;
  Army* army = server->GetArmyByID(this->army_id());
  if (!army) return ERR_ARMY_NOT_JOINED;
  sy::ArmyWarInfo* current_war = NULL;
  std::vector<sy::ArmyWarInfo>* today_wars = NULL;
  army->GetArmyWars(current_war, today_wars);
  if (!current_war || !today_wars) {
    ERROR_LOG(logger)("PlayerID:%ld, Army:%ld, GetArmyWarInfo fail"
        , this->uid(), this->army_id());
    return ERR_INTERNAL;
  }

  for (uint32_t i = 0; i < today_wars->size(); ++i) {
    response.add_info()->CopyFrom(today_wars->at(i));
  }
  response.add_info()->CopyFrom(*current_war);
  response.set_next_chapter(army->next_chapter());
  response.set_current_chapter(army->max_chapter());

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ARMY_WAR_INFO, &response);
  return ERR_OK;
}

void LogicPlayer::OnArmyBossCopy(sy::ArmyBossInfo& boss,
                                 const ArmyBossBase* base,
                                 std::vector<int64_t>& current_hp, PK& pk,
                                 Army* army) {

  int64_t total_damage = 0;
  for (int32_t i = 0; i < 6; ++i) {
    int64_t sub = boss.left_hp(i) - current_hp[i];
    total_damage += sub > 0l ? sub : 0l;
    boss.set_left_hp(i, current_hp[i]);
  }

  ModifyCurrency modify(MSG_CS_REQUEST_ARMY_WAR_FIGHT, SYSTEM_ID_ARMY_WAR);
  modify[MONEY_KIND_UNION] += RandomBetween(base->reward_challenge.v1, base->reward_challenge.v2);
  //BOSS击杀状态(非0表示击杀)
  int32_t boss_status = army->OnArmyWarCopy(base, this->name());
  (void)boss_status;

  if (boss_status) modify[MONEY_KIND_UNION] += base->reward_kill2;
  if (this->CheckCurrency(modify)) {
    ERROR_LOG(logger)("OnArmyBossCopy PlayerID:%ld, MONEY_KIND_UNION:%d"
        , this->uid(), modify[MONEY_KIND_UNION]);
  }
  this->UpdateCurrency(modify);

  AddSubItemSet _;
  MessageNotifyFightResult notify;
  notify.set_win(true);
  notify.set_star(3);
  notify.set_round(pk.round);
  notify.set_copy_id(pk.copy->id());
  notify.set_total_damage(total_damage);
  FillFightResultMessage(notify, _, modify);
  if (boss_status) {
    notify.set_army_war_kill(base->reward_kill2);
    //增加军团经验
    army->AddArmyExp(base->reward_kill, this->uid(),
                     MSG_CS_REQUEST_ARMY_WAR_FIGHT);
  }
  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  this->SendMessageToSelf(MSG_CS_REQUEST_GET_ARMY_WAR_INFO, kEmptyMessage);

  army->UpdateArmyWarRankList(this, total_damage);
}

int32_t LogicPlayer::ProcessRequestSetArmyWarChapter(CSMessageEntry& entry) {
  MessageRequestSetArmyWarChapter* message = static_cast<MessageRequestSetArmyWarChapter*>(entry.get());
  if (!message) return ERR_INTERNAL;

  Army* army = server->GetArmyByID(this->army_id());
  if (!army) return ERR_PARAM_INVALID;
  sy::ArmyMemberInfo* member = army->GetMember(this->uid());
  if (!member) return ERR_INTERNAL;
  if (!member->position()) return ERR_ARMY_PERMISSION;

  int32_t chapter = message->is_current_chapter() ? army->max_chapter()
                                                  : army->max_chapter() + 1;
  army->next_chapter(chapter);

  MessageResponseSetArmyWarChapter response;
  response.set_next_chapter(chapter);
  this->SendMessageToClient(MSG_CS_RESPONSE_SET_ARMY_WAR_CHAPTER, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetArmyWarChapterAward(
    CSMessageEntry& entry) {
  MessageRequestGetArmyWarChapterAward *message = static_cast<MessageRequestGetArmyWarChapterAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const int32_t array[] = {
      sy::ACHIEVEMENT_TYPE_ARMY_WAR_CHAPTER1, //[0, 31]
      sy::ACHIEVEMENT_TYPE_ARMY_WAR_CHAPTER2, //[32, 63]
      sy::ACHIEVEMENT_TYPE_ARMY_WAR_CHAPTER3, //[64, 95]
      sy::ACHIEVEMENT_TYPE_ARMY_WAR_CHAPTER4, //[96, 127]
  };
  int32_t kChapterMax = sizeof(array) * 8 - 1;
  if (message->chapter() >= kChapterMax || message->chapter() < 0)
    return ERR_PARAM_ARRAY_BOUND;

  int32_t achievement_index = array[message->chapter() / 32];
  int32_t& value = this->achievements_[achievement_index];
  if (value & (1 << (message->chapter() % 32))) return ERR_PARAM_INVALID;

  AddSubItemSet item_set;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_ARMY_WAR);
  MessageResponseGetArmyWarChapterAward response;
  response.set_chapter(message->chapter());

  value |= (1 << (message->chapter() % 32));
  const ArmyBossBase* base = ARMY_BOSS_BASE.GetEntryByID(message->chapter() * 10 + 1).get();
  if (!base) return ERR_INTERNAL;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->reward_once);

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, SYSTEM_ID_ARMY_WAR, entry.head.msgid);
  this->UpdateAchievement(achievement_index, value);

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ARMY_WAR_CHAPTER_AWARD, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetArmyWarBossAward(CSMessageEntry& entry) {
  MessageRequestGetArmyWarBossAward* message = static_cast<MessageRequestGetArmyWarBossAward*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->copy_id() % 10 > 4 || message->copy_id() % 10 < 1)
    return ERR_PARAM_INVALID;
  const ArmyBossBase* base = ARMY_BOSS_BASE.GetEntryByID(message->copy_id()).get();
  if (!base) return ERR_PARAM_INVALID;

  Army* army = server->GetArmyByID(this->army_id());
  if (!army) return ERR_ARMY_NOT_JOINED;

  int32_t result = ERR_OK;

  MessageResponseGetArmyWarBossAward response;
  response.set_index(message->index());
  response.set_copy_id(message->copy_id());

  sy::ArmyBossMemAward* award_info = NULL;
  sy::ArmyWarInfo* war = NULL;
  std::vector<sy::ArmyWarInfo>* passed_war = NULL;
  army->GetArmyWars(war, passed_war);
  sy::ArmyWarInfo* save_war = NULL;
  VectorSet<int32_t> index_set;
  //今天通关的
  for (std::vector<sy::ArmyWarInfo>::iterator iter = passed_war->begin();
       iter != passed_war->end(); ++iter) {
    if (iter->boss1().copy_id() / 10 != message->copy_id() / 10) continue;
    sy::ArmyBossInfo* array[] = {iter->mutable_boss1(), iter->mutable_boss2(),
                                 iter->mutable_boss3(), iter->mutable_boss4()};
    sy::ArmyBossInfo& info = *array[message->copy_id() % 10 - 1];
    for (int32_t index = 0; index < info.awards_size(); ++index) {
      if (info.awards(index).player_id() != this->uid()) continue;
      award_info = info.mutable_awards(index);
      save_war = &*iter;
      break;
    }
  }
  //正在打的
  if (war && message->copy_id() / 10 == war->boss1().copy_id() / 10) {
    sy::ArmyBossInfo* array[] = {war->mutable_boss1(), war->mutable_boss2(),
                                 war->mutable_boss3(), war->mutable_boss4()};
    sy::ArmyBossInfo& info = *array[message->copy_id() % 10 - 1];
    for (int32_t index = 0; index < info.awards_size(); ++index) {
      if (info.awards(index).player_id() != this->uid()) {
        if (info.awards(index).index()) {
          index_set.insert(info.awards(index).index());
        }
        continue;
      }
      award_info = info.mutable_awards(index);
      save_war = war;
    }
  }
  if (!save_war) return ERR_INTERNAL;
  if (!award_info) result = ERR_PARAM_INVALID;
  if (award_info->index()) result = ERR_DEBUG_ARMY_WAR_BOSS_REWARDED;
  if (!index_set.insert(message->index()).second)
    result = ERR_ARMY_BOSS_ANOTHER_AWARD;
  if (!result) {
    ValuePair3<int32_t, int32_t, int32_t> award =
        base->reward_choose[award_info->award() % base->reward_choose.size()];

    ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_ARMY_WAR);
    AddSubItemSet item_set;
    FillCurrencyAndItem<kAdd>(modify, item_set,
                              ValuePair2<int32_t, int32_t>(award.v1, award.v2));
    CHECK_RET(this->CheckCurrency(modify));
    CHECK_RET(this->CheckItem(&item_set, NULL, NULL););

    award_info->set_index(message->index());
    army->SaveArmyWar(*save_war);

    this->UpdateCurrency(modify);
    this->ObtainItem(&item_set, NULL, NULL, SYSTEM_ID_ARMY_WAR,
                     entry.head.msgid);
  }
  if (result) {
    response.clear_index();
    response.clear_copy_id();
  }
  response.mutable_war()->CopyFrom(*save_war);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ARMY_WAR_BOSS_AWARD, &response);
  return result;
}

int32_t LogicPlayer::ProcessRequestArmyWarFight(CSMessageEntry& entry) {
  MessageRequestArmyWarFight* message = static_cast<MessageRequestArmyWarFight*>(entry.get());
  if (!message) return ERR_INTERNAL;
  Army* army = server->GetArmyByID(this->army_id());
  if (!army) return ERR_ARMY_NOT_JOINED;
  int32_t chapter = message->copy_id() / 10;
  int32_t chapter_index = message->copy_id() % 10;
  if (chapter_index < 0 || chapter_index > 4) return ERR_PARAM_INVALID;
  const CopyBase* copy_base = COPY_BASE.GetEntryByID(62900).get();
  if (!copy_base) return ERR_COPY_INVALID;
  int32_t hour = GetTime().tm_hour;
  if (hour <= GetSettingValue(league_copy_time1)) hour = GetSettingValue(league_copy_time1);
  if (hour >= GetSettingValue(league_copy_time2)) hour = GetSettingValue(league_copy_time2);
  int32_t time_count = (hour - GetSettingValue(league_copy_time1)) / GetSettingValue(league_copy_recover_time);
  int32_t left_count = GetSettingValue(league_copy_free_time) +
                       this->daily_counter_[COUNT_TYPE_ARMY_WAR_BUY_COUNT] +
                       time_count -
                       this->daily_counter_[COUNT_TYPE_ARMY_WAR_COUNT];
  if (left_count <= 0) return ERR_COPY_COUNT;

  sy::ArmyWarInfo* war = army->army_war(chapter);
  if (!war) {
    ERROR_LOG(logger)("ArmyWarInfo not found, CopyID:%d", message->copy_id());
    return ERR_PARAM_INVALID;
  }
  if (war->boss1().copy_id() != message->copy_id() &&
      war->boss2().copy_id() != message->copy_id() &&
      war->boss3().copy_id() != message->copy_id() &&
      war->boss4().copy_id() != message->copy_id()) {
    ERROR_LOG(logger)("PlayerID:%ld, ArmyID:%ld, BossID:%d, CopyID:%d"
        , this->uid(), this->army_id(), war->boss1().copy_id(), message->copy_id());
    return ERR_PARAM_INVALID;
  }
  sy::ArmyBossInfo* boss = NULL;
  switch (chapter_index) {
    case 1: boss = war->mutable_boss1(); break;
    case 2: boss = war->mutable_boss2(); break;
    case 3: boss = war->mutable_boss3(); break;
    case 4: boss = war->mutable_boss4(); break;
  }
  if (!boss) {
    ERROR_LOG(logger)("ArmyWarBoss not found, BossID:%d", chapter_index);
    return ERR_PARAM_INVALID;
  }

  const ArmyBossBase* base = ARMY_BOSS_BASE.GetEntryByID(message->copy_id()).get();
  if (!base) {
    ERROR_LOG(logger)("ArmyBossBase not found, CopyID:%d", message->copy_id());
    return ERR_PARAM_INVALID;
  }

  //检查血量
  int64_t total_hp = 0;
  for (int32_t i = 0; i < boss->left_hp_size(); ++i) {
    total_hp += boss->left_hp(i);
  }
  if (total_hp <= 0) {
    this->SendMessageToSelf(MSG_CS_REQUEST_GET_ARMY_WAR_INFO, kEmptyMessage);
    return ERR_ARMY_BOSS_DEAD;
  }

  sy::CurrentCarrierInfo carrier;
  std::vector<sy::HeroInfo> monster;
  carrier = base->monster_base->carrier_info();
  monster = base->monster_base->hero_info();

  //战斗
  PK pk(copy_base);
  pk.InitPlayer(this, true);
  pk.InitMonsterInfo(carrier, monster, boss->mutable_left_hp()->begin());
  pk.GeneratePVPReport();
  if (server_config->report()) {
    LogReportToFile(pk.report);
  }

  std::vector<int64_t> current_hp;
  current_hp.resize(6, 0);
  for (int32_t pos = 1; pos <= 6; ++pos) {
    PkHero* hero = pk.b.GetHeroByPos(pos);
    if (hero) {
      current_hp[pos - 1] = hero->hp();
    } else {
      INFO_LOG(logger)("Pos:%d, invalid", pos);
    }
  }

  this->UpdateBuyCount(COUNT_TYPE_ARMY_WAR_COUNT, 1);
  //发送战报
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
  //奖励
  this->OnArmyBossCopy(*boss, base, current_hp, pk, army);
  army->SaveArmyWar(*war);

  server->UpdateCopyStatistics(copy_base->type, message->copy_id(), this->uid());
  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_ARMY_COPY, 1, this);
  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_ARMY_COPY, 1, this);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetDailyVIPAward(CSMessageEntry& entry) {
  if (this->GetDailySign(VIP_DAILY_AWARD)) return ERR_PARAM_INVALID;

  VIPDailyAwardBase* base =
      VIP_DAILY_AWARD_BASE.GetEntryByID(this->vip_level()).get();
  if (!base) return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_VIP_DAILY_AWARD,
                        SYSTEM_ID_DAILY_ACTIVITY);
  AddSubItemSet item;
  FillCurrencyAndItem<kAdd>(modify, item, base->award);

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item, NULL, NULL, MSG_CS_REQUEST_GET_VIP_DAILY_AWARD,
             SYSTEM_ID_DAILY_ACTIVITY);

  this->SetDailySign(VIP_DAILY_AWARD, true);

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_VIP_DAILY_AWARD, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestRaiseFunding(CSMessageEntry& entry) {
  MessageRequestRaiseFunding* message =
      static_cast<MessageRequestRaiseFunding*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ModifyCurrency modify(MSG_CS_REQUEST_RAISE_FUNDING, SYSTEM_ID_DAILY_ACTIVITY);
  AddSubItemSet item_set;

  if (message->is_reward()) {
    if (this->achievements_[OTHER_RAISE_FUNDING] >= 0) return ERR_PARAM_INVALID;

    modify[MONEY_KIND_COIN] = this->achievements_[OTHER_RAISE_FUNDING_MONEY];
    CHECK_RET(CheckCurrency(modify));
    UpdateCurrency(modify);

    UpdateAchievement(OTHER_RAISE_FUNDING,
                      abs(this->achievements_[OTHER_RAISE_FUNDING]));
    UpdateAchievement(OTHER_RAISE_FUNDING_MONEY, 0);

  } else {
    if (this->daily_counter_[COUNT_TYPE_RAISE_FUNDING] >=
            GetSettingValue(God_of_Wealth_click_everyday) ||
        this->achievements_[OTHER_RAISE_FUNDING] < 0)
      return ERR_PARAM_INVALID;

    if (((GetVirtualSeconds() -
          this->daily_counter_[COUNT_TYPE_RAISE_FUNDING_TIME]) <
         GetSettingValue(God_of_Wealth_cd)))
      return ERR_PARAM_INVALID;

    LootBase* loot =
        LootConfigFile::Get(GetSettingValue(God_of_Wealth_money_drop),
                            this->level())
            .get();
    if (!loot) return sy::ERR_INTERNAL;
    loot->Loot(modify, item_set, NULL);
    loot = LootConfigFile::Get(GetSettingValue(God_of_Wealth_oil_drop),
                               this->level())
               .get();
    if (!loot) return sy::ERR_INTERNAL;
    loot->Loot(modify, item_set, NULL);

    CHECK_RET(CheckCurrency(modify));
    CHECK_RET(CheckItem(&item_set, NULL, NULL));
    UpdateCurrency(modify);
    ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_RAISE_FUNDING,
               SYSTEM_ID_DAILY_ACTIVITY);

    if (!(++this->achievements_[OTHER_RAISE_FUNDING] %
          GetSettingValue(God_of_Wealth_total_click)))
      this->achievements_[OTHER_RAISE_FUNDING] *= -1;
    UpdateAchievement(OTHER_RAISE_FUNDING,
                      this->achievements_[OTHER_RAISE_FUNDING]);

    UpdateBuyCount(COUNT_TYPE_RAISE_FUNDING, 1);
    this->daily_counter_[COUNT_TYPE_RAISE_FUNDING_TIME] =
        (int32_t)GetVirtualSeconds();
    MessageNotifyBuyCount notify;
    sy::KVPair2* info = notify.add_infos();
    info->set_key(COUNT_TYPE_RAISE_FUNDING_TIME);
    info->set_value(this->daily_counter_[COUNT_TYPE_RAISE_FUNDING_TIME]);
    this->SendMessageToClient(MSG_CS_NOTIFY_BUY_COUNT, &notify);
    MessageSSUpdateBuyCount request;
    request.add_infos()->CopyFrom(*info);
    this->SendMessageToDB(MSG_SS_UPDATE_BUY_COUNT, &request);

    UpdateAchievement(OTHER_RAISE_FUNDING_MONEY,
                      this->achievements_[OTHER_RAISE_FUNDING_MONEY] +
                          modify[MONEY_KIND_COIN]);
  }

  MessageRespnseRaiseFunding response;
  response.set_is_reward(message->is_reward());
  FillToKVPair2(&modify, &item_set, response.mutable_award());
  this->SendMessageToClient(MSG_CS_RESPONSE_RAISE_FUNDING, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestFirstRecharge(CSMessageEntry& entry) {
  MessageResquestFirstRecharge* message =
      static_cast<MessageResquestFirstRecharge*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageResponseFirstRecharge res;
  res.set_is_second(message->is_second());

  ModifyCurrency modify(MSG_CS_REQUEST_FIRST_RECHARGE,
                        SYSTEM_ID_DAILY_ACTIVITY);
  AddSubItemSet item_set;

  int32_t temp = this->achievements_[OTHER_FIRST_RECHARGE];

  if (message->is_second()) {
    if (!(temp & (1 << 1)) || temp & (1 << 2)) return ERR_PARAM_INVALID;
    FillCurrencyAndItem<kAdd>(
        modify, item_set, Setting::GetValue2(Setting::second_charge_reward));
    temp |= (1 << 2);
  } else {
    if (!(temp & (1 << 4))) return ERR_PARAM_INVALID;
    if (temp & 1) return ERR_PARAM_INVALID;
    temp |= 1;
    FillCurrencyAndItem<kAdd>(modify, item_set,
                              Setting::GetValue2(Setting::first_charge_reward));
  }

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_FIRST_RECHARGE,
             SYSTEM_ID_DAILY_ACTIVITY);

  this->UpdateAchievement(OTHER_FIRST_RECHARGE, temp);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIRST_RECHARGE, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetVIPWeekly(CSMessageEntry& entry) {
  MessageRequestGetVIPWeekly* message =
      static_cast<MessageRequestGetVIPWeekly*>(entry.get());
  if (!message) return ERR_INTERNAL;

  VIPWeeklyShopBase* base =
      VIP_WEEKLY_SHOP_BASE.GetEntryByID(message->id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (this->level() < base->lv || this->vip_level() < base->vip)
    return ERR_PARAM_INVALID;

  if (vip_weekly_[message->id()] >= base->available_times)
    return ERR_PARAM_INVALID;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_VIP_WEEKLY,
                        SYSTEM_ID_DAILY_ACTIVITY);
  AddSubItemSet item_set;
  modify[base->moneytype] = -base->price;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_VIP_WEEKLY,
             SYSTEM_ID_DAILY_ACTIVITY);

  vip_weekly_[message->id()]++;
  this->UpdateVIPDailyWeekly();

  MessageResponseGetVIPWeekly res;
  res.set_id(message->id());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_VIP_WEEKLY, &res);

  return ERR_OK;
}

void LogicPlayer::UpdateVIPDailyWeekly() {
  MessageSSUpdateVIPWeekly update;
  MessageNotifyVIPWeekly notify;

  for (VectorMap<int32_t, int32_t>::iterator it = this->vip_weekly_.begin();
       it != this->vip_weekly_.end(); ++it) {
    KVPair2* data1 = update.add_vip_weekly();
    data1->set_key(it->first);
    data1->set_value(it->second);

    KVPair2* data2 = notify.add_vip_weekly();
    data2->set_key(it->first);
    data2->set_value(it->second);
  }

  this->SendMessageToClient(MSG_CS_NOTIFY_VIP_WEEKLY, &notify);
  this->SendMessageToDB(MSG_SS_UPDATE_VIP_WEEKLY, &update);
}

void LogicPlayer::RefreshWeeklySystem(int32_t diff_day) {
  tm tm_1;
  time_t curr = GetVirtualSeconds();
  localtime_r(&curr, &tm_1);
  if (tm_1.tm_wday - diff_day < 0) {
    TRACE_LOG(logger)("LogicPlayer::RefreshWeeklySystem PlayerID:%ld"
        , this->uid());
    this->vip_weekly_.clear();
    vip_weekly_[-1] = this->vip_level();
    this->UpdateVIPDailyWeekly();

    int32_t temp_shops[] = {11, 15, 16, 17, 18, 19, 20, 21, 22, 26};
    for (size_t i = 0; i < sizeof(temp_shops) / sizeof(*temp_shops); i++) {
      RefreshShopInfo& shop_info = refresh_shop_info_[temp_shops[i]];
      shop_info.set_shop_id(temp_shops[i]);
      ShopBase::RandomFeatsCommodity(temp_shops[i], this->level(),
                                     shop_info.mutable_feats_commodity(),
                                     server->GetServerStartDays());
      UpdateFeatsShopInfo(temp_shops[i]);
    }
    //移除跨服积分战积分
    int32_t point_count = 0;
    point_count = this->GetItemCountByItemID(23900024);
    if (point_count > 0) {
      AddSubItemSet items;
      items.push_back(ItemParam(23900024, -point_count));
      if (!CheckItem(&items, NULL, NULL))
        this->ObtainItem(&items, NULL, NULL, 0, 0);
    }
  }
}

bool LevelSort(const Army* v1, const Army* v2) {
  return v1->level() < v2->level();
}
bool CopySort(const Army* v1, const Army* v2) {
  return v1->max_chapter() < v2->max_chapter();
}

bool Acd(const Army* v1, const Army* v2) { return v1->level() < v2->level(); }

int32_t LogicPlayer::ProcessRequestArmyList(CSMessageEntry& entry) {
  MessageRequestArmyList* message =
      static_cast<MessageRequestArmyList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->page_size() <= 1 || message->page_size() > 100)
    return ERR_PARAM_INVALID;

  boost::unordered_map<int64_t, Server::ArmyPtr>& armys = server->army_list();
  std::vector<Army*> all_list;
  std::vector<Army*> apply_list;
  for (boost::unordered_map<int64_t, Server::ArmyPtr>::iterator it =
           armys.begin();
       it != armys.end(); ++it) {
    if (message->has_apply() &&
        server->IsPlayerApplyArmy(this->uid(), it->first)) {
      apply_list.push_back(it->second.get());
    } else {
      //军团战役排行榜过滤掉没有打过的军团
      if (message->army_copy() && !it->second->max_chapter()) continue;
      all_list.push_back(it->second.get());
    }
  }

  if (message->army_copy())
    std::sort(all_list.begin(), all_list.end(), CopySort);
  else
    std::sort(all_list.begin(), all_list.end(), LevelSort);
  all_list.insert(all_list.end(), apply_list.begin(), apply_list.end());
  std::reverse(all_list.begin(), all_list.end());

  int32_t all_page = armys.size() / message->page_size();
  int32_t start_index = message->page_size() * (message->page() - 1);
  int32_t num = 0;
  MessageResponseArmyList res;
  res.set_all_page(all_page);
  res.set_page(message->page());
  VectorSet<int64_t>& vsa = server->applies(this->uid());
  for (VectorSet<int64_t>::iterator iter = vsa.begin(); iter != vsa.end();
       ++iter)
    res.add_apply_army_id(*iter);
  for (size_t i = start_index; i < all_list.size(); ++i) {
    if (num >= message->page_size()) break;
    sy::ArmyInfo* army_info = res.add_info();
    army_info->CopyFrom(all_list[i]->info());
    army_info->mutable_log()->Clear();
    army_info->mutable_army_shop()->Clear();
    army_info->mutable_buy_record()->Clear();
    army_info->set_member_count(all_list[i]->members().size());
    num++;
  }

  //军团战役排行
  //军团排行
  if (message->army_copy() || !message->has_apply())
    SendMessageToClient(MSG_CS_RESPONSE_ARMY_LIST_RANK, &res);
  else
    SendMessageToClient(MSG_CS_RESPONSE_ARMY_LIST, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSearchArmy(CSMessageEntry& entry) {
  MessageRequestSearchArmy* message =
      static_cast<MessageRequestSearchArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->army_name().empty() ||
      message->army_name().length() > 64u)
    return ERR_PARAM_INVALID;

  MessageResponseSearchArmy res;
  boost::unordered_map<int64_t, Server::ArmyPtr>& armys = server->army_list();
  for (boost::unordered_map<int64_t, Server::ArmyPtr>::iterator it =
           armys.begin();
       it != armys.end(); ++it) {
    if (message->army_name() == it->second->name()) {
      sy::ArmyInfo* army_info = res.add_info();
      army_info->CopyFrom(it->second->info());
      army_info->mutable_log()->Clear();
      army_info->set_member_count(it->second->members().size());
      break;
    }
  }

  VectorSet<int64_t>& vsa = server->applies(this->uid());
  for (VectorSet<int64_t>::iterator iter = vsa.begin(); iter != vsa.end();
       ++iter)
    res.add_apply_army_id(*iter);

  this->SendMessageToClient(MSG_CS_RESPONSE_SEARCH_ARMY, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestChangeArmyPos(CSMessageEntry& entry) {
  MessageRequestChangeArmyPos* message =
      static_cast<MessageRequestChangeArmyPos*>(entry.get());
  if (!message) return ERR_INTERNAL;

  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;

  if (message->player_id() == this->uid()) return ERR_PARAM_INVALID;

  ArmyMemberInfo* mine = army->GetMember(this->uid());
  if (!mine) return ERR_ARMY_MEMBER_NOT_FOUND;

  if (mine->position() != ARMY_POSITION_MASTER) return ERR_ARMY_PERMISSION;

  ArmyMemberInfo* other = army->GetMember(message->player_id());
  if (!other) return ERR_ARMY_MEMBER_NOT_FOUND;

  if (message->pos() == ARMY_POSITION_MASTER) {
    if (other->position() != ARMY_POSITION_VP) return ERR_ARMY_PERMISSION;

    mine->set_position(other->position());
    other->set_position(message->pos());
    army->UpdateMember(mine->player_id());
    army->UpdateMember(other->player_id());
    army->SetMaster(other->player_id(),other->name());
    MessageNotifyArmyMemberPos notify;
    notify.set_pos(mine->position());
    this->SendMessageToClient(MSG_CS_NOTIFY_ARMY_MEMBER_POS, &notify);
  } else {
    if (message->pos() == ARMY_POSITION_VP) {
      int32_t vp_num = 0;
      for (size_t i = 0; i < army->members().size(); i++) {
        if (army->members()[i].position() == ARMY_POSITION_VP) ++vp_num;
      }
      if (vp_num >= GetSettingValue(league_controller_limit))
        return ERR_ARMY_VP_HAS_TOO_MUCH;
    }
    other->set_position(message->pos());
    army->UpdateMember(other->player_id());
  }

  LogicPlayer* op = server->GetPlayerByID(other->player_id());
  if (op) {
    MessageNotifyArmyMemberPos notify;
    notify.set_pos(other->position());
    op->SendMessageToClient(MSG_CS_NOTIFY_ARMY_MEMBER_POS, &notify);
  }

  army->ChangePosLog(this->name(), other->name(), message->pos());

  MessageResponseChangeArmyPos res;
  res.set_player_id(other->player_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_CHANGE_ARMY_POS, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessReuqestGetArmyShopInfo(CSMessageEntry& entry) {
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;

  army->RefreshArmyShop();

  MessageResponseGetArmyShopInfo res;
  res.mutable_army_shop()->CopyFrom(army->info().army_shop());
  res.mutable_buy_record()->CopyFrom(army->info().buy_record());

  SendMessageToClient(MSG_CS_RESPONSE_GET_ARMY_SHOP_INFO, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetWorldBossInfo(CSMessageEntry& entry) {
  if (Server::IsWorldBossTime(GetVirtualSeconds())) {
    if (!IsSameDay(this->world_boss_info_.refresh_time(),
                   GetVirtualSeconds())) {
      INFO_LOG(logger)("WorldBoss Clear, PlayerID:%ld", this->uid());
      this->world_boss_info_.Clear();
      this->world_boss_info_.set_refresh_time(GetVirtualSeconds());
      UpdateWorldBossInfo();
    }
  }
  MessageResponseGetWorldBoss res;
  res.mutable_info()->CopyFrom(this->world_boss_info_);
  res.mutable_boss()->CopyFrom(server->world_boss());
  Army* army = server->GetArmyByID(this->army_id_);
  if (army) res.set_army_merit(army->army_merit());

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_WORLD_BOSS, &res);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestFightWorldBoss(CSMessageEntry& entry) {
  if (!Server::IsWorldBossTime(GetVirtualSeconds())) return ERR_PARAM_INVALID;
  if (!this->world_boss_info_.country()) return ERR_PARAM_INVALID;

  if (GetVirtualSeconds() - server->world_boss().last_dead_time() <=
      Setting::GetValue1(Setting::pirate_boss_recover_time)[0])
    return ERR_WORLD_BOSS_CD;

  const std::vector<int32_t>& challenge =
      Setting::GetValue1(Setting::pirate_boss_challenge_time);
  const std::vector<int32_t>& activity =
      Setting::GetValue1(Setting::pirate_boss_activity_time2);

  tm tm_1 = GetTime();
  int32_t add_time = (tm_1.tm_hour - activity[0]) / challenge[1];
  add_time = add_time > 0 ? add_time : 0;
  int max_fight_count =
      challenge[0] + add_time +
      this->daily_counter_[COUNT_TYPE_WORLD_BOSS_BOUGHT_COUNT];

  if (this->daily_counter_[COUNT_TYPE_WORLD_BOSS_FIGHT] >= max_fight_count)
    return ERR_PARAM_INVALID;

  int32_t result = FightWorldBoss();
  if (result) return result;

  this->UpdateBuyCount(COUNT_TYPE_WORLD_BOSS_FIGHT, 1);
  MessageResponseFightWorldBoss response;
  response.mutable_boss()->CopyFrom(server->world_boss());
  response.mutable_info()->CopyFrom(this->world_boss_info_);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_WORLD_BOSS, &response);

  return ERR_OK;
}

int32_t LogicPlayer::FightWorldBoss() {
  const CopyBase* copy_base = COPY_BASE.GetEntryByID(63000).get();
  if (!copy_base) return ERR_COPY_INVALID;

  sy::CurrentCarrierInfo carrier;
  std::vector<sy::HeroInfo> monster;
  WorldBossInfo& boss = server->world_boss();
  WorldBossAttrBase::FillMonsterInfos(carrier, monster, boss.level(),
                                      boss.group_id());
  for (size_t i = 0; i < monster.size(); i++) {
    //世界BOSS的国家伤害加成
    int32_t index_map[] = {
        ATTACK_ATTR_UK_DAMAGE_DEC,
        ATTACK_ATTR_US_DAMAGE_DEC,
        ATTACK_ATTR_GE_DAMAGE_DEC,
        ATTACK_ATTR_JP_DAMAGE_DEC,
    };
    if (this->world_boss_info_.country() >= 1 &&
        this->world_boss_info_.country() <= 4) {
      int32_t index = index_map[this->world_boss_info_.country() - 1];
      monster[i].set_attr1(index, -3000);
    }
  }
  PK pk(copy_base);
  pk.InitPlayer(this, true, NULL, 0.01f);
  pk.InitMonsterInfo(carrier, monster, boss.current_hp().begin());
  pk.GeneratePVPReport();
  if (server_config->report()) LogReportToFile(pk.report);
  MessageNotifyWorldBossFightResult notify;

  std::vector<int64_t> current_hp;
  current_hp.resize(6, 0);
  int64_t all_current_hp = 0;
  int64_t all_damage = 0;
  for (int32_t i = 0; i < 6; i++) {
    PkHero* hero = pk.b.GetHeroByPos(i + 1);
    if (hero) {
      current_hp[i] = hero->hp();
      all_current_hp += current_hp[i];
      all_damage += boss.current_hp(i) - current_hp[i];
      boss.set_current_hp(i, current_hp[i]);
    } else {
      INFO_LOG(logger)("WorldBoss Pos:%d, invalid", i + 1);
    }
  }
  int64_t all_merit = all_damage / 10000;
  notify.set_merit(all_merit);
  notify.set_damage(all_damage);
  if (all_damage > this->world_boss_info_.damage())
    this->world_boss_info_.set_damage(all_damage);
  this->world_boss_info_.set_merit(this->world_boss_info_.merit() + all_merit);

  RANK_LIST.OnWorldBossMerit(this);
  RANK_LIST.OnWorldBossDamage(this);

  Army* army = server->GetArmyByID(this->army_id_);
  if (army) {
    army->AddArmyMerit(all_merit);
    RANK_LIST.OnArmyAddMerit(army);
  }

  ModifyCurrency modify(MSG_CS_REQUEST_FIGHT_WORLD_BOSS, SYSTEM_ID_WORLD_BOSS);
  AddSubItemSet item_set;

  //最后一击
  if (all_current_hp <= 0) {
    const LootBasePtr& loot = LootConfigFile::Get(
        Setting::GetValue1(Setting::pirate_boss_last_hit_reward)[0],
        boss.level());
    if (loot) loot->Loot(modify, item_set, NULL);
    FillToKVPair2(&modify, &item_set, notify.mutable_last_straw());
    boss.set_last_dead_time(GetVirtualSeconds());
    boss.set_kill_player_name(this->name());
    if (!item_set.empty())
      server->AddWorldBossLog(1, this->name(), item_set[0].item_id,
                              item_set[0].item_count);
  }
  //战功
  {
    const LootBasePtr& loot = LootConfigFile::Get(
        Setting::GetValue1(Setting::pirate_boss_every_hit_reward)[0],
        boss.level());
    if (loot) loot->Loot(modify, item_set, NULL);
    notify.set_exploit(modify[MONEY_KIND_EXPLOIT]);
  }
  //幸运一击
  if (!boss.luck_player() &&
      ((RandomBetween(0, 100000) <= 3222 * boss.fight_count()) ||
       (all_current_hp <= 0))) {
    const LootBasePtr& loot = LootConfigFile::Get(
        Setting::GetValue1(Setting::pirate_boss_lucky_hit_reward)[0],
        boss.level());
    if (loot) loot->Loot(modify, item_set, NULL);
    KVPair2* pair = notify.add_luck_hit();
    pair->set_key(MONEY_KIND_MONEY);
    pair->set_value(modify[MONEY_KIND_MONEY]);
    boss.set_luck_player(this->uid());
    server->AddWorldBossLog(2, this->name(), MONEY_KIND_MONEY,
                            modify[MONEY_KIND_MONEY]);
  }

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_FIGHT_WORLD_BOSS, SYSTEM_ID_WORLD_BOSS);

  boss.set_fight_count(boss.fight_count() + 1);
  server->SaveWorldBoss();
  this->UpdateWorldBossInfo();

  MessageNotifyWorldBossStatus status;
  status.set_player_name(this->player_.name());
  status.mutable_info()->CopyFrom(server->world_boss());
  status.set_damage(all_damage);
  server->SendMessageToWorldBossPlayers(MSG_CS_NOTIFY_WORLD_BOSS_STATUS,
                                        &status, this->uid());

  this->SendMessageToClient(MSG_CS_NOTIFY_WORLD_BOSS_FIGHT_RESULT, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestWorldBossCountry(CSMessageEntry& entry) {
  MessageRequestWorldBossCountry* message =
      static_cast<MessageRequestWorldBossCountry*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (this->world_boss_info_.country()) return ERR_PARAM_INVALID;
  if (message->country() < 1 || message->country() > 4)
    return ERR_PARAM_INVALID;
  this->world_boss_info_.set_country(message->country());
  UpdateWorldBossInfo();

  MessageResponseWorldBossCountry res;
  res.set_country(this->world_boss_info_.country());
  this->SendMessageToClient(MSG_CS_RESPONSE_WORLD_BOSS_COUNTRY, &res);

  return ERR_OK;
}

void LogicPlayer::UpdateWorldBossInfo() {
  server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypeServerBoss,
                            world_boss_info_.SerializeAsString());
}

int32_t LogicPlayer::ProcessRequestGetWorldBossMeritAward(
    CSMessageEntry& entry) {
  MessageRequestGetEWorldBossMeritAward* message =
      static_cast<MessageRequestGetEWorldBossMeritAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_WORLD_BOSS_MERIT_AWARD,
                        SYSTEM_ID_WORLD_BOSS);
  AddSubItemSet item_set;

  int64_t record = this->world_boss_info_.merit_award();
  for (int32_t i = 0; i < message->id_size(); i++) {
    WorldBossMeritAwardBase* base =
        WORLD_BOSS_MERTIA_AWARD_BASE.GetEntryByID(message->id(i)).get();
    if (!base) return ERR_PARAM_INVALID;
    int64_t flag = (int64_t)1 << base->index;
    if (record & flag) return ERR_PARAM_INVALID;
    if (this->level() < base->lv) return ERR_PARAM_INVALID;
    if (this->world_boss_info_.merit() < base->merit_num)
      return ERR_PARAM_INVALID;
    FillCurrencyAndItem<kAdd>(modify, item_set, base->merit_award);
    record |= flag;
  }

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_WORLD_BOSS_MERIT_AWARD,
             SYSTEM_ID_WORLD_BOSS);

  this->world_boss_info_.set_merit_award(record);
  MessageResponseGetEWorldBossMeritAward res;
  res.set_got_record(record);
  res.mutable_id()->CopyFrom(message->id());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_WORLD_BOSS_MERIT_AWARD, &res);
  this->UpdateWorldBossInfo();
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetWorldBossKillAward(
    CSMessageEntry& entry) {
  MessageRequestGetEWorldBossKillAward* message =
      static_cast<MessageRequestGetEWorldBossKillAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ModifyCurrency modify(MSG_CS_REQUEST_GET_WORLD_BOSS_KILL_AWARD,
                        SYSTEM_ID_WORLD_BOSS);
  AddSubItemSet item_set;

  int64_t record = this->world_boss_info_.kill_award();
  int64_t total_hp = 0;
  for (int32_t index = 0; index < server->world_boss().current_hp_size();
       ++index) {
    int64_t hp = server->world_boss().current_hp(index);
    total_hp += hp <= 0 ? 0 : hp;
  }

  for (int32_t i = 0; i < message->id_size(); i++) {
    WorldBossKillAwardBase* base =
        WORLD_BOSS_KILL_AWARD_BASE.GetEntryByID(message->id(i)).get();
    if (!base) return ERR_PARAM_INVALID;
    int64_t flag = (int64_t)1 << (base->id() / 5);
    if (record & flag) return ERR_PARAM_INVALID;
    if (server->world_boss().level() < base->id()) return ERR_PARAM_INVALID;
    if (server->world_boss().level() == base->id() && total_hp > 0)
      return ERR_PARAM_INVALID;
    FillCurrencyAndItem<kAdd>(modify, item_set, base->merit_award);
    record |= flag;
  }

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_WORLD_BOSS_KILL_AWARD,
             SYSTEM_ID_WORLD_BOSS);
  this->world_boss_info_.set_kill_award(record);
  MessageResponseGetEWorldBossKillAward res;
  res.set_got_record(record);
  res.mutable_id()->CopyFrom(message->id());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_WORLD_BOSS_KILL_AWARD, &res);
  this->UpdateWorldBossInfo();
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetWorldBossMeritRank(
    CSMessageEntry& entry) {
  MessageResponseGetWorldBossMeritRank res;
  res.mutable_uk_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_MERIT_UK).data());
  res.mutable_us_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_MERIT_US).data());
  res.mutable_ge_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_MERIT_GE).data());
  res.mutable_jp_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_MERIT_JP).data());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_WORLD_BOSS_MERIT_RANK, &res);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetWorldBossDamageRank(
    CSMessageEntry& entry) {
  MessageResponseGetWorldBossDamageRank res;
  res.mutable_uk_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_DAMAGE_UK).data());
  res.mutable_us_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_DAMAGE_US).data());
  res.mutable_ge_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_DAMAGE_GE).data());
  res.mutable_jp_list()->CopyFrom(
      RANK_LIST.GetByType(RANK_TYPE_WORLD_BOSS_DAMAGE_JP).data());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_WORLD_BOSS_DAMAGE_RANK, &res);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestEliteRandomCopyFight(CSMessageEntry& entry) {
  MessageRequestEliteRandomCopyFight* message =
      static_cast<MessageRequestEliteRandomCopyFight*>(entry.get());
  if (!message) return ERR_INTERNAL;
  sy::EliteRandomCopyStatus* status = NULL;
  for (int32_t i = 0; i < this->elite_random_copy_.copys_size(); ++i) {
    if (this->elite_random_copy_.copys(i).chapter() == message->chapter()) {
      status = this->elite_random_copy_.mutable_copys(i);
      break;
    }
  }
  if (!status) {
    DEBUG_LOG(logger)("ProcessRequestEliteRandomCopyFight, PlayerID:%ld, Chapter:%d"
        , this->uid(), message->chapter());
    return ERR_COPY_INVALID;
  }
  if (status->passed()) return ERR_COPY_PASSED;
  const EliteCopyBase* base = ELITEC_COPY_BASE.GetEntryByID(status->chapter() * 10 + status->quality()).get();
  if (!base) return ERR_PARAM_INVALID;
  if (this->player_.oil() < base->power) return ERR_CURRENCY_OIL;

  const CopyBase* copy_base = COPY_BASE.GetEntryByID(63100).get();
  if (!copy_base) return ERR_INTERNAL;
  const MonsterGroupBase* monster = MONSTER_GROUP_BASE.GetEntryByID(base->monster).get();
  if (!monster) return ERR_INTERNAL;

  PK pk(copy_base);
  pk.InitPlayer(this, true);
  pk.InitMonsterGroup(monster, false, false);
  pk.GeneratePVPReport();

  MessageNotifyFightResult notify;
  notify.set_star(pk.star);
  notify.set_win(pk.star ? 1 : 0);
  notify.set_copy_id(copy_base->id());
  notify.set_round(pk.round);

  if (pk.star) {

    AddSubItemSet item_set;
    ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_COPY);

    modify[MONEY_KIND_OIL] -= base->power;
    int32_t add_exp = 0;
    int32_t add_money = 0;
    this->FillCopyExpAndMoney(base->power, 0, add_exp, add_money);
    modify[MONEY_KIND_EXP] += add_exp;
    modify[MONEY_KIND_COIN] += add_money;
    item_set.push_back(ItemParam(23900017, RandomBetween(base->item1, base->item2)));

    CHECK_RET(this->CheckCurrency(modify));
    CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

    FillFightResultMessage(notify, item_set, modify);
    this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_COPY);
    this->UpdateCurrency(modify);

    status->set_passed(1);
    this->UpdateEliteRandomCopy();

    UpdateAchievement(
        ACHIEVEMENT_TYPE_COUNT_HARD_ENEMY,
        this->achievements_[ACHIEVEMENT_TYPE_COUNT_HARD_ENEMY] + 1);
    UpdateBuyCount(COUNT_TYPE_DAILY_HARD_ENEMY, 1);
  }

  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
  server->UpdateCopyStatistics(copy_base->type, copy_base->id(), this->uid());
  return ERR_OK;
}

void LogicPlayer::LoadEliteRandomCopy() {
  const std::string& key = MakeKVStorageKey(kKVPrefixPlayer, kKVTypeEliteCopy, this->uid());
  const std::string& value = server->GetKeyValue(key);
  this->elite_random_copy_.Clear();
  if (!value.empty()) {
    this->elite_random_copy_.ParseFromString(value);
  }
  TRACE_LOG(logger)("LoadEliteRandomCopy PlayerID:%ld, FreshTime:%ld"
      , this->uid(), this->elite_random_copy_.last_update_time());
}

//获取全三星的章节
static inline void FillElitePassedCopy(
    const VectorMap<int32_t, int32_t>& copy_star, int32_t chapter,
    VectorSet<int32_t>& passed_copy) {
  const CopyChapterBase* chapter_base = COPY_CHAPTER_BASE.GetEntryByID(chapter).get();
  bool all_3_star = true;
  while (chapter_base) {
    all_3_star = true;
    for (size_t gate_index = 0; gate_index < chapter_base->levels.size();
         ++gate_index) {
      const CopyGateBasePtr& gate = chapter_base->levels[gate_index];
      for (size_t copy_index = 0; copy_index < gate->copys.size();
           ++copy_index) {
        int32_t copy_id = gate->copys[copy_index] ? gate->copys[copy_index]->id() : 0;
        if (!copy_id) {
          all_3_star = false;
          break;
        }
        VectorMap<int32_t, int32_t>::const_iterator iter = copy_star.find(copy_id);
        if (iter == copy_star.end() || iter->second < 3) {
          all_3_star = false;
          break;
        }
      }
      if (!all_3_star) break;
    }
    if (all_3_star) {
      passed_copy.insert(chapter_base->id());
    }
    chapter_base = COPY_CHAPTER_BASE.GetEntryByID(chapter_base->last_chapter).get();
  }
}

void LogicPlayer::RefreshEliteRandomCopy() {
  if (this->level() < 50) return;

  tm last;
  int64_t time_sec = this->elite_random_copy_.last_update_time();
  localtime_r(&time_sec, &last);
  tm now = GetTime();
  if (now.tm_yday == last.tm_yday && now.tm_hour == last.tm_hour) return;

  int32_t begin_hour = now.tm_yday != last.tm_yday ? 0 : last.tm_hour;
  int32_t change = 0;

  if (!begin_hour) {
    ++change;
    this->elite_random_copy_.clear_copys();
  }
  boost::shared_ptr<VectorSet<int32_t> > left_copy;
  if (begin_hour <= 6 && 6 <= now.tm_hour) {
    ++change;
    this->FreshEliteHourCopy(6, left_copy);
  }
  if (begin_hour <= 12 && 12 <= now.tm_hour) {
    ++change;
    this->FreshEliteHourCopy(12, left_copy);
  }
  if (begin_hour <= 18 && 18 <= now.tm_hour) {
    ++change;
    this->FreshEliteHourCopy(18, left_copy);
  }

  if (change) {
    DEBUG_LOG(logger)("RefreshEliteRandomCopy PlayerID:%ld, TimeSec:%ld", this->uid(), time_sec);
    this->elite_random_copy_.set_last_update_time(GetSeconds());
    this->UpdateEliteRandomCopy();
  }
}

void LogicPlayer::FreshEliteHourCopy(
    int32_t hour, boost::shared_ptr<VectorSet<int32_t> >& left_copy) {
  if (!left_copy) {
    left_copy.reset(new VectorSet<int32_t>());
    sy::CopyProgress* progress = this->GetCopyProgress(COPY_TYPE_HARD);
    if (progress) FillElitePassedCopy(this->copy_star_, progress->chapter(), *left_copy);
  }

  if (left_copy->empty()) return;
  const EliteCopyRefreshBase* base = GetEliteCopyRefreshByLevel(this->level());
  if (!base) {
    DEBUG_LOG(logger)("FreshEliteHourCopy Error, PlayerID:%ld, Level:%d", this->uid(), this->level());
    return;
  }
  ValuePair2<int32_t, int32_t> limit = base->GetHourLimit(hour);

  DEBUG_LOG(logger)("FreshEliteHourCopy PlayerID:%ld, Hour:%d, LeftCopyCount:%lu, EliteCopyCount:%d, Limit:%d-%d"
      , this->uid(), hour, left_copy->size(), this->elite_random_copy_.copys_size()
      , limit.v1, limit.v2);

  if (this->elite_random_copy_.copys_size() >= limit.v2) return;
  for (int32_t i = 0; i < this->elite_random_copy_.copys_size(); ++i) {
    left_copy->erase(this->elite_random_copy_.copys(i).chapter());
  }
  if (left_copy->empty()) {
    DEBUG_LOG(logger)("FreshEliteHourCopy, PlayerID:%ld, NoMoreCopyLeft", this->uid());
    return;
  }
  for (int32_t i = 0; i < limit.v1; ++i) {
    if (left_copy->empty()) break;
    int32_t quality = RandomBetween(3, 5);
    int32_t random_chapter = RandomBetween(0, left_copy->size() - 1);
    random_chapter = *(left_copy->begin() + random_chapter);
    left_copy->erase(random_chapter);

    sy::EliteRandomCopyStatus* status = this->elite_random_copy_.add_copys();
    status->set_passed(0);
    status->set_chapter(random_chapter);
    status->set_quality(quality);
    DEBUG_LOG(logger)("FreshEliteHourCopy, PlayerID:%ld, Chapter:%d, Quality:%d"
        , this->uid(), random_chapter, quality);
  }
}

void LogicPlayer::UpdateEliteRandomCopy() {
  this->SendMessageToClient(MSG_CS_NOTIFY_ELITE_RANDOM_COPY, &this->elite_random_copy_);
  const std::string& key = MakeKVStorageKey(kKVPrefixPlayer, kKVTypeEliteCopy, this->uid());
  const std::string& value = this->elite_random_copy_.SerializeAsString();
  server->SetKeyValue(key, value);

  TRACE_LOG(logger)("SaveEliteRandomCopy PlayerID:%ld, FreshTime:%ld, CopyCount:%d"
      , this->uid(), this->elite_random_copy_.last_update_time()
      , this->elite_random_copy_.copys_size());
}

int32_t LogicPlayer::OnDailyRechargeItem(int32_t time, const std::string& goodid,
                            int32_t money, int32_t msgid) {
  int32_t temp = this->daily_counter_[COUNT_TYPE_DAILY_RECHARGE_ITEM];
  const DailyRechargeItemBase* base = GetDailyRechargeItemByGoodID(goodid);
  int32_t result = 0;
  ModifyCurrency modify(msgid, SYSTEM_ID_RECHARGE);
  AddSubItemSet item_set;
  do {
    if (!base) {
      ERROR_LOG(logger)("OnDailyRechargeItem, PlayerID:%ld, GoodID:%s, DailyRechargeItemBase not found"
        , this->uid(), goodid.c_str());
      result = -1;
      break;
    }
    INFO_LOG(logger)("OnDailyRechargeItem, PlayerID:%ld, GoodID:%s, BaseID:%ld, Status:%d"
        , this->uid(), goodid.c_str(), base->id(), temp);
    if (temp & (1 << base->id())) {
      result = -2;
      break;
    }
    const LootBasePtr& loot = LootConfigFile::Get(base->drop_box, this->create_days());
    if (loot) {
      loot->Loot(modify, item_set, NULL);
    }
    result = this->CheckCurrency(modify);
    if (result) break;
    result = this->CheckItem(&item_set, NULL, NULL);
    if (result) break;
  } while (false);

  if (!result) {
    this->UpdateCurrency(modify);
    this->ObtainItem(&item_set, NULL, NULL, msgid, SYSTEM_ID_RECHARGE);
    temp |= (1 << base->id());
    this->daily_counter_[COUNT_TYPE_DAILY_RECHARGE_ITEM] = temp;
    this->UpdateBuyCount(COUNT_TYPE_DAILY_RECHARGE_ITEM, 0);
  } else {
    ERROR_LOG(logger)("OnDailyRechargeItem, PlayerID:%ld, GoodID:%s, ErroCode:%d"
        , this->uid(), goodid.c_str(), result);
  }
  return ERR_OK;
}

int32_t LogicPlayer::OnRechargeItem(int32_t time, const std::string& goodid,
                                    int32_t money, int32_t msgid) {
  if (goodid.find("item_1_") == 0) this->RechargeActivity1(time, goodid, money, msgid);
  if (goodid.find("item_2_") == 0) this->OnRecharge1RMB(time, goodid, money, msgid);
  if (goodid.find("item_3_") == 0) this->OnDailyRechargeItem(time, goodid, money, msgid);

  this->AddRechargeInfo(time, money * 10, 0);
  return ERR_OK;
}

int32_t LogicPlayer::RechargeActivity1(int32_t time, const std::string& goodid,
                                       int32_t money, int32_t msgid) {
  int32_t result = 0;
  TRACE_LOG(logger)("RechargeActivity1 PlayerID:%ld, Time:%d, Money:%d, MSGID:0x%04X, goodid:%s"
              , this->uid(), time, money, msgid, goodid.c_str());
  TimeActivityType activity_type = TIME_ACTIVITY_RECHARGE;
  TimeActivityNew* base = ACTIVITY.GetAs(activity_type);
  if (!base) return ERR_OK;
  time_t now = GetVirtualSeconds();
  if (now < base->begin_time() || now > base->end_time()) return ERR_OK;
  for (int32_t i = 0; i < base->row_count(); i++) {
    const ActivityRowNew* row = base->row(i);
    if (!row) return ERR_OK;
    if (row->GetString("goodid") == goodid) {
      int32_t row_id = row->GetInt32("id");
      if (this->GetActivityRecordNew(activity_type, base->id(), row_id))
        return ERR_OK;
      this->SetActivityRecordNew(activity_type, base->id(), row_id, 1);

      ModifyCurrency modify(msgid, SYSTEM_ID_RECHARGE);
      AddSubItemSet item_set;
      FillCurrencyAndItem<kAdd>(modify, item_set, row->GetAward());

      result = this->CheckCurrency(modify);
      if (result) break;
      result = this->CheckItem(&item_set, NULL, NULL);
      if (result) break;

      UpdateCurrency(modify);
      ObtainItem(&item_set, NULL, NULL, msgid, SYSTEM_ID_RECHARGE);

      MessageResponseGetTimeActivityAward res;
      res.set_activiy_type(activity_type);
      res.set_id(row_id);
      this->SendMessageToClient(MSG_CS_RESPONSE_GET_TIME_ACTIVITY_AWARD, &res);

      break;
    }
  }

  player().set_total_recharge(player().total_recharge() + money * 10);
  MessageSSUpdateTotalRecharge update;
  update.set_total_recharge(player().total_recharge());
  this->SendMessageToDB(MSG_SS_UPDATE_TOTAL_RECHARGE, &update);

  if (result)
    TRACE_LOG(logger)("RechargeActivity1 Fail PlayerID:%ld, ResultID: %d", this->uid(), result);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestRefreshDiamondShop(CSMessageEntry& entry) {
  RefreshShopInfo& info = this->refresh_shop_info_[10];

  if (IsSameDay(info.last_time(), GetVirtualSeconds())) {
    int32_t last_refresh_hour = 0;
    const std::vector<int32_t>& refresh_time =
        Setting::GetValue1(Setting::shop10_refresh_time);
    for (std::vector<int32_t>::const_iterator it = refresh_time.begin();
         it != refresh_time.end(); ++it)
      if (*it <= GetTime().tm_hour)
        last_refresh_hour = last_refresh_hour > *it ? last_refresh_hour : *it;
    tm time_tm;
    time_t temp_t = info.last_time();
    localtime_r(&temp_t, &time_tm);
    if (time_tm.tm_hour >= last_refresh_hour) return ERR_PARAM_INVALID;
  }

  ShopBase::RandomFeatsCommodity(10, this->level(),
                                 info.mutable_feats_commodity(),
                                 server->GetServerStartDays());
  info.set_last_time(GetVirtualSeconds());
  UpdateFeatsShopInfo(10);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestMakeWakeItem(CSMessageEntry& entry) {
  MessageRequestMakeWakeItem* message =
      static_cast<MessageRequestMakeWakeItem*>(entry.get());
  if (!message) return ERR_INTERNAL;

  WakeItemBase* base = WAKE_ITEM_BASE.GetEntryByID(message->item_id()).get();
  if (!base) return ERR_PARAM_INVALID;
  if (base->cost.empty()) return ERR_PARAM_INVALID;
  ModifyCurrency modify(MSG_CS_REQUEST_MAKE_WAKE_ITEM, SYSTEM_ID_WAKE);
  AddSubItemSet item_set;
  for (int32_t i = 0; i < message->count(); ++i) {
    FillCurrencyAndItem<kSub>(modify, item_set, base->cost);
    modify[MONEY_KIND_COIN] -= base->cost_money;
  }
  item_set.push_back(ItemParam(base->id(), message->count()));
  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_MAKE_WAKE_ITEM,
             SYSTEM_ID_WAKE);

  MessageResponseMakeWakeItem res;
  KVPair2* pair = res.add_items();
  pair->set_key(base->id());
  pair->set_value(message->count());
  this->SendMessageToClient(MSG_CS_RESPONSE_MAKE_WAKE_ITEM, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestRecoverWakeItem(CSMessageEntry& entry) {
  MessageRequestRecoverWakeItem* message =
      static_cast<MessageRequestRecoverWakeItem*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageResponseRecoverWakeItem res;

  WakeItemBase* base = WAKE_ITEM_BASE.GetEntryByID(message->item_id()).get();
  ItemBase* item = ITEM_BASE.GetEntryByID(message->item_id()).get();
  if (!item) return ERR_ITEM_NOT_FOUND;
  if (!base) return ERR_PARAM_INVALID;
  ModifyCurrency modify(MSG_CS_REQUEST_RECOVER_WAKE_ITEM, SYSTEM_ID_WAKE);
  AddSubItemSet item_set;
  for (int32_t i = 0; i < message->count(); ++i) {
    FillCurrencyAndItem<kAdd>(modify, item_set, item->sell);
  }
  FillToKVPair2(&modify, &item_set, res.mutable_items());
  item_set.push_back(ItemParam(base->id(), -message->count()));

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_RECOVER_WAKE_ITEM,
             SYSTEM_ID_WAKE);

  this->SendMessageToClient(MSG_CS_RESPONSE_RECOVER_WAKE_ITEM, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestEquipWakeItem(CSMessageEntry& entry) {
  MessageRequestEquipWakeItem* message =
      static_cast<MessageRequestEquipWakeItem*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicHero* hero = this->GetHeroByUID(message->hero_uid());
  if (!hero) return ERR_HERO_NOT_FOUND;
  HeroBase* base = HERO_BASE.GetEntryByID(hero->first.hero_id()).get();
  if (!base) return ERR_HERO_NOT_FOUND;

  WakeConditionBase* wake_condition =
      WAKE_CONDITION_BASE.GetEntryByID(hero->first.wake_level()).get();
  if (!wake_condition) return ERR_PARAM_INVALID;

  const std::vector<int32_t>* wake_item_vct =
      GetWakeItemTree(base->wake_itemtree, wake_condition);
  if (!wake_item_vct) return ERR_INTERNAL;

  AddSubItemSet item_set;
  item_set.push_back(ItemParam(message->item_id(), -1));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  bool flag = false;
  for (size_t i = 0; i < wake_item_vct->size(); i++) {
    if (wake_item_vct->at(i) == message->item_id()) {
      hero->first.mutable_wake_item()->Resize(4, 0);
      hero->first.set_wake_item(i, message->item_id());
      flag = true;
    }
  }
  if (!flag) return ERR_PARAM_INVALID;

  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_EQUIP_WAKE_ITEM,
             SYSTEM_ID_WAKE);

  if (this->IsInTactic(hero->first.uid())) {
    this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_WAKE, entry.head.msgid);
  } else {
    MessageNotifyHeroInfo notify;
    notify.add_info()->CopyFrom(hero->first);
    this->SendMessageToClient(MSG_CS_NOTIFY_HERO_INFO, &notify);
  }

  MessageSSUpdateHeroInfo update;
  update.add_info()->CopyFrom(hero->first);
  update.set_tid(server->GetTID());
  update.set_system(SYSTEM_ID_WAKE);
  update.set_msgid(MSG_CS_REQUEST_EQUIP_WAKE_ITEM);
  this->SendMessageToDB(MSG_SS_UPDATE_HERO_INFO, &update);

  MessageResponseEquipWakeItem res;
  res.set_hero_uid(message->hero_uid());
  res.set_item_id(message->item_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_EQUIP_WAKE_ITEM, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestShipWake(CSMessageEntry& entry) {
  MessageRequestShipWake* message =
      static_cast<MessageRequestShipWake*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicHero* hero = this->GetHeroByUID(message->hero_uid());
  if (!hero) return ERR_HERO_NOT_FOUND;
  HeroBase* base = HERO_BASE.GetEntryByID(hero->first.hero_id()).get();
  if (!base) return ERR_HERO_NOT_FOUND;

  WakeConditionBase* wake_condition =
      WAKE_CONDITION_BASE.GetEntryByID(hero->first.wake_level()).get();
  if (!wake_condition) return ERR_PARAM_INVALID;

  if (wake_condition->need_lv > this->level()) return ERR_HERO_NEED_LEVEL_UP;

  AddSubItemSet item_set;
  ModifyCurrency modify(MSG_CS_REQUEST_SHIP_WAKE, SYSTEM_ID_WAKE);
  item_set.push_back(ItemParam(23900017, -wake_condition->need_item));
  modify[MONEY_KIND_COIN] -= wake_condition->need_coin;
  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  const std::vector<int32_t>* wake_item_vct =
      GetWakeItemTree(base->wake_itemtree, wake_condition);
  if (!wake_item_vct) return ERR_INTERNAL;
  bool flag = true;
  for (size_t i = 0; i < wake_item_vct->size(); i++) {
    if (wake_item_vct->at(i) != hero->first.wake_item(i)) {
      flag = false;
      break;
    }
  }
  if (!flag) return ERR_PARAM_INVALID;

  NotifyHeroSet notify_set;
  notify_set.push_back(hero->first.uid());
  DeleteHeroSet delete_heros;
  std::sort(message->mutable_other_hero_uid()->begin(),
            message->mutable_other_hero_uid()->end());
  int64_t* end = std::unique(message->mutable_other_hero_uid()->begin(),
                             message->mutable_other_hero_uid()->end());
  for (int64_t* iter = message->mutable_other_hero_uid()->begin(); iter != end;
       ++iter) {
    LogicHero* other = this->GetHeroByUID(*iter);
    if (!other) return ERR_HERO_NOT_FOUND;
    if (other == hero) return ERR_PARAM_INVALID;
    if (other->first.hero_id() != base->id()) return ERR_PARAM_INVALID;

    if (other->first.level() > 1) return ERR_PARAM_INVALID;
    if (other->first.fate_level() > 1) return ERR_PARAM_INVALID;
    if (other->first.train_cost() > 0) return ERR_PARAM_INVALID;
    if (other->first.grade() > 0) return ERR_PARAM_INVALID;
    if (other->first.wake_level() > 0) return ERR_PARAM_INVALID;

    if (delete_heros.full()) break;
    delete_heros.push_back(*iter);
  }
  if (int32_t(delete_heros.size()) != wake_condition->need_hero)
    return ERR_HERO_NEED_MORE;
  CHECK_RET(this->CheckHero(&delete_heros, &notify_set));

  hero->first.mutable_wake_item()->Resize(0, 0);
  hero->first.mutable_wake_item()->Resize(4, 0);
  hero->first.set_wake_level(hero->first.wake_level() + 1);

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_SHIP_WAKE,
                   SYSTEM_ID_WAKE);
  if (this->IsInTactic(hero->first.uid())) {
    this->ObtainHero(&delete_heros, NULL, SYSTEM_ID_WAKE,
                     MSG_CS_REQUEST_SHIP_WAKE);
    this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_WAKE, entry.head.msgid);
  } else {
    this->ObtainHero(&delete_heros, &notify_set, SYSTEM_ID_WAKE,
                     MSG_CS_REQUEST_SHIP_WAKE);
  }

  MessageResponseShipWake res;
  res.set_hero_uid(message->hero_uid());
  this->SendMessageToClient(MSG_CS_RESPONSE_SHIP_WAKE, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetLegionWarLog(CSMessageEntry& entry) {
  MessageResponseGetLegionWarLog response;
  const std::string& key = MakeKVStorageKey(KVStorage::kKVPrefixLegionWar,
                                            KVStorage::kKVTypeLOG, this->uid());
  const std::string& value = storage::Get(key);
  response.ParseFromString(value);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_LEGION_WAR_LOG, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSetFocusCity(CSMessageEntry& entry) {
  MessageRequestSetFocusCity* message =
      static_cast<MessageRequestSetFocusCity*>(entry.get());
  if (!message) return ERR_INTERNAL;
  Army* army = server->GetArmyByID(this->army_id());
  if (!army) return ERR_ARMY_NOT_JOINED;
  LegionWar::Instance().SetArmyFocusCity(army->army_id(), message->city_id());

  MessageNotifyLegionWarWarArmyInfo notify;
  notify.set_focus_city(message->city_id());

  army->SendMessageToArmy(MSG_CS_NOTIFY_LEGION_WAR_ARMY_INFO, &notify);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetLegionWarTargetAward(
    CSMessageEntry& entry) {
  if (!IsInLegionWarOpenDays()) return ERR_ACTIVITY_NOT_OPEN;
  if (!this->daily_counter_[sy::COUNT_TYPE_LEGION_WAR_TARGET])
    return ERR_PARAM_INVALID;
  int32_t left_award_count =
      1 + this->daily_counter_[COUNT_TYPE_LEGION_WAR_TARGET_BUY] -
      this->daily_counter_[COUNT_TYPE_LEGION_WAR_TARGET_AWARD];
  if (left_award_count <= 0) return ERR_DEBUG_LEGION_WAR_TARGET_COUNT;

  int64_t target = this->daily_counter_[sy::COUNT_TYPE_LEGION_WAR_TARGET];
  sy::OtherPlayerInfo* player = LegionWar::Instance().GetPlayerByID(target);
  sy::OtherPlayerInfo* this_player = LegionWar::Instance().GetPlayerByID(this->uid());
  if (!player || !this_player) {
    ERROR_LOG(logger)("GetLegionWarTargetAward, PlayerID:%ld not found", target);
    return ERR_INTERNAL;
  }
  const LegionWarTargetBase* base = GetLegionWarTargetBaseByLevel(this->level()).get();
  if (!base) {
    DEBUG_LOG(logger)("LegionWarTargetBase:%d not found", this->level());
    return ERR_INTERNAL;
  }

  int64_t this_fight = this->max_fight_attr();
  int64_t target_fight = 0;
  for (int32_t index = 0; index < player->heros_size(); ++index) {
    target_fight += player->heros(index).attr1(0);
  }

  //23900063
  int32_t award_1 =
      (base->reward_a * target_fight / this_fight + base->reward_b) *
      this->level() / 100;

  if (award_1 >= GetSettingValue(legion_war_region_award1))
    award_1 = GetSettingValue(legion_war_region_award1);

  AddSubItemSet item_set;
  item_set.push_back(ItemParam(23900063, award_1));

  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  this->ObtainItem(&item_set, NULL, NULL, SYSTEM_ID_LEGION_WAR,
                   entry.head.msgid);
  int32_t award_2 =
      (base->reward_c * target_fight / this_fight + base->reward_d) *
      this->level() / 100;
  if (award_2 >= GetSettingValue(legion_war_region_award2))
    award_2 = GetSettingValue(legion_war_region_award2);
  DEBUG_LOG(logger)("GetLegionWarTargetAward, PlayerID:%ld, Fight:%ld, TargetPlayer:%ld, TargetFight:%ld, Award1:%d, Award2:%d"
      , this->uid(), this_fight
      , player->player_id(), target_fight
      , award_1, award_2);
  MessageRespopnseGetLegionWarTargetAward response;
  if (award_2) {
    //更新排行榜
    int32_t score = LegionWar::Instance().AddPlayerScore(this->uid(), award_2);
    sy::RankItemInfo info;
    RANK_LIST.OnLegionWar1(this, score, info);
    if (score >= GetSettingValue(legion_war_region_score1)) {
      server->SendCrossServerRankItem(info, sy::RANK_TYPE_LEGION_WAR_2);
    }
    response.set_score(score);
    VectorSet<int64_t> set;
    Army* army = server->GetArmyByID(this->army_id());
    if (army) {
      set.insert(this->army_id());
      int32_t army_score = LegionWar::Instance().CalcArmyScore(set);

      MessageNotifyLegionWarWarArmyInfo notify;
      notify.set_army_score(army_score);
      army->SendMessageToArmy(MSG_CS_NOTIFY_LEGION_WAR_ARMY_INFO, &notify);
    }
  }

  this->daily_counter_[sy::COUNT_TYPE_LEGION_WAR_TARGET] = 0;
  this->UpdateBuyCount(sy::COUNT_TYPE_LEGION_WAR_TARGET, 0);
  this->UpdateBuyCount(COUNT_TYPE_LEGION_WAR_TARGET_AWARD, 1);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_LEGION_WAR_TARGET_AWARD, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetLegionWarTarget(CSMessageEntry& entry) {
  MessageRequestGetLegionWarTarget* message = static_cast<MessageRequestGetLegionWarTarget*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!IsInLegionWarOpenDays()) return ERR_ACTIVITY_NOT_OPEN;
  int64_t target = LegionWar::Instance().GetPlayerTarget(this->uid(), false);
  if (message->use_money()) {
    target = LegionWar::Instance().GetPlayerTarget(this->uid(), true);
    this->daily_counter_[COUNT_TYPE_LEGION_WAR_TARGET] = 0;
    this->UpdateBuyCount(COUNT_TYPE_LEGION_WAR_TARGET, 0);
  }

  MessageResponseGetLegionWarTarget response;
  response.set_player_id(target);
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_LEGION_WAR_TARGET, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetLegionWarInfo(CSMessageEntry& entry) {
  LegionWar::Instance().SendCitiesToPlayer(this);
  LegionWar::Instance().Subscribe(this);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLegionWarFight(CSMessageEntry& entry) {
  MessageRequestLegionWarFight* message =
      static_cast<MessageRequestLegionWarFight*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (!IsInLegionWarOpenDays()) return ERR_ACTIVITY_NOT_OPEN;
  std::pair<int32_t, int32_t> player_pos = LegionWar::Instance().GetPlayerPos(this->uid());
  if (!player_pos.first) {
    this->SendMessageToSelf(MSG_CS_REQUEST_LEGION_WAR_REGISTER, kEmptyMessage);
    ERROR_LOG(logger)("RequestLegionWarFight PlayerID:%ld, does not register"
        , this->uid());
    return ERR_OK;
  }

  const LegionCityBase* city =
      LEGION_CITY_BASE.GetEntryByID(player_pos.first).get();
  if (!city) return ERR_DEBUG_LEGION_WAR_CITY_NOT_FOUND;
  if (std::find(city->nearby.begin(), city->nearby.end(), message->city()) ==
      city->nearby.end()) {
    DEBUG_LOG(logger)("MyPos:(%d-%d), RequestPos:(%d-%d)"
        , player_pos.first, player_pos.second
        , message->city(), message->position()
        );
    return ERR_DEBUG_LEGION_WAR_IMPASSABLE;
  }

  const std::vector<int32_t>& army_score = GetLegionPersonalAward(city->quality);
  if (message->position() > (int32_t)army_score.size())
    return ERR_DEBUG_LEGION_WAR_POSITION;

  sy::OtherPlayerInfo* target = LegionWar::Instance().GetOtherPlayerByPos(
      std::make_pair(message->city(), message->position()));
  const CopyBase* base = COPY_BASE.GetEntryByID(63400).get();
  if (!base) return ERR_INTERNAL;
  const MonsterGroupBase* monster =
      MONSTER_GROUP_BASE.GetEntryByID(GetSettingValue(legion_war_monstergroup))
          .get();
  if (!monster) return ERR_INTERNAL;

  LegionWar::Instance().Subscribe(this);
  PK pk(base);
  pk.InitPlayer(this, true, NULL, 1.0f);
  if (target) {
    pk.InitPlayer(target, false, 1.0, 1.0, 1.0, 1.0);
  } else {
    pk.InitMonsterGroup(monster, false, false);
  }
  pk.GeneratePVPReport();

  MessageNotifyFightResult notify;
  notify.set_win((bool)pk.star);
  notify.set_star(pk.star);
  //战胜
  if (pk.star) {
    notify.set_new_city(message->city());
    notify.set_new_position(message->position());

    MessageSSUpdateLegionWarPos update;
    update.set_city_1(player_pos.first);
    update.set_position_1(player_pos.second);
    update.set_city_2(message->city());
    update.set_position_2(message->position());
    update.set_server_id(server_config->server_id());
    server->SendMessageToCenter(MSG_SS_UPDATE_LEGION_WAR_POS, &update);
    this->UpdateBuyCount(COUNT_TYPE_LEGION_WAR, 1);

    this->OnLegionWarWin(message->city(), message->position());
  }

  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);
  this->SendMessageToClient(MSG_CS_RESPONSE_LEGION_WAR_FIGHT, NULL);
  return ERR_OK;
}

void LogicPlayer::OnLegionWarWin(int32_t city, int32_t pos) {
  sy::OtherPlayerInfo* target =
      LegionWar::Instance().GetOtherPlayerByPos(std::make_pair(city, pos));
  if (!target) return;
  int64_t target1 = LegionWar::Instance().GetPlayerTarget(this->uid(), false);
  if (target1 == target->player_id()) {
    this->daily_counter_[sy::COUNT_TYPE_LEGION_WAR_TARGET] = target1;
    this->UpdateBuyCount(sy::COUNT_TYPE_LEGION_WAR_TARGET, 0);
  }
}

int32_t LogicPlayer::ProcessRequestGetLegionWarReward(CSMessageEntry& entry) {
  MessageRequestGetLegionWarReward* message = static_cast<MessageRequestGetLegionWarReward*>(entry.get());
  if (!message || message->index() < 0 || message->index() > 31)
    return ERR_INTERNAL;
  if (!IsInLegionWarOpenDays()) return ERR_ACTIVITY_NOT_OPEN;
  int32_t status = this->daily_counter_[sy::COUNT_TYPE_LEGION_WAR_REWARD];
  const LegionDailyRewardBase* base = LEGION_DAILY_REWARD_BASE.GetEntryByID(message->index()).get();
  if (!base) return ERR_PARAM_INVALID;
  if (status & (1 << message->index())) return ERR_DEBUG_LEGION_DAILY_REWARDED;
  status |= 1 << message->index();
  this->daily_counter_[sy::COUNT_TYPE_LEGION_WAR_REWARD] = status;
  this->UpdateBuyCount(sy::COUNT_TYPE_LEGION_WAR_REWARD, 0);

  AddSubItemSet item_set;
  item_set.push_back(ItemParam(23900063, base->money));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_LEGION_WAR);

  MessageResponseGetLegionWarReward response;
  response.set_index(message->index());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_LEGION_WAR_REWARD, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetLegionWarPlayer(CSMessageEntry& entry) {
  LegionWar::Instance().SendPlayersToPlayer(this);
  LegionWar::Instance().Subscribe(this);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLegionWarRegister(CSMessageEntry& entry) {
#ifdef DEBUG
  if (LegionWar::Instance().GetPlayerByID(this->uid()) &&
      !LegionWar::Instance().GetPlayerPos(this->uid()).second) {
    const std::string& pos = LegionWar::Instance().dump();
    ERROR_LOG(logger)("LegionWar Register, CurrentPos:\n%s", pos.c_str());
    logger->Flush();
  }
#endif
  MessageSSRequestLegionWarNewPlayer request;
  request.set_server_id(server_config->server_id());
  sy::OtherPlayerInfo* player = request.mutable_player();
  this->FillOtherPlayerInfo(player);
  server->SendMessageToCenter(MSG_SS_REQUEST_LEGION_WAR_NEW_PLAYER, &request);
  LegionWar::Instance().Subscribe(this);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSetUserDefined(CSMessageEntry& entry) {
  MessageRequestSetUserDefined* message =
      static_cast<MessageRequestSetUserDefined*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->content().size() > 8192) return ERR_PARAM_INVALID;

  this->user_defined_ = message->content();
  server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypeUserDefined,
                            this->user_defined_);
  this->SendMessageToClient(MSG_CS_RESPONSE_SET_USER_DEFINED, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetTimeActivityAwardNew(
    CSMessageEntry& entry) {
  MessageRequestGetTimeActivityAward* message =
      static_cast<MessageRequestGetTimeActivityAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  TimeActivityType activity_type = (TimeActivityType)message->activiy_type();
  TimeActivityNew* base = ACTIVITY.GetAs(activity_type);
  if (!base) return ERR_PARAM_INVALID;

  int32_t key =
      TIME_ACTIVITY_FESTIVAL_RECHARGE == base->type() ? message->id() : 0;
  int32_t ncount = this->GetActivityRecordNew(activity_type, base->id(), -key);

  for (int32_t i = 0; i < base->row_count(); i++) {
    const ActivityRowNew* row = base->row(i);
    if (!row) return ERR_PARAM_INVALID;
    if (row->GetInt32("id") == message->id()) {
      int32_t condition = row->GetInt32("count");
      if (!condition) return ERR_PARAM_INVALID;

      if (TIME_ACTIVITY_FESTIVAL_RECHARGE == base->type()) {
        int32_t got_count =
            this->GetActivityRecordNew(activity_type, base->id(), key);
        if (got_count >= condition) return ERR_PARAM_INVALID;
        if (ncount <= got_count) return ERR_PARAM_INVALID;
      } else if (TIME_ACTIVITY_FESTIVAL_LOGIN == base->type()) {
        if (!(ncount & (1 << (condition - 1)))) return ERR_PARAM_INVALID;
        if (this->GetActivityRecordNew(activity_type, base->id(),
                                       row->GetInt32("id")))
          return ERR_PARAM_INVALID;
      } else {
        if (ncount < condition) return ERR_PARAM_INVALID;
        if (this->GetActivityRecordNew(activity_type, base->id(),
                                       row->GetInt32("id")))
          return ERR_PARAM_INVALID;
      }

      this->AddActivityRecordNew(activity_type, base->id(), row->GetInt32("id"),
                                 1);

      ModifyCurrency modify(MSG_CS_REQUEST_GET_TIME_ACTIVITY_AWARD,
                            SYSTEM_ID_ACTIVITY);
      AddSubItemSet item_set;
      FillCurrencyAndItem<kAdd>(modify, item_set, row->GetAward());
      CHECK_RET(this->CheckCurrency(modify));
      CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
      UpdateCurrency(modify);
      ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_GET_TIME_ACTIVITY_AWARD,
                 SYSTEM_ID_ACTIVITY);
      MessageResponseGetTimeActivityAward res;
      res.set_activiy_type(activity_type);
      res.set_id(message->id());
      this->SendMessageToClient(MSG_CS_RESPONSE_GET_TIME_ACTIVITY_AWARD, &res);
      return ERR_OK;
    }
  }
  return ERR_PARAM_INVALID;
}

int32_t LogicPlayer::ProcessRequestAstrology(CSMessageEntry& entry) {
  MessageRequestAstrology* message =
      static_cast<MessageRequestAstrology*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!(message->type() == 1 || message->type() == 10))
    return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_ASTROLOGY);
  AddSubItemSet item_set;

  int32_t award = Setting::GetValueInVec2(Setting::hero_recruit_drop3,
                                          server->AstrologyAwardCountry());
  const LootBasePtr& loot =
      LootConfigFile::Get(GetSettingValue(hero_recruit_drop1), this->level());
  const LootBasePtr& loot_luck =
      LootConfigFile::Get(GetSettingValue(hero_recruit_drop2), this->level());
  const LootBasePtr& loot_award = LootConfigFile::Get(award, this->level());

  MessageResponseAstrology res;
  std::vector<int32_t> vct_index;
  for (int32_t i = 0; i < message->type(); i++) {
    if (loot) {
      vct_index.clear();
      loot->Loot(modify, item_set, &vct_index);
      FillToKVPair2L(loot.get(), vct_index, res.mutable_items());
    }
    if (loot_luck) {
      vct_index.clear();
      loot_luck->Loot(modify, item_set, &vct_index);
      FillToKVPair2L(loot_luck.get(), vct_index, res.mutable_items());
    }
    if (loot_award) {
      vct_index.clear();
      loot_award->Loot(modify, item_set,&vct_index);
      FillToKVPair2L(loot_award.get(), vct_index, res.mutable_items());
    }
  }

  bool is_free = false;
  if (1 == message->type()) {
    if (this->GetDailySign(ASTROLOGY_FREE_BUY))
      item_set.push_back(
          ItemParam(23900160, -GetSettingValue(hero_recruit_cost1)));
    else
      is_free = true;
  }
  if (10 == message->type()) {
    item_set.push_back(
        ItemParam(23900160, -GetSettingValue(hero_recruit_cost10)));
  }

  if (!is_free) {
    if (this->daily_counter_[COUNT_TYPE_ASTROLOGY] + message->type() >
        GetSettingValue(hero_recruit_everyday_times))
      return ERR_PARAM_INVALID;
  }

  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_ASTROLOGY);

  if (is_free)
    this->SetDailySign(ASTROLOGY_FREE_BUY, true);
  else
    UpdateBuyCount(COUNT_TYPE_ASTROLOGY, message->type());
  res.set_type(message->type());
  this->SendMessageToClient(MSG_CS_RESPONSE_ASTROLOGY, &res);

  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_ASTROLOGY, message->type(),
                                  this);
  ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_FESTIVAL_ASTROLOGY,
                                  message->type(), this);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestAstrologyChange(CSMessageEntry& entry) {
  MessageRequestAstrologyChange* message =
      static_cast<MessageRequestAstrologyChange*>(entry.get());
  if (!message) return ERR_INTERNAL;

  AddSubItemSet item_set;
  item_set.push_back(ItemParam(GetSettingValue(hero_recruit_itemid),
                               -GetSettingValue(hero_recruit_cost_item)));

  ModifyCurrency temp(entry.head.msgid, SYSTEM_ID_ASTROLOGY);
  AddSubItemSet temp_set;
  int32_t award = Setting::GetValueInVec2(Setting::hero_recruit_drop4,
                                          server->AstrologyAwardCountry());
  const LootBasePtr& loot = LootConfigFile::Get(award, this->level());
  std::vector<int32_t> vct;
  vct.push_back(message->hero_id());
  if (loot) loot->Loot(temp, temp_set, &vct);

  bool flag = false;
  for (size_t i = 0; i < temp_set.size(); i++) {
    if (temp_set[i].item_id == message->hero_id()) {
      flag = true;
      break;
    }
  }
  if (!flag) return ERR_PARAM_INVALID;

  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_ASTROLOGY);

  sy::HeroInfo info;
  info.set_hero_id(message->hero_id());
  this->UpdateHeroInfo(info, kNotifyAll, SYSTEM_ID_ASTROLOGY, entry.head.msgid);

  MessageResponseAstrologyChange res;
  res.set_hero_id(message->hero_id());
  this->SendMessageToClient(MSG_CS_RESPONSE_ASTROLOGY_CHANGE, &res);

  return ERR_OK;
}

void LogicPlayer::DeleteApply(int32_t player_id) {
  VectorSet<int64_t>& apply_army = server->applies(player_id);
  for (VectorSet<int64_t>::iterator iter = apply_army.begin();
       iter != apply_army.end(); ++iter) {
    Army* army_temp = server->GetArmyByID(*iter);
    if (army_temp) {
      army_temp->DeleteApply(player_id);
    }
  }
  apply_army.clear();
}

int32_t LogicPlayer::ProcessRequestResearchItem(CSMessageEntry& entry) {
  MessageRequestResearchItem* message =
      static_cast<MessageRequestResearchItem*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (GetVirtualSeconds() < Setting::kLimitedRecruitStarttime ||
      GetVirtualSeconds() > Setting::kLimitedRecruitEndtime)
    return ERR_PARAM_INVALID;

  int32_t type = message->type();
  if (!(type == 1 || type == 10)) return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_RESEARCH_ITEM);
  AddSubItemSet item_set;

  int32_t point = 0;
  if (type == 1) {
    item_set.push_back(
        ItemParam(23900160, -(GetSettingValue(limited_recruit_1gold))));
    point = GetSettingValue(limited_recruit_1score);
  }
  if (type == 10) {
    item_set.push_back(
        ItemParam(23900160, -(GetSettingValue(limited_recruit_10gold))));
    point = GetSettingValue(limited_recruit_10score);
  }

  MessageResponseResearchItem res;
  for (int32_t i = 0; i < type; ++i) {
    ModifyCurrency t_modify(entry.head.msgid, SYSTEM_ID_RESEARCH_ITEM);
    AddSubItemSet t_set;
    const LootBasePtr& loot = LootConfigFile::Get(
        GetSettingValue(limited_recruit_1drop), this->level());
    if (loot)
      loot->Loot(t_modify, t_set, NULL);
    else
      return ERR_PARAM_INVALID;
    FillToKVPair2(&t_modify, &t_set, res.mutable_awards());

    for (AddSubItemSet::iterator it = t_set.begin(); it != t_set.end(); ++it)
      item_set.push_back(ItemParam(it->item_id, it->item_count));
    for (int32_t i = 0; i < MoneyKind_ARRAYSIZE; ++i)
      if (t_modify[i]) modify[i] += t_modify[i];
  }
  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_RESEARCH_ITEM);

  UpdateAchievement(OTHER_RESEARCH_ITEM_POINT,
                    this->achievements_[OTHER_RESEARCH_ITEM_POINT] + point);

  if (10 == type) {
    UpdateAchievement(SEVEN_DAY_TYPE_SHIPBUILD,
                      this->achievements_[SEVEN_DAY_TYPE_SHIPBUILD] + 1);
  }

  res.set_point(this->achievements_[OTHER_RESEARCH_ITEM_POINT]);
  res.set_type(type);
  this->SendMessageToClient(MSG_CS_RESPONSE_RESEARCH_ITEM, &res);

  RANK_LIST.OnResearchItem(this);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetResearchItemAward(CSMessageEntry& entry) {
  MessageRequestGetResearchItemAward* message =
      static_cast<MessageRequestGetResearchItemAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LimitedRecruitBoxBase* base =
      LIMITED_RECRUIT_BOX_BASE.GetEntryByID(message->type()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (this->achievements_[OTHER_RESEARCH_ITEM_POINT] < base->score)
    return ERR_PARAM_INVALID;

  int32_t record_temp = this->achievements_[OTHER_RESEARCH_ITEM_RECORD];

  time_t refresh_time =
      atol(server->GetPlayerValue(this->uid(), kKVTypeResearchItem).c_str());
  if (refresh_time < Setting::kLimitedRecruitStarttime) {
    record_temp = 0;
    char buff[16] = {0};
    sprintf(buff, "%ld", GetVirtualSeconds());
    server->SetPlayerKeyValue(this->uid(), kKVTypeResearchItem, buff);
  }

  if (record_temp & (1 << (base->id() - 1))) return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_RESEARCH_ITEM);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_RESEARCH_ITEM);

  record_temp |= (1 << (base->id() - 1));
  UpdateAchievement(OTHER_RESEARCH_ITEM_RECORD, record_temp);

  MessageResponseGetResearchItemAward res;
  res.set_type(message->type());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_RESEARCH_ITEM_AWARD, &res);

  return ERR_OK;
}

void LogicPlayer::DispatchQueryPlayerMessage(
    MessageSSResponseQueryOtherPlayer& msg) {
  for (int32_t i = 0; i < msg.player_size(); i++) {
    LogicPlayer::ChangeCrossInfoPlayer(*msg.mutable_player(i));
  }
  switch (msg.msg_id()) {
    case MSG_CS_REQUEST_CROSS_SERVER_COUNTRY:
      ProcessResponseCrossServerCountry(msg);
      break;
    case MSG_CS_REQUEST_CROSS_SERVER_RANDOM_PLAYER:
      ProcessResponseCrossServerRandomPlayer(msg);
      break;
  }
}

int32_t LogicPlayer::ProcessResponseCrossServerCountry(
    MessageSSResponseQueryOtherPlayer& msg) {
  this->cross_server_info_.mutable_enemy()->CopyFrom(msg.player());
  server->SetPlayerKeyValue(this->uid(), kKVTypeCrossServer,
                            cross_server_info_.SerializeAsString());

  MessageResponseCrossServerCountry res;
  res.mutable_info()->CopyFrom(this->cross_server_info_);
  this->SendMessageToClient(MSG_CS_RESPONSE_CROSS_SERVER_COUNTRY, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestCrossServerCountry(CSMessageEntry& entry) {
  MessageRequestCrossServerCountry* message =
      static_cast<MessageRequestCrossServerCountry*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!LogicPlayer::IsCrossServerTime()) return ERR_PARAM_INVALID;
  if (cross_server_info_.country()) return ERR_PARAM_INVALID;
  if (message->country() < 1 || message->country() > 4)
    return ERR_PARAM_INVALID;

  CHECK_RET(CrossServerQueryPlayers(entry.head.msgid));

  cross_server_info_.set_country(message->country());

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestCrossServerRandomPlayer(
    CSMessageEntry& entry) {
  if (!LogicPlayer::IsCrossServerTime()) return ERR_PARAM_INVALID;
  if (this->daily_counter_[COUNT_TYPE_CROSS_SERVER_RANDOM] <
      GetSettingValue(crossserverpk1_refresh_count)) {
    CHECK_RET(CrossServerQueryPlayers(entry.head.msgid));
  } else {
    MessageRequestBuyCount* message = new MessageRequestBuyCount();
    message->set_buy_count(1);
    message->set_count_type(COUNT_TYPE_CROSS_BUY_SERVER_RANDOM);
    this->SendMessageToSelf(MSG_CS_REQUEST_BUY_COUNT,
                            boost::shared_ptr<Message>(message));
  }
  UpdateBuyCount(COUNT_TYPE_CROSS_SERVER_RANDOM, 1);

  return ERR_OK;
}

int32_t LogicPlayer::CrossServerQueryPlayers(int32_t msg_id) {
  CrossServerPK1RollPlayerBase* base =
      CROSS_SERVER_PK1_ROLL_PLAYER_BASE.GetEntryByID(this->level() / 10 * 10)
          .get();
  if (!base) return ERR_PARAM_INVALID;
  MessageSSRequestQueryOtherPlayer request;
  for (size_t i = 0; i < 3; ++i) {
    int32_t random_lv = base->lvl_base + this->level() +
                        RandomBetween(-base->lvl_change, base->lvl_change);
    QueryOtherPlayer* op = request.add_query();
    op->set_level(random_lv);
  }
  request.set_msg_id(msg_id);
  request.set_request_player_id(this->uid());
  server->SendMessageToCenter(MSG_SS_REQUEST_QUERY_OTHER_PLAYER, &request);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessResponseCrossServerRandomPlayer(
    intranet::MessageSSResponseQueryOtherPlayer& msg) {
  this->cross_server_info_.mutable_enemy()->CopyFrom(msg.player());
  this->cross_server_info_.set_fight_record(0);
  server->SetPlayerKeyValue(this->uid(), kKVTypeCrossServer,
                            cross_server_info_.SerializeAsString());

  MessageResponseCrossServerRandomPlayer res;
  res.mutable_enemy()->CopyFrom(this->cross_server_info_.enemy());
  this->SendMessageToClient(MSG_CS_RESPONSE_CROSS_SERVER_RANDOM_PLAYER, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestCrossServerFight(CSMessageEntry& entry) {
  MessageRequestCrossServerFight* message =
      static_cast<MessageRequestCrossServerFight*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!LogicPlayer::IsCrossServerTime()) return ERR_PARAM_INVALID;

  if (!this->cross_server_info_.country() ||
      this->cross_server_info_.enemy_size() < 3)
    return ERR_PARAM_INVALID;
  int32_t index = -1;
  for (int32_t i = 0; i < this->cross_server_info_.enemy_size(); i++) {
    if (this->cross_server_info_.enemy(i).player_id() == message->uid())
      index = i;
  }
  if (index < 0) return ERR_PARAM_INVALID;
  if (this->daily_counter_[COUNT_TYPE_CROSS_BUY_SERVER_FIGHT] +
          GetSettingValue(crossserverpk1_challenge_count) <=
      this->daily_counter_[COUNT_TYPE_CROSS_SERVER_FIGHT])
    return ERR_PARAM_INVALID;
  int32_t temp_record = this->cross_server_info_.fight_record();
  if (temp_record & (1 << index)) return ERR_PARAM_INVALID;

  CrossServerPK1RollPlayerBase* roll_base =
      CROSS_SERVER_PK1_ROLL_PLAYER_BASE.GetEntryByID(this->level() / 10 * 10)
          .get();
  if (!roll_base) return ERR_PARAM_INVALID;

  const CopyBase* copy_base = COPY_BASE.GetEntryByID(63200).get();
  if (!copy_base) return ERR_COPY_INVALID;

  PK pk(copy_base);
  sy::OtherPlayerInfo me;
  this->FillOtherPlayerInfo(&me);
  float us, uk, ge, jp;
  us = uk = ge = jp = 1.0f;
  int32_t rank_type = 0;
  float add_count = GetSettingValue(crossserverpk1_buff) / 1000.0f;
  switch (this->cross_server_info_.country()) {
    case 1:
      rank_type = RANK_TYPE_CROSS_SERVER_UK;
      uk += add_count;
      break;
    case 2:
      rank_type = RANK_TYPE_CROSS_SERVER_US;
      us += add_count;
      break;
    case 3:
      rank_type = RANK_TYPE_CROSS_SERVER_GE;
      ge += add_count;
      break;
    case 4:
      rank_type = RANK_TYPE_CROSS_SERVER_JP;
      jp += add_count;
      break;
  }
  pk.InitPlayer(&me, true, uk, us, ge, jp);
  pk.InitPlayer(&this->cross_server_info_.enemy(index), false, 1.0f, 1.0f, 1.0f,
                1.0f);
  pk.GeneratePVPReport();
  if (server_config->report()) LogReportToFile(pk.report);

  UpdateBuyCount(COUNT_TYPE_CROSS_SERVER_FIGHT, 1);
  if (pk.star > 0) {
    temp_record |= (1 << index);
    this->cross_server_info_.set_fight_record(temp_record);
    this->cross_server_info_.set_win_count(
        this->cross_server_info_.win_count() + 1);
  }

  int64_t my_fight = 0;
  int64_t other_fight = 0;
  for (int32_t i = 0; i < me.heros_size(); ++i)
    my_fight += me.heros(i).attr1(0);
  for (int32_t i = 0; i < this->cross_server_info_.enemy(index).heros_size();
       ++i)
    other_fight += this->cross_server_info_.enemy(index).heros(i).attr1(0);

  int32_t award_count =
      int32_t((roll_base->reward_a * other_fight * 1.0f / my_fight +
               roll_base->reward_b) *
              (this->level() / 100.0f));
  if (award_count > GetSettingValue(crossserverpk1_fight_award1))
    award_count = GetSettingValue(crossserverpk1_fight_award1);
  if (pk.star <= 0) award_count = GetSettingValue(crossserverpk1_fight_award2);
  AddSubItemSet item_set;
  item_set.push_back(
      ItemParam(23900023, IsHeroComeBack() ? award_count * 1.5f : award_count));
  item_set.push_back(ItemParam(23900024, award_count));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid,
             SYSTEM_ID_CROSS_SERVER_POINT);

  MessageResponseCrossServerFight res;
  res.set_fight_record(this->cross_server_info_.fight_record());
  res.set_win_count(this->cross_server_info_.win_count());
  this->SendMessageToClient(MSG_CS_RESPONSE_CROSS_SERVER_FIGHT, &res);

  MessageNotifyFightResult notify;
  notify.set_win(pk.star > 0);
  notify.set_round(pk.round);
  notify.set_star(pk.star);
  notify.set_copy_id(63200);
  ItemSimpleInfo* its = notify.add_item();
  its->set_item_id(23900023);
  its->set_count(IsHeroComeBack() ? award_count * 1.5f : award_count);
  its = notify.add_item();
  its->set_item_id(23900024);
  its->set_count(award_count);

  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);

  if (this->cross_server_info_.fight_record() == 7)
    this->CrossServerQueryPlayers(MSG_CS_REQUEST_CROSS_SERVER_RANDOM_PLAYER);

  server->SetPlayerKeyValue(this->uid(), kKVTypeCrossServer,
                            cross_server_info_.SerializeAsString());

  RankItemInfo rank_item_info;
  rank_item_info.set_uid(this->uid());
  rank_item_info.set_name(this->name());
  rank_item_info.set_star(this->GetItemCountByItemID(23900024));
  rank_item_info.set_vip_level(this->vip_level());
  rank_item_info.set_fight_attr(my_fight);
  rank_item_info.set_damage(this->cross_server_info_.win_count());
  rank_item_info.set_exploit(rank_type);
  rank_item_info.set_level(server_config->server_id());
  rank_item_info.set_avatar(this->avatar());
  server->SendCrossServerRankItem(rank_item_info, rank_type);
  return ERR_OK;
}

void LogicPlayer::ChangeCrossInfoPlayer(OtherPlayerInfo& info) {
  static int32_t count = 0;
  static int32_t offset[] = {1000000, 2000000, 3000000};
  info.set_player_id(offset[(count % 3)] + info.player_id());
  ++count;
}

int32_t LogicPlayer::ProcessRequestCrossServerGetAward(CSMessageEntry& entry) {
  MessageRequestCrossServerGetAward* message =
      static_cast<MessageRequestCrossServerGetAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  CrossServerPK1WinAwardBase* base =
      CROSS_SERVER_PK1_WIN_AWARD_BASE.GetEntryByID(message->id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (this->cross_server_info_.win_count() < base->win)
    return ERR_PARAM_INVALID;

  int32_t temp_record = this->cross_server_info_.win_get_award();
  if (temp_record & (1 << (base->id() - 1))) return ERR_PARAM_INVALID;

  AddSubItemSet item_set;
  item_set.push_back(ItemParam(23900023, base->award));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid,
             SYSTEM_ID_CROSS_SERVER_POINT);

  temp_record |= (1 << (base->id() - 1));
  this->cross_server_info_.set_win_get_award(temp_record);
  server->SetPlayerKeyValue(this->uid(), kKVTypeCrossServer,
                            cross_server_info_.SerializeAsString());

  MessageResponseCrossServerGetAward res;
  res.set_record(temp_record);
  res.set_id(message->id());
  this->SendMessageToClient(MSG_CS_RESPONSE_CROSS_SERVER_GET_AWARD, &res);

  return ERR_OK;
}


struct SendAwardByPrefixFn : storage::ForEachCallback {
 public:
  SendAwardByPrefixFn(int64_t uid, int32_t type) : uid_(uid), type_(type) {
    this->prefix_ = MakeKVStorageKey(kKVPrefixTimeActivity, uid_, type);
    this->prefix_ += ":";
    INFO_LOG(logger)("BUFA KEY:%s", prefix_.c_str());
  }
  const std::string& prefix() const { return this->prefix_; }

  bool every(const storage::Slice& key, const storage::Slice& value) const {
    if (prefix_.length() >= sizeof(int64_t) &&
        *(int64_t*)key.data() != *(int64_t*)prefix_.data())
      return false;
    std::vector<std::string> temp;
    SplitString(value.data(), temp, ",");
    for (size_t i = 0; i < temp.size(); i++) {
      int32_t item_id = 0;
      int32_t item_count = 0;
      if (sscanf(temp[i].c_str(), "%d|%d", &item_id, &item_count) >= 2) {
        this->award_.push_back(std::make_pair(item_id, item_count));
      }
    }
    this->delete_items_.push_back(std::string(key.data(), key.size()));
    return true;
  }

  ~SendAwardByPrefixFn() {
    if (!award_.empty()) {
      DefaultArrayStream stream;
      stream.Append("%d", type_);
      int32_t mail_type = sy::MAIL_TYPE_TIME_ACTIVITY;
      LogicPlayer::SendMail(uid_, GetSeconds(), mail_type, stream.str(),
                            &award_);
    }
    for (std::vector<std::string>::const_iterator iter =
             this->delete_items_.begin();
         iter != this->delete_items_.end(); ++iter) {
      storage::Delete(*iter);
      INFO_LOG(logger)("BUFA DEL KEY:%s", (*iter).c_str());
    }
    this->delete_items_.clear();
  }

 private:
  std::string prefix_;
  mutable std::vector<std::string> delete_items_;
  mutable std::vector<std::pair<int32_t, int32_t> > award_;
  int64_t uid_;
  int32_t type_;
};

void LogicPlayer::RefreshTimeActivity() {
  for (VectorMap<std::pair<int32_t, int64_t>, sy::ActivityRecord>::iterator it =
           activity_record_new_.begin();
       it != activity_record_new_.end(); ++it) {
    if (it->second.award_size() <= 0) continue;
    TimeActivityNew* act = ACTIVITY.FindAs(it->first.first, it->first.second);
    if (!act || act->end_time() < GetVirtualSeconds()) {
      std::vector<std::pair<int32_t, int32_t> > mail_award;
      for (int32_t i = 0; i < it->second.award_size(); i++) {
        std::vector<std::string> temp;
        SplitString(it->second.award(i), temp, ":");
        if (temp.size() < 2) continue;

        std::vector<std::string> temp2;
        SplitString(temp[1], temp2, ",");
        for (size_t j = 0; j < temp2.size(); j++) {
          int32_t item_id = 0;
          int32_t item_count = 0;
          if (sscanf(temp2[j].c_str(), "%d|%d", &item_id, &item_count) >= 2) {
            mail_award.push_back(std::make_pair(item_id, item_count));
          }
        }
      }
      DefaultArrayStream stream;
      stream.Append("%d", it->first.first);
      LogicPlayer::SendMail(this->uid(), GetSeconds(), MAIL_TYPE_TIME_ACTIVITY,
                            stream.str(), &mail_award);
      it->second.clear_award();
      MessageSSUpdateActivityRecordNew update;
      update.mutable_records()->CopyFrom(it->second);
      this->SendMessageToDB(MSG_SS_UPDATE_ACTIVITY_RECORD_NEW, &update);
    }
  }
  TimeActivityNew* activity = ACTIVITY.GetAs(TIME_ACTIVITY_LOGIN);
  if (activity) {
    int32_t days =
        GetSecondsDiffDays(activity->begin_time(), GetVirtualSeconds());
    if (days >= 0 && days <= 30) {
      int32_t record =
          this->GetActivityRecordNew(TIME_ACTIVITY_LOGIN, activity->id(), -1);
      if (!(record & (1 << days))) {
        record |= (1 << days);
        this->SetActivityRecordNew(TIME_ACTIVITY_LOGIN, activity->id(), -1,
                                   record);
        ACTIVITY.AddActivityRecordCount(TIME_ACTIVITY_LOGIN, 1, this);
      }
    }
  }
  TimeActivityNew* festvial = ACTIVITY.GetAs(TIME_ACTIVITY_FESTIVAL_LOGIN);
  if (festvial) {
    int32_t days =
        GetSecondsDiffDays(festvial->begin_time(), GetVirtualSeconds());
    if (days >= 0 && days <= 30) {
      int32_t record = this->GetActivityRecordNew(TIME_ACTIVITY_FESTIVAL_LOGIN,
                                                  festvial->id(), 0);
      if (!(record & (1 << days))) {
        record |= (1 << days);
        ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_FESTIVAL_LOGIN, record,
                                        this);
      }
    }
  }
}

bool LogicPlayer::IsCrossServerTime() {
  tm tm1 = GetTime();
  const std::vector<int32_t>& begin =
      Setting::GetValue1(Setting::crossserverpk1_rankpk_time1);
  const std::vector<int32_t>& end =
      Setting::GetValue1(Setting::crossserverpk1_rankpk_time2);
  if (begin.size() < 2 || end.size() < 2) return false;
  if (tm1.tm_wday < begin[0] || tm1.tm_wday > end[0]) return false;
  if (tm1.tm_wday == begin[0] && tm1.tm_hour < begin[1]) return false;
  if (tm1.tm_wday == end[0] && tm1.tm_hour >= end[1]) return false;
  return true;
}

void LogicPlayer::ClearCrossServer() {
  this->cross_server_info_.Clear();
  this->cross_server_info_.set_country(0);
  if (this->is_online()) {
    MessageNotifyCrossServer notify;
    notify.mutable_info()->CopyFrom(this->cross_server_info_);
    this->SendMessageToClient(MSG_CS_NOTIFY_CROSS_SERVER, &notify);
  }
}

int32_t LogicPlayer::ProcessRequestAstrologyInfo(CSMessageEntry& entry) {
  MessageResponseGetAstrologyInfo res;
  res.set_astrology_award_refresh_time(server->AstrologyRefreshTime());
  res.set_astrology_award_country(server->AstrologyAwardCountry());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_ASTROLOGY_INFO, &res);
  return ERR_OK;
}

static inline int32_t GetDiamondFundBaseIndex(int32_t type,
                                              const DiamondFundBase* base) {
  int32_t index = 0;
  for (int32_t days = 1; days <= 1000; ++days) {
    const DiamondFundBase* b =
        DIAMOND_FUND_BASE.GetEntryByID(type * 1000 + days).get();
    if (b) ++index;
    if (b == base) return index;
  }
  return 0;
}

int32_t LogicPlayer::ProcessRequestGetLoginAward(CSMessageEntry& entry) {
  MessageRequestGetLoginAward* message = static_cast<MessageRequestGetLoginAward*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t type = message->is_vip() ? 5 : 4;
  const DiamondFundBase* base = DIAMOND_FUND_BASE.GetEntryByID(type * 1000 + message->days()).get();
  if (!base) return ERR_PARAM_INVALID;
  int32_t index = GetDiamondFundBaseIndex(type, base);
  if (index <= 0) return ERR_PARAM_INVALID;
  int32_t login_days = this->player_.login_days();
  if (login_days < message->days()) return ERR_DEBUG_LOGIN_AWARD_LOGIN_DAYS;
  int32_t month_card_days =
      this->achievements_[ACHIEVEMENT_TYPE_COUNT_MONTH_CARD] * 30;
  if (message->is_vip() && month_card_days < message->days())
    return ERR_DEBUG_LOGIN_AWARD_MONTH_CARD;

  uint32_t low = message->is_vip()
                    ? this->achievements_[ACHIEVEMENT_TYPE_VIP_LOGIN_AWARD_1]
                    : this->achievements_[ACHIEVEMENT_TYPE_LOGIN_AWARD_1];
  uint32_t high = message->is_vip()
                     ? this->achievements_[ACHIEVEMENT_TYPE_VIP_LOGIN_AWARD_2]
                     : this->achievements_[ACHIEVEMENT_TYPE_LOGIN_AWARD_2];
  uint64_t status = uint64_t(high) << 32 | low;
  if (status & (1ul << index)) return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_LOGIN_AWARD);
  AddSubItemSet item_set;
  if (base->award.v1 < 100) {
    modify[base->award.v1] += base->award.v2;
  } else {
    item_set.push_back(ItemParam(base->award.v1, base->award.v2));
  }
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_LOGIN_AWARD);

  status |= (1ul << index);
  if ((status << 32 >> 32) != low) {
    low = status << 32 >> 32;
    this->UpdateAchievement(message->is_vip()
                                ? ACHIEVEMENT_TYPE_VIP_LOGIN_AWARD_1
                                : ACHIEVEMENT_TYPE_LOGIN_AWARD_1,
                            low);
  }
  if ((status >> 32) != high) {
    high = status >> 32;
    this->UpdateAchievement(message->is_vip()
                                ? ACHIEVEMENT_TYPE_VIP_LOGIN_AWARD_2
                                : ACHIEVEMENT_TYPE_LOGIN_AWARD_2,
                            high);
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_LOGIN_AWARD, NULL);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestWeeklyCardSign(CSMessageEntry& entry) {
  MessageRequestWeeklyCardSign* message =
      static_cast<MessageRequestWeeklyCardSign*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t valid_days = 0;
  int32_t buy_count = 0;
  int32_t* card = this->GetWeeklyCardByType(message->type(), valid_days, buy_count);
  if (!card || !card[0] || GetSecondsDiffDays(card[0], GetSeconds()) > valid_days) {
    return ERR_DEBUG_WEEKLY_CARD_INVALID;
  }
  if (card[1] >= valid_days ||
      card[1] >= GetSecondsDiffDays(card[0], GetSeconds()) + 1) {
    return ERR_DEBUG_WEEKLY_CARD_SIGN;
  }

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_WEEKLY_CARD);
  modify.money -= GetSettingValue(weekly_card_sign);
  CHECK_RET(this->CheckCurrency(modify));
  this->UpdateCurrency(modify);

  card[1]++;
  this->UpdateMonthCard(true);
  MessageResponseWeeklyCardSign response;
  response.set_type(message->type());
  this->SendMessageToClient(MSG_CS_RESPONSE_WEEKL_CARD_SIGN, &response);
  return ERR_OK;
}

static inline const DiamondFundBase* GetWeeklyCardBase(int32_t type,
                                                       int32_t days,
                                                       int32_t count) {
  while (count >= 0) {
    int32_t id = count * 10000 + type * 1000 + days;
    const DiamondFundBase* base = DIAMOND_FUND_BASE.GetEntryByID(id).get();
    if(base) return base;
    --count;
  }
  return NULL;
}

int32_t LogicPlayer::ProcessRequestGetWeeklyCardAward(CSMessageEntry& entry) {
  MessageRequestGetWeeklyCardAward* message =
      static_cast<MessageRequestGetWeeklyCardAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t valid_days = 0;
  int32_t buy_count = 0;
  int32_t* card = this->GetWeeklyCardByType(message->type(), valid_days, buy_count);
  if (!card || !card[0] || GetSecondsDiffDays(card[0], GetSeconds()) > valid_days) {
    return ERR_DEBUG_WEEKLY_CARD_INVALID;
  }
  const DiamondFundBase* base =
      GetWeeklyCardBase(message->type() + 1, message->days(), buy_count);
  if (!base) return ERR_PARAM_INVALID;
  if (card[2] & (1 << message->days())) return ERR_DEBUG_WEEKLY_CARD_AWARD;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_WEEKLY_CARD);
  AddSubItemSet item_set;
  if (base->award.v1 < 100) {
    modify[base->award.v1] += base->award.v2;
  } else {
    item_set.push_back(ItemParam(base->award.v1, base->award.v2));
  }
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  this->UpdateCurrency(modify);
  this->ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_WEEKLY_CARD);
  card[2] |= (1 << message->days());
  this->UpdateMonthCard(true);
  MessageResponseGetWeeklyCardAward response;
  response.set_type(message->type());
  response.set_days(message->days());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_WEEKLY_CARD_AWARD, &response);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLegionForeplayFight(CSMessageEntry& entry) {
  MessageRequestLegionForeplayFight* message =
      static_cast<MessageRequestLegionForeplayFight*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (server->GetServerStartDays() > GetSettingValue(legion_foreplay_endday))
    return ERR_PARAM_INVALID;
  int max_fight_count =
      GetSettingValue(legion_foreplay_challenge_count) +
      (GetTime().tm_hour /
       GetSettingValue(legion_foreplay_challenge_recovertime)) +
      this->daily_counter_[COUNT_TYPE_LEGION_FOREPLAY_BUY_COUNT];
  if (this->daily_counter_[COUNT_TYPE_LEGION_FOREPLAY_FIGHT_COUNT] >=
      max_fight_count)
    return ERR_PARAM_INVALID;

  sy::LegionForeplayInfo& info = server->LegionForeplayInfo();
  bool is_dead = true;
  for (int32_t i = 0; i < info.current_hp_size(); i++) {
    if (info.current_hp(i) > 0) {
      is_dead = false;
      break;
    }
  }

  if (message->id() != info.id() || is_dead) {
    MessageResponseLegionForeplayFight res_err;
    res_err.set_id(0);
    res_err.mutable_boss_info()->CopyFrom(server->LegionForeplayInfo());
    this->SendMessageToClient(MSG_CS_RESPONSE_LEGION_FOREPLAY_FIGHT, &res_err);
    return ERR_OK;
  }
  LegionForeplayCopyBase* legion_base =
      LEGION_FOREPLAY_COPY_BASE.GetEntryByID(message->id()).get();
  if (!legion_base) return ERR_PARAM_INVALID;
  const CopyBase* copy_base = COPY_BASE.GetEntryByID(63500).get();
  if (!copy_base) return ERR_COPY_INVALID;
  MonsterGroupBase* group =
      MONSTER_GROUP_BASE.GetEntryByID(legion_base->monster).get();
  if (!group) return ERR_PARAM_INVALID;

  PK pk(copy_base);
  pk.InitPlayer(this, true);
  pk.InitMonsterInfo(group->carrier_info(), group->hero_info(),
                     info.current_hp().begin());
  pk.GeneratePVPReport();
  if (server_config->report()) LogReportToFile(pk.report);

  std::vector<int64_t> current_hp;
  current_hp.resize(6, 0);
  int64_t all_current_hp = 0;
  int64_t all_damage = 0;
  for (int32_t i = 0; i < 6; i++) {
    PkHero* hero = pk.b.GetHeroByPos(i + 1);
    if (hero) {
      current_hp[i] = hero->hp();
      all_current_hp += current_hp[i];
      all_damage += info.current_hp(i) - current_hp[i];
      info.set_current_hp(i, current_hp[i]);
    } else {
      INFO_LOG(logger)("LegionForeplay Pos:%d, invalid", i + 1);
    }
  }

  int64_t origin_damage = all_damage;
  all_damage /= 10000;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_LEGION_FOREPLAY);
  AddSubItemSet item_set;
  int32_t award =
      LegionForeplayAward1Base::GetAward(server->GetServerStartDays());
  modify[MONEY_KIND_COIN] += award;

  if (info.log_size() >= 5) info.mutable_log()->DeleteSubrange(0, 1);
  ForeplayInfoLog* flog = info.add_log();
  flog->set_name(this->name());
  flog->set_damage(all_damage);

  MessageResponseLegionForeplayFight res;
  MessageNotifyFightResult notify;
  FillFightResultMessage(notify, item_set, modify);

  if (all_current_hp <= 0) {
    FillCurrencyAndItem<kAdd>(modify, item_set, legion_base->merit_award);
    server->RefreshLegionForeplay(info.id() + 1);
    res.set_merit_award(legion_base->merit_award.v2);
    res.set_merit_award_item(legion_base->merit_award.v1);
    MessageNotifyServerNotice notice;
    notice.set_type(1);
    char temp[20] = {0};
    sprintf(temp, "%ld", legion_base->id());
    notice.add_param(this->name());
    notice.add_param(temp);
    server->SendMessageToAllClient(MSG_CS_NOTIFY_SERVER_NOTICE, &notice);
  } else {
    server->SetKeyValue(kKVTypeLegionForeplay, info.SerializeAsString());
  }


  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid,
             SYSTEM_ID_LEGION_FOREPLAY);

  this->UpdateBuyCount(COUNT_TYPE_LEGION_FOREPLAY_DAMAGE, all_damage);
  this->UpdateBuyCount(COUNT_TYPE_LEGION_FOREPLAY_FIGHT_COUNT, 1);

  res.set_old_rank(RANK_LIST.GetByType(sy::RANK_TYPE_LEGION_FOREPLAY).GetRankByUID(this->uid()));
  RANK_LIST.OnLegionForeplay(
      this, this->daily_counter_[COUNT_TYPE_LEGION_FOREPLAY_DAMAGE]);
  res.set_new_rank(RANK_LIST.GetByType(sy::RANK_TYPE_LEGION_FOREPLAY).GetRankByUID(this->uid()));

  MessageNotifyForeplayStatus status;
  status.set_player_name(this->player_.name());
  status.mutable_info()->CopyFrom(server->LegionForeplayInfo());
  status.set_damage(origin_damage);
  server->SendMessageToForeplayPlayers(MSG_CS_NOTIFY_FOREPLAY_STATUS, &status,
                                       this->uid());

  notify.set_star(pk.star);
  notify.set_copy_id(copy_base->id());
  notify.set_win(pk.star > 0);
  notify.set_total_damage(all_damage);
  notify.set_round(pk.round);
  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  res.set_id(message->id());
  res.mutable_boss_info()->CopyFrom(server->LegionForeplayInfo());
  this->SendMessageToClient(MSG_CS_RESPONSE_LEGION_FOREPLAY_FIGHT, &res);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLegionForeplayGetServerAward(
    CSMessageEntry& entry) {
  MessageRequestLegionForeplayGetServerAward* message =
      static_cast<MessageRequestLegionForeplayGetServerAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LegionForeplayCopyBase* base =
      LEGION_FOREPLAY_COPY_BASE.GetEntryByID(message->id()).get();
  if (!base) return ERR_PARAM_INVALID;

  int32_t current_id = server->LegionForeplayInfo().id();
  sy::LegionForeplayInfo& info = server->LegionForeplayInfo();
  bool is_dead = true;
  for (int32_t i = 0; i < info.current_hp_size(); i++) {
    if (info.current_hp(i) > 0) {
      is_dead = false;
      break;
    }
  }
  if (message->id() > current_id || ((message->id() == current_id) && !is_dead))
    return ERR_PARAM_INVALID;

  int32_t record = achievements_[OTHER_LEGION_FOREPLAY_RECORD];
  if (record & (1 << (base->id() - 1))) return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_LEGION_FOREPLAY);
  AddSubItemSet item_set;
  FillCurrencyAndItem<kAdd>(modify, item_set, base->server_award);

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid,
             SYSTEM_ID_LEGION_FOREPLAY);

  record |= (1 << (base->id() - 1));
  UpdateAchievement(OTHER_LEGION_FOREPLAY_RECORD, record);

  MessageResponseLegionForeplayGetServerAward res;
  res.set_id(message->id());
  FillToKVPair2(&modify, &item_set, res.mutable_award());
  this->SendMessageToClient(MSG_CS_RESPONSE_LEGION_FOREPLAY_GET_SERVER_AWARD,
                            &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLegionForeplayGetDamageAward(
    CSMessageEntry& entry) {
  MessageRequestLegionForeplayGetDamageAward* message =
      static_cast<MessageRequestLegionForeplayGetDamageAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t record =
      this->daily_counter_[COUNT_TYPE_LEGION_FOREPLAY_AWARD_RECORD];
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_LEGION_FOREPLAY);
  AddSubItemSet item_set;

  for (int32_t i = 0; i < message->id_size(); i++) {
    LegionForeplayAwardBase* base =
        LEGION_FOREPLAY_AWARD_BASE.GetEntryByID(message->id(i)).get();
    if (!base) return ERR_PARAM_INVALID;

    if (base->merit_num >
        this->daily_counter_[COUNT_TYPE_LEGION_FOREPLAY_DAMAGE])
      return ERR_PARAM_INVALID;

    if (record & (1 << (base->id() - 1))) return ERR_PARAM_INVALID;

    FillCurrencyAndItem<kAdd>(modify, item_set, base->merit_award);
    record |= (1 << (base->id() - 1));
  }

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid,
             SYSTEM_ID_LEGION_FOREPLAY);

  this->daily_counter_[COUNT_TYPE_LEGION_FOREPLAY_AWARD_RECORD] = record;
  MessageNotifyBuyCount notify;
  sy::KVPair2* info = notify.add_infos();
  info->set_key(COUNT_TYPE_LEGION_FOREPLAY_AWARD_RECORD);
  info->set_value(daily_counter_[COUNT_TYPE_LEGION_FOREPLAY_AWARD_RECORD]);
  this->SendMessageToClient(MSG_CS_NOTIFY_BUY_COUNT, &notify);
  MessageSSUpdateBuyCount request;
  request.add_infos()->CopyFrom(*info);
  this->SendMessageToDB(MSG_SS_UPDATE_BUY_COUNT, &request);

  MessageResponseLegionForeplayGetDamageAward res;
  res.mutable_id()->CopyFrom(message->id());
  FillToKVPair2(&modify, &item_set, res.mutable_award());
  this->SendMessageToClient(MSG_CS_RESPONSE_LEGION_FOREPLAY_GET_DAMAGE_AWARD,
                            &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLegionForeplayGetInfo(
    CSMessageEntry& entry) {
  MessageResponseLegionForeplayGetInfo res;
  res.mutable_info()->CopyFrom(server->LegionForeplayInfo());
  this->SendMessageToClient(MSG_CS_RESPONSE_LEGION_FOREPLAY_GET_INFO, &res);
  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestEnterStage(CSMessageEntry& entry) {
  MessageRequestEnterStage* message =
      static_cast<MessageRequestEnterStage*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->stage_id() == 1) {
    server->WorldBossPlayers()[this->uid()] = entry.session_ptr;
  }
  if (message->stage_id() == 2) {
    server->ForeplayPlayers()[this->uid()] = entry.session_ptr;
  }
  this->SendMessageToClient(MSG_CS_REQUEST_ENTER_STAGE, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestLeaveStage(CSMessageEntry& entry) {
  MessageRequestLeaveStage* message =
      static_cast<MessageRequestLeaveStage*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->stage_id() == 1) {
    server->WorldBossPlayers().erase(this->uid());
  }
  if (message->stage_id() == 2) {
    server->ForeplayPlayers().erase(this->uid());
  }
  this->SendMessageToClient(MSG_CS_REQUEST_LEAVE_STAGE, NULL);

  return ERR_OK;
}

int32_t LogicPlayer::IsRobSuccess(ArmyBase* army_base, bool is_rob_player) {
  if (!army_base) return 0;

  int32_t probability = 0;
  if (is_rob_player)
    probability = army_base->probability_player;
  else
    probability = army_base->probability_computer;

  if ((QUALITY_ORANGE == army_base->quality &&
       achievements_[ACHIEVEMENT_TYPE_NAVY_ORANGE_COUNT] < 1) ||
      (QUALITY_PURPLE == army_base->quality &&
       achievements_[ACHIEVEMENT_TYPE_NAVY_PURPLE_COUNT] < 2)) {
    int32_t lack_num = 0;
    for (ValuePair2Vec::const_iterator it = army_base->soldier.begin();
         it != army_base->soldier.end(); ++it) {
      if (!this->GetItemCountByItemID(it->v1)) lack_num++;
    }
    if (lack_num > 1) return 2;
  }
  if (RandomIn10000() <= probability) return 1;
  return 0;
}

int32_t LogicPlayer::ProcessNotifyMoneyInfo(CSMessageEntry& entry) {
  MessageNotifyMoneyInfo notify;
  notify.set_exp(this->player_.exp());
  notify.set_vip_exp(this->player_.vip_exp());
  notify.set_level(this->player_.level());
  notify.set_vip_level(this->player_.vip_level());
  notify.set_coin(this->player_.coin());
  notify.set_money(this->player_.money());
  notify.set_prestige(this->player_.prestige());
  notify.set_hero(this->player_.hero());
  notify.set_plane(this->player_.plane());
  notify.set_exploit(this->player_.exploit());
  notify.set_muscle(this->player_.muscle());
  notify.set_union_(this->player_.union_());
  this->SendMessageToClient(MSG_CS_NOTIFY_MONEY_INFO, &notify);
  return ERR_OK;
}

int32_t LogicPlayer::GetActivityRecordNew(sy::TimeActivityType type, int64_t id,
                                          int32_t key) {
  VectorMap<std::pair<int32_t, int64_t>, sy::ActivityRecord>::iterator it =
      activity_record_new_.find(std::make_pair(type, id));
  if (it == activity_record_new_.end()) return 0;

  for (int32_t i = 0; i < it->second.record_size(); ++i) {
    if (it->second.record(i).key() == key) return it->second.record(i).value();
  }

  return 0;
}

ActivityRecord* LogicPlayer::GetActivityRecordX(sy::TimeActivityType type, int64_t id) {
  VectorMap<std::pair<int32_t, int64_t>, sy::ActivityRecord>::iterator it =
      this->activity_record_new_.find(std::make_pair(type, id));
  if (it == this->activity_record_new_.end()) return NULL;
  return &it->second;
}

void LogicPlayer::SetActivityRecordNew(int32_t type, int64_t id,
                                       int32_t key, int32_t value) {
  sy::ActivityRecord& record = activity_record_new_[std::make_pair(type, id)];
  record.set_type(type);
  record.set_id(id);
  bool flag = false;
  for (int32_t i = 0; i < record.record_size(); i++) {
    if (record.record(i).key() == key) {
      record.mutable_record(i)->set_value(value);
      flag = true;
      break;
    }
  }
  if (!flag) {
    KVPair2* pair = record.add_record();
    pair->set_key(key);
    pair->set_value(value);
  }
  ACTIVITY.SetActivityRecord(record, this);
  record.set_refresh_time(GetVirtualSeconds());
  UpdateActivityRecordNew(type, id);
}

void LogicPlayer::AddActivityRecordNew(int32_t type, int64_t id,
                                       int32_t key, int32_t value) {
  sy::ActivityRecord& record = activity_record_new_[std::make_pair(type, id)];
  record.set_type(type);
  record.set_id(id);
  bool flag = false;
  for (int32_t i = 0; i < record.record_size(); i++) {
    if (record.record(i).key() == key) {
      record.mutable_record(i)->set_value(record.record(i).value() + value);
      flag = true;
      break;
    }
  }
  if (!flag) {
    KVPair2* pair = record.add_record();
    pair->set_key(key);
    pair->set_value(value);
  }
  ACTIVITY.SetActivityRecord(record, this);
  record.set_refresh_time(GetVirtualSeconds());
  UpdateActivityRecordNew(type, id);
}

void LogicPlayer::UpdateActivityRecordNew(int32_t type, int64_t id) {
  MessageNotifyActivityRecord notify;
  MessageSSUpdateActivityRecordNew update;

  const ActivityRecord* record = this->GetActivityRecordX((sy::TimeActivityType)type, id);
  if (!record) {
    ERROR_LOG(logger)("ActivityRecord Not Found type:%d,id:%ld", type, id);
    return;
  }

  notify.mutable_records()->CopyFrom(*record);
  update.mutable_records()->CopyFrom(*record);

  this->SendMessageToClient(MSG_CS_NOTIFY_ACTIVITY_RECORD, &notify);
  this->SendMessageToDB(MSG_SS_UPDATE_ACTIVITY_RECORD_NEW, &update);
}

int32_t LogicPlayer::ProcessRequestFestivalReplenishSign(
    CSMessageEntry& entry) {
  MessageRequestFestivalReplenishSign* message =
      static_cast<MessageRequestFestivalReplenishSign*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->day() < 1 || message->day() > 30) return ERR_PARAM_INVALID;

  TimeActivityNew* activity = ACTIVITY.GetAs(TIME_ACTIVITY_FESTIVAL_LOGIN);
  if (!activity) return ERR_PARAM_INVALID;

  if ((message->day() - 1) >=
      GetSecondsDiffDays(activity->begin_time(), GetVirtualSeconds()))
    return ERR_PARAM_INVALID;

  int32_t record = this->GetActivityRecordNew(TIME_ACTIVITY_FESTIVAL_LOGIN,
                                              activity->id(), 0);

  if (record & (1 << (message->day() - 1))) return ERR_PARAM_INVALID;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_FESTIVAL_ACTIVITY);
  modify[MONEY_KIND_MONEY] = -GetSettingValue(login_sign_cost);

  CHECK_RET(CheckCurrency(modify));
  UpdateCurrency(modify);

  record |= (1 << (message->day() - 1));
  ACTIVITY.SetActivityRecordCount(TIME_ACTIVITY_FESTIVAL_LOGIN, record, this);

  MessageResponseFestivalReplenishSign res;
  res.set_day(message->day());
  this->SendMessageToClient(MSG_CS_RESPONSE_FESTIVAL_REPLENISHSIGN, &res);

  return ERR_OK;
}

void LogicPlayer::RefreshFestivalShop() {
  if (GetVirtualSeconds() < Setting::kFestivalStartTime ||
      GetVirtualSeconds() >= Setting::kFestivalEndTime)
    return;
  if (this->festival_shop_refresh_time_ >= Setting::kFestivalStartTime &&
      this->festival_shop_refresh_time_ < Setting::kFestivalEndTime)
    return;
  this->festival_shop_refresh_time_ = GetVirtualSeconds();
  char temp[20] = {0};
  sprintf(temp, "%ld", this->festival_shop_refresh_time_);
  server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypeFestivalRefreshTime,
                            temp);
  int32_t temp_shops[] = {101, 102, 103, 104, 105, 106, 107};
  for (size_t i = 0; i < sizeof(temp_shops) / sizeof(*temp_shops); i++) {
    RefreshShopInfo& shop_info = this->refresh_shop_info_[temp_shops[i]];
    shop_info.set_shop_id(temp_shops[i]);
    ShopBase::RandomFeatsCommodity(temp_shops[i], this->level(),
                                   shop_info.mutable_feats_commodity(),
                                   server->GetServerStartDays());
    UpdateFeatsShopInfo(temp_shops[i]);
  }
}

int32_t LogicPlayer::ProcessRequestPlaneChange(CSMessageEntry& entry) {
  MessageRequestPlaneChange* message =
      static_cast<MessageRequestPlaneChange*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!PlaneKind_IsValid(message->type())) return ERR_PARAM_INVALID;

  AddSubItemSet item_set;
  ModifyCurrency modify(MSG_CS_REQUEST_PLANE_CHANGE, SYSTEM_ID_PLANE_SYNTHESIS);

  VectorMap<int32_t, int32_t> new_planes;
  for (int32_t i = 0; i < message->planes_size(); i++) {
    const KVPair2& pair = message->planes(i);
    if (pair.value() <= 0) return ERR_PARAM_INVALID;
    int32_t plane_id = pair.key();
    int32_t count = (pair.value() <= 100 ? pair.value() : 100);

    const CarrierPlaneBase* base =
        CARRIER_PLANE_BASE.GetEntryByID(pair.key()).get();
    if (!base) return ERR_PARAM_INVALID;
    if ((int32_t)base->type == message->type()) return ERR_PARAM_INVALID;
    const CarrierPlaneBase* dest_base =
        CarrierPlaneBase::GetPlaneBase(message->type(), base->plane_lv);
    if (!dest_base) return ERR_INTERNAL;

    int32_t money_num =
        Setting::GetValueInVec2(Setting::plane_transfer_cost, base->plane_lv);
    if (!money_num) return ERR_INTERNAL;

    item_set.push_back(ItemParam(plane_id, -count));
    item_set.push_back(ItemParam(dest_base->id(), count));
    modify[MONEY_KIND_MONEY] -= money_num * count;

    new_planes[dest_base->id()] += count;
  }

  MessageResponsePlaneChange res;
  for (VectorMap<int32_t, int32_t>::iterator it = new_planes.begin();
       it != new_planes.end(); ++it) {
    KVPair2* res_pair = res.add_planes();
    res_pair->set_key(it->first);
    res_pair->set_value(it->second);
  }

  CHECK_RET(CheckItem(&item_set, NULL, NULL));
  CHECK_RET(CheckCurrency(modify));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, MSG_CS_REQUEST_PLANE_CHANGE,
             SYSTEM_ID_PLANE_SYNTHESIS);

  this->SendMessageToClient(MSG_CS_RESPONSE_PLANE_CHANGE, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetVersionAward(CSMessageEntry& entry) {
  MessageRequestGetVersionAward* message =
      static_cast<MessageRequestGetVersionAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  bool flag = CanGetUpdateVersionAward(message->type_id(), message->version(),
                                       this->got_award_version_);
  if (!flag) return ERR_PARAM_INVALID;

  AddSubItemSet item_set;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_BASE);

  const LootBasePtr& loot =
      LootConfigFile::Get(GetSettingValue(version_reward_id), this->level());
  if (loot)
    loot->Loot(modify, item_set, NULL);
  else
    return ERR_PARAM_INVALID;

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_BASE);
  this->got_award_version_ = message->version();
  server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypeGotVersion,
                            this->got_award_version_);

  MessageResponseGetVersionAward res;
  res.set_type_id(message->type_id());
  res.set_version(message->version());

  FillToKVPair2(&modify, &item_set, res.mutable_award());

  this->SendMessageToClient(MSG_CS_RESPONSE_GET_VERSION_AWARD, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestMedalResearch(CSMessageEntry& entry) {
  MessageRequestMedalResearch* message =
      static_cast<MessageRequestMedalResearch*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t count = message->type();
  if (count != 1 && count != 5) return ERR_PARAM_INVALID;

  int64_t get_coin = GetSettingValue(medal_recruit_reward1) * count;
  int32_t need_money = GetSettingValue(medal_recruit_1gold) * count;
  if (!this->daily_counter_[COUNT_TYPE_MEDAL_RESEARCH_COUNT])
    need_money -= GetSettingValue(medal_recruit_1gold);
  if (count == 5) need_money = GetSettingValue(medal_recruit_5gold);

  AddSubItemSet item_set;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_MEDAL);
  MessageResponseMedalResearch res;

  int32_t researched_count = this->achievements_[OTHER_MEDAL_RESEARCH_COUNT];
  for (int32_t i = 0; i < count; i++) {
    if (!(++researched_count % GetSettingValue(medal_recruit_number5))) {
      const LootBasePtr& loot = LootConfigFile::Get(
          GetSettingValue(medal_recruit_5drop), this->level());
      if (loot) loot->Loot(modify, item_set, NULL);
    } else {
      const LootBasePtr& loot = LootConfigFile::Get(
          GetSettingValue(medal_recruit_1drop), this->level());
      if (loot) loot->Loot(modify, item_set, NULL);
    }
  }
  FillToKVPair2(&modify, &item_set, res.mutable_medal(), true);

  int32_t brozen_count = 0;
  for (AddSubItemSet::iterator it = item_set.begin(); it != item_set.end();
       ++it) {
    ItemBase* base = ITEM_BASE.GetEntryByID(it->item_id).get();
    if (!base) continue;
    int32_t brozen =
        Setting::GetValueInVec2(Setting::medal_recruit_reward2, base->quality);
    res.add_bronze(brozen);
    brozen_count += brozen;
  }
  if (brozen_count) item_set.push_back(ItemParam(23900009, brozen_count));

  modify[MONEY_KIND_COIN] += get_coin;
  if (need_money) item_set.push_back(ItemParam(23900160, -need_money));
  CHECK_RET(this->CheckCurrency(modify));
  CHECK_RET(this->CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_MEDAL);
  UpdateAchievement(OTHER_MEDAL_RESEARCH_COUNT, researched_count);
  UpdateBuyCount(COUNT_TYPE_MEDAL_RESEARCH_COUNT, count);

  this->SendMessageToClient(MSG_CS_RESPONSE_MEDAL_RESEARCH, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestMedalFight(CSMessageEntry& entry) {
  VipFunctionBase* vip_base = VIP_FUNCTION_BASE.GetEntryByID(1971).get();
  if (!vip_base) return ERR_INTERNAL;
  int32_t can_fight_count =
      vip_base->GetValue(this->level(), this->vip_level());
  if (this->daily_counter_[COUNT_TYPE_MEDAL_FIGHT] >= can_fight_count)
    return ERR_PARAM_INVALID;

  const CopyBase* copy_base = COPY_BASE.GetEntryByID(63600).get();
  if (!copy_base) return ERR_COPY_INVALID;
  const MedalCopyBase* medal_copy_base =
      MEDAL_COPY_BASE.GetEntryByID(this->medal_copy_id_).get();
  if (!medal_copy_base) return ERR_COPY_INVALID;
  MonsterGroupBase* monster =
      MONSTER_GROUP_BASE.GetEntryByID(medal_copy_base->monster).get();
  if (!monster) return ERR_COPY_INVALID;

  PK pk(copy_base);
  pk.InitPlayer(this, true);
  pk.InitMonsterGroup(monster, false, false);
  pk.GeneratePVPReport();

  MessageNotifyFightResult notify;
  notify.set_star(pk.star);
  notify.set_win(pk.star ? 1 : 0);
  notify.set_copy_id(copy_base->id());
  notify.set_round(pk.round);

  if (pk.star) {
    ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_MEDAL);
    AddSubItemSet item_set;
    FillCurrencyAndItem<kAdd>(modify, item_set, medal_copy_base->reward);

    CHECK_RET(CheckCurrency(modify));
    CHECK_RET(CheckItem(&item_set, NULL, NULL));
    UpdateCurrency(modify);
    ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_MEDAL);

    this->medal_copy_id_ = RefreshMedalCopyID();

    UpdateBuyCount(COUNT_TYPE_MEDAL_FIGHT, 1);
  }

  this->SendMessageToClient(MSG_CS_NOTIFY_FIGTH_RESULT, &notify);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);

  MessageResponseMedalFight res;
  res.set_medal_copy_id(this->medal_copy_id_);
  this->SendMessageToClient(MSG_CS_RESPONSE_MEDAL_FIGHT, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestMedalFightRefresh(CSMessageEntry& entry) {

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_MEDAL);

  if (this->daily_counter_[COUNT_TYPE_MEDAL_REFRESH] >=
      GetSettingValue(medal_number2))
    modify[MONEY_KIND_MONEY] = -GetSettingValue(medal_number3);

  CHECK_RET(CheckCurrency(modify));

  this->medal_copy_id_ = RefreshMedalCopyID();

  UpdateCurrency(modify);
  UpdateBuyCount(COUNT_TYPE_MEDAL_REFRESH, 1);

  MessageResponseMedalFightRefresh res;
  res.set_medal_copy_id(this->medal_copy_id_);
  this->SendMessageToClient(MSG_CS_RESPONSE_MEDAL_FIGHT_REFRESH, &res);

  MessageUpdateMedalCopyID upmcid;
  upmcid.set_medal_copy_id(this->medal_copy_id_);
  this->SendMessageToDB(MSG_SS_UPDATE_MEDAL_COPY_ID, &upmcid);

  return ERR_OK;
}

int32_t LogicPlayer::RefreshMedalCopyID() {
  AddSubItemSet item_set;
  ModifyCurrency modify(0, SYSTEM_ID_MEDAL);
  const LootBasePtr& loot =
      LootConfigFile::Get(GetSettingValue(medal_copy_id), this->level());
  if (loot) loot->Loot(modify, item_set, NULL);
  if (item_set.empty()) return 0;
  MedalCopyBase* base = MEDAL_COPY_BASE.GetEntryByID(item_set[0].item_id).get();
  if (!base) return 0;

  return base->id();
}

int32_t LogicPlayer::ProcessRequestMedalActive(CSMessageEntry& entry) {
  MessageRequestMedalActive* message =
      static_cast<MessageRequestMedalActive*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->id() > 999 || message->id() <= 0) return ERR_PARAM_INVALID;

  MedalChartBase* base = MEDAL_CHART_BASE.GetEntryByID(message->id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if ((int32_t)medal_state_.size() < message->id())
    medal_state_.resize(message->id(), 'x');

  AddSubItemSet item_set;
  if (message->is_reset()) {
    for (std::vector<int32_t>::iterator it = base->cost.begin();
         it != base->cost.end(); it++)
      item_set.push_back(ItemParam(*it, 1));

    CHECK_RET(CheckItem(&item_set, NULL, NULL));
    ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_MEDAL);
    this->medal_star_ -= base->cost.size();

    this->medal_state_[message->id() - 1] = 'x';
  } else {
    for (std::vector<int32_t>::iterator it = base->cost.begin();
         it != base->cost.end(); it++)
      item_set.push_back(ItemParam(*it, -1));

    CHECK_RET(CheckItem(&item_set, NULL, NULL));
    ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_MEDAL);
    this->medal_star_ += base->cost.size();

    this->medal_state_[message->id() - 1] = '0';
  }

  RANK_LIST.OnMedalRank(this);
  RankItemInfo rank_info;
  rank_info.set_uid(this->uid());
  rank_info.set_name(this->name());
  rank_info.set_star(this->medal_star_);
  rank_info.set_vip_level(this->vip_level());
  rank_info.set_damage(this->medal_state_count());
  rank_info.set_level(server_config->server_id());
  rank_info.set_avatar(this->avatar());
  server->SendCrossServerRankItem(rank_info, RANK_TYPE_MEDAL_CROSS_SERVER);

  MessageResponseMedalActive res;
  res.set_active_state(this->medal_state_);
  res.set_id(message->id());
  res.set_medal_star(this->medal_star_);
  this->SendMessageToClient(MSG_CS_RESPONSE_MEDAL_ACTIVE, &res);

  MessageUpdateMedalCopyID update;
  update.set_medal_star(this->medal_star_);
  update.set_medal_state(this->medal_state_);
  this->SendMessageToDB(MSG_SS_UPDATE_MEDAL_COPY_ID, &update);

  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_MEDAL, 0);

  return ERR_OK;
}

void LogicPlayer::AddMedalAttr(AttackAttrArray& attr) {
  for (size_t i = 0; i < this->medal_state_.size(); i++) {
    if (this->medal_state_[i] == 'x') continue;
    MedalChart2Base* base = MEDAL_CHART2_BASE.GetEntryByID(i + 1).get();
    if (!base) continue;
    for (ValuePair2Vec::const_iterator it = base->attr.begin();
         it != base->attr.end(); ++it)
      attr[it->v1] += it->v2;
  }
  const MedalFormationBase* formation_base =
      MEDAL_FORMATION_BASE.GetEntryByID(this->medal_achi_).get();
  if (!formation_base) return;
  for (ValuePair2Vec::const_iterator it = formation_base->attr.begin();
       it != formation_base->attr.end(); ++it)
    attr[it->v1] += it->v2;
}

int32_t LogicPlayer::ProcessRequestRedEquipStarLevelUp(CSMessageEntry& entry) {
  MessageRequestRedEquipStarLevelUp* message =
      static_cast<MessageRequestRedEquipStarLevelUp*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicItem* equip = this->items().GetItemByUniqueID(message->uid());
  if (!equip || !equip->equip_base()) return ERR_ITEM_NOT_FOUND;
  EquipBase* equip_base = EQUIP_BASE.GetEntryByID(equip->item_id()).get();
  if (!equip_base) return ERR_ITEM_NOT_FOUND;

  int32_t red_equip_star = equip->GetAttr(ITEM_ATTR_STAR_LEVEL);
  int32_t red_equip_exp = equip->GetAttr(ITEM_ATTR_STAR_EXP);
  int32_t red_equip_lucky = equip->GetAttr(ITEM_ATTR_STAR_LUCKY);

  RedequipCostBase* cost_base =
      REDEQUIP_COST_BASE.GetEntryByID(red_equip_star).get();
  if (!cost_base) return ERR_PARAM_INVALID;

  if (!cost_base->exp) return ERR_ITEM_LEVEL_MAX;

  const ValuePair2Vec* need = NULL;
  switch (message->type()) {
    case 1:
      need = &cost_base->cost1;
      break;
    case 2:
      need = &cost_base->cost2;
      break;
    case 3:
      need = &cost_base->cost3;
      break;
    default:
      return ERR_PARAM_INVALID;
  }
  if (!need || need->empty()) return ERR_PARAM_INVALID;

  AddSubItemSet item_set;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_RED_EQUIP_STAR);
  FillCurrencyAndItem<kSub>(modify, item_set, *need);

  for (AddSubItemSet::iterator it = item_set.begin(); it != item_set.end();
       ++it) {
    if (it->item_id == 101) it->item_id = equip_base->equip_piece.v1;
  }

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  int32_t crit_num = 0;
  if ((cost_base->rate1 + red_equip_lucky / 5) > RandomBetween(0, 999)) {
    int32_t add_exp = cost_base->exp1;
    if (cost_base->exp2 > RandomBetween(0, 99)) {
      add_exp *= 1.5;
      crit_num = 1;
    } else if (cost_base->exp3 > RandomBetween(0, 99)) {
      add_exp *= 2;
      crit_num = 2;
    }

    red_equip_exp += add_exp;
    if (red_equip_exp >= cost_base->exp) {
      ++red_equip_star;
      red_equip_exp = 0;
      red_equip_lucky = 0;
    }
  } else {
    red_equip_lucky += cost_base->rate2;
  }

  equip->SetAttr(ITEM_ATTR_STAR_LEVEL, red_equip_star);
  equip->SetAttr(ITEM_ATTR_STAR_EXP, red_equip_exp);
  equip->SetAttr(ITEM_ATTR_STAR_LUCKY, red_equip_lucky);

  NotifyItemSet notify_set;
  notify_set.push_back(equip->uid());

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, &notify_set, entry.head.msgid,
             SYSTEM_ID_RED_EQUIP_STAR);
  this->OnEquipChanged(equip->GetAttr(sy::ITEM_ATTR_EQUIPED_HERO));

  MessageResponseRedEquipStarLevelUp res;
  res.set_uid(message->uid());
  res.set_crit(crit_num);
  this->SendMessageToClient(MSG_CS_RESPONSE_RED_EQUIP_STAR_LEVEL_UP, &res);

  return ERR_OK;
}

void LogicPlayer::HeroComeBack() {
  if (this->IsHeroComeBack()) return;
  int32_t diff_day =
      GetSecondsDiffDays(this->fresh_time_, GetVirtualSeconds()) - 1;
  if (diff_day <= GetSettingValue(hero_back_start_time) || this->level() < 60)
    return;

  this->come_back_info_.set_start_time(GetZeroClock(GetVirtualSeconds()));
  this->come_back_info_.set_end_time(
      come_back_info_.start_time() +
      (diff_day < GetSettingValue(hero_back_max_time)
           ? diff_day
           : GetSettingValue(hero_back_max_time)) *
          86400);
  this->come_back_info_.set_login_record(0);
  this->come_back_info_.set_award_record(0);
  this->come_back_info_.set_diff_day(diff_day);

  server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypePlayerComeBack,
                            this->come_back_info_.SerializeAsString());

  TRACE_LOG(logger)("Player HeroComeBack uid:%ld, diff_day:%d, come back time:%ld ",
                                                                                                    this->uid(), diff_day, come_back_info_.end_time());
  MessageNotifyHeroComeBackInfo notify;
  notify.mutable_info()->CopyFrom(this->come_back_info_);
  this->SendMessageToClient(MSG_CS_NOYIFY_COME_BACK_INFO, &notify);
}

void LogicPlayer::HeroComeBackLogin() {
  this->HeroComeBack();
  if (this->IsHeroComeBack()) {
    int32_t diff_day = GetSecondsDiffDays(this->come_back_info_.start_time(),
                                          GetVirtualSeconds());
    if (diff_day <= GetSettingValue(hero_back_max_time)) {
      int32_t login_record = this->come_back_info_.login_record();
      if (!(login_record & (1 << diff_day))) {
        login_record |= (1 << diff_day);
        this->come_back_info_.set_login_record(login_record);
        server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypePlayerComeBack,
                                  this->come_back_info_.SerializeAsString());
      }
    }
  }
  TRACE_LOG(logger)("Player HeroComeBack Login uid:%ld", this->uid());
  MessageNotifyHeroComeBackInfo notify;
  notify.mutable_info()->CopyFrom(this->come_back_info_);
  this->SendMessageToClient(MSG_CS_NOYIFY_COME_BACK_INFO, &notify);
}

int32_t LogicPlayer::medal_state_count() {
  int32_t count = 0;
  for (size_t i = 0; i < medal_state_.size(); i++) {
    if (medal_state_[i] != 'x') ++count;
  }
  return count;
}

int32_t LogicPlayer::ProcessRequestGetComeBackLoginAward(
    CSMessageEntry& entry) {
  MessageRequestGetComeBackLoginAward* message =
      static_cast<MessageRequestGetComeBackLoginAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!this->IsHeroComeBack()) return ERR_PARAM_INVALID;
  if (message->day() < 1 ||
      message->day() > GetSettingValue(hero_back_max_time))
    return ERR_PARAM_INVALID;

  int count_day = 0;
  for (int32_t i = 1; i <= GetSettingValue(hero_back_max_time); i++) {
    if ((this->come_back_info_.login_record() & (1 << (i - 1)))) count_day++;
  }
  if (count_day < message->day()) return ERR_PARAM_INVALID;
  if (this->come_back_info_.award_record() & (1 << (message->day() - 1)))
    return ERR_PARAM_INVALID;

  HeroreturnBase* base = HERORETURN_BASE.GetEntryByID(message->day()).get();
  if (!base) return ERR_PARAM_INVALID;

  AddSubItemSet item_set;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_HERO_COME_BACK);
  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_HERO_COME_BACK);

  this->come_back_info_.set_award_record(this->come_back_info_.award_record() |
                                         (1 << (message->day() - 1)));
  server->SetPlayerKeyValue(this->uid(), KVStorage::kKVTypePlayerComeBack,
                            this->come_back_info_.SerializeAsString());

  MessageResponseGetComeBackLoginAward res;
  res.set_login_record(this->come_back_info_.login_record());
  res.set_award_record(this->come_back_info_.award_record());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_COME_BACK_LOGIN_AWARD, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestGetComeBackRechargeAward(
    CSMessageEntry& entry) {
  MessageRequestGetComeBackRechargeAward* message =
      static_cast<MessageRequestGetComeBackRechargeAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!IsHeroComeBack()) return ERR_PARAM_INVALID;
  if (message->id() > 31) return ERR_PARAM_INVALID;
  HeroreturnRechargeBase* base =
      HERORETURN_RECHARGE_BASE.GetEntryByID(message->id()).get();
  if (!base) return ERR_PARAM_INVALID;

  int32_t record = daily_counter_[COUNT_TYPE_COME_BACK_RECHARGE_AWARD];
  if (record & (1 << (message->id() - 1))) return ERR_PARAM_INVALID;

  if (base->type == 1) {
    if (!TodayHasRechargeOrder(base->condition)) return ERR_PARAM_INVALID;
  }
  if (base->type == 2) {
    if ((GetTodayRechargeNum(GetVirtualSeconds()) * 10) < base->condition)
      return ERR_PARAM_INVALID;
  }

  AddSubItemSet item_set;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_HERO_COME_BACK);
  FillCurrencyAndItem<kAdd>(modify, item_set, base->award);

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_HERO_COME_BACK);

  record |= (1 << (message->id() - 1));
  daily_counter_[COUNT_TYPE_COME_BACK_RECHARGE_AWARD] = record;
  MessageNotifyBuyCount notify;
  sy::KVPair2* info = notify.add_infos();
  info->set_key(COUNT_TYPE_COME_BACK_RECHARGE_AWARD);
  info->set_value(daily_counter_[COUNT_TYPE_COME_BACK_RECHARGE_AWARD]);
  this->SendMessageToClient(MSG_CS_NOTIFY_BUY_COUNT, &notify);
  MessageSSUpdateBuyCount request;
  request.add_infos()->CopyFrom(*info);
  this->SendMessageToDB(MSG_SS_UPDATE_BUY_COUNT, &request);

  MessageResponseGetComeBackRechargeAward res;
  res.set_id(message->id());
  this->SendMessageToClient(MSG_CS_RESPONSE_GET_COME_BACK_RECHARGE_AWARD, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestSellItemEx(CSMessageEntry& entry) {
  MessageRequestSellItemEx* message =
      static_cast<MessageRequestSellItemEx*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ItemBase* item = ITEM_BASE.GetEntryByID(message->item_id()).get();
  if (!item) return ERR_ITEM_NOT_FOUND;

  MessageResponseSellItemEx res;

  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_BASE);
  AddSubItemSet item_set;
  for (int32_t i = 0; i < message->count(); ++i) {
    FillCurrencyAndItem<kAdd>(modify, item_set, item->sell);
  }
  FillToKVPair2(&modify, &item_set, res.mutable_items());
  item_set.push_back(ItemParam(message->item_id(), -message->count()));

  CHECK_RET(CheckCurrency(modify));
  CHECK_RET(CheckItem(&item_set, NULL, NULL));

  UpdateCurrency(modify);
  ObtainItem(&item_set, NULL, NULL, entry.head.msgid, SYSTEM_ID_BASE);

  this->SendMessageToClient(MSG_CS_RESPONSE_SELL_ITEM_EX, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestMedalActiveAchi(CSMessageEntry& entry) {
  MessageRequestMedalActiveAchi* message =
      static_cast<MessageRequestMedalActiveAchi*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MedalFormationBase* base =
      MEDAL_FORMATION_BASE.GetEntryByID(message->id()).get();
  if (!base) return ERR_PARAM_INVALID;

  if (base->id() > this->medal_star_) return ERR_PARAM_INVALID;

  this->medal_achi_ = base->id();

  MessageResponseMedalActiveAchi res;
  res.set_id(message->id());
  this->SendMessageToClient(MSG_CS_RESPONSE_MEDAL_ACTIVE_ACHI, &res);

  MessageUpdateMedalCopyID update;
  update.set_medal_achi(this->medal_achi_);
  this->SendMessageToDB(MSG_SS_UPDATE_MEDAL_COPY_ID, &update);

  this->CalcTacticAttr(kNotifyAll, SYSTEM_ID_MEDAL, 0);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPearlHarborFight(CSMessageEntry& entry) {
  MessageRequestPearlHarborFight* message =
      static_cast<MessageRequestPearlHarborFight*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!server->IsPearlHarborTime()) return ERR_PARAM_INVALID;

  const CopyBase* copy_base = COPY_BASE.GetEntryByID(63700).get();
  if (!copy_base) return ERR_COPY_INVALID;

  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_ARMY_NOT_JOINED;

  PearlHarborInfo& pearl_info = army->PearlHarborInfo();
  if (pearl_info.invade_count() >= GetSettingValue(pearlharbor_missing_enemy))
    return ERR_PARAM_INVALID;

  if (this->daily_counter_[COUNT_TYPE_PEARL_HARBOR_BUY_COUNT] +
          GetSettingValue(pearlharbor_battle_num) <=
      this->daily_counter_[COUNT_TYPE_PEARL_HARBOR_FIGHT_COUNT])
    return ERR_PARAM_INVALID;

  if (message->index() >= pearl_info.monster_hp_size())
    return ERR_PARAM_INVALID;

  PearlHarborMonsterHP* monster_info =
      pearl_info.mutable_monster_hp(message->index());
  if (!monster_info) return ERR_PARAM_INVALID;

  MonsterGroupBase* monster_group =
      MONSTER_GROUP_BASE.GetEntryByID(monster_info->monster_group()).get();
  if (!monster_group) return ERR_PARAM_INVALID;

  const std::vector<int32_t>& buff =
      Setting::GetValue1(Setting::pearlharbor_buff_effect);
  OtherPlayerInfo player_info;
  this->FillOtherPlayerInfo(&player_info);
  if (pearl_info.buff_end_time() < GetVirtualSeconds()) {
    for (int i = 0; i < player_info.battle_pos1_size(); ++i) {
      const sy::PositionInfo& pos = player_info.battle_pos1(i);
      for (int j = 0; j < player_info.heros_size(); ++j) {
        if (player_info.heros(j).uid() == pos.hero_uid()) {
          sy::HeroInfo* hero = player_info.mutable_heros(j);
          hero->set_attr1(buff[0], hero->attr1(buff[0]) + buff[1]);
          break;
        }
      }
    }
  }

  PK pk(copy_base);
  pk.InitPlayer(&player_info, true, 1, 1, 1, 1);
  monster_info->mutable_hp()->Resize(6, 0);
  pk.InitMonsterInfo(monster_group->carrier_info(), monster_group->hero_info(),
                     monster_info->mutable_hp()->begin());
  pk.GeneratePVPReport();
  if (server_config->report()) LogReportToFile(pk.report);

  std::vector<int64_t> current_hp;
  current_hp.resize(6, 0);
  int64_t all_current_hp = 0;
  int64_t all_damage = 0;
  for (int32_t i = 0; i < 6; i++) {
    PkHero* hero = pk.b.GetHeroByPos(i + 1);
    if (hero) {
      current_hp[i] = hero->hp();
      all_current_hp += current_hp[i];
      all_damage += monster_info->hp(i) - current_hp[i];
      monster_info->set_hp(i, current_hp[i]);
    } else {
      INFO_LOG(logger)("PearlHarbor Pos:%d, invalid", i + 1);
    }
  }

  PearlharborCopyBase* pearl_harbor_copy_base =
      PEARLHARBOR_COPY_BASE.GetEntryByID(pearl_info.war_zone() * 10 +
                                         pearl_info.batch())
          .get();
  if (!pearl_harbor_copy_base) return ERR_PARAM_INVALID;

  int32_t score = all_damage / 10000;
  pearl_info.set_score(pearl_info.score() + score);
  int32_t pearl_harbor_score = 0;
  for (int32_t k = 0; k < pearl_info.player_score_size(); k++) {
    if (pearl_info.player_score(k).key() == this->uid()) {
      pearl_info.mutable_player_score(k)
          ->set_value(pearl_info.player_score(k).value() + score);
      pearl_harbor_score = pearl_info.player_score(k).value();
      break;
    }
  }
  for (int32_t k = 0; k < pearl_info.player_today_score_size(); k++) {
    if (pearl_info.player_today_score(k).key() == this->uid()) {
      pearl_info.mutable_player_today_score(k)
          ->set_value(pearl_info.player_today_score(k).value() + score);
      break;
    }
  }

  MessageResponsePearlHarborFight res;
  ModifyCurrency modify(entry.head.msgid, SYSTEM_ID_PEARL_HARBOR);
  int32_t union_count =
      RandomBetween(pearl_harbor_copy_base->reward_challenge.v1,
                    pearl_harbor_copy_base->reward_challenge.v2);
  modify[MONEY_KIND_UNION] += union_count;
  res.set_fight_union(union_count);
  if (all_current_hp <= 0) {
    union_count = 0;
    for (size_t i = 0; i < pearl_harbor_copy_base->reward_kill2.size(); i++) {
      if (pearl_harbor_copy_base->reward_kill2[i].v1 ==
          monster_info->quality()) {
        union_count = pearl_harbor_copy_base->reward_kill2[i].v2;
        break;
      }
    }
    modify[MONEY_KIND_UNION] += union_count;
    res.set_last_hit_union(union_count);
  }

  CHECK_RET(CheckCurrency(modify));
  UpdateCurrency(modify);

  RANK_LIST.OnPearlHarborArmy(army);
  RANK_LIST.OnPearlHarborPlayer(this, pearl_harbor_score);

  army->SavePearlHarbor();

  this->UpdateBuyCount(COUNT_TYPE_PEARL_HARBOR_FIGHT_COUNT, 1);
  this->SendMessageToClient(MSG_CS_RESPONSE_FIGHT_REPORT, &pk.response);

  res.set_index(message->index());
  res.set_score(pearl_harbor_score);
  res.set_army_score(pearl_info.score());
  this->SendMessageToClient(MSG_CS_RESPONSE_PEARL_HARBOR_FIGHT, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPearlHarborGetInfo(CSMessageEntry& entry) {
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_PARAM_INVALID;

  MessageResponsePearlHarborGetInfo res;
  res.mutable_info()->CopyFrom(army->PearlHarborInfo());
  this->SendMessageToClient(MSG_CS_RESPONSE_PEARL_HARBOR_GET_INFO, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPearlHarborWarZone(CSMessageEntry& entry) {
  MessageRequestPearlHarborWarZone* message =
      static_cast<MessageRequestPearlHarborWarZone*>(entry.get());
  if (!message) return ERR_INTERNAL;

  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_PARAM_INVALID;
  const ArmyMemberInfo* member_info = army->GetArmyMemberInfo(this->uid());
  if (!member_info) return ERR_ARMY_MEMBER_NOT_FOUND;
  if (member_info->position() < sy::ARMY_POSITION_VP)
    return ERR_ARMY_PERMISSION;

  PearlHarborInfo& pearl_info = army->PearlHarborInfo();
  pearl_info.set_tomorrow_type(message->type());

  MessageResponsePearlHarborWarZone res;
  res.set_type(pearl_info.tomorrow_type());
  this->SendMessageToClient(MSG_CS_RESPONSE_PEARL_HARBOR_WAR_ZONE, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPearlHarborStartBuff(CSMessageEntry& entry) {
  if (!server->IsPearlHarborTime()) return ERR_PARAM_INVALID;
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_PARAM_INVALID;

  PearlHarborInfo& pearl_info = army->PearlHarborInfo();
  int32_t buff_end_time = pearl_info.buff_end_time() > GetVirtualSeconds()
                              ? pearl_info.buff_end_time()
                              : GetVirtualSeconds();
  buff_end_time += GetSettingValue(pearlharbor_time);

  pearl_info.set_buff_end_time(buff_end_time);

  MessageResponsePearlHarborStartBuff res;
  res.set_end_time(pearl_info.buff_end_time());
  this->SendMessageToClient(MSG_CS_RESPONSE_PEARL_HARBOR_START_BUFF, &res);

  return ERR_OK;
}

int32_t LogicPlayer::ProcessRequestPearlHarborArmyScore(CSMessageEntry& entry) {
  Army* army = server->GetArmyByID(this->army_id_);
  if (!army) return ERR_PARAM_INVALID;

  std::vector<sy::ArmyMemberInfo>& members = army->members();
  MessageResponsePearlHarborArmyScore res;
  res.set_army_score(army->PearlHarborInfo().score());
  for (size_t i = 0; i < members.size(); i++) {
    PearlHarborScoreInfo* info = res.add_member_score();
    info->set_uid(members[i].player_id());
    info->set_name(members[i].name());
    info->set_vip(members[i].vip_level());
    const sy::PearlHarborInfo& peral_info = army->PearlHarborInfo();
    for (int32_t k = 0; k < peral_info.player_score_size(); k++) {
      if (peral_info.player_score(k).key() == members[i].player_id()) {
        info->set_score(peral_info.player_score(k).value());
        break;
      }
    }
    for (int32_t k = 0; k < peral_info.player_today_score_size(); k++) {
      if (peral_info.player_today_score(k).key() == members[i].player_id()) {
        info->set_today_score(peral_info.player_today_score(k).value());
        break;
      }
    }
    info->set_position(members[i].position());
  }
  this->SendMessageToClient(MSG_CS_RESPONSE_PEARL_HARBOR_ARMY_SCORE, &res);

  return ERR_OK;
}
