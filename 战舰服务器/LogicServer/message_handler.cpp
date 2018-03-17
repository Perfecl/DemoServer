#include <google/protobuf/text_format.h>
#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <net/MessageCache.h>
#include <system.h>
#include "logic_player.h"
#include "server.h"
#include "rank_list.h"
#include "murmur3.h"
#include "army.h"
#include "time_activity.h"
#include "legion_war.h"
#include <md5.h>
#include <array_stream.h>
#include <str_util.h>
#include <stdlib.h>
#include <algorithm>

using namespace sy;
using namespace intranet;

int32_t ProcessSkipThisMessage(CSMessageEntry& entry) { return ERR_OK; }
int32_t ProcessSkipThisMessage(SSMessageEntry& entry) { return ERR_OK; }

static VectorSet<std::string> kBanIpList;

static inline void InitBanIpList() {
  static int init = 0;
  if (init) return;
  kBanIpList.insert("180.102.103.150");
  kBanIpList.insert("121.229.24.130");
  kBanIpList.insert("180.110.162.157");
  kBanIpList.insert("180.110.163.18");
}

int32_t ProcessRequestPlayerLogin(CSMessageEntry& entry) {
  MessageRequestLogin* message = static_cast<MessageRequestLogin*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->account().size() >= 64) return ERR_PARAM_INVALID;
  if (message->channel().size() >= 64) return ERR_PARAM_INVALID;
  if (message->device_id().size() >= 128) return ERR_PARAM_INVALID;
  if (message->idfa().size() >= 128) return ERR_PARAM_INVALID;

  InitBanIpList();

  MessageSSRequestGetUID request;
  request.set_openid(message->account());
  request.set_channel(message->channel());
  request.set_device_id(message->device_id());
  request.set_idfa(message->idfa());

  if (!server->init_state()) {
    DEBUG_LOG(logger)("PlayerLogin Fail,server not inited:%s", request.openid().c_str());
    entry.session_ptr->Shutdown();
    return ERR_SERVER_INIT;
  }

  const std::string& ip_addr = entry.session_ptr->IpAddr();
  if (kBanIpList.find(ip_addr) != kBanIpList.end()) {
    DEBUG_LOG(logger)("PlayerLogin Account:%s, IpAddr:%s In Black List"
        , request.openid().c_str(), ip_addr.c_str());
    entry.session_ptr->Shutdown();
    return ERR_OK;
  }

  if (server->white_list().empty() && server->black_list().count(ip_addr)) {
    MessageErrorCode code;
    code.set_err_code(ERR_IN_BLACK_LIST);
    code.set_msg_id(MSG_CS_REQUEST_LOGIN_C);
    server->SendMessageToClient(entry.session_ptr.get(), MSG_CS_ERROR_CODE, &code);
    entry.session_ptr->Shutdown();
    DEBUG_LOG(logger)("PlayerLogin Account:%s, IpAddr:%s In Black List"
        , request.openid().c_str(), ip_addr.c_str());
    return ERR_OK;
  }
  if (!server->white_list().empty() && !server->white_list().count(ip_addr)) {
    MessageErrorCode code;
    code.set_err_code(ERR_NOT_IN_WHITE_LIST);
    code.set_msg_id(MSG_CS_REQUEST_LOGIN_C);
    server->SendMessageToClient(entry.session_ptr.get(), MSG_CS_ERROR_CODE, &code);
    entry.session_ptr->Shutdown();
    DEBUG_LOG(logger)("PlayerLogin Account:%s, IpAddr:%s Not In White List"
        , request.openid().c_str(), ip_addr.c_str());
    return ERR_OK;
  }
  if(message->has_server_id())
    request.set_server(message->server_id());
  else
    request.set_server(server_config->server_id());
  request.set_session_id(entry.session_ptr->GetSessionID());
  server->SendMessageToAuth(MSG_SS_REQUEST_GET_UID, &request);

  TRACE_LOG(logger)("PlayerLogin, OpenID:%s, SessionID:%ld, Channel:%s, DeviceID:%s, IDFA:%s",
                    message->account().c_str(),
                    entry.session_ptr->GetSessionID(),
                    message->channel().c_str(), message->device_id().c_str(),
                    message->idfa().c_str());
  return ERR_OK;
}


int32_t ProcessResponsePlayerLogin(SSMessageEntry& entry) {
  MessageSSResponseGetUID* message =
      static_cast<MessageSSResponseGetUID*>(entry.get());
  if (!message) return ERR_INTERNAL;

  TcpSessionPtr session = TcpSessionManager::Instance().GetTcpSessionBySessionID(message->session_id());
  if (!session) {
   TRACE_LOG(logger)("%s SessionID:%ld not found", __PRETTY_FUNCTION__, message->session_id());
   return ERR_OK;
  }

  if (message->error_code() || !message->uid()) {
    ERROR_LOG(logger)("OpenID:%s GetUID Fail, ErrorCode:%d"
        , message->openid().c_str(), message->error_code());

    MessageErrorCode code;
    code.set_err_code(message->error_code());
    code.set_msg_id(MSG_CS_REQUEST_LOGIN_C);
    server->SendMessageToClient(session.get(), MSG_CS_ERROR_CODE, &code);
    session->Shutdown();
    return ERR_OK;
  }

  LogicPlayer *player = server->GetOrNewPlayer(message->uid());
  if (!player) {
   ERROR_LOG(logger)("%s GetPlayer:%ld fail", __PRETTY_FUNCTION__, message->uid());
   return ERR_INTERNAL;
  }

  const boost::shared_ptr<TcpSession>& old_session = player->session().lock();
  if (old_session && old_session->GetSessionID() != session->GetSessionID()) {
    DEBUG_LOG(logger)("PlayerID:%ld, ReLogin, Close OldSession:%ld, NewSession:%ld"
        , message->uid(), old_session->GetSessionID(), session->GetSessionID());
    player->SendMessageToClient(MSG_CS_NOTIFY_RELOGIN, NULL);
    old_session->Shutdown();
    old_session->SetUID(0);
  }

  player->channel(message->channel());
  player->device_id(message->device_id());
  player->idfa(message->idfa());
  player->session(session);
  player->account(message->openid());
  player->active();
  player->player().set_server(message->server());
  session->SetUID(message->uid());

  MessageResponseLogin response;
  response.set_account(message->openid());
  response.set_player_id(message->uid());
  response.set_token(server->tokens().MakeToken(message->openid(),message->uid(),message->expiry_time()));
  response.set_expiry_time(message->expiry_time());
  response.set_recharge_platform(server_config->recharge_platform());
  response.set_start_time(server->server_start_time());
  response.set_create_time(message->create());
  response.set_is_create(message->create());

  player->SendMessageToClient(MSG_CS_RESPONSE_LOGIN_S, &response);

  if (message->create()) {
    MessageSSUpdateCreatePlayerInfo req;
    req.set_time(GetSeconds());
    req.set_server(message->server());
    req.set_openid(message->openid());
    req.set_player_id(message->uid());
    req.set_channel(message->channel());
    server->SendServerMessageToDB(MSG_SS_UPDATE_CREATE_PLAYER_INFO, &req);
  }

  TRACE_LOG(logger)(
      "PlayerLogin success, OpenID:%s, PlayerID:%ld, SessionID:%ld",
      message->openid().c_str(), player->uid(), message->session_id());
  return ERR_OK;
}

