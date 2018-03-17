#include "stdafx.h"
#include "ResolveTP.h"

std::map<int, const CResolveTP*> CResolveTP::resolve_library_;

CResolveTP::CResolveTP()
{
}

CResolveTP::~CResolveTP()
{
}

void CResolveTP::Load()
{
	dat_MAKEEQUIP_STRUCT_MakeEquipLibrary make_equip_library;
	make_equip_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"MakeEquip.txt"));

	for (int i = 0; i < make_equip_library.resolve_library_size(); i++)
	{
		auto pto_resolve = make_equip_library.resolve_library(i);

		if (0 == pto_resolve.quality())
			continue;

		CResolveTP* resolve = new CResolveTP;

		resolve->quality_ = pto_resolve.quality();
		resolve->enchant_chance_ = pto_resolve.enchant_chance();
		resolve->enchant_min_ = pto_resolve.enchant_min();
		resolve->enchant_max_ = pto_resolve.enchant_max();
		resolve->catalyze_chance_ = pto_resolve.catalyze_chance();
		resolve->catalyze_min_ = pto_resolve.catalyze_min();
		resolve->catalyze_max_ = pto_resolve.catalyze_max();

		auto it = resolve_library_.insert(std::make_pair(resolve->quality_, resolve));
		if (false == it.second)
			delete resolve;
	}
	printf(FormatString("加载", resolve_library_.size(), "个装备分解\n").c_str());
}

const CResolveTP* CResolveTP::GetResolveTP(int quality)
{
	auto it = resolve_library_.find(quality);
	if (it != resolve_library_.cend())
		return it->second;
	return nullptr;
}