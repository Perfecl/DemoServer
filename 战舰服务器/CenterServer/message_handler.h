#pragma once
#include "server.h"

//服务器登录
int32_t ProcessServerLogin(SSMessageEntry& entry);

//服务器人数
int32_t ProcessPlayerNum(SSMessageEntry& entry);

//添加物品返回
int32_t ProcessAddGoods(SSMessageEntry& entry);

//充值确认
int32_t ProcessUpdateRecharge(SSMessageEntry& entry);

//更新OtherPlayerInfo
int32_t ProcessUpdateOtherPlayerInfo(SSMessageEntry& entry);

//获取OtherPlayer
int32_t ProcessRequestQueryOtherPlayer(SSMessageEntry& entry);

//给制霸全球玩家分配槽位
int32_t ProcessRequestRegisterLegionPlayer(SSMessageEntry& entry);

//制霸全球玩家交换位置
int32_t ProcessRequestLegionWarSwapPlayer(SSMessageEntry& entry);

//更新跨服排行榜
int32_t ProcessRequestUpdateCrossServerRankList(SSMessageEntry& entry);

//更新跨服OtherPlayer军团信息
int32_t ProcessUpdateLegionWarOtherPlayer(SSMessageEntry& entry);

//玩家登录(充值补偿)
int32_t ProcessPlayerLogin(SSMessageEntry& entry);

//创建血战珊瑚海队伍
int32_t ProcessRequestCreateCoralSeaTeam(SSMessageEntry& entry);

//离开血战珊瑚海队伍
int32_t ProcessRequestLeaveCoralSeaTeam(SSMessageEntry& entry);

//寻找一个血战珊瑚海队伍
int32_t ProcessRequestSearchCoralSeaTeam(SSMessageEntry& entry);

//加入一个血战珊瑚海队伍
int32_t ProcessRequestJoinCoralSeaTeam(SSMessageEntry& entry);

//血战珊瑚海调试
int32_t ProcessRequestCoralSeaDebug(SSMessageEntry& entry);
