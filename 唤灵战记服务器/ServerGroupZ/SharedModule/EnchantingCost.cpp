#include "stdafx.h"
#include "EnchantingCost.h"

std::map<int, const CEnchantingCost*> CEnchantingCost::enchanting_cost_lirary_;
CEnchantingCost::CEnchantingCost()
{
}


CEnchantingCost::~CEnchantingCost()
{
}

void CEnchantingCost::Load()
{
	dat_MAKEEQUIP_STRUCT_MakeEquipLibrary make_equip_library;
	make_equip_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"MakeEquip.txt"));

	for (int i = 0; i < make_equip_library.enchanting_cost_library_size(); i++)
	{
		auto pto_enchanting_cost = make_equip_library.enchanting_cost_library(i);

		if (0 == pto_enchanting_cost.equip_stage())
			continue;

		CEnchantingCost* enchanting_cost = new CEnchantingCost;
		enchanting_cost->equip_stage_ = pto_enchanting_cost.equip_stage();
		enchanting_cost->enchanting_stone_id_ = pto_enchanting_cost.enchanting_stone_id();
		enchanting_cost->silver_ = pto_enchanting_cost.silver();

		auto it = enchanting_cost_lirary_.insert(std::make_pair(enchanting_cost->equip_stage_, enchanting_cost));
		if (false == it.second)
			delete enchanting_cost;
	}
	printf(FormatString("加载", enchanting_cost_lirary_.size(), "个精炼花费\n").c_str());
}

const CEnchantingCost* CEnchantingCost::GetEnchantingCost(int stage)
{
	auto it = enchanting_cost_lirary_.find(stage);

	if (it == enchanting_cost_lirary_.cend())
		return nullptr;
	else
		return it->second;
}