#include "stdafx.h"
#include "FashionTP.h"

std::map<int, const CFashionTP*>  CFashionTP::fashion_tp_;

CFashionTP::CFashionTP()
{
}


CFashionTP::~CFashionTP()
{
}

void CFashionTP::Load()
{
	dat_Library_Fashion lib_fashion;
	lib_fashion.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Fashion.txt"));

	for (int i = 0; i < lib_fashion.fashion_library_size(); i++)
	{
		auto pto_equip = lib_fashion.fashion_library(i);

		if (0 == pto_equip.id())
			continue;

		CFashionTP* fashion = new CFashionTP;
		fashion->id_ = lib_fashion.fashion_library(i).id();
		fashion->type_ = lib_fashion.fashion_library(i).type();
		fashion->renew_ = lib_fashion.fashion_library(i).renew();
		fashion->price_ = lib_fashion.fashion_library(i).price();
		fashion->hero_atk_ = lib_fashion.fashion_library(i).hero_atk();
		fashion->hero_matk_ = lib_fashion.fashion_library(i).hero_matk();
		fashion->hero_def_ = lib_fashion.fashion_library(i).hero_def();
		fashion->hero_mdef_ = lib_fashion.fashion_library(i).hero_mdef();
		fashion->hero_hp_ = lib_fashion.fashion_library(i).hero_hp();
		fashion->unit_atk_ = lib_fashion.fashion_library(i).unit_atk();
		fashion->unit_matk_ = lib_fashion.fashion_library(i).unit_matk();
		fashion->unit_def_ = lib_fashion.fashion_library(i).unit_def();
		fashion->unit_mdef_ = lib_fashion.fashion_library(i).unit_mdef();
		fashion->unit_hp_ = lib_fashion.fashion_library(i).unit_hp();

		auto it = fashion_tp_.insert(std::make_pair(fashion->id_, fashion));
		if (false == it.second)
			delete fashion;
	}
	printf(FormatString("加载", fashion_tp_.size(), "个时装\n").c_str());
}

const CFashionTP* CFashionTP::GetFashionTP(int id)
{
	auto it = fashion_tp_.find(id);

	if (it == fashion_tp_.cend())
		return nullptr;
	else
		return it->second;
}