int32_t ProcessRequestReportToken(CSMessageEntry& entry) {
  MessageReportToken* message = static_cast<MessageReportToken*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->token().size() >= 512) return ERR_PARAM_INVALID;
  if (message->account().size() >= 64) return ERR_PARAM_INVALID;

  int32_t result = 0;
  int64_t player_id = 0;

  do {
    result = server->tokens().CheckToken(message->token(), player_id);
    if (result) break;

    LogicPlayer* player = server->GetPlayerByID(player_id);

    if (!player || !player->load_complete()) {
      result = ERR_TOKEN_TIMEOUT;
      break;
    }

    const boost::shared_ptr<TcpSession>& old_session = player->session().lock();
    if (old_session && old_session != entry.session_ptr) {
      TRACE_LOG(logger)("PlayerID:%ld OldSession:%ld Shutdown", player->uid(), old_session->GetSessionID());
      old_session->SetUID(0);
      old_session->Shutdown();
    }
    player->session(entry.session_ptr);
    player->active();
    //重发消息给客户端
    std::vector<MessageCache::RawMessage> cached_message =
        MessageCache::Instance().GetCachedMessage(player_id);
    for (std::vector<MessageCache::RawMessage>::iterator iter =
             cached_message.begin();
         iter != cached_message.end(); ++iter) {
      entry.session_ptr->ASyncWrite(&*iter->begin(), iter->size());
    }
  } while (false);

  MessageReportTokenS msg;
  msg.set_error_id(result);
  server->SendMessageToClient(entry.session_ptr.get(), MSG_CS_REPORT_TOKEN_S,
                              &msg);

  return ERR_OK;
}

int32_t ProcessRequestHeartBeat(CSMessageEntry& entry) {
  MessageRequestHeartBeat* message =
      static_cast<MessageRequestHeartBeat*>(entry.get());
  if (!message) return ERR_INTERNAL;
  MessageResponseHeartBeat response;
  response.set_sec(GetMilliSeconds());
  response.set_millisec(message->millisec());
  response.set_version(server_config->version());

  server->SendMessageToClient(entry.session_ptr.get(),
                              MSG_CS_RESPONSE_HEART_BEAT, &response);
  return ERR_OK;
}

int32_t ProcessResponseNewMail(SSMessageEntry& entry) {
  MessageSSSendMail *message = static_cast<MessageSSSendMail*>(entry.get());
  if (!message) return ERR_INTERNAL;
  LogicPlayer* player = server->GetPlayerByID(message->player_id());
  if (player && !player->session().expired()) {
    MessageNotifyMailInfo notify;
    notify.mutable_new_mail()->CopyFrom(message->mail());
    player->SendMessageToClient(MSG_CS_NOTIFY_MAIL_INFO, &notify);
  }
  return ERR_OK;
}

int32_t ProcessResponseGetPKRankList(SSMessageEntry& entry) {
  MessageSSResponseGetPKRankList* message =
      static_cast<MessageSSResponseGetPKRankList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageSSRequestLoadMultiPlayer request;
  server->ClearPKRank();
  for (int32_t i = 0; i < message->ranks_size(); ++i) {
    const sy::PKRankInfo& info = message->ranks(i);
    if (info.rank() > 0 && info.rank() < sy::MAX_ROBOT_ID) {
      server->InitPkRank(info.player_id(), info.rank());
      int32_t new_rank = server->GetRank(info.player_id());
      DEBUG_LOG(logger)("PK Rank Player:%ld, Rank:%d, NewRank:%d", info.player_id(), info.rank(), new_rank);
    }
    if (info.rank() > 0 && info.rank() <= ARENA_LIST_COUNT) {
      server->GetOrNewPlayer(info.player_id());
      request.add_player_ids(info.player_id());
    }
  }

  TRACE_LOG(logger)("Server Load PK Rank List Complete");
  TRACE_LOG(logger)("Load Arena Player:%d", request.player_ids_size());
  request.set_msg_id(MSG_CS_REQUEST_GET_OTHER_PLAYER);
  server->SendServerMessageToDB(MSG_SS_REQUEST_LOAD_MULTI_PLAYER, &request);

  server->set_init_state(0);

  return ERR_OK;
}

int32_t ProcessGM(MessageSSResponseLoadMultiPlayer* message) {
  MessageSSRequestLoadMultiPlayer* request = message->mutable_request();
  if (!request->has_forward_gm()) return ERR_OK;

  //GM修改数据
  sy::GMContent* info = request->mutable_forward_gm();

  int32_t level = -1;
  int32_t vip_level = -1;
  ModifyCurrency modify(intranet::MSG_SS_REQUEST_ADD_GOODS, SYSTEM_ID_GM);
  AddSubItemSet add_sub_set;

  if (info->has_level()) {
    const ExpBasePtr& base = EXP_BASE.GetEntryByID(info->level());
    if (!base) {
      ERROR_LOG(logger)("ProcessGM, Level:%d not found", info->level());
      return ERR_PARAM_OVERFLOW;
    }
    level = info->level();
  }
  if (info->has_vip_level()) {
    const VipExpBasePtr& base = VIP_EXP_BASE.GetEntryByID(info->vip_level());
    if (!base) {
      ERROR_LOG(logger)("ProcessGM, VipLevel:%d not found", info->vip_level());
      return ERR_PARAM_OVERFLOW;
    }
    vip_level = info->vip_level();
  }

  for (int32_t i = 0; i < info->modify_size(); ++i) {
    modify[info->modify(i).key()] += info->modify(i).value();
  }
  if (modify.empty) {
    ERROR_LOG(logger)("ProcessGM, Unkown Currency");
    return ERR_CURRENCY_UNKNOW;
  }

  if (info->add_sub_item_size() > ITEM_COUNT_ONE_TIME) {
    ERROR_LOG(logger)("ProcessGM, AddSubItemCount:%d", info->add_sub_item_size());
    return ERR_PARAM_ARRAY_BOUND;
  }
  for (int32_t i = 0; i < info->add_sub_item_size(); ++i) {
    add_sub_set.push_back(
        ItemParam(info->add_sub_item(i).key(), info->add_sub_item(i).value()));
  }

  for (int32_t i = 0; i < request->forward_ids_size(); ++i) {
    LogicPlayer* player = server->GetPlayerByID(request->forward_ids(i));
    if (!player) {
      ERROR_LOG(logger)("ProcessGM, PlayerID:%ld not found", request->player_ids(i));
      return ERR_PLAYER_NOT_EXIST;
    }
    CHECK_RET(player->CheckCurrency(modify));
    CHECK_RET(player->CheckItem(&add_sub_set, NULL, NULL));
  }

  for (int32_t i = 0; i < request->forward_ids_size(); ++i) {
    LogicPlayer* player = server->GetPlayerByID(request->forward_ids(i));
    if (level >= 0) player->SetLevel(level);
    if (vip_level >= 0) player->SetVipLevel(vip_level);
    if (level >= 0 || vip_level >= 0) modify.force_update = true;
    player->UpdateCurrency(modify);
    player->ObtainItem(&add_sub_set, NULL, NULL,
                       intranet::MSG_SS_REQUEST_ADD_GOODS, SYSTEM_ID_GM);
  }

  return ERR_OK;
}

int32_t ProcessPK(MessageSSResponseLoadMultiPlayer* message) {
  MessageSSRequestLoadMultiPlayer* request = message->mutable_request();
  LogicPlayer* p1 = server->GetPlayerByID(request->pk_player());
  if (request->forward_ids_size() < 1) return ERR_INTERNAL;
  LogicPlayer* p2 = server->GetPlayerByID(request->forward_ids(0));

  if (p1 && p2) {
    //竞技场
    if (request->has_arena()) {
      int32_t result = p1->ArenaPK(request->mutable_arena());
      if (result) {
        p1->SendErrorCodeToClient(result, MSG_CS_REQUEST_TEST_PK_TARGET);
      }
    } else {
      //切磋
      p1->TestPK(request->pk_player());
    }
  }

  return ERR_OK;
}

int32_t ProcessResponseGetOtherPlayerInfo(SSMessageEntry& entry) {
  MessageSSResponseLoadMultiPlayer* message =
      static_cast<MessageSSResponseLoadMultiPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;
  MessageSSRequestLoadMultiPlayer* request = message->mutable_request();

  // GMContent callback
  if (request->has_forward_gm()) {
    int32_t result = ProcessGM(message);

    MessageSSResponseAddGood response;
    response.set_result(result);
    response.set_conn_id(request->conn_id());
    response.mutable_player_uid()->CopyFrom(request->forward_ids());
    TcpSessionPtr session = TcpSessionManager::Instance().GetTcpSessionBySessionID(request->session_id());
    if (session) {
      server->SendMessageToServer(session.get(), MSG_SS_RESPONSE_ADD_GOODS, &response);
    }
  }

  //切磋
  if (request->has_pk_player()) {
  }

  return ERR_OK;
}

