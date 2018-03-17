#include "stdafx.h"
#include "ItemTP.h"

std::map<int, const CItemTP*> CItemTP::ms_mapItemTPs;

const CItemTP* CItemTP::GetItemTP(int nID)
{
	if (nID <= 0)
		return nullptr;

	auto it = ms_mapItemTPs.find(nID);
	if (ms_mapItemTPs.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CItemTP::Load()
{
	Item_Library libItems;
	libItems.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Item.txt"));

	for (int i = 0; i < libItems.item_librarys_size(); i++)
	{
		auto ptoItem = libItems.item_librarys(i);

		if (0 == ptoItem.id())
			continue;

		CItemTP *pItem = new CItemTP;

		pItem->m_nID = ptoItem.id();
		pItem->m_nName = ptoItem.name();
		pItem->m_nLevel = ptoItem.item_lv();
		pItem->m_nIconID = ptoItem.icon_id();
		pItem->m_enType = (ITEM_ATTRIBUTE)ptoItem.type();
		pItem->m_nFunctional = ptoItem.function_id();
		pItem->m_nUseLevel = ptoItem.use_lv();
		pItem->m_nMaxNum = ptoItem.max_num();
		pItem->m_nPrice = ptoItem.price();
		pItem->m_nSellPrice = ptoItem.sell_price();

		auto it = ms_mapItemTPs.insert(std::make_pair(pItem->m_nID, pItem));
		if (false == it.second)
			delete pItem;
	}

	printf(FormatString("加载", ms_mapItemTPs.size(), "个物品\n").c_str());
}