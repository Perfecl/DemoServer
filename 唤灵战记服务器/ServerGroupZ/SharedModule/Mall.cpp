#include "stdafx.h"
#include "Mall.h"

std::map<int, const CMall*> CMall::mall_library_;
std::map<int, const float> CMall::discount_library_;


CMall::CMall()
{

}

CMall::~CMall()
{

}

void CMall::Load()
{
	dat_MALL_STRUCT_MallLibrary mall_library;
	mall_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Mall.txt"));

	for (int i = 0; i < mall_library.mall_library_size(); i++)
	{
		auto pto_mall = mall_library.mall_library(i);

		if (0 == pto_mall.id())
			continue;

		CMall* mall = new CMall;
		mall->id_ = pto_mall.id();
		mall->item_id_ = pto_mall.item_id();
		mall->num_ = pto_mall.num();
		mall->currency_type_ = pto_mall.currency_type();
		mall->currency_parameter_ = pto_mall.currency_parameter();
		mall->currency_num_ = pto_mall.currency_num();
		mall->page_type_ = pto_mall.page_tyype();

		auto it = mall_library_.insert(std::make_pair(mall->id_, mall));
		if (false == it.second)
			delete mall;
	}

	for (int i = 0; i < mall_library.discount_library_size(); i++)
	{
		auto pto_discount = mall_library.discount_library(i);
		if (!pto_discount.mall_id())
			continue;
		auto it = discount_library_.insert(std::make_pair(pto_discount.mall_id(), pto_discount.discount_percent()));
	}

	printf(FormatString("加载", mall_library_.size(), "商城条目\n").c_str());
}

const CMall* CMall::GetMall(int id)
{
	auto it = mall_library_.find(id);

	if (mall_library_.cend() == it)
		return nullptr;
	else
		return it->second;

}

const float  CMall::GetDiscount(int id)
{
	auto it = discount_library_.find(id);

	if (discount_library_.cend() == it)
	{
		return 1;
	}
	else
		return it->second;
}