int64_t GetBossBlood(const sy::DstrikeBoss& boss) {
  int64_t blood = 0;
  for (int32_t i = 0; i < boss.boss_blood_size(); ++i) {
    if (boss.boss_blood(i) > 0) blood += boss.boss_blood(i);
  }
  return blood;
}

int32_t Server::GetDstrikeBossList(LogicPlayer* player,
                                   sy::MessageResponseDstrikeList& response) {
  if (!player) return ERR_PARAM_INVALID;
  std::vector<int64_t> friends;
  player->GetAllFriends(friends);
  friends.push_back(player->uid());

  for (std::vector<int64_t>::const_iterator iter = friends.begin();
       iter != friends.end(); ++iter) {
    sy::DstrikeBoss* boss = this->GetDstrikeBossByPlayerId(*iter);
    if (boss && IsBossValid(*boss) &&
        (boss->boss_status() || boss->player_id() == player->uid())) {
      INFO_LOG(logger)("PlayerID:%ld, BossBlood:%ld, CreateTime:%d, ExpireTime:%d, CurrentTime:%ld",
          player->uid(), GetBossBlood(*boss), boss->boss_time(), boss->boss_expire_time(), GetSeconds());
      response.add_list()->CopyFrom(*boss);
    }
  }
  return ERR_OK;
}

bool IsBossValid(const sy::DstrikeBoss& boss) {
  long blood = GetBossBlood(boss);
  if (blood <= 0ll) {
    INFO_LOG(logger)("BossID:%ld blood is 0", boss.player_id());
    return false;
  }
  if (boss.boss_time() + boss.boss_expire_time() <= GetSeconds()) {
    INFO_LOG(logger)("BossID:%ld escape", boss.player_id());
    return false;
  }
  return true;
}


sy::DstrikeBoss* Server::GetDstrikeBossByPlayerId(int64_t player_id) {
  BossContainer::iterator iter = this->dstrike_boss_.find(player_id);
  if (iter == this->dstrike_boss_.end()) return NULL;
  sy::DstrikeBoss& boss = iter->second;
  return &boss;
}

void Server::AddDstrikeBoss(const sy::DstrikeBoss& info, bool with_save) {
  if (!IsBossValid(info)) return;
  this->dstrike_boss_[info.player_id()].CopyFrom(info);


  if (with_save) {
    MessageSSUpdateDstrikeBossInfo request;
    request.set_server_id(server_config->server_id());
    request.mutable_boss()->CopyFrom(info);
    this->SendServerMessageToDB(MSG_SS_UPDATE_DSTRIKE_BOSS_INFO, &request);
  }
}

// 0, 正常扣血
// 1, 最后一击
// 2, boss不存在
int32_t Server::SubDstrikeBossHP(int64_t player_id,
                                 const std::vector<int64_t>& blood) {
  sy::DstrikeBoss* boss = this->GetDstrikeBossByPlayerId(player_id);
  if (!boss || !IsBossValid(*boss)) return 2;
  MessageSSUpdateDstrikeBossInfo request;
  request.set_server_id(server_config->server_id());

  boss->mutable_boss_blood()->Resize(6, 0);
  int32_t index = 1;
  for(std::vector<int64_t>::const_iterator iter = blood.begin();
      iter != blood.end(); ++iter, ++index) {
    int64_t current_hp = boss->mutable_boss_blood()->Get(index - 1);
    current_hp -= *iter;
    boss->mutable_boss_blood()->Set(index - 1, current_hp < 0 ? 0 : current_hp);
  }
  request.mutable_boss()->CopyFrom(*boss);

  DEBUG_LOG(logger)("PlayerID:%ld BossBlood:%ld,%ld,%ld,%ld,%ld,%ld TotalBlood:%ld"
      , player_id
      , boss->boss_blood(0), boss->boss_blood(1)
      , boss->boss_blood(2), boss->boss_blood(3)
      , boss->boss_blood(4), boss->boss_blood(5)
      , GetBossBlood(*boss)
      );
  boss = this->GetDstrikeBossByPlayerId(player_id);
  //删除BOSS
  if (!boss || !IsBossValid(*boss)) {
    DEBUG_LOG(logger)("DstrikeBoss Dead, PlayerID:%ld", player_id);
    this->dstrike_boss_.erase(player_id);
    request.set_player_id(player_id);
    return 1;
  }
  this->SendServerMessageToDB(MSG_SS_UPDATE_DSTRIKE_BOSS_INFO, &request);
  return 0;
}

int32_t ProcessResponseLoadRankList(SSMessageEntry& entry) {
  MessageSSResponseLoadRankList *message = static_cast<MessageSSResponseLoadRankList*>(entry.get());
  if (!message) return ERR_INTERNAL;
  TRACE_LOG(logger)("Load RankList:%d Complete:%d", message->type(), message->list().items_size());
  RANK_LIST.OnRecvRankListData(message->type(), message->list());
  if ( message->list().items_size()) DEBUG_LOG(logger)("\n%s", RankListBase::Print(RANK_LIST.GetByType(message->type()).data()).c_str());
  return ERR_OK;
}

struct KickAllPlayers {
  KickAllPlayers(std::vector<LogicPlayer*>& players) : players(players) {}
  std::vector<LogicPlayer*>& players;
  bool operator()(LogicPlayer* p) const {
    players.push_back(p);
    return true;
  }
};

int32_t ProcessRequestKickUser(SSMessageEntry& entry) {
  MessageSSRequestKickUser* message =
      static_cast<MessageSSRequestKickUser*>(entry.get());
  if (!message) return ERR_INTERNAL;

  std::vector<LogicPlayer*> players;
  if (message->all()) {
    KickAllPlayers for_each(players);
    server->Players().ForEach(for_each);
  } else {
    for (int32_t i = 0; i < message->player_id_size(); ++i) {
      LogicPlayer* player = server->GetPlayerByID(message->player_id(i));
      if (player) {
        players.push_back(player);
      }
    }
  }
  for (std::vector<LogicPlayer*>::iterator iter = players.begin();
       iter != players.end(); ++iter) {
    const TcpSessionPtr& ptr = (*iter)->session().lock();
    if (ptr) ptr->Close();
    TRACE_LOG(logger)("GM KickPlayer:%ld", (*iter)->uid());
    server->ErasePlayer((*iter)->uid());
  }

  server->SendServerMessageToDB(entry.head.msgid, message);
  return ERR_OK;
}

int32_t ProcessResponseLoadBossList(SSMessageEntry& entry) {
  MessageSSResponseLoadBossList* message =
      static_cast<MessageSSResponseLoadBossList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  for (int32_t i = 0; i < message->boss_size(); ++i) {
    server->AddDstrikeBoss(message->boss(i), false);
    TRACE_LOG(logger)("DstrikeBoss PlayerID:%ld, BossID:%d, time:%d, expiry_time:%d"
        , message->boss(i).player_id(), message->boss(i).boss_id()
        , message->boss(i).boss_time(), message->boss(i).boss_expire_time());
  }
  server->set_init_state(1);

  return ERR_OK;
}

int32_t ProcessResponseLoadServerShop(SSMessageEntry& entry) {
  MessageSSResponseLoadServerShop* message =
      static_cast<MessageSSResponseLoadServerShop*>(entry.get());
  if (!message) return ERR_INTERNAL;

  server->server_shop().clear_items();
  server->server_shop().mutable_items()->CopyFrom(message->items());
  server->AstrologyAwardCountry(message->astrology_country_id());
  server->AstrologyRefreshTime(message->astrology_refresh_time());
  TRACE_LOG(logger)("astrology_country_id:%d", server->AstrologyAwardCountry());

  server->RefreshAstrologyAward();

  server->set_init_state(2);

  return ERR_OK;
}

