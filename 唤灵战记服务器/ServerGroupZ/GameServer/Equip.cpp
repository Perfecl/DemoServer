#include "stdafx.h"
#include "Equip.h"

CEquip::CEquip(int equip_id) :
equip_template_{ CEquipTP::GetEquipTP(equip_id) }
{
	if (!equip_template_)
		RECORD_WARNING("×°±¸ID³ö´í");
	gems_.fill(0);
}

CEquip::~CEquip()
{

}

void CEquip::SetProtocol(Equip_Model* pto)
{
	pto->set_uniqueid(unique_id_);
	pto->set_id(GetID());
	pto->set_lv(level_);
	pto->set_quality(quality_);
	pto->set_suit_id(suit_id_);
	pto->set_enchant(enchanting1_.first);
	pto->set_enchant_value(enchanting1_.second);
	pto->set_enchant2(enchanting2_.first);
	pto->set_enchant2_value(enchanting2_.second);
	pto->set_enchant2_is_active(enchanting2_is_active_);
	for (auto &it_gem : gems_)
		pto->add_jewel(it_gem);
}

int CEquip::GetID() const
{
	if (equip_template_)
		return equip_template_->id();
	else
		return 0;
}
