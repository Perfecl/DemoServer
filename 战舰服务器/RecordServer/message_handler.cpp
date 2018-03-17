#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <system.h>
#include <array_stream.h>
#include <str_util.h>
#include <system.h>
#include "record_player.h"
#include "server.h"
#include "callback.h"
#include "message_handler.h"

using namespace sy;
using namespace intranet;

int32_t ProcessServerLogin(SSMessageEntry& entry) {
  MessageSSServerLogin* message = static_cast<MessageSSServerLogin*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->server_ids_size() <= 0) {
    ERROR_LOG(logger)("ServerLogin Fail, ServerCount:0");
    entry.session_ptr->Close();
    return ERR_INTERNAL;
  }

  uint32_t server_id = message->server_ids(0);
  int32_t server_count = message->server_ids_size();
  for (int i = 0; i < server_count; ++i) {
    server->AddServer(message->server_ids(i), entry.session_ptr);
    if (i) server->AddServerMap(message->server_ids(i), server_id);
  }
  TRACE_LOG(logger)("New Server Connected:%s, ServerID:%u",
                    message->server_name().c_str(), server_id);

  return ERR_OK;
}

int32_t ProcessLoadPlayerInfo(SSMessageEntry& entry) {
  MessageSSRequestGetPlayerInfo* message =
      static_cast<MessageSSRequestGetPlayerInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  RecordPlayer *player = server->GetOrNewPlayer(message->uid());
  if (!player) return ERR_INTERNAL;
  player->session(entry.session_ptr);
  player->active();
  player->LoadPlayerAsync(message->msgid());

  return ERR_OK;
}

int32_t ProcessPingPong(SSMessageEntry& entry) {
  server->SendServerMessage(entry.session_ptr.get(), entry.head.msgid,
                            entry.get());
  return ERR_OK;
}

int32_t ProcessGetPKRankList(SSMessageEntry& entry) {
  MessageSSRequestGetPKRankList* message =
      static_cast<MessageSSRequestGetPKRankList*>(entry.get());
  if (!message) return ERR_INTERNAL;
  ArrayStream<1024 * 8> stream;
  stream.Append("select rank, player_id from pk_rank_list where server=%u",
                message->server_ids(0));
  for (int32_t i = 1; i < message->server_ids_size(); ++i) {
    stream.Append(" or server=%u", message->server_ids(i));
  }
  stream.Append(" order by rank");
  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s"
        , stream.c_str()
        , name_thread->mysql_conn().GetLastError().c_str()
        );
    return abs(result->error) + RECORD_SQL_ERROR;
  }

  MessageSSResponseGetPKRankList response;
  while (result && result->IsValid()) {
    sy::PKRankInfo* info = response.add_ranks();
    info->set_rank(result->at(0).to_int32());
    info->set_player_id(result->at(1).to_int64());
    result->Next();
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_GET_PK_RANK_LIST, &response);
  return ERR_OK;
}

int32_t ProcessGetUIDByName(SSMessageEntry& entry) {
  MessageSSRequestGetUIDByName* message =
      static_cast<MessageSSRequestGetUIDByName*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DefaultArrayStream stream;
  const std::string escape_name = name_thread->mysql_conn().EscapeString(
      message->name().c_str(), message->name().length());
  stream.Append("select uid from role_name where name='");
  stream.Append(escape_name);
  stream.Append("'");

  MessageSSResponseGetUIDByName response;
  response.set_name(message->name());
  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result && result->IsValid()) {
    response.set_player_id(result->at(0).to_int64());
  } else {
    response.set_player_id(0);
  }

  if (entry.head.dest_type == ENTRY_TYPE_PLAYER) {
    server->SendPlayerMessage(entry.session_ptr.get(), entry.head.dest_id,
                              MSG_SS_RESPONSE_GET_UID_BY_NAME, &response);
  } else {
    server->SendServerMessage(entry.session_ptr.get(),
                              MSG_SS_RESPONSE_GET_UID_BY_NAME, &response);
  }
  return ERR_OK;
}