int32_t ProcessSetAccountStatus(SSMessageEntry& entry) {
  MessageSSSetAccountStatus* message =
      static_cast<MessageSSSetAccountStatus*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicPlayer *player = server->GetPlayerByID(message->player_id());
  if (player) {
    if (message->has_status()) player->player().set_status(message->status());
    if (message->has_status_time()) player->player().set_status_time(message->status_time());
    if (message->has_flag()) player->player().set_flag(message->flag());
    //判断玩家是否需要被T下线
    player->CheckBanToLogin();
  }

  if (message->status() == 2) RANK_LIST.RemoveAll(message->player_id());

  server->SendServerMessageToDB(MSG_SS_SET_ACCOUNT_STATUS, message);
  return ERR_OK;
}

int32_t ProcessResponseLoadServerMail(SSMessageEntry& entry) {
  MessageSSResponseLoadServerMail* message =
      static_cast<MessageSSResponseLoadServerMail*>(entry.get());
  if (!message) return ERR_INTERNAL;

  server->server_mail().Clear();
  server->server_mail().CopyFrom(*message);
  server->set_init_state(6);
  return ERR_OK;
}

int32_t ProcessRequestAddServerMail(SSMessageEntry& entry) {
  MessageSSAddServerMail* message =
      static_cast<MessageSSAddServerMail*>(entry.get());
  if (!message) return ERR_INTERNAL;

  message->mutable_mail()->set_mail_id(server->GetNewMailID());
  TRACE_LOG(logger)("Send ServerMail:%ld, Content:%s"
      , message->mail().mail_id(), message->mail().mail_content().c_str());
  server->server_mail().add_mails()->CopyFrom(message->mail());

  server->SendServerMessageToDB(MSG_SS_ADD_SERVER_MAIL, message);
  return ERR_OK;
}

int32_t ProcessRequestSendMailToMulti(SSMessageEntry& entry) {
  MessageSSSendMailToMulti* message =
      static_cast<MessageSSSendMailToMulti*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (!message->player_ids_size()) {
    ERROR_LOG(logger)("Send Mail To MultiPlayer, PlayerCount:0");
  }

  const sy::MailInfo& mail = message->mail();
  std::vector<std::pair<int32_t,int32_t> > reward;
  for (int32_t reward_index = 0; reward_index < mail.mail_attachment_size();
       ++reward_index) {
    reward.push_back(
        std::make_pair(mail.mail_attachment(reward_index).key(),
                       mail.mail_attachment(reward_index).value()));
  }
  TRACE_LOG(logger)("Send Mail To MultiPlayer, MailID:%ld", mail.mail_id());
  for (int32_t i = 0; i < message->player_ids_size(); ++i) {
    int64_t player_id = message->player_ids(i);
    TRACE_LOG(logger)("PlayerID:%ld", player_id);
    LogicPlayer::SendMail(player_id, mail.mail_time(), mail.mail_type(),
                          mail.mail_content(), &reward);
  }

  return ERR_OK;
}

int32_t ProcessRequestSetIpList(SSMessageEntry& entry) {
  MessageSSSetIpList* message = static_cast<MessageSSSetIpList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  boost::unordered_set<std::string>& set =
      message->type() == 2 ? server->white_list() : server->black_list();
  if (message->append() != 2) set.clear();
  for (int32_t i = 0; i < message->list_size(); ++i) {
    set.insert(message->list(i));
  }
  for (boost::unordered_set<std::string>::const_iterator iter =
           server->white_list().begin();
       iter != server->white_list().end(); ++iter) {
    INFO_LOG(logger)("WhiteList, %s", iter->c_str());
  }
  for (boost::unordered_set<std::string>::const_iterator iter =
           server->black_list().begin();
       iter != server->black_list().end(); ++iter) {
    INFO_LOG(logger)("BlackList, %s", iter->c_str());
  }

  server->SendGetServerStartDaysMessage();
  return ERR_OK;
}

static inline int32_t ProcessAddGoodsInternal(
    MessageSSRequestAddGood* message, int64_t session_id) {
  MessageSSRequestLoadMultiPlayer request;
  for (int32_t i = 0; i < message->player_uid_size(); ++i) {
    int64_t player_id = message->player_uid(i);
    if (player_id <= sy::MAX_ROBOT_ID) continue;

    request.add_forward_ids(player_id);
    LogicPlayer* player = server->GetPlayerByID(player_id);
    if (!player) {
      request.add_player_ids(player_id);
      player = server->GetOrNewPlayer(player_id);
    }
  }
  request.set_msg_id(MSG_CS_REQUEST_GET_OTHER_PLAYER);
  request.mutable_forward_gm()->CopyFrom(message->content());
  request.set_conn_id(message->conn_id());
  request.set_session_id(session_id);
  server->SendServerMessageToDB(MSG_SS_REQUEST_LOAD_MULTI_PLAYER, &request);
  return ERR_OK;
}

int32_t ProcessAddGoods(SSMessageEntry& entry) {
  MessageSSRequestAddGood* message = static_cast<MessageSSRequestAddGood*>(entry.get());
  if (!message) return ERR_INTERNAL;
  return ProcessAddGoodsInternal(message, entry.session_ptr->GetSessionID());
}

int32_t ProcessResponseLoadIPList(SSMessageEntry& entry) {
  MessageSSResponseLoadIPList* message =
      static_cast<MessageSSResponseLoadIPList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  server->white_list().clear();
  server->black_list().clear();

  for (int32_t i = 0; i < message->white_list_size(); i++) {
    server->white_list().insert(message->white_list(i));
  }
  for (int32_t i = 0; i < message->black_list_size(); i++) {
    server->black_list().insert(message->black_list(i));
  }
  server->set_init_state(3);

  return ERR_OK;
}

int32_t ProcessSetNotice(SSMessageEntry& entry) {
  MessageSSServerNotice* message =
      static_cast<MessageSSServerNotice*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->has_notice()) {
    const NoticeInfo& info = message->notice();

    if (info.end_time() > GetSeconds()) {
      TRACE_LOG(logger)("ServerNotice, ID:%ld", info.tid());
      server->AddNotice(info);
      MessageNotifyNotice msg;
      NoticeInfo* notice = msg.add_notices();
      notice->CopyFrom(info);
      server->SendMessageToAllClient(MSG_CS_NOTIFY_NOTICE, &msg);
    }
  }
  if (message->has_delete_id()) {
    server->DelNotice(message->delete_id());
    MessageNotifyNotice msg;
    msg.add_delete_notices(message->delete_id());
    server->SendMessageToAllClient(MSG_CS_NOTIFY_NOTICE, &msg);
  }

  server->SendServerMessageToDB(MSG_SS_SERVER_NOTICE, message);
  return ERR_OK;
}

int32_t ProcessResponseLoadNotice(SSMessageEntry& entry) {
  MessageSSResponseLoadNotice* message =
      static_cast<MessageSSResponseLoadNotice*>(entry.get());
  if (!message) return ERR_INTERNAL;

  for (int32_t i = 0; i < message->notices_size(); i++) {
    server->AddNotice(message->notices(i));
  }
  server->set_init_state(5);

  return ERR_OK;
}

int32_t ProcessRequestRegisteAccount(CSMessageEntry& entry) {
  MessageRequestRegisteAccount* message =
      static_cast<MessageRequestRegisteAccount*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->account().empty() ||
      message->account().length() >= 64u)
    return ERR_PARAM_INVALID;
  if (message->password().empty() ||
      message->password().length() >= 64u)
    return ERR_PARAM_INVALID;

  MessageSSRequestRegisteAccount request;
  request.set_account(message->account());
  request.set_password(message->password());
  request.set_session_id(entry.session_ptr->GetSessionID());

  INFO_LOG(logger)("Registerccount, SessionID:%ld", request.session_id());
  server->SendMessageToAuth(MSG_SS_REQUEST_REGISTE_ACCOUNT, &request);
  return ERR_OK;
}

