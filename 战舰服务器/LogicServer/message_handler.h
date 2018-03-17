#pragma once
#include "server.h"

//这边是处理服务器消息
//玩家消息都在Player身上处理

//账号注册
int32_t ProcessRequestRegisteAccount(CSMessageEntry& entry);
int32_t ProcessResponseRegisteAccount(SSMessageEntry& entry);
//账号登陆
int32_t ProcessRequestAccountLogin(CSMessageEntry& entry);
int32_t ProcessResponseAccountLogin(SSMessageEntry& entry);

//登录
int32_t ProcessRequestPlayerLogin(CSMessageEntry& entry);
int32_t ProcessResponsePlayerLogin(SSMessageEntry& entry);
//忽略该条消息
int32_t ProcessSkipThisMessage(CSMessageEntry& entry);
int32_t ProcessSkipThisMessage(SSMessageEntry& entry);

//上报Token
int32_t ProcessRequestReportToken(CSMessageEntry& entry);

int32_t ProcessRequestHeartBeat(CSMessageEntry& entry);

//新邮件
int32_t ProcessResponseNewMail(SSMessageEntry& entry);

//竞技场列表
int32_t ProcessResponseGetPKRankList(SSMessageEntry& entry);

//拉取多个玩家返回
int32_t ProcessResponseGetOtherPlayerInfo(SSMessageEntry& entry);

//Load RankList 返回
int32_t ProcessResponseLoadRankList(SSMessageEntry& entry);

// CenterServer消息
//踢人
int32_t ProcessRequestKickUser(SSMessageEntry& entry);

//拉取被围剿BOSS列表
int32_t ProcessResponseLoadBossList(SSMessageEntry& entry);

//拉取全服商店
int32_t ProcessResponseLoadServerShop(SSMessageEntry& entry);

//设置账号状态类型
int32_t ProcessSetAccountStatus(SSMessageEntry& entry);

//拉取全服邮件返回
int32_t ProcessResponseLoadServerMail(SSMessageEntry& entry);

//发送全服邮件
int32_t ProcessRequestAddServerMail(SSMessageEntry& entry);

//发送邮件给多个人
int32_t ProcessRequestSendMailToMulti(SSMessageEntry& entry);

//GM设置IpList
int32_t ProcessRequestSetIpList(SSMessageEntry& entry);

//设置添加删除资源
int32_t ProcessAddGoods(SSMessageEntry& entry);

//添加IP列表
int32_t ProcessResponseLoadIPList(SSMessageEntry& entry);

//添加修改公告
int32_t ProcessSetNotice(SSMessageEntry& entry);

//加载公告
int32_t ProcessResponseLoadNotice(SSMessageEntry& entry);

//拉取军团返回
int32_t ProcessResponseLoadArmy(SSMessageEntry& entry);
int32_t ProcessResponseLoadArmyMember(SSMessageEntry& entry);

//创建军团返回
int32_t ProcessResponseCreateArmy(SSMessageEntry& entry);

//充值
int32_t ProcessRequestRecharge(SSMessageEntry& entry);

//设置玩家引导
int32_t ProcessSetPlayerGuide(SSMessageEntry& entry);

//获取开服时间
int32_t  ProcessGetSetServerStart(SSMessageEntry& entry);

//服务器清挡
int32_t ProcessClearServerData(SSMessageEntry& entry);

//读取限时活动
int32_t ProcessResponseLoadTimeActivityNew(SSMessageEntry& entry);

//更新限时活动
int32_t ProcessServerTimeActivity(SSMessageEntry& entry);

//查询玩家返回
int32_t ProcessResponseQueryOtherPlayer(SSMessageEntry& entry);

//制霸全球注册玩家返回
int32_t ProcessResponseLegionWarRegister(SSMessageEntry& entry);

//改变玩家的位置
int32_t ProcessResponseUpdateLegionWarPlayerPos(SSMessageEntry& entry);

//更新跨服排行榜
int32_t ProcessResponseCrossServerRankList(SSMessageEntry& entry);

//更新跨服玩家军团信息发生变化
int32_t ProcessUpdateLegionWarPlayerInfo(SSMessageEntry& entry);

//查询活跃玩家返回
int32_t ProcessResponseQueryLivelyAccount(SSMessageEntry& entry);

//服务器心跳
int32_t ProcessRequestServerHeartBeat(SSMessageEntry& entry);

//拉取所有名字返回
int32_t ProcessResponseLoadAllName(SSMessageEntry& entry);

int32_t ProcessProcessGmCommand(CSMessageEntry& entry);

int32_t ProcessProcessNotifyGlobalConfig(SSMessageEntry& entry);
