#include "stdafx.h"
#include "Gift.h"

std::map<int, SPGift> CGift::gift_library;
CGift::CGift()
{
}


CGift::~CGift()
{
}

void CGift::Load()
{
	dat_GIFT_STRUCT_GifLibrary pto_gift_library;
	pto_gift_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Gift.txt"));

	for (int i = 0; i < pto_gift_library.gift_library_size(); i++)
	{
		SPGift gift = std::make_shared<CGift>();
		gift->id = pto_gift_library.gift_library(i).id();
		for (int k = 0; k < pto_gift_library.gift_library(i).item_size(); k++)
		{
			int id = pto_gift_library.gift_library(i).item(k).id();
			if (id)
			{
				int num = pto_gift_library.gift_library(i).item(k).num();
				gift->item.push_back(std::make_pair(id, num));
			}
		}

		gift_library.insert(std::make_pair(gift->id, gift));
	}
}

SPGift CGift::GetGift(int id)
{
	auto it = gift_library.find(id);
	if (gift_library.cend() == it)
		return nullptr;
	else
		return it->second;
}