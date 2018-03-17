#include "stdafx.h"
#include "AdditionLibrary.h"

std::map<int, const CAddition*> CAddition::addition_library_;
int CAddition::fair_equip_atk_;
int CAddition::fair_equip_def_;
int CAddition::fair_equip_hp_;
int CAddition::fair_soldier_atk_;
int CAddition::fair_soldier_matk_;
int CAddition::fair_soldier_def_;
int CAddition::fair_soldier_mdef_;
int CAddition::fair_soldier_hp_;

CAddition::CAddition()
{
}

CAddition::~CAddition()
{
}

void CAddition::Load()
{
	Addition_Library lib_addition;
	lib_addition.ParseFromString(GetDataFromFile(GAME_DATA_PATH"NewAddition.txt"));

	for (int i = 0; i < lib_addition.addition_size(); i++)
	{
		auto pto_equip = lib_addition.addition(i);

		if (0 == pto_equip.stage_lv())
			continue;

		CAddition* addition = new CAddition;

		addition->stage_ = pto_equip.stage_lv();

		addition->atk_ = pto_equip.atk();
		addition->m_atk_ = pto_equip.m_atk();
		addition->def_ = pto_equip.def();
		addition->m_def_ = pto_equip.m_def();
		addition->hp_ = pto_equip.hp();

		addition->weapon_ = pto_equip.weapon();
		addition->hands_ = pto_equip.hands();
		addition->chest_ = pto_equip.chest();
		addition->legs_ = pto_equip.legs();
		addition->head_ = pto_equip.head();
		addition->feet_ = pto_equip.feet();

		auto it = addition_library_.insert(std::make_pair(addition->stage_, addition));
		if (false == it.second)
			delete addition;
	}
	printf(FormatString("加载", addition_library_.size(), "条加成\n").c_str());

	fair_equip_atk_ = CAddition::GetCustomMasterEquipAtk(50);
	fair_equip_def_ = CAddition::GetCustomMasterEquipDef(50);
	fair_equip_hp_ = CAddition::GetCustomMasterEquipHP(50);
	fair_soldier_atk_ = CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitAtk);
	fair_soldier_matk_ = CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitMAtk);
	fair_soldier_def_ = CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitDef);
	fair_soldier_mdef_ = CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitMDef);
	fair_soldier_hp_ = CAddition::GetSoldierAddition(50, AdditionType::kAdditionTypeUnitHP);
}

const CAddition* CAddition::GetAddition(int stage)
{
	auto it = addition_library_.find(stage);

	if (addition_library_.cend() == it)
		return nullptr;
	else
		return it->second;
}

const float CAddition::GetAddition(int stage, AdditionType type)
{
	auto it = addition_library_.find(stage);

	if (addition_library_.cend() != it)
		return it->second->GetTypeAddtion(type);
	return 0;
}

float CAddition::GetTypeAddtion(AdditionType type) const
{
	switch (type)
	{
	case kAdditionTypeUnitAtk:
		return atk_;
	case kAdditionTypeUnitMAtk:
		return m_atk_;
	case kAdditionTypeUnitDef:
		return def_;
	case kAdditionTypeUnitMDef:
		return m_def_;
	case kAdditionTypeUnitHP:
		return hp_;
	case kAdditionTypeWeapon:
		return weapon_;
	case kAdditionTypeHands:
		return hands_;
	case kAdditionTypeChest:
		return chest_;
	case kAdditionTypeLegs:
		return legs_;
	case kAdditionTypeHead:
		return head_;
	case kAdditionTypeFeet:
		return feet_;
	default:
		return 0;
	}
}

