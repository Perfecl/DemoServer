#include "stdafx.h"
#include "Blueprint.h"

std::map<int, const CBlueprint*> CBlueprint::blueprint_library_;
CBlueprint::CBlueprint()
{
	rank_odds_.fill(0);
}


CBlueprint::~CBlueprint()
{
}

void CBlueprint::Load()
{
	dat_MAKEEQUIP_STRUCT_MakeEquipLibrary make_equip_library;
	make_equip_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"MakeEquip.txt"));

	for (int i = 0; i < make_equip_library.blueprint_library_size(); i++)
	{
		auto pto_equip = make_equip_library.blueprint_library(i);

		if (0 == pto_equip.id())
			continue;

		CBlueprint* new_blueprint = new CBlueprint;
		new_blueprint->id_ = pto_equip.id();
		new_blueprint->target_id_ = pto_equip.target_id();
		new_blueprint->material_1_id_ = pto_equip.material_1_id();
		new_blueprint->material_1_num_ = pto_equip.material_1_num();
		new_blueprint->material_2_id_ = pto_equip.material_2_id();
		new_blueprint->material_2_num_ = pto_equip.material_2_num();
		new_blueprint->rank_odds_[0] = pto_equip.green_odds();
		new_blueprint->rank_odds_[1] = pto_equip.blue_odds();
		new_blueprint->rank_odds_[2] = pto_equip.purple_odds();
		new_blueprint->rank_odds_[3] = pto_equip.orange_odds();
		new_blueprint->rank_odds_[4] = pto_equip.red_odds();

		auto it = blueprint_library_.insert(std::make_pair(new_blueprint->id_, new_blueprint));
		if (false == it.second)
			delete new_blueprint;
	}
	printf(FormatString("¼ÓÔØ", blueprint_library_.size(), "¸öÍ¼Ö½\n").c_str());
}

const CBlueprint* CBlueprint::GetBlueprint(int id)
{
	auto it = blueprint_library_.find(id);

	if (blueprint_library_.cend() == it)
		return nullptr;
	else
		return it->second;
}

EquipRankColor CBlueprint::ProduceEquipRankColor() const
{
	int random_num = GetRandom(0, 100);
	int rank_odds = 0;
	for (size_t i = 0; i < 5; i++)
	{
		rank_odds += int(rank_odds_[i] * 100);
		if (random_num < (rank_odds))
			return (EquipRankColor)(i + 1);
	}
	return EquipRankColor::kEquipRankWhite;
}
