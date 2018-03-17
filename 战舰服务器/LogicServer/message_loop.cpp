#include <net/MessageQueue.h>
#include <cpp/message.pb.h>
#include <cpp/server_message.pb.h>
#include <google/protobuf/text_format.h>
#include <system.h>
#include "logic_player.h"
#include "server.h"
#include "message_handler.h"
#include <net/MessageCache.h>
#include <storage.h>

using namespace sy;
using namespace intranet;

//注册消息回调
void Server::InitMessageHandler() {
  //客户端发来的服务器消息
  this->cs_server_handler_[MSG_CS_REQUEST_LOGIN_C] = ProcessRequestPlayerLogin;
  this->cs_server_handler_[MSG_CS_REPORT_TOKEN_C] = ProcessRequestReportToken;
  this->cs_server_handler_[MSG_CS_REQUEST_HEART_BEAT] = ProcessRequestHeartBeat;
  this->cs_server_handler_[MSG_CS_REQUEST_LOGOUT] = ProcessSkipThisMessage;
  this->cs_server_handler_[MSG_CS_REQUEST_REGISTE_ACCOUNT] = ProcessRequestRegisteAccount;
  this->cs_server_handler_[MSG_CS_REQUEST_ACCOUNT_LOGIN] = ProcessRequestAccountLogin;
  this->cs_server_handler_[MSG_CS_REQUEST_GM_COMMAND] = ProcessProcessGmCommand;

  //服务器发来的服务器消息
  this->ss_server_handler_[MSG_SS_RESPONSE_GET_UID] = ProcessResponsePlayerLogin;
  this->ss_server_handler_[MSG_SS_SEND_MAIL] = ProcessResponseNewMail;
  this->ss_server_handler_[MSG_SS_RESPONSE_GET_PK_RANK_LIST] = ProcessResponseGetPKRankList;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_MULTI_PLAYER] = ProcessResponseGetOtherPlayerInfo;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_RANK_LIST] = ProcessResponseLoadRankList;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_BOSS_LIST] = ProcessResponseLoadBossList;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_SERVER_SHOP] = ProcessResponseLoadServerShop;
  this->ss_server_handler_[MSG_SS_ADD_SERVER_MAIL] = ProcessRequestAddServerMail;
  this->ss_server_handler_[MSG_SS_SEND_MAIL_TO_MULTI] = ProcessRequestSendMailToMulti;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_IP_LIST] = ProcessResponseLoadIPList;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_NOTICE] = ProcessResponseLoadNotice;
  this->ss_server_handler_[MSG_SS_RESPONSE_REGISTE_ACCOUNT] = ProcessResponseRegisteAccount;
  this->ss_server_handler_[MSG_SS_RESPONSE_ACCOUNT_LOGIN] = ProcessResponseAccountLogin;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_ARMY] = ProcessResponseLoadArmy;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_ARMY_MEMBER] = ProcessResponseLoadArmyMember;
  this->ss_server_handler_[MSG_SS_RESPONSE_CREATE_ARMY] = ProcessResponseCreateArmy;
  this->ss_server_handler_[MSG_SS_UPDATE_RECHARGE_DETAILS] = ProcessRequestRecharge;
  this->ss_server_handler_[MSG_SS_RESPONSE_GET_SERVER_START] = ProcessGetSetServerStart;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_TIME_ACTIVITY_NEW] = ProcessResponseLoadTimeActivityNew;
  this->ss_server_handler_[MSG_SS_UPDATE_LEGION_WAR_POS] = ProcessResponseUpdateLegionWarPlayerPos;
  this->ss_server_handler_[MSG_SS_RESPONSE_LEGION_WAR_NEW_PLAYER] = ProcessResponseLegionWarRegister;
  this->ss_server_handler_[MSG_SS_UPDATE_CROSS_SERVER_RANK_LIST] = ProcessResponseCrossServerRankList;
  this->ss_server_handler_[MSG_SS_UPDATE_LEGION_WAR_PLAYER] = ProcessUpdateLegionWarPlayerInfo;
  this->ss_server_handler_[MSG_SS_RESPONSE_QUERY_LIVELY_ACCOUNT] = ProcessResponseQueryLivelyAccount;
  this->ss_server_handler_[MSG_SS_SERVER_HEART_BEAT] = ProcessRequestServerHeartBeat;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_ALL_NAME] = ProcessResponseLoadAllName;
  this->ss_server_handler_[MSG_SS_UPDATE_LEGION_WAR_PLAYER] = ProcessUpdateLegionWarPlayerInfo;
  this->ss_server_handler_[MSG_SS_NOTIFY_GLOBAL_CONFIG] = ProcessProcessNotifyGlobalConfig;

  //center server消息
  this->ss_server_handler_[MSG_SS_REQUEST_KICK_USER] = ProcessRequestKickUser;
  this->ss_server_handler_[MSG_SS_SET_ACCOUNT_STATUS] = ProcessSetAccountStatus;
  this->ss_server_handler_[MSG_SS_REQUEST_ADD_GOODS] = ProcessAddGoods;
  this->ss_server_handler_[MSG_SS_SET_IP_LIST] = ProcessRequestSetIpList;
  this->ss_server_handler_[MSG_SS_SERVER_NOTICE] = ProcessSetNotice;
  this->ss_server_handler_[MSG_SS_RESPONSE_LOAD_SERVER_MAIL] = ProcessResponseLoadServerMail;
  this->ss_server_handler_[MSG_SS_SERVER_SET_DIALOG] = ProcessSetPlayerGuide;
  this->ss_server_handler_[MSG_SS_SERVER_CLEAR_DATA] = ProcessClearServerData;
  this->ss_server_handler_[MSG_SS_SREVER_TIME_ACTIVITY] = ProcessServerTimeActivity;
  this->ss_server_handler_[MSG_SS_RESPONSE_QUERY_OTHER_PLAYER] = ProcessResponseQueryOtherPlayer;

  //客户端发来的玩家消息
  this->cs_player_handler_[MSG_CS_REQUEST_TEST] = &LogicPlayer::ProcessRequestTest;
  this->cs_player_handler_[MSG_CS_REPORT_TOKEN_C] = &LogicPlayer::ProcessIgnorePlayerMessage;
  this->cs_player_handler_[MSG_CS_REQUEST_HEART_BEAT] = &LogicPlayer::ProcessRequestHeartBeat;
  this->cs_player_handler_[MSG_CS_REQUEST_CREATE_PLAYER] = &LogicPlayer::ProcessRequestCreatePlayer;
  this->cs_player_handler_[MSG_CS_REQUEST_CHANGE_NAME] = &LogicPlayer::ProcessRequestChangeName;
  this->cs_player_handler_[MSG_CS_REQUEST_SET_TACTIC] = &LogicPlayer::ProcessRequestSetTacticInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_ACTIVE_CARRIER] = &LogicPlayer::ProcessRequestActiveCarrier;
  this->cs_player_handler_[MSG_CS_REQUEST_RESEARCH_HERO] = &LogicPlayer::ProcessRequestResearchHero;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_RESEARCH_HERO] = &LogicPlayer::ProcessRequestGetResearchHero;
  this->cs_player_handler_[MSG_CS_REQUEST_EQUIP_LEVEL_UP] = &LogicPlayer::ProcessRequestEquipLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_EQUIP_REFINE] = &LogicPlayer::ProcessRequestEquipRefine;
  this->cs_player_handler_[MSG_CS_REQUEST_HERO_LEVEL_UP] = &LogicPlayer::ProcessRequestHeroLevelUp;
  //this->cs_player_handler_[MSG_CS_REQUEST_CLEAR_CD] = &LogicPlayer::ProcessRequestClearCD;
  this->cs_player_handler_[MSG_CS_REQUEST_HERO_GRADE_LEVEL_UP] = &LogicPlayer::ProcessRequestHeroGradeLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_FLOP] = &LogicPlayer::ProcessRequestFlop;
  this->cs_player_handler_[MSG_CS_REQUEST_EQUIP_ITEM] = &LogicPlayer::ProcessRequestEquipItem;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_COPY_AWARD] = &LogicPlayer::ProcessRequestGetCopyAward;
  this->cs_player_handler_[MSG_CS_REQUEST_SAVE_RANDOM_ATTR] = &LogicPlayer::ProcessRequestSaveRandomAttr;
  this->cs_player_handler_[MSG_CS_REQUEST_HERO_RANDOM_ATTR] = &LogicPlayer::ProcessRequestHeroRandomAttr;
  this->cs_player_handler_[MSG_CS_REQUEST_COMPOSE_NAVY] = &LogicPlayer::ProcessRequestComposeNavy;
  this->cs_player_handler_[MSG_CS_REQUEST_NAVY_LEVEL_UP] = &LogicPlayer::ProcessRequestNavyLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_NAVY_REFINE] = &LogicPlayer::ProcessRequestNavyRefine;
  this->cs_player_handler_[MSG_CS_REQUEST_COMPOSE_EQUIP] = &LogicPlayer::ProcessRequestComposeEquip;
  this->cs_player_handler_[MSG_CS_REQUEST_SELL_ITEM] = &LogicPlayer::ProcessRequestSellItem;
  this->cs_player_handler_[MSG_CS_REQUEST_SELL_SHIP] = &LogicPlayer::ProcessRequestSellShip;
  this->cs_player_handler_[MSG_CS_REQUEST_BUY_ITEM] = &LogicPlayer::ProcessRequestBuyItem;
  this->cs_player_handler_[MSG_CS_REQUEST_REFRESH_FEATS] = &LogicPlayer::ProcessRequestRefreshFeats;
  this->cs_player_handler_[MSG_CS_REQUEST_SET_MAIL_ID] = &LogicPlayer::ProcessRequestUpdateMailID;
  this->cs_player_handler_[MSG_CS_REQUEST_SEND_MAIL] = &LogicPlayer::ProcessRequestSendMail;
  this->cs_player_handler_[MSG_CS_REQUEST_FATE_LEVEL_UP] = &LogicPlayer::ProcessRequestFateLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_OTHER_PLAYER] = &LogicPlayer::ProcessRequestGetOtherPlayerInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_PK_RANK_LIST] = &LogicPlayer::ProcessRequestGetPKRankList;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_MY_PK_RANK_INFO] = &LogicPlayer::ProcessRequestGetMyPkRankInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_REFRESH_PK_TARGETS] = &LogicPlayer::ProcessRequestRefreshPKTargets;
  this->cs_player_handler_[MSG_CS_REQUEST_TEST_PK_TARGET] = &LogicPlayer::ProcessRequestTestPkTarget;
  this->cs_player_handler_[MSG_CS_REQUEST_START_PATROL] = &LogicPlayer::ProcessRequestStartPatrol;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_PATROL_AWARDS] = &LogicPlayer::ProcessRequestGetPatrolAwards;
  this->cs_player_handler_[MSG_CS_REQUEST_PATROL_LEVEL_UP] = &LogicPlayer::ProcessRequestPatrolLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_BUY_COUNT] = &LogicPlayer::ProcessRequestBuyCount;
  this->cs_player_handler_[MSG_CS_REQUEST_TALK] = &LogicPlayer::ProcessRequestTalk;
  this->cs_player_handler_[MSG_CS_REQUEST_LOGOUT] = &LogicPlayer::ProcessRequestLogOut;
  this->cs_player_handler_[MSG_CS_REQUEST_FIGHT_REPORT] = &LogicPlayer::ProcessRequestGetReprot;
  this->cs_player_handler_[MSG_CS_REQUEST_RECOVER] = &LogicPlayer::ProcessRequestRecover;
  this->cs_player_handler_[MSG_CS_REQUEST_TOWER_BUY_BUFF] = &LogicPlayer::ProcessRequestTowerBuyBuff;
  this->cs_player_handler_[MSG_CS_REQUEST_TOWER_AWARD] = &LogicPlayer::ProcessRequestTowerAward;
  this->cs_player_handler_[MSG_CS_REQUEST_TOWER_BUY_BOX] = &LogicPlayer::ProcessRequestTowerBuyBox;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_UID_BY_NAME] = &LogicPlayer::ProcessRequestGetUIDByName;
  this->cs_player_handler_[MSG_CS_REQUEST_COPY_SWEEP] = &LogicPlayer::ProcessRequestCopySweep;
  this->cs_player_handler_[MSG_CS_REQUEST_USE_TRUCE] = &LogicPlayer::ProcessRequestUseTruce;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ROB_OPPONENT] = &LogicPlayer::ProcessRequestGetRobOpponent;
  this->cs_player_handler_[MSG_CS_REQUEST_ADD_FRIEND] = &LogicPlayer::ProcessRequestAddFriend;
  this->cs_player_handler_[MSG_CS_REQUEST_FRIEND_ENERGY] = &LogicPlayer::ProcessRequestFriendEnergy;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_FRIEND_ENERGY] = &LogicPlayer::ProcessRequestGetFriendEnergy;
  this->cs_player_handler_[MSG_CS_REQUEST_AGREE_ADD_FRIEND] = &LogicPlayer::ProcessRequestAgreeAddFriend;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_FRIEND_INFO] = &LogicPlayer::ProcessRequestGetFriendInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_DSTRIKE_LIST] = &LogicPlayer::ProcessRequestDstrikeList;
  this->cs_player_handler_[MSG_CS_REQUEST_DSTRIKE_FIGHT] = &LogicPlayer::ProcessRequestDstrikePreFight;
  this->cs_player_handler_[MSG_CS_REQUEST_DSTRIKE_SHARE] = &LogicPlayer::ProcessRequestDstrikeShare;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_RANK_LIST] = &LogicPlayer::ProcessRequestGetRankList;
  this->cs_player_handler_[MSG_CS_REQUEST_USE_ITEM] = &LogicPlayer::ProcessRequestUseItem;
  this->cs_player_handler_[MSG_CS_REQUEST_SIGN_IN] = &LogicPlayer::ProcessRequestSignIn;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_RANK_AWARD] = &LogicPlayer::ProcessRequestGetRankAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_DAILY_AWARD] = &LogicPlayer::ProcessRequestGetDailyAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ACHIEVEMENT] = &LogicPlayer::ProcessRequestGetAchievement;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_MAIL_REWARD] = &LogicPlayer::ProcessRequestGetMailReward;
  this->cs_player_handler_[MSG_CS_REQUEST_RANK_INFO] = &LogicPlayer::ProcessRequestUpRankInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_SEVEN_DAYS] = &LogicPlayer::ProcessRequestGetSevenDays;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_FOURTEEN_DAYS] = &LogicPlayer::ProcessRequestGetFourteenDays;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_SERVER_SHOP_INFO] = &LogicPlayer::ProcessRequestGetServerShopInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_EQUIP_PLANE_NEW] = &LogicPlayer::ProcessRequestEquipPlaneNew;
  this->cs_player_handler_[MSG_CS_REQUEST_PLANE_LEVEL_UP_NEW] = &LogicPlayer::ProcessRequestPlaneLevelUpNew;
  this->cs_player_handler_[MSG_CS_REQUEST_CHANGE_CARRIER] = &LogicPlayer::ProcessRequestChangeCarrier;
  this->cs_player_handler_[MSG_CS_REQUEST_DIALOG] = &LogicPlayer::ProcessRequestDialog;
  this->cs_player_handler_[MSG_CS_REQUEST_LOAD_PLAYER] = &LogicPlayer::ProcessLoadPlayerBegin;
  this->cs_player_handler_[MSG_CS_REQUEST_PATROL_HELP] = &LogicPlayer::ProcessRequestPatrolHelp;
  this->cs_player_handler_[MSG_CS_REQUEST_ACTIVE_RELATION] = &LogicPlayer::ProcessRequestActiveRelation;
  this->cs_player_handler_[MSG_CS_REQUEST_DSTRIKE_DAILY_AWARD] = &LogicPlayer::ProcessRequestDstrikeDailyAward;
  this->cs_player_handler_[MSG_CS_REQUEST_FIGHT] = &LogicPlayer::ProcessRequestFight;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_RANDOM_FRIEND] = &LogicPlayer::ProcessRequestGetRandomFriend;
  this->cs_player_handler_[MSG_CS_REQUEST_CLIENT_FLAG] = &LogicPlayer::ProcessRequestClientFlag;
  this->cs_player_handler_[MSG_CS_REQUEST_EQUIP_CARRIER] = &LogicPlayer::ProcessRequestEquipCarrier;
  this->cs_player_handler_[MSG_CS_REQUEST_CARRIER_LEVEL_UP] = &LogicPlayer::ProcessRequestCarrierLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_CARRIER_REFORM_UP] = &LogicPlayer::ProcessRequestCarrierReformUp;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_CARRIER_COPY_AWARD] = &LogicPlayer::ProcessRequestGetCarrierCopyAward;
  this->cs_player_handler_[MSG_CS_REQUEST_CARRIER_COPY_NEXT_LEVEL] = &LogicPlayer::ProcessRequestCarrierCopyNextLevel;
  this->cs_player_handler_[MSG_CS_REQUEST_FIGHT_CARRIER_COPY] = &LogicPlayer::ProcessRequestFightCarrierCopy;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_OPEN_FUND] = &LogicPlayer::ProcessRequestServerOpenFund;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_OPEN_WELFARE] = &LogicPlayer::ProcessRequestServerOpenWelfare;
  this->cs_player_handler_[MSG_CS_REQUEST_ARMY_SKILL_UP] = &LogicPlayer::ProcessRequestArmySkillUp;
  this->cs_player_handler_[MSG_CS_REQUEST_CREATE_ARMY] = &LogicPlayer::ProcessRequestCreateArmy;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ARMY_WAR_INFO] = &LogicPlayer::ProcessRequestGetArmyWarInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_ARMY_WAR_FIGHT] = &LogicPlayer::ProcessRequestArmyWarFight;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ARMY_INFO] = &LogicPlayer::ProcessRequestGetArmyInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_ARMY_APPLY] = &LogicPlayer::ProcessRequestArmyApply;
  this->cs_player_handler_[MSG_CS_REQUEST_KICK_ARMY_MEMBER] = &LogicPlayer::ProcessRequestKickArmyMember;
  this->cs_player_handler_[MSG_CS_REQUEST_LEAVE_ARMY] = &LogicPlayer::ProcessRequestLeaveArmy;
  this->cs_player_handler_[MSG_CS_REQUEST_AGREE_ARMY_APPLY] = &LogicPlayer::ProcessRequestAgreeArmyApply;
  this->cs_player_handler_[MSG_CS_REQUEST_CHANGE_ARMY_ANNOUNCEMENT] = &LogicPlayer::ProcessRequestChangeArmyAnnouncement;
  this->cs_player_handler_[MSG_CS_REQUEST_DISMISS_ARMY] = &LogicPlayer::ProcessRequestDismissArmy;
  this->cs_player_handler_[MSG_CS_REQUEST_ARMY_SIGN] = &LogicPlayer::ProcessRequestArmySign;
  this->cs_player_handler_[MSG_CS_REQUEST_ARMY_LEVEL_UP] = &LogicPlayer::ProcessRequestArmyLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ARMY_SIGN_AWARD] = &LogicPlayer::ProcessRequestGetArmySignAward;
  this->cs_player_handler_[MSG_CS_REQUEST_ARMY_MAX_LEVEL_UP] = &LogicPlayer::ProcessRequsetArmyMaxLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_DAILY_ACTIVITY] = &LogicPlayer::ProcessRequestGetDailyActivity;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_VIP_DAILY_AWARD] = &LogicPlayer::ProcessRequestGetDailyVIPAward;
  this->cs_player_handler_[MSG_CS_REQUEST_RAISE_FUNDING] = &LogicPlayer::ProcessRequestRaiseFunding;
  this->cs_player_handler_[MSG_CS_REQUEST_FIRST_RECHARGE] = &LogicPlayer::ProcessRequestFirstRecharge;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_VIP_WEEKLY] = &LogicPlayer::ProcessRequestGetVIPWeekly;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ARMY_WAR_INFO] = &LogicPlayer::ProcessRequestGetArmyWarInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ARMY_WAR_CHAPTER_AWARD] = &LogicPlayer::ProcessRequestGetArmyWarChapterAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ARMY_WAR_BOSS_AWARD] = &LogicPlayer::ProcessRequestGetArmyWarBossAward;
  this->cs_player_handler_[MSG_CS_REQUEST_SET_ARMY_WAR_CHAPTER] = &LogicPlayer::ProcessRequestSetArmyWarChapter;
  this->cs_player_handler_[MSG_CS_REQUEST_ARMY_LIST] = &LogicPlayer::ProcessRequestArmyList;
  this->cs_player_handler_[MSG_CS_REQUEST_SEARCH_ARMY] = &LogicPlayer::ProcessRequestSearchArmy;
  this->cs_player_handler_[MSG_CS_REQUEST_CHANGE_ARMY_POS] = &LogicPlayer::ProcessRequestChangeArmyPos;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ARMY_SHOP_INFO] = &LogicPlayer::ProcessReuqestGetArmyShopInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_WORLD_BOSS] = &LogicPlayer::ProcessRequestGetWorldBossInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_FIGHT_WORLD_BOSS] = &LogicPlayer::ProcessRequestFightWorldBoss;
  this->cs_player_handler_[MSG_CS_REQUEST_WORLD_BOSS_COUNTRY] = &LogicPlayer::ProcessRequestWorldBossCountry;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_WORLD_BOSS_MERIT_AWARD] = &LogicPlayer::ProcessRequestGetWorldBossMeritAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_WORLD_BOSS_KILL_AWARD] = &LogicPlayer::ProcessRequestGetWorldBossKillAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_WORLD_BOSS_MERIT_RANK] = &LogicPlayer::ProcessRequestGetWorldBossMeritRank;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_WORLD_BOSS_DAMAGE_RANK] = &LogicPlayer::ProcessRequestGetWorldBossDamageRank;
  this->cs_player_handler_[MSG_CS_REQUEST_REFRESH_DIAMOND_SHOP] = &LogicPlayer::ProcessRequestRefreshDiamondShop;
  this->cs_player_handler_[MSG_CS_REQUEST_MAKE_WAKE_ITEM] = &LogicPlayer::ProcessRequestMakeWakeItem;
  this->cs_player_handler_[MSG_CS_REQUEST_RECOVER_WAKE_ITEM] = &LogicPlayer::ProcessRequestRecoverWakeItem;
  this->cs_player_handler_[MSG_CS_REQUEST_EQUIP_WAKE_ITEM] = &LogicPlayer::ProcessRequestEquipWakeItem;
  this->cs_player_handler_[MSG_CS_REQUEST_SHIP_WAKE] = &LogicPlayer::ProcessRequestShipWake;
  this->cs_player_handler_[MSG_CS_REQUEST_ELITE_RANDOM_COPY_FIGHT] = &LogicPlayer::ProcessRequestEliteRandomCopyFight;
  this->cs_player_handler_[MSG_CS_REQUEST_SET_USER_DEFINED] = &LogicPlayer::ProcessRequestSetUserDefined;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_TIME_ACTIVITY_AWARD] = &LogicPlayer::ProcessRequestGetTimeActivityAwardNew;
  this->cs_player_handler_[MSG_CS_REQUEST_ASTROLOGY] = &LogicPlayer::ProcessRequestAstrology;
  this->cs_player_handler_[MSG_CS_REQUEST_ASTROLOGY_CHANGE] = &LogicPlayer::ProcessRequestAstrologyChange;
  this->cs_player_handler_[MSG_CS_REQUEST_RESEARCH_ITEM] = &LogicPlayer::ProcessRequestResearchItem;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_RESEARCH_ITEM_AWARD] = &LogicPlayer::ProcessRequestGetResearchItemAward;
  this->cs_player_handler_[MSG_CS_REQUEST_CROSS_SERVER_COUNTRY] = &LogicPlayer::ProcessRequestCrossServerCountry;
  this->cs_player_handler_[MSG_CS_REQUEST_CROSS_SERVER_RANDOM_PLAYER] = &LogicPlayer::ProcessRequestCrossServerRandomPlayer;
  this->cs_player_handler_[MSG_CS_REQUEST_CROSS_SERVER_FIGHT] = &LogicPlayer::ProcessRequestCrossServerFight;
  this->cs_player_handler_[MSG_CS_REQUEST_CROSS_SERVER_GET_AWARD] = &LogicPlayer::ProcessRequestCrossServerGetAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_LEGION_WAR_REWARD] = &LogicPlayer::ProcessRequestGetLegionWarReward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_LEGION_WAR_INFO] = &LogicPlayer::ProcessRequestGetLegionWarInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_LEGION_WAR_FIGHT] = &LogicPlayer::ProcessRequestLegionWarFight;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_LEGION_WAR_PLAYER] = &LogicPlayer::ProcessRequestGetLegionWarPlayer;
  this->cs_player_handler_[MSG_CS_REQUEST_LEGION_WAR_REGISTER] = &LogicPlayer::ProcessRequestLegionWarRegister;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_LEGION_WAR_TARGET] = &LogicPlayer::ProcessRequestGetLegionWarTarget;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_LEGION_WAR_TARGET_AWARD] = &LogicPlayer::ProcessRequestGetLegionWarTargetAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_ASTROLOGY_INFO] = &LogicPlayer::ProcessRequestAstrologyInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_LOGIN_AWARD] = &LogicPlayer::ProcessRequestGetLoginAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_WEEKLY_CARD_AWARD] = &LogicPlayer::ProcessRequestGetWeeklyCardAward;
  this->cs_player_handler_[MSG_CS_REQUEST_LEGION_FOREPLAY_FIGHT] = &LogicPlayer::ProcessRequestLegionForeplayFight;
  this->cs_player_handler_[MSG_CS_REQUEST_LEGION_FOREPLAY_GET_SERVER_AWARD] = &LogicPlayer::ProcessRequestLegionForeplayGetServerAward;
  this->cs_player_handler_[MSG_CS_REQUEST_LEGION_FOREPLAY_GET_DAMAGE_AWARD] = &LogicPlayer::ProcessRequestLegionForeplayGetDamageAward;
  this->cs_player_handler_[MSG_CS_REQUEST_LEGION_FOREPLAY_GET_INFO] = &LogicPlayer::ProcessRequestLegionForeplayGetInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_WEEKL_CARD_SIGN] = &LogicPlayer::ProcessRequestWeeklyCardSign;
  this->cs_player_handler_[MSG_CS_REQUEST_ENTER_STAGE] = &LogicPlayer::ProcessRequestEnterStage;
  this->cs_player_handler_[MSG_CS_REQUEST_LEAVE_STAGE] = &LogicPlayer::ProcessRequestLeaveStage;
  this->cs_player_handler_[MSG_CS_REQUEST_SET_FOCUS_CITY] = &LogicPlayer::ProcessRequestSetFocusCity;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_LEGION_WAR_LOG] = &LogicPlayer::ProcessRequestGetLegionWarLog;
  this->cs_player_handler_[MSG_CS_NOTIFY_MONEY_INFO] = &LogicPlayer::ProcessNotifyMoneyInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_SWEEP_STAKE_EQUIP] = &LogicPlayer::ProcessRequestSweepStakeEquip;
  this->cs_player_handler_[MSG_CS_REQUEST_SWEEP_STAKE_CARRIER] = &LogicPlayer::ProcessRequestSweepStakeCarrier;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_SWEEP_STAKE_EQUIP_AWARD] = &LogicPlayer::ProcessRequestGetSweepStakeEquipAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_SWEEP_STAKE_COUNT_AWARD] = &LogicPlayer::ProcessRequestGetSweepStakeCountAward;
  this->cs_player_handler_[MSG_CS_REQUEST_ACTIVE_RELATION_ALL] = &LogicPlayer::ProcessRequestActiveRelationAll;
  this->cs_player_handler_[MSG_CS_REQUEST_FESTIVAL_REPLENISHSIGN] = &LogicPlayer::ProcessRequestFestivalReplenishSign;
  this->cs_player_handler_[MSG_CS_REQUEST_PLANE_CHANGE] = &LogicPlayer::ProcessRequestPlaneChange;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_VERSION_AWARD] = &LogicPlayer::ProcessRequestGetVersionAward;
  this->cs_player_handler_[MSG_CS_REQUEST_MEDAL_RESEARCH] = &LogicPlayer::ProcessRequestMedalResearch;
  this->cs_player_handler_[MSG_CS_REQUEST_MEDAL_FIGHT] = &LogicPlayer::ProcessRequestMedalFight;
  this->cs_player_handler_[MSG_CS_REQUEST_MEDAL_FIGHT_REFRESH] = &LogicPlayer::ProcessRequestMedalFightRefresh;
  this->cs_player_handler_[MSG_CS_REQUEST_MEDAL_ACTIVE] = &LogicPlayer::ProcessRequestMedalActive;
  this->cs_player_handler_[MSG_CS_REQUEST_RED_EQUIP_STAR_LEVEL_UP] = &LogicPlayer::ProcessRequestRedEquipStarLevelUp;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_COME_BACK_LOGIN_AWARD] = &LogicPlayer::ProcessRequestGetComeBackLoginAward;
  this->cs_player_handler_[MSG_CS_REQUEST_GET_COME_BACK_RECHARGE_AWARD] = &LogicPlayer::ProcessRequestGetComeBackRechargeAward;
  this->cs_player_handler_[MSG_CS_REQUEST_SELL_ITEM_EX] = &LogicPlayer::ProcessRequestSellItemEx;
  this->cs_player_handler_[MSG_CS_REQUEST_MEDAL_ACTIVE_ACHI] = &LogicPlayer::ProcessRequestMedalActiveAchi;
  this->cs_player_handler_[MSG_CS_REQUEST_PEARL_HARBOR_FIGHT] = &LogicPlayer::ProcessRequestPearlHarborFight;
  this->cs_player_handler_[MSG_CS_REQUEST_PEARL_HARBOR_GET_INFO] = &LogicPlayer::ProcessRequestPearlHarborGetInfo;
  this->cs_player_handler_[MSG_CS_REQUEST_PEARL_HARBOR_WAR_ZONE] = &LogicPlayer::ProcessRequestPearlHarborWarZone;
  this->cs_player_handler_[MSG_CS_REQUEST_PEARL_HARBOR_START_BUFF] = &LogicPlayer::ProcessRequestPearlHarborStartBuff;
  this->cs_player_handler_[MSG_CS_REQUEST_PEARL_HARBOR_ARMY_SCORE] = &LogicPlayer::ProcessRequestPearlHarborArmyScore;

  //服务器发来的玩家消息
  this->ss_player_handler_[MSG_SS_MESSAGE_ERROR] = &LogicPlayer::ProcessServerErrorMessage;
  this->ss_player_handler_[MSG_SS_RESPONSE_PLAYER_NOT_EXIST] = &LogicPlayer::ProcessPlayerNotExist;
  this->ss_player_handler_[MSG_SS_GET_PLAYER_INFO_BEGIN] = &LogicPlayer::ProcessResponseGetPlayerInfo;
  this->ss_player_handler_[MSG_SS_GET_ITEM_INFO] = &LogicPlayer::ProcessResponseGetItemInfo;
  this->ss_player_handler_[MSG_SS_GET_CARRIER_INFO] = &LogicPlayer::ProcessResponseGetCarrierInfo;
  this->ss_player_handler_[MSG_SS_GET_HERO_INFO] = &LogicPlayer::ProcessResponseGetHeroInfo;
  this->ss_player_handler_[MSG_SS_GET_TACTIC_INFO] = &LogicPlayer::ProcessResponseGetTacticInfo;
  this->ss_player_handler_[MSG_SS_GET_COPY_INFO] = &LogicPlayer::ProcessResponseGetCopyInfo;
  this->ss_player_handler_[MSG_SS_GET_MAIL_INFO] = &LogicPlayer::ProcessResponseGetMailInfo;
  this->ss_player_handler_[MSG_SS_GET_SHOP_INFO] = &LogicPlayer::ProcessResponseGetShopInfo;
  this->ss_player_handler_[MSG_SS_RESPONSE_LOAD_MULTI_PLAYER] = &LogicPlayer::ProcessResponseGetOtherPlayerInfo;
  this->ss_player_handler_[MSG_SS_GET_REWARD_INFO] = &LogicPlayer::ProcessResponseGetRewardInfo;
  this->ss_player_handler_[MSG_SS_GET_FRIEND_INFO] = &LogicPlayer::ProcessResponseGetFriendInfo;//YJX ADD
  this->ss_player_handler_[MSG_SS_GET_PATROL_INFO] = &LogicPlayer::ProcessResponseGetPatrolInfo;
  this->ss_player_handler_[MSG_SS_GET_REPORT_ABSTRACT] = &LogicPlayer::ProcessResponseGetReportAbstract;
  this->ss_player_handler_[MSG_SS_RESPONSE_GET_UID_BY_NAME] = &LogicPlayer::ProcessResponseGetUIDByName;
  this->ss_player_handler_[MSG_SS_RESPONSE_GET_MAIL_REWARD] = &LogicPlayer::ProcessResponseGetMailReward;
  this->ss_player_handler_[MSG_SS_RESPONSE_CHANGE_NAME] = &LogicPlayer::ProcessResponseChangeName;

  this->ss_player_handler_[MSG_SS_GET_PLAYER_INFO_END] = &LogicPlayer::ProcessLoadPlayerEnd;
  this->ss_player_handler_[MSG_SS_RESPONSE_CREATE_PLAYER] = &LogicPlayer::ProcessResponseCreatePlayer;
}

