#include "stdafx.h"
#include "CombinationReward.h"

map<int, const CCombinationReward*> CCombinationReward::ms_mapCombinationReward;

CCombinationReward::CCombinationReward()
{
}


CCombinationReward::~CCombinationReward()
{
}

void CCombinationReward::Load()
{
	Pray_Card_Library libPrayCard;
	libPrayCard.ParseFromString(GetDataFromFile(GAME_DATA_PATH"PrayCard.txt"));

	for (int i = 0; i < libPrayCard.combination_reward_library_size(); i++)
	{
		CCombinationReward* pCombinationReward = new CCombinationReward;
		pCombinationReward->m_nLevel = libPrayCard.combination_reward_library(i).lv();
		pCombinationReward->m_nCards_2 = libPrayCard.combination_reward_library(i).cards_2();
		pCombinationReward->m_nCards_3 = libPrayCard.combination_reward_library(i).cards_3();
		pCombinationReward->m_nCards_4 = libPrayCard.combination_reward_library(i).cards_4();

		ms_mapCombinationReward.insert(std::make_pair(pCombinationReward->m_nLevel, pCombinationReward));
	}
}

const long long CCombinationReward::GetCombinationReward(int nLV, int nCardNum)
{
	auto it = ms_mapCombinationReward.find(nLV);
	if (ms_mapCombinationReward.cend() != it)
	{
		switch (nCardNum)
		{
		case 2:
			return it->second->m_nCards_2;
		case 3:
			return it->second->m_nCards_3;
		case 4:
			return it->second->m_nCards_4;
		default:
			return 0;
		}
	}
	else
		return 0;
}