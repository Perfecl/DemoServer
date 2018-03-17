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
#include <net/http/async_client.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

using namespace http::server2;
using namespace rapidjson;
using namespace sy;
using namespace intranet;

typedef int32_t (*HttpRequestProc)(Document& doc, reply& rep, const connection_ptr& conn);
boost::unordered_map<std::string, HttpRequestProc> kHttpCallback;

#pragma GCC diagnostic ignored "-Wunused-variable"

static std::string DocToString(Document& doc) {
  std::string result;
  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  doc.Accept(writer);
  result = buffer.GetString();
  return result;
}

int32_t EmptyHttpRequestProc(Document& doc, reply& rep, const connection_ptr& conn) {
  rep.status = reply::not_found;
  rep.content = "<h1>404</h1>";
  return 0;
}

//函数签名是int32_t(Document& doc, reply& rep, const connection_ptr& conn)
//返回0表示立即发送response
//否则就需要自己手动给conn发送response
#define REG_CALLBACK(name)                                                    \
  static int32_t name(Document& doc, reply& rep, const connection_ptr& conn); \
  static int32_t reg_##name() { kHttpCallback[#name] = name; return 1; }      \
  static int32_t __##name = reg_##name();                                     \
  static int32_t name(Document& doc, reply& rep, const connection_ptr& conn)

int64_t GetPlayerIDByOpenIDAndServer(const std::string& openid,
                                     uint32_t server) {
  const boost::shared_ptr<MySqlConnection>& mysql_conn =
      MySqlConnManager::Instance().GetConnection(CONN_TYPE_AUTH);

  uint32_t hash_value = GetHashValue(openid.c_str(), openid.length());
  std::string op = mysql_conn->EscapeString(openid.c_str(), openid.length());
  DefaultArrayStream stream;
  stream.Append("select uid from account_%d where openid='%s' and server=%u",
                hash_value % 16, op.c_str(), server);

  int64_t uid = 0;
  const boost::shared_ptr<ResultSet>& result =
      mysql_conn->ExecSelect(stream.c_str(), stream.size());
  if (result && result->IsValid()) {
    uid = result->at(0).to_int64();
  }
  return uid;
}

std::string SimpleResultString(int32_t s,
                               std::vector<std::string>* vct = NULL) {
  Document output;
  output.SetObject();
  Document::AllocatorType& alloc = output.GetAllocator();
  Value status;
  status.SetInt(s);
  Value data(rapidjson::kObjectType);
  if (vct && !vct->empty()) {
    Value list(rapidjson::kArrayType);
    for (std::vector<std::string>::iterator it = vct->begin(); it != vct->end();
         ++it) {
      Value s;
      s.SetString(it->c_str(), it->length());
      list.PushBack(s, alloc);
    }
    data.AddMember("list", list, alloc);
  }
  output.AddMember("status", status, output.GetAllocator());
  output.AddMember("data", data, output.GetAllocator());

  return DocToString(output);
}

template <size_t N1, size_t N2>
static inline void PushBackObject(Value& array, Document::AllocatorType& alloc,
                                  const char(&name1)[N1],
                                  const std::string& value1,
                                  const char(&name2)[N2],
                                  const std::string& value2) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3>
static inline void PushBackObject(Value& array, Document::AllocatorType& alloc,
                                  const char(&name1)[N1],
                                  const std::string& value1,
                                  const char(&name2)[N2],
                                  const std::string& value2,
                                  const char(&name3)[N3],
                                  const std::string& value3) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3, size_t N4>
