#include "record_player.h"
#include <array.h>
#include <cpp/server_message.pb.h>
#include <array_stream.h>
#include <slice.h>
#include <stdio.h>
#include <str_util.h>
#include <string.h>
#include "message_handler.h"
#include "server.h"
#include "callback.h"

using namespace sy;
using namespace intranet;
using google::protobuf::RepeatedPtrField;
using google::protobuf::RepeatedField;

//decode int array
template <typename T, typename V>
inline void PushBack(T& t, const V& v) {
  t.Add(v);
}

inline void PushBack(std::vector<int32_t>& t, const int64_t& v) {
  t.push_back(v);
}

inline void PushBack(std::vector<int64_t>& t, const int64_t& v) {
  t.push_back(v);
}

template <typename T, int N>
inline void PushBack(Array<T, N>& t, const int64_t& v) {
  t.push_back(v);
}

template <typename T>
inline void DecodeIntArray(T& out, const std::string& str) {
  if (str.length() < 1) return;

  char* line = NULL, *ptr = NULL;
  line = strtok_r(const_cast<char*>(str.c_str()), ",", &ptr);
  while (line) {
    PushBack(out, int64_t(atoll(line)));
    line = strtok_r(NULL, ",", &ptr);
  }
}

template <typename T, int N>
inline void EncodeIntArray(const T& in, ArrayStream<N>& stream) {
  typedef typename T::const_iterator const_iterator;
  const_iterator iter = in.begin();
  if (iter == in.end()) return;
  stream.Append("%ld", int64_t(*iter));
  for (iter = iter + 1; iter != in.end(); ++iter) {
    stream.Append(",%ld", int64_t(*iter));
  }
}

//key/value的特征
template <typename T>
struct KeyValueTrait;

template <>
struct KeyValueTrait<sy::Item> {
  typedef RepeatedPtrField<sy::ItemAttribute>::const_iterator const_iterator;

  static inline const_iterator begin(const sy::Item& item) { return item.attr().begin(); }
  static inline const_iterator end(const sy::Item& item) { return item.attr().end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%d", iter->key(), iter->value());
  }

  static inline void decode(const char* str, sy::Item& item) {
    int32_t key = 0, value = 0;
    if (sscanf(str, "%d:%d", &key, &value) >= 2) {
      sy::ItemAttribute *info = item.add_attr();
      info->set_key(key);
      info->set_value(value);
    }
  }
};

template <>
struct KeyValueTrait<std::vector<sy::CarrierCopy> > {
  typedef std::vector<sy::CarrierCopy> ValueType;
  typedef ValueType::const_iterator const_iterator;

  static inline const_iterator begin(const ValueType& item) { return item.begin(); }
  static inline const_iterator end(const ValueType& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d,%d,%d,%ld,%d", iter->index(), iter->robot_id(),
                  iter->level(), iter->fight_attr(), iter->carrier_id());
    for (int32_t i = 0; i < iter->heros_size(); ++i) {
      stream.Append(",%d", iter->heros(i));
    }
  }

  static inline void decode(const char* str, ValueType& item) {
    int64_t fight_attr = 0;
    int32_t index = 0, robot_id = 0, level = 0, carrier_id = 0;
    int32_t h1 = 0, h2 = 0, h3 = 0, h4 = 0, h5 = 0, h6 = 0;
    if (sscanf(str, "%d,%d,%d,%ld,%d,%d,%d,%d,%d,%d,%d", &index, &robot_id,
               &level, &fight_attr, &carrier_id, &h1, &h2, &h3, &h4, &h5,
               &h6) >= 6) {
      sy::CarrierCopy copy;
      copy.set_index(index);
      copy.set_level(level);
      copy.set_robot_id(robot_id);
      copy.set_carrier_id(carrier_id);
      copy.set_fight_attr(fight_attr);
      copy.add_heros(h1);
      copy.add_heros(h2);
      copy.add_heros(h3);
      copy.add_heros(h4);
      copy.add_heros(h5);
      copy.add_heros(h6);
      item.push_back(copy);
    }
  }
};

template <>
struct KeyValueTrait<sy::CarrierCopyInfo > {
  typedef sy::CarrierCopyInfo ValueType;

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const ValueType& iter) {
    stream.Append("%d,%d,%d,%d,%d", iter.layer(), iter.passed_copy(), iter.current_index(), iter.award_count(), iter.count());
    for (int32_t i = 0; i < iter.left_hp_size(); ++i) {
      stream.Append(",%d", iter.left_hp(i));
    }
    stream.Append(",%d,%d", iter.item_count(), iter.item_max_count());
  }

  static inline void decode(const char* str, ValueType& item) {
    int32_t layer= 0, passed_copy= 0, current_index= 0, award_count= 0, count=0;
    int32_t h1 = 0, h2 = 0, h3 = 0, h4 = 0, h5 = 0, h6 = 0;
    int32_t item_count = 0;
    int32_t item_max_count = 0;
    if (sscanf(str, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &layer, &passed_copy,
               &current_index, &award_count, &count, &h1, &h2, &h3, &h4, &h5,
               &h6, &item_count, &item_max_count) >= 5) {
      item.set_layer(layer);
      item.set_passed_copy(passed_copy);
      item.set_award_count(award_count);
      item.set_current_index(current_index);
      item.set_count(count);
      item.add_left_hp(h1);
      item.add_left_hp(h2);
      item.add_left_hp(h3);
      item.add_left_hp(h4);
      item.add_left_hp(h5);
      item.add_left_hp(h6);
      item.set_item_count(item_count);
      item.set_item_max_count(item_max_count);
    }
  }
};


template <>
struct KeyValueTrait<RepeatedPtrField<sy::PositionInfo> > {
  typedef RepeatedPtrField<sy::PositionInfo> ValueType;
  typedef ValueType::const_iterator const_iterator;

  static inline const_iterator begin(const ValueType& item) { return item.begin(); }
  static inline const_iterator end(const ValueType& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%ld", iter->position(), iter->hero_uid());
  }

  static inline void decode(const char* str, ValueType& item) {
    int32_t key = 0, value = 0;
    if (sscanf(str, "%d:%d", &key, &value) >= 2) {
      sy::PositionInfo *info = item.Add();
      info->set_position(key);
      info->set_hero_uid(value);
    }
  }
};


template <>
struct KeyValueTrait<VectorMap<int32_t, int32_t> > {
  typedef VectorMap<int32_t, int32_t> ValueType;
  typedef ValueType::const_iterator const_iterator;

  static inline const_iterator begin(const ValueType& item) { return item.begin(); }
  static inline const_iterator end(const ValueType& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%d", iter->first, iter->second);
  }

  static inline void decode(const char* str, ValueType& item) {
    int32_t key = 0, value = 0;
    if (sscanf(str, "%d:%d", &key, &value) >= 2) {
      item[key] = value;
    }
  }
};

template <>
struct KeyValueTrait<RepeatedPtrField<sy::ShopCommodityInfo> > {
  typedef RepeatedPtrField<sy::ShopCommodityInfo> ValueType;
  typedef ValueType::const_iterator const_iterator;

  static inline const_iterator begin(const ValueType& item) {
    return item.begin();
  }
  static inline const_iterator end(const ValueType& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%d", iter->commodity_id(), iter->bought_count());
  }

  static inline void decode(const char* str, ValueType& item) {
    int32_t key = 0, value = 0;
    if (sscanf(str, "%d:%d", &key, &value) >= 2) {
      sy::ShopCommodityInfo* info = item.Add();
      info->set_commodity_id(key);
      info->set_bought_count(value);
    }
  }
};

template <>
struct KeyValueTrait<RepeatedPtrField<sy::KVPair2> > {
  typedef RepeatedPtrField<sy::KVPair2> ValueType;
  typedef ValueType::const_iterator const_iterator;

  static inline const_iterator begin(const ValueType& item) {
    return item.begin();
  }
  static inline const_iterator end(const ValueType& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%d", iter->key(), iter->value());
  }

  static inline void decode(const char* str, ValueType& item) {
    int32_t key = 0, value = 0;
    if (sscanf(str, "%d:%d", &key, &value) >= 2) {
      sy::KVPair2* info = item.Add();
      info->set_key(key);
      info->set_value(value);
    }
  }
};

template <>
struct KeyValueTrait<std::vector<sy::CopyStarInfo> > {
  typedef std::vector<sy::CopyStarInfo> ValueType;
  typedef ValueType::const_iterator const_iterator;

  static inline const_iterator begin(const ValueType& item) { return item.begin(); }
  static inline const_iterator end(const ValueType& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%d", iter->copy_id(), iter->star());
  }

  static inline void decode(const char* str, ValueType& vec) {
    int32_t copy_id = 0, star = 0;
    if (sscanf(str, "%d:%d", &copy_id, &star) >= 2) {
      sy::CopyStarInfo info;
      info.set_copy_id(copy_id);
      info.set_star(star);
      vec.push_back(info);
    }
  }
};

template <>
struct KeyValueTrait<std::vector<sy::CopyCount> > {
  typedef std::vector<sy::CopyCount> ValueType;
  typedef ValueType::const_iterator const_iterator;

  static inline const_iterator begin(const ValueType& item) { return item.begin(); }
  static inline const_iterator end(const ValueType& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%d", iter->copy_id(), iter->count());
  }

  static inline void decode(const char* str, ValueType& vec) {
    int32_t copy_id = 0, count = 0;
    if (sscanf(str, "%d:%d", &copy_id, &count) >= 2) {
      sy::CopyCount info;
      info.set_copy_id(copy_id);
      info.set_count(count);
      vec.push_back(info);
    }
  }
};

template<>
struct KeyValueTrait<std::vector<sy::CopyProgress> > {
  typedef std::vector<sy::CopyProgress>::const_iterator const_iterator;

  static inline const_iterator begin(const std::vector<sy::CopyProgress>& item) { return item.begin(); }
  static inline const_iterator end(const std::vector<sy::CopyProgress>& item) { return item.end(); }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d:%d:%d", iter->copy_type(), iter->chapter(), iter->copy_id());
  }

  static inline void decode(const char* str, std::vector<sy::CopyProgress>& vec) {
    int32_t copy_type = 0, chapter = 0, copy_id = 0;
    if (sscanf(str, "%d:%d:%d", &copy_type, &chapter, &copy_id) >= 3) {
      sy::CopyProgress info;
      info.set_copy_id(copy_id);
      info.set_chapter(chapter);
      info.set_copy_type(copy_type);
      vec.push_back(info);
    }
  }
};

template <>
struct KeyValueTrait<std::vector<sy::PatrolInfo> > {
  typedef std::vector<sy::PatrolInfo>::const_iterator const_iterator;

  static inline const_iterator begin(const std::vector<sy::PatrolInfo>& item) {
    return item.begin();
  }
  static inline const_iterator end(const std::vector<sy::PatrolInfo>& item) {
    return item.end();
  }

  template <int32_t N>
  static inline void encode(ArrayStream<N>& stream, const_iterator iter) {
    stream.Append("%d,%ld,%d,%d,%ld,%d,%d,", iter->patrol_id(), iter->ship_uid(),
                  iter->patrol_type(), iter->patrol_mode(),
                  iter->patrol_start_time(), iter->patrol_level(),iter->patrol_awards_size());
    for (int32_t i = 0; i < iter->patrol_awards_size(); ++i) {
      const KVPair2& pair = iter->patrol_awards(i);
      stream.Append("%d|%d", pair.key(), pair.value());
      if (i != iter->patrol_awards_size() - 1) stream.Append(",");
    }
  }

  static inline void decode(const char* str,
                            std::vector<sy::PatrolInfo>& item) {
    std::vector<std::string> v;
    SplitString(str, v, ",");

    if (v.size() < 7) return;

    sy::PatrolInfo info;
    info.set_patrol_id(atoi(v[0].c_str()));
    info.set_ship_uid(atoll(v[1].c_str()));
    info.set_patrol_type(atoi(v[2].c_str()));
    info.set_patrol_mode(atoi(v[3].c_str()));
    info.set_patrol_start_time(atoll(v[4].c_str()));
    info.set_patrol_level(atoi(v[5].c_str()));
    int32_t award_size = atoi(v[6].c_str());

    if (static_cast<int32_t>(v.size()) < 7 + award_size) return;

    for (int32_t i = 0; i < award_size; i++) {
      int32_t key = 0, value = 0;
      if (sscanf(v[7 + i].c_str(), "%d|%d", &key, &value) >= 2) {
        sy::KVPair2* pair = info.add_patrol_awards();
        pair->set_key(key);
        pair->set_value(value);
      }
    }

    item.push_back(info);
  }
};

template <typename T, int N>
inline void EncodeKeyValuePair(const T& item, ArrayStream<N>& stream) {
  typename KeyValueTrait<T>::const_iterator begin = KeyValueTrait<T>::begin(item);
  typename KeyValueTrait<T>::const_iterator end = KeyValueTrait<T>::end(item);
  for (typename KeyValueTrait<T>::const_iterator iter = begin; iter != end; ++iter) {
    if (iter != begin) stream.Append(";");
    KeyValueTrait<T>::encode(stream, iter);
  }
}

template <typename T>
struct DecodeKeyValuePair {
  DecodeKeyValuePair(T& item) : item(item) {}

  void operator()(StringSlice str) {
    if (str.size()) {
      KeyValueTrait<T>::decode(str.data(), item);
    }
  }

  T& item;
};

RecordPlayer::RecordPlayer(int64_t uid)
    : Player(uid),
      load_complete_(false),
      last_mail_id_(0),
      last_server_mail_id_(0),
      max_fight_attr_(0),
      pk_rank_times_(0),
      last_pk_rank_(0),
      pk_rank_reward_time_(0),
      pk_max_rank_(0),
      last_pk_time_(0),
      patrol_total_time_(0),
      army_id_(0),
      army_leave_time_(0),
      month_card_(0),
      big_month_card_(0),
      life_card_(0),
      medal_copy_id_(0),
      medal_star_(0),
      medal_achi_(0) {
  this->last_login_time_ = GetSeconds();
  this->player_.set_uid(uid);
  this->tower_state_.set_max_order(0);
  this->tower_state_.set_max_star(0);
  this->tower_state_.set_current_order(0);
  this->tower_state_.set_current_star(0);
  this->tower_state_.set_award(0);
  this->tower_state_.set_buff_star(0);
  this->tower_state_.set_copy_star(0);
  this->tower_state_.set_random_buff("");
  this->tower_state_.set_max_star_order(0);
  bzero(&this->month_card_1_, sizeof(this->month_card_1_));
  bzero(&this->month_card_2_, sizeof(this->month_card_2_));
  bzero(&this->weekly_card_, sizeof(this->weekly_card_));
}

RecordPlayer::~RecordPlayer() {}

bool RecordPlayer::can_be_delete() {
  if (GetSeconds() - this->last_active_time() >= 90 * 60) return true;
  return false;
}

void RecordPlayer::SendMessageToPlayer(int16_t msgid, Message* msg) {
  const boost::shared_ptr<TcpSession>& session = this->session_.lock();
  if (session) {
    server->SendPlayerMessage(session.get(), this->uid(), msgid, msg);
  } else {
    ERROR_LOG(logger)("SendPlayerMessage fail, PlayerID:%ld, MSG:0x%04X", this->uid(), msgid);
  }
}

void RecordPlayer::ExecSqlAsync(const std::string& sql) {
  server->PushSql(this->uid(), sql);
}

int32_t UpdateRoleName(MySqlConnection& conn, uint32_t server_id,
                       const std::string& open_id, const std::string& name,
                       int64_t uid) {
  const std::string& escape_openid = conn.EscapeString(open_id);
  const std::string& escape_name =
      conn.EscapeString(name.c_str(), name.length());

  ArrayStream<1024 * 8> stream;
  stream.Append("select count(*) from role_name where uid=%ld", uid);
  const boost::shared_ptr<ResultSet>& result =
      conn.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }
  stream.clear();
  if (result && result->IsValid()) {
    if (result->at(0).to_int32() <= 0) {
      stream.Append(
          "insert into role_name(server, openid, name, uid) values (%u,'%s', "
          "'%s', %ld)",
          server_id, escape_openid.c_str(), escape_name.c_str(), uid);
    } else {
      stream.Append("update role_name set name='%s' where server=%u and openid='%s'",
                    escape_name.c_str(), server_id, escape_openid.c_str());
    }
  }

  INFO_LOG(sql_log)("%s", stream.c_str());
  int32_t insert_result = conn.ExecSql(stream.c_str(), stream.size());
  if (insert_result <= 0) {
    ERROR_LOG(logger)("Insert Error CreatePlayer Player:%ld, Name:%s exist", uid, escape_name.c_str());
    return ERR_PLAYER_NAME_EXIST;
  }
  return ERR_OK;
}

std::pair<int32_t, int32_t> RecordPlayer::CreatePlayerSync(
    intranet::MessageSSRequestCreatePlayer& msg, MySqlConnection& conn) {
  if (this->loaded()) return std::pair<int32_t, int32_t>(ERR_OK, 0);

  PlayerInfo& player_info = *msg.mutable_info();
  ArrayStream<1024 * 8> stream;

  const std::string& escape_openid = conn.EscapeString(player_info.openid());
  const std::string& escape_name = conn.EscapeString(
      player_info.name().c_str(), player_info.name().length());
  //插入名字到role_name表
  time_t begin = GetMilliSeconds();
  int32_t result =
      UpdateRoleName(conn, player_info.server(), player_info.openid(),
                     player_info.name(), player_info.uid());
  time_t end = GetMilliSeconds();
  if (end - begin > LOG_SQL_TIME * 2) {
    WARN_LOG(logger)("ExecSqlTime:%ldms, Sql:%s", end - begin, "CreateName");
  }
  if (result) return std::pair<int32_t, int32_t>(result, __LINE__);

  //插入玩家到玩家表里面去
  {
    stream.clear();
    stream.Append(
        "insert into player_%ld(uid, openid, server, name, create_time, "
        "last_login_time, oil, last_oil_time, level, energy, "
        "last_energy_time, fresh_time, avatar, last_server_mail_id, "
        "login_days, channel, login_channel, device_id) "
        "values(%ld, '%s', "
        "%u, '%s', %ld, %ld, %d, %d, %d, %d, %d, %ld, %d, %ld, %d, '%s', '%s', "
        "'%s') "
        "on "
        "duplicate "
        "key "
        "update name=values(name), last_login_time=values(last_login_time), "
        "level=values(level), energy=values(energy), "
        "last_energy_time=values(last_energy_time), avatar=values(avatar), "
        "last_server_mail_id=values(last_server_mail_id), "
        "login_days=values(login_days), channel=values(channel)",
        player_info.uid() % RECORD_TABLE_COUNT, player_info.uid(),
        escape_openid.c_str(), player_info.server(), escape_name.c_str(),
        player_info.create_time(), GetSeconds(), player_info.oil(),
        player_info.last_oil_time(), player_info.level(), player_info.energy(),
        player_info.last_energy_time(), GetSeconds(), player_info.avatar(),
        msg.last_server_mail_id(), player_info.login_days(),
        player_info.channel().c_str(), player_info.channel().c_str(),
        player_info.device_id().c_str());
  }
  result = conn.ExecSql(stream.c_str(), stream.size());
  if (result < 0) {
    ERROR_LOG(sql_log)("%s", stream.c_str());
    return std::pair<int32_t, int32_t>(ERR_PLAYER_NAME_EXIST, __LINE__);
  }
  INFO_LOG(sql_log)("%s", stream.c_str());

  //初始化数据,这样就不需要到数据库再次Load
  //初始化角色数据
  this->player_.CopyFrom(msg.info());
  this->last_server_mail_id_ = msg.last_server_mail_id();
  //初始化研发信息
  this->hero_research_.set_last_time(0);
  this->hero_research_.set_hero_id(0);
  this->hero_research_.set_item_count(0);
  this->hero_research_.set_money_count(0);
  this->hero_research_.set_money_count2(0);
  this->hero_research_.set_last_free_time(0);
  this->hero_research_.set_rd_count(0);
  this->hero_research_.set_last_free_rd_time(0);
  this->hero_research_.set_day_free_rd_count(0);
  //初始化围剿BOSS信息
  this->dstrike_.set_level(0);
  this->dstrike_.set_update_time(GetSeconds());
  this->dstrike_.set_merit(0);
  this->dstrike_.set_damage(0);
  this->dstrike_.set_daily_award(0);
  this->fresh_time_ = GetSeconds();

  //!!!
  //剩下的东西都可以异步插入到数据库里面去
  //!!!
  this->ExecSqlAsync("BEGIN");

  //舰船
  for (int32_t i = 0; i < msg.ships_size(); ++i) {
    stream.clear();
    sy::HeroInfo* info = msg.mutable_ships(i);
    this->UpdateHero(info, msg.tid(), msg.system(), msg.msgid());
  }
  //航母
  stream.clear();
  if (msg.has_carrier()) this->UpdateCarrier(msg.mutable_carrier(), msg.tid());

  //阵型
  {
    intranet::MessageSSRequestUpdateTacticInfo m;
    m.mutable_infos()->CopyFrom(msg.tactic().infos());
    m.mutable_battle_pos()->CopyFrom(msg.tactic().battle_pos());
    this->UpdateTactic(m);
  }

  //初始化道具
  int32_t count = msg.items_size();
  for (int32_t i = 0; i < count; ++i) {
    sy::Item* item = msg.mutable_items(i);
    this->UpdateItem(item, msg.tid(), msg.system(), msg.msgid());
  }
  //默认航母上阵
  this->current_carrier_.set_carrier_id(msg.carrier().carrier_id());
  stream.clear();
  stream.Append("update player_%ld set current_carrier_id=%d where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT, msg.carrier().carrier_id(),
                this->uid());
  this->ExecSqlAsync(stream.str());

  this->ExecSqlAsync("COMMIT");

  //创建完成
  this->load_complete_ = true;
  this->SendAllInfoToClient(0);
  MessageSSResponseCreatePlayer response;
  response.set_name(this->player_.name());
  this->SendMessageToPlayer(MSG_SS_RESPONSE_CREATE_PLAYER, &response);
  return std::pair<int32_t, int32_t>(ERR_OK, 0);
}

