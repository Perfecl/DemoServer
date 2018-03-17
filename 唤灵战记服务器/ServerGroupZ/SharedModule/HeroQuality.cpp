#include "stdafx.h"
#include "HeroQuality.h"

std::map<int, const CHeroQuality*>	CHeroQuality::hero_quality_lib_;

CHeroQuality::CHeroQuality()
{
}

CHeroQuality::~CHeroQuality()
{
}

void CHeroQuality::Load()
{
	dat_UNIT_Library_All datAll;
	datAll.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Unit.txt"));

	for (int i = 0; i < datAll.hero_quality_size(); i++)
	{
		auto pto_hero_quality = datAll.hero_quality(i);

		if (0 == pto_hero_quality.lv())
			continue;

		CHeroQuality* hero_quality = new CHeroQuality;
		hero_quality->lv_ = pto_hero_quality.lv();
		hero_quality->chance_ = pto_hero_quality.chance();
		hero_quality->time_ = pto_hero_quality.time();
		hero_quality->hp_ = pto_hero_quality.hp();
		hero_quality->atk_ = pto_hero_quality.atk();
		hero_quality->def_ = pto_hero_quality.def();
		hero_quality->matk_ = pto_hero_quality.matk();
		hero_quality->mdef_ = pto_hero_quality.mdef();
		hero_quality->strength_ = pto_hero_quality.strength();
		hero_quality->command_ = pto_hero_quality.command();
		hero_quality->intelligence_ = pto_hero_quality.intelligence();
		hero_quality->min_safety_ = pto_hero_quality.min_safety();

		auto it = hero_quality_lib_.insert(std::make_pair(hero_quality->lv_, hero_quality));

		if (false == it.second)
			delete hero_quality;
	}

	printf(FormatString("加载", hero_quality_lib_.size(), "个英雄品质\n").c_str());
}

const CHeroQuality* CHeroQuality::GetHeroQuality(int quality)
{
	auto it = hero_quality_lib_.find(quality);
	if (it != hero_quality_lib_.cend())
		return it->second;
	return nullptr;
}

bool CHeroQuality::QualityUp(int quality, float ex_chance)
{
	const CHeroQuality* hero_quality = CHeroQuality::GetHeroQuality(quality + 1);
	if (!quality)
		return false;
	float final_chance = hero_quality->chance_ + ex_chance;
	if (GetRandom(0, 10000) <= (int)(final_chance * 10000))
		return true;
	return false;
}