int32_t ProcessResponseRegisteAccount(SSMessageEntry& entry) {
  MessageSSResponseRegisteAccount* message =
      static_cast<MessageSSResponseRegisteAccount*>(entry.get());
  if (!message) return ERR_INTERNAL;

  const TcpSessionPtr& session =
      TcpSessionManager::Instance().GetTcpSessionBySessionID(
          message->session_id());
  if (!session) {
    TRACE_LOG(logger)("%s SessionID:%ld not found", __PRETTY_FUNCTION__, message->session_id());
    return ERR_OK;
  }

  if (message->success()) {
    MessageResponseRegisteAccount response;
    response.set_account(message->account());
    response.set_password(message->password());

    server->SendMessageToClient(session.get(), MSG_CS_RESPONSE_REGISTE_ACCOUNT, &response);
    return ERR_OK;
  }

  MessageErrorCode msg;
  msg.set_err_code(ERR_TEST_ACCOUNT);
  msg.set_msg_id(MSG_CS_REQUEST_REGISTE_ACCOUNT);
  server->SendMessageToClient(session.get(), MSG_CS_ERROR_CODE, &msg);
  return ERR_OK;
}

int32_t ProcessRequestAccountLogin(CSMessageEntry& entry) {
  MessageRequestAccountLogin* message =
      static_cast<MessageRequestAccountLogin*>(entry.get());
  if (!message) return ERR_INTERNAL;

  if (message->account().empty() ||
      message->account().length() >= 64u)
    return ERR_PARAM_INVALID;
  if (message->password().empty() ||
      message->password().length() >= 64u)
    return ERR_PARAM_INVALID;

  MessageSSRequestAccountLogin request;
  request.set_account(message->account());
  request.set_password(message->password());
  request.set_session_id(entry.session_ptr->GetSessionID());

  server->SendMessageToAuth(MSG_SS_REQUEST_ACCOUNT_LOGIN, &request);
  return ERR_OK;
}

int32_t ProcessResponseAccountLogin(SSMessageEntry& entry) {
  MessageSSResponseAccountLogin* message =
      static_cast<MessageSSResponseAccountLogin*>(entry.get());
  if (!message) return ERR_INTERNAL;

  TcpSessionPtr session =
      TcpSessionManager::Instance().GetTcpSessionBySessionID(
          message->session_id());
  if (!session) {
    TRACE_LOG(logger)("%s SessionID:%ld not found", __PRETTY_FUNCTION__, message->session_id());
    return ERR_OK;
  }

  if (message->success()) {
    MessageResponseAccountLogin response;
    response.set_account(message->account());

    server->SendMessageToClient(session.get(), MSG_CS_RESPONSE_ACCOUNT_LOGIN, &response);
    return ERR_OK;
  }

  MessageErrorCode msg;
  msg.set_err_code(ERR_TEST_PASSWORD);
  msg.set_msg_id(MSG_CS_REQUEST_ACCOUNT_LOGIN);
  server->SendMessageToClient(session.get(), MSG_CS_ERROR_CODE, &msg);
  return ERR_OK;
}

int32_t ProcessResponseLoadArmy(SSMessageEntry& entry) {
  MessageSSResponseLoadArmy* message =
      static_cast<MessageSSResponseLoadArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;

  //army id = server_id * 100000 + sequece
  int64_t army_id = int64_t(server_config->server_id()) * 1000000;
  for (int32_t index = 0; index < message->info_size(); ++index) {
    server->AddArmy(message->info(index));
    if (message->info(index).army_id() >= army_id) {
      army_id = message->info(index).army_id();
    }
  }

  TRACE_LOG(logger)("Load ArmyCount:%d, ArmyID:%ld", message->info_size(), army_id);
  server->InitArmyID(army_id);
  server->set_init_state(7);

  Army* army = server->GetArmyByID(army_id);
  if (army) {
    army->LoadPearlHarbor();
    TRACE_LOG(logger)("Load amry_pearl_harbor army_id:%ld", army_id);
    if (!army->PearlHarborInfo().fresh_time()) server->RefreshPearlHarbor();
  }
  return ERR_OK;
}

int32_t ProcessGetSetServerStart(SSMessageEntry& entry) {
  MessageSSResponseGetServerStart* message =
      static_cast<MessageSSResponseGetServerStart*>(entry.get());
  if (!message) return ERR_INTERNAL;

  server->set_server_start_time(message->start_time());
  server->set_init_state(8);
  TRACE_LOG(logger)("ServerStartDays:%d", server->GetServerStartDays());

  server->ReInitServerStartActivity();
  server->ReInitWeeklyActivity();
  return ERR_OK;
}

int32_t ProcessResponseLoadArmyMember(SSMessageEntry& entry) {
  MessageSSReponseLoadArmyMember* message =
      static_cast<MessageSSReponseLoadArmyMember*>(entry.get());
  if (!message) return ERR_INTERNAL;

  for (int32_t index = 0; index < message->apply_size(); ++index) {
    const sy::ArmyApplyInfo& info = message->apply(index);
    Army* army = server->GetArmyByID(info.army_id());
    if (!army) {
      ERROR_LOG(logger)("LoadArmyApply, ArmyID:%ld not found", info.army_id());
      continue;
    }
    army->applies().push_back(info);
    server->AddArmyApply(info.player_id(), army->army_id());
  }

  for (int32_t index = 0; index < message->member_size(); ++index) {
    Army* army = server->GetArmyByID(message->member(index).army_id());
    if (!army) {
      ERROR_LOG(logger)("LoadArmyMember, ArmyID:%ld not found", message->member(index).army_id());
      continue;
    }
    army->members().push_back(message->member(index));
  }

  return ERR_OK;
}

int32_t ProcessResponseCreateArmy(SSMessageEntry& entry) {
  MessageSSResponseCreateArmy* message =
      static_cast<MessageSSResponseCreateArmy*>(entry.get());
  if (!message) return ERR_INTERNAL;
  int64_t player_id = message->master_id();
  LogicPlayer* player = server->GetPlayerByID(player_id);
  if (message->is_fail()) {
    if (!player) {
      ERROR_LOG(logger)("CreateArmy Fail, PlayerID:%ld, player not exist", player_id);
      return ERR_OK;
    }
    player->SendErrorCodeToClient(ERR_ARMY_NAME, MSG_CS_REQUEST_CREATE_ARMY);
    return ERR_OK;
  }
  sy::ArmyInfo& info = *message->mutable_info();

  //在LogicServer里面创建军团
  info.set_level(1);
  info.set_donate_time(GetVirtualSeconds());
  server->AddArmy(info);
  Army* army_info = server->GetArmyByID(info.army_id());
  if (!army_info) {
    ERROR_LOG(logger)("CreateArmy, InternalError");
    return ERR_INTERNAL;
  }

  //把自己插入到军团长的位子上面去
  sy::ArmyMemberInfo& member = *message->mutable_master();
  member.set_army_id(info.army_id());
  member.set_position(ARMY_POSITION_MASTER);
  if (player) {
    player->MakeArmyMemberInfo(&member);
    player->UpdateArmyMemberInfo(member);
  }
  army_info->members().push_back(member);

  if (player) {
    player->ProcessResponseCreateArmy(info);
  }
  server->OnPlayerJoinArmy(player->uid(), army_info);

  //更新玩家的军团
  MessageSSOnJoinArmy update_army;
  update_army.set_army_id(info.army_id());
  update_army.set_player_id(member.player_id());
  server->SendServerMessageToDB(MSG_SS_ON_JOIN_ARMY, &update_army);
  return ERR_OK;
}

