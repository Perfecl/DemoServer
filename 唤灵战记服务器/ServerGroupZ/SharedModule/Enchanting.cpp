#include "stdafx.h"
#include "Enchanting.h"

std::map<int, const CEnchanting*>CEnchanting::enchanting_library_;
std::vector<int> CEnchanting::enchanting_id_;

CEnchanting::CEnchanting()
{

}

CEnchanting::~CEnchanting()
{

}

void CEnchanting::Load()
{
	dat_MAKEEQUIP_STRUCT_MakeEquipLibrary make_equip_library;
	make_equip_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"MakeEquip.txt"));

	for (int i = 0; i < make_equip_library.enchanting_library_size(); i++)
	{
		auto pto_enchanting = make_equip_library.enchanting_library(i);

		if (0 == pto_enchanting.id())
			continue;

		CEnchanting* enchanting = new CEnchanting;
		enchanting->id_ = pto_enchanting.id();
		enchanting->min_base_ = pto_enchanting.min_base();
		enchanting->min_parameter_ = pto_enchanting.min_parameter();
		enchanting->max_base_ = pto_enchanting.max_base();
		enchanting->max_parameter_ = pto_enchanting.max_parameter();
		enchanting->quality_parameter_ = pto_enchanting.quality_parameter();

		auto it = enchanting_library_.insert(std::make_pair(enchanting->id_, enchanting));
		if (false == it.second)
			delete enchanting;
		else
			enchanting_id_.push_back(enchanting->id_);
	}
	printf(FormatString("加载", enchanting_library_.size(), "个精炼公式\n").c_str());
}

int CEnchanting::RandomEnchantingID()
{
	if (enchanting_id_.size() < 1)
		return 0;

	if (enchanting_id_.size() == 1)
		return enchanting_id_.at(0);

	int pos = GetRandom(0, (int)(enchanting_id_.size() - 1));
	return enchanting_id_.at(pos);
}

const CEnchanting* CEnchanting::GetEnchanting(int id)
{
	auto it = enchanting_library_.find(id);

	if (it == enchanting_library_.cend())
		return nullptr;
	else
		return it->second;
}