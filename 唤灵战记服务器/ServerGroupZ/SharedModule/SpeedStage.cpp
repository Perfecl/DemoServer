#include "stdafx.h"
#include "SpeedStage.h"


map<int, SpeedStageReward*> CSpeedStage::m_mapSpeedStageReward;
vector<pair<int, int>> CSpeedStage::m_vctSpeedStage;

CSpeedStage::CSpeedStage()
{

}

CSpeedStage::~CSpeedStage()
{

}

void CSpeedStage::Load()
{
	pto_FAIRARENA_STRUCT_FairArena libFairArena;
	libFairArena.ParseFromString(GetDataFromFile(GAME_DATA_PATH"FairArena.txt"));
	for (int i = 0; i < libFairArena.speed_reward_library_size(); i++)
	{
		SpeedStageReward* pReward = new SpeedStageReward;
		pReward->m_nRank = libFairArena.speed_reward_library(i).rank();
		pReward->m_nGold = libFairArena.speed_reward_library(i).reward_gold();
		pReward->m_fCoefficient = libFairArena.speed_reward_library(i).coefficient();

		m_mapSpeedStageReward.insert(std::make_pair(pReward->m_nRank, pReward));
	}

	for (int i = 0; i < libFairArena.speed_stage_library_size(); i++)
	{
		int nID = libFairArena.speed_stage_library(i).id();
		int nMapID = libFairArena.speed_stage_library(i).map_id();

		m_vctSpeedStage.push_back(std::make_pair(nID, nMapID));
	}
}

int CSpeedStage::GetGoldReward(int nRank)
{
	auto it = m_mapSpeedStageReward.find(nRank);
	if (m_mapSpeedStageReward.cend() == it)
		return 10;
	else
		return it->second->m_nGold;
}

float CSpeedStage::GetRewardCoefficient(int nRank)
{
	auto it = m_mapSpeedStageReward.find(nRank);
	if (m_mapSpeedStageReward.cend() == it)
		return 0.75;
	else
		return it->second->m_fCoefficient;
}

int CSpeedStage::ProduceSpeedStageID(int nOldId)
{
	if (m_vctSpeedStage.size() <= 0)
		return 0;
	int nID;
	do
	{
		nID = m_vctSpeedStage[GetRandom<int>(0, m_vctSpeedStage.size() - 1)].first;
	} while (nID == nOldId);
	
	return nID;
}

int CSpeedStage::GetMapID(int nID)
{
	for (auto it : m_vctSpeedStage)
	{
		if (nID == it.first)
			return it.second;
	}
	return 0;
}