static inline int32_t ProcessRechargeInternal(
    MessageSSUpdateRechargeDetails* message) {
  TRACE_LOG(logger)("LogicServer Recharge, OpenID:%s, PlayerID:%ld, GoodID:%s, Money:%d"
      , message->user_id().c_str(), message->role_id()
      , message->goods_id().c_str()
      , message->game_coin()
      );
  LogicPlayer* player = server->GetPlayerByID(message->role_id());
  if (!player) {
    ERROR_LOG(logger)("Recharge Fail, PlayerID:%ld not found", message->role_id());
    //发送返回消息
    message->set_result(ERR_PLAYER_NOT_EXIST);
    server->SendMessageToCenter(MSG_SS_UPDATE_RECHARGE_DETAILS, message);
    return ERR_OK;
  }

  int32_t game_coin = message->game_coin();
  std::string goods_id = message->goods_id();
  if (player->player().openid().find("DEBUG_") == 0 && !server_config->test_gm()) {
    game_coin = 0;
    goods_id = "test";
  }

  goods_id = GetMappedGoodID(goods_id);
  int32_t result =
      player->AddRecharge(message->request_time(), goods_id, game_coin, true,
                          MSG_SS_UPDATE_RECHARGE_DETAILS);
  if (result) {
    ERROR_LOG(logger)("Recharge Fail, PlayerID:%ld, ErrorCode:%d", message->role_id(), result);
    return ERR_CURRENCY_MONEY;
  }

  MessageNotifyRechargeDetails notify;
  notify.set_timestamp(message->timestamp());
  notify.set_goods_type(message->goods_type());
  notify.set_device_type(message->device_type());
  notify.set_role_id(message->role_id());
  notify.set_user_id(message->user_id());
  notify.set_goods_id(message->order_id());
  notify.set_game_coin(message->game_coin());
  notify.set_server_id(message->server_id());
  notify.set_channel_id(message->channel_id());
  notify.set_pay_amount(message->pay_amount());
  notify.set_currency_code(message->currency_code());
  player->SendMessageToClient(MSG_CS_NOTIFY_RECHARGE_DETAILS, &notify);

  server->SendMessageToCenter(MSG_SS_UPDATE_RECHARGE_DETAILS, message);
  message->set_tid(server->GetTID());
  server->SendServerMessageToDB(MSG_SS_UPDATE_RECHARGE_DETAILS, message);

  MessageSSUpdateFirstRechargeInfo msg;
  msg.set_recharge_time(GetSeconds());
  msg.set_money(game_coin * 10);
  msg.set_openid(message->user_id());
  msg.set_player_id(message->role_id());
  msg.set_server_id(server_config->server_id());
  server->SendServerMessageToDB(MSG_SS_UPDATE_FIRST_RECHAGE_INFO, &msg);
  return ERR_OK;
}

int32_t ProcessRequestRecharge(SSMessageEntry& entry) {
  MessageSSUpdateRechargeDetails* message =
      static_cast<MessageSSUpdateRechargeDetails*>(entry.get());
  if (!message) return ERR_INTERNAL;
  if (ProcessRechargeInternal(message)) {
    //充值失败,发送失败消息回去
    message->set_result(ERR_CURRENCY_MONEY);
    server->SendMessageToCenter(MSG_SS_UPDATE_RECHARGE_DETAILS, message);
  }
  return ERR_OK;
}

int32_t ProcessSetPlayerGuide(SSMessageEntry& entry) {
  MessageSSServerSetDialog* message =
      static_cast<MessageSSServerSetDialog*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicPlayer* player = server->GetPlayerByID(message->player_id());
  if (player) player->SetDialog(message->guide());

  server->SendServerMessageToDB(MSG_SS_SERVER_SET_DIALOG, message);

  return ERR_OK;
}

int32_t ProcessClearServerData(SSMessageEntry& entry) {
  MessageSSServerClearData* message =
      static_cast<MessageSSServerClearData*>(entry.get());
  if (!message) return ERR_INTERNAL;

  return ERR_OK;
}

int32_t ProcessResponseLoadTimeActivityNew(SSMessageEntry& entry) {
  MessageResponseLoadTimeActivityNew* message =
      static_cast<MessageResponseLoadTimeActivityNew*>(entry.get());
  if (!message) return ERR_INTERNAL;

  ACTIVITY.ClearGMActivity();
  for (int32_t i = 0; i < message->activitis_size(); i++) {
    const TimeActivityInfo& info = message->activitis(i);
    ACTIVITY.AddActivityTemplate(info);
  }

  server->ReInitServerStartActivity();
  server->ReInitWeeklyActivity();

  server->set_init_state(9);
  return ERR_OK;
}

extern void SignalUsr1(int);

int32_t ProcessServerTimeActivity(SSMessageEntry& entry) {
  SignalUsr1(1);
  return ERR_OK;
}

int32_t ProcessResponseQueryOtherPlayer(SSMessageEntry& entry) {
  intranet::MessageSSResponseQueryOtherPlayer* message =
      static_cast<intranet::MessageSSResponseQueryOtherPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LogicPlayer* player = server->GetPlayerByID(message->request_player_id());
  if (player) player->DispatchQueryPlayerMessage(*message);

  return ERR_OK;
}

int32_t ProcessResponseLegionWarRegister(SSMessageEntry& entry) {
  MessageSSResponseLegionNewPlayer* message =
      static_cast<MessageSSResponseLegionNewPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  LegionWar::Instance().NewPlayer(
      *message->mutable_player(),
      std::make_pair(message->city_id(), message->position()));
  return ERR_OK;
}

int32_t ProcessResponseUpdateLegionWarPlayerPos(SSMessageEntry& entry) {
  MessageSSUpdateLegionWarPos* message =
      static_cast<MessageSSUpdateLegionWarPos*>(entry.get());
  if (!message) return ERR_INTERNAL;
  LegionWar::Instance().SwapPos(
      std::make_pair(message->city_1(), message->position_1()),
      std::make_pair(message->city_2(), message->position_2()));
  return ERR_OK;
}

int32_t ProcessResponseCrossServerRankList(SSMessageEntry& entry) {
  MessageSSUpdateCrossServerRankList* message =
      static_cast<MessageSSUpdateCrossServerRankList*>(entry.get());
  if (!message) return ERR_INTERNAL;

  DEBUG_LOG(logger)("CrossServerRankList, RankType:%d, Count:%d"
      , message->rank_type(), message->info_size());

  if (message->rank_type() >= RANK_TYPE_CROSS_SERVER_UK &&
      message->rank_type() <= RANK_TYPE_CROSS_SERVER_JP) {
    for (int32_t i = 0; i < message->info_size(); i++)
      RANK_LIST.OnCrossServerFight(*message->mutable_info(i));
  }

  if (message->rank_type() >= RANK_TYPE_LEGION_WAR_2 &&
      message->rank_type() <= RANK_TYPE_LEGION_WAR_3) {
    for (int32_t i = 0; i < message->info_size(); i++)
      RANK_LIST.OnLegionWar(message->rank_type(), *message->mutable_info(i));
  }

  if (message->rank_type() == RANK_TYPE_MEDAL_CROSS_SERVER) {
    for (int32_t i = 0; i < message->info_size(); i++)
      RANK_LIST.OnMedalCrossServerRank(*message->mutable_info(i));
  }

  return ERR_OK;
}

int32_t ProcessUpdateLegionWarPlayerInfo(SSMessageEntry& entry) {
  MessageSSUpdateLegionWarPlayer* message =
      static_cast<MessageSSUpdateLegionWarPlayer*>(entry.get());
  if (!message) return ERR_INTERNAL;

  sy::OtherPlayerInfo* player = LegionWar::Instance().GetPlayerByID(message->player_id());
  if (!player) return ERR_OK;
  if (message->army_id()){
    player->set_army_id(message->army_id());
    player->set_army_name(message->army_name());
    player->set_army_avatar(message->army_avatar());
  } else {
    player->clear_army_id();
    player->clear_army_avatar();
    player->clear_army_name();
  }
  sy::OtherPlayerInfo p(*player);
  LegionWar::Instance().SavePlayer(p);

  MessageNotifyLegionWarPlayer notify;
  notify.mutable_player()->CopyFrom(p);
  LegionWar::Instance().SendMessageToAll(MSG_CS_NOTIFY_LEGION_WAR_PLAYER, &notify);
  return ERR_OK;
}

int32_t ProcessResponseQueryLivelyAccount(SSMessageEntry& entry) {
  MessageSSResponseQueryLivelyAccount* message =
      static_cast<MessageSSResponseQueryLivelyAccount*>(entry.get());
  if (!message) return ERR_INTERNAL;
  MessageSSRequestLoadMultiPlayer request;
  request.set_forward_player(0);
  request.set_msg_id(MSG_CS_REQUEST_GET_OTHER_PLAYER);

  for (int32_t index = 0; index < message->players_size(); ++index) {
    LogicPlayer* player = server->GetOrNewPlayer(message->players(index));
    (void)player;
    request.add_player_ids(message->players(index));
  }

  server->SendServerMessageToDB(MSG_SS_REQUEST_LOAD_MULTI_PLAYER, &request);
  return ERR_OK;
}

