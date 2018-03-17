#include "stdafx.h"
#include "EliteStage.h"
#include "GoldSpend.h"

std::map<int, const CEliteStage*> CEliteStage::ms_mapEliteStages;
std::map<int, const CEliteStage*> CEliteStage::ms_mapMultiStages;

CEliteStage::CEliteStage()
{
	m_arrProbability.fill(0);
}

CEliteStage::~CEliteStage()
{

}

void CEliteStage::Load()
{
	dat_STAGE_STRUCT_EliteStageLibrary libEliteStage;
	libEliteStage.ParseFromString(GetDataFromFile(GAME_DATA_PATH"EliteStage.txt"));

	for (int i = 0; i < libEliteStage.elite_stage_library_size(); i++)
	{
		auto ptoEliteStage = libEliteStage.elite_stage_library(i);

		CEliteStage* pEliteStage = new CEliteStage;

		pEliteStage->m_nMapID = ptoEliteStage.map_id();
		pEliteStage->m_nLevel = ptoEliteStage.stage_lv();
		pEliteStage->m_nOpenLevel = ptoEliteStage.open_level();

		pEliteStage->m_nRewardExp = ptoEliteStage.reward_exp();
		pEliteStage->m_nRewardSilver = ptoEliteStage.reward_silver();
		pEliteStage->m_nRewardHonour = ptoEliteStage.reward_honour();

		pEliteStage->m_nRewardBox = ptoEliteStage.reward_box();
		pEliteStage->m_arrProbability[0] = ptoEliteStage.probability_2();
		pEliteStage->m_arrProbability[1] = ptoEliteStage.probability_3();

		ms_mapEliteStages.insert(make_pair(pEliteStage->m_nLevel, pEliteStage));
	}

	dat_STAGE_STRUCT_EliteStageLibrary libMultiStage;
	libMultiStage.ParseFromString(GetDataFromFile(GAME_DATA_PATH"MultiStage.txt"));

	for (int i = 0; i < libMultiStage.elite_stage_library_size(); i++)
	{
		auto ptoMultiStage = libMultiStage.elite_stage_library(i);

		CEliteStage* pMultiStage = new CEliteStage;

		pMultiStage->m_nMapID = ptoMultiStage.map_id();
		pMultiStage->m_nLevel = ptoMultiStage.stage_lv();
		pMultiStage->m_nOpenLevel = ptoMultiStage.open_level();

		pMultiStage->m_nRewardExp = ptoMultiStage.reward_exp();
		pMultiStage->m_nRewardSilver = ptoMultiStage.reward_silver();
		pMultiStage->m_nRewardHonour = ptoMultiStage.reward_honour();

		pMultiStage->m_nRewardBox = ptoMultiStage.reward_box();
		pMultiStage->m_arrProbability[0] = ptoMultiStage.probability_2();
		pMultiStage->m_arrProbability[1] = ptoMultiStage.probability_3();

		ms_mapMultiStages.insert(make_pair(pMultiStage->m_nLevel, pMultiStage));
	}
}

const CEliteStage* CEliteStage::GetEliteStageByMapID(int nMapID)
{
	for (auto it : ms_mapEliteStages)
	if (nMapID == it.second->m_nMapID)
		return it.second;

	return nullptr;
}

const CEliteStage* CEliteStage::GetEliteStageByLevel(int nLevel)
{
	auto it = ms_mapEliteStages.find(nLevel);
	if (it != ms_mapEliteStages.cend())
		return it->second;
	return nullptr;
}

const CEliteStage* CEliteStage::GetMultiStageByMapID(int nMapID)
{
	for (auto it : ms_mapMultiStages)
	if (nMapID == it.second->m_nMapID)
		return it.second;

	return nullptr;
}

const CEliteStage* CEliteStage::GetMultiStageByLevel(int nLevel)
{
	auto it = ms_mapMultiStages.find(nLevel);
	if (it != ms_mapMultiStages.cend())
		return it->second;
	return nullptr;
}

int CEliteStage::ProduceBoxNum() const
{
	int random = GetRandom(1, 100);

	if (random <= (m_arrProbability[1] * 100))
		return 3;
	if (random <= ((m_arrProbability[1] + m_arrProbability[0]) * 100))
		return 2;
	else
		return 1;
}