static inline void PushBackObject(
    Value& array, Document::AllocatorType& alloc, const char(&name1)[N1],
    const std::string& value1, const char(&name2)[N2],
    const std::string& value2, const char(&name3)[N3],
    const std::string& value3, const char(&name4)[N4],
    const std::string& value4) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  obj.AddMember(name4, value4, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5>
static inline void PushBackObject(
    Value& array, Document::AllocatorType& alloc, const char(&name1)[N1],
    const std::string& value1, const char(&name2)[N2],
    const std::string& value2, const char(&name3)[N3],
    const std::string& value3, const char(&name4)[N4],
    const std::string& value4, const char(&name5)[N5],
    const std::string& value5) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  obj.AddMember(name4, value4, alloc);
  obj.AddMember(name5, value5, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5, size_t N6>
static inline void PushBackObject(
    Value& array, Document::AllocatorType& alloc, const char(&name1)[N1],
    const std::string& value1, const char(&name2)[N2],
    const std::string& value2, const char(&name3)[N3],
    const std::string& value3, const char(&name4)[N4],
    const std::string& value4, const char(&name5)[N5],
    const std::string& value5, const char(&name6)[N6],
    const std::string& value6) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  obj.AddMember(name4, value4, alloc);
  obj.AddMember(name5, value5, alloc);
  obj.AddMember(name6, value6, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5, size_t N6,
          size_t N7>
static inline void PushBackObject(
    Value& array, Document::AllocatorType& alloc, const char(&name1)[N1],
    const std::string& value1, const char(&name2)[N2],
    const std::string& value2, const char(&name3)[N3],
    const std::string& value3, const char(&name4)[N4],
    const std::string& value4, const char(&name5)[N5],
    const std::string& value5, const char(&name6)[N6],
    const std::string& value6, const char(&name7)[N7],
    const std::string& value7) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  obj.AddMember(name4, value4, alloc);
  obj.AddMember(name5, value5, alloc);
  obj.AddMember(name6, value6, alloc);
  obj.AddMember(name7, value7, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5, size_t N6,
          size_t N7, size_t N8>
static inline void PushBackObject(
    Value& array, Document::AllocatorType& alloc, const char(&name1)[N1],
    const std::string& value1, const char(&name2)[N2],
    const std::string& value2, const char(&name3)[N3],
    const std::string& value3, const char(&name4)[N4],
    const std::string& value4, const char(&name5)[N5],
    const std::string& value5, const char(&name6)[N6],
    const std::string& value6, const char(&name7)[N7],
    const std::string& value7, const char(&name8)[N8],
    const std::string& value8) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  obj.AddMember(name4, value4, alloc);
  obj.AddMember(name5, value5, alloc);
  obj.AddMember(name6, value6, alloc);
  obj.AddMember(name7, value7, alloc);
  obj.AddMember(name8, value8, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5, size_t N6,
          size_t N7, size_t N8, size_t N9>
static inline void PushBackObject(
    Value& array, Document::AllocatorType& alloc, const char(&name1)[N1],
    const std::string& value1, const char(&name2)[N2],
    const std::string& value2, const char(&name3)[N3],
    const std::string& value3, const char(&name4)[N4],
    const std::string& value4, const char(&name5)[N5],
    const std::string& value5, const char(&name6)[N6],
    const std::string& value6, const char(&name7)[N7],
    const std::string& value7, const char(&name8)[N8],
    const std::string& value8, const char(&name9)[N9],
    const std::string& value9) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  obj.AddMember(name4, value4, alloc);
  obj.AddMember(name5, value5, alloc);
  obj.AddMember(name6, value6, alloc);
  obj.AddMember(name7, value7, alloc);
  obj.AddMember(name8, value8, alloc);
  obj.AddMember(name9, value9, alloc);
  array.PushBack(obj, alloc);
}

template <size_t N1, size_t N2, size_t N3, size_t N4, size_t N5, size_t N6,
          size_t N7, size_t N8, size_t N9, size_t N10, size_t N11>
static inline void PushBackObject(
    Value& array, Document::AllocatorType& alloc, const char(&name1)[N1],
    const std::string& value1, const char(&name2)[N2],
    const std::string& value2, const char(&name3)[N3],
    const std::string& value3, const char(&name4)[N4],
    const std::string& value4, const char(&name5)[N5],
    const std::string& value5, const char(&name6)[N6],
    const std::string& value6, const char(&name7)[N7],
    const std::string& value7, const char(&name8)[N8],
    const std::string& value8, const char(&name9)[N9],
    const std::string& value9, const char(&name10)[N10],
    const std::string& value10, const char(&name11)[N11],
    const std::string& value11) {
  Value obj;
  obj.SetObject();
  obj.AddMember(name1, value1, alloc);
  obj.AddMember(name2, value2, alloc);
  obj.AddMember(name3, value3, alloc);
  obj.AddMember(name4, value4, alloc);
  obj.AddMember(name5, value5, alloc);
  obj.AddMember(name6, value6, alloc);
  obj.AddMember(name7, value7, alloc);
  obj.AddMember(name8, value8, alloc);
  obj.AddMember(name9, value9, alloc);
  obj.AddMember(name10, value10, alloc);
  obj.AddMember(name11, value11, alloc);
  array.PushBack(obj, alloc);
}

template <typename T>
T JsonGetInt(const Document& doc, const char* key) {
  if (!doc.HasMember(key)) {
    TRACE_LOG(logger)("key: %s not found", key);
    return 0;
  }

  const Value& v = doc[key];
  if (v.IsInt())
    return v.GetInt();
  else if (v.IsInt64())
    return v.GetInt64();
  else if (v.IsString())
    return atoll(v.GetString());
  else
    return 0;
}

std::string JsonGetString(const Document& doc, const char* key) {
  if (!doc.HasMember(key)) {
    TRACE_LOG(logger)("key: %s not found", key);
    return "";
  }

  const Value& v = doc[key];

  if (v.IsInt())
    return boost::lexical_cast<std::string>(v.GetInt());
  else if (v.IsInt64())
    return boost::lexical_cast<std::string>(v.GetInt64());
  else if (v.IsString())
    return v.GetString();
  else
    return "";
}

template <typename T>
std::vector<T> JsonGetIntArray(const Document& doc, const char* key) {
  std::vector<T> vct;
  if (!doc.HasMember(key)) {
    ERROR_LOG(logger)("key: %s not found", key);
    return vct;
  }
  const Value& v = doc[key];
  if (!v.IsArray()) return vct;
  for (SizeType i = 0; i < v.Size(); ++i) {
    if (v[i].IsInt())
      vct.push_back(v[i].GetInt());
    else if (v[i].IsInt64())
      vct.push_back(v[i].GetInt64());
    else if (v[i].IsString())
      vct.push_back(atoll(v[i].GetString()));
  }
  return vct;
}

std::vector<std::string> JsonGetStringArray(const Document& doc,
                                             const char* key) {
  std::vector<std::string> vct;
  if (!doc.HasMember(key)) {
    ERROR_LOG(logger)("key: %s not found", key);
    return vct;
  }
  const Value& v = doc[key];
  if (!v.IsArray()) return vct;
  for (SizeType i = 0; i < v.Size(); ++i) {
    if (v[i].IsInt())
      vct.push_back(boost::lexical_cast<std::string>(v[i].GetInt()));
    else if (v[i].IsInt64())
      vct.push_back(boost::lexical_cast<std::string>(v[i].GetInt64()));
    else if (v[i].IsString())
      vct.push_back(v[i].GetString());
  }
  return vct;
}

void GetValuePair(const std::string& str, const std::string& s1,
                  const std::string& s2,
                  std::vector<std::pair<int32_t, int32_t> >& vct) {
  std::vector<std::string> vct1;
  SplitString(str, vct1, s1);

  for (size_t i = 0; i < vct1.size(); i++) {
    std::vector<std::string> vct2;
    SplitString(vct1[i], vct2, s2);
    if (vct2.size() >= 2) {
      vct.push_back(
          std::make_pair(atoi(vct2[0].c_str()), atoi(vct2[1].c_str())));
    }
  }
}

REG_CALLBACK(getUser) {
  int32_t page = JsonGetInt<int32_t>(doc, "page");
  int32_t page_size = JsonGetInt<int32_t>(doc, "size");
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  int64_t player_id = JsonGetInt<int64_t>(doc, "player_id");
  const std::string& user_account = JsonGetString(doc, "user_account");
  const std::string& name = JsonGetString(doc, "name");
  std::vector<int> cash = JsonGetIntArray<int32_t>(doc, "cash");
  std::vector<int> vip = JsonGetIntArray<int32_t>(doc, "vip");
  std::vector<int> level = JsonGetIntArray<int32_t>(doc, "level");
  std::vector<std::string> created_at = JsonGetStringArray(doc, "create_at");

  const boost::shared_ptr<MySqlConnection>& mysql_conn =
      Server::GetGameConn(server_id, true);

  std::ostringstream condition;
  condition << " server = '" << server_id << "' ";
  if (player_id) condition << " and (uid = " << player_id << ") ";
  if (!user_account.empty())
    condition << " and (openid = '"
              << mysql_conn->EscapeString(user_account.c_str(),
                                          user_account.length())
              << "') ";
  if (!name.empty())
    condition << " and (name = '"
              << mysql_conn->EscapeString(name.c_str(), name.length()) << "') ";
  if (vip.size() > 1 && vip[1])
    condition << " and (vip_level between " << vip[0] << " and " << vip[1]
              << ") ";
  if (level.size() > 1 && level[1])
    condition << " and (level between " << level[0] << " and " << level[1]
              << ") ";
  if (created_at.size() > 1 && !created_at[1].empty())
    condition << " and (create_time between unix_timestamp('"
              << mysql_conn->EscapeString(created_at[0].c_str(),
                                          created_at[0].length())
              << "') and unix_timestamp('"
              << mysql_conn->EscapeString(created_at[1].c_str(),
                                          created_at[1].length())
              << "')) ";
  if (cash.size() > 1 && cash[1])
    condition << "and (total_recharge between " << cash[0] * 10 << " and "
              << cash[1] * 10 << ") ";

  int32_t status = 1;
  int32_t all_page = 0;
  Document output;
  output.SetObject();
  Document::AllocatorType& alloc = output.GetAllocator();
  Value data(rapidjson::kObjectType);
  Value list(rapidjson::kArrayType);

  int32_t table_count = server->GetTableCount(server_id);
  std::vector<int32_t> per_count;

  std::ostringstream sql;
  std::string str_sql;
  for (int32_t i = 0; i < table_count; i++) {
    sql.clear();
    sql.str("");
    sql << "select count(*) from zhanjian.player_" << i << " where "
        << condition.str();
    str_sql = sql.str();

    const boost::shared_ptr<ResultSet>& result_count =
        mysql_conn->ExecSelect(str_sql.c_str(), str_sql.size());
    if (result_count->error) {
      ERROR_LOG(logger)("ExecSql fail:%d, %s", result_count->error, str_sql.c_str());
      status = 0;
      break;
    }

    int32_t player_count = 0;
    if (result_count && result_count->IsValid())
      player_count = result_count->at(0).to_int32();
    per_count.push_back(player_count);
  }

  int32_t all_num = 0;
  for (size_t i = 0; i < per_count.size(); i++) all_num += per_count[i];
  data.AddMember("all_total", all_num, alloc);

  all_page = all_num / page_size;
  if (all_num % page_size) all_page++;

  int32_t index = (page - 1) * page_size;
  if (index < 0) index = 0;

  for (size_t i = 0; i < per_count.size(); i++) {
    if (page_size <= 0) break;
    if (index >= per_count[i]) {
      index -= per_count[i];
      continue;
    }

    int32_t count =
        page_size < (per_count[i] - index) ? page_size : (per_count[i] - index);

    sql.clear();
    sql.str("");
    sql << "select uid, openid, name, level, vip_level, "
           "from_unixtime(create_time), flag, status, "
           "from_unixtime(last_login_time), money,(status_time <= unix_timestamp(now())), "
           "total_recharge from "
           "player_"
        << i << " where " << condition.str() << " limit " << index << ","
        << count;

    page_size -= count;
    index = 0;

    str_sql = sql.str();
    const boost::shared_ptr<ResultSet>& result_players =
        mysql_conn->ExecSelect(str_sql.c_str(), str_sql.size());
    if (result_players->error) {
      ERROR_LOG(logger)("ExecSql fail:%d, %s", result_players->error, str_sql.c_str());
      status = 0;
      break;
    }

    while (result_players && result_players->IsValid()) {
      Value obj;
      obj.SetObject();
      obj.AddMember("player_id", result_players->at(0).to_str(), alloc);
      obj.AddMember("user_account", result_players->at(1).to_str(), alloc);
      obj.AddMember("name", result_players->at(2).to_str(), alloc);
      obj.AddMember("level", result_players->at(3).to_str(), alloc);
      obj.AddMember("vip", result_players->at(4).to_str(), alloc);
      obj.AddMember("create", result_players->at(5).to_str(), alloc);
      int32_t gm_v = result_players->at(6).to_int32();
      if (gm_v < 1) gm_v = 1;
      obj.AddMember("gm", gm_v, alloc);
      int32_t status_v = result_players->at(7).to_int32();
      if (result_players->at(10).to_int32()) status_v = 0;
      obj.AddMember("ban_status", status_v, alloc);
      obj.AddMember("logout_time", result_players->at(8).to_str(), alloc);
      obj.AddMember("yuan_bao", result_players->at(9).to_str(), alloc);

      const std::string& cash_s = result_players->at(11).to_str();
      obj.AddMember("cash", cash_s.size() >= 2u ? cash_s.substr(0, cash_s.size() - 1) : cash_s, alloc);
      list.PushBack(obj, alloc);
      result_players->Next();
    }
  }

  data.AddMember("list", list, alloc);

  output.AddMember("status", status, alloc);
  output.AddMember("all_page", all_page, alloc);

  Value fields(rapidjson::kObjectType);
  fields.AddMember("player_id", "玩家ID", alloc);
  fields.AddMember("name", "昵称", alloc);
  fields.AddMember("user_account", "玩家OpenID", alloc);
  fields.AddMember("level", "等级", alloc);
  fields.AddMember("vip", "VIP等级", alloc);
  fields.AddMember("cash", "冲值(分)", alloc);
  fields.AddMember("gm", "玩家类型", alloc);
  fields.AddMember("create", "创建时间", alloc);
  fields.AddMember("ban_status", "封禁状态", alloc);
  fields.AddMember("yuan_bao", "黄金", alloc);
  fields.AddMember("logout_time", "最后登陆", alloc);
  data.AddMember("fields", fields, alloc);

  output.AddMember("data", data, alloc);

  rep.content = DocToString(output);

  std::string content = rep.content;
  content.resize(content.size() + 1, 0);
  logger->LogMessage(const_cast<char*>(content.c_str()), content.size() - 1);
  return 0;
}

int32_t UpdateGoods(Document& doc, reply& rep, const connection_ptr& conn,
                    bool is_add) {
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  const std::string& goods_list = JsonGetString(doc, "goods_list");
  const std::string& item_list = JsonGetString(doc, "item_list");
  const std::string& type = JsonGetString(doc, "type");

  const std::string& user_list = JsonGetString(doc, "user_list");
  std::vector<std::string> user_list_vct;
  SplitString(user_list, user_list_vct, ",");

  std::vector<std::pair<int32_t, int32_t> > items;
  std::vector<std::pair<int32_t, int32_t> > modify;
  GetValuePair(goods_list, ";", ":", modify);
  GetValuePair(item_list, ";", ":", items);

  const boost::shared_ptr<MySqlConnection>& mysql_conn =
      Server::GetGameConn(server_id, true);

  std::vector<std::string> err_str;
  if (type == "server_id" || user_list_vct.empty()) {
    MessageSSAddServerMail msg;
    sy::MailInfo* mail_info = msg.mutable_mail();
    mail_info->set_mail_content("系统邮件");
    mail_info->set_mail_time(GetSeconds());
    mail_info->set_mail_id(0);
    mail_info->set_mail_type(MAIL_TYPE_SYS);
    for (size_t i = 0; i < items.size(); i++) {
      KVPair2* pair = mail_info->add_mail_attachment();
      pair->set_key(items[i].first);
      pair->set_value(items[i].second);
    }
    for (size_t i = 0; i < modify.size(); i++) {
      KVPair2* pair = mail_info->add_mail_attachment();
      pair->set_key(modify[i].first);
      pair->set_value(modify[i].second);
    }
    msg.set_server_id(server_id);
    bool success =
        server->SendServerMessage(server_id, MSG_SS_ADD_SERVER_MAIL, &msg);
    rep.content = SimpleResultString(success);
    return 0;
  } else {
    std::ostringstream sql;
    sql << "select uid, openid, name from role_name where server='" << server_id
        << "' ";
    if (!user_list_vct.empty()) {
      bool flag = true;
      if (type == "player_id")
        sql << " and uid in ";
      else if (type == "user_account")
        sql << " and openid in ";
      else if (type == "name")
        sql << " and name in ";
      else if (type == "server_id")
        sql << " and server in ";
      else
        flag = false;
      if (flag) {
        sql << "(";
        for (size_t i = 0; i < user_list_vct.size(); ++i) {
          if (i) sql << ",";
          sql << "'"
              << mysql_conn->EscapeString(user_list_vct[i].c_str(),
                                          user_list_vct[i].length())
              << "'";
        }
        sql << ") ";
      }
    }
    VectorSet<std::string> check_param;
    MessageSSRequestAddGood msg;
    GMContent* content = msg.mutable_content();
    int sign = is_add ? 1 : -1;
    for (std::vector<std::pair<int32_t, int32_t> >::iterator it =
             modify.begin();
         it != modify.end(); ++it) {
      sy::KVPair2* good = content->add_modify();
      good->set_key(it->first);
      good->set_value(it->second * sign);
    }
    for (std::vector<std::pair<int32_t, int32_t> >::iterator it = items.begin();
         it != items.end(); ++it) {
      sy::KVPair2* item = content->add_add_sub_item();
      item->set_key(it->first);
      item->set_value(it->second * sign);
    }
    const std::string sql_string = sql.str().c_str();
    const boost::shared_ptr<ResultSet>& result =
        mysql_conn->ExecSelect(sql_string.c_str(), sql_string.size());
    if (result->error) {
      rep.content = SimpleResultString(0);
      ERROR_LOG(logger)("ExecSql fail:%d, %s", result->error, sql_string.c_str());
      return 0;
    }
    while (result && result->IsValid()) {
      msg.add_player_uid(result->at(0).to_int64());
      if (type == "player_id")
        check_param.insert(result->at(0).to_c_str());
      else if (type == "user_account")
        check_param.insert(result->at(1).to_c_str());
      else if (type == "name")
        check_param.insert(result->at(2).to_c_str());
      result->Next();
    }
    for (size_t i = 0; i < user_list_vct.size(); i++) {
      if (check_param.find(user_list_vct[i].c_str()) == check_param.end()) {
        err_str.push_back(user_list_vct[i].c_str());
        ERROR_LOG(logger)("addgoods not found player by param(uid,openid,name):%s", user_list_vct[i].c_str());
      }
    }
    msg.set_conn_id(conn->id());
    bool success =
        server->SendServerMessage(server_id, MSG_SS_REQUEST_ADD_GOODS, &msg);
    if (!success) {
      rep.content = SimpleResultString(success, &err_str);
      return 0;
    }
  }

  return 1;
}
REG_CALLBACK(addGoods) { return UpdateGoods(doc, rep, conn, true); }
REG_CALLBACK(delGoods) { return UpdateGoods(doc, rep, conn, false); }

REG_CALLBACK(editUser) {
  //{"level":2222,"name":"11111","player_id":"10034","server_id":"2206420002","vip_level":3333}
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  int64_t player_id = JsonGetInt<int64_t>(doc, "player_id");
  const std::string& info = JsonGetString(doc, "player");
  int32_t level = JsonGetInt<int32_t>(doc, "level");
  int32_t vip_level = JsonGetInt<int32_t>(doc, "vip_level");
  const std::string& player_name = JsonGetString(doc, "name");

  MessageSSRequestAddGood msg;
  GMContent* content = msg.mutable_content();
  msg.add_player_uid(player_id);
  msg.set_conn_id(conn->id());

  if (level > 0) content->set_level(level);
  if (vip_level >= 0) content->set_vip_level(vip_level);
  if (player_name.length() > 0) content->set_player_name(player_name);

  bool success =
      server->SendServerMessage(server_id, MSG_SS_REQUEST_ADD_GOODS, &msg);
  if (!success) rep.content = SimpleResultString(success);

  return 1;
}

REG_CALLBACK(freshGlobalConfig) {
  const std::string& config = JsonGetString(doc, "config");
  MessageSSNotifyGlobalConfig msg;
  msg.set_global_str(config);
  server->SendMessageToAllServer(MSG_SS_NOTIFY_GLOBAL_CONFIG, &msg);
  return 0;
}

REG_CALLBACK(sendMail) {
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  const std::string& goods_list = JsonGetString(doc, "goods_list");
  const std::string& item_list = JsonGetString(doc, "item_list");
  int32_t level_start = JsonGetInt<int32_t>(doc, "level_start");
  int32_t level_end = JsonGetInt<int32_t>(doc, "level_end");
  int32_t vip_level_start = JsonGetInt<int32_t>(doc, "vip_level_start");
  int32_t vip_level_end = JsonGetInt<int32_t>(doc, "vip_level_end");
  int32_t mail_type = JsonGetInt<int32_t>(doc, "mail_type");
  if (!mail_type) mail_type = MAIL_TYPE_SYS;

  const std::string& title = JsonGetString(doc, "title");
  const std::string& content = JsonGetString(doc, "content");

  const std::string& type = JsonGetString(doc, "type");
  const std::string& user_list = JsonGetString(doc, "user_list");

  std::vector<std::string> user_list_vct;
  SplitString(user_list, user_list_vct, ",");

  std::vector<std::pair<int32_t, int32_t> > items;
  GetValuePair(goods_list, ";", ":", items);
  GetValuePair(item_list, ";", ":", items);

  sy::MailInfo mail_info;
  mail_info.set_mail_content(content);
  mail_info.set_mail_time(GetSeconds());
  mail_info.set_mail_id(0);
  mail_info.set_mail_type(mail_type);
  for (size_t i = 0; i < items.size(); i++) {
    KVPair2* pair = mail_info.add_mail_attachment();
    pair->set_key(items[i].first);
    pair->set_value(items[i].second);
  }

  bool success = false;
  std::vector<std::string> err_str;

  if (0 == level_end && 0 == vip_level_end && user_list_vct.empty()) {
    MessageSSAddServerMail msg;
    msg.mutable_mail()->CopyFrom(mail_info);
    msg.set_server_id(server_id);
    success =
        server->SendServerMessage(server_id, MSG_SS_ADD_SERVER_MAIL, &msg);
  } else {
    const boost::shared_ptr<MySqlConnection>& mysql_conn =
        Server::GetGameConn(server_id, true);
    MessageSSSendMailToMulti msg;
    msg.mutable_mail()->CopyFrom(mail_info);

    VectorSet<std::string> check_param;
    for (int32_t i = 0; i < server->GetTableCount(server_id); i++) {
      std::ostringstream sql;
      sql << "select uid, openid, name from player_" << i << " where server = " << server_id
          << " ";
      if (level_end)
        sql << " and (level between " << level_start << " and " << level_end
            << ") ";
      if (vip_level_end)
        sql << "and (vip between " << vip_level_start << " and "
            << vip_level_end << ") ";
      if (!user_list_vct.empty()) {
        bool flag = true;
        if (type == "player_id")
          sql << " and uid in ";
        else if (type == "user_account")
          sql << " and openid in ";
        else if (type == "name")
          sql << " and name in ";
        else if (type == "server_id")
          sql << " and server in ";
        else
          flag = false;
        if (flag) {
          sql << "(";
          for (size_t i = 0; i < user_list_vct.size(); ++i) {
            if (i) sql << ",";
            sql << "'"
                << mysql_conn->EscapeString(user_list_vct[i].c_str(),
                                            user_list_vct[i].length())
                << "'";
          }
          sql << ") ";
        }
      }
      const std::string sql_string = sql.str().c_str();
      INFO_LOG(logger)("%s", sql_string.c_str());
      const boost::shared_ptr<ResultSet>& result =
          mysql_conn->ExecSelect(sql_string.c_str(), sql_string.size());
      if (result->error) {
        ERROR_LOG(logger)("ExecSql fail:%d, %s", result->error, sql_string.c_str());
        return 0;
      }
      while (result && result->IsValid()) {
        msg.add_player_ids(result->at(0).to_int64());
        if (type == "player_id")
          check_param.insert(result->at(0).to_c_str());
        else if (type == "user_account")
          check_param.insert(result->at(1).to_c_str());
        else if (type == "name")
          check_param.insert(result->at(2).to_c_str());
        result->Next();
      }
    }
    if (type != "server_id") {
      for (size_t i = 0; i < user_list_vct.size(); i++) {
        if (check_param.find(user_list_vct[i].c_str()) == check_param.end()) {
          err_str.push_back(user_list_vct[i].c_str());
          ERROR_LOG(logger)("sendmail not found player by param(uid,openid,name):%s", user_list_vct[i].c_str());
        }
      }
    }
    success =
        server->SendServerMessage(server_id, MSG_SS_SEND_MAIL_TO_MULTI, &msg);
  }
  rep.content = SimpleResultString(success, &err_str);
  return 0;
}

REG_CALLBACK(kickUser) {
  const std::string& name = JsonGetString(doc, "name");
  const std::string& openid = JsonGetString(doc, "user_account");
  int64_t player_id = JsonGetInt<int64_t>(doc, "player_id");
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  if (!player_id) {
    player_id = GetPlayerIDByOpenIDAndServer(openid, server_id);
  }

  bool success = false;
  if (player_id) {
    MessageSSRequestKickUser msg;
    msg.add_player_id(server_id);
    success = server->SendServerMessage(server_id, MSG_SS_REQUEST_KICK_USER, &msg);
  }

  rep.content = SimpleResultString(success);
  return 0;
}

REG_CALLBACK(notice) {
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  std::vector<uint32_t> server_list =
      JsonGetIntArray<uint32_t>(doc, "server_list");
  if (server_id) server_list.push_back(server_id);
  const std::string& action = JsonGetString(doc, "action");
  int32_t tid = JsonGetInt<int32_t>(doc, "tid");
  int32_t notice_type = JsonGetInt<int32_t>(doc, "notice_type");
  time_t begin_time = JsonGetInt<time_t>(doc, "begin_time");
  time_t end_time = JsonGetInt<time_t>(doc, "end_time");
  int32_t type = JsonGetInt<int32_t>(doc, "type");
  const std::string& skip_url = JsonGetString(doc, "skip_url");
  int32_t order = JsonGetInt<int32_t>(doc, "order");
  int32_t exec_time = JsonGetInt<int32_t>(doc, "exec_time");
  const std::string& text = JsonGetString(doc, "text");

  bool success = true;

  for (size_t i = 0; i < server_list.size(); i++) {
    if (!server_list[i]) continue;

    MessageSSServerNotice msg;

    sy::NoticeInfo* info = msg.mutable_notice();

    info->set_server_id(server_list[i]);

    info->set_tid(tid);
    info->set_type(type);
    info->set_order(order);
    if (!skip_url.empty()) info->set_link_url(skip_url.c_str());
    info->set_begin_time(begin_time);
    info->set_end_time(end_time);
    info->set_interval(exec_time * 60);
    info->set_content(text.c_str());

    bool result =
        server->SendServerMessage(server_list[i], MSG_SS_SERVER_NOTICE, &msg);
    if (!result) success = false;
  }

  rep.content = SimpleResultString(success);

  return 0;
}

REG_CALLBACK(delNotice) {
  //{"notice_id":97,"platform_id":"1","server_id":"10001","type":1}
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  int64_t tid = JsonGetInt<int64_t>(doc, "notice_id");
  MessageSSServerNotice msg;
  msg.set_delete_id(tid);
  msg.set_server_id(server_id);
  server->SendServerMessage(server_id, MSG_SS_SERVER_NOTICE, &msg);

  bool success = true;
  rep.content = SimpleResultString(success);
  return 0;
}

REG_CALLBACK(banUser) {
  int64_t player_id = JsonGetInt<int64_t>(doc, "player_id");
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  const std::string& openid = JsonGetString(doc, "user_account");
  if (!player_id) {
    player_id = GetPlayerIDByOpenIDAndServer(openid, server_id);
  }
  int32_t is_ban = JsonGetInt<int32_t>(doc, "is_ban");
  int32_t end_time = JsonGetInt<int32_t>(doc, "end_time");

  bool success = false;
  if (player_id) {
    MessageSSSetAccountStatus request;
    request.set_player_id(player_id);
    request.set_status(is_ban);
    request.set_status_time(end_time);
    success = server->SendServerMessage(server_id, MSG_SS_SET_ACCOUNT_STATUS, &request);
  }

  rep.content = SimpleResultString(success);
  return 0;
}

REG_CALLBACK(setGuide) {
  int64_t player_id = JsonGetInt<int64_t>(doc, "player_id");
  uint32_t server_id = JsonGetInt<uint32_t>(doc, "server_id");
  const std::string& guide = JsonGetString(doc, "guide");

  MessageSSServerSetDialog req;
  req.set_player_id(player_id);
  req.set_guide(guide);
  bool success = false;
  success =
      server->SendServerMessage(server_id, MSG_SS_SERVER_SET_DIALOG, &req);
  rep.content = SimpleResultString(success);
  return 0;
}

REG_CALLBACK(getTimeActivity) {
  std::vector<uint32_t> servers = JsonGetIntArray<uint32_t>(doc, "server_ids");
  for (size_t i = 0; i < servers.size(); i++) {
    server->SendServerMessage(servers[i], MSG_SS_SREVER_TIME_ACTIVITY, NULL);
  }
  rep.content = SimpleResultString(true);
  return 0;
}

REG_CALLBACK(blackList) {
  std::vector<int64_t> server_id = JsonGetIntArray<int64_t>(doc, "server_id");
  int32_t type = JsonGetInt<int32_t>(doc, "type");
  int32_t add_type = JsonGetInt<int32_t>(doc, "add_type");
  std::vector<std::string> ip_list = JsonGetStringArray(doc, "ip_list");

  MessageSSSetIpList msg;
  msg.set_type(type);
  msg.set_append(add_type);
  for (std::vector<std::string>::iterator it = ip_list.begin();
       it != ip_list.end(); ++it) {
    msg.add_list(*it);
  }
  for (std::vector<int64_t>::iterator it = server_id.begin();
       it != server_id.end(); ++it) {
    msg.set_server_id(*it);
    server->SendServerMessage(*it, MSG_SS_SET_IP_LIST, &msg);
    int32_t type = msg.type();
    int32_t add_type = msg.append();
    std::string column;
    if (1 == type)
      column = "black_list";
    else if (2 == type)
      column = "white_list";
    else
      continue;

    if (add_type != 1 && add_type != 2) continue;
    const boost::shared_ptr<MySqlConnection>& mysql_conn =
        Server::GetGameConn(*it, true);
    std::ostringstream sql;
    sql << "update ip_list set " << column.c_str() << " = ";
    if (2 == add_type) sql << "concat(" << column.c_str() << ",";
    sql << "'";
    for (int32_t j = 0; j < msg.list_size(); j++)
      sql << ","
          << mysql_conn->EscapeString(msg.list(j).c_str(), msg.list(j).length())
                 .c_str();
    sql << "'";
    if (2 == add_type) sql << ")";
    sql << " where server_id=" << msg.server_id();
    const std::string sql_string = sql.str();
    mysql_conn->ExecSelect(sql_string.c_str(), sql_string.size());
  }

  rep.content = SimpleResultString(1);

  return 0;
}

namespace http {
namespace server2 {

HttpRequestHandler::HttpRequestHandler() {
}

const std::string kAction = "action";
const std::string kData = "data";

int32_t HttpRequestHandler::handle_request(const request& req, reply& rep,
                                           const connection_ptr& conn) {
  rep.status = reply::ok;
  //跨域请求
  if (req.method == "OPTIONS") {
    INFO_LOG(logger)("Uri:%s, Method:%s", req.uri.c_str(), req.method.c_str());
    rep.AddHeader("Access-Control-Allow-Origin", "*");
    rep.AddHeader("Access-Control-Allow-Methods", "GET,DELETE,POST,PUT");
    rep.AddHeader("Access-Control-Allow-Headers", "content-type");
    rep.content = "12121212";
    return 0;
  }

  if (req.uri.find("ipaddr") != std::string::npos) {
    boost::system::error_code err;
    try {
      rep.content =
          conn.get()->socket().remote_endpoint().address().to_string();
    } catch (...) {
      DEBUG_LOG(logger)("GetIPAddr fail:%s", err.message().c_str());
    }
    rep.status = reply::ok;
    return 0;
  }

  //充值请求
  if (req.uri.find("pay") != std::string::npos) {
    return this->handle_payment(req, rep, conn);
  }

  // HTTP逻辑处理
  std::string action;
  std::string data;
  for (std::vector<query_str>::const_iterator iter = req.post_contents.begin();
       iter != req.post_contents.end(); ++iter) {
    INFO_LOG(logger)("QueryString %s => %s", iter->key.c_str(), iter->value.c_str());
    if (iter->key == kAction) { action = iter->value; continue; }
    if (iter->key == kData) { data = iter->value; continue; }
  }

  int32_t result = 0;
  do {
    if (action.empty() || data.empty()) {
      rep.status = reply::bad_request;
      rep.content = "<h1>400</h1>";
      result = 0;
      break;
    }

    const std::string& json_data = decode64(data);
    Document doc;
    doc.Parse(json_data.c_str());

    INFO_LOG(logger)("%s", json_data.c_str());

    const MySqlConnPtr& gm_conn = MySqlConnManager::Instance().GetConnection(CONN_TYPE_GM);
    std::ostringstream oss;
    oss << "insert into http_action(http_time, account, action, data) values ("
        << GetSeconds() << ",'','"
        << gm_conn->EscapeString(action.c_str(), action.length()) << "','"
        << gm_conn->EscapeString(json_data.c_str(), strlen(json_data.c_str())) << "')";
    const std::string& sql = oss.str();
    gm_conn->ExecSql(sql.c_str(), sql.length());
    DEBUG_LOG(logger)("%s", sql.c_str());

    rep.status = reply::ok;
    boost::unordered_map<std::string, HttpRequestProc>::iterator iter =
        kHttpCallback.find(action);
    if (iter == kHttpCallback.end()) {
      result = EmptyHttpRequestProc(doc, rep, conn);
      break;
    }
    result = (*iter->second)(doc, rep, conn);
  } while (0);

  std::string content = rep.content;
  content.resize(content.size() + 1, 0);
  logger->LogMessage(const_cast<char*>(content.c_str()), content.size() - 1);

  DEBUG_LOG(logger)("HttpRequest:%s, Result:%d", action.c_str(), result);
  if (result) ConnectionManager::Instance().AddConnection(conn);
  return result;
}
}}

void ConnectionManager::AddConnection(connection_ptr conn) {
  lock_.lock();
  conn->time_stamp = ::GetSeconds();
  connection_map_[conn->id()] = conn;
  lock_.unlock();
}

connection_ptr ConnectionManager::FetchConnection(int64_t id) {
  std::lock_guard<std::mutex> gd(lock_);
  boost::unordered_map<int64_t, connection_ptr>::iterator it =
      connection_map_.find(id);
  if (it == connection_map_.end()) return connection_ptr();

  connection_ptr conn_ptr = it->second;
  connection_map_.erase(it);
  return conn_ptr;
}

void ConnectionManager::RemoveIdleConnection() {
  for (boost::unordered_map<int64_t, http::server2::connection_ptr>::iterator
           it = connection_map_.begin();
       it != connection_map_.end();
       /*i++*/) {
    if (GetSeconds() - it->second->time_stamp > 60 * 2) {
      it = connection_map_.erase(it);
    } else {
      it++;
    }
  }
}

int32_t ProcessAddGoods(SSMessageEntry& entry) {
  MessageSSResponseAddGood* message =
      static_cast<MessageSSResponseAddGood*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const connection_ptr& conn =
      ConnectionManager::Instance().FetchConnection(message->conn_id());
  if (!conn) return ERR_INTERNAL;

  Document output;
  output.SetObject();
  Document::AllocatorType& alloc = output.GetAllocator();
  Value status;
  status.SetInt(!message->result());
  Value data(rapidjson::kArrayType);

  for (int32_t i = 0; i < message->player_uid_size(); i++)
    data.PushBack(message->player_uid(i), alloc);

  output.AddMember("status", status, output.GetAllocator());
  output.AddMember("data", data, output.GetAllocator());

  conn->repsonse_header().content = DocToString(output);

  conn->repsonse_header().status = http::server2::reply::ok;
  conn->async_send(conn->repsonse_header().content);

  DEBUG_LOG(logger)("%s",conn->repsonse_header().content.c_str());

  return ERR_OK;
}