void RecordPlayer::LoadPlayerAsync(int32_t msgid) {
  server->PushAsyncClosure(this->uid(),
      boost::shared_ptr<ClosureLoadSinglePlayer>(new ClosureLoadSinglePlayer(
                       this, server->GetAsyncConn(this->uid()), msgid)));
}

int32_t RecordPlayer::LoadRecharge(MySqlConnection& conn) {
  ArrayStream<1024> stream;
  stream.Append(
      "select `time`, `money`, `goodid` from recharge where "
      "player_id=%ld",
      this->uid());
  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());

  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  int64_t seconds = GetSeconds() - 24 * 3600 * 31;
  int32_t count = 0;

  sy::RechargeInfo item;
  this->recharge_.clear();
  while (result && result->IsValid()) {
    item.Clear();
    item.set_recharge_time(result->at(0).to_int32());
    item.set_money(result->at(1).to_int32());
    item.set_goodid(result->at(2).to_int32());

    if (item.recharge_time() <= seconds) {
      count++;
    } else {
      this->recharge_.push_back(item);
    }
    result->Next();
  }

  if (count) {
    stream.clear();
    stream.Append("delete from recharge where player_id=%ld and `time` < %ld",
                  this->uid(), seconds);
    this->ExecSqlAsync(stream.str());
  }
  return ERR_OK;
}

std::pair<int32_t, int32_t> RecordPlayer::LoadPlayerSync(MySqlConnection& conn) {
  if (this->loaded()) {
    return std::pair<int32_t, int32_t>(ERR_OK, 0);
  }
  int32_t result = ERR_OK;
  int32_t line = 0;
  do {
    if ((result = this->LoadItemInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadHeroInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadTacticInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadCopyInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadMailInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadShopInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadRewardInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadFriendInfo(conn))) { line = __LINE__; break; }
    if ((result = this->LoadReportAbstract(conn))) { line = __LINE__; break; }
    if ((result = this->LoadCarrier(conn))) { line = __LINE__; break; }
    if ((result = this->LoadRecharge(conn))) { line = __LINE__; break; }
    if ((result = this->LoadActivityRecord(conn))) { line = __LINE__; break; }

    if ((result = this->LoadPlayerInfo(conn))) { line = __LINE__; break; }
  } while (false);

  if (!result) {
    this->load_complete_ = true;
  } else {
    if (result >= RECORD_SQL_ERROR){
      ERROR_LOG(logger)("LoadPlayer:%ld Fail, result:%d, line:%d", this->uid(), result, line);
    } else {
      TRACE_LOG(logger)("LoadPlayer:%ld Fail, result:%d, line:%d", this->uid(), result, line);
    }
  }
  return std::make_pair(result, line);
}

