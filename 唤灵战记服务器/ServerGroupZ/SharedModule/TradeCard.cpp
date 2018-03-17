#include "stdafx.h"
#include "TradeCard.h"

map<int, const CTradeCard*> CTradeCard::ms_mapTradeCard;

void CTradeCard::Load()
{
	Pray_Card_Library libPrayCard;
	libPrayCard.ParseFromString(GetDataFromFile(GAME_DATA_PATH"PrayCard.txt"));

	for (int i = 0; i < libPrayCard.pray_card_library_size(); i++)
	{
		CTradeCard* pTradeCard = new CTradeCard;
		pTradeCard->m_nID = libPrayCard.pray_card_library(i).id();
		pTradeCard->m_nRewardType = libPrayCard.pray_card_library(i).type();
		pTradeCard->m_nRewardBase = libPrayCard.pray_card_library(i).base_reward();
		pTradeCard->m_nRewardMul = libPrayCard.pray_card_library(i).reward_override();

		ms_mapTradeCard.insert(make_pair(pTradeCard->m_nID, pTradeCard));
	}
}

const CTradeCard* CTradeCard::GetTradeCard(int nID)
{
	auto it = ms_mapTradeCard.find(nID);
	if (ms_mapTradeCard.cend() == it)
		return nullptr;
	else
		return it->second;
}