static std::vector<CSMessageEntry> cs_messages;
void Server::ParseCSMessageOnce() {
  if (!PopMessages(cs_messages)) return;
  for (size_t i = 0; i < cs_messages.size(); ++i) {
    CSMessageEntry& entry = cs_messages[i];
    server->IncTID();
    if (entry.head.msgid != MSG_CS_REQUEST_HEART_BEAT)
      INFO_LOG(logger)("ParseCSMessage SessionID:%ld, MSG:0x%04X",
                     entry.session_ptr->GetSessionID(), entry.head.msgid);
    //非法消息直接断开链接
    if (entry.head.msgid <= MSG_CS_MSG_ID_BEGIN ||
        entry.head.msgid >= MSG_CS_MSG_ID_END) {
      ERROR_LOG(logger)("Client MSG:0x%04X out of bound", entry.head.msgid);
      entry.session_ptr->Close();
      continue;
    }

    ResultID result = ERR_OK;
    int64_t uid = entry.session_ptr->GetUID();
    LogicPlayer* player = server->GetPlayerByID(uid);
    if (player) {
      if (entry.head.msgid >= MSG_CS_LOGIC_BEGIN) {
        MessageCache::Instance().ClearRawMessage(uid);
      }

      boost::unordered_map<uint16_t, PlayerCSMessageHandler>::iterator iter =
          this->cs_player_handler_.find(entry.head.msgid);
      if (iter == this->cs_player_handler_.end()) {
        ERROR_LOG(logger)("Unkown PlayerMessage, PlayerID:%ld, MSG:0x%04X", uid, entry.head.msgid);
        continue;
      }
      //客户端每次处理消息之前被动更新一次
      if (player->load_complete()) player->Update();
      if (entry.head.msgid > MSG_CS_LOGIC_BEGIN && !player->load_complete())
        result = ERR_PLAYER_DATA_NOT_LOADED;
      else
        result = ResultID(((*player).*iter->second)(entry));
    } else {
      boost::unordered_map<uint16_t, ServerCSMessageHandler>::iterator iter =
          this->cs_server_handler_.find(entry.head.msgid);
      if (iter == this->cs_server_handler_.end()) {
        ERROR_LOG(logger)("Unkown ClientMessage, PlayerID:%ld, MSG:0x%04X", uid, entry.head.msgid);
        continue;
      }
      result = ResultID((*iter->second)(entry));
    }

    if (result) {
      static google::protobuf::TextFormat::Printer printer;
      static std::string debug_str;
      printer.SetUseUtf8StringEscaping(true);
      printer.SetSingleLineMode(true);
      debug_str.clear();
      if (entry.message) {
        printer.PrintToString(*entry.message, &debug_str);
        if (debug_str.size() > 0 &&
            debug_str[debug_str.size() - 1] == ' ') {
          debug_str.resize(debug_str.size() - 1);
        }
      }
      TRACE_LOG(logger)("ParseCSMessage fail, PlayerID:%ld, MSG:0x%04X, Request:{%s}, ErrorCode:%d",
                  uid, entry.head.msgid, debug_str.c_str(), result);
      MessageErrorCode response;
#ifndef DEBUG
      if (result >= ERR_DEBUG_MESSAGE_START) result = ERR_PARAM_INVALID;
#endif
      response.set_err_code(result);
      response.set_msg_id(entry.head.msgid);
      this->SendMessageToClient(entry.session_ptr.get(), MSG_CS_ERROR_CODE,
                                &response);
    }
  }
  cs_messages.clear();
}