int32_t RecordPlayer::LoadPlayerInfo(MySqlConnection& conn) {
  ArrayStream<1024> stream;
  stream.Append(
      "select `openid`, `server`, `name`, `level`, `exp`, `vip_level`"
      ", `vip_exp`, `coin`, `money`, `last_mail_id`, `oil`, `last_oil_time`"
      ", current_carrier_id, carrier_plane_attr, carrier_plane"
      ", research_hero_time, research_hero_id, research_hero_item_count "
      ", research_hero_money_count"
      ", energy, last_energy_time"
      ", free_hero_time, research_hero_money_count2"
      ", fresh_time"
      ", carrier_extra_damage"
      ", prestige, hero, plane"
      ", `muscle`, `exploit`, `union`, `truce_time`"
      ", dstrike_level, dstrike_time, dstrike_merit, dstrike_damage, sign_id"
      ", sign_time"
      ", avatar ,rank_id "
      ", create_time"
      ", status, status_time, flag"
      ", last_server_mail_id"
      ", research_hero_rd_count, research_hero_last_free_rd_time"
      ", research_hero_day_free_rd_count, dialog_id"
      ", dstrike_daily_award, client_flag, story_id"
      ", total_recharge"
      ", last_login_time"
      ", login_days"
      ", channel"
      " from player_%ld where uid=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result =
      conn.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  if (result && result->IsValid()) {
    this->player_.set_openid(result->at(0).to_str());
    this->player_.set_server(result->at(1).to_int32());
    this->player_.set_name(result->at(2).to_c_str());
    this->player_.set_level(result->at(3).to_int32());
    this->player_.set_exp(result->at(4).to_int64());
    this->player_.set_vip_level(result->at(5).to_int32());
    this->player_.set_vip_exp(result->at(6).to_int32());
    this->player_.set_coin(result->at(7).to_int64());
    this->player_.set_money(result->at(8).to_int64());
    this->last_mail_id_ = result->at(9).to_int64();
    this->player_.set_oil(result->at(10).to_int32());
    this->player_.set_last_oil_time(result->at(11).to_int32());
    this->current_carrier_.set_carrier_id(result->at(12).to_int32());
    const std::string& carrier_plane_attr = result->at(13).to_str();
    if (carrier_plane_attr.length()) {
      DecodeIntArray(*this->current_carrier_.mutable_attr1(),
                     carrier_plane_attr);
      this->current_carrier_.mutable_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
    }
    const std::string& carrier_plane = result->at(14).to_str();
    if (carrier_plane.length()) {
      DecodeIntArray(*this->current_carrier_.mutable_plane_id(), carrier_plane);
    }

    //船只研发信息
    this->hero_research_.set_last_time(result->at(15).to_int32());
    this->hero_research_.set_hero_id(result->at(16).to_int32());
    this->hero_research_.set_item_count(result->at(17).to_int32());
    this->hero_research_.set_money_count(result->at(18).to_int32());
    //精力
    this->player_.set_energy(result->at(19).to_int32());
    this->player_.set_last_energy_time(result->at(20).to_int32());
    //免费抽和十连抽信息
    this->hero_research_.set_last_free_time(result->at(21).to_int32());
    this->hero_research_.set_money_count2(result->at(22).to_int32());
    this->fresh_time_ = result->at(23).to_int32();
    //增加给船的属性
    const std::string& extra_damage_str = result->at(24).to_str();
    if (extra_damage_str.length()) {
      DecodeIntArray(
          *this->current_carrier_.mutable_extra_damage1(),
          extra_damage_str);
      this->current_carrier_.mutable_extra_damage1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
    }
    this->player_.set_prestige(result->at(25).to_int32());
    this->player_.set_hero(result->at(26).to_int32());
    this->player_.set_plane(result->at(27).to_int32());
    this->player_.set_muscle(result->at(28).to_int32());
    this->player_.set_exploit(result->at(29).to_int32());
    this->player_.set_union_(result->at(30).to_int32());
    this->player_.set_truce_time(result->at(31).to_int64());
    //围剿BOSS
    this->dstrike_.set_level(result->at(32).to_int32());
    this->dstrike_.set_update_time(result->at(33).to_int32());
    this->dstrike_.set_merit(result->at(34).to_int32());
    this->dstrike_.set_damage(result->at(35).to_int64());
    this->player_.set_sign_id(result->at(36).to_int32());
    this->player_.set_sign_time(result->at(37).to_int64());
    this->player_.set_avatar(result->at(38).to_int32());
    this->player_.set_rank_id(result->at(39).to_int32());
    this->player_.set_create_time(result->at(40).to_int64());
    this->player_.set_status(result->at(41).to_int32());
    this->player_.set_status_time(result->at(42).to_int32());
    this->player_.set_flag(result->at(43).to_int32());
    this->last_server_mail_id_ = result->at(44).to_int64();
    this->hero_research_.set_rd_count(result->at(45).to_int32());
    this->hero_research_.set_last_free_rd_time(result->at(46).to_int32());
    this->hero_research_.set_day_free_rd_count(result->at(47).to_int32());
    this->player_.set_dialog_id(result->at(48).to_c_str());
    this->dstrike_.set_daily_award(result->at(49).to_uint64());
    this->player_.set_client_flag(result->at(50).to_c_str());
    this->player_.set_story_id(result->at(51).to_c_str());
    this->player_.set_total_recharge(result->at(52).to_int32());
    this->last_login_time_ = result->at(53).to_int32();
    this->player_.set_login_days(result->at(54).to_int32());
    this->player_.set_channel(result->at(55).to_str());

    if (this->player_.level() <= 0) this->player_.set_level(1);
    return ERR_OK;
  } else {
    this->last_login_time_ = GetSeconds();
  }
  return ERR_INTERNAL;
}

int32_t RecordPlayer::LoadItemInfo(MySqlConnection& conn) {
  ArrayStream<1024> stream;
  stream.Append(
      "select uid, item_id, item_count, item_attr from item_%ld where "
      "player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());

  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  RecordItem item;
  this->items_.clear();
  while (result && result->IsValid()) {
    item.Clear();
    item.set_uid(result->at(0).to_int64());
    item.set_item_id(result->at(1).to_int32());
    item.set_count(result->at(2).to_int32());

    ForEachString(StringSlice(result->at(3).data, result->at(3).len),
                  StringSlice(";"), DecodeKeyValuePair<RecordItem>(item));

    this->items_.AddItem(item);
    result->Next();
  }
  return ERR_OK;
}

int32_t RecordPlayer::LoadHeroInfo(MySqlConnection& conn) {
  DefaultArrayStream stream;
  stream.Append(
      "select `hero_id`, `level`, `exp`, '', `uid`, `grade`"
      ",`rand_attr`, `rand_attr_1`,`fate_level`,`fate_exp`,`fate_seed`,`train_cost`,`fate_cost`"
      ", `relation`, `wake_level`, `wake_item`"
      " from hero_%ld where player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());

  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  sy::HeroInfo info;
  this->heros_.clear();
  while (result && result->IsValid()) {
    info.Clear();
    info.set_hero_id(result->at(0).to_int64());
    info.set_level(result->at(1).to_int32());
    info.set_exp(result->at(2).to_int32());
    info.mutable_attr1()->Resize(sy::AttackAttr_ARRAYSIZE, 0);
    info.set_uid(result->at(4).to_int64());
    info.set_grade(result->at(5).to_int32());
    const std::string& rand_attr_str = result->at(6).to_str();
    if (rand_attr_str.length()) {
      DecodeIntArray(*info.mutable_rand_attr(), rand_attr_str);
      info.mutable_rand_attr()->Resize(sy::MAX_HERO_RAND_ATTR_COUNT, 0);
    }
    const std::string& rand_attr_1_str = result->at(7).to_str();
    if (rand_attr_1_str.length()) {
      DecodeIntArray(*info.mutable_rand_attr_1(), rand_attr_1_str);
      info.mutable_rand_attr_1()->Resize(sy::MAX_HERO_RAND_ATTR_COUNT, 0);
    }
    info.set_fate_level(result->at(8).to_int32());
    info.set_fate_exp(result->at(9).to_int32());
    info.set_fate_seed(result->at(10).to_int32());
    info.set_train_cost(result->at(11).to_int32());
    info.set_fate_cost(result->at(12).to_int32());
    const std::string& relation_str = result->at(13).to_str();
    if (relation_str.length()) {
      DecodeIntArray(*info.mutable_relation(), relation_str);
    }
    info.set_wake_level(result->at(14).to_int32());
    const std::string& wake_item_str = result->at(15).to_str();
    if (wake_item_str.length()) {
      DecodeIntArray(*info.mutable_wake_item(), wake_item_str);
    }
    info.mutable_wake_item()->Resize(4, 0);

    this->heros_.push_back(info);
    result->Next();
  }
  return ERR_OK;
}

const uint32_t kMaxMailCount = 32;
int32_t RecordPlayer::LoadMailInfo(MySqlConnection& conn) {
  ArrayStream<1024> stream;
  stream.Append(
      "select mail_id, mail_time, mail_type, mail_content, mail_reward from mail_%ld where "
      "player_id=%ld order by mail_id desc",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());

  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }
  this->mails_.clear();

  sy::MailInfo mail;
  while (result && result->IsValid()) {
    mail.Clear();

    mail.set_mail_id(result->at(0).to_int64());
    mail.set_mail_time(result->at(1).to_int64());
    mail.set_mail_type(result->at(2).to_int32());
    mail.set_mail_content(result->at(3).to_str());
    ForEachString(StringSlice(result->at(4).data, result->at(4).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<RepeatedPtrField<sy::KVPair2> >(
                      *mail.mutable_mail_attachment()));
    this->mails_[mail.mail_type()].push_front(mail);

    result->Next();
  }

  for (VectorMap<int32_t, std::deque<sy::MailInfo> >::iterator iter =
           this->mails_.begin();
       iter != this->mails_.end(); ++iter) {
    std::deque<sy::MailInfo>& queue = iter->second;
    if (queue.size() >= kMaxMailCount) {
      stream.clear();
      stream.Append("delete from mail_%ld where player_id=%ld and mail_type=%d and mail_id<%ld"
          , this->uid() % RECORD_TABLE_COUNT, this->uid()
          , queue.front().mail_type(), queue.front().mail_id());
      this->ExecSqlAsync(stream.str());
    }
  }

  return ERR_OK;
}

void RecordPlayer::SendMail(sy::MailInfo& info) {
  std::deque<sy::MailInfo>& queue = this->mails_[info.mail_type()];
  queue.push_front(info);
  while (queue.size() > kMaxMailCount) {
    queue.pop_back();
  }
}

void RecordPlayer::SendAllInfoToClient(int32_t msgid) {
  if (!this->loaded()) return;
  this->SendPlayerInfo();
  this->SendItemInfo();
  this->SendHeroInfo();
  this->SendCarrierInfo();
  this->SendTacticInfo();
  this->SendCopyInfo();
  this->SendShopInfo();
  this->SendRewardInfo();
  this->SendPatrolInfo();
  this->SendFriendInfo();

  this->SendReportAbstract();

  MessageSSResponseGetPlayerInfoEnd end;
  end.set_msgid(msgid);
  this->SendMessageToPlayer(MSG_SS_GET_PLAYER_INFO_END, &end);
}

void RecordPlayer::SendPlayerInfo() {
  MessageSSResponsePlayerInfo message;
  message.mutable_player()->CopyFrom(this->player_);
  message.set_fresh_time(this->fresh_time_);
  DEBUG_LOG(logger)("SendPlayerInfo, PlayerID:%ld, FreshTime:%d, LoginDays:%d"
      , this->uid(), this->fresh_time_, this->player_.login_days());
  for (BuyCountType::const_iterator iter = this->buy_count_.begin();
       iter != this->buy_count_.end(); ++iter) {
    sy::KVPair2 *info = message.add_buy_count();
    info->set_key(iter->first);
    info->set_value(iter->second);
  }
  message.mutable_dstrike()->CopyFrom(this->dstrike_);
  for (KVType::const_iterator iter = this->achievements_.begin();
       iter != this->achievements_.end(); ++iter) {
    sy::KVPair2* info = message.add_achievements();
    info->set_key(iter->first);
    info->set_value(iter->second);
  }
  for (std::vector<sy::RechargeInfo>::iterator iter = this->recharge_.begin();
       iter != this->recharge_.end(); ++iter) {
    message.add_recharge()->CopyFrom(*iter);
  }
  message.set_last_login_time(this->last_login_time_);
  for (VectorMap<std::pair<int32_t, int64_t>, sy::ActivityRecord>::iterator it =
           activity_record_new_.begin();
       it != activity_record_new_.end(); ++it)
    message.add_records_new()->CopyFrom(it->second);
  this->SendMessageToPlayer(MSG_SS_GET_PLAYER_INFO_BEGIN, &message);
}

void RecordPlayer::DeleteItem(int64_t uid, int64_t tid, int32_t system,
                              int32_t msgid) {
  DefaultArrayStream stream;
  stream.Append("delete from item_%ld where uid=%ld and player_id=%ld",
                this->uid() % RECORD_TABLE_COUNT, uid, this->uid());
  this->ExecSqlAsync(stream.str());

  //删除道具LOG
  RecordItem* item = this->items_.GetItemByUniqueID(uid);
  if (item) {
    stream.clear();
    const std::string& date_str = GetDateStr();
    stream.Append(
        "insert into zhanjian_log.`item_delete_%s`(tid, time, server, "
        "player_id, system, msgid, item_uid, item_id, item_count, item_attr) "
        "values(%ld, %ld, %u, %ld, %d, %d, %ld, %d, %d, '"
        , date_str.c_str()
        , tid, GetSeconds()
        , server->GetMainServerID(this->player_.server())
        , this->uid()
        , system, msgid
        , item->uid(), item->item_id(), item->count()
        );
    EncodeKeyValuePair(*item, stream);
    stream.Append("')");
    this->ExecSqlAsync(stream.str());
  }
  this->items_.RemoveItem(uid);
}

//如果item的item_count是0,那么就是删除
//如果item在缓存里面找不到, 那么就是插入
//否则就是更新
void RecordPlayer::UpdateItem(RecordItem* item, int64_t tid, int32_t system, int32_t msgid) {
  if (!item) return;
  //删除
  if (item->uid() && !item->count()) {
    this->DeleteItem(item->uid(), tid, system, msgid);
    return;
  }

  const std::string& date_str = GetDateStr();
  DefaultArrayStream log_stream;
  log_stream.Append(
      "insert into zhanjian_log.`item_%s`(tid, time, server, "
      "player_id, system, msgid, item_uid, item_id, item_count, item_attr, item_old_count, item_old_attr) "
      "values(%ld, %ld, %u, %ld, %d, %d, %ld, %d, %d, '"
      , date_str.c_str()
      , tid, GetSeconds()
      , server->GetMainServerID(this->player_.server())
      , this->uid()
      , system, msgid
      , item->uid(), item->item_id(), item->count()
      );
  EncodeKeyValuePair(*item, log_stream);
  log_stream.Append("'");

  DefaultArrayStream stream;
  RecordItem* player_item = this->items_.GetItemByUniqueID(item->uid());
  if (!player_item) {
    player_item = item;
    //插入
    stream.Append("insert into item_%ld(uid, player_id, item_id, item_count, item_attr) values(%ld, %ld, %d, %d, '",
        this->uid() % RECORD_TABLE_COUNT, player_item->uid(), this->uid(), player_item->item_id(), player_item->count());
    EncodeKeyValuePair(*player_item, stream);
    stream.Append("')");
    this->items_.AddItem(*player_item);
    log_stream.Append(", 0, '')");
  } else {
    log_stream.Append(", %d, '", player_item->count());
    EncodeKeyValuePair(*player_item, log_stream);
    log_stream.Append("')");

    player_item->CopyFrom(*item);
    //更新
    stream.Append("update item_%ld set item_id=%d, item_count=%d, item_attr='",
                  this->uid() % RECORD_TABLE_COUNT, player_item->item_id(),
                  player_item->count());
    EncodeKeyValuePair(*player_item, stream);
    stream.Append("' where player_id=%ld and uid=%ld", this->uid(),
                  player_item->uid());
  }
  this->ExecSqlAsync(stream.str());
  this->ExecSqlAsync(log_stream.str());
}

void RecordPlayer::DeleteHero(int64_t hero_uid, int64_t tid, int32_t system, int32_t msgid) {
  {
    DefaultArrayStream stream;
    stream.Append("delete from hero_%ld where player_id=%ld and uid=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->uid(), hero_uid);
    this->ExecSqlAsync(stream.str());
  }

  DefaultArrayStream log_stream;
  for (std::vector<sy::HeroInfo>::iterator iter = this->heros_.begin();
       iter != this->heros_.end(); ++iter) {
    if (iter->uid() == hero_uid) {
      sy::HeroInfo* info = &*iter;
      const std::string& date_str = GetDateStr();
      log_stream.Append(
          "insert into zhanjian_log.`hero_delete_%s`(tid, time, server, "
          "player_id, system, msgid, hero_uid, `hero_id`, `level`, "
          "`exp`,`fate_level`,`fate_exp`,`fate_seed`,`train_cost`,`fate_cost`, "
          "`wake_level`, `rand_attr`, `wake_item`) "
          "values "
          "(%ld, %ld, %u, %ld, %d, %d, %ld, %d, %d, %d, %d, %d, "
          "%d, %d, %d, %d"
          , date_str.c_str(), tid, GetSeconds()
          , server->GetMainServerID(this->player_.server())
          , this->uid() , system , msgid
          , info->uid(), info->hero_id(), info->level(), info->exp(), info->fate_level(),
          info->fate_exp(), info->fate_seed(), info->train_cost(),
          info->fate_cost(),info->wake_level());
      log_stream.Append(", '");
      EncodeIntArray(info->rand_attr(), log_stream);
      log_stream.Append("', '");
      EncodeIntArray(info->wake_item(), log_stream);
      log_stream.Append("'");
      log_stream.Append(")");
      this->ExecSqlAsync(log_stream.str());

      std::iter_swap(iter, --this->heros_.end());
      this->heros_.pop_back();
      break;
    }
  }
}

//用来决定Hero是否需要更新
//否则数据太多了
bool operator!=(const sy::HeroInfo& h1, const sy::HeroInfo& h2) {
  bool ret =
      h1.level() != h2.level() || h1.exp() != h2.exp() ||
      h1.fate_level() != h2.fate_level() || h1.fate_exp() != h2.fate_exp() ||
      h1.fate_seed() != h2.fate_seed() || h1.train_cost() != h2.train_cost() ||
      h1.fate_cost() != h2.fate_cost() ||
      h1.relation_size() != h2.relation_size() || h1.grade() != h2.grade() ||
      h1.rand_attr_size() != h2.rand_attr_size() ||
      h1.rand_attr_1_size() != h2.rand_attr_1_size() ||
      h1.wake_item_size() != h2.wake_item_size();
  if (ret) return ret;
  if (memcmp(&*h1.relation().begin(), &*h2.relation().begin(),
             h1.relation_size() * sizeof(*h1.relation().begin())) != 0) {
    return true;
  }
  if (memcmp(&*h1.rand_attr().begin(), &*h2.rand_attr().begin(),
             h1.rand_attr_size() * sizeof(*h1.rand_attr().begin())) != 0) {
    return true;
  }
  if (memcmp(&*h1.rand_attr_1().begin(), &*h2.rand_attr_1().begin(),
             h1.rand_attr_1_size() * sizeof(*h1.rand_attr_1().begin())) != 0) {
    return true;
  }
  if (memcmp(&*h1.wake_item().begin(), &*h2.wake_item().begin(),
             h1.wake_item_size() * sizeof(*h1.wake_item().begin())) != 0) {
    return true;
  }
  return false;
}

sy::HeroInfo* RecordPlayer::GetHeroByUID(int64_t uid) {
  for (std::vector<sy::HeroInfo>::iterator iter = this->heros_.begin();
       iter != this->heros_.end(); ++iter) {
    if (iter->uid() == uid) {
      return &*iter;
    }
  }
  return NULL;
}

void RecordPlayer::UpdateHero(sy::HeroInfo* info, int64_t tid, int32_t system,
                              int32_t msgid) {
  if (!info->has_grade()) info->set_grade(0);

  sy::HeroInfo* source = this->GetHeroByUID(info->uid());
  //如果跟内存里面的属性没区别
  //那么就跳过
  if (source != NULL && !(*info != *source))
    return;

  DefaultArrayStream log_stream;
  const std::string& date_str = GetDateStr();
  log_stream.Append(
      "insert into zhanjian_log.`hero_%s`(tid, time, server, "
      "player_id, system, msgid, hero_uid, `hero_id`, `grade`, `level`, "
      "`exp`,`fate_level`,`fate_exp`,`fate_seed`,`train_cost`,`fate_cost`,`"
      "wake_level`,"
      "`rand_attr`,`wake_item`, old_level, old_exp, old_grade, old_fate_level, "
      "old_fate_exp, old_fate_seed, "
      "old_train_cost, old_fate_cost, old_wake_level,old_rand_attr, "
      "old_wake_item) "
      "values "
      "(%ld, %ld, %u, %ld, %d, %d, %ld, %d, %d, %d, %d, %d, %d, "
      "%d, %d, %d, %d",
      date_str.c_str(), tid, GetSeconds(),
      server->GetMainServerID(this->player_.server()), this->uid(), system,
      msgid, info->uid(), info->hero_id(), info->grade(), info->level(),
      info->exp(), info->fate_level(), info->fate_exp(), info->fate_seed(),
      info->train_cost(), info->fate_cost(),info->wake_level());
  log_stream.Append(", '");
  EncodeIntArray(info->rand_attr(), log_stream);
  log_stream.Append("','");
  EncodeIntArray(info->wake_item(), log_stream);
  log_stream.Append("'");

  if (source) {
    log_stream.Append(", %d, %d, %d, %d, %d, %d, %d, %d, %d", source->level(),
                      source->exp(), source->grade(), source->fate_level(),
                      source->fate_exp(), source->fate_seed(),
                      source->train_cost(), source->fate_cost(),
                      source->wake_level());
    log_stream.Append(", '");
    EncodeIntArray(source->rand_attr(), log_stream);
    log_stream.Append("','");
    EncodeIntArray(source->wake_item(), log_stream);
    log_stream.Append("'");
    log_stream.Append(")");

    source->CopyFrom(*info);
  }

  if (!source) {
    this->heros_.push_back(*info);
    log_stream.Append(", 0, 0, 0, 0, 0, 0, 0, 0, 0, '', '')");
  }
  this->ExecSqlAsync(log_stream.str());

  DefaultArrayStream stream;
  stream.Append(
      "insert into hero_%ld(`player_id`, `uid`, `hero_id`, `grade`, `level`, "
      "`exp`,`fate_level`,`fate_exp`,`fate_seed`,`train_cost`,`fate_cost`, "
      "`rand_attr`, `rand_attr_1`, `relation`,`wake_level`, `wake_item`) "
      "values "
      "(%ld, %ld, %d, %d, %d, %d, %d, %d, %d, %d, %d",
      this->uid() % RECORD_TABLE_COUNT, this->uid(), info->uid(),
      info->hero_id(), info->grade(), info->level(), info->exp(),
      info->fate_level(), info->fate_exp(), info->fate_seed(),
      info->train_cost(), info->fate_cost());

  stream.Append(", '");
  EncodeIntArray(info->rand_attr(), stream);
  stream.Append("'");

  stream.Append(", '");
  EncodeIntArray(info->rand_attr_1(), stream);
  stream.Append("'");

  stream.Append(", '");
  EncodeIntArray(info->relation(), stream);
  stream.Append("',%d,'", info->wake_level());
  EncodeIntArray(info->wake_item(), stream);
  stream.Append("'");

  stream.Append(")");
  stream.Append(
      " ON DUPLICATE KEY UPDATE hero_id=values(hero_id), "
      "grade=values(grade), level=values(level), "
      "`exp`=values(`exp`), fate_level=values(fate_level), "
      "fate_exp=values(fate_exp), fate_seed=values(fate_seed), "
      "train_cost=values(train_cost), fate_cost=values(fate_cost), "
      "rand_attr=values(rand_attr), "
      "rand_attr_1=values(rand_attr_1), relation=values(relation), "
      "wake_level=values(wake_level), wake_item=values(wake_item)");
  this->ExecSqlAsync(stream.str());
}

void RecordPlayer::UpdateCarrier(sy::CarrierInfo* info, int64_t tid) {
  DefaultArrayStream stream;

  bool found = false;
  int32_t old_level = 0;
  int32_t old_exp = 0;
  int32_t old_reform_level = 0;
  for (std::vector<sy::CarrierInfo>::iterator iter = this->carriers_.begin();
       iter != this->carriers_.end(); ++iter) {
    if (iter->carrier_id() == info->carrier_id()) {
      old_level = iter->level();
      old_exp = iter->exp();
      old_reform_level = iter->reform_level();
      iter->CopyFrom(*info);
      found = true;
      break;
    }
  }

  if (!found) {
    this->carriers_.push_back(*info);
    stream.Append(
        "insert into carrier_%ld (player_id,carrier_id) values(%ld,%d)",
        this->uid() % RECORD_TABLE_COUNT, this->uid(), info->carrier_id());
  } else {
    stream.Append(
        "update carrier_%ld set level=%d, exp=%d, reform_level=%d where "
        "player_id=%ld and carrier_id=%d",
        this->uid() % RECORD_TABLE_COUNT, info->level(), info->exp(),
        info->reform_level(), this->uid(), info->carrier_id());
  }

  this->ExecSqlAsync(stream.str());
  stream.clear();
  const std::string& date_str = GetDateStr();
  stream.Append(
      "insert into zhanjian_log.`carrier_%s` (tid, time, server, player_id, "
      "carrier, level, exp, reform_level, old_level, old_exp, old_reform_level) "
      "values (%ld, %ld, %u, %ld, %d, %d, "
      "%d, %d, %d, %d, %d)",
      date_str.c_str(), tid, GetSeconds(),
      server->GetMainServerID(this->player_.server()), this->uid(),
      info->carrier_id(), info->level(), info->exp(), info->reform_level(),
      old_level, old_exp, old_reform_level);
  this->ExecSqlAsync(stream.str());
}

void RecordPlayer::SendItemInfo() {
  MessageSSResponseGetItemInfo message;
  message.mutable_items()->Reserve(this->items_.size());
  for (RecordItemManager::const_iterator iter = this->items_.begin();
       iter != this->items_.end(); ++iter) {
    Item *item = message.add_items();
    item->CopyFrom(iter->second);
  }
  message.mutable_equips()->CopyFrom(this->equips_);
  this->SendMessageToPlayer(MSG_SS_GET_ITEM_INFO, &message);
}

void RecordPlayer::SendHeroInfo() {
  MessageSSResponseGetHeroInfo message;
  message.mutable_ships()->Reserve(this->heros_.size());
  for (std::vector<sy::HeroInfo>::iterator iter = this->heros_.begin();
       iter != this->heros_.end(); ++iter) {
    sy::HeroInfo *info = message.add_ships();
    info->CopyFrom(*iter);
  }
  message.mutable_research()->CopyFrom(this->hero_research_);
  this->SendMessageToPlayer(MSG_SS_GET_HERO_INFO, &message);
}

void RecordPlayer::SendMailInfo() {
  MessageSSResponseGetMailInfo message;
  message.set_last_mail_id(this->last_mail_id_);
  message.set_last_server_mail_id(this->last_server_mail_id_);
  for (VectorMap<int32_t, std::deque<sy::MailInfo> >::iterator iter =
           this->mails_.begin();
       iter != this->mails_.end(); ++iter) {
    for (std::deque<sy::MailInfo>::iterator iter_mail = iter->second.begin();
         iter_mail != iter->second.end(); ++iter_mail) {
      message.add_mails()->CopyFrom(*iter_mail);
    }
  }

  this->SendMessageToPlayer(MSG_SS_GET_MAIL_INFO, &message);
}

void RecordPlayer::SendCarrierInfo() {
  MessageSSResponseGetCarrierInfo message;
  message.mutable_carrier()->Reserve(this->carriers_.size());
  //carrier
  for (std::vector<sy::CarrierInfo>::iterator iter = this->carriers_.begin();
       iter != this->carriers_.end(); ++iter) {
    sy::CarrierInfo *info = message.add_carrier();
    info->CopyFrom(*iter);
  }
  //current carrier
  message.mutable_current_carrier()->CopyFrom(this->current_carrier_);
  this->SendMessageToPlayer(MSG_SS_GET_CARRIER_INFO, &message);
}

void RecordPlayer::CreatePlayerAsync(
    intranet::MessageSSRequestCreatePlayer& entry) {
  boost::shared_ptr<ClosureCreatePlayer> ptr(
      new ClosureCreatePlayer(this, server->GetAsyncConn(this->uid()), entry));
  server->PushAsyncClosure(this->uid(), ptr);
}

int32_t RecordPlayer::ProcessCreatePlayer(SSMessageEntry& entry) {
  MessageSSRequestCreatePlayer *message = static_cast<MessageSSRequestCreatePlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->CreatePlayerAsync(*message);
  return ERR_OK;

  //time_t begin_time = GetMilliSeconds();
  //std::pair<int32_t, int32_t> result =
  //    this->CreatePlayerSync(*message, server->mysql_conn());
  //time_t end_time = GetMilliSeconds();
  //if (end_time - begin_time > LOG_SQL_TIME) {
  //  WARN_LOG(logger)("CreatePlayer:%ld CostTime:%ldms", this->uid(), end_time - begin_time);
  //} else {
  //  TRACE_LOG(logger)("CreatePlayer:%ld CostTime:%ldms", this->uid(), end_time - begin_time);
  //}
  //if (result.first) return result.first;
  //return ERR_OK;
}

int32_t RecordPlayer::ProcessChangeName(SSMessageEntry& entry) {
  MessageSSRequestChangeName* message =
      static_cast<MessageSSRequestChangeName*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (this->player_.name() == message->name()) {
    WARN_LOG(logger)("Player:%ld ChangeName, From:%s, To:%s", this->uid(),
                        this->player_.name().c_str(), message->name().c_str());
    return ERR_OK;
  }
  DEBUG_LOG(logger)("ProcessChangeName, PlayerID:%ld, OldName:%s, NewName:%s"
      , this->uid(), this->player_.name().c_str(), message->name().c_str());

  MessageSSResponseChangeName response;
  response.set_name(message->name());

  time_t begin = GetMilliSeconds();
  int32_t result =
      UpdateRoleName(server->mysql_conn(), this->player_.server(),
                     this->player_.openid(), message->name(), this->uid());
  time_t end = GetMilliSeconds();
  if (end - begin > LOG_SQL_TIME * 2) {
    WARN_LOG(logger)("ExecSqlTime:%ldms, Sql:%s", end - begin, "ChangeName");
  }
  if (result) {
    return result;
  } else {
    const std::string& escape_name = server->mysql_conn().EscapeString(
        message->name().c_str(), message->name().length());
    ArrayStream<1024> stream;
    this->player_.set_name(escape_name);
    stream.Append("update player_%ld set name='%s' where uid=%ld",
                  this->uid() % RECORD_TABLE_COUNT, escape_name.c_str(),
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }

  this->SendMessageToPlayer(MSG_SS_RESPONSE_CHANGE_NAME, &response);
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateItemInfo(SSMessageEntry& entry) {
  MessageSSUpdateItemInfo* message =
      static_cast<MessageSSUpdateItemInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  for (int32_t i = 0; i < message->delete_items_size(); ++i) {
    //DEBUG_LOG(logger)("player:%ld, delete item:%ld", this->uid(),
    //                  message->delete_items(i));
    this->DeleteItem(message->delete_items(i), message->tid(),
                     message->system(), message->msgid());
  }
  for (int32_t i = 0; i < message->update_items_size(); ++i) {
    //DEBUG_LOG(logger)("player:%ld, update item:%ld, item_id:%d, item_count:%d",
    //                  this->uid(), message->update_items(i).uid(),
    //                  message->update_items(i).item_id(),
    //                  message->update_items(i).count());
    this->UpdateItem(message->mutable_update_items(i), message->tid(),
                     message->system(), message->msgid());
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateOilInfo(SSMessageEntry& entry) {
  MessageSSUpdateOilInfo* message =
      static_cast<MessageSSUpdateOilInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t old_oil = this->player_.oil();
  int32_t old_energy = this->player_.energy();
  this->player_.set_oil(message->oil());
  this->player_.set_last_oil_time(message->last_oil_time());
  this->player_.set_energy(message->energy());
  this->player_.set_last_energy_time(message->last_energy_time());

  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set oil=%d, last_oil_time=%d, energy=%d, "
      "last_energy_time=%d where uid=%ld",
      this->uid() % RECORD_TABLE_COUNT, message->oil(),
      message->last_oil_time(), message->energy(), message->last_energy_time(),
      this->uid());
  this->ExecSqlAsync(stream.str());

  if (old_oil != this->player_.oil() || old_energy != this->player_.energy()) {
    stream.clear();
    const std::string& date_str = GetDateStr();
    stream.Append(
        "insert into zhanjian_log.`oil_%s`(tid, time, server, "
        "player_id, system, msgid, "
        "oil, delta_oil, energy, delta_energy) values(%ld, %ld, %u, "
        "%ld, %d, %d, %d, %d, %d, %d)"
        , date_str.c_str()
        , message->tid(), GetSeconds()
        , server->GetMainServerID(this->player_.server())
        , this->uid()
        , message->system(), message->msgid()
        , this->player_.oil(), this->player_.oil() - old_oil
        , this->player_.energy(), this->player_.energy() - old_energy
        );
    this->ExecSqlAsync(stream.str());
  }
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateHeroInfo(SSMessageEntry& entry) {
  MessageSSUpdateHeroInfo* message =
      static_cast<MessageSSUpdateHeroInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  for (int32_t i = 0; i < message->info_size(); ++i) {
    this->UpdateHero(message->mutable_info(i), message->tid(),
                     message->system(), message->msgid());
  }
  for (int32_t i = 0; i < message->delete_items_size(); ++i) {
    this->DeleteHero(message->delete_items(i), message->tid(),
                     message->system(), message->msgid());
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateCurrentCarrierInfo(SSMessageEntry& entry) {
  MessageSSUpdateCurrentCarrierInfo* message =
      static_cast<MessageSSUpdateCurrentCarrierInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->UpdateCurrentCarrier(message->mutable_info());
  return ERR_OK;
}

int32_t RecordPlayer::LoadTacticInfo(MySqlConnection& conn) {
  DefaultArrayStream stream;
  stream.Append(
      "select "
      " hero_pos, battle_pos, support_pos, equip1, equip2, equip3, equip4"
      ", equip5, equip6, obtained_carriers"
      ", max_fight_attr, army_id, army_skill, leave_time"
      " from tactic_%ld where "
      " player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result =
      conn.ExecSelect(stream.c_str(), stream.size());

  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  this->tactic_.Clear();
  this->equips_.Clear();
  this->obtained_carriers_.clear();
  if (result && result->IsValid()) {
    ForEachString(StringSlice(result->at(0).data, result->at(0).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<RepeatedPtrField<sy::PositionInfo> >(
                      *this->tactic_.mutable_infos()));

    ForEachString(StringSlice(result->at(1).data, result->at(1).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<RepeatedPtrField<sy::PositionInfo> >(
                      *this->tactic_.mutable_battle_pos()));

    ForEachString(StringSlice(result->at(2).data, result->at(2).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<RepeatedPtrField<sy::PositionInfo> >(
                      *this->tactic_.mutable_support_pos()));
    DecodeIntArray(*this->equips_.mutable_equips_1(), result->at(3).to_str());
    DecodeIntArray(*this->equips_.mutable_equips_2(), result->at(4).to_str());
    DecodeIntArray(*this->equips_.mutable_equips_3(), result->at(5).to_str());
    DecodeIntArray(*this->equips_.mutable_equips_4(), result->at(6).to_str());
    DecodeIntArray(*this->equips_.mutable_equips_5(), result->at(7).to_str());
    DecodeIntArray(*this->equips_.mutable_equips_6(), result->at(8).to_str());
    DecodeIntArray(this->obtained_carriers_, result->at(9).to_str());
    this->max_fight_attr_ = result->at(10).to_int64();
    this->army_id_ = result->at(11).to_int64();
    this->army_skill_.clear();
    DecodeIntArray(this->army_skill_, result->at(12).to_str());
    this->army_skill_.resize(sy::ArmySkill_ARRAYSIZE, 0);
    this->army_leave_time_ = result->at(13).to_int64();

  } else {
    this->max_fight_attr_ = 0;
    stream.clear();
    stream.Append("insert into tactic_%ld(player_id) values(%ld)",
                  this->uid() % RECORD_TABLE_COUNT, this->uid());
    this->ExecSqlAsync(stream.str());
  }
  return ERR_OK;
}

void RecordPlayer::UpdateTactic(const intranet::MessageSSRequestUpdateTacticInfo& msg) {
  DefaultArrayStream stream;
  if (msg.infos_size()) {
    this->tactic_.mutable_infos()->Clear();
    this->tactic_.mutable_infos()->CopyFrom(msg.infos());

    stream.clear();
    stream.Append("update tactic_%ld set hero_pos='", this->uid() % RECORD_TABLE_COUNT);
    EncodeKeyValuePair(msg.infos(), stream);
    stream.Append("'");
    stream.Append(" where player_id=%ld", this->uid());
    this->ExecSqlAsync(stream.str());
  }

  if (msg.battle_pos_size()) {
    this->tactic_.mutable_battle_pos()->Clear();
    this->tactic_.mutable_battle_pos()->CopyFrom(msg.battle_pos());

    stream.clear();
    stream.Append("update tactic_%ld set battle_pos='", this->uid() % RECORD_TABLE_COUNT);
    EncodeKeyValuePair(msg.battle_pos(), stream);
    stream.Append("'");
    stream.Append(" where player_id=%ld", this->uid());
    this->ExecSqlAsync(stream.str());
  }

  if (msg.support_pos_size()) {
    this->tactic_.mutable_support_pos()->Clear();
    this->tactic_.mutable_support_pos()->CopyFrom(msg.support_pos());
    stream.clear();
    stream.Append("update tactic_%ld set support_pos='",
                  this->uid() % RECORD_TABLE_COUNT);
    EncodeKeyValuePair(msg.support_pos(), stream);
    stream.Append("'");
    stream.Append(" where player_id=%ld", this->uid());
    this->ExecSqlAsync(stream.str());
  }
}

void RecordPlayer::UpdateCurrentCarrier(const sy::CurrentCarrierInfo* info) {
  this->current_carrier_.CopyFrom(*info);

  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set `current_carrier_id`=%d",
      this->uid() % RECORD_TABLE_COUNT, info->carrier_id());

  stream.Append(", `carrier_plane_attr`='");
  EncodeIntArray(info->attr1(), stream);
  stream.Append("'");

  stream.Append(", carrier_plane='");
  EncodeIntArray(info->plane_id(), stream);
  stream.Append("'");

  stream.Append(", carrier_extra_damage='");
  EncodeIntArray(info->extra_damage1(), stream);
  stream.Append("'");

  stream.Append(" where uid=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
}

int32_t RecordPlayer::ProcessUpdateTactcInfo(SSMessageEntry& entry) {
  MessageSSRequestUpdateTacticInfo* message =
      static_cast<MessageSSRequestUpdateTacticInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->UpdateTactic(*message);
  return ERR_OK;
}

void RecordPlayer::SendTacticInfo() {
  MessageSSResponseGetTacticInfo message;
  message.mutable_info()->CopyFrom(this->tactic_);
  for (std::vector<int32_t>::iterator it = obtained_carriers_.begin();
       it != obtained_carriers_.end(); ++it) {
    message.add_obtained_carriers(*it);
  }
  message.set_max_fight_attr(this->max_fight_attr_);
  message.set_army_id(this->army_id_);
  for (std::vector<int32_t>::iterator it = army_skill_.begin(); it != army_skill_.end();
       ++it) {
    message.add_army_skill(*it);
  }
  message.set_leave_time(this->army_leave_time_);
  this->SendMessageToPlayer(MSG_SS_GET_TACTIC_INFO, &message);
}

int32_t RecordPlayer::ProcessUpdateMoneyInfo(SSMessageEntry& entry) {
  MessageSSUpdateMoneyInfo* message =
      static_cast<MessageSSUpdateMoneyInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->player_.set_vip_level(message->vip_level());
  this->player_.set_level(message->level());
  this->player_.set_vip_exp(message->vip_exp());
  this->player_.set_exp(message->exp());
  this->player_.set_coin(message->coin());
  this->player_.set_money(message->money());
  this->player_.set_prestige(message->prestige());
  this->player_.set_plane(message->plane());
  this->player_.set_hero(message->hero());
  this->player_.set_muscle(message->muscle());
  this->player_.set_exploit(message->exploit());
  this->player_.set_union_(message->union_());


  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set vip_level=%d, level=%d, vip_exp=%d, `exp`=%ld"
      ", coin=%ld, money=%ld"
      ", prestige=%d, hero=%d, plane=%d"
      ", `muscle`=%d, `exploit`=%d, `union`=%d"
      " where uid=%ld",
      this->uid() % RECORD_TABLE_COUNT, message->vip_level(), message->level(),
      message->vip_exp(), message->exp(), message->coin(), message->money(),
      message->prestige(), message->hero(), message->plane(), message->muscle(),
      message->exploit(), message->union_(), this->uid());
  this->ExecSqlAsync(stream.str());

  stream.clear();
  const std::string& date_str = GetDateStr();
  stream.Append(
      "insert into zhanjian_log.`currency_%s`(tid, time, server, "
      "player_id, system, msgid, level, vip_level, money_1, delta_1, "
      "money_2, delta_2, money_5, delta_5, money_6, delta_6, "
      "money_7, delta_7, money_8, delta_8, money_9, delta_9, money_10, "
      "delta_10, money_11, delta_11, money_12, delta_12, delta_vip, delta_level) values(%ld, %ld, %u, "
      "%ld, %d, %d, %d, %d, %ld, %ld, %ld, %ld, %ld, %ld, %d, %ld, "
      "%d, %ld, %d, %ld, %d, %ld, %d, %ld, %d, %ld, %d, %ld, %d, %d)"
      , date_str.c_str()
      , message->tid(), GetSeconds()
      , server->GetMainServerID(this->player_.server())
      , this->uid()
      , message->system(), message->msgid()
      , this->player_.level(), this->player_.vip_level()
      , this->player_.coin(), message->delta(sy::MONEY_KIND_COIN)
      , this->player_.money(), message->delta(sy::MONEY_KIND_MONEY)
      , this->player_.exp(), message->delta(sy::MONEY_KIND_EXP)
      , this->player_.vip_exp(), message->delta(sy::MONEY_KIND_VIPEXP)
      , this->player_.hero(), message->delta(sy::MONEY_KIND_HERO)
      , this->player_.plane(), message->delta(sy::MONEY_KIND_PLANE)
      , this->player_.prestige(), message->delta(sy::MONEY_KIND_PRESTIGE)
      , this->player_.muscle(), message->delta(sy::MONEY_KIND_MUSCLE)
      , this->player_.exploit(), message->delta(sy::MONEY_KIND_EXPLOIT)
      , this->player_.union_(), message->delta(sy::MONEY_KIND_UNION)
      , message->delta_vip_level(), message->delta_level()
      );
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateCarrierInfo(SSMessageEntry& entry) {
  MessageSSUpdateCarrierInfo* message =
      static_cast<MessageSSUpdateCarrierInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->has_delete_id()) {
    ArrayStream<1024> stream;
    stream.Append(
        "delete from carrier_%ld where player_id = %ld and carrier_id = %d",
        this->uid() % RECORD_TABLE_COUNT, this->uid(), message->delete_id());
    this->ExecSqlAsync(stream.str());
    for (std::vector<sy::CarrierInfo>::iterator iter = this->carriers_.begin();
         iter != this->carriers_.end(); ++iter) {
      if (iter->carrier_id() == message->delete_id()) {
        const std::string& date_str = GetDateStr();
        stream.clear();
        stream.Append(
            "insert into zhanjian_log.`carrier_delete_%s` (tid, time, server, "
            "player_id, "
            "carrier,level, exp, reform_level) values (%ld, %ld, %u, %ld, %d, "
            "%d, %d, %d)",
            date_str.c_str(), message->tid(), GetSeconds(),
            server->GetMainServerID(this->player_.server()), this->uid(),
            iter->carrier_id(), iter->level(), iter->exp(),
            iter->reform_level());
        this->ExecSqlAsync(stream.str());
        this->carriers_.erase(iter);
        break;
      }
    }
  }

  if (message->has_info()) {
    this->UpdateCarrier(message->mutable_info(), message->tid());
  }
  return ERR_OK;
}

int32_t RecordPlayer::LoadCopyInfo(MySqlConnection& conn) {
  ArrayStream<1024> stream;
  stream.Append(
      "select 0, 1, 2, `progress`"
      ", `copy_count`, `passed_copy`"
      ", `chapter_award`, `gate_award`"
      ", `buy_count`"
      ", tower_max_order, tower_max_star, tower_current_order, tower_current_star, tower_buff"
      ", tower_buff_star, tower_award, tower_copy_star, tower_max_star_order, tower_current_buff, achievement"
      ", carrier_copy, carrier_copy_info, medal_copy_id, medal_star, medal_state, medal_achi"
      " from copy_%ld where player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  this->copy_star_.clear();
  this->gate_award_.clear();
  this->chapter_award_.clear();
  this->copy_progress_.clear();
  this->copy_count_.clear();
  this->buy_count_.clear();
  this->achievements_.clear();

  if (result && result->IsValid()) {
    ForEachString(StringSlice(result->at(3).data, result->at(3).len),
                  StringSlice(";"), DecodeKeyValuePair<CopyProgressType>(this->copy_progress_));

    ForEachString(StringSlice(result->at(4).data, result->at(4).len),
                  StringSlice(";"), DecodeKeyValuePair<CopyCountType>(this->copy_count_));

    DecodeIntArray(this->passed_copy_, result->at(5).to_str());

    ForEachString(StringSlice(result->at(6).data, result->at(6).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<KVType>(this->chapter_award_));

    DecodeIntArray(this->gate_award_, result->at(7).to_str());

    ForEachString(StringSlice(result->at(8).data, result->at(8).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<KVType>(this->buy_count_));

    this->tower_state_.set_max_order(result->at(9).to_int32());
    this->tower_state_.set_max_star(result->at(10).to_int32());
    this->tower_state_.set_current_order(result->at(11).to_int32());
    this->tower_state_.set_current_star(result->at(12).to_int32());

    ForEachString(StringSlice(result->at(13).data, result->at(13).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<KVType>(this->tower_buff_));
    this->tower_state_.set_buff_star(result->at(14).to_int32());
    this->tower_state_.set_award(result->at(15).to_int32());
    this->tower_state_.set_copy_star(result->at(16).to_int32());
    this->tower_state_.set_max_star_order(result->at(17).to_int32());
    this->tower_state_.set_random_buff(result->at(18).to_str());

    ForEachString(StringSlice(result->at(19).data, result->at(19).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<KVType>(this->achievements_));
    ForEachString(
        StringSlice(result->at(20).data, result->at(20).len), StringSlice(";"),
        DecodeKeyValuePair<std::vector<sy::CarrierCopy> >(this->carrier_copy));
    KeyValueTrait<sy::CarrierCopyInfo>::decode(result->at(21).data, this->carrier_copy_info);
    this->medal_copy_id_ = result->at(22).to_int32();
    this->medal_star_ = result->at(23).to_int32();
    this->medal_state_ = result->at(24).to_c_str();
    this->medal_achi_ = result->at(25).to_int32();

  } else {
    this->tower_state_.set_max_order(0);
    this->tower_state_.set_max_star(0);
    this->tower_state_.set_current_order(0);
    this->tower_state_.set_current_star(0);
    this->tower_state_.set_buff_star(0);
    this->tower_state_.set_award(0);
    this->tower_state_.set_copy_star(0);
    this->tower_state_.set_random_buff("");
    this->tower_buff_.clear();
    this->carrier_copy.clear();

    DefaultArrayStream stream;
    stream.Append("insert into copy_%ld(player_id) values(%ld)",
                  this->uid() % RECORD_TABLE_COUNT, this->uid());
    this->ExecSqlAsync(stream.str());
  }

  //加载副本星数
  stream.clear();
  stream.Append("select copy_id, star from copy_star_%ld where player_id=%ld",
                this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& star_result = conn.ExecSelect(stream.c_str(), stream.size());
  if (star_result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(star_result->error) + RECORD_SQL_ERROR;
  }

  while (star_result && star_result->IsValid()) {
    int32_t copy_id = star_result->at(0).to_int32();
    int32_t str = star_result->at(1).to_int32();
    this->copy_star_[copy_id] = str;
    star_result->Next();
  }

  return ERR_OK;
}

int32_t RecordPlayer::LoadReportAbstract(MySqlConnection& conn) {
  DefaultArrayStream stream;
  stream.Append(
      "select uid, report_type, report_content from report_%ld where "
      "player_id=%ld order by uid desc",
      this->uid() % RECORD_TABLE_COUNT, this->uid());

  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  this->report_abstract_.clear();

  VectorMap<int32_t, int64_t> delete_report;
  while (result && result->IsValid()) {
    int64_t uid = result->at(0).to_int64();
    int32_t type = result->at(1).to_int32();
    const std::string& content = result->at(2).to_str();

    if (this->report_abstract_[type].size() < sy::MAX_REPORT_ABSTRACT_COUNT)
      this->report_abstract_[type].push_front(content);
    if (this->report_abstract_[type].size() >= sy::MAX_REPORT_ABSTRACT_COUNT &&
        delete_report[type] == 0) {
      delete_report[type] = uid;
    }
    result->Next();
  }

  for (VectorMap<int32_t, int64_t>::const_iterator iter = delete_report.begin();
       iter != delete_report.end(); ++iter) {
    stream.clear();
    stream.Append(
        "delete from report_%ld where player_id=%ld and report_type=%d and uid < %ld",
        this->uid() % RECORD_TABLE_COUNT, this->uid(), iter->first,
        iter->second);
    this->ExecSqlAsync(stream.str());
  }
  return ERR_OK;
}

int32_t RecordPlayer::LoadCarrier(MySqlConnection& conn) {
  DefaultArrayStream stream;
  stream.Append(
      "select carrier_id, level, exp, reform_level from "
      "carrier_%ld where player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result =
      conn.ExecSelect(stream.c_str(), stream.size());

  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  this->carriers_.clear();
  sy::CarrierInfo carrier_info;
  while (result && result->IsValid()) {
    carrier_info.Clear();
    carrier_info.set_carrier_id(result->at(0).to_int32());
    carrier_info.set_level(result->at(1).to_int32());
    carrier_info.set_exp(result->at(2).to_int32());
    carrier_info.set_reform_level(result->at(3).to_int32());
    this->carriers_.push_back(carrier_info);
    result->Next();
  }

  return ERR_OK;
}

void RecordPlayer::SendReportAbstract() {
  MessageSSGetReportAbstract message;
  for (VectorMap<int32_t, std::deque<std::string> >::const_iterator iter =
           this->report_abstract_.begin();
       iter != this->report_abstract_.end(); ++iter) {
    for (std::deque<std::string>::const_iterator iter_content =
             iter->second.begin();
         iter_content != iter->second.end(); ++iter_content) {
      message.add_report_abstract(*iter_content);
    }
  }
  this->SendMessageToPlayer(MSG_SS_GET_REPORT_ABSTRACT, &message);
}

int32_t RecordPlayer::LoadRewardInfo(MySqlConnection& conn) {
  DefaultArrayStream stream;
  stream.Append(
      "select pk_targets, pk_rank_times, pk_rank_reward_rank"
      ", pk_rank_reward_time, pk_max_rank"
      ", last_pk_time, patrol_total_time, patrol_infos, daily_sign"
      ", month_card, big_month_card, life_card, vip_weekly"
      ", weekly_card, weekly_card_login, weekly_card_status"
      ", month_card_1, month_card_1_login, month_card_1_status"
      ", month_card_2, month_card_2_login, month_card_2_status"
      " from reward_%ld where player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());

  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  this->pk_rank_.clear();
  this->vip_weekly_.clear();
  if (result && result->IsValid()) {
    DecodeIntArray(this->pk_rank_, result->at(0).to_str());
    this->pk_rank_times_ = result->at(1).to_int32();
    this->last_pk_rank_ = result->at(2).to_int32();
    this->pk_rank_reward_time_ = result->at(3).to_int32();
    this->pk_max_rank_ = result->at(4).to_int32();
    this->last_pk_time_ = result->at(5).to_int32();
    this->patrol_total_time_ = result->at(6).to_int32();
    ForEachString(
        StringSlice(result->at(7).data, result->at(7).len), StringSlice(";"),
        DecodeKeyValuePair<std::vector<sy::PatrolInfo> >(this->patrol_infos_));
    DecodeIntArray(this->daily_sign_, result->at(8).to_str());
    this->month_card_ = result->at(9).to_int64();
    this->big_month_card_ = result->at(10).to_int64();
    this->life_card_ = result->at(11).to_int64();
    ForEachString(
        StringSlice(result->at(12).data, result->at(12).len), StringSlice(";"),
        DecodeKeyValuePair<VectorMap<int32_t, int32_t> >(this->vip_weekly_));
    this->weekly_card_[0] = result->at(13).to_int32();
    this->weekly_card_[1] = result->at(14).to_int32();
    this->weekly_card_[2] = result->at(15).to_int32();
    this->month_card_1_[0] = result->at(16).to_int32();
    this->month_card_1_[1] = result->at(17).to_int32();
    this->month_card_1_[2] = result->at(18).to_int32();
    this->month_card_2_[0] = result->at(19).to_int32();
    this->month_card_2_[1] = result->at(20).to_int32();
    this->month_card_2_[2] = result->at(21).to_int32();
  } else {
    stream.clear();
    stream.Append("insert into reward_%ld(player_id) values(%ld)",
                  this->uid() % RECORD_TABLE_COUNT, this->uid());
    this->ExecSqlAsync(stream.str());
  }

  return ERR_OK;
}

void RecordPlayer::SendRewardInfo() {
  MessageSSGetRewardInfo message;
  message.set_rank_times(this->pk_rank_times_);
  message.set_pk_rank_reward_time(this->pk_rank_reward_time_);
  message.set_pk_rank_reward_rank(this->last_pk_rank_);
  message.set_pk_rank_max(this->pk_max_rank_);
  message.set_last_pk_time(this->last_pk_time_);
  for (PkRankType::const_iterator iter = this->pk_rank_.begin();
       iter != this->pk_rank_.end(); ++iter) {
    sy::PKRankInfo* info = message.add_rank_info();
    info->set_rank(*iter);
    info->set_player_id(0);
  }
  for (size_t i = 0; i < this->daily_sign_.size(); i++) {
    message.add_daily_sign(this->daily_sign_[i]);
  }
  message.set_month_card(this->month_card_);
  message.set_big_month_card(this->big_month_card_);
  message.set_life_card(this->life_card_);
  message.set_weekly_card(this->weekly_card_[0]);
  message.set_weekly_card_login(this->weekly_card_[1]);
  message.set_weekly_card_status(this->weekly_card_[2]);
  message.set_month_card_1(this->month_card_1_[0]);
  message.set_month_card_1_login(this->month_card_1_[1]);
  message.set_month_card_1_status(this->month_card_1_[2]);
  message.set_month_card_2(this->month_card_2_[0]);
  message.set_month_card_2_login(this->month_card_2_[1]);
  message.set_month_card_2_status(this->month_card_2_[2]);
  for (VectorMap<int32_t, int32_t>::iterator it = this->vip_weekly_.begin();
       it != this->vip_weekly_.end(); ++it) {
    KVPair2* data = message.add_vip_weekly();
    data->set_key(it->first);
    data->set_value(it->second);
  }
  this->SendMessageToPlayer(MSG_SS_GET_REWARD_INFO, &message);
}

void RecordPlayer::SendCopyInfo() {
  MessageSSResponseGetCopyInfo message;
  for (CopyProgressType::const_iterator iter = this->copy_progress_.begin();
       iter != this->copy_progress_.end(); ++iter) {
    message.add_progress()->CopyFrom(*iter);
  }

  for (CopyStarType::const_iterator iter = this->copy_star_.begin();
       iter != this->copy_star_.end(); ++iter) {
    sy::CopyStarInfo* info = message.add_copy_star();
    info->set_copy_id(iter->first);
    info->set_star(iter->second);
  }

  for (CopyCountType::const_iterator iter = this->copy_count_.begin();
       iter != this->copy_count_.end(); ++iter) {
    sy::CopyCount *info = message.add_copy_count();
    info->set_copy_id(iter->first);
    info->set_count(iter->second);
  }

  for (std::vector<int32_t>::const_iterator iter = this->passed_copy_.begin();
       iter != this->passed_copy_.end(); ++iter) {
    message.add_passed_copy(*iter);
  }

  for (KVType::const_iterator iter = this->chapter_award_.begin();
       iter != this->chapter_award_.end(); ++iter) {
    sy::ChapterAwardInfo *info = message.add_chapter_award();
    info->set_chapter(iter->first);
    info->set_mask(iter->second);
  }

  for (std::vector<int32_t>::const_iterator iter = this->gate_award_.begin();
       iter != this->gate_award_.end(); ++iter) {
    message.add_gate_award(*iter);
  }

  message.mutable_tower_stat()->CopyFrom(this->tower_state_);
  for (KVType::const_iterator iter = this->tower_buff_.begin();
       iter != this->tower_buff_.end(); ++iter) {
    sy::KVPair2* info = message.add_tower_buff();
    info->set_key(iter->first);
    info->set_value(iter->second);
  }

  for (std::vector<sy::CarrierCopy>::const_iterator iter =
           this->carrier_copy.begin();
       iter != this->carrier_copy.end(); ++iter) {
    message.add_carrier_copy()->CopyFrom(*iter);
  }
  message.mutable_carrier_copy_info()->CopyFrom(this->carrier_copy_info);
  message.set_medal_copy_id(this->medal_copy_id_);
  message.set_medal_star(this->medal_star_);
  message.set_medal_state(this->medal_state_);
  message.set_medal_achi(this->medal_achi_);

  this->SendMessageToPlayer(MSG_SS_GET_COPY_INFO, &message);
}

void RecordPlayer::UpdateCopyStar(
    google::protobuf::RepeatedPtrField< ::sy::CopyStarInfo>* info) {
  if (!info->size()) return;
  ArrayStream<20 * 1024> stream;
  stream.Append("insert into copy_star_%ld(player_id, copy_id, star) values",
                this->uid() % RECORD_TABLE_COUNT);

  for (int32_t i = 0; i < info->size(); ++i) {
    int32_t copy_id = info->Get(i).copy_id();
    int32_t star = info->Get(i).star();
    this->copy_star_[copy_id] = star;
    if (i) {
      stream.Append(",\n");
    }
    stream.Append("(%ld,%d,%d)", this->uid(), copy_id, star);
  }

  stream.Append(" ON DUPLICATE KEY UPDATE star=values(star)");
  this->ExecSqlAsync(stream.str());
}

void RecordPlayer::UpdateChapterAwardInfo(const sy::ChapterAwardInfo* info) {
  this->chapter_award_[info->chapter()] = info->mask();
  ArrayStream<1024 * 10> stream;
  stream.Append("update copy_%ld set `chapter_award`='",
                this->uid() % RECORD_TABLE_COUNT);
  EncodeKeyValuePair<KVType>(this->chapter_award_, stream);
  stream.Append("'");
  stream.Append(" where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
}

void RecordPlayer::UpdateGateAwardInfo(int32_t gate) {
  if (std::find(this->gate_award_.begin(), this->gate_award_.end(), gate) !=
      this->gate_award_.end())
    return;
  this->gate_award_.push_back(gate);
  ArrayStream<1024 * 10> stream;
  stream.Append("update copy_%ld set `gate_award`='", this->uid() % RECORD_TABLE_COUNT);
  EncodeIntArray(this->gate_award_, stream);
  stream.Append("'");
  stream.Append(" where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
}

void RecordPlayer::UpdateCopyProgress(const sy::CopyProgress* copy) {
  DefaultArrayStream stream;
  bool update = false;
  for (CopyProgressType::iterator iter = this->copy_progress_.begin();
       iter != this->copy_progress_.end(); ++iter) {
    if (iter->copy_type() == copy->copy_type()) {
      iter->CopyFrom(*copy);
      update = true;
      break;
    }
  }
  if (!update) this->copy_progress_.push_back(*copy);

  stream.Append("update copy_%ld set `progress`='",
                this->uid() % RECORD_TABLE_COUNT);
  EncodeKeyValuePair<CopyProgressType>(this->copy_progress_, stream);
  stream.Append("' where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
}

int32_t RecordPlayer::ProcessUpdateCopyInfo(SSMessageEntry& entry) {
  MessageSSUpdateCopyInfo* message =
      static_cast<MessageSSUpdateCopyInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  //副本进度
  if (message->has_copy()) this->UpdateCopyProgress(&message->copy());
  //副本星级
  this->UpdateCopyStar(message->mutable_copy_star());
  //副本通关次数
  if (message->copy_count_size()) {
    for (int32_t i = 0; i < message->copy_count_size(); ++i) {
      int32_t copy_id = message->copy_count(i).copy_id();
      int32_t count = message->copy_count(i).count();
      if (count) this->copy_count_[copy_id] = count;
      else this->copy_count_.erase(copy_id);
    }
    ArrayStream<1024 * 10> stream;
    stream.Append("update copy_%ld set copy_count='", this->uid() % RECORD_TABLE_COUNT);
    EncodeKeyValuePair<CopyCountType>(this->copy_count_, stream);
    stream.Append("' where player_id=%ld", this->uid());

    this->ExecSqlAsync(stream.str());
  }
  //一次性通关副本
  if (message->passed_copy_size()) {
    for (int32_t i = 0; i < message->passed_copy_size(); ++i) {
      int32_t copy_id = message->passed_copy(i);
      if (std::find(this->passed_copy_.begin(), this->passed_copy_.end(),
                    copy_id) == this->passed_copy_.end()) {
        this->passed_copy_.push_back(copy_id);
      }
    }
    ArrayStream<1024 * 10> stream;
    stream.Append("update copy_%ld set passed_copy='", this->uid() % RECORD_TABLE_COUNT);
    EncodeIntArray(this->passed_copy_, stream);
    stream.Append("' where player_id=%ld", this->uid());

    this->ExecSqlAsync(stream.str());
  }
  //章节奖励
  if(message->has_chapter_award()) this->UpdateChapterAwardInfo(message->mutable_chapter_award());
  //关卡奖励
  if (message->has_gate_award()) this->UpdateGateAwardInfo(message->gate_award());

  return ERR_OK;
}

void RecordPlayer::UpdateResearchHeroInfo(const sy::HeroResearchInfo* info) {
  this->hero_research_.CopyFrom(*info);
  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set research_hero_time=%d, research_hero_id=%d "
      ", research_hero_item_count=%d, research_hero_money_count=%d "
      ", research_hero_money_count2=%d "
      ", free_hero_time=%d ,research_hero_rd_count=%d "
      ",research_hero_last_free_rd_time=%d ,research_hero_day_free_rd_count=%d "
      " where uid=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->hero_research_.last_time(),
      this->hero_research_.hero_id(), this->hero_research_.item_count(),
      this->hero_research_.money_count(), this->hero_research_.money_count2(),
      this->hero_research_.last_free_time(), this->hero_research_.rd_count(),
      this->hero_research_.last_free_rd_time(),
      this->hero_research_.day_free_rd_count(), this->uid());
  this->ExecSqlAsync(stream.str());
}

int32_t RecordPlayer::ProcessUpdateHeroResearchInfo(SSMessageEntry& entry) {
  MessageSSUpdateResearchHeroInfo* message =
      static_cast<MessageSSUpdateResearchHeroInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->UpdateResearchHeroInfo(message->mutable_info());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateFreshTime(SSMessageEntry& entry) {
  MessageSSUpdateFreshTime *message = static_cast<MessageSSUpdateFreshTime*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->fresh_time_ = message->fresh_time();

  DefaultArrayStream stream;
  stream.Append("update player_%ld set fresh_time=%d where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT, this->fresh_time_,
                this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateEquipsInfo(SSMessageEntry& entry) {
  MessageSSUpdateEquipInfo* message =
      static_cast<MessageSSUpdateEquipInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->position() < 1 || message->position() > 6) return ERR_PARAM_INVALID;
  RepeatedField<int64_t>* array[] = {
      this->equips_.mutable_equips_1(), this->equips_.mutable_equips_2(),
      this->equips_.mutable_equips_3(), this->equips_.mutable_equips_4(),
      this->equips_.mutable_equips_5(), this->equips_.mutable_equips_6(),
  };
  array[message->position() - 1]->Clear();
  array[message->position() - 1]->CopyFrom(message->equips());

  DefaultArrayStream stream;
  stream.Append("update tactic_%ld set equip%d='",
                this->uid() % RECORD_TABLE_COUNT, message->position());
  EncodeIntArray(message->equips(), stream);
  stream.Append("'");
  stream.Append(" where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateMailID(SSMessageEntry& entry) {
  MessageSSUpdateMailID* message =
      static_cast<MessageSSUpdateMailID*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;

  if (message->has_server_mail_id()) {
    this->last_server_mail_id_ = message->has_server_mail_id()
                                     ? message->server_mail_id()
                                     : this->last_server_mail_id_;

    stream.Append("update player_%ld set last_server_mail_id=%ld where uid=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->last_server_mail_id_,
                  this->uid());
  } else {
    int64_t temp = this->last_mail_id_;
    this->last_mail_id_ = this->GetLastMailID();
    if (temp == this->last_mail_id_) return ERR_OK;

    stream.Append("update player_%ld set last_mail_id=%ld where uid=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->last_mail_id_,
                  this->uid());
  }

  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateShopInfo(SSMessageEntry& entry) {
  MessageSSUpdateShopInfo* message =
      static_cast<MessageSSUpdateShopInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ArrayStream<1024 * 10> stream;

  for (int32_t i = 0; i < message->refresh_commodity_size(); ++i) {
    sy::RefreshShopInfo* info = message->mutable_refresh_commodity(i);
    this->refresh_info_[info->shop_id()].CopyFrom(*info);
    stream.clear();
    stream.Append(
        "insert into "
        "new_shop_%ld(player_id,shop_id,last_time,used_count,refresh_shop_info)"
        " values(%ld,%d,%ld,%d,'",
        this->uid() % RECORD_TABLE_COUNT, this->uid(), info->shop_id(),
        info->last_time(), info->used_count());
    EncodeKeyValuePair(*info->mutable_feats_commodity(), stream);
    stream.Append(
        "') on duplicate key update last_time=values(last_time), used_count"
        "=values(used_count),refresh_shop_info=values(refresh_shop_info)");

    this->ExecSqlAsync(stream.str());
  }

  if (message->normal_commodity_size() > 0) {
    for (int32_t i = 0; i < message->normal_commodity_size(); ++i) {
      const ShopCommodityInfo& info = message->normal_commodity(i);
      this->normal_shop_info_[info.commodity_id()] = info.bought_count();
    }
    stream.clear();
    stream.Append("update shop_%ld set normal_shop_info='",
                  this->uid() % RECORD_TABLE_COUNT);
    EncodeKeyValuePair(this->normal_shop_info_, stream);
    stream.Append("' where player_id=%ld", this->uid());
    this->ExecSqlAsync(stream.str());
  }
  if (message->life_commodity_size() > 0) {
    for (int32_t i = 0; i < message->life_commodity_size(); ++i) {
      const ShopCommodityInfo& info = message->life_commodity(i);
      this->life_shop_info_[info.commodity_id()] = info.bought_count();
    }
    stream.clear();
    stream.Append("update shop_%ld set life_shop_info='",
                  this->uid() % RECORD_TABLE_COUNT);
    EncodeKeyValuePair(this->life_shop_info_, stream);
    stream.Append("' where player_id=%ld", this->uid());
    this->ExecSqlAsync(stream.str());
  }

  return ERR_OK;
}

void RecordPlayer::SendShopInfo() {
  MessageSSResponseGetShopInfo message;

  message.set_last_time(0);
  message.set_used_count(0);
  for (VectorMap<int32_t, sy::RefreshShopInfo>::iterator it =
           refresh_info_.begin();
       it != refresh_info_.end(); ++it) {
    message.add_refresh_commodity()->CopyFrom(it->second);
  }

  for (ShopType::const_iterator it = this->normal_shop_info_.begin();
       it != this->normal_shop_info_.end(); ++it) {
    sy::ShopCommodityInfo* info = message.add_normal_commodity();
    info->set_commodity_id(it->first);
    info->set_bought_count(it->second);
  }
  for (ShopType::const_iterator it = this->life_shop_info_.begin();
       it != this->life_shop_info_.end(); ++it) {
    sy::ShopCommodityInfo* info = message.add_life_commodity();
    info->set_commodity_id(it->first);
    info->set_bought_count(it->second);
  }
  this->SendMessageToPlayer(MSG_SS_GET_SHOP_INFO, &message);
}

int32_t RecordPlayer::LoadShopInfo(MySqlConnection& conn) {
  ArrayStream<1024> stream;
  stream.Append(
      "select `normal_shop_info`, `life_shop_info` from shop_%ld where "
      "player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result =
      conn.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  if (result && result->IsValid()) {
    ForEachString(StringSlice(result->at(0).data, result->at(0).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<ShopType>(this->normal_shop_info_));
    ForEachString(StringSlice(result->at(1).data, result->at(1).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<ShopType>(this->life_shop_info_));
  } else {
    DefaultArrayStream stream;
    stream.clear();
    stream.Append(
        "insert into shop_%ld(player_id, feats_last_time, feats_used_count) "
        "values(%ld, %ld, %d)",
        this->uid() % RECORD_TABLE_COUNT, this->uid(), 0L, 0);
    this->ExecSqlAsync(stream.str());
  }

  stream.clear();
  stream.Append(
      "select `shop_id`,`last_time`,`used_count`,`refresh_shop_info` from "
      "new_shop_%ld where player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& new_result =
      conn.ExecSelect(stream.c_str(), stream.size());
  if (new_result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }
  refresh_info_.clear();
  while (new_result && new_result->IsValid()) {
    sy::RefreshShopInfo& info = refresh_info_[new_result->at(0).to_int32()];
    info.set_shop_id(new_result->at(0).to_int32());
    info.set_last_time(new_result->at(1).to_int64());
    info.set_used_count(new_result->at(2).to_int64());

    ForEachString(StringSlice(new_result->at(3).data, new_result->at(3).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<RepeatedPtrField<sy::ShopCommodityInfo> >(
                      *info.mutable_feats_commodity()));

    new_result->Next();
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdatePKRankInfo(SSMessageEntry& entry) {
  MessageSSUpdatePKRankInfo* message =
      static_cast<MessageSSUpdatePKRankInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  if (message->player_id() == this->uid()) {
    this->pk_rank_.clear();
    for (int32_t i = 0; i < message->rank_info_size(); ++i) {
      this->pk_rank_.push_back(message->rank_info(i).rank());
    }
    stream.clear();
    stream.Append("update reward_%ld set pk_targets='", this->uid() % RECORD_TABLE_COUNT);
    EncodeIntArray(this->pk_rank_, stream);
    stream.Append("'");

    if (message->has_rank_times()) {
      this->pk_rank_times_ = message->rank_times();
      stream.Append(", pk_rank_times=%d", this->pk_rank_times_);
    }
    if (message->has_last_pk_time()) {
      this->last_pk_time_ = message->last_pk_time();
      stream.Append(", last_pk_time=%d", this->last_pk_time_);
    }
    if (message->has_max_rank()) {
      this->pk_max_rank_ = message->max_rank();
      stream.Append(", pk_max_rank=%d", this->pk_max_rank_);
    }

    stream.Append(" where player_id=%ld", this->uid());
    this->ExecSqlAsync(stream.str());
  } else {
    RecordPlayer *player = server->GetPlayerByID(message->player_id());
    std::vector<int32_t> pk_rank;
    for (int32_t i = 0; i < message->rank_info_size(); ++i) {
      pk_rank.push_back(message->rank_info(i).rank());
    }
    if (player) player->pk_rank_ = pk_rank;
    stream.clear();

    stream.Append("update reward_%ld set pk_targets='", message->player_id() % RECORD_TABLE_COUNT);
    EncodeIntArray(pk_rank, stream);
    stream.Append("' where player_id=%ld", message->player_id());
    server->PushSql(message->player_id(), stream.str());
  }
  stream.clear();
  if (message->rank() <= 0 || message->rank() >= sy::MAX_ROBOT_ID) {
    stream.Append("delete from pk_rank_list where player_id = %ld and %d=%d",
                  message->player_id(), message->rank(), message->rank());
  } else {
    stream.Append(
        "insert into pk_rank_list(player_id, server, rank) values (%ld, %u, "
        "%d) on duplicate key update server=values(server), rank=values(rank)",
        message->player_id(), message->server(), message->rank());
  }
  server->PushSql(message->player_id(), stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdatePKRankRewardInfo(SSMessageEntry& entry) {
  MessageSSUpdatePKRankReward* message =
      static_cast<MessageSSUpdatePKRankReward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->last_pk_rank_ = message->pk_rank_reward_rank();
  this->pk_rank_reward_time_ = message->pk_rank_reward_time();

  DefaultArrayStream stream;
  stream.Append(
      "update reward_%ld set pk_rank_reward_rank=%d, pk_rank_reward_time=%d "
      " where player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->last_pk_rank_,
      this->pk_rank_reward_time_, this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessRequestLoadMultiPlayer(SSMessageEntry& entry) {
  return ProcessLoadMultiPlayer(entry);
}

void RecordPlayer::UpdateDailyPKRankInfo(int32_t rank, int32_t time) {
  if (this->loaded()) {
    last_pk_rank_ = rank;
    pk_rank_reward_time_ = time;
  }
}

int32_t RecordPlayer::ProcessUpdateBuyCount(SSMessageEntry& entry) {
  MessageSSUpdateBuyCount *message = static_cast<MessageSSUpdateBuyCount*>(entry.get());

  if (message->has_is_clear()) {
    this->buy_count_.clear();
  } else {
    for (int32_t i = 0; i < message->infos_size(); ++i) {
      this->buy_count_[message->infos(i).key()] = message->infos(i).value();
    }
  }

  DefaultArrayStream stream;
  stream.Append("update copy_%ld set buy_count='", this->uid() % RECORD_TABLE_COUNT);
  EncodeKeyValuePair(this->buy_count_, stream);
  stream.Append("' where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

void RecordPlayer::SendPatrolInfo() {
  MessageSSGetPatrolInfo message;
  message.set_total_time(this->patrol_total_time_);
  for (std::vector<sy::PatrolInfo>::const_iterator it =
           this->patrol_infos_.begin();
       it != this->patrol_infos_.end(); ++it) {
    sy::PatrolInfo* info = message.add_infos();
    info->CopyFrom(*it);
  }
  this->SendMessageToPlayer(MSG_SS_GET_PATROL_INFO, &message);
}

int32_t RecordPlayer::ProcessUpdatePatrolInfo(SSMessageEntry& entry) {
  MessageSSUpdatePatrolInfo* message =
      static_cast<MessageSSUpdatePatrolInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->patrol_total_time_ = message->total_time();

  this->patrol_infos_.clear();
  for (int32_t i = 0; i < message->infos_size(); ++i) {
    this->patrol_infos_.push_back(message->infos(i));
  }

  ArrayStream<1024 * 10> stream;
  stream.Append("update reward_%ld set patrol_total_time=%d, patrol_infos='",
                this->uid() % RECORD_TABLE_COUNT, this->patrol_total_time_);
  EncodeKeyValuePair(this->patrol_infos_, stream);
  stream.Append("' where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::LoadFriendInfo(MySqlConnection& conn) {
  ArrayStream<1024> stream;
  stream.Append(
      "select `player_id`,`friend_id`,`type`,`name`,`level`,`vip_level`, "
      "`avatar`,`energy_time`,`score`,`last_active_time`, `energy`, "
      "`patrol_id`, `army_name`, `rank_id` from friend_%ld "
      " where player_id = %ld",
      this->uid() % RECORD_TABLE_COUNT, this->uid());
  const boost::shared_ptr<ResultSet>& result = conn.ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExeSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  this->friends_.clear();
  sy::FriendInfo friendInfo;
  while (result && result->IsValid()) {
    friendInfo.Clear();
    (void)result->at(0).to_int64();
    friendInfo.set_friend_id(result->at(1).to_int64());
    friendInfo.set_type(result->at(2).to_int32());
    friendInfo.set_name(result->at(3).to_str());
    friendInfo.set_level(result->at(4).to_int32());
    friendInfo.set_vip_level(result->at(5).to_int32());
    friendInfo.set_avatar(result->at(6).to_int32());
    friendInfo.set_energy_time(result->at(7).to_int32());
    friendInfo.set_score(result->at(8).to_int64());
    friendInfo.set_last_active_time(result->at(9).to_int32());
    friendInfo.set_energy(result->at(10).to_int32());
    friendInfo.set_patrol_id(result->at(11).to_int32());
    friendInfo.set_army_name(result->at(12).to_str());
    friendInfo.set_rank_id(result->at(13).to_int32());

    this->friends_.push_back(friendInfo);
    result->Next();
  }

  return ERR_OK;
}

void RecordPlayer::SendFriendInfo() {
  MessageSSGetFriendInfo message;
  message.mutable_infos()->Reserve(this->friends_.size());
  for (std::vector<sy::FriendInfo>::const_iterator it = this->friends_.begin();
       it != this->friends_.end(); ++it) {
    message.add_infos()->CopyFrom(*it);
  }
  this->SendMessageToPlayer(MSG_SS_GET_FRIEND_INFO, &message);
}

void RecordPlayer::DeleteFriendCache(int64_t uid) {
  for (std::vector<sy::FriendInfo>::iterator iter = this->friends_.begin();
       iter != this->friends_.end(); /*++iter*/) {
    if (iter->friend_id() == uid) {
      iter = this->friends_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void RecordPlayer::DeleteFriend(int64_t uid) {
  this->DeleteFriendCache(uid);

  DefaultArrayStream stream;
  stream.Append("delete from friend_%ld where player_id=%ld and friend_id=%ld",
                this->uid() % RECORD_TABLE_COUNT, this->uid(), uid);
  this->ExecSqlAsync(stream.str());

  RecordPlayer *player = server->GetPlayerByID(uid);
  if (player) player->DeleteFriendCache(this->uid());
  stream.clear();
  stream.Append("delete from friend_%ld where player_id=%ld and friend_id=%ld",
                uid % RECORD_TABLE_COUNT, uid, this->uid());
  server->PushSql(uid, stream.str());
}

void RecordPlayer::UpdateFriendInfo(const sy::FriendInfo* info, int64_t player_id) {
  RecordPlayer* player =
      player_id == this->uid() ? this : server->GetPlayerByID(player_id);
  if (player) {
    bool found = false;
    for (std::vector<sy::FriendInfo>::iterator iter = player->friends_.begin();
         iter != player->friends_.end(); ++iter) {
      if (iter->friend_id() == info->friend_id()) {
        iter->CopyFrom(*info);
        found = true;
        break;
      }
    }
    if (!found) player->friends_.push_back(*info);
  }

  DefaultArrayStream stream;
  const std::string& name = server->mysql_conn().EscapeString(
      info->name().c_str(), info->name().length());
  const std::string& army_name = server->mysql_conn().EscapeString(
      info->army_name().c_str(), info->army_name().length());

  stream.Append(
      "insert into friend_%ld (`player_id`,`friend_id`,`type`,`name`,`level`,"
      "`vip_level`,`avatar`,`energy_time`,`score`,`last_active_time`, "
      "`energy`,`patrol_id`,`army_name`,`rank_id`) "
      " values (%ld,%ld,%d,'%s',%d,%d,%d,%d,%ld,%d,%d,%d,'%s',%d) on duplicate "
      "key "
      "update `type`=values(`type`), "
      "`name`=values(name), level=values(level), "
      "vip_level=values(vip_level), avatar=values(avatar), "
      "energy_time=values(energy_time), score=values(score), "
      "last_active_time=values(last_active_time), "
      "patrol_id=values(patrol_id),army_name=values(army_name),rank_id=values(rank_id)",
      player_id % RECORD_TABLE_COUNT, player_id, info->friend_id(),
      info->type(), name.c_str(), info->level(), info->vip_level(),
      info->avatar(), info->energy_time(), info->score(),
      info->last_active_time(), info->energy(), info->patrol_id(),
      army_name.c_str(),info->rank_id());

  server->PushSql(player_id, stream.str());
}

int32_t RecordPlayer::ProcessUpdateFriendInfo(SSMessageEntry& entry) {
  MessageSSUpdateFriendInfo* message = static_cast<MessageSSUpdateFriendInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->has_delete_friend()) {
    this->DeleteFriend(message->delete_friend());
  }
  for (int32_t i = 0; i < message->friends_size(); ++i) {
    this->UpdateFriendInfo(&message->friends(i), message->player_id());
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateReportAbstract(SSMessageEntry& entry) {
  MessageSSUpdateReportAbstract* message =
      static_cast<MessageSSUpdateReportAbstract*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int64_t playerids[] = {message->player_1(), message->player_2()};
  INFO_LOG(logger)("ReportAbstract, Players:%ld,%ld", playerids[0], playerids[1]);

  for (int32_t i = 0; i < ArraySize(playerids); ++i) {
    RecordPlayer *player = server->GetPlayerByID(playerids[i]);
    if (!playerids[i]) continue;

    if (player) {
      player->report_abstract_[message->report_type()].push_back(
          message->report_content());
      if (player->report_abstract_[message->report_type()].size() >=
          sy::MAX_REPORT_ABSTRACT_COUNT) {
        player->report_abstract_[message->report_type()].pop_front();
      }
    }
    const std::string& escape_content = server->mysql_conn().EscapeString(
        message->report_content().c_str(), message->report_content().length());

    DefaultArrayStream stream;
    stream.Append(
        "insert into report_%ld(player_id, report_type, report_content, uid) "
        "values(%ld, %d, '%s', %ld) ON DUPLICATE key update "
        "report_content=values(report_content), "
        "report_type=values(report_type)",
        playerids[i] % RECORD_TABLE_COUNT, playerids[i], message->report_type(),
        escape_content.c_str(), message->report_uid());
    if (playerids[i] > sy::MAX_ROBOT_ID)
      server->PushSql(playerids[i], stream.str());
  }
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateTowerState(SSMessageEntry& entry) {
  MessageSSUpdateTowerInfo* message =
      static_cast<MessageSSUpdateTowerInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->tower_buff_.clear();
  for (int32_t i = 0; i < message->buff_size(); ++i) {
    this->tower_buff_[message->buff(i).key()] = message->buff(i).value();
  }

  this->tower_state_.CopyFrom(message->state());
  DefaultArrayStream stream;
  stream.Append(
      "update copy_%ld set tower_max_order=%d, tower_max_star=%d, "
      "tower_max_star_order=%d,"
      "tower_current_order=%d, tower_current_star=%d, tower_buff='",
      this->uid() % RECORD_TABLE_COUNT, this->tower_state_.max_order(),
      this->tower_state_.max_star(), this->tower_state_.max_star_order(),
      this->tower_state_.current_order(), this->tower_state_.current_star());
  EncodeKeyValuePair(this->tower_buff_, stream);
  stream.Append("'");
  stream.Append(
      ", tower_buff_star=%d, tower_award=%d, tower_copy_star=%d, tower_current_buff='%s'",
      this->tower_state_.buff_star(), this->tower_state_.award(),
      this->tower_state_.copy_star(), this->tower_state_.random_buff().c_str());
  stream.Append(" where player_id=%ld", this->uid());

  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessRequestGetFriend(SSMessageEntry &entry){
  this->SendFriendInfo();
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateTruceTime(SSMessageEntry& entry) {
  MessageSSUpdateTruceTime* message =
      static_cast<MessageSSUpdateTruceTime*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->player_.set_truce_time(message->truce_time());
  DefaultArrayStream stream;
  stream.Append("update player_%ld set truce_time=%ld where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT, this->player_.truce_time(),
                this->uid());
  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateDialog(SSMessageEntry& entry) {
  MessageSSUpdateDialog* message =
      static_cast<MessageSSUpdateDialog*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->has_dialog_id()) {
    this->player_.set_dialog_id(message->dialog_id());
    DefaultArrayStream stream;
    const std::string& escape_dialog = server->mysql_conn().EscapeString(
        this->player_.dialog_id().c_str(), this->player_.dialog_id().length());
    stream.Append("update player_%ld set dialog_id='%s' where uid=%ld",
                  this->uid() % RECORD_TABLE_COUNT, escape_dialog.c_str(),
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }

  if (message->has_story_id()) {
    this->player_.set_story_id(message->story_id());
    DefaultArrayStream stream;
    const std::string& escape_story = server->mysql_conn().EscapeString(
        this->player_.story_id().c_str(), this->player_.story_id().length());
    stream.Append("update player_%ld set story_id='%s' where uid=%ld",
                  this->uid() % RECORD_TABLE_COUNT, escape_story.c_str(),
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateClientFlag(SSMessageEntry& entry) {
  MessageSSUpdateClientFlag* message =
      static_cast<MessageSSUpdateClientFlag*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->player_.set_client_flag(message->client_flag());
  DefaultArrayStream stream;
  const std::string& escape_client_flag =
      server->mysql_conn().EscapeString(this->player_.client_flag().c_str(),
                                        this->player_.client_flag().length());
  stream.Append("update player_%ld set client_flag='%s' where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT, escape_client_flag.c_str(),
                this->uid());
  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateSignIn(SSMessageEntry& entry) {
  MessageSSUpdateSignIn* message =
      static_cast<MessageSSUpdateSignIn*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->player_.set_sign_id(message->sign_id());
  this->player_.set_sign_time(message->sign_time());
  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set sign_id=%d, sign_time = %ld where uid=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->player_.sign_id(),
      this->player_.sign_time(), this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t ProcessLoadRankList(SSMessageEntry& entry) {
  MessageSSRequestLoadRankList* message =
      static_cast<MessageSSRequestLoadRankList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ArrayStream<1024 * 1024> stream;

  std::vector<int64_t> player_ids;
  std::vector<sy::RankItemInfo> rank_info;

  intranet::MessageSSResponseLoadRankList response;
  for (int32_t type_index = 0; type_index < message->type_size(); ++type_index) {
    response.Clear();
    player_ids.clear();
    rank_info.clear();
    response.set_server_id(message->server_id());
    response.set_type(message->type(type_index));
    response.mutable_list()->mutable_items();

    stream.clear();
    stream.Append(
        "select players from rank_list_details where server_id=%u and "
        "rank_type=%d",
        message->server_id(), message->type(type_index));
    const boost::shared_ptr<ResultSet>& result_set = name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
    if (!result_set || !result_set->IsValid()) {
      server->SendServerMessage(entry.session_ptr.get(),
                                MSG_SS_RESPONSE_LOAD_RANK_LIST, &response);
      continue;
    }
    DecodeIntArray(player_ids, result_set->at(0).to_str());
    if (player_ids.empty()) {
      server->SendServerMessage(entry.session_ptr.get(),
                                MSG_SS_RESPONSE_LOAD_RANK_LIST, &response);
      continue;
    }

    rank_info.resize(player_ids.size());
    stream.clear();
    stream.Append(
        "select player_id, name, army_name, level, fight_attr, vip_level, "
        "exploit, damage, star, avatar from rank_player_details where rank_type=%d and "
        "player_id in (",
        message->type(type_index));
    for (size_t i = 0; i < player_ids.size(); ++i) {
      if (i) stream.Append(",");
      stream.Append("%ld", player_ids[i]);
    }
    stream.Append(")");
    const boost::shared_ptr<ResultSet>& result = name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
    while (result && result->IsValid()) {
      int32_t offset = std::find(player_ids.begin(), player_ids.end(),
                                 result->at(0).to_int64()) -
                       player_ids.begin();
      if (offset < 0 || offset >= (int32_t)player_ids.size()) continue;

      sy::RankItemInfo* item = &rank_info[offset];
      item->set_uid(result->at(0).to_int64());
      item->set_name(result->at(1).to_str());
      item->set_army_name(result->at(2).to_str());
      item->set_level(result->at(3).to_int32());
      item->set_fight_attr(result->at(4).to_int64());
      item->set_vip_level(result->at(5).to_int32());
      item->set_exploit(result->at(6).to_int32());
      item->set_damage(result->at(7).to_int64());
      item->set_star(result->at(8).to_int32());
      item->set_avatar(result->at(9).to_int32());

      result->Next();
    }

    for (std::vector<sy::RankItemInfo>::iterator iter = rank_info.begin();
         iter != rank_info.end(); ++iter) {
      if (iter->uid()) response.mutable_list()->add_items()->CopyFrom(*iter);
    }
    server->SendServerMessage(entry.session_ptr.get(),
                              MSG_SS_RESPONSE_LOAD_RANK_LIST, &response);
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateFriendEnergy(SSMessageEntry& entry) {
  MessageSSUpdateFriendEnergy *message = static_cast<MessageSSUpdateFriendEnergy*>(entry.get());
  if (!message) return ERR_INTERNAL;
  RecordPlayer *player = server->GetPlayerByID(message->friend_id());
  if (player) {
    for (std::vector<sy::FriendInfo>::iterator iter = player->friends_.begin();
         iter != player->friends_.end(); ++iter) {
      if (iter->friend_id() == this->uid()) {
        iter->set_energy(message->enery());
        break;
      }
    }
  }

  DefaultArrayStream stream;
  stream.Append(
      "update friend_%ld set energy=%d where player_id=%ld and friend_id=%ld",
      message->friend_id() % RECORD_TABLE_COUNT, message->enery(),
      message->friend_id(), this->uid());
  server->PushSql(message->friend_id(), stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateDstrikeInfo(SSMessageEntry& entry) {
  MessageSSUpdateDstrikeInfo* message =
      static_cast<MessageSSUpdateDstrikeInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->dstrike_.CopyFrom(message->info());

  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set dstrike_level=%d, dstrike_time=%d, "
      "dstrike_merit=%d, dstrike_damage=%ld, dstrike_daily_award=%lu"
      " where uid=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->dstrike_.level(),
      this->dstrike_.update_time(), this->dstrike_.merit(),
      this->dstrike_.damage(), this->dstrike_.daily_award(), this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessRequestGetMail(SSMessageEntry& entry) {
  INFO_LOG(logger)("ProcessRequestGetMail, PlayerID:%ld", this->uid());
  this->SendMailInfo();
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateAchievements(SSMessageEntry& entry) {
  MessageSSUpdateAchievement* message =
      static_cast<MessageSSUpdateAchievement*>(entry.get());
  for (int32_t i = 0; i < message->infos_size(); ++i) {
    this->achievements_[message->infos(i).key()] = message->infos(i).value();
  }
  DefaultArrayStream stream;
  stream.Append("update copy_%ld set achievement='",
                this->uid() % RECORD_TABLE_COUNT);
  EncodeKeyValuePair(this->achievements_, stream);
  stream.Append("' where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t ProcessSendMail(SSMessageEntry& entry) {
  MessageSSSendMail* message = static_cast<MessageSSSendMail*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int64_t uid = message->player_id();
  MailInfo& mail = *message->mutable_mail();
  server->SendServerMessage(entry.session_ptr.get(), MSG_SS_SEND_MAIL, message);

  RecordPlayer* player = server->GetPlayerByID(uid);
  if (player) player->SendMail(mail);

  ArrayStream<1024 * 128> stream;
  const std::string& escape_content = server->mysql_conn().EscapeString(
      mail.mail_content().c_str(), mail.mail_content().size());
  stream.Append(
      "insert into mail_%ld(player_id, mail_id, mail_time, mail_type, "
      "mail_content, mail_reward) values (%ld, %ld, %d, %d, '%s','",
      uid % RECORD_TABLE_COUNT, uid, mail.mail_id(), mail.mail_time(),
      mail.mail_type(), escape_content.c_str());
  EncodeKeyValuePair(*mail.mutable_mail_attachment(), stream);
  stream.Append("')");
  stream.Append(
      " ON DUPLICATE KEY UPDATE mail_type=values(mail_type), "
      "mail_content=values(mail_content), mail_reward=values(mail_reward)");

  server->PushSql(uid, stream.str());

  if (mail.mail_attachment_size() > 0) {
    const std::string& date_str = GetDateStr();
    stream.clear();
    stream.Append(
        "insert into zhanjian_log.`mail_%s`(tid, time, server, player_id, "
        "mail_id, mail_time, mail_type,mail_content,mail_reward) values(%ld, "
        "%ld, %u, %ld, %ld, %d, %d, '%s','",
        date_str.c_str(), message->tid(), GetSeconds(), message->server_id(),
        uid, mail.mail_id(), mail.mail_time(), mail.mail_type(),
        escape_content.c_str());
    EncodeKeyValuePair(*mail.mutable_mail_attachment(), stream);
    stream.Append("')");
    server->PushSql(uid, stream.str());
  }
  return ERR_OK;
}

int32_t RecordPlayer::ProcessRequestGetMailReward(SSMessageEntry& entry) {
  MessageSSRequestGetMailReward* message =
      static_cast<MessageSSRequestGetMailReward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSResponseGetMailReward response;
  response.set_mail_id(message->mail_id());

  for (VectorMap<int32_t, std::deque<sy::MailInfo> >::const_iterator
           iter_queue = this->mails_.begin();
       iter_queue != this->mails_.end(); ++iter_queue) {
    for (std::deque<sy::MailInfo>::const_iterator it =
             iter_queue->second.begin();
         it != iter_queue->second.end(); ++it) {
      if (it->mail_id() == message->mail_id()) {
        response.set_result_id(ERR_OK);
        response.mutable_rewards()->CopyFrom(it->mail_attachment());
        break;
      }
    }
  }

  if (!response.has_mail_id()) response.set_result_id(ERR_PARAM_INVALID);

  SendMessageToPlayer(MSG_SS_RESPONSE_GET_MAIL_REWARD, &response);

  return ERR_OK;
}

int32_t RecordPlayer::ProcessNotifyGetMailReward(SSMessageEntry& entry) {
  MessageSSNotifyGetMailReward* message =
      static_cast<MessageSSNotifyGetMailReward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  for (VectorMap<int32_t, std::deque<sy::MailInfo> >::iterator
           iter_queue = this->mails_.begin();
       iter_queue != this->mails_.end(); ++iter_queue) {
    for (std::deque<sy::MailInfo>::iterator it =
             iter_queue->second.begin();
         it != iter_queue->second.end(); ++it) {
      if (it->mail_id() == message->mail_id()) {
        ArrayStream<1024> stream;
        stream.Append(
            "delete from mail_%ld where player_id=%ld and mail_id=%ld",
            this->uid() % RECORD_TABLE_COUNT, this->uid(), it->mail_id());
        this->ExecSqlAsync(stream.str());
        iter_queue->second.erase(it);

        break;
      }
    }
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateRankID(SSMessageEntry& entry) {
  MessageSSUpdateRankID* message =
      static_cast<MessageSSUpdateRankID*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->player_.set_rank_id(message->rank_id());
  DefaultArrayStream stream;
  stream.Append("update player_%ld set rank_id=%d where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT, this->player_.rank_id(),
                this->uid());
  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

int32_t ProcessUpdateDstrikeBoss(SSMessageEntry& entry) {
  MessageSSUpdateDstrikeBossInfo* message =
      static_cast<MessageSSUpdateDstrikeBossInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  ArrayStream<1024 * 128> stream;

  //删除BOSS或者更新
  if (message->has_player_id()) {
    stream.Append("delete from dstrike_boss where server=%u and player_id=%ld",
                  message->server_id(), message->player_id());
  } else {
    const std::string& name = server->mysql_conn().EscapeString(
        message->boss().name().c_str(), message->boss().name().length());

    stream.Append(
        "insert into dstrike_boss(server_id, player_id, name, boss_id, "
        "quality, "
        "level, status, time, expire_time, blood, total_blood) "
        "values(%u,%ld,'%s',%d,%d,%d,%d,%d,%d",
        message->server_id(), message->boss().player_id(), name.c_str(),
        message->boss().boss_id(), message->boss().boss_quality(),
        message->boss().boss_level(), message->boss().boss_status(),
        message->boss().boss_time(), message->boss().boss_expire_time());
    stream.Append(",'");
    EncodeIntArray(message->boss().boss_blood(), stream);
    stream.Append("'");
    stream.Append(", %ld", message->boss().total_blood());
    stream.Append(")");

    stream.Append(
        " ON DUPLICATE KEY UPDATE name=values(name), boss_id=values(boss_id)"
        ", quality=values(quality), level=values(level), status=values(status)"
        ", time=values(time), expire_time=values(expire_time), "
        "blood=values(blood), total_blood=values(total_blood)");
  }

  server->PushSql(message->boss().player_id(), stream.str());
  return ERR_OK;
}

int32_t ProcessLoadDstrikeBossList(SSMessageEntry& entry) {
  MessageSSRequestLoadBossList* message =
      static_cast<MessageSSRequestLoadBossList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSResponseLoadBossList response;
  response.set_server_id(message->server_id());
  DefaultArrayStream stream;
  stream.Append(
      "select player_id, name, boss_id, quality, level, status, time, "
      "expire_time, blood, total_blood from dstrike_boss where server_id=%u",
      message->server_id());

  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql Fail:%s, %s"
        , stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  while (result && result->IsValid()) {
    sy::DstrikeBoss* info = response.add_boss();

    info->set_player_id(result->at(0).to_int64());
    info->set_name(result->at(1).to_str());
    info->set_boss_id(result->at(2).to_int32());
    info->set_boss_quality(result->at(3).to_int32());
    info->set_boss_level(result->at(4).to_int32());
    info->set_boss_status(result->at(5).to_int32());
    info->set_boss_time(result->at(6).to_int32());
    info->set_boss_expire_time(result->at(7).to_int32());
    const std::string& blood = result->at(8).to_str();
    if (blood.length()) DecodeIntArray(*info->mutable_boss_blood(), blood);
    info->set_total_blood(result->at(9).to_int64());

    result->Next();
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_LOAD_BOSS_LIST, &response);
  return ERR_OK;
}

int32_t ProcessRequestLoadServerShop(SSMessageEntry& entry) {
  MessageSSRequestLoadServerShop* message =
      static_cast<MessageSSRequestLoadServerShop*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DefaultArrayStream stream;
  stream.Append(
      "select shop_data, astrology_country_id, astrology_refresh_time from "
      "server_shop where server_id=%u",
      message->server_id());

  MessageSSResponseLoadServerShop response;
  response.set_server_id(message->server_id());

  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql Fail:%s, %s"
        , stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }
  if (result && result->IsValid()) {
    const std::string& str = result->at(0).to_str();
    if (str.length()) {
      ForEachString(StringSlice(str), StringSlice(";"),
                    DecodeKeyValuePair<RepeatedPtrField<sy::KVPair2> >(
                        *response.mutable_items()));
    }
    response.set_astrology_country_id(result->at(1).to_int32());
    response.set_astrology_refresh_time(result->at(2).to_int64());
    TRACE_LOG(logger)("astrology_refresh_time:%ld", response.astrology_refresh_time());
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_LOAD_SERVER_SHOP, &response);

  return ERR_OK;
}

int32_t ProcessUpdateServerShop(SSMessageEntry& entry) {
  MessageSSUpdateServerShop* message =
      static_cast<MessageSSUpdateServerShop*>(entry.get());
  if (!message) return ERR_INTERNAL;
  ArrayStream<1024*128> stream;
  stream.Append("insert into server_shop(server_id, shop_data) values(%u, ", message->server_id());
  stream.Append("'");
  EncodeKeyValuePair(*message->mutable_items(), stream);
  stream.Append("')");
  stream.Append(" ON DUPLICATE KEY UPDATE shop_data=values(shop_data)");

  server->PushSql(message->server_id(), stream.str());
  return ERR_OK;
}

int32_t ProcessRequestLoadServerMail(SSMessageEntry& entry) {
  MessageSSRequestLoadServerMail* message =
      static_cast<MessageSSRequestLoadServerMail*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSResponseLoadServerMail response;
  response.set_server_id(message->server_id());
  DefaultArrayStream stream;
  stream.Append(
      "select server_mail_id, mail_time, mail_content, mail_reward, level_min, level_max, vip_min, vip_max from "
      "server_mail where server_id=%u",
      message->server_id());

  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql Fail:%s, %s"
        , stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }
  while (result && result->IsValid()) {
    sy::MailInfo* info = response.add_mails();
    info->set_mail_id(result->at(0).to_int64());
    info->set_mail_time(result->at(1).to_int32());
    info->set_mail_type(sy::MAIL_TYPE_SYS);
    info->set_mail_content(result->at(2).to_str());
    ForEachString(StringSlice(result->at(3).data, result->at(3).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<RepeatedPtrField<sy::KVPair2> >(
                      *info->mutable_mail_attachment()));
    info->set_level_min(result->at(4).to_int32());
    info->set_level_max(result->at(5).to_int32());
    info->set_vip_min(result->at(6).to_int32());
    info->set_vip_max(result->at(7).to_int32());

    result->Next();
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_LOAD_SERVER_MAIL, &response);
  return ERR_OK;
}

int32_t ProcessRequestAddServerMail(SSMessageEntry& entry) {
  MessageSSAddServerMail* message =
      static_cast<MessageSSAddServerMail*>(entry.get());
  if (!message) return ERR_INTERNAL;

  sy::MailInfo& mail = *message->mutable_mail();

  ArrayStream<1024 * 128> stream;
  const std::string& escape_content = server->mysql_conn().EscapeString(
      mail.mail_content().c_str(), mail.mail_content().size());

  stream.Append(
      "insert into server_mail(server_id, server_mail_id, mail_time"
      ", mail_content, mail_reward, level_min, level_max, vip_min, vip_max) "
      "values (%u, %ld, %d, '%s','",
      message->server_id(), mail.mail_id(), mail.mail_time(),
      escape_content.c_str());
  EncodeKeyValuePair(*mail.mutable_mail_attachment(), stream);

  stream.Append("', %d, %d, %d, %d)", mail.level_min(), mail.level_max(),
                mail.vip_min(), mail.vip_max());

  server->PushSql(message->server_id(), stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateLoginTime(SSMessageEntry& entry) {
  MessageSSUpdateLoginTime* message =
      static_cast<MessageSSUpdateLoginTime*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set last_login_time=%ld, login_channel='%s' "
      " where "
      "uid=%ld",
      this->uid() % RECORD_TABLE_COUNT, GetSeconds(),
      message->channel().c_str(), this->uid());
  this->last_login_time_ = GetSeconds();
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t ProcessLoadSetIpList(SSMessageEntry& entry) {
  MessageSSRequestLoadIPList* message =
      static_cast<MessageSSRequestLoadIPList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.Append(
      "select white_list,black_list from ip_list where server_id = %u",
      message->server_id());

  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s"
        , stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  MessageSSResponseLoadIPList response;

  if (result && result->IsValid()) {
    std::istringstream white_list;
    white_list.str(result->at(0).to_c_str());
    std::istringstream black_list;
    black_list.str(result->at(1).to_c_str());

    std::string line;
    while (std::getline(white_list, line, ',')) {
      if (line.empty()) continue;
      response.add_white_list(line);
    }
    line.clear();
    while (std::getline(black_list, line, ',')) {
      if (line.empty()) continue;
      response.add_black_list(line);
    }
  } else {
    stream.clear();
    stream.Append(
        "insert into ip_list (server_id,white_list,black_list) "
        "values(%u,'','')",
        message->server_id());
    server->PushSql(0, stream.str());
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_LOAD_IP_LIST, &response);

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateLoginInfo(SSMessageEntry& entry) {
  MessageSSUpdateLoginInfo* message =
      static_cast<MessageSSUpdateLoginInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (this->session().lock() != entry.session_ptr)
    this->session(entry.session_ptr);

  DefaultArrayStream stream;

  const std::string& date_str = GetDateStr();
  stream.Append(
      "insert into zhanjian_log.`login_%s`(tid, time, server, "
      "player_id, "
      "login, online_time, ipaddr) values(%ld, %ld, %u, "
      "%ld, %d, %d, '%s')"
      , date_str.c_str()
      , message->tid(), GetSeconds()
      , server->GetMainServerID(this->player_.server())
      , this->uid()
      , message->login(), message->online_time()
      , message->ipaddr().c_str()
      );
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t ProcessLoadNotice(SSMessageEntry& entry) {
  MessageSSRequestLoadNotice* message =
      static_cast<MessageSSRequestLoadNotice*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.Append(
      "select "
      "notice_id,notice_type,start_time,end_time,interval_time,content,`order`,"
      "link_url from server_notice where server_id = %u",
      message->server_id());
  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s"
        , stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  MessageSSResponseLoadNotice response;
  while (result && result->IsValid()) {
    sy::NoticeInfo* info = response.add_notices();
    info->set_tid(result->at(0).to_int64());
    info->set_type(result->at(1).to_int32());
    info->set_begin_time(result->at(2).to_int64());
    info->set_end_time(result->at(3).to_int64());
    info->set_interval(result->at(4).to_int32());
    info->set_content(result->at(5).to_c_str());
    info->set_order(result->at(6).to_int32());
    info->set_link_url(result->at(7).to_c_str());
    result->Next();
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_LOAD_NOTICE, &response);

  return ERR_OK;
}

int32_t ProcessUpdateRankListDetails(SSMessageEntry& entry) {
  MessageSSUpdateRankListDetails* message = static_cast<MessageSSUpdateRankListDetails*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ArrayStream<1028 * 1024> stream;
  stream.Append(
      "insert into rank_list_details(server_id, rank_type, players) values(%u, "
      "%d, '",
      message->server_id(), message->type());
  EncodeIntArray(message->players(), stream);
  stream.Append("'");
  stream.Append(") ON DUPLICATE KEY UPDATE players=values(players)");
  server->PushSql(message->server_id(), stream.str());
  return ERR_OK;
}

int32_t ProcessUpdateRankListPlayer(SSMessageEntry& entry) {
  MessageSSUpdateRankListPlayer* message = static_cast<MessageSSUpdateRankListPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DefaultArrayStream stream;

  stream.Append(
      "insert into rank_player_details(`rank_type`, `player_id`, update_time, "
      "`name`, `army_name`, `level`, `fight_attr`, `vip_level`, `exploit`, "
      "`damage`, `star`, `avatar`) values");
  const sy::RankItemInfo& info = message->info();
  const std::string& name = server->mysql_conn().EscapeString(
      info.name().c_str(), info.name().length());
  const std::string& army_name = server->mysql_conn().EscapeString(
      info.army_name().c_str(), info.army_name().length());

  stream.Append("(%d,%ld,%ld,'%s','%s',%d,%ld,%d,%d,%ld,%d,%d)",
                message->type(), info.uid(), GetSeconds(), name.c_str(),
                army_name.c_str(), info.level(), info.fight_attr(),
                info.vip_level(), info.exploit(), info.damage(), info.star(),
                info.avatar());
  stream.Append(
      " ON DUPLICATE KEY UPDATE update_time=values(update_time), "
      "name=values(name), army_name=values(army_name), level=values(level), "
      "fight_attr=values(fight_attr), vip_level=values(vip_level), "
      "exploit=values(exploit), damage=values(damage), star=values(star), "
      "avatar=values(avatar)");

  server->PushSql(info.uid(), stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateOtherDstrikeBoss(SSMessageEntry& entry) {
  MessageSSUpdateOtherDstrikeBoss* message =
      static_cast<MessageSSUpdateOtherDstrikeBoss*>(entry.get());
  if (!message) return ERR_INTERNAL;
  RecordPlayer* player = server->GetPlayerByID(message->player_id());
  if (player) {
    player->dstrike_.set_level(player->dstrike_.level() + 1);
  }
  DefaultArrayStream stream;
  stream.Append(
      "update player_%ld set dstrike_level = dstrike_level + 1 where uid=%ld",
      message->player_id() % RECORD_TABLE_COUNT, message->player_id());
  server->PushSql(message->player_id(), stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateCopyStatus(SSMessageEntry& entry) {
  MessageSSUpdateCopyStatus* message = static_cast<MessageSSUpdateCopyStatus*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DefaultArrayStream stream;

  const std::string& date_str = GetDateStr();
  stream.Append(
      "insert into zhanjian_log.`copy_%s`(tid, time, server, "
      "player_id, copy_type, copy_id, copy_order, star)"
      "values(%ld, %ld, %u, %ld, %d, %d, %d, %d)",
      date_str.c_str(), message->tid(), GetSeconds(),
      server->GetMainServerID(this->player_.server()), this->uid(),
      message->copy_type(), message->copy_id(), message->copy_order(),
      message->copy_star());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessHeartBeat(SSMessageEntry& entry) { return ERR_OK; }

int32_t RecordPlayer::ProcessUpdateCarrierCopy(SSMessageEntry& entry) {
  MessageSSUpdateCarrierCopyInfo* message = static_cast<MessageSSUpdateCarrierCopyInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ArrayStream<16 * 1024> stream;
  stream.Append("update copy_%ld set", this->uid() % RECORD_TABLE_COUNT);

  if (message->has_carrier_copy_info()) {
    this->carrier_copy_info.CopyFrom(message->carrier_copy_info());
    stream.Append(" carrier_copy_info='");
    this->carrier_copy_info.mutable_left_hp()->Resize(6, 0);
    KeyValueTrait<sy::CarrierCopyInfo>::encode(stream, this->carrier_copy_info);
    stream.Append("'");
  }
  if (message->carrier_copy_size()) {
    this->carrier_copy.clear();
    for (int32_t i = 0; i < message->carrier_copy_size(); ++i) {
      this->carrier_copy.push_back(message->carrier_copy(i));
    }
    if (message->has_carrier_copy_info()) stream.Append(",");
    stream.Append(" carrier_copy='");
    EncodeKeyValuePair(this->carrier_copy, stream);
    stream.Append("'");
  }
  stream.Append(" where player_id=%ld", this->uid());

  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateObtainedCarriers(SSMessageEntry& entry) {
  MessageSSUpdateObtainedCarriers* message =
      static_cast<MessageSSUpdateObtainedCarriers*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->obtained_carriers_.clear();
  for (int32_t i = 0; i < message->carrier_id_size(); i++)
    this->obtained_carriers_.push_back(message->carrier_id(i));

  ArrayStream<16 * 1024> stream;
  stream.Append("update tactic_%ld set obtained_carriers='",
                this->uid() % RECORD_TABLE_COUNT);
  EncodeIntArray(this->obtained_carriers_, stream);
  stream.Append("' where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateRecharge(SSMessageEntry& entry) {
  MessageSSUpdateRecharge *message = static_cast<MessageSSUpdateRecharge*>(entry.get());
  if (!message) return ERR_INTERNAL;

  TRACE_LOG(logger)("RecordServer Recharge Success, PlayerID:%ld, Time:%d, Money:%d"
      , this->uid(), message->recharge_time(), message->money());

  sy::RechargeInfo info;
  info.set_recharge_time(message->recharge_time());
  info.set_money(message->money());
  this->recharge_.push_back(info);

  DefaultArrayStream stream;
  stream.Append(
      "insert into recharge(`player_id`, `time`, `money`, `goodid`) values (%ld, %d, %d, %d)",
      this->uid(), message->recharge_time(), message->money(), message->goodid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessChangeAvatar(SSMessageEntry& entry) {
  MessageSSChangeAvatar *message = static_cast<MessageSSChangeAvatar*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->player_.set_avatar(message->avatar());

  DefaultArrayStream stream;
  stream.Append("update player_%ld set avatar=%d where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT, message->avatar(),
                this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int64_t RecordPlayer::GetLastMailID() const {
  uint64_t max_mail_id = 0;
  for (VectorMap<int32_t, std::deque<sy::MailInfo> >::const_iterator iter =
           this->mails_.begin();
       iter != this->mails_.end(); ++iter) {
    if (iter->second.size() && iter->second.front().mail_id() > max_mail_id) {
      max_mail_id = iter->second.front().mail_id();
    }
  }
  return max_mail_id;
}

int32_t RecordPlayer::ProcessUpdateMaxFightAttr(SSMessageEntry& entry) {
  MessageSSUpdateMaxFightAttr* message =
      static_cast<MessageSSUpdateMaxFightAttr*>(entry.get());
  if (!message) return ERR_INTERNAL;

  //更新最高战斗力
  this->max_fight_attr_ = message->attr();
  DefaultArrayStream stream;
  stream.Append("update tactic_%ld set max_fight_attr=%ld where player_id=%ld",
                this->uid() % RECORD_TABLE_COUNT, this->max_fight_attr_,
                this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateArmyInfo(SSMessageEntry& entry) {
  MessageSSUpdateLeguageInfo* message =
      static_cast<MessageSSUpdateLeguageInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->has_army_id()) {
    this->army_id_ = message->army_id();
    DefaultArrayStream stream;
    stream.Append("update tactic_%ld set army_id=%ld where player_id=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->army_id_,
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }
  if (message->leave_time()) {
    this->army_leave_time_ = message->leave_time();
    DefaultArrayStream stream;
    stream.Append("update tactic_%ld set leave_time=%ld where player_id=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->army_leave_time_,
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }
  if (message->army_skill_size() > 0) {
    this->army_skill_.clear();
    for (int32_t i = 0; i < message->army_skill_size(); i++) {
      this->army_skill_.push_back(message->army_skill(i));
    }
    this->army_skill_.resize(sy::ArmySkill_ARRAYSIZE, 0);

    DefaultArrayStream stream;
    stream.Append("update tactic_%ld set army_skill='",
                  this->uid() % RECORD_TABLE_COUNT);
    EncodeIntArray(this->army_skill_, stream);
    stream.Append("' where player_id=%ld", this->uid());
    this->ExecSqlAsync(stream.str());
  }

  return ERR_OK;
}

int32_t ProcessUpdateArmyExpInfo(SSMessageEntry& entry) {
  MessageSSUpdateArmyExpInfo* message =
      static_cast<MessageSSUpdateArmyExpInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.Append(
      "update army set `exp` = %d, `level` = %d, `donate_count` = %d, `donate_value` = "
      "%d, `donate_time` = %d, `army_skill` ='",
      message->army_exp(), message->army_level(), message->donate_count(),
      message->donate_value(), message->donate_time());
  std::vector<int32_t> temp;
  for (int32_t i = 0; i < message->army_skill_size(); i++) {
    temp.push_back(message->army_skill(i));
  }
  EncodeIntArray(temp, stream);
  stream.Append("' where army_id = %ld", message->army_id());
  server->PushSql(0, stream.str());

  const std::string& date_str = GetDateStr();
  stream.clear();
  stream.Append(
      "insert into zhanjian_log.`army_exp_%s`(tid, time, server, player_id, "
      "msgid, army_id, army_level,army_exp,army_exp_delta,army_skill) "
      "values(%ld, %ld, %u, %ld, %d, %ld, %d, %d, %d,'",
      date_str.c_str(), message->tid(), GetSeconds(), message->server_id(),
      message->player_id(), message->msgid(), message->army_id(),
      message->army_level(), message->army_exp(), message->exp_delta());
  EncodeIntArray(temp, stream);
  stream.Append("')");
  server->PushSql(0, stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateDailySign(SSMessageEntry& entry) {
  MessageSSUpdateDailySign* message =
      static_cast<MessageSSUpdateDailySign*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->daily_sign_.clear();
  for (int32_t i = 0; i < message->daily_sign_size(); i++)
    this->daily_sign_.push_back(message->daily_sign(i));
  DefaultArrayStream stream;
  stream.Append("update reward_%ld set daily_sign='",
                this->uid() % RECORD_TABLE_COUNT);
  EncodeIntArray(this->daily_sign_, stream);
  stream.Append("' where player_id=%ld", this->uid());
  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateMonthCard(SSMessageEntry& entry) {
  MessageSSUpdateMonthCard* message =
      static_cast<MessageSSUpdateMonthCard*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->month_card_ = message->month_card();
  this->big_month_card_ = message->big_month_card();
  this->life_card_ = message->life_card();
  this->weekly_card_[0] = message->weekly_card();
  this->weekly_card_[1] = message->weekly_card_login();
  this->weekly_card_[2] = message->weekly_card_status();
  this->month_card_1_[0] = message->month_card_1();
  this->month_card_1_[1] = message->month_card_1_login();
  this->month_card_1_[2] = message->month_card_1_status();
  this->month_card_2_[0] = message->month_card_2();
  this->month_card_2_[1] = message->month_card_2_login();
  this->month_card_2_[2] = message->month_card_2_status();

  DefaultArrayStream stream;
  stream.Append(
      "update reward_%ld set month_card=%ld, big_month_card=%ld, life_card=%ld "
      ", weekly_card=%d, weekly_card_login=%d, weekly_card_status=%d"
      ", month_card_1=%d, month_card_1_login=%d, month_card_1_status=%d"
      ", month_card_2=%d, month_card_2_login=%d, month_card_2_status=%d"
      " where player_id=%ld",
      this->uid() % RECORD_TABLE_COUNT, this->month_card_,
      this->big_month_card_, this->life_card_
      , this->weekly_card_[0], this->weekly_card_[1], this->weekly_card_[2]
      , this->month_card_1_[0], this->month_card_1_[1], this->month_card_1_[2]
      , this->month_card_2_[0], this->month_card_2_[1], this->month_card_2_[2]
      , this->uid());
  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateVIPWeekly(SSMessageEntry& entry) {
  MessageSSUpdateVIPWeekly* message =
      static_cast<MessageSSUpdateVIPWeekly*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->vip_weekly_.clear();
  for (int32_t i = 0; i < message->vip_weekly_size(); i++) {
    const KVPair2& pair = message->vip_weekly(i);
    this->vip_weekly_[pair.key()] = pair.value();
  }

  DefaultArrayStream stream;
  stream.Append("update reward_%ld set vip_weekly='",
                this->uid() % RECORD_TABLE_COUNT);
  EncodeKeyValuePair(this->vip_weekly_, stream);
  stream.Append("' where player_id=%ld", this->uid());

  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessAddBuyItemLog(SSMessageEntry& entry) {
  MessageSSAddBuyItemLog* message =
      static_cast<MessageSSAddBuyItemLog*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.clear();
  const std::string& date_str = GetDateStr();
  stream.Append(
      "insert into zhanjian_log.`shop_%s`(tid, time, server, "
      "player_id, commodity_id, shop_type, buy_count) "
      "values(%ld, %ld, %u, %ld, %d, %d, %d)",
      date_str.c_str(), message->tid(), GetSeconds(),
      server->GetMainServerID(this->player_.server()), this->uid(),
      message->commodity_id(), message->shop_type(), message->buy_count());
  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

std::string MakeBuyRecord(const ArmyShopRecord& record) {
  DefaultArrayStream stream;
  stream.clear();
  for (int32_t i = 0; i < record.slot1_size(); ++i) {
    if (i) stream.Append(",");
    stream.Append("%ld", record.slot1(i));
  }
  stream.Append(";");
  for (int32_t i = 0; i < record.slot2_size(); ++i) {
    if (i) stream.Append(",");
    stream.Append("%ld", record.slot2(i));
  }
  stream.Append(";");
  for (int32_t i = 0; i < record.slot3_size(); ++i) {
    if (i) stream.Append(",");
    stream.Append("%ld", record.slot3(i));
  }
  stream.Append(";");
  for (int32_t i = 0; i < record.slot4_size(); ++i) {
    if (i) stream.Append(",");
    stream.Append("%ld", record.slot4(i));
  }
  stream.Append(";");
  for (int32_t i = 0; i < record.slot5_size(); ++i) {
    if (i) stream.Append(",");
    stream.Append("%ld", record.slot5(i));
  }
  stream.Append(";");
  for (int32_t i = 0; i < record.slot6_size(); ++i) {
    if (i) stream.Append(",");
    stream.Append("%ld", record.slot6(i));
  }
  stream.Append(";");
  return stream.str();
}

const ArmyShopRecord ParseBuyRecord(const std::string& str) {
  ArmyShopRecord record;
  std::vector<std::string> list;
  SplitString(str, list, ";");
  for (size_t i = 0; i < list.size(); i++) {
    ::google::protobuf::RepeatedField< ::google::protobuf::int64>* it = NULL;
    if (i == 0) it = record.mutable_slot1();
    if (i == 1) it = record.mutable_slot2();
    if (i == 2) it = record.mutable_slot3();
    if (i == 3) it = record.mutable_slot4();
    if (i == 4) it = record.mutable_slot5();
    if (i == 5) it = record.mutable_slot6();
    std::vector<std::string> uid_list;
    SplitString(list[i], uid_list, ",");
    for (size_t j = 0; j < uid_list.size(); j++)
      it->Add(atol(uid_list[j].c_str()));
  }
  return record;
}

int32_t ProcessUpdateArmyOtherInfo(SSMessageEntry& entry) {
  MessageSSUpdateArmyInfo* message =
      static_cast<MessageSSUpdateArmyInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.clear();
  stream.Append("update army set master_id=%ld, master_name='%s', army_shop='",
                message->master_id(), message->master_name().c_str());
  EncodeKeyValuePair(*message->mutable_army_shop(), stream);

  stream.Append("' ,buy_record='%s', shop_refresh_time=%ld where army_id = %ld",
                MakeBuyRecord(message->buy_record()).c_str(),
                message->shop_refresh_time(), message->army_id());

  server->PushSql(message->army_id(), stream.str());

  return ERR_OK;
}

int32_t LoadArmy(boost::shared_ptr<TcpSession>& session,
                 const uint32_t* server_id, int32_t count) {
  ArrayStream<10 * 1024> stream;
  stream.Append(
      "select army_id, army_name, avatar, level, `exp`, master_id, "
      "master_name, announcement1, announcement2, army_log, army_skill, "
      "donate_count, donate_time, donate_value, army_shop, buy_record, shop_refresh_time"
      " from army where server in(%u",
      server_id[0]);
  for (int32_t i = 1; i < count; ++i) {
    stream.Append(",%u", server_id[i]);
  }
  stream.Append(")");
  const boost::shared_ptr<ResultSet>& result_set =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result_set->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s"
        , stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result_set->error) + RECORD_SQL_ERROR;
  }

  std::vector<std::string> str;
  MessageSSResponseLoadArmy response;
  while (result_set && result_set->IsValid()) {
    str.clear();
    sy::ArmyInfo* info = response.add_info();
    info->set_army_id(result_set->at(0).to_int64());
    info->set_army_name(result_set->at(1).to_str());
    info->set_avatar(result_set->at(2).to_int32());
    info->set_level(result_set->at(3).to_int32());
    info->set_exp(result_set->at(4).to_int64());
    info->set_master_id(result_set->at(5).to_int64());
    info->set_master_name(result_set->at(6).to_str());
    info->set_announcement1(result_set->at(7).to_str());
    info->set_announcement2(result_set->at(8).to_str());
    SplitString(result_set->at(9).to_str(), str, ";");
    for (std::vector<std::string>::const_iterator iter = str.begin();
         iter != str.end(); ++iter) {
      *info->add_log() = *iter;
    }
    str.clear();
    SplitString(result_set->at(10).to_str(), str, ",");
    for (std::vector<std::string>::const_iterator iter = str.begin();
         iter != str.end(); ++iter) {
      info->add_skills(atoi(iter->c_str()));
    }
    info->mutable_skills()->Resize(sy::ArmySkill_ARRAYSIZE, 0);
    info->set_donate_count(result_set->at(11).to_int32());
    info->set_donate_time(result_set->at(12).to_int32());
    info->set_donate_value(result_set->at(13).to_int32());

    ForEachString(StringSlice(result_set->at(14).data, result_set->at(14).len),
                  StringSlice(";"),
                  DecodeKeyValuePair<RepeatedPtrField<sy::ShopCommodityInfo> >(
                      *info->mutable_army_shop()));

    info->mutable_buy_record()->CopyFrom(
        ParseBuyRecord(result_set->at(15).to_str()));

    info->set_shop_refresh_time(result_set->at(16).to_int64());

    result_set->Next();
  }

  server->SendServerMessage(session.get(), MSG_SS_RESPONSE_LOAD_ARMY,
                            &response);
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateTotalRecharge(SSMessageEntry& entry) {
  MessageSSUpdateTotalRecharge* message =
      static_cast<MessageSSUpdateTotalRecharge*>(entry.get());
  if (!message) return ERR_INTERNAL;

  this->player_.set_total_recharge(message->total_recharge());

  DefaultArrayStream stream;
  stream.Append("update player_%ld set total_recharge=%d where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT,
                this->player_.total_recharge(), this->uid());
  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::LoadActivityRecord(MySqlConnection& conn) {
  {
    DefaultArrayStream stream;
    stream.Append(
        "delete from activity_record_new_%ld where award = '' and "
        "activity_id not in (select activity_id from activity_new where "
        "end_time > %ld) and player_id = %ld",
        this->uid() % RECORD_TABLE_COUNT, GetSeconds(), this->uid());
    int32_t res_c = conn.ExecSql(stream.c_str(), stream.size());
    if (res_c < 0) {
      ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
      return abs(res_c) + RECORD_SQL_ERROR;
    }
    if (res_c > 0) {
      TRACE_LOG(logger)("Delete ActivityRecord: %s", stream.c_str());
    }
  }

  {
    DefaultArrayStream stream;
    stream.Append(
        "select `activity_type`, `activity_id`,`refresh_time`, "
        "`bought_record`,`award` "
        " from activity_record_new_%ld where player_id=%ld",
        this->uid() % RECORD_TABLE_COUNT, this->uid());
    const boost::shared_ptr<ResultSet>& result =
        conn.ExecSelect(stream.c_str(), stream.size());

    if (result->error) {
      ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), conn.GetLastError().c_str());
      return abs(result->error) + RECORD_SQL_ERROR;
    }

    activity_record_new_.clear();
    while (result && result->IsValid()) {
      int32_t type = result->at(0).to_int32();
      int64_t id = result->at(1).to_int64();
      sy::ActivityRecord& record =
          activity_record_new_[std::make_pair(type, id)];
      record.set_type(type);
      record.set_id(id);
      record.set_refresh_time(result->at(2).to_int64());
      ForEachString(StringSlice(result->at(3).data, result->at(3).len),
                    StringSlice(";"),
                    DecodeKeyValuePair<RepeatedPtrField<sy::KVPair2> >(
                        *record.mutable_record()));
      std::vector<std::string> strvct;
      std::string award = result->at(4).to_str();
      SplitString(award, strvct, "@");
      for (size_t i = 0; i < strvct.size(); i++)
        if (!strvct.empty()) record.add_award(strvct[i]);
      result->Next();
      TRACE_LOG(logger)("LoadActivity: player_id:%ld, type:%d, id:%ld", this->uid(), type, id);
    }
  }

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateActivityRecordNew(SSMessageEntry& entry) {
  MessageSSUpdateActivityRecordNew* message =
      static_cast<MessageSSUpdateActivityRecordNew*>(entry.get());
  if (!message) return ERR_INTERNAL;

  std::pair<int32_t, int64_t> pair =
      std::make_pair(message->records().type(), message->records().id());
  this->activity_record_new_[pair] = message->records();
  sy::ActivityRecord& record = this->activity_record_new_[pair];

  DefaultArrayStream stream;
  stream.Append(
      "insert into activity_record_new_%ld "
      "(player_id,activity_type,activity_id,refresh_time,bought_record,award) "
      "values(%ld,%d,%ld,%ld,'",
      this->uid() % RECORD_TABLE_COUNT, this->uid(), record.type(), record.id(),
      record.refresh_time());
  EncodeKeyValuePair(record.record(), stream);
  stream.Append("','");
  for (int32_t i = 0; i < record.award_size(); i++) {
    stream.Append(record.award(i).c_str(), "");
    stream.Append("@");
  }
  stream.Append(
      "') on  DUPLICATE KEY UPDATE refresh_time=values(refresh_time),"
      "bought_record=values(bought_record),award=values(award)");

  this->ExecSqlAsync(stream.str());

  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateCreateTime(SSMessageEntry& entry) {
  MessageSSUpdateCreateTime* message =
      static_cast<MessageSSUpdateCreateTime*>(entry.get());
  if (!message) return ERR_INTERNAL;
  player_.set_create_time(message->create_time());
  DefaultArrayStream stream;
  stream.Append("update player_%ld set create_time =%ld where uid=%ld",
                this->uid() % RECORD_TABLE_COUNT, player_.create_time(),
                this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateLoginDays(SSMessageEntry& entry) {
  MessageSSUpdateLoginDays* message =
      static_cast<MessageSSUpdateLoginDays*>(entry.get());
  if (!message) return ERR_INTERNAL;
  this->player_.set_login_days(message->login_days());

  DefaultArrayStream stream;
  stream.Append("update player_%ld set login_days=%d where uid=%ld"
      , this->uid() % RECORD_TABLE_COUNT, this->player_.login_days()
      , this->uid());
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateUpdateDeviceID(SSMessageEntry& entry) {
  MessageSSUpdateDeviceID* message = static_cast<MessageSSUpdateDeviceID*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!message->device_id().empty() && !message->idfa().empty() &&
      message->device_id() != message->idfa()) {
    DEBUG_LOG(logger)("IDFA, DeviceID:%s, IDFA:%s"
        , message->device_id().c_str(), message->idfa().c_str());
  }
  const std::string& device_id = server->mysql_conn().EscapeString(message->idfa());
  this->player_.set_device_id(device_id);

  DefaultArrayStream stream;
  stream.Append("update player_%ld set device_id='%s' where uid=%ld"
      , this->uid() % RECORD_TABLE_COUNT, device_id.c_str(), this->uid()
      );
  this->ExecSqlAsync(stream.str());
  return ERR_OK;
}

int32_t RecordPlayer::ProcessUpdateMedalCopyID(SSMessageEntry& entry) {
  MessageUpdateMedalCopyID* message =
      static_cast<MessageUpdateMedalCopyID*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->has_medal_copy_id()) {
    this->medal_copy_id_ = message->medal_copy_id();
    DefaultArrayStream stream;
    stream.Append("update copy_%ld set medal_copy_id=%d where player_id=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->medal_copy_id_,
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }
  if (message->has_medal_star()) {
    this->medal_star_ = message->medal_star();
    DefaultArrayStream stream;
    stream.Append("update copy_%ld set medal_star=%d where player_id=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->medal_star_,
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }
  if (message->has_medal_state()) {
    this->medal_state_ = message->medal_state();
    DefaultArrayStream stream;
    stream.Append("update copy_%ld set medal_state='%s' where player_id=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->medal_state_.c_str(),
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }

  if (message->has_medal_achi()) {
    this->medal_achi_= message->medal_achi();
    DefaultArrayStream stream;
    stream.Append("update copy_%ld set medal_achi=%d where player_id=%ld",
                  this->uid() % RECORD_TABLE_COUNT, this->medal_achi_,
                  this->uid());
    this->ExecSqlAsync(stream.str());
  }

  return ERR_OK;
}

