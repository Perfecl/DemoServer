#include "stdafx.h"
#include "LotteryChance.h"

std::map<int, const CLotteryChance*> CLotteryChance::ms_mapLotteryChanceLibrary;

void CLotteryChance::Load()
{
	dat_LOTTERY_LIBRARY_All libLotteryAll;
	libLotteryAll.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Lottery.txt"));

	for (int i = 0; i < libLotteryAll.lottery_chance_library_size(); i++)
	{
		CLotteryChance* pLotteryChance = new CLotteryChance;
		pLotteryChance->m_nID = libLotteryAll.lottery_chance_library(i).id();
		pLotteryChance->m_nType = libLotteryAll.lottery_chance_library(i).type();
		pLotteryChance->m_nBaseNum = libLotteryAll.lottery_chance_library(i).base_num();
		pLotteryChance->m_nMul = libLotteryAll.lottery_chance_library(i).mul();
		pLotteryChance->m_fChance = libLotteryAll.lottery_chance_library(i).chance();

		ms_mapLotteryChanceLibrary.insert(std::make_pair(pLotteryChance->m_nID, pLotteryChance));
	}
}

const CLotteryChance* CLotteryChance::GetLotteryChance(int nID)
{
	auto it = ms_mapLotteryChanceLibrary.find(nID);

	if (ms_mapLotteryChanceLibrary.cend() == it)
		return nullptr;
	else
		return it->second;
}