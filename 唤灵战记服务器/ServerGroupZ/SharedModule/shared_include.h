#pragma once

#ifdef _DEBUG
#pragma comment(lib,"../Debug/SharedModule.lib")
#else
#pragma comment(lib,"../Release/SharedModule.lib")
#endif

#include "shared_header.h"

#include "BasicServer.h"
#include "ServerList.h"
#include "NameLibrary.h"


//本地数据
#include "RewardMission.h"
#include "ItemTP.h"
#include "EquipTP.h"
#include "MissionTP.h"
#include "ExpLibrary.h"
#include "OpenGuid.h"
#include "CombinationReward.h"
#include "Lottery.h"
#include "LotteryChance.h"
#include "TimeBox.h"
#include "TownBox.h"
#include "VIPFun.h"
#include "GoldSpend.h"
#include "RootData.h"
#include "SoldierTP.h"
#include "HeroTP.h"
#include "RecastLib.h"
#include "RecruitLib.h"
#include "OpenTP.h"
#include "InteriorEvent.h"
#include "InteriorLib.h"
#include "Skill.h"
#include "PassSkill.h"
#include "TradeCard.h"
#include "Stage.h"
#include "EliteStage.h"
#include "SpeedStage.h"
#include "TownRankBox.h"
#include "AdditionLibrary.h"
#include "OfflineRewardStage.h"
#include "ServerList.h"
#include "RuneLibrary.h"
#include "FairArenaFirstWin.h"
#include "NameLibrary.h"
#include "FashionTP.h"
#include "SuitTP.h"
#include "StageLoot.h"
#include "Blueprint.h"
#include "HeroQuality.h"
#include "Enchanting.h"
#include "EnchantingCost.h"
#include "ResolveTP.h"
#include "RecastCost.h"
#include "Mall.h"
#include "IllegalWords.h"
#include "VIPGift.h"
#include "Gift.h"
#include "GuildLibrary.h"