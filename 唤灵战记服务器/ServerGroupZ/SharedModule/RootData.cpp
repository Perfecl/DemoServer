#include "stdafx.h"
#include "RootData.h"

std::map<int, const CRootData*> CRootData::ms_mapRootData;

CRootData::CRootData()
{
}

CRootData::~CRootData()
{
}

void CRootData::Load()
{
	dat_RootData_Library libRootData;
	libRootData.ParseFromString(GetDataFromFile(GAME_DATA_PATH"RootData.txt"));

	for (int i = 0; i < libRootData.root_data_library_size(); i++)
	{
		CRootData* pRootData = new CRootData;
		pRootData->m_nLevel = libRootData.root_data_library(i).lv();
		pRootData->m_nTrainSpendHonour = libRootData.root_data_library(i).train_spend_honour();

		pRootData->m_nRewardMissionExp = libRootData.root_data_library(i).reward_mission_exp();
		pRootData->m_nRewardMissionSilver = libRootData.root_data_library(i).reward_mission_silver();
		pRootData->m_nRewardMissionHonour = libRootData.root_data_library(i).reward_mission_honour();

		pRootData->m_nOfflineBattleWeekSilver = libRootData.root_data_library(i).offline_battle_week_silver();
		pRootData->m_nOfflineBattleWeekReputation = libRootData.root_data_library(i).offline_battle_week_reputation();
		pRootData->m_nOfflineBattleSingleSilver = libRootData.root_data_library(i).offline_battle_single_silver();

		pRootData->m_nEscortSilver = libRootData.root_data_library(i).escort_silver();
		pRootData->m_nEscortProtectHonour = libRootData.root_data_library(i).escort_reputation();

		pRootData->m_nMeditationExp = libRootData.root_data_library(i).meditation_exp();

		pRootData->m_nArenaSilver = libRootData.root_data_library(i).arena_silver();
		pRootData->m_nArenaHonour = libRootData.root_data_library(i).arena_honour();

		pRootData->m_nFairSingleSilver = libRootData.root_data_library(i).fair_single_silver();
		pRootData->m_nFairSingleHonour = libRootData.root_data_library(i).fair_single_honour();
		pRootData->m_nSpeedSilver = libRootData.root_data_library(i).speed_silver();
		pRootData->m_nSpeedHonour = libRootData.root_data_library(i).speed_honour();

		pRootData->m_nWashRuneHonour = libRootData.root_data_library(i).wash_rune_honour();
		pRootData->m_nHeroLevelHonour = libRootData.root_data_library(i).hero_level_honour();

		pRootData->train_soldier_silver_ = libRootData.root_data_library(i).train_soldier_silver();

		pRootData->levy_stage_ = libRootData.root_data_library(i).levy_stage();
		pRootData->gold_stone_silver_ = libRootData.root_data_library(i).gold_stone_silver();
		pRootData->castle_update_silver_ = libRootData.root_data_library(i).castle_update_silver();

		pRootData->exercise_platform_exp_ = libRootData.root_data_library(i).exercise_platform_exp();

		pRootData->mission_exp_ = libRootData.root_data_library(i).mission_exp();

		ms_mapRootData.insert(std::make_pair(pRootData->m_nLevel, pRootData));
	}
}

const CRootData* CRootData::GetRootData(int nLevel)
{
	auto it = ms_mapRootData.find(nLevel);
	if (ms_mapRootData.cend() == it)
		return nullptr;
	else
		return it->second;
}

__int64 CRootData::GetTrainSoldierSilver(int level)
{
	auto it = ms_mapRootData.find(level);
	if (ms_mapRootData.cend() == it)
		return 0;
	else
		return it->second->train_soldier_silver_;
}

__int64 CRootData::GetRootData(int nLevel, RootDataType enRootDataType)
{
	auto it = ms_mapRootData.find(nLevel);
	if (ms_mapRootData.cend() == it)
		return 0;
	else
	{
		switch (enRootDataType)
		{
		case RDT_TrainSpendHonour:
			return it->second->m_nTrainSpendHonour;
		case RDT_RewardMissionExp:
			return it->second->m_nRewardMissionExp;
		case RDT_RewardMissionSilver:
			return it->second->m_nRewardMissionSilver;
		case RDT_RewardMissionHonour:
			return it->second->m_nRewardMissionHonour;

		case RDT_OfflineBattleSingleSilver:
			return it->second->m_nOfflineBattleSingleSilver;
		case RDT_OfflineBattleWeekSilver:
			return it->second->m_nOfflineBattleWeekSilver;
		case RDT_OfflineBattleWeekReputation:
			return it->second->m_nOfflineBattleWeekReputation;

		case RDT_EscortSilver:
			return it->second->m_nEscortSilver;
		case RDT_EscortReputation:
			return it->second->m_nEscortProtectHonour;

		case RDT_MeditationExp:
			return it->second->m_nMeditationExp;

		case RDT_ArenaSilver:
			return it->second->m_nArenaSilver;
		case RDT_ArenaHonour:
			return it->second->m_nArenaHonour;

		case RDT_FairSingleSilver:
			return it->second->m_nFairSingleSilver;
		case RDT_FairSingleHonour:
			return it->second->m_nFairSingleHonour;
		case RDT_SpeedSilver:
			return it->second->m_nSpeedSilver;
		case RDT_SpeedHonour:
			return it->second->m_nSpeedHonour;

		case RDT_WashRuneHonour:
			return it->second->m_nWashRuneHonour;
		case RDT_HeroLevelHonour:
			return it->second->m_nHeroLevelHonour;
		
		case RDT_MissionExp:
			return it->second->mission_exp_;
		default:
			return 0;
		}
	}
}

int CRootData::GetRewardMissionResource(int nLevel, int type)
{
	auto it = ms_mapRootData.find(nLevel);
	if (ms_mapRootData.cend() == it)
		return 0;
	else
	{
		switch (type)
		{
		case (int)PRT_Silver:
			return it->second->m_nRewardMissionSilver;
			break;
		case (int)PRT_Exp:
			return it->second->m_nRewardMissionExp;
			break;
		case (int)PRT_Honour:
			return it->second->m_nRewardMissionHonour;
			break;
		default:
			return 0;
		}
	}
}
