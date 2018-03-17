#include "stdafx.h"
#include "MissionTP.h"

std::map<int, const CMissionTP*> CMissionTP::m_mapMissions;

void CMissionTP::Load()
{
	Mission_Library_For_Server MissionLibrary;
	MissionLibrary.ParseFromString(GetDataFromFile(GAME_DATA_PATH"MissionForServer.txt"));

	for (int i = 0; i < MissionLibrary.missions_for_server_size(); i++)
	{
		auto ptoMission = MissionLibrary.missions_for_server(i);
		CMissionTP* pMission = new CMissionTP;
		pMission->m_nID = ptoMission.mission_id();
		pMission->m_nType = ptoMission.mission_type();
		pMission->m_nNeedMainProgress = ptoMission.nessary_maineline_progress();
		pMission->m_nNeedLevel = ptoMission.nessary_player_lv();
		pMission->m_nFollowMisssionID = ptoMission.followup_mission_id();
		pMission->m_nAcceptNPC = ptoMission.offer_mission_npc_id();
		pMission->m_nCommitNPC = ptoMission.giveback_mission_npc_id();
		pMission->m_nTargetType = ptoMission.mission_object_type();
		pMission->m_nTargetID = ptoMission.mission_object_id();
		pMission->m_nTargetIDEx = ptoMission.mission_object_add_id();
		pMission->m_nTargetNeedNum = ptoMission.mission_object_nessary_num();
		pMission->m_nRewardExp = ptoMission.reward_exp();
		pMission->m_nRewardSilver = ptoMission.reward_gold();
		pMission->m_nRewardHeroID = ptoMission.reward_hero_id();
		pMission->m_nRewardHonor = ptoMission.reward_honour();
		pMission->m_nRewardReputation = ptoMission.reward_reputation();
		pMission->m_nRewardGold = ptoMission.reward_coupon();
		pMission->m_nRewardStamina = ptoMission.reward_stamina();
		pMission->m_nRewardItem1 = ptoMission.reward_item_1_id();
		pMission->m_nRewardItem1Num = ptoMission.reward_item_1_num();
		pMission->m_nRewardItem2 = ptoMission.reward_item_2_id();
		pMission->m_nRewardItem2Num = ptoMission.reward_item_2_num();
		pMission->m_nRewardItem3 = ptoMission.reward_item_3_id();
		pMission->m_nRewardItem3Num = ptoMission.reward_item_3_num();
		pMission->m_nRewardItem4 = ptoMission.reward_item_4_id();
		pMission->m_nRewardItem4Num = ptoMission.reward_item_4_num();
		pMission->m_nRewardItem5 = ptoMission.reward_item_5_id();
		pMission->m_nRewardItem5Num = ptoMission.reward_item_5_num();
		pMission->m_nRewardItem6 = ptoMission.reward_item_6_id();
		pMission->m_nRewardItem6Num = ptoMission.reward_item_6_num();
		pMission->m_nRewardEquip1 = ptoMission.reward_equip_1_id();
		pMission->m_nRewardEquip2 = ptoMission.reward_equip_2_id();
		pMission->m_nRewardEquip3 = ptoMission.reward_equip_3_id();
		pMission->m_nRewardEquip4 = ptoMission.reward_equip_4_id();
		pMission->m_nBeginStoryID = ptoMission.begin_story_id();
		pMission->m_nFinishStoryID = ptoMission.finish_story_id();
		pMission->m_nOpenFunction = ptoMission.open_function_id();
		pMission->m_nRewardSoldier = ptoMission.reward_soldier_id();

		pMission->m_nOfferSilver = ptoMission.offer_silver();
		pMission->m_nOfferHonor = ptoMission.offer_honor();
		pMission->m_nOfferReputation = ptoMission.offer_reputation();
		pMission->m_nOfferStamina = ptoMission.offer_stamina();
		pMission->m_nOfferEquip1 = ptoMission.offer_equip_1();
		pMission->m_nOfferEquip2 = ptoMission.offer_equip_2();
		pMission->m_nOfferSoldier = ptoMission.offer_soldier();
		pMission->m_nOfferHero = ptoMission.offer_hero();
		pMission->m_nOfferEx = ptoMission.offer_exp();

		auto it = m_mapMissions.insert(std::make_pair(pMission->m_nID, pMission));
		if (false == it.second)
			delete pMission;
	}

	printf(FormatString("加载", m_mapMissions.size(), "个任务\n").c_str());
}

const CMissionTP* CMissionTP::GetMission(int nID)
{
	auto it = m_mapMissions.find(nID);
	if (m_mapMissions.cend() == it)
		return nullptr;
	else
		return it->second;
}

const CMissionTP* CMissionTP::GetMissionByMainProgress(int nProgress)
{
	for (auto &it : m_mapMissions)
	if (nProgress == it.second->m_nNeedMainProgress)
		return it.second;

	return nullptr;
}

void CMissionTP::AvailableBranchList(std::vector<int>* branch, int player_level, int main_progress)
{
	for (auto it : m_mapMissions)
	{
		if (it.second->m_nNeedLevel <= player_level && it.second->m_nNeedMainProgress <= main_progress && it.second->GetType() == 2) 
			branch->push_back(it.first);
	}
}