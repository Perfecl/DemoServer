#include "stdafx.h"
#include "Lottery.h"

map<int, const CLottery*> CLottery::ms_mapLotteryLibrary;

CLottery::CLottery()
{
}


CLottery::~CLottery()
{
}

void CLottery::Load()
{
	dat_LOTTERY_LIBRARY_All libLotteryAll;
	libLotteryAll.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Lottery.txt"));

	for (int i = 0; i < libLotteryAll.lottery_library_size(); i++)
	{
		CLottery* pLottery = new CLottery;
		pLottery->m_nID = libLotteryAll.lottery_library(i).id();
		pLottery->m_nExpendItemID = libLotteryAll.lottery_library(i).expend_item_id();
		pLottery->m_nExpendItemNum = libLotteryAll.lottery_library(i).expend_item_num();
		pLottery->m_nLootLevel = libLotteryAll.lottery_library(i).loot_level();

		pLottery->m_arrCertainLoot.at(0).id_ = libLotteryAll.lottery_library(i).certain_loot_1_id();
		pLottery->m_arrCertainLoot.at(0).num_ = libLotteryAll.lottery_library(i).certain_loot_1_num();

		pLottery->m_arrCertainLoot.at(1).id_ = libLotteryAll.lottery_library(i).certain_loot_2_id();
		pLottery->m_arrCertainLoot.at(1).num_ = libLotteryAll.lottery_library(i).certain_loot_2_num();

		pLottery->m_arrCertainLoot.at(2).id_ = libLotteryAll.lottery_library(i).certain_loot_3_id();
		pLottery->m_arrCertainLoot.at(2).num_ = libLotteryAll.lottery_library(i).certain_loot_3_num();

		pLottery->m_arrCertainLoot.at(3).id_ = libLotteryAll.lottery_library(i).certain_loot_4_id();
		pLottery->m_arrCertainLoot.at(3).num_ = libLotteryAll.lottery_library(i).certain_loot_4_num();

		pLottery->m_arrCertainLoot.at(4).id_ = libLotteryAll.lottery_library(i).certain_loot_5_id();
		pLottery->m_arrCertainLoot.at(4).num_ = libLotteryAll.lottery_library(i).certain_loot_5_num();

		pLottery->m_arrCertainLoot.at(5).id_ = libLotteryAll.lottery_library(i).certain_loot_6_id();
		pLottery->m_arrCertainLoot.at(5).num_ = libLotteryAll.lottery_library(i).certain_loot_6_num();

		for (int k = 0; k < 8 && k < libLotteryAll.lottery_library(i).chance_loot_size(); k++)
		{
			pLottery->m_arrChanceLoot.at(k).m_nID = libLotteryAll.lottery_library(i).chance_loot(k).id();
			pLottery->m_arrChanceLoot.at(k).num_ = libLotteryAll.lottery_library(i).chance_loot(k).num();
			pLottery->m_arrChanceLoot.at(k).m_fChance = libLotteryAll.lottery_library(i).chance_loot(k).chance();
		}

		ms_mapLotteryLibrary.insert(std::make_pair(pLottery->m_nID, pLottery));
	}
}

const CLottery* CLottery::GetLottery(int nID)
{
	auto it = ms_mapLotteryLibrary.find(nID);

	if (ms_mapLotteryLibrary.cend() == it)
		return nullptr;
	else
		return it->second;
}