static std::vector<SSMessageEntry> ss_messages;
void Server::ParseSSMessageOnce() {
  if (!PopMessages(ss_messages)) return;

  for (size_t i = 0; i < ss_messages.size(); ++i) {
    SSMessageEntry& entry = ss_messages[i];
    server->IncTID();
    if (entry.head.msgid != MSG_SS_SERVER_HEART_BEAT)
      INFO_LOG(logger)("ParseSSMessage SessionID:%ld, MSG:0x%04X",
                     entry.session_ptr->GetSessionID(), entry.head.msgid);

    ResultID result = ERR_OK;
    if (entry.head.dest_type == ENTRY_TYPE_PLAYER) {
      int64_t uid = entry.head.dest_id;
      LogicPlayer* player = server->GetPlayerByID(uid);
      if (!player) {
        ERROR_LOG(logger)(
            "ParseSSMessage Player Not found, uid:%ld, MSG:0x%04X", uid,
            entry.head.msgid);
        continue;
      }
      boost::unordered_map<uint16_t, PlayerSSMessageHandler>::iterator iter =
          this->ss_player_handler_.find(entry.head.msgid);
      if (iter == this->ss_player_handler_.end()) {
        ERROR_LOG(logger)("Unkown PlayerMessage, MSG:0x%04X", entry.head.msgid);
        continue;
      }
      result = ResultID(((*player).*iter->second)(entry));
    } else {
      boost::unordered_map<uint16_t, ServerSSMessageHandler>::iterator iter =
          this->ss_server_handler_.find(entry.head.msgid);
      if (iter == this->ss_server_handler_.end()) {
        ERROR_LOG(logger)("Unkown ServerMessage, MSG:0x%04X", entry.head.msgid);
        continue;
      }
      result = ResultID((*iter->second)(entry));
    }

    if (result) {
      TRACE_LOG(logger)("ParseSSMessage fail, MSG:0x%04X", entry.head.msgid);
    }
  }
  ss_messages.clear();
}

