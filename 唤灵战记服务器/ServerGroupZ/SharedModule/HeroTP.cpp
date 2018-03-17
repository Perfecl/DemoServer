#include "stdafx.h"
#include "HeroTP.h"
#include "PassSkill.h"

float	CHeroTP::m_fATKHero[5] = { 2.6f, 1.05f, 1.0f, 1.0f, 1.0f };		//力量型加成 血，物攻，物防，魔攻，魔防
float	CHeroTP::m_fDEFHero[5] = { 2.8f, 1.02f, 1.01f, 1.0f, 1.0f };		//统御型加成
float	CHeroTP::m_fINTHero[5] = { 2.2f, 0.95f, 0.95f, 1.1f, 1.05f };		//智力型加成
float	CHeroTP::m_fSKILLHero[5] = { 2.3f, 1.05f, 0.97f, 1.05f, 0.97f };	//技巧型加成

int	CHeroTP::ms_ActiveSkill[3] = { 1, 2, 10 };
int	CHeroTP::ms_PassSkill[3] = { 5, 20, 35 };

std::map<int, const CHeroTP*> CHeroTP::ms_mapHeroTPs;
float	CHeroTP::m_fCrit{ 0.2f };
float	CHeroTP::m_fCritValue{ 1.5f };

std::vector<int> CHeroTP::ms_vctHeroID;