int32_t ProcessRequestServerHeartBeat(SSMessageEntry& entry) {
  server->LastHeartBeat();
  return ERR_OK;
}

int32_t ProcessResponseLoadAllName(SSMessageEntry& entry) {
  MessageSSResponseLoadAllName* message =
      static_cast<MessageSSResponseLoadAllName*>(entry.get());
  if (!message) return ERR_INTERNAL;
  TRACE_LOG(logger)("Load %d Names", message->name_size());
  for (int32_t i = 0; i < message->name_size(); ++i) {
    server->AddNewName(message->name(i), 1);
  }
  server->set_init_state(4);
  return ERR_OK;
}

const std::string& PRIVATE_KEY = "TmV3rHKyguQDYh9Ru4wzz8EhtI4N*zdh";

static inline std::string CalcMd5(const std::string& s) {
  MD5 md5(s.c_str(), s.size());
  return md5.str();
}

typedef int32_t (*GmCommandCallback)(
    VectorMap<std::string, std::string>& params, std::vector<std::string>& output);

#define IGNORE_SERVER                                                                   \
  TRACE_LOG(logger)("%s, DestServerID:%s", __FUNCTION__, params["server_id"].c_str());  \
  if (std::find(server_config->server_ids().begin(),                                    \
                server_config->server_ids().end(),                                      \
                (uint32_t)(atoi(params["server_id"].c_str()))) ==                       \
      server_config->server_ids().end()) {                                              \
    TRACE_LOG(logger)("%s, DestServerID:%s, Ignored", __FUNCTION__, params["server_id"].c_str()); \
    return ERR_OK;                                                                      \
  }

static inline int32_t ProcessGmCommandRecharge(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  MessageSSUpdateRechargeDetails msg;
  msg.set_order_id(params["order_id"]);
  msg.set_user_id(params["user_id"]);
  msg.set_device_type(atoi(params["device_type"].c_str()));
  msg.set_pay_amount(atof(params["pay_amount"].c_str()) * 100);
  msg.set_currency_code(params["currency_code"]);
  msg.set_channel_id(params["channel_id"]);
  msg.set_server_id(atoll(params["server_id"].c_str()));
  msg.set_role_id(atoll(params["role_id"].c_str()));
  msg.set_goods_type(atoi(params["goods_type"].c_str()));
  msg.set_game_coin(atoi(params["game_coin"].c_str()));
  msg.set_goods_id(params["goods_id"]);
  msg.set_timestamp(params["timestamp"]);
  msg.set_request_time(GetSeconds());

  return ProcessRechargeInternal(&msg);
}