struct DeleteReportCallback : public storage::ForEachCallback {
  DeleteReportCallback() : prefix_("ReportID:") {}

  const std::vector<std::string>& queue() const { return this->delete_keys_; }

  const std::string& prefix() const { return prefix_; }

  bool every(const storage::Slice& key, const storage::Slice& value) const {
    if (*(int64_t*)key.data() != *(int64_t*)prefix_.data()) return false;
    INFO_LOG(logger)("%.28s", key.data());
    char str[6] = {0};
    sscanf(key.data() + prefix_.size(), "%c%c%c%c%c", &str[0], &str[1], &str[2],
           &str[3], &str[4]);
    const tm& tm_now = GetTime();
    int32_t current_days = (tm_now.tm_year % 100) * 365 + tm_now.tm_yday + 1;
    int32_t report_day = atoi(str);
    int32_t report_days = ((report_day / 1000) * 365) + (report_day % 1000);
    if (report_days + 150 < current_days)
      this->delete_keys_.push_back(std::string(key.data(), key.size()));
    INFO_LOG(logger)("CurrentDays:%d, ReportDays:%d", current_days, report_days);
    return true;
  }

 private:
  std::string prefix_;
  mutable std::vector<std::string> delete_keys_;
};

void FlushLog() {
  while (Server::LoopFlag) {
    if (logger) logger->Flush();
    Yield(FLUSH_LOG_FRAME_TIME);
  }
}

void Server::Loop() {
  this->reconnect_thread_ =
      new boost::thread(boost::bind(&Server::TryConnectServer, this));
  this->flush_log_thread_ = new boost::thread(boost::bind(&FlushLog));
  storage::Init("./logic_db", 32 * 1024 * 1024);

  DeleteReportCallback fn;
  storage::ForEach(fn);
  //删除过期的战报
  for (std::vector<std::string>::const_iterator iter = fn.queue().begin();
       iter != fn.queue().end(); ++iter) {
    storage::Delete(*iter);
  }

  this->LoadLocalStorage();
  server_.Run();

  while (Server::LoopFlag) {
    time_t begin = GetMilliSeconds();
    ParseCSMessageOnce();
    ParseSSMessageOnce();
    //一秒一次定时器
    if (GetSeconds() != this->last_active_seconds_) this->OnSecondsChanged();
    time_t end = GetMilliSeconds();

    if (end - begin < ONE_FRAME_TIME) {
      Yield(ONE_FRAME_TIME - (end - begin));
    }
  }
  this->server_.Stop();
  storage::UnInit();
  TRACE_LOG(logger)("Storage UnInit");
  logger->Flush();
}
