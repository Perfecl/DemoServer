#include "stdafx.h"
#include "SuitTP.h"

std::map<int, const CSuitTP*> CSuitTP::suit_tp_;
CSuitTP::CSuitTP()
{
}


CSuitTP::~CSuitTP()
{
}

void CSuitTP::Load()
{
	Equip_Library lib_new_equip;
	lib_new_equip.ParseFromString(GetDataFromFile(GAME_DATA_PATH"NewEquip.txt"));

	for (int i = 0; i < lib_new_equip.suit_librarys_size(); i++)
	{
		auto pto_suit = lib_new_equip.suit_librarys(i);

		if (0 == pto_suit.id())
			continue;

		CSuitTP* new_suit = new CSuitTP;
		new_suit->id_ = pto_suit.id();
		new_suit->attribute_type_2_ = (SuitAttributrType)pto_suit.attribute_type_2();
		new_suit->attribute_2_ = pto_suit.attribute_2();
		new_suit->stage_2_ = pto_suit.stage_2();
		new_suit->attribute_type_4_ = (SuitAttributrType)pto_suit.attribute_type_4();
		new_suit->attribute_4_ = pto_suit.attribute_4();
		new_suit->stage_4_ = pto_suit.stage_4();
		new_suit->skill_id_ = pto_suit.skill_id();

		auto it = suit_tp_.insert(std::make_pair(new_suit->id_, new_suit));
		if (false == it.second)
			delete new_suit;
	}
	printf(FormatString("加载", suit_tp_.size(), "个套装信息\n").c_str());
}

const CSuitTP* CSuitTP::GetSuitTP(int id)
{
	auto it = suit_tp_.find(id);

	if (suit_tp_.cend() == it)
		return nullptr;
	else
		return it->second;
}

SuitAttributrType CSuitTP::attribute_type(int suit_num) const
{
	if (2 == suit_num)
		return attribute_type_2_;
	if (4 == suit_num)
		return attribute_type_4_;
	return SuitAttributrType::kSuitNull;
}

float CSuitTP::attribute(int suit_num) const
{
	if (2 == suit_num)
		return attribute_2_;
	if (4 == suit_num)
		return attribute_4_;
	return 0;
}

float CSuitTP::stage(int suit_num) const 
{
	if (2 == suit_num)
		return stage_2_;
	if (4 == suit_num)
		return stage_4_;
	return 0;
}