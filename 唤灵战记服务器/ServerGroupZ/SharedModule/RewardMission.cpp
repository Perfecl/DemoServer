#include "stdafx.h"
#include "RewardMission.h"

map<int, const  SRewardMission*> CRewardMission::ms_mapRewardMission;
map<int, const  SRewardMissionRank*> CRewardMission::ms_mapRewardMissionRank;
map<int, const  SRewardMissionResource*> CRewardMission::ms_mapRewardMissionResource;
std::vector<int> CRewardMission::ms_vctMissionType;

void CRewardMission::Load()
{
	dat_REWARDMISSION_STRUCT_Library libAll;
	libAll.ParseFromString(GetDataFromFile(GAME_DATA_PATH"RewardMission.txt"));

	for (int i = 0; i < libAll.reward_mission_library_size(); i++)
	{
		SRewardMission* pMission = new SRewardMission;
		pMission->id_ = libAll.reward_mission_library(i).id();
		pMission->m_enType = (RewardMissionType)libAll.reward_mission_library(i).type();
		pMission->mission_target_num_ = libAll.reward_mission_library(i).target_num();
		pMission->m_nRewardType = libAll.reward_mission_library(i).reward_type();
		pMission->m_fRewardMul = libAll.reward_mission_library(i).reward_mul();
		pMission->condition_ = libAll.reward_mission_library(i).condition();
		pMission->value_ = libAll.reward_mission_library(i).value();

		ms_mapRewardMission.insert(std::make_pair((int)pMission->m_enType, pMission));

		ms_vctMissionType.push_back(pMission->m_enType);
	}

	for (int i = 0; i < libAll.rank_library_size(); i++)
	{
		SRewardMissionRank* pRank = new SRewardMissionRank;
		pRank->m_nRank = libAll.rank_library(i).rank();
		pRank->m_fProbability = libAll.rank_library(i).probability();
		pRank->m_fMul = libAll.rank_library(i).mul();

		ms_mapRewardMissionRank.insert(std::make_pair(pRank->m_nRank, pRank));
	}

	for (int i = 0; i < libAll.reward_resource_library_size(); i++)
	{
		SRewardMissionResource* pResource = new SRewardMissionResource;
		pResource->m_nType = libAll.reward_resource_library(i).type();
		pResource->m_nFixedValue = libAll.reward_resource_library(i).fixed_value();

		ms_mapRewardMissionResource.insert(std::make_pair(pResource->m_nType, pResource));
	}
}

int CRewardMission::ProduceRewardMissionRank()
{
	while (true)
	{
		int nRank = GetRandom(1, 4);
		auto it = ms_mapRewardMissionRank.find(nRank);
		if (ms_mapRewardMissionRank.cend() != it)
		if (GetRandom(0,100) <= (it->second->m_fProbability * 100))
		{
			return nRank;
		}
	}
}

RewardMissionType CRewardMission::ProduceRewardMissionType(int level, int main_progress)
{
	std::vector<RewardMissionType> temp_type;
	for (size_t i = 0; i < ms_vctMissionType.size(); i++)
	{
		const SRewardMission* reward_mission = CRewardMission::GetRewardMissionStruce((RewardMissionType)ms_vctMissionType.at(i));
		if (reward_mission)
		{
			int max{ 0 };
			if (reward_mission->condition_ == 1)
				max = main_progress;
			if (reward_mission->condition_ == 2)
				max = level;
			if (max >= reward_mission->value_)
				temp_type.push_back(reward_mission->m_enType);
		}
	}
	if (temp_type.size() <= 0)
		return RMT_Null;
	if (temp_type.size() == 1)
		return temp_type.at(0);
	return (RewardMissionType)temp_type[GetRandom<int>(0, temp_type.size() - 1)];
}

int CRewardMission::GetRewardMissionTargetNum(RewardMissionType enType)
{
	auto it = ms_mapRewardMission.find(enType);
	if (ms_mapRewardMission.cend() == it)
		return 0;
	else
		return it->second->mission_target_num_;
}


float CRewardMission::GetRankMul(int nRank)
{
	auto it = ms_mapRewardMissionRank.find(nRank);
	if (ms_mapRewardMissionRank.cend() == it)
		return 0;
	else
		return it->second->m_fMul;
}


int CRewardMission::GetFixedValue(int type)
{
	auto it = ms_mapRewardMissionResource.find(type);
	if (ms_mapRewardMissionResource.cend() == it)
		return 0;
	else
		return it->second->m_nFixedValue;
}


const SRewardMission* CRewardMission::GetRewardMissionStruce(RewardMissionType enType)
{
	auto it = ms_mapRewardMission.find(enType);
	if (ms_mapRewardMission.cend() == it)
		return nullptr;
	else
		return it->second;
}