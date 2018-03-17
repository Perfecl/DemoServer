#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <system.h>
#include <array_stream.h>
#include "server.h"
#include "http_handler.h"
#include "coral_sea.h"
#include "legion_war.h"

using namespace sy;
using namespace intranet;

int32_t ProcessServerLogin(SSMessageEntry& entry) {
  MessageSSServerLogin* message = static_cast<MessageSSServerLogin*>(entry.get());
  if (!message) return ERR_INTERNAL;

  uint32_t server_id = message->server_ids(0);
  int32_t server_count = message->server_ids_size();
  for (int i = 0; i < server_count; ++i) {
    server->AddServer(message->server_ids(i), entry.session_ptr);
    server->SetServer2Server(message->server_ids(i), server_id);
  }
  TRACE_LOG(logger)("New Server Connected:%s, ServerID:%u",
                    message->server_name().c_str(), server_id);

  return ERR_OK;
}

int32_t ProcessPlayerNum(SSMessageEntry& entry) {
  MessageSSNotifyPlayerNum* message =
      static_cast<MessageSSNotifyPlayerNum*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ArrayStream<128 * 1024> stream;
  stream.Append(
      "insert into gm_online_info(server_id, cur_time, "
      "online_num) values (%u, %ld, %d)",
      message->server_id(), GetSeconds(), message->player_num());
  server->PushSql(message->server_id(), CONN_TYPE_GM, stream.str());

  stream.clear();
  stream.Append(
      "update `server` set server_version='%s', last_update_time=%ld where "
      "server_id in (%u",
      message->version().c_str(), message->update_time(), message->server_id());
  for (int32_t i = 0; i < message->servers_size(); ++i) {
    stream.Append(",%u", message->servers(i));
  }
  stream.Append(")");
  server->PushSql(message->server_id(), CONN_TYPE_GM, stream.str());

  if (message->has_open_days())
    server->SetServerOpenDays(message->server_id(), message->open_days());
  return ERR_OK;
}

int32_t ProcessUpdateRecharge(SSMessageEntry& entry) {
  MessageSSUpdateRechargeDetails* message =
      static_cast<MessageSSUpdateRechargeDetails*>(entry.get());
  if (!message) return ERR_INTERNAL;

  http::server2::connection_ptr conn =
      ConnectionManager::Instance().FetchConnection(message->session_id());
  if (conn) {
    DefaultArrayStream stream;
    if (!message->result()) {
      stream.Append("{\n\"code\":\"0\",\n\"message\":\"success\",\n\"cp_order_id\":\"%s\"\n}"
          , message->order_id().c_str());
      conn->async_send(stream.str());
      TRACE_LOG(logger)("Recharge Success, PlayerID:%ld, Money:%d, OrderID:%s"
          , message->role_id(), message->game_coin(), message->order_id().c_str());
      TRACE_LOG(logger)("%s", stream.c_str());
    } else {
      if (message->result() == ERR_PLAYER_NOT_EXIST) {
        stream.Append("{\n\"code\":\"2\",\n\"message\":\"PlayerNotFound\"\n}");
      } else {
        stream.Append("{\n\"code\":\"6\",\n\"message\":\"MoneyOverflow\"\n}");
      }
      conn->async_send(stream.str());
      ERROR_LOG(logger)("Recharge Fail:%d, PlayerID:%ld, Money:%d, OrderID:%s"
          , message->result()
          , message->role_id(), message->game_coin(), message->order_id().c_str());
    }
  } else {
    WARN_LOG(logger)("HttpConnectionPtr:%d not found", message->session_id());
  }
  server->ClearPay(message->session_id());
  return ERR_OK;
}

int32_t ProcessRequestRegisterLegionPlayer(SSMessageEntry& entry) {
  MessageSSRequestLegionWarNewPlayer* message =
      static_cast<MessageSSRequestLegionWarNewPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int32_t region = server->GetLegionRegionByServerID(message->server_id());
  DEBUG_LOG(logger)("LegionWar Register New Player:%ld, ServerID:%u, Region:%d"
      , message->player().player_id()
      , message->server_id()
      , region);
  const LegionWarPtr& war = LegionWar::GetLegionWar(region);
  std::pair<int32_t, int32_t> result =
      war->Register(message->player().player_id());

  server->SaveOtherPlayer(message->player());

  const std::string& value = war->Dump();
  DEBUG_LOG(logger)("LegionWar Region:%d, PostionInfo\n%s", region, value.c_str());

  MessageSSResponseLegionNewPlayer response;
  response.mutable_player()->CopyFrom(message->player());
  response.set_city_id(result.first);
  response.set_position(result.second);

  server->SendMessageToLegionWarRegion(
      message->server_id(), MSG_SS_RESPONSE_LEGION_WAR_NEW_PLAYER, &response);
  return ERR_OK;
}

int32_t ProcessRequestLegionWarSwapPlayer(SSMessageEntry& entry) {
  MessageSSUpdateLegionWarPos* message = static_cast<MessageSSUpdateLegionWarPos*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DEBUG_LOG(logger)("ServerID:%u, Pos1:(%d,%d), Pos2:(%d,%d)"
      , message->server_id()
      , message->city_1(), message->position_1()
      , message->city_2(), message->position_2());
  int32_t region = server->GetLegionRegionByServerID(message->server_id());
  const LegionWarPtr& war = LegionWar::GetLegionWar(region);
  if (war) {
    war->SwapPosition(std::make_pair(message->city_1(), message->position_1()),
                      std::make_pair(message->city_2(), message->position_2()));

    const std::string& value = war->Dump();
    DEBUG_LOG(logger)("LegionWar Region:%d, PostionInfo\n%s", region, value.c_str());
  }

  server->SendMessageToLegionWarRegion(message->server_id(),
                                       MSG_SS_UPDATE_LEGION_WAR_POS, message);
  return ERR_OK;
}