int  CAddition::GetHeroEquipAddition(int level, const CEquipTP* equip, int quality)
{
	float EQUIP_BASE_RATE[6] = { 0, 0.5, 1.5, 3, 5, 7.5 };
	float QUALITY[6] = { 1.0, 1.03, 1.05, 1.07, 1.09, 1.15 };
	float additionRate = CAddition::GetAddition(equip->stage(), (AdditionType)equip->type());
	float result = equip->attribute() + additionRate * EQUIP_BASE_RATE[quality];
	int stage = 1;

	float strengthAttr = 0;

	while (level > 20 && stage < equip->stage())
	{
		strengthAttr += CAddition::GetAddition(stage, (AdditionType)equip->type()) * 20;
		level -= 20;
		stage++;
	}

	strengthAttr += CAddition::GetAddition(stage, (AdditionType)equip->type()) * level;
	strengthAttr *= QUALITY[quality];

	return  static_cast<int>(result + strengthAttr);
}

int	CAddition::GetSoldierAddition(int level, const AdditionType type)
{
	float result = 0;

	int stage = 1;

	while (level > 20)
	{
		result += CAddition::GetAddition(stage, type) * 20;
		level -= 20;
		stage++;
	}

	result += CAddition::GetAddition(stage, type) * level;
	return  static_cast<int>(result);
}

int CAddition::GetCustomMasterEquipAtk(int level)
{
	float reslut = 0;
	int stage_leavel = (level - (level % 20)) / 20 + 1;

	int temp_stage = stage_leavel;
	while (true)
	{
		if (temp_stage == 0)
			break;
		const CEquipTP* equip = CEquipTP::GetEquipTP(temp_stage, EquipPart::kWeapon);
		if (nullptr != equip)
		{
			reslut += CAddition::GetHeroEquipAddition(level, equip, 0);
			break;
		}
		else
		{
			temp_stage--;
		}
	}

	temp_stage = stage_leavel;
	while (true)
	{
		if (temp_stage == 0)
			break;
		const CEquipTP* equip = CEquipTP::GetEquipTP(temp_stage, EquipPart::kHands);
		if (nullptr != equip)
		{
			reslut += CAddition::GetHeroEquipAddition(level, equip, 0);
			break;
		}
		else
		{
			temp_stage--;
		}
	}

	return (int)reslut;
}

int CAddition::GetCustomMasterEquipDef(int level)
{
	float reslut = 0;
	int stage_leavel = (level - (level % 20)) / 20 + 1;

	int temp_stage = stage_leavel;
	while (true)
	{
		if (temp_stage == 0)
			break;
		const CEquipTP* equip = CEquipTP::GetEquipTP(temp_stage, EquipPart::kChest);
		if (nullptr != equip)
		{
			reslut += CAddition::GetHeroEquipAddition(level, equip, 0);
			break;
		}
		else
		{
			temp_stage--;
		}
	}

	temp_stage = stage_leavel;
	while (true)
	{
		if (temp_stage == 0)
			break;
		const CEquipTP* equip = CEquipTP::GetEquipTP(temp_stage, EquipPart::kLegs);
		if (nullptr != equip)
		{
			reslut += CAddition::GetHeroEquipAddition(level, equip, 0);
			break;
		}
		else
		{
			temp_stage--;
		}
	}

	return (int)reslut;
}

int CAddition::GetCustomMasterEquipHP(int level)
{
	float reslut = 0;
	int stage_leavel = (level - (level % 20)) / 20 + 1;

	int temp_stage = stage_leavel;
	while (true)
	{
		if (temp_stage == 0)
			break;
		const CEquipTP* equip = CEquipTP::GetEquipTP(temp_stage, EquipPart::kHead);
		if (nullptr != equip)
		{
			reslut += CAddition::GetHeroEquipAddition(level, equip, 0);
			break;
		}
		else
		{
			temp_stage--;
		}
	}

	temp_stage = stage_leavel;
	while (true)
	{
		if (temp_stage == 0)
			break;
		const CEquipTP* equip = CEquipTP::GetEquipTP(temp_stage, EquipPart::kFeet);
		if (nullptr != equip)
		{
			reslut += CAddition::GetHeroEquipAddition(level, equip, 0);
			break;
		}
		else
		{
			temp_stage--;
		}
	}

	return (int)reslut;
}