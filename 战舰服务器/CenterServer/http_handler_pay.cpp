#include "http_handler.h"
#include <boost/unordered_map.hpp>
#include <rapidjson/document.h>
#include <str_util.h>
#include "server.h"
#include <cpp/server_message.pb.h>
#include <sstream>
#include <array_stream.h>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <stdlib.h>
#include <google/protobuf/text_format.h>
#include <md5.h>

using namespace http::server2;
using namespace sy;
using namespace intranet;

const std::string& kGameKey = "3295487fd764c16d5b00d19ae3bc8d49";

//充值错误码:
// 1, 充值超时
// 2, 玩家不存在
// 3, 重复的订单
// 4, 错误的订单
// 5, 内部错误
// 6, 货币溢出

void TraceRecharge(int64_t conn_id,
                   const MessageSSUpdateRechargeDetails& request) {
  static google::protobuf::TextFormat::Printer printer;
  std::string debug_str;
  printer.SetUseUtf8StringEscaping(true);
  printer.SetSingleLineMode(true);
  printer.PrintToString(request, &debug_str);
  TRACE_LOG(logger)("HttpConnection:%ld, Request:{%s}", conn_id, debug_str.c_str());
}

static inline std::string CalcMd5(const std::string& s) {
  //INFO_LOG(logger)("%s", s.c_str());
  MD5 md5(s.c_str(), s.size());
  return md5.str();
}

void FillRequestParam(MessageSSUpdateRechargeDetails& msg, const request& req,
                      std::string& game_id, std::string& order_id,
                      std::string& user_id, std::string& sign) {
  for (std::vector<query_str>::const_iterator iter = req.post_contents.begin();
       iter != req.post_contents.end(); ++iter) {
    if (iter->key == "game_id") {
      game_id = iter->value;
    } else if (iter->key == "order_id") {
      order_id = iter->value;
      msg.set_order_id(order_id);
    } else if (iter->key == "user_id") {
      user_id = iter->value;
      msg.set_user_id(user_id);
    } else if (iter->key == "device_type") {
      msg.set_device_type(atoi(iter->value.c_str()));
    } else if (iter->key == "pay_amount") {
      msg.set_pay_amount(atof(iter->value.c_str()) * 100);
    } else if (iter->key == "currency_code") {
      msg.set_currency_code(iter->value);
    } else if (iter->key == "channel_id") {
      msg.set_channel_id(iter->value.c_str());
    } else if (iter->key == "server_id") {
      msg.set_server_id(atoll(iter->value.c_str()));
    } else if (iter->key == "role_id") {
      msg.set_role_id(atoll(iter->value.c_str()));
    } else if (iter->key == "goods_type") {
      msg.set_goods_type(atoi(iter->value.c_str()));
    } else if (iter->key == "game_coin") {
      msg.set_game_coin(atoi(iter->value.c_str()));
    } else if (iter->key == "goods_id") {
      msg.set_goods_id(iter->value);
    } else if (iter->key == "timestamp") {
      msg.set_timestamp(iter->value);
    } else if (iter->key == "sign") {
      sign = iter->value;
    }
  }
  msg.set_request_time(GetSeconds());
}

int32_t HttpRequestHandler::handle_payment(
    const request& req, reply& rep, const boost::shared_ptr<connection>& conn) {
  MessageSSUpdateRechargeDetails request;
  std::string game_id, order_id, user_id, sign;
  FillRequestParam(request, req, game_id, order_id, user_id, sign);
  request.set_session_id(conn->id());
  server->SetPlayTime(conn->id());
  TraceRecharge(conn->id(), request);

  //订单的MD5校验
  //game_id+order_id+user_id+game_key
  DefaultArrayStream stream;
  stream.Append(game_id);
  stream.Append(order_id);
  stream.Append(user_id);
  stream.Append(kGameKey);
  const std::string& sign1 = CalcMd5(stream.str());
  if (sign1 != sign) {
    rep.content = "{\n\"code\":\"4\",\n\"message\":\"VerifyFail\"\n}";
    ERROR_LOG(logger)("Recharge Sign:%s, Sign1:%s", sign.c_str(), sign1.c_str());
    return 0;
  }

  const boost::shared_ptr<MySqlConnection>& auth_conn =
      MySqlConnManager::Instance().GetConnection(CONN_TYPE_AUTH);
  const std::string& openid = auth_conn->EscapeString(user_id.c_str());
  const std::string& goods_id = auth_conn->EscapeString(request.goods_id());
  const std::string& timestamp = auth_conn->EscapeString(request.timestamp());

  uint32_t hash = GetHashValue(user_id.c_str(), user_id.length());
  int32_t table_index = hash % 16;
  stream.clear();
  stream.Append("select server from account_%d where openid='%s' and uid=%ld",
                table_index, openid.c_str(), request.role_id());
  const boost::shared_ptr<ResultSet>& result_set =
      auth_conn->ExecSelect(stream.c_str(), stream.size());
  //玩家账号不存在
  if (!(result_set && result_set->IsValid())) {
    rep.content = "{\n\"code\":\"2\",\n\"message\":\"PlayerNotFound\"\n}";
    ERROR_LOG(logger)("Recharge Fail, PlayerNotFound, OpenID:%s, PlayerID:%ld"
        , request.user_id().c_str(), request.role_id());
    return 0;
  }
  uint32_t server_id = result_set->at(0).to_uint32();
  TRACE_LOG(logger)("PlayerID:%ld, OpenID:%s, Server:%u"
      , request.role_id(), request.user_id().c_str(), server_id);
  request.set_server_id(server_id);

  const boost::shared_ptr<MySqlConnection>& game_conn =
      Server::GetGameConn(server_id, true);
  stream.clear();

  //重复的订单
  stream.Append(
      "select * from recharge_details where order_id='%s' and stage=3",
      request.order_id().c_str());
  const boost::shared_ptr<ResultSet>& result = game_conn->ExecSelect(stream.c_str(), stream.size());
  if (result && result->IsValid()) {
    rep.content = "{\n\"code\":\"3\",\n\"message\":\"OrderIdConflict\"\n}";
    ERROR_LOG(logger)("OrderID:%s Conflict", request.order_id().c_str());
    return 0;
  }

  stream.clear();
  request.set_stage(1);
  stream.Append(
      "insert into recharge_details(order_id, device_type, user_id, "
      "pay_amount, currency_code, channel_id, server_id, role_id, goods_type, "
      "game_coin, goods_id, timestamp, stage, recharge_time) values ('%s', %d, '%s', %d, "
      "'%s', '%s', %u, %ld, %d, %d, '%s', '%s', %d, %d)",
      request.order_id().c_str(), request.device_type(),
      request.user_id().c_str(), request.pay_amount(),
      request.currency_code().c_str(), request.channel_id().c_str(),
      request.server_id(), request.role_id(), request.goods_type(),
      request.game_coin(), goods_id.c_str(), timestamp.c_str(),
      request.stage(), request.request_time());
  game_conn->ExecSql(stream.c_str(), stream.size());

  if (!server->SendServerMessage(server_id, MSG_SS_UPDATE_RECHARGE_DETAILS,
                                 &request)) {
    rep.content = "{\n\"code\":\"5\",\n\"message\":\"InternalError\"\n}";
    ERROR_LOG(logger)("Server:%u not found", server_id);
    return 0;
  }

  ConnectionManager::Instance().AddConnection(conn);
  return 1;
}