int32_t ProcessRequestUpdateCrossServerRankList(SSMessageEntry& entry) {
  MessageSSUpdateCrossServerRankList* message =
      static_cast<MessageSSUpdateCrossServerRankList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  server->SendMessageToLegionWarRegion(
      message->server_id(), MSG_SS_UPDATE_CROSS_SERVER_RANK_LIST, message);
  return ERR_OK;
}

int32_t ProcessPlayerLogin(SSMessageEntry& entry) {
  MessageSSRequestPlayerLogin* message =
      static_cast<MessageSSRequestPlayerLogin*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const boost::shared_ptr<MySqlConnection>& mysql_conn =
      Server::GetGameConn(message->server_id(), true);
  if (!mysql_conn) {
    ERROR_LOG(logger)("ProcessPlayerLogin, ServerID:%u, PlayerID:%ld"
        , message->server_id(), message->player_id());
    return ERR_OK;
  }

  DefaultArrayStream stream;
  stream.Append(
      "select order_id, device_type, user_id, pay_amount, currency_code, "
      "channel_id, server_id, role_id, goods_type, game_coin, `timestamp`, "
      "goods_id "
      " from recharge_details where role_id=%ld group by order_id having "
      "count(order_id) < 2",
      message->player_id());

  const boost::shared_ptr<ResultSet>& result =
      mysql_conn->ExecSelect(stream.c_str(), stream.size());

  while (result && result->IsValid()) {
    const std::string& order_id = result->at(0).to_str();
    int64_t& time = server->order_time(order_id);
    if (time && time + 3600 >= GetSeconds()) {
      TRACE_LOG(logger)("Reissue OrderID:%s, LastTime:%ld, CurrentTime:%ld"
          , order_id.c_str(), time, GetSeconds());
      result->Next();
      continue;
    }
    time = GetSeconds();

    MessageSSUpdateRechargeDetails request;
    request.set_order_id(order_id);
    request.set_device_type(result->at(1).to_int32());
    request.set_user_id(result->at(2).to_str());
    request.set_pay_amount(result->at(3).to_int32());
    request.set_currency_code(result->at(4).to_str());
    request.set_channel_id(result->at(5).to_str());
    request.set_server_id(result->at(6).to_uint32());
    request.set_role_id(result->at(7).to_int64());
    request.set_goods_type(result->at(8).to_int32());
    request.set_game_coin(result->at(9).to_int32());
    request.set_timestamp(result->at(10).to_str());
    request.set_goods_id(result->at(11).to_str());
    request.set_request_time(GetSeconds());

    TRACE_LOG(logger)("Reissue OrderID:%s, PlayerID:%ld, OpenID:%s, GoodsID:%s"
        , order_id.c_str(), request.role_id(), request.user_id().c_str(), request.goods_id().c_str());

    server->SendServerMessage(request.server_id(),
                              MSG_SS_UPDATE_RECHARGE_DETAILS, &request);
    result->Next();
  }

  return ERR_OK;
}

int32_t ProcessRequestCreateCoralSeaTeam(SSMessageEntry& entry) {
  MessageSSRequestCreateCoralSeaTeam* message =
      static_cast<MessageSSRequestCreateCoralSeaTeam*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const CoralSeaTeamPtr& ptr = TEAM_MANAGER.CreateTeam(message->player());
  if (ptr) {
    DEBUG_LOG(logger)("CreateCoralSeaTeam, PlayerID:%ld, ServerID:%u, TeamID:%ld"
        , message->player().player_id(), message->player().server(), ptr->id());
  }
  return ERR_OK;
}

int32_t ProcessRequestLeaveCoralSeaTeam(SSMessageEntry& entry) {
  MessageSSRequestLeaveCoralSeaTeam* message =
      static_cast<MessageSSRequestLeaveCoralSeaTeam*>(entry.get());
  if (!message) return ERR_INTERNAL;
  DEBUG_LOG(logger)("LeaveCoralSeaTeam, PlayerID:%ld", message->player_id());
  TEAM_MANAGER.RemovePlayer(message->player_id());

  return ERR_OK;
}

int32_t ProcessRequestSearchCoralSeaTeam(SSMessageEntry& entry) {
  MessageSSRequestSearchCoralSeaTeam* message =
      static_cast<MessageSSRequestSearchCoralSeaTeam*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DEBUG_LOG(logger)("SearchCoralTeam, PlayerID:%ld, ServerID:%u"
      , message->player().player_id(), message->player().server());
  TEAM_MANAGER.SearchTeam(message->player());
  return ERR_OK;
}

int32_t ProcessRequestJoinCoralSeaTeam(SSMessageEntry& entry) {
  MessageSSRequestJoinCoralSeaTeam* message =
      static_cast<MessageSSRequestJoinCoralSeaTeam*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const CoralSeaTeamPtr& ptr = TEAM_MANAGER.GetTeam(message->team_id());
  if (ptr && ptr->player_count() < 5) {
    ptr->AddPlayer(message->player(), false, true);
    DEBUG_LOG(logger)("JoinCoralSeaTeam, PlayerID:%ld, ServerID:%u, TeamID:%ld"
        , message->player().player_id(), message->player().server(), ptr->id());
  } else {
    DEBUG_LOG(logger)("JoinCoralSeaTeam, PlayerID:%ld, TeamID:%ld"
        , message->player().player_id(), ptr ? ptr->id() : -message->team_id());
  }

  return ERR_OK;
}

int32_t ProcessRequestCoralSeaDebug(SSMessageEntry& entry) {
  TEAM_MANAGER.Dump();
  return ERR_OK;
}