static inline int32_t ProcessGmCommandKickUser(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {
  MessageSSRequestKickUser message;

  IGNORE_SERVER

  std::vector<LogicPlayer*> players;
  if (atoi(params["all"].c_str())) {
    KickAllPlayers for_each(players);
    server->Players().ForEach(for_each);
  } else {
    std::vector<std::string> players_id;
    SplitString(params["players"], players_id, ",");
    for (size_t i = 0; i < players_id.size(); ++i) {
      LogicPlayer* player = server->GetPlayerByID(atoll(players_id[i].c_str()));
      if (player) {
        players.push_back(player);
      }
    }
  }
  for (std::vector<LogicPlayer*>::iterator iter = players.begin();
       iter != players.end(); ++iter) {
    const TcpSessionPtr& ptr = (*iter)->session().lock();
    if (ptr) ptr->Close();
    TRACE_LOG(logger)("GM KickPlayer:%ld", (*iter)->uid());
    server->ErasePlayer((*iter)->uid());
  }

  server->SendServerMessageToDB(MSG_SS_REQUEST_KICK_USER, &message);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandSetGuide(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER

  MessageSSServerSetDialog message;
  message.set_guide(params["guide"]);
  message.set_player_id(atoll(params["player_id"].c_str()));

  LogicPlayer* player = server->GetPlayerByID(message.player_id());
  if (player) player->SetDialog(message.guide());

  server->SendServerMessageToDB(MSG_SS_SERVER_SET_DIALOG, &message);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandSetBlackList(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER


  int32_t type = atoi(params["type"].c_str());
  if (!(type == 1 || type == 2)) return ERR_OK;

  const std::string& ip_list = params["ip_list"];
  std::vector<std::string> list;
  SplitString(ip_list, list, ",");

  MessageSSSetServerIpList message;
  message.set_server_id(server_config->server_id());
  if (type == 1) message.set_black_list(ip_list);
  if (type == 2) message.set_white_list(ip_list);

  boost::unordered_set<std::string>* set = NULL;
  if (type == 1) set = &server->black_list();
  if (type == 2) set = &server->white_list();
  if (set) {
    set->clear();
    for (std::vector<std::string>::const_iterator iter = list.begin();
         iter != list.end(); ++iter) {
      set->insert(*iter);
      if (type ==1) INFO_LOG(logger)("BlackList, %s", iter->c_str());
      if (type ==2) INFO_LOG(logger)("WhiteList, %s", iter->c_str());
    }
  }

  server->SendServerMessageToDB(MSG_SS_UPDATE_SERVER_IP_LIST, &message);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandReloadConfig(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {
  SignalUsr1(1);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandBanUser(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER

  MessageSSSetAccountStatus message;
  message.set_status_time(atoll(params["end_time"].c_str()));
  message.set_status(atoi(params["is_ban"].c_str()));
  message.set_player_id(atoll(params["player_id"].c_str()));

  LogicPlayer *player = server->GetPlayerByID(message.player_id());
  if (player) {
    if (message.has_status()) player->player().set_status(message.status());
    if (message.has_status_time()) player->player().set_status_time(message.status_time());
    //判断玩家是否需要被T下线
    player->CheckBanToLogin();
  }

  if (message.status() == 2) RANK_LIST.RemoveAll(message.player_id());

  server->SendServerMessageToDB(MSG_SS_SET_ACCOUNT_STATUS, &message);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandDeleteNotice(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER

  MessageSSServerNotice message;
  int64_t delete_id = atoll(params["notice_id"].c_str());
  message.set_delete_id(delete_id);

  if (delete_id) {
    server->DelNotice(delete_id);
    MessageNotifyNotice msg;
    msg.add_delete_notices(delete_id);
    server->SendMessageToAllClient(MSG_CS_NOTIFY_NOTICE, &msg);
  }

  server->SendServerMessageToDB(MSG_SS_SERVER_NOTICE, &message);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandAddNotice(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER

  MessageSSServerNotice message;
  uint32_t server_id = server_config->server_id();
  int64_t id = atoll(params["notice_id"].c_str());
  int32_t type = atoi(params["type"].c_str());
  int32_t order = atoi(params["order"].c_str());
  const std::string& url = params["url"];
  int64_t begin_time = atoll(params["start_time"].c_str());
  int64_t end_time = atoll(params["end_time"].c_str());
  int32_t exec_time = atoi(params["exec_time"].c_str()) * 60;
  const std::string& text = params["text"];

  NoticeInfo* info = message.mutable_notice();
  info->set_tid(id);
  info->set_server_id(server_id);
  info->set_type(type);
  info->set_order(order);
  info->set_link_url(url);
  info->set_begin_time(begin_time);
  info->set_end_time(end_time);
  info->set_interval(exec_time);
  info->set_content(text);

  if (info->end_time() > GetSeconds() && info->tid()) {
    TRACE_LOG(logger)("ServerNotice, ID:%ld", info->tid());
    server->AddNotice(*info);
    MessageNotifyNotice msg;
    NoticeInfo* notice = msg.add_notices();
    notice->CopyFrom(*info);
    server->SendMessageToAllClient(MSG_CS_NOTIFY_NOTICE, &msg);
  }

  server->SendServerMessageToDB(MSG_SS_SERVER_NOTICE, &message);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandFreshGlobalConfig(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  const std::string& str = params["config"];
  MessageNotifyGlobalConfig msg;
  msg.set_config_str(str);
  server->SendMessageToAllClient(MSG_CS_NOTIFY_GLOBAL_CONFIG, &msg);
  return ERR_OK;
}

static inline int32_t ProcessGmCommandSendMail(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER

  int32_t all = atoi(params["all"].c_str());
  int32_t level_min = params["level_min"].empty() ? -1 : atoi(params["level_min"].c_str());
  int32_t level_max = params["level_max"].empty() ? -1 : atoi(params["level_max"].c_str());
  int32_t vip_min = params["vip_level_min"].empty() ? -1 : atoi(params["vip_level_min"].c_str());
  int32_t vip_max = params["vip_level_max"].empty() ? -1 : atoi(params["vip_level_max"].c_str());
  const std::string& player_str = params["players"];
  const std::string& content = params["mail_content"];
  const std::string& reward = params["mail_reward"];

  sy::MailInfo mail;
  mail.set_mail_id(server->GetNewMailID());
  mail.set_mail_content(content);
  mail.set_mail_time(GetSeconds());
  mail.set_mail_type(MAIL_TYPE_SYS);
  mail.set_level_min(level_min);
  mail.set_level_max(level_max);
  mail.set_vip_min(vip_min);
  mail.set_vip_max(vip_max);
  std::vector<std::string> items;
  SplitString(reward, items, ";");
  for (std::vector<std::string>::iterator iter = items.begin();
       iter != items.end(); ++iter) {
    int32_t item_id = 0, item_count = 0;
    if (sscanf(iter->c_str(), "%d:%d", &item_id, &item_count) >= 2) {
      sy::KVPair2* info = mail.add_mail_attachment();
      info->set_key(item_id);
      info->set_value(item_count);
    }
  }

  if (all){
    server->server_mail().add_mails()->CopyFrom(mail);
    MessageSSAddServerMail msg;
    msg.mutable_mail()->CopyFrom(mail);
    msg.set_server_id(server_config->server_id());
    server->SendServerMessageToDB(MSG_SS_ADD_SERVER_MAIL, &msg);
    TRACE_LOG(logger)("Send ServerMail:%ld, Content:%s"
      , mail.mail_id(), mail.mail_content().c_str());
  } else {
    std::vector<std::string> players;
    SplitString(player_str, players, ",");

    std::vector<std::pair<int32_t, int32_t> > item_reward;
    for (int32_t reward_index = 0; reward_index < mail.mail_attachment_size();
         ++reward_index) {
      item_reward.push_back(
          std::make_pair(mail.mail_attachment(reward_index).key(),
                         mail.mail_attachment(reward_index).value()));
    }
    TRACE_LOG(logger)("Send Mail To MultiPlayer, MailID:%ld", mail.mail_id());
    for (std::vector<std::string>::iterator iter = players.begin();
         iter != players.end(); ++iter) {
      int64_t player_id = atoll(iter->c_str());
      TRACE_LOG(logger)("PlayerID:%ld", player_id);
      LogicPlayer::SendMail(player_id, mail.mail_time(), mail.mail_type(),
                            mail.mail_content(), &item_reward);
    }
  }

  return ERR_OK;
}

static inline int32_t ProcessGmCommandEditUser(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER

  int64_t player_id = atoll(params["player_id"].c_str());
  int32_t vip_level = params["vip_level"].empty() ? -1 :atoi(params["vip_level"].c_str());
  int32_t level = atoi(params["level"].c_str());
  const std::string& name = params["name"];

  MessageSSRequestAddGood msg;
  msg.add_player_uid(player_id);

  GMContent* content = msg.mutable_content();
  if (name.length()) content->set_player_name(name);
  if (level > 0) content->set_level(level);
  if (vip_level >= 0) content->set_vip_level(vip_level);
  return ProcessAddGoodsInternal(&msg, 0);
}

static inline int32_t ProcessGmCommandAddItems(
    VectorMap<std::string, std::string>& params,
    std::vector<std::string>& output) {

  IGNORE_SERVER

  int64_t player_id = atoll(params["player_id"].c_str());
  const std::string& items_str = params["items"];

  MessageSSRequestAddGood msg;
  msg.add_player_uid(player_id);

  GMContent* content = msg.mutable_content();
  std::vector<std::string> items;
  SplitString(items_str, items, ";");
  for (std::vector<std::string>::iterator iter = items.begin();
       iter != items.end(); ++iter) {
    int32_t item_id = 0, item_count = 0;
    if (sscanf(iter->c_str(), "%d:%d", &item_id, &item_count) >= 2) {
      sy::KVPair2* pair =
          item_id < 100 ? content->add_modify() : content->add_add_sub_item();
      pair->set_key(item_id);
      pair->set_value(item_count);
    }
  }

  return ProcessAddGoodsInternal(&msg, 0);
}

static std::pair<const char*, GmCommandCallback> kGmCommandCallback[] = {
    std::pair<const char*, GmCommandCallback>("recharge", ProcessGmCommandRecharge),
    std::pair<const char*, GmCommandCallback>("kickUser", ProcessGmCommandKickUser),
    std::pair<const char*, GmCommandCallback>("setGuide", ProcessGmCommandSetGuide),
    std::pair<const char*, GmCommandCallback>("blackList", ProcessGmCommandSetBlackList),
    std::pair<const char*, GmCommandCallback>("banUser", ProcessGmCommandBanUser),
    std::pair<const char*, GmCommandCallback>("delNotice", ProcessGmCommandDeleteNotice),
    std::pair<const char*, GmCommandCallback>("addNotice", ProcessGmCommandAddNotice),
    std::pair<const char*, GmCommandCallback>("reloadConfig", ProcessGmCommandReloadConfig),
    std::pair<const char*, GmCommandCallback>("freshGlobalConfig", ProcessGmCommandFreshGlobalConfig),
    std::pair<const char*, GmCommandCallback>("sendMail", ProcessGmCommandSendMail),
    std::pair<const char*, GmCommandCallback>("editUser", ProcessGmCommandEditUser),
    std::pair<const char*, GmCommandCallback>("addItem", ProcessGmCommandAddItems),
};

int32_t ProcessProcessGmCommand(CSMessageEntry& entry) {
  MessageRequestGMCommand* message =
      static_cast<MessageRequestGMCommand*>(entry.get());
  int32_t result = ERR_OK;
  VectorMap<std::string, std::string> params;
  std::vector<std::string> output;

  do {
    if (!message) {
      result = ERR_INTERNAL;
      break;
    }
    ArrayStream<128 * 1024> stream;
    stream.Append(message->action());
    for (int32_t i = 0; i < message->param_size(); ++i) {
      params[message->param(i).key()] = message->param(i).value();
      stream.Append(message->param(i).key());
      stream.Append(message->param(i).value());
    }

    stream.Append(PRIVATE_KEY);
    const std::string& md5 = CalcMd5(stream.str());
    if (message->sign() != md5) {
      ERROR_LOG(logger)("Sign Not Equal, %s, %s", message->sign().c_str(), md5.c_str());
      result = ERR_SIGN;
      break;
    }
    bool found = false;
    for (int32_t i = 0; i < ArraySize(kGmCommandCallback); ++i) {
      std::pair<const char*, GmCommandCallback> pair = kGmCommandCallback[i];
      if (!strcmp(pair.first, message->action().c_str()) && pair.second) {
        found = true;
        result = (*pair.second)(params, output);
        break;
      }
    }
    result = !found ? ERR_NOT_PROCESS : result;
  } while (false);

  MessageResponseGMCommand response;
  response.set_result(result);
  for (std::vector<std::string>::const_iterator iter = output.begin();
       iter != output.end(); ++iter) {
    *response.add_param() = *iter;
  }
  server->SendMessageToClient(entry.session_ptr.get(), MSG_CS_RESPONSE_GM_COMMAND, &response);
  return ERR_OK;
}

int32_t ProcessProcessNotifyGlobalConfig(SSMessageEntry& entry) {
  MessageSSNotifyGlobalConfig* message = static_cast<MessageSSNotifyGlobalConfig*>(entry.get());
  if (!message) return ERR_INTERNAL;

  MessageNotifyGlobalConfig msg;
  msg.set_config_str(message->global_str());
  server->SendMessageToAllClient(MSG_CS_NOTIFY_GLOBAL_CONFIG, &msg);
  return ERR_OK;
}
