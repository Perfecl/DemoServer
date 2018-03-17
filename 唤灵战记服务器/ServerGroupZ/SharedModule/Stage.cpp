#include "stdafx.h"
#include "Stage.h"

vector<const CStage*> CStage::ms_vctStages;

void CStage::Load()
{
	Stage_Map_Library libStage;
	libStage.ParseFromString(GetDataFromFile(GAME_DATA_PATH"StageMap.txt"));

	for (int i = 0; i < libStage.stage_maps_library_size(); i++)
	{
		auto ptoStage = libStage.stage_maps_library(i);

		CStage* pStage = new CStage;

		pStage->m_nMapID = ptoStage.map_id();
		pStage->m_nLevel = ptoStage.stage_lv();
		pStage->m_nDifficulty = ptoStage.stage_difficulty();
		pStage->m_nName = ptoStage.stage_name();
		pStage->m_nInfo = ptoStage.stage_info();
		pStage->m_nSceneID = ptoStage.scene_id();
		pStage->m_nBossID = ptoStage.boss_id();

		pStage->m_nRewardExp = ptoStage.reward_exp();
		pStage->m_nRewardSilver = ptoStage.reward_silver();
		pStage->m_nRewardHonor = ptoStage.reward_honour();
		;

		pStage->m_nRewardItem1ID = ptoStage.reward_item_1_id();
		pStage->m_nRewardItem1Num = ptoStage.reward_item_1_num();

		pStage->m_nRewardItem2ID = ptoStage.reward_item_2_id();
		pStage->m_nRewardItem2Num = ptoStage.reward_item_2_num();

		pStage->m_nRewardItem3ID = ptoStage.reward_item_3_id();
		pStage->m_nRewardItem3Num = ptoStage.reward_item_3_num();

		pStage->m_nRewardItem4ID = ptoStage.reward_item_4_id();
		pStage->m_nRewardItem4Num = ptoStage.reward_item_4_num();

		pStage->m_nRewardItem5ID = ptoStage.reward_item_5_id();
		pStage->m_nRewardItem5Num = ptoStage.reward_item_5_num();

		pStage->m_nRewardItem6ID = ptoStage.reward_item_6_id();
		pStage->m_nRewardItem6Num = ptoStage.reward_item_6_num();

		pStage->m_nFightingCapacity = ptoStage.fighting_capacity();
		pStage->m_nUnlockLevel = ptoStage.unlock_lv();

		pStage->m_nMusicID = ptoStage.music_id();

		ms_vctStages.push_back(pStage);
	}
}

const CStage* CStage::GetStage(int nMapID)
{
	for (auto &it : ms_vctStages)
	if (nMapID == it->m_nMapID)
		return it;

	return nullptr;
}

const CStage* CStage::GetStage(int nLv, int nDifficult)
{
	for (auto &it : ms_vctStages)
	if (nLv == it->m_nLevel && nDifficult == it->m_nDifficulty)
		return it;

	return nullptr;
}
