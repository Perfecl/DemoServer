#include "stdafx.h"
#include "SoldierTP.h"

int	CSoldierTP::ms_nStr{ 0 };
int	CSoldierTP::ms_nCmd{ 0 };
int	CSoldierTP::ms_nInt{ 0 };
std::map<int, const CSoldierTP*> CSoldierTP::ms_mapSoldierTPs;

void CSoldierTP::Load()
{
	dat_UNIT_Library_All datAll;
	datAll.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Unit.txt"));

	auto baseValues = datAll.base_value();
	ms_nStr = baseValues.soldier_str();
	ms_nCmd = baseValues.soldier_cmd();
	ms_nInt = baseValues.soldier_int();

	auto soldiers = datAll.soldiers();

	for (int i = 0; i < soldiers.soldier_size(); i++)
	{
		auto soldier = soldiers.soldier(i);

		if (0 == soldier.id())
			continue;

		CSoldierTP *pSoldier = new CSoldierTP;

		pSoldier->m_nID = soldier.id();
		pSoldier->m_strName = soldier.name();
		pSoldier->m_fVolume = soldier.vloume();
		pSoldier->m_nResourceID = soldier.res_id();
		pSoldier->m_fMoveSpeed = soldier.move_speed();
		pSoldier->m_bIsRange = soldier.is_range();
		pSoldier->m_fAtkDistance = soldier.distance();
		pSoldier->m_nAtkPrev = soldier.atk_prev();
		pSoldier->m_nAtkInterval = soldier.atk_interval();
		pSoldier->m_enAtkType = (ATK_TYPE)soldier.atk_type();
		pSoldier->m_nAtkBuildingDamage = soldier.building_damaged();
		pSoldier->m_enNormalAtk = (TARGET_TYPE)soldier.normal_target_type();
		pSoldier->m_enHideAtk = (TARGET_TYPE)soldier.hide_target_type();
		pSoldier->m_enSpecialAtk = (TARGET_TYPE)soldier.special_target_type();

		for (int i = 0; i < soldier.pass_skills_size(); i++)
			pSoldier->m_vctPassSkills.push_back(soldier.pass_skills(i));

		pSoldier->m_enSoldierType = (SOLDIER_TYPE)soldier.type();
		pSoldier->m_nCanUseNumMax = soldier.can_use_number();
		pSoldier->m_bCanHide = soldier.can_hide();

		pSoldier->m_nPrice = soldier.price();
		pSoldier->m_nPopulation = soldier.population();
		pSoldier->m_nCD = soldier.cd();

		pSoldier->m_fHP_mul = soldier.hp_mul();
		pSoldier->m_fATK_mul = soldier.atk_mul();
		pSoldier->m_fDEF_mul = soldier.def_mul();
		pSoldier->m_fMATK_mul = soldier.matk_mul();
		pSoldier->m_fMDEF_mul = soldier.mdef_mul();

		pSoldier->m_bIsBreakHero = soldier.is_break_hero();
		pSoldier->m_nBirthBuff = soldier.buff_id();

		pSoldier->strategy_id_ = soldier.strategy_id();
		pSoldier->cross_line_ = soldier.cross_line();

		auto it = ms_mapSoldierTPs.insert(std::make_pair(pSoldier->m_nID, pSoldier));
		if (false == it.second)
			delete pSoldier;
	}

	printf(FormatString("加载", ms_mapSoldierTPs.size(), "个士兵\n").c_str());
}

const CSoldierTP* CSoldierTP::GetSoldierTP(int nID)
{
	auto it = ms_mapSoldierTPs.find(nID);

	if (ms_mapSoldierTPs.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CSoldierTP::GetAllSoldierID(std::vector<int>* vctSoldier)
{
	for (auto &it : ms_mapSoldierTPs)
		vctSoldier->push_back(it.first);
}

void CSoldierTP::SetProtocol(pto_BATTLE_Struct_SoldierCard* ptoSoldier) const
{
	ptoSoldier->set_ncanusenummax(m_nCanUseNumMax);
	auto ptoCardBase = ptoSoldier->mutable_data();

	ptoCardBase->set_nmaxhp((int)(m_nBaseHP * m_fHP_mul));
	ptoCardBase->set_natk((int)(m_nBaseATK * m_fATK_mul));
	ptoCardBase->set_ndef((int)(m_nBaseDEF * m_fDEF_mul));
	ptoCardBase->set_nmatk((int)(m_nBaseMATK * m_fMATK_mul));
	ptoCardBase->set_nmdef((int)(m_nBaseMDEF * m_fMDEF_mul));
	ptoCardBase->set_szname(ANSI_to_UTF8(m_strName));
	ptoCardBase->set_nid(m_nID);
	ptoCardBase->set_nmaxhp(m_nBaseHP);
	ptoCardBase->set_nbodyrange(m_fVolume);
	ptoCardBase->set_nattackpretime(m_nAtkPrev);
	ptoCardBase->set_nattackintervaltime(m_nAtkInterval);
	ptoCardBase->set_fmovespeed(m_fMoveSpeed);
	ptoCardBase->set_ncooldown(m_nCD);
	ptoCardBase->set_nexpense(m_nPrice);
	ptoCardBase->set_bishide(m_bCanHide);

	//-----临时-----	
	ptoCardBase->set_nsmallicon(m_nID - 1);
	ptoCardBase->set_nresourcesmoduleid(m_nResourceID);
	//--------------

	ptoCardBase->set_enattackpettern_normal(m_enNormalAtk);
	ptoCardBase->set_enattackpettern_ishiding(m_enHideAtk);
	ptoCardBase->set_enattackpettern_support(m_enSpecialAtk);
}