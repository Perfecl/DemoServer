#include "stdafx.h"
#include "StageLoot.h"

std::vector<const CStageLoot*> CStageLoot::stage_loot_lib_;
CStageLoot::CStageLoot()
{
}


CStageLoot::~CStageLoot()
{
}

void CStageLoot::Load()
{
	dat_STAGE_STRUCT_StageLootLibrary stage_loot_lib;
	stage_loot_lib.ParseFromString(GetDataFromFile(GAME_DATA_PATH"StageLoot.txt"));

	for (int i = 0; i < stage_loot_lib.stage_loot_library_size(); i++)
	{
		auto pto_loot = stage_loot_lib.stage_loot_library(i);

		if (0 == pto_loot.id())
			continue;

		CStageLoot* new_loot = new CStageLoot;
		new_loot->id_ = pto_loot.id();
		new_loot->odds_ = pto_loot.odds();
		new_loot->item_id_ = pto_loot.item_id();
		new_loot->num_ = pto_loot.num();
		new_loot->bullet_id_ = pto_loot.bullet_id();

		stage_loot_lib_.push_back(new_loot);
	}
	printf(FormatString("º”‘ÿ", stage_loot_lib_.size(), "Ãıπÿø®µÙ¬‰\n").c_str());
}

int CStageLoot::ProduceBulletID()
{
	int bullet_id{ 0 };
	if (stage_loot_lib_.size() <= 0)
		return 0;
	int rand = GetRandom(0, 100);

	int step = 0;
	for (size_t i = 0; i < stage_loot_lib_.size(); i++)
	{
		const CStageLoot* loot = stage_loot_lib_.at(i);
		if (rand <= loot->odds_ * 100 + step)
		{
			return loot->bullet_id_;
		}
		else
		{
			step += static_cast<int>(loot->odds_ * 100);
		}
			
	}
	return 0;
}

const CStageLoot* CStageLoot::GetStageLootByBulletID(int bullet_id)
{
	for (auto it : stage_loot_lib_)
	{
		if (it->bullet_id_ == bullet_id)
			return it;
	}
	return nullptr;
}

bool CStageLoot::IsStageLoot(int item_id)
{
	for (auto it : stage_loot_lib_)
	{
		if (it->item_id_ == item_id)
			return true;
	}
	return false;
}