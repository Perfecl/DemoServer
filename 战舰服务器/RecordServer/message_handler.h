#pragma once
#include "server.h"


//服务器登录
int32_t ProcessServerLogin(SSMessageEntry& entry);

//拉取玩家信息
int32_t ProcessLoadPlayerInfo(SSMessageEntry& entry);

//发送邮件给玩家
int32_t ProcessSendMail(SSMessageEntry& entry);

//原封返回的消息
int32_t ProcessPingPong(SSMessageEntry& entry);

//拉取竞技场列表
int32_t ProcessGetPKRankList(SSMessageEntry& entry);

//通过名字获取玩家UID
int32_t ProcessGetUIDByName(SSMessageEntry& entry);

//Load多个玩家
int32_t ProcessLoadMultiPlayer(SSMessageEntry& entry);

//更新多个玩家的每日竞技场奖励
int32_t ProcessUpdateDailyPKRankInfo(SSMessageEntry& entry);

//拉取排行榜
int32_t ProcessLoadRankList(SSMessageEntry& entry);

//更新排行榜
int32_t ProcessUpdateRankListDetails(SSMessageEntry& entry);

//更新排行榜玩家
int32_t ProcessUpdateRankListPlayer(SSMessageEntry& entry);

//更新被围剿的BOSS信息
int32_t ProcessUpdateDstrikeBoss(SSMessageEntry& entry);

//拉取被围剿的BOSS列表
int32_t ProcessLoadDstrikeBossList(SSMessageEntry& entry);

//T玩家下线
int32_t ProcessKickPlayer(SSMessageEntry& entry);

//拉取商店信息
int32_t ProcessRequestLoadServerShop(SSMessageEntry& entry);

//更新商店信息
int32_t ProcessUpdateServerShop(SSMessageEntry& entry);

//设置玩家账号类型,禁言等信息
int32_t ProcessSetAccountStatus(SSMessageEntry& entry);

//拉取服务器邮件
int32_t ProcessRequestLoadServerMail(SSMessageEntry& entry);

//发送全服邮件
int32_t ProcessRequestAddServerMail(SSMessageEntry& entry);

//设置IP列表
int32_t ProcessLoadSetIpList(SSMessageEntry& entry);

//添加公告
int32_t ProcessSetNotice(SSMessageEntry& entry);

//读取公告
int32_t ProcessLoadNotice(SSMessageEntry& entry);

//更新创建玩家信息
int32_t ProcessUpdateCreatePlayerInfo(SSMessageEntry& entry);

//拉取军团信息
int32_t ProcessLoadArmyInfo(SSMessageEntry& entry);

//创建军团
int32_t ProcessRequestCreateArmy(SSMessageEntry& entry);

//解散军团
int32_t ProcessRequestDestoryArmy(SSMessageEntry& entry);

//更新军团简单信息
int32_t ProcessUpdateArmyExpInfo(SSMessageEntry& entry);

//当玩家离开或者加入军团
int32_t ProcessOnPlayerJoinArmy(SSMessageEntry& entry);

//更新军团Log
int32_t ProcessUpdateArmyLog(SSMessageEntry& entry);

//更新军团申请
int32_t ProcessUpdateArmyApply(SSMessageEntry& entry);

//更新玩家的军团状态
int32_t ProcessUpdateArmyMemberStatus(SSMessageEntry& entry);

//更新军团公告
int32_t ProcssUpdateArmyNotice(SSMessageEntry& entry);

//记录玩家充值
int32_t ProcessUpdatePlayerRecharge(SSMessageEntry& entry);

//记录首次充值信息
int32_t ProcessUpdateFirstRechargeInfo(SSMessageEntry& entry);

//更新玩家引导ID
int32_t ProcessUpdateDialogID(SSMessageEntry& entry);

//读取开服时间
int32_t ProcessServerStartTime(SSMessageEntry& entry);

//更新军团成员
int32_t ProcessUpdateArmyMember(SSMessageEntry& entry);

//更新军团信息
int32_t ProcessUpdateArmyOtherInfo(SSMessageEntry& entry);
int32_t LoadArmy(boost::shared_ptr<TcpSession>& session,
                 const uint32_t* server_id, int32_t count);

//拉取限时活动
int32_t ProcessResponseLoadTimeActivityNew(SSMessageEntry& entry);

//更新占星奖池
int32_t ProcessUpdateAstrologyAward(SSMessageEntry& entry);

//更新服务器副本统计信息
int32_t ProcessUpdateServerCopyInfo(SSMessageEntry& entry);

//查询最近活跃的玩家
int32_t ProcessRequestQueryLivelyAccount(SSMessageEntry& entry);

//服务器心跳
int32_t ProcessRequestServerHeartBeat(SSMessageEntry& entry);

//获取服务器内所有的玩家名字
int32_t ProcessRequestLoadALlName(SSMessageEntry& entry);

//设置黑白名单
int32_t ProcessRequestUpdateIpList(SSMessageEntry& entry);
