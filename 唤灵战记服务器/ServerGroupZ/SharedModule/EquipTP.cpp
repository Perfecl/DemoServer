#include "stdafx.h"
#include "EquipTP.h"

std::map<int, const CEquipTP*>				CEquipTP::new_equip_tp_;
std::map<int, const ImproveEquipPrice*>		CEquipTP::improve_equip_price;

CEquipTP::CEquipTP()
{

}

CEquipTP::~CEquipTP()
{

}

void CEquipTP::Load()
{
	Equip_Library lib_new_equip;
	lib_new_equip.ParseFromString(GetDataFromFile(GAME_DATA_PATH"NewEquip.txt"));

	for (int i = 0; i < lib_new_equip.equip_librarys_size(); i++)
	{
		auto pto_equip = lib_new_equip.equip_librarys(i);

		if (0 == pto_equip.id())
			continue;

		CEquipTP* new_equip = new CEquipTP;
		new_equip->id_ = pto_equip.id();
		new_equip->suit_id_ = pto_equip.suit_id();
		new_equip->type_ = (EquipPart)pto_equip.type();
		new_equip->use_level_ = pto_equip.use_lv();
		new_equip->stage_ = pto_equip.stage_lv();
		new_equip->attribute_type_ = pto_equip.attribute_type();
		new_equip->attribute_ = pto_equip.attribute();
		new_equip->jewel_1_ = pto_equip.jewel_1();
		new_equip->jewel_2_ = pto_equip.jewel_2();
		new_equip->jewel_3_ = pto_equip.jewel_3();
		new_equip->jewel_4_ = pto_equip.jewel_4();

		for (int k = 0; k < pto_equip.possible_suit_size(); k++)
			new_equip->possible_suit_.push_back(pto_equip.possible_suit(k));

		new_equip->price_ = pto_equip.price();
		new_equip->sell_price_ = pto_equip.sell_price();

		auto it = new_equip_tp_.insert(std::make_pair(new_equip->id_, new_equip));
		if (false == it.second)
			delete new_equip;
	}
	printf(FormatString("加载", new_equip_tp_.size(), "个新装备\n").c_str());

	Strengthen_Price_Library  datPrice;
	datPrice.ParseFromString(GetDataFromFile(GAME_DATA_PATH"StrengthenPrice.txt"));
	for (int i = 0; i < datPrice.strengthen_price_size(); i++)
	{
		auto ptoPrice = datPrice.strengthen_price(i);
		ImproveEquipPrice *price{ new ImproveEquipPrice };
		price->level = ptoPrice.lv();
		price->weapon = ptoPrice.weapon();
		price->hands = ptoPrice.hands();
		price->feet = ptoPrice.feet();
		price->chest = ptoPrice.chest();
		price->head = ptoPrice.head();
		price->legs = ptoPrice.legs();
		improve_equip_price[price->level] = price;
	}
}

const CEquipTP* CEquipTP::GetEquipTP(int id)
{
	auto it = new_equip_tp_.find(id);

	if (it == new_equip_tp_.cend())
		return nullptr;
	else
		return it->second;
}

const CEquipTP* CEquipTP::GetEquipTP(int stage, EquipPart type)
{
	for (auto it : new_equip_tp_)
	{
		if (it.second->stage() == stage &&
			it.second->type() == type)
			return it.second;
	}
	return nullptr;
}

int	CEquipTP::ProduceSuitID(int except_id) const
{
	if (possible_suit_.size() <= 0)
		return 0;
	if (possible_suit_.size() == 1)
		return possible_suit_[0];

	int max_num = possible_suit_.size() - 1;

	int result = possible_suit_.at(GetRandom(0, max_num));

	while (result == except_id)
		result = possible_suit_.at(GetRandom(0, max_num));

	return result;
}

__int64	CEquipTP::GetImproveEquipPrice(int nLv, EquipPart type)
{
	auto it = improve_equip_price.find(nLv);

	if (improve_equip_price.cend() == it)
		return 0;

	switch (type)
	{
	case EquipPart::kHead:return it->second->head;
	case EquipPart::kChest:return it->second->chest;
	case EquipPart::kFeet:return it->second->feet;
	case EquipPart::kHands:return it->second->hands;
	case EquipPart::kLegs:return it->second->legs;
	case EquipPart::kWeapon:return it->second->weapon;
	default:return 0;
	}
}

int	CEquipTP::GetEquipAttribute(int level, EquipPart type)
{
	int stage = (level - (level % 20)) / 20 + 1;

	while (stage > 0)
	{
		for (auto it : new_equip_tp_)
		{
			if (it.second->stage_ == stage &&
				it.second->type_ == type)
				return static_cast<int> (it.second->attribute_);
		}
		stage--;
	}
	return 0;
}