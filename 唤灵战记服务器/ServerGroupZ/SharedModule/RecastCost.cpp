#include "stdafx.h"
#include "RecastCost.h"

std::map<int, int> CRecastCost::recast_cost_lib_;

CRecastCost::CRecastCost()
{
}


CRecastCost::~CRecastCost()
{
}

void CRecastCost::Load()
{
	dat_MAKEEQUIP_STRUCT_MakeEquipLibrary make_equip_library;
	make_equip_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"MakeEquip.txt"));

	for (int i = 0; i < make_equip_library.recast_cost_library_size(); i++)
	{
		auto pto_recast_cost = make_equip_library.recast_cost_library(i);
		if (0 == pto_recast_cost.quality())
			continue;

		auto it = recast_cost_lib_.insert(std::make_pair(pto_recast_cost.quality(), pto_recast_cost.stone_num()));
	}
	printf(FormatString("加载", recast_cost_lib_.size(), "个重铸花费\n").c_str());
}

int CRecastCost::GetStoneNum(int quality)
{
	auto it = recast_cost_lib_.find(quality);
	if (it != recast_cost_lib_.cend())
		return it->second;
	return 0;
}
