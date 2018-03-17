#include "stdafx.h"
#include "OpenTP.h"

std::map<int, const COpenTP*> COpenTP::ms_mapOpenTPs;

COpenTP::COpenTP()
{
}

COpenTP::~COpenTP()
{
}

void COpenTP::Load()
{
	Item_Library libItems;
	libItems.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Item.txt"));

	for (int i = 0; i < libItems.open_library_size(); i++)
	{
		COpenTP *pOpen = new COpenTP;

		pOpen->m_nOpenID= libItems.open_library(i).open_id();

		pOpen->m_nRewardExp= libItems.open_library(i).reward_exp();    
		pOpen->m_nRewardSilver= libItems.open_library(i).reward_silver();    
		pOpen->m_nRewardHonour= libItems.open_library(i).reward_honour();    
		pOpen->m_nRewardReputation= libItems.open_library(i).reward_reputation();
		pOpen->m_nRewardGold= libItems.open_library(i).reward_gold();    
		pOpen->m_nRewardStamina= libItems.open_library(i).reward_stamina();  

		pOpen->m_nRewardItem1ID= libItems.open_library(i).reward_item_1_id(); 
		pOpen->m_nRewardItem1Num= libItems.open_library(i).reward_item_1_num(); 
		pOpen->m_nRewardItem2ID= libItems.open_library(i).reward_item_2_id();
		pOpen->m_nRewardItem2Num = libItems.open_library(i).reward_item_2_num();
		pOpen->m_nRewardItem3ID = libItems.open_library(i).reward_item_3_id();
		pOpen->m_nRewardItem3Num = libItems.open_library(i).reward_item_3_num();
		pOpen->m_nRewardItem4ID = libItems.open_library(i).reward_item_4_id();
		pOpen->m_nRewardItem4Num = libItems.open_library(i).reward_item_4_num();
		pOpen->m_nRewardItem5ID = libItems.open_library(i).reward_item_5_id();
		pOpen->m_nRewardItem5Num = libItems.open_library(i).reward_item_5_num();
		pOpen->m_nRewardItem6ID = libItems.open_library(i).reward_item_6_id();
		pOpen->m_nRewardItem6Num = libItems.open_library(i).reward_item_6_num();

		pOpen->m_nRewardEquip1ID= libItems.open_library(i).reward_equip_1_id();
		pOpen->m_nRewardEquip2ID = libItems.open_library(i).reward_equip_1_id();
		pOpen->m_nRewardEquip3ID = libItems.open_library(i).reward_equip_1_id();
		pOpen->m_nRewardEquip4ID = libItems.open_library(i).reward_equip_1_id();

		ms_mapOpenTPs.insert(std::make_pair(pOpen->m_nOpenID, pOpen));
	}
}

const COpenTP* COpenTP::GetOpenTP(int nID)
{
	auto it = ms_mapOpenTPs.find(nID);
	if (ms_mapOpenTPs.cend() == it)
		return nullptr;
	else
		return it->second;
}