const CHeroTP* CHeroTP::GetHeroTP(int nID)
{
	auto it = ms_mapHeroTPs.find(nID);
	if (ms_mapHeroTPs.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CHeroTP::GetAllHeroID(std::vector<int>* vctHero)
{
	for (auto &it : ms_mapHeroTPs)
		vctHero->push_back(it.first);
}

void CHeroTP::Load()
{
	dat_UNIT_Library_All datAll;
	datAll.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Unit.txt"));

	auto datHeroes = datAll.heroes();

	for (int i = 0; i < datHeroes.hero_size(); i++)
	{
		auto hero = datHeroes.hero(i);

		if (0 == hero.id())
			continue;

		CHeroTP *pHero = new CHeroTP;
		pHero->m_nID = hero.id();
		pHero->m_strName = hero.name();
		pHero->m_nResourceID = hero.res_id();
		pHero->m_fVolume = hero.vloume();
		pHero->m_fMoveSpeed = hero.move_speed();
		pHero->m_bIsRange = hero.is_range();
		pHero->m_fAtkDistance = hero.distance();
		pHero->m_nAtkPrev = hero.atk_prev();
		pHero->m_nAtkInterval = hero.atk_interval();
		pHero->m_enAtkType = (ATK_TYPE)hero.atk_type();
		pHero->m_nAtkBuildingDamage = hero.building_damaged();
		pHero->m_enNormalAtk = (TARGET_TYPE)hero.normal_target_type();
		pHero->m_enHideAtk = (TARGET_TYPE)hero.hide_target_type();
		pHero->m_enSpecialAtk = (TARGET_TYPE)hero.special_target_type();

		for (int i = 0; i < hero.pass_skills_size(); i++)
			pHero->m_vctPassSkills.push_back(hero.pass_skills(i));

		pHero->m_nStr = hero.strength();
		pHero->m_nCmd = hero.command();
		pHero->m_nInt = hero.intelligence();

		pHero->m_fGrowupAtk = hero.growup_atk();
		pHero->m_fGrowupDef = hero.growup_def();
		pHero->m_fGrowupMAtk = hero.growup_matk();
		pHero->m_fGrowupMDef = hero.growup_mdef();
		pHero->m_fGrowupHP = hero.growup_hp();

		pHero->m_enHeroType = (HERO_TYPE)hero.type();
		pHero->m_nQuality = hero.quality();

		for (int i = 0; i < hero.skills_size(); i++)
			pHero->m_vctSkills.push_back(hero.skills(i));

		auto it = ms_mapHeroTPs.insert(std::make_pair(pHero->m_nID, pHero));

		if (false == it.second)
			delete pHero;
		else
			ms_vctHeroID.push_back(pHero->m_nID);
	}

	printf(FormatString("加载", ms_mapHeroTPs.size(), "个英雄\n").c_str());
}

int CHeroTP::GetMaxTime(int nQuality)
{
	return HERO_DURATION;
}

int CHeroTP::GetPassSkillExTime(int level) const
{
	int result{ 0 };
	int nIndex{ 0 };

	for (auto &it : m_vctPassSkills)
	{
		const CPassSkill *pSkill = CPassSkill::GetPassSkill(it);

		if (level < CHeroTP::GetPassSkillOpenLevel(nIndex))
		{
			nIndex++;
			continue;
		}
		nIndex++;

		if (!pSkill)
			continue;


		if (TriggerTime_Always == pSkill->GetTriggerTime())
		{
			for (int i = 0; i < pSkill->GetBaseSize(); i++)
			{
				switch (pSkill->GetBaseType(i))
				{
				case BaseType_MaxTime:
					result += int(pSkill->GetBaseValue(i));
					break;
				default:
					break;
				}
			}
		}
	}

	return result;
}

int CHeroTP::GetExMaxTime(int nQuality, int nTrainRank, int nLevel) const
{
	int nExMaxTime = CHeroTP::GetMaxTime(nQuality);

	int nIndex{ 0 };
	for (auto &it : m_vctPassSkills)
	{
		const CPassSkill *pSkill = CPassSkill::GetPassSkill(it);

		if (nLevel < CHeroTP::GetPassSkillOpenLevel(nIndex))
		{
			nIndex++;
			continue;
		}
		nIndex++;

		if (!pSkill)
			continue;


		if (TriggerTime_Always == pSkill->GetTriggerTime())
		{
			for (int i = 0; i < pSkill->GetBaseSize(); i++)
			{
				switch (pSkill->GetBaseType(i))
				{
				case BaseType_MaxTime:
					nExMaxTime += int(pSkill->GetBaseValue(i));
					break;
				default:
					break;
				}
			}
		}
	}

	return nExMaxTime;
}

void CHeroTP::SetProtocol(pto_BATTLE_Struct_HeroCard* ptoHero, int nTrainRank, int nLevel) const
{
	int nMaxTime = GetMaxTime(m_nQuality);

	ptoHero->set_nheroexisttimemax(GetExMaxTime(m_nQuality, nTrainRank, nLevel));
	for (auto &it : m_vctSkills)
		ptoHero->add_vctactskillid(it);
	ptoHero->set_nquality(nTrainRank);
	ptoHero->set_nlevel(nLevel);
	auto ptoCardBase = ptoHero->mutable_data();

	ptoCardBase->set_szname(ANSI_to_UTF8(m_strName));
	ptoCardBase->set_nid(m_nID);
	ptoCardBase->set_nmaxhp(m_nBaseHP);
	ptoCardBase->set_nbodyrange(m_fVolume);
	ptoCardBase->set_nattackpretime(m_nAtkPrev);
	ptoCardBase->set_nattackintervaltime(m_nAtkInterval);

	ptoCardBase->set_nmaxhp(m_nBaseHP);
	ptoCardBase->set_natk(m_nBaseATK);
	ptoCardBase->set_ndef(m_nBaseDEF);
	ptoCardBase->set_nmatk(m_nBaseMATK);
	ptoCardBase->set_nmdef(m_nBaseMDEF);
	ptoCardBase->set_nstr(m_nStr);
	ptoCardBase->set_nint(m_nInt);
	ptoCardBase->set_ncmd(m_nCmd);
	ptoCardBase->set_fcrit(m_fCrit);

	ptoCardBase->set_fmovespeed(m_fMoveSpeed);

	//-----临时-----
	ptoCardBase->set_nsmallicon(m_nID);
	ptoCardBase->set_nresourcesmoduleid(m_nResourceID);
	//--------------

	ptoCardBase->set_enattackpettern_normal(m_enNormalAtk);
	ptoCardBase->set_enattackpettern_ishiding(m_enHideAtk);
	ptoCardBase->set_enattackpettern_support(m_enSpecialAtk);
}

int CHeroTP::RandomHeroID()
{
	int nRandom = GetRandom(1, 3);
	switch (nRandom)
	{
	case 1:
		return 2;
	case 2:
		return 3;
	case 3:
		return 8;
	default:
		return 0;
	}
}

float CHeroTP::GetHeroTypeAddition(HERO_TYPE enHeroType, int nType)
{
	switch (enHeroType)
	{
	case HERO_TYPE::HT_ATK:
		return m_fATKHero[nType];
	case HERO_TYPE::HT_DEF:
		return m_fDEFHero[nType];
	case HERO_TYPE::HT_INT:
		return m_fINTHero[nType];
	case HERO_TYPE::HT_SKILL:
		return m_fSKILLHero[nType];
	default:
		return 0;
	}
}