int32_t ProcessLoadMultiPlayer(SSMessageEntry& entry) {
  MessageSSRequestLoadMultiPlayer* message =
      static_cast<MessageSSRequestLoadMultiPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSResponseLoadMultiPlayer response;
  response.mutable_request()->CopyFrom(*message);
  SharedCounter counter(new boost::atomic_int32_t(0));

  for (int32_t i = 0; i < message->player_ids_size(); ++i) {
    int64_t player_id = message->player_ids(i);
    RecordPlayer* player = server->GetOrNewPlayer(player_id);
    if (!player) {
      ERROR_LOG(logger)("ProcessLoadMultiPlayer CreatePlayer Fail");
      return ERR_INTERNAL;
    }
    boost::shared_ptr<ClosureLoadMultiPlayer> ptr(new ClosureLoadMultiPlayer(
        player, server->GetAsyncConn(player->uid()), entry, response, counter));
    server->PushAsyncClosure(player->uid(), ptr);
  }

  if (!message->player_ids_size()) {
     if (entry.head.dest_type == ENTRY_TYPE_PLAYER) {
      server->SendPlayerMessage(entry.session_ptr.get(), entry.head.dest_id,
                                MSG_SS_RESPONSE_LOAD_MULTI_PLAYER, &response);
    } else {
      server->SendServerMessage(entry.session_ptr.get(),
                                MSG_SS_RESPONSE_LOAD_MULTI_PLAYER, &response);
    }
  }

  return ERR_OK;
}

int32_t ProcessUpdateDailyPKRankInfo(SSMessageEntry& entry) {
  MessageSSUpdateDailyPkRankInfo* message =
      static_cast<MessageSSUpdateDailyPkRankInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (message->rank_size() != message->player_ids_size() ||
      message->rank_size() != message->time_size())
    return ERR_INTERNAL;

  DefaultArrayStream stream;
  for (int32_t i = 0; i < message->rank_size(); ++i) {
    stream.clear();
    RecordPlayer *player = server->GetPlayerByID(message->player_ids(i));
    if (player) {
      player->UpdateDailyPKRankInfo(message->rank(i), message->time(i));
    }
    stream.Append(
        "update reward_%ld set pk_rank_reward_rank=%d, pk_rank_reward_time=%d "
        "where player_id=%ld",
        message->player_ids(i) % RECORD_TABLE_COUNT, message->rank(i),
        message->time(i), message->player_ids(i));
    server->PushSql(message->player_ids(i), stream.str());
  }

  return ERR_OK;
}

struct KickAllPlayers {
  KickAllPlayers(std::vector<RecordPlayer*>& players) : players(players) {}
  std::vector<RecordPlayer*>& players;
  bool operator()(RecordPlayer* p) const {
    players.push_back(p);
    return true;
  }
};

int32_t ProcessKickPlayer(SSMessageEntry& entry) {
  MessageSSRequestKickUser* message =
      static_cast<MessageSSRequestKickUser*>(entry.get());
  if (!message) return ERR_INTERNAL;

  std::vector<RecordPlayer*> players;
  if (message->all()) {
    KickAllPlayers for_each(players);
    server->Players().ForEach(for_each);
  } else {
    for (int32_t i = 0; i < message->player_id_size(); ++i) {
      RecordPlayer* player = server->GetPlayerByID(message->player_id(i));
      if (player) {
        players.push_back(player);
      }
    }
  }
  for (std::vector<RecordPlayer*>::iterator iter = players.begin();
       iter != players.end(); ++iter) {
    TRACE_LOG(logger)("GM KickPlayer:%ld", (*iter)->uid());
    server->ErasePlayer((*iter)->uid());
  }

  return ERR_OK;
}

int32_t ProcessSetAccountStatus(SSMessageEntry& entry) {
  MessageSSSetAccountStatus* message =
      static_cast<MessageSSSetAccountStatus*>(entry.get());
  if (!message) return ERR_INTERNAL;

  RecordPlayer *player = server->GetPlayerByID(message->player_id());
  if (player) {
    if (message->has_status()) player->player_info().set_status(message->status());
    if (message->has_status_time()) player->player_info().set_status_time(message->status_time());
    if (message->has_flag()) player->player_info().set_flag(message->flag());
  }
  int32_t count = 0;

  DefaultArrayStream stream;
  stream.Append("update player_%ld set ",
                message->player_id() % RECORD_TABLE_COUNT);
  if (message->has_status()) {
    if (count) stream.Append(",");
    stream.Append("status=%d", message->status());
    ++count;
  }
  if (message->has_status_time()) {
    if (count) stream.Append(",");
    stream.Append("status_time=%d", message->status_time());
    ++count;
  }
  if (message->has_flag()) {
    if (count) stream.Append(",");
    stream.Append("flag=%d", message->flag());
    ++count;
  }
  stream.Append(" where uid=%ld", message->player_id());

  if (count) server->PushSql(message->player_id(), stream.str());
  return ERR_OK;
}

int32_t ProcessSetNotice(SSMessageEntry& entry) {
  MessageSSServerNotice* message =
      static_cast<MessageSSServerNotice*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  if (message->has_notice()) {
    const NoticeInfo& info = message->notice();

    stream.Append(
        "insert into "
        "server_notice(notice_id,server_id,notice_type,start_time,end_time,"
        "interval_time,content,`order`,link_url) "
        " values(%ld,%u,%d,%ld,%ld,%d,'%s',%d,'%s') on duplicate key "
        "update "
        "notice_type=values(notice_type),start_time=values(start_time),end_"
        "time=values(end_time),"
        "interval_time=values(interval_time),content=values(content),`order`="
        "values(`order`),link_url=values(link_url)",
        info.tid(), info.server_id(), info.type(), info.begin_time(),
        info.end_time(), info.interval(),
        server->EscapeString(info.content().c_str(), info.content().length())
            .c_str(),
        info.order(),
        server->EscapeString(info.link_url().c_str(), info.link_url().length())
            .c_str());

    server->PushSql(info.tid(), stream.str());
  }
  if (message->has_delete_id()) {
    stream.Append(
        "delete from server_notice where server_id=%u and notice_id=%ld",
        message->server_id(), message->delete_id());
    server->PushSql(message->delete_id(), stream.str());
  }

  return ERR_OK;
}

int32_t ProcessUpdateCreatePlayerInfo(SSMessageEntry& entry) {
  MessageSSUpdateCreatePlayerInfo* message =
      static_cast<MessageSSUpdateCreatePlayerInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;

  const std::string& openid = server->mysql_conn().EscapeString(message->openid());
  const std::string& channel = server->mysql_conn().EscapeString(message->channel());
  const std::string& date_str = GetDateStr();
  stream.Append(
      "insert into zhanjian_log.`newplayer_%s`(server, openid, player_id, time, channel)"
      "values(%u, '%s', %ld, %d, '%s')"
      , date_str.c_str()
      , message->server()
      , openid.c_str()
      , message->player_id()
      , message->time()
      , channel.c_str()
      );

  server->PushSql(message->player_id(), stream.str());
  return ERR_OK;
}


int32_t LoadArmyMember(boost::shared_ptr<TcpSession>& session,
                       const uint32_t* server_id, int32_t count) {
  ArrayStream<10 * 1024> stream;
  stream.Append(
      "select player_id, army_id, name, position, level, vip_level, "
      "fight, avatar, army_exp, today_exp, army_update_time "
      "from army_member where server in(%u",
      server_id[0]);
  for (int32_t i = 1; i < count; ++i) {
    stream.Append(",%u", server_id[i]);
  }
  stream.Append(")");
  const boost::shared_ptr<ResultSet>& result_set =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result_set->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s"
        , stream.c_str()
        , name_thread->mysql_conn().GetLastError().c_str()
        );
    return abs(result_set->error) + RECORD_SQL_ERROR;
  }

  MessageSSReponseLoadArmyMember response;

  while (result_set && result_set->IsValid()) {
    sy::ArmyMemberInfo* member = response.add_member();
    member->set_player_id(result_set->at(0).to_int64());
    member->set_army_id(result_set->at(1).to_int64());
    member->set_name(result_set->at(2).to_str());
    member->set_position(result_set->at(3).to_int32());
    member->set_level(result_set->at(4).to_int32());
    member->set_vip_level(result_set->at(5).to_int32());
    member->set_fight(result_set->at(6).to_int64());
    member->set_avatar(result_set->at(7).to_int32());
    member->set_army_exp(result_set->at(8).to_int32());
    member->set_today_exp(result_set->at(9).to_int32());
    member->set_update_time(result_set->at(10).to_int32());

    result_set->Next();
  }

  server->SendServerMessage(session.get(), MSG_SS_RESPONSE_LOAD_ARMY_MEMBER,
                            &response);
  return ERR_OK;
}

int32_t LoadArmyApply(boost::shared_ptr<TcpSession>& session,
                      const uint32_t* server_id, int32_t count) {
  ArrayStream<10 * 1024> stream;
  stream.Append(
      "select player_id, army_id, name, level, vip_level, fight, avatar "
      "from army_apply where server_id in(%u",
      server_id[0]);
  for (int32_t i = 1; i < count; ++i) {
    stream.Append(",%u", server_id[i]);
  }
  stream.Append(")");
  const boost::shared_ptr<ResultSet>& result_set =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result_set->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s"
        , stream.c_str()
        , name_thread->mysql_conn().GetLastError().c_str()
        );
    return abs(result_set->error) + RECORD_SQL_ERROR;
  }

  MessageSSReponseLoadArmyMember response;

  while (result_set && result_set->IsValid()) {
    sy::ArmyApplyInfo* info = response.add_apply();

    info->set_player_id(result_set->at(0).to_int64());
    info->set_army_id(result_set->at(1).to_int64());
    info->set_name(result_set->at(2).to_str());
    info->set_level(result_set->at(3).to_int32());
    info->set_vip_level(result_set->at(4).to_int32());
    info->set_fight(result_set->at(5).to_int64());
    info->set_avatar(result_set->at(6).to_int32());

    result_set->Next();
  }

  server->SendServerMessage(session.get(), MSG_SS_RESPONSE_LOAD_ARMY_MEMBER,
                            &response);
  return ERR_OK;
}

int32_t ProcessLoadArmyInfo(SSMessageEntry& entry) {
  MessageSSLoadArmy* message = static_cast<MessageSSLoadArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int32_t result = 0;
  result = LoadArmy(entry.session_ptr, &*message->mutable_server_id()->begin(),
                    message->server_id_size());
  if (result) return result;
  result = LoadArmyMember(entry.session_ptr, &*message->mutable_server_id()->begin(),
                     message->server_id_size());
  if (result) return result;
  result = LoadArmyApply(entry.session_ptr, &*message->mutable_server_id()->begin(),
                    message->server_id_size());
  if (result) return result;
  return ERR_OK;
}

void UpdateArmyMember(uint32_t server_id, const sy::ArmyMemberInfo& member) {
  DefaultArrayStream stream;
  const std::string& name = name_thread->mysql_conn().EscapeString(member.name());
  stream.Append(
      "insert into army_member(player_id, army_id, server, name, position, "
      "level, vip_level, fight, avatar, army_exp, today_exp, army_update_time) "
      "values (%ld, %ld, %u, '%s', %d, %d, %d, %ld, %d, %d, %d, %d) ON "
      "DUPLICATE KEY UPDATE "
      " name=values(name), army_id=values(army_id), position=values(position), "
      "level=values(level), vip_level=values(vip_level), fight=values(fight),"
      "avatar=values(avatar),army_exp=values(army_exp),"
      "today_exp=values(today_exp),army_update_time=values(army_update_time)",
      member.player_id(), member.army_id(), server_id, name.c_str(),
      member.position(), member.level(), member.vip_level(), member.fight(),
      member.avatar(), member.army_exp(), member.today_exp(),
      member.update_time());
  server->PushSql(member.player_id(), stream.str());
}

int32_t ProcessRequestCreateArmy(SSMessageEntry& entry) {
  MessageSSRequestCreateArmy* message =
      static_cast<MessageSSRequestCreateArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const sy::ArmyInfo& army = message->info();
  const std::string& army_name = name_thread->mysql_conn().EscapeString(army.army_name());
  const std::string& master_name = name_thread->mysql_conn().EscapeString(army.master_name());

  MessageSSResponseCreateArmy response;
  response.mutable_info()->CopyFrom(message->info());
  response.set_master_id(army.master_id());

  DefaultArrayStream stream;
  stream.Append(
      "insert into army(army_id, army_name, server, avatar, level, master_id, "
      "master_name) values (%ld, '%s', %u, %d, %d, %ld, '%s')",
      army.army_id(), army_name.c_str(), message->server(), army.avatar(),
      army.level(), army.master_id(), master_name.c_str());

  //把军团插入到军团表里面去
  int32_t result = name_thread->mysql_conn().ExecSql(stream.c_str(), stream.size());
  if (result <= 0) {
    ERROR_LOG(logger)("ExecSql fail, %s", name_thread->mysql_conn().GetLastError().c_str());
    response.set_is_fail(true);
    server->SendServerMessage(entry.session_ptr.get(),
                              MSG_SS_RESPONSE_CREATE_ARMY, &response);
    return ERR_OK;
  }
  //把自己插入到Member里面去
  UpdateArmyMember(message->server(), message->master());

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_CREATE_ARMY, &response);
  return ERR_OK;
}

//删除军团
//删除军团的成员
//删除军团的申请
int32_t ProcessRequestDestoryArmy(SSMessageEntry& entry) {
  MessageSSRequestDestoryArmy* message =
      static_cast<MessageSSRequestDestoryArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DefaultArrayStream stream;
  stream.Append("delete from army where army_id=%ld", message->army_id());
  server->PushSql(0, stream.str());
  for (int32_t i = 0; i < message->player_ids_size(); ++i) {
    stream.clear();
    stream.Append("delete from army_member where player_id=%ld", message->player_ids(i));
    server->PushSql(message->player_ids(i), stream.str());
  }
  stream.clear();
  stream.Append("delete from army_apply where army_id=%ld", message->army_id());
  server->PushSql(0, stream.str());
  return ERR_OK;
}

int32_t ProcessOnPlayerJoinArmy(SSMessageEntry& entry) {
  MessageSSOnJoinArmy* message = static_cast<MessageSSOnJoinArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DefaultArrayStream stream;
  RecordPlayer* player = server->GetPlayerByID(message->player_id());
  if (player) {
    player->army_id(message->army_id());
  }
  stream.Append("update tactic_%ld set army_id=%ld where player_id=%ld",
                message->player_id() % RECORD_TABLE_COUNT, message->army_id(),
                message->player_id());
  server->PushSql(message->player_id(), stream.str());
  return ERR_OK;
}

int32_t ProcessUpdateArmyApply(SSMessageEntry& entry) {
  MessageSSUpdateArmyApply* message =
      static_cast<MessageSSUpdateArmyApply*>(entry.get());
  if (!message) return ERR_INTERNAL;

  int64_t army_id = 0;
  DefaultArrayStream stream;
  if (message->has_info()) {
    sy::ArmyApplyInfo& apply = *message->mutable_info();
    const std::string& name = server->mysql_conn().EscapeString(apply.name());
    army_id = apply.army_id();
    stream.Append(
        "insert into army_apply(player_id, army_id, server_id, name, avatar, "
        "level, vip_level, fight) values (%ld, %ld, %u, '%s', %d, %d, %d, "
        "%ld) ON duplicate key update server_id=values(server_id), "
        "name=values(name), avatar=values(avatar), level=values(level), "
        "vip_level=values(vip_level), fight=values(fight)",
        apply.player_id(), apply.army_id(), message->server(), name.c_str(),
        apply.avatar(), apply.level(), apply.vip_level(), apply.fight());
  } else {
    army_id = message->army_id();
    stream.Append("delete from army_apply where army_id=%ld and player_id=%ld",
                  message->army_id(), message->player_id());
  }

  server->PushSql(army_id, stream.str());
  return ERR_OK;
}

int32_t ProcessUpdateArmyLog(SSMessageEntry& entry) {
  MessageSSUpdateArmyLog* message = static_cast<MessageSSUpdateArmyLog*>(entry.get());
  if (!message) return ERR_INTERNAL;
  ArrayStream<1024*10> stream;
  stream.Append("update army set army_log='");
  for (int32_t i = 0; i < message->logs_size(); ++i) {
    if (i != 0) stream.Append(";");
    stream.Append(message->logs(i));
  }
  stream.Append("' where army_id=%ld", message->army_id());
  server->PushSql(message->army_id(), stream.str());
  return ERR_OK;
}

int32_t ProcessUpdateArmyMemberStatus(SSMessageEntry& entry) {
  MessageSSUpdateLeguageInfo* message =
      static_cast<MessageSSUpdateLeguageInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;
  RecordPlayer *player = server->GetPlayerByID(message->player_id());
  if (player) {
    player->set_army_id(message->army_id());
    if (message->leave_time()) player->set_army_leave_time(message->leave_time());
  }

  DefaultArrayStream stream;
  stream.Append("update tactic_%ld set army_id=%ld",
                message->player_id() % RECORD_TABLE_COUNT, message->army_id());
  if (message->leave_time()) {
    stream.Append(", leave_time=%ld", message->leave_time());
  }
  stream.Append(" where player_id=%ld", message->player_id());
  server->PushSql(message->player_id(), stream.str());
  return ERR_OK;
}

int32_t ProcssUpdateArmyNotice(SSMessageEntry& entry) {
  MessageSSUpdateArmyNotice* message =
      static_cast<MessageSSUpdateArmyNotice*>(entry.get());
  if (!message) return ERR_INTERNAL;
  ArrayStream<1024 * 10> stream;
  const std::string& notice1 = server->mysql_conn().EscapeString(message->notice1());
  const std::string& notice2 = server->mysql_conn().EscapeString(message->notice2());
  stream.Append(
      "update army set announcement1='%s', announcement2='%s', avatar=%d where "
      "army_id=%ld",
      notice1.c_str(), notice2.c_str(), message->avatar(), message->army_id());
  server->PushSql(message->army_id(), stream.str());
  return ERR_OK;
}

int32_t ProcessUpdatePlayerRecharge(SSMessageEntry& entry) {
  MessageSSUpdateRechargeDetails* message =
      static_cast<MessageSSUpdateRechargeDetails*>(entry.get());
  if (!message) return ERR_INTERNAL;

  message->set_stage(3);

  const std::string& goods_id = server->mysql_conn().EscapeString(message->goods_id());
  const std::string& timestamp = server->mysql_conn().EscapeString(message->timestamp());

  DefaultArrayStream stream;
  stream.Append(
      "insert ignore into recharge_details(order_id, device_type, user_id, "
      "pay_amount, currency_code, channel_id, server_id, role_id, goods_type, "
      "game_coin, goods_id, timestamp, stage, recharge_time, tid) values ('%s', %d, '%s', %d, "
      "'%s', '%s', %u, %ld, %d, %d, '%s', '%s', %d, %d, %ld)",
      message->order_id().c_str(), message->device_type(),
      message->user_id().c_str(), message->pay_amount(),
      message->currency_code().c_str(), message->channel_id().c_str(),
      message->server_id(), message->role_id(), message->goods_type(),
      message->game_coin(), goods_id.c_str(), timestamp.c_str(),
      1, message->request_time(), message->tid());
  server->PushSql(message->role_id(), stream.str());

  stream.clear();
  stream.Append(
      "insert into recharge_details(order_id, device_type, user_id, "
      "pay_amount, currency_code, channel_id, server_id, role_id, goods_type, "
      "game_coin, goods_id, timestamp, stage, recharge_time, tid) values ('%s', %d, '%s', %d, "
      "'%s', '%s', %u, %ld, %d, %d, '%s', '%s', %d, %d, %ld)",
      message->order_id().c_str(), message->device_type(),
      message->user_id().c_str(), message->pay_amount(),
      message->currency_code().c_str(), message->channel_id().c_str(),
      message->server_id(), message->role_id(), message->goods_type(),
      message->game_coin(), goods_id.c_str(), timestamp.c_str(),
      message->stage(), message->request_time(), message->tid());
  server->PushSql(message->role_id(), stream.str());

  return ERR_OK;
}

int32_t ProcessUpdateFirstRechargeInfo(SSMessageEntry& entry) {
  MessageSSUpdateFirstRechargeInfo* message =
      static_cast<MessageSSUpdateFirstRechargeInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const std::string& openid = server->mysql_conn().EscapeString(message->openid());
  DefaultArrayStream stream1, stream2;
  stream1.Append(
      "insert ignore into recharge_openid(server, openid, player_id, "
      "recharge_time, money) values (%u, '%s', %ld, %d, %d)",
      message->server_id(), openid.c_str(), message->player_id(),
      message->recharge_time(), message->money());
  stream2.Append(
      "insert ignore into recharge_uid(server, player_id, recharge_time, "
      "money) values(%u, %ld, %d, %d)",
      message->server_id(), message->player_id(), message->recharge_time(),
      message->money());

  server->PushSql(message->player_id(), stream1.str());
  server->PushSql(message->player_id(), stream2.str());
  return ERR_OK;
}

int32_t ProcessUpdateDialogID(SSMessageEntry& entry) {
  MessageSSServerSetDialog* message =
      static_cast<MessageSSServerSetDialog*>(entry.get());
  if (!message) return ERR_INTERNAL;

  RecordPlayer* player = server->GetPlayerByID(message->player_id());
  if (player) {
    player->set_dialog_id(message->guide());
  }

  const std::string& escape_guide = server->mysql_conn().EscapeString(
      message->guide().c_str(), message->guide().length());
  DefaultArrayStream stream;
  stream.Append("update player_%ld set dialog_id='%s' where uid=%ld",
                message->player_id() % RECORD_TABLE_COUNT, escape_guide.c_str(),
                message->player_id());
  server->PushSql(message->player_id(), stream.str());

  return ERR_OK;
}

int32_t ProcessServerStartTime(SSMessageEntry& entry) {
  MessageSSRequestGetServerStart* message =
      static_cast<MessageSSRequestGetServerStart*>(entry.get());
  if (!message) return ERR_INTERNAL;
  time_t start_time = 0;
  DefaultArrayStream stream;
  stream.Append(
      "select server_start_time FROM zhanjian_gm.server where server_id = %u",
      message->server_id());
  const boost::shared_ptr<ResultSet>& result_set =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result_set->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result_set->error) + RECORD_SQL_ERROR;
  }

  if (result_set && result_set->IsValid()) {
    start_time = result_set->at(0).to_int64();
  }

  MessageSSResponseGetServerStart response;
  response.set_start_time(start_time);
  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_GET_SERVER_START, &response);

  return ERR_OK;
}

int32_t ProcessUpdateArmyMember(SSMessageEntry& entry) {
  MessageSSUpdateArmyMember* message =
      static_cast<MessageSSUpdateArmyMember*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->has_army_id()) {
    DefaultArrayStream stream;
    stream.Append(
        "delete from army_member where army_id = %ld and player_id = %ld",
        message->army_id(), message->member_id());
    server->PushSql(message->member_id(), stream.str());
  } else {
    UpdateArmyMember(message->server(), message->member());
  }

  return ERR_OK;
}

int32_t ProcessResponseLoadTimeActivityNew(SSMessageEntry& entry) {
  MessageRequestLoadTimeActivityNew* message =
      static_cast<MessageRequestLoadTimeActivityNew*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.Append(
      "select "
      "activity_type,activity_id,begin_time,end_time,field,content,description "
      "from activity_new where server_id = %u and end_time > %ld and activity_type > 0",
      message->server_id(), GetSeconds());

  const boost::shared_ptr<ResultSet>& result_set =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  if (result_set->error) {
    ERROR_LOG(logger)("ExecSql fail:%s, %s", stream.c_str(), name_thread->mysql_conn().GetLastError().c_str());
    return abs(result_set->error) + RECORD_SQL_ERROR;
  }

  MessageResponseLoadTimeActivityNew res;
  while (result_set && result_set->IsValid()) {
    sy::TimeActivityInfo* info = res.add_activitis();
    info->set_type(result_set->at(0).to_int32());
    info->set_id(result_set->at(1).to_int64());
    info->set_begin_time(result_set->at(2).to_int64());
    info->set_end_time(result_set->at(3).to_int64());
    std::string headers = result_set->at(4).to_c_str();
    std::vector<std::string> field;
    SplitString(headers, field, "@*");
    for (size_t i = 0; i < field.size(); i++) info->add_headers(field[i]);
    std::string content = result_set->at(5).to_c_str();
    std::vector<std::string> content_row;
    SplitString(content, content_row, "#$");
    for (size_t i = 0; i < content_row.size(); i++) {
      TimeActivityRow* row = info->add_rows();
      std::vector<std::string> content_column;
      SplitString(content_row[i], content_column, "@*");
      for (size_t j = 0; j < content_column.size(); j++)
        row->add_columns(content_column[j]);
    }
    info->set_description(result_set->at(6).to_c_str());
    result_set->Next();
  }
  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_LOAD_TIME_ACTIVITY_NEW, &res);
  return ERR_OK;
}

int32_t ProcessUpdateAstrologyAward(SSMessageEntry& entry) {
  MessageSSUpdateAstrologyAward* message =
      static_cast<MessageSSUpdateAstrologyAward*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  stream.Append(
      "insert into server_shop (server_id, shop_data, astrology_country_id, "
      "astrology_refresh_time) values(%u,'',%d,%ld) "
      " on duplicate key update "
      "astrology_country_id=values(astrology_country_id), "
      "astrology_refresh_time=values(astrology_refresh_time)",
      message->server_id(), message->astrology_country_id(),
      message->astrology_refresh_time());

  server->PushSql(message->server_id(), stream.str());

  return ERR_OK;
}

int32_t ProcessUpdateServerCopyInfo(SSMessageEntry& entry) {
  MessageSSUpdateServerCopyInfo* message = static_cast<MessageSSUpdateServerCopyInfo*>(entry.get());
  if (!message) return ERR_INTERNAL;

  //2017-01-01
  char date[12] = {0};
  FormatDate(date, GetTime());
  ArrayStream<1024 * 16> stream;
  stream.clear();
  stream.Append("insert into zhanjian_gm.stat_copy_count(`date`, `server`, `copy_type`, `count`, `max_copy_id`, `player_count`) values");
  for (int32_t index = 0; index < message->info_size(); ++index) {
    const intranet::CopyStatInfo& info = message->info(index);
    if (index != 0) stream.Append(",");
    stream.Append("('%s', %u, %d, %d, %d, %d)", date, message->server_id(),
                  info.copy_type(), info.count(), info.max_copy_id(),
                  info.player_count());
  }

  stream.Append(
      "ON DUPLICATE KEY update count=values(count), "
      "max_copy_id=values(max_copy_id), player_count=values(player_count)");
  if (message->info_size()) server->PushSql(message->server_id(), stream.str());

  return ERR_OK;
}

int32_t ProcessRequestQueryLivelyAccount(SSMessageEntry& entry) {
  DefaultArrayStream stream;
  stream.Append(
      "select uid, fresh_time, level, name from player_0 where level >= 50 "
      "order by fresh_time desc limit 200");

  const boost::shared_ptr<ResultSet>& result =
    name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());

  MessageSSResponseQueryLivelyAccount response;
  while(result && result->IsValid()) {
    response.add_players(result->at(0).to_int64());
    result->Next();
  }
  if (response.players_size() < 200) {
    for (int32_t index = 1; index < 8; ++index) {
      stream.clear();
      stream.Append(
          "select uid, fresh_time, level, name from player_%d where level >= 50 "
          " order by fresh_time desc limit 30", index);
      const boost::shared_ptr<ResultSet>& result =
          name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
      while (result && result->IsValid()) {
        response.add_players(result->at(0).to_int64());
        result->Next();
      }
    }
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            MSG_SS_RESPONSE_QUERY_LIVELY_ACCOUNT, &response);
  return ERR_OK;
}

int32_t ProcessRequestServerHeartBeat(SSMessageEntry& entry) {
  server->SendServerMessage(entry.session_ptr.get(), entry.head.msgid, NULL);
  return ERR_OK;
}

int32_t ProcessRequestLoadALlName(SSMessageEntry& entry) {
  MessageSSRequestLoadAllName* message =
      static_cast<MessageSSRequestLoadAllName*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSResponseLoadAllName response;
  DefaultArrayStream stream;
  stream.Append("select name from role_name where server in (");
  for (int32_t i = 0; i < message->server_id_size(); ++i) {
    if (i != 0) stream.Append(",");
    stream.Append("%u", message->server_id(i));
  }
  stream.Append(")");
  const boost::shared_ptr<ResultSet>& result =
      name_thread->mysql_conn().ExecSelect(stream.c_str(), stream.size());
  while (result && result->IsValid()) {
    *response.add_name() = result->at(0).to_str();

    result->Next();
  }

  server->SendServerMessage(entry.session_ptr.get(),
                            intranet::MSG_SS_RESPONSE_LOAD_ALL_NAME, &response);
  return ERR_OK;
}

int32_t ProcessRequestUpdateIpList(SSMessageEntry& entry) {
  MessageSSSetServerIpList* message = static_cast<MessageSSSetServerIpList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DefaultArrayStream stream;
  if (message->has_black_list()) {
    const std::string& escape_str =
        server->mysql_conn().EscapeString(message->black_list());
    stream.Append(
        "insert into ip_list(server_id, black_list) values (%u, '%s') On DUPLICATE KEY "
        "update black_list=values(black_list)", message->server_id(), escape_str.c_str());
  } else if (message->has_white_list()) {
    const std::string& escape_str =
        server->mysql_conn().EscapeString(message->white_list());
    stream.Append(
        "insert into ip_list(server_id, white_list) values (%u, '%s') On DUPLICATE KEY "
        "update white_list=values(white_list)", message->server_id(), escape_str.c_str());
  }
  server->PushSql(message->server_id(), stream.str());
  return ERR_OK;
}
