#include "stdafx.h"
#include "OfflineRewardStage.h"

vector<COfflineRewardStage*> COfflineRewardStage::ms_vctOfflineRewardStage;

COfflineRewardStage::COfflineRewardStage()
{
}


COfflineRewardStage::~COfflineRewardStage()
{
}

void COfflineRewardStage::Load()
{
	Reputation_Library libReputation;
	libReputation.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Reputation.txt"));

	for (int i = 0; i < libReputation.offlinebattle_reputation_library_size(); i++)
	{
		COfflineRewardStage* pStage = new COfflineRewardStage;
		pStage->m_nMinRank = libReputation.offlinebattle_reputation_library(i).min_rank();
		pStage->m_nMaxRank = libReputation.offlinebattle_reputation_library(i).max_rank();
		pStage->m_nRank = libReputation.offlinebattle_reputation_library(i).rank();
		pStage->m_nStage = libReputation.offlinebattle_reputation_library(i).stage();
		pStage->m_fStageReduce = libReputation.offlinebattle_reputation_library(i).stage_reduce();
		pStage->m_fCoefficient = libReputation.offlinebattle_reputation_library(i).coefficient();

		ms_vctOfflineRewardStage.push_back(pStage);
	}
}

const COfflineRewardStage* COfflineRewardStage::GetOfflineRewardStage(int nRank)
{
	for (auto it : ms_vctOfflineRewardStage)
	{
		if (nRank >= it->m_nMinRank &&
			nRank <= it->m_nMaxRank)
			return it;
	}
	return nullptr;
}