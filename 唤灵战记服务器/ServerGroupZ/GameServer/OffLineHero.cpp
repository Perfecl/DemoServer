#include "stdafx.h"
#include "OffLineHero.h"
#include "OffLinePlayer.h"
#include "HeroCard.h"
#include "Army.h"
#include "Equip.h"

COffLineHero::COffLineHero()
{

}

COffLineHero::~COffLineHero()
{

}

COffLineHero* COffLineHero::InitHero(const CHeroCard* hero_card, COffLinePlayer* pPlayer)
{
	if (!hero_card)
		return nullptr;

	RuneAddition nRuneAddition;

	CRuneLibrary::CalculateRuneAddition(&nRuneAddition, &hero_card->GetRunes());//符文

	COffLineHero* pOfflineHero = new COffLineHero;
	pOfflineHero->m_pHeroTP = hero_card->hero_template();
	pOfflineHero->m_enHeroType = hero_card->hero_template()->GetType();

	pOfflineHero->level_ = hero_card->hero_level();

	pOfflineHero->m_nStr = hero_card->hero_template()->GetStr() + hero_card->train_str() + nRuneAddition.m_nAddStr;
	pOfflineHero->m_nCmd = hero_card->hero_template()->GetCmd() + hero_card->train_cmd() + nRuneAddition.m_nAddCmd;
	pOfflineHero->m_nInt = hero_card->hero_template()->GetInt() + hero_card->train_int() + nRuneAddition.m_nAddInt;

	float fHPMul = CHeroTP::GetHeroTypeAddition(pOfflineHero->m_enHeroType, 0);
	float fAtkMul = CHeroTP::GetHeroTypeAddition(pOfflineHero->m_enHeroType, 1);
	float fDefMul = CHeroTP::GetHeroTypeAddition(pOfflineHero->m_enHeroType, 2);
	float fMatkMul = CHeroTP::GetHeroTypeAddition(pOfflineHero->m_enHeroType, 3);
	float fMdefMul = CHeroTP::GetHeroTypeAddition(pOfflineHero->m_enHeroType, 4);

	int equip_atk{ 0 };
	int equip_def{ 0 };
	int equip_hp{ 0 };

	SuitAddition suit_addition;
	__GetSuitAddtion(hero_card, pPlayer, &suit_addition);//套装

	for (auto it = pPlayer->GetAllEquip()->begin();it != pPlayer->GetAllEquip()->cend();it++)
	{
		if (abs(it->second) != hero_card->hero_template()->GetID())
			continue;
		switch (it->first->equip_template()->attribute_type())
		{
		case 1:
			equip_atk += CAddition::GetHeroEquipAddition(it->first->level(), it->first->equip_template(), it->first->quality());
			break;
		case 2:
			equip_def += CAddition::GetHeroEquipAddition(it->first->level(), it->first->equip_template(), it->first->quality());
			break;
		case 3:
			equip_hp += CAddition::GetHeroEquipAddition(it->first->level(), it->first->equip_template(), it->first->quality());
			break;
		default:
			break;
		}
	}

	pOfflineHero->m_nHP = pOfflineHero->m_nMaxHp = (int)((hero_card->hero_template()->m_nBaseHP + equip_hp) * fHPMul) + nRuneAddition.m_nHP;
	pOfflineHero->m_nAtk = (int)((hero_card->hero_template()->m_nBaseATK + equip_atk) * fAtkMul) + nRuneAddition.m_nAtk;
	pOfflineHero->m_nDef = (int)((hero_card->hero_template()->m_nBaseDEF + equip_def) * fDefMul) + nRuneAddition.m_nDef;
	pOfflineHero->m_nMatk = (int)((hero_card->hero_template()->m_nBaseMATK + equip_atk) * fMatkMul) + nRuneAddition.m_nMAtk;
	pOfflineHero->m_nMdef = (int)((hero_card->hero_template()->m_nBaseMDEF + equip_def) * fMdefMul) + nRuneAddition.m_nMDef;

	pOfflineHero->m_nHP += int((hero_card->hero_level() - 1) * hero_card->hero_template()->GetGrowupHP());
	pOfflineHero->m_nAtk += int((hero_card->hero_level() - 1) * hero_card->hero_template()->GetGrowupAtk());
	pOfflineHero->m_nDef += int((hero_card->hero_level() - 1) * hero_card->hero_template()->GetGrowupDef());
	pOfflineHero->m_nMatk += int((hero_card->hero_level() - 1) * hero_card->hero_template()->GetGrowupMAtk());
	pOfflineHero->m_nMdef += int((hero_card->hero_level() - 1) * hero_card->hero_template()->GetGrowupMDef());

	const CFashionTP* fashion = CFashionTP::GetFashionTP(pPlayer->GetDressID()); //时装
	if (fashion)
	{
		pOfflineHero->m_nAtk += int(fashion->hero_atk() * fAtkMul);
		pOfflineHero->m_nMatk += int(fashion->hero_matk() * fMatkMul);
		pOfflineHero->m_nDef += int(fashion->hero_def() * fDefMul);
		pOfflineHero->m_nMdef += int(fashion->hero_mdef() * fMdefMul);
		pOfflineHero->m_nHP += int(fashion->hero_hp() * fHPMul);
	}

	const CHeroQuality* quality = CHeroQuality::GetHeroQuality(hero_card->quality()); //英雄品质
	if (quality)
	{
		pOfflineHero->m_nHP += quality->hp();
		pOfflineHero->m_nAtk += quality->atk();
		pOfflineHero->m_nMatk += quality->matk();
		pOfflineHero->m_nDef += quality->def();
		pOfflineHero->m_nMdef += quality->mdef();
		pOfflineHero->m_nStr += quality->strength();
		pOfflineHero->m_nCmd += quality->command();
		pOfflineHero->m_nInt += quality->intelligence();
	}

	for (auto it = pPlayer->GetAllEquip()->begin(); it != pPlayer->GetAllEquip()->cend(); it++) //精炼
	{
		if (abs(it->second) != hero_card->hero_template()->GetID())
			continue;

		switch (it->first->enchanting1())
		{
		case kEnchantingTypeAtk:			pOfflineHero->m_nAtk += (int)it->first->enchanting1_value();			break;
		case kEnchantingTypeMAtk:			pOfflineHero->m_nMatk += (int)it->first->enchanting1_value();			break;
		case kEnchantingTypeDef:			pOfflineHero->m_nDef += (int)it->first->enchanting1_value();			break;
		case kEnchantingTypeMDef:			pOfflineHero->m_nMdef += (int)it->first->enchanting1_value();			break;
		case kEnchantingTypeHP:				pOfflineHero->m_nHP += (int)it->first->enchanting1_value();				break;
		case kEnchantingTypeCritOdds:		pOfflineHero->m_fCritOdds += (int)it->first->enchanting1_value();		break;
		case kEnchantingTypePreventCrit:	pOfflineHero->m_fPreventCrit += (int)it->first->enchanting1_value();	break;
		default:break;
		}
		if (it->first->enchanting2_is_active())
		{
			switch (it->first->enchanting2())
			{
			case kEnchantingTypeAtk:			pOfflineHero->m_nAtk += (int)it->first->enchanting1_value();			break;
			case kEnchantingTypeMAtk:			pOfflineHero->m_nMatk += (int)it->first->enchanting1_value();			break;
			case kEnchantingTypeDef:			pOfflineHero->m_nDef += (int)it->first->enchanting1_value();			break;
			case kEnchantingTypeMDef:			pOfflineHero->m_nMdef += (int)it->first->enchanting1_value();			break;
			case kEnchantingTypeHP:				pOfflineHero->m_nHP += (int)it->first->enchanting1_value();				break;
			case kEnchantingTypeCritOdds:		pOfflineHero->m_fCritOdds += (int)it->first->enchanting1_value();		break;
			case kEnchantingTypePreventCrit:	pOfflineHero->m_fPreventCrit += (int)it->first->enchanting1_value();	break;
			default:break;
			}
		}
	}

	pOfflineHero->m_bIsRange = hero_card->hero_template()->m_bIsRange;
	pOfflineHero->m_fAtkDistance = hero_card->hero_template()->m_fAtkDistance;
	pOfflineHero->m_enAtkType = hero_card->hero_template()->m_enAtkType;

	pOfflineHero->m_nQuality = hero_card->hero_template()->m_nQuality;
	pOfflineHero->m_nAtkBuildingDamage = hero_card->hero_template()->m_nAtkBuildingDamage + nRuneAddition.m_nBulidingAtk;

	pOfflineHero->m_fCritOdds += nRuneAddition.m_fCrit;
	pOfflineHero->m_fPreventCrit += nRuneAddition.m_fPreventCrit;

	int nSkillIndex{ 0 };
	for (auto &it : hero_card->hero_template()->m_vctPassSkills)
	{
		const CPassSkill* pPassSkill = CPassSkill::GetPassSkill(it);
		if (pPassSkill && hero_card->hero_level() >= CHeroTP::GetPassSkillOpenLevel(nSkillIndex))
			pOfflineHero->m_vctPasSkills.push_back(pPassSkill);

		nSkillIndex++;
	}

	pOfflineHero->GetAlwaysPassSkill();

	pOfflineHero->rank_ = hero_card->quality();
	pOfflineHero->suit_id_ = suit_addition.skill_id;
	return pOfflineHero;
}

int COffLineHero::Attack(COffLineHero* pHero, pto_OFFLINEBATTLE_Struct_UnitRound* pUnitRound)
{
	PrevAttackSkillCalculate(pHero);
	OfflineDamageInfo damgeInfo;
	damgeInfo.bIsHeroAtk = true;
	damgeInfo.bIsRangeAtk = IsRangeUnit();
	damgeInfo.nStr = GetStr();
	damgeInfo.nInt = GetInt();
	damgeInfo.nCmd = GetCmd();

	if (AT_PHYSICS == m_enAtkType)
	{
		damgeInfo.nDamage = GetAtk();
	}
	else if (AT_MAGIC == m_enAtkType)
	{
		damgeInfo.nMDamage = GetMatk();
	}
	else if (AT_BLEND == m_enAtkType)
	{
		damgeInfo.nDamage = GetAtk();
		damgeInfo.nMDamage = GetMatk();
	}

	damgeInfo.nAtkType = m_enAtkType;
	damgeInfo.pSrcHero = this;

	int nDmg = (int)pHero->BeDamaged(damgeInfo, pUnitRound);
	AfterAttackSkillCalculate(pHero);

	return nDmg;
}

__int64 COffLineHero::BeDamaged(OfflineDamageInfo di, pto_OFFLINEBATTLE_Struct_UnitRound* pUnitRound)
{
	if (US_DEAD == m_enState)
		return 0;

	float fFinalDamage{ 0 };
	float fDamage = static_cast<float>(di.nDamage);
	float fMDamage = static_cast<float>(di.nMDamage);


	int def = GetDef();
	int m_def = GetMdef();

	if (armor_weak_ > 0)
	{
		def = def - 30;
		m_def = m_def - 30;
	}

	//伤害公式
	if (fDamage > 0)
	{
		fDamage = (fDamage - GetDef()) * _DamageFormula(di.nStr, GetStr(), GetCmd()) * di.fPhysicsRatio;

		if (fDamage < 0)
			fDamage = 0;
	}
	if (fMDamage > 0)
	{
		fMDamage = (fMDamage - GetMdef()) * _DamageFormula(di.nInt, GetInt(), GetCmd()) * di.fMagicRatio;

		if (fMDamage < 0)
			fMDamage = 0;
	}

	float fCritOdds{ 0 };
	float fCritValue{ 0 };

	PrevBeDamagedSkillCalculate(di, fDamage, fMDamage);

	fCritOdds = di.pSrcHero->GetCritOdds();
	fCritValue = di.pSrcHero->GetCritValue();
	di.pSrcHero->AttackingSkillCalculate(this, fDamage, fMDamage, fCritOdds, fCritValue);

	//攻击类型判断
	if (di.bIsSkill)
	{
		fFinalDamage = fDamage + fMDamage;
	}
	else
	{
		if (AT_BLEND == di.nAtkType)
			fFinalDamage = (fDamage + fMDamage) * 0.6f;
		else if (AT_PHYSICS == di.nAtkType)
			fFinalDamage = fDamage;
		else if (AT_MAGIC == di.nAtkType)
			fFinalDamage = fMDamage;
	}

	fFinalDamage *= di.fRatio;

	bool		bIsCrit{ false };

	//计算暴击率
	fCritOdds = fCritOdds - m_fPreventCrit;
	if (GetRandom(1, 100) <= (100 * fCritOdds))
	{
		fFinalDamage *= fCritValue;
		bIsCrit = true;
	}

	//英雄相克
	if (HT_ATK == GetHeroType() && HT_DEF == di.pSrcHero->GetHeroType())
		fFinalDamage *= 1.25f;
	else if (HT_DEF == GetHeroType() && HT_INT == di.pSrcHero->GetHeroType())
		fFinalDamage *= 1.25f;
	else if (HT_DEF == GetHeroType() && HT_SKILL == di.pSrcHero->GetHeroType())
		fFinalDamage *= 1.25f;
	else if (HT_INT == GetHeroType() && HT_ATK == di.pSrcHero->GetHeroType())
		fFinalDamage *= 1.25f;
	else if (HT_SKILL == GetHeroType() && HT_ATK == di.pSrcHero->GetHeroType())
		fFinalDamage *= 1.25f;
	else if (HT_ATK == GetHeroType() && HT_INT == di.pSrcHero->GetHeroType())
		fFinalDamage *= 0.75f;
	else if (HT_ATK == GetHeroType() && HT_SKILL == di.pSrcHero->GetHeroType())
		fFinalDamage *= 0.75f;
	else if (HT_DEF == GetHeroType() && HT_ATK == di.pSrcHero->GetHeroType())
		fFinalDamage *= 0.75f;
	else if (HT_INT == GetHeroType() && HT_DEF == di.pSrcHero->GetHeroType())
		fFinalDamage *= 0.75f;
	else if (HT_SKILL == GetHeroType() && HT_DEF == di.pSrcHero->GetHeroType())
		fFinalDamage *= 0.75f;

	__int64 nFinalDamage = static_cast<int>(fFinalDamage) > 5 ? static_cast<int>(fFinalDamage) : 5;

	nFinalDamage = nFinalDamage * 2;

	if (di.pSrcHero->dmg_up() > 0)
		nFinalDamage = int(nFinalDamage * 1.10f);

	if (di.pSrcHero->double_attack())
	{
		nFinalDamage = nFinalDamage * 2;
		pUnitRound->set_num((int)nFinalDamage / 2);
		di.pSrcHero->set_double_attack(false);

		if (nFinalDamage >= m_nHP)
			nFinalDamage = m_nHP;

		m_nHP -= nFinalDamage;
	}

	else
	{
		pUnitRound->set_num((int)nFinalDamage);
		if (nFinalDamage >= m_nHP)
			nFinalDamage = m_nHP;

		m_nHP -= nFinalDamage;
	}

	if (m_nHP <= 0)
		_Die();

	return nFinalDamage;
}

float COffLineHero::_DamageFormula(int src_Str_Int, int dest_Str_Int, int dest_cmd)
{
	float fResult{ 0 };

	float fV1 = static_cast<float>(src_Str_Int);
	float fV2 = static_cast<float>(dest_Str_Int);
	float fV3 = static_cast<float>(dest_cmd);

	float fValueDef = (fV2 + fV3) / 2.0f;

	if (fV1 >= fValueDef)
	{
		fResult = 1.0f + ((fV1 - fValueDef) * 0.03f);
		if (fResult > 3.0f)
			fResult = 3.0f;
	}
	else if (fV1 < fValueDef)
	{
		fResult = 1.0f - ((fValueDef - fV1) * 0.01f);
		if (fResult < 0.25f)
			fResult = 0.25f;
	}

	return fResult;
}

void COffLineHero::_Die()
{
	m_enState = US_DEAD;
}

void COffLineHero::AttackingSkillCalculate(COffLineHero* pTarget, float& fDamage, float& fMDamage, float& fCritOdds, float& fCritValue)
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pPassSkill = it;

		if (!pPassSkill)
			continue;

		bool  bFlag{ false };
		const float nParama{ pPassSkill->GetTriggerConditionParama() };
		const __int64 nSrcHp{ pTarget->GetHP() };
		const __int64 nSrcMaxHp{ pTarget->GetMaxHP() };
		const int ReduceType = pPassSkill->GetReduceType();
		const int ReduceTypeExcept = pPassSkill->GetReduceTypeExpect();

		switch (pPassSkill->GetTriggerCondition())
		{
		case 0:bFlag = true; break;
		case 1:if (m_nHP < nParama) bFlag = true; break;
		case 2:if (m_nHP < m_nMaxHp * nParama) bFlag = true; break;
		case 3:if (m_nHP > nParama) bFlag = true; break;
		case 4:if (m_nHP > m_nMaxHp * nParama) bFlag = true; break;
		case 5:if (nSrcHp < nParama) bFlag = true; break;
		case 6:if (nSrcHp < nSrcMaxHp * nParama) bFlag = true; break;
		case 7:if (nSrcHp > nParama) bFlag = true; break;
		case 8:if (nSrcHp > nSrcMaxHp * nParama) bFlag = true; break;
		case 9: bFlag = true; break;
		case 10:
		{
			if (pTarget && false == pTarget->IsRangeUnit())
				bFlag = true;
		}
			break;
		case 11:
		{
			if (pTarget && true == pTarget->IsRangeUnit())
				bFlag = true;
		}
			break;
		case 12:
		{
			if (GetRandom(0, 100) <= nParama * 100)
				bFlag = true;
		}
			break;
		case 14:
		{
			// if (pTarget->HasAdvanceBuffEffect(BUFF_ADVANCE_EFFECT::BAE_STUN) || pTarget->HasAdvanceBuffEffect(BUFF_ADVANCE_EFFECT::BAE_MOVE_HOLD))
			//bFlag = true;
		}
			break;
		case 15:
		{
			// if (m_pBattle->GetFriendlyUnitNum(m_enForce))
			bFlag = true;
		}
			break;
		case 16:
		{
			//if (m_pBattle->GetRangeFriendlyUnitNum(this, 1000) < nParama)
			bFlag = true;
		}
			break;
		default:bFlag = false; break;
		}

		if (false == bFlag)
			continue;

		for (int i = 0; i < pPassSkill->GetBaseSize(); i++)
		{
			float fAtkValue = pPassSkill->GetBaseValue(i) / 100;
			if (15 == pPassSkill->GetTriggerCondition())
			{
				fAtkValue = 3 * fAtkValue;
				fAtkValue = fAtkValue < 0.3f ? fAtkValue : 0.3f;
			}

			switch (pPassSkill->GetBaseType(i))
			{
			case BaseType_PhysicsDmg:
				fDamage = fDamage * (1.0f + fAtkValue);
				break;
			case BaseType_MagicDmg:
				fMDamage = fMDamage * (1.0f + fAtkValue);
				break;
			case BaseType_CritPercent:
				fCritOdds += pPassSkill->GetBaseValue(i) / 100;
				break;
			case BaseType_CritValue:
				fCritValue = pPassSkill->GetBaseValue(i) / 100;
				break;
			default:
				break;
			}
		}
	}
}

void COffLineHero::PrevBeDamagedSkillCalculate(const OfflineDamageInfo& di, float &fDamage, float &fMDamage)
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pPassSkill = it;

		if (!pPassSkill)
			continue;

		for (int i = 0; i < pPassSkill->GetBaseSize(); i++)
		{
			switch (pPassSkill->GetBaseType(i))
			{
			case BaseType_BePhysicsDmg:
				fDamage = (fDamage)* (1.0f + pPassSkill->GetBaseValue(i) / 100);
				break;
			case BaseType_BeMagicDmg:
				fMDamage = (fMDamage)* (1.0f + pPassSkill->GetBaseValue(i) / 100);
				break;
			default:
				break;
			}
		}

		if (TriggerTime_PreBeDamaged == pPassSkill->GetTriggerTime())
		{
			if (0 == it->GetTriggerInterval() % pPassSkill->GetTriggerInterval())
			{

				bool  bFlag{ false };
				const float nParama{ pPassSkill->GetTriggerConditionParama() };
				const __int64 nSrcHp{ di.pSrcHero->GetHP() };
				const __int64 nSrcMaxHp{ di.pSrcHero->GetMaxHP() };
				const int ReduceType = pPassSkill->GetReduceType();
				const int ReduceTypeExcept = pPassSkill->GetReduceTypeExpect();

				switch (pPassSkill->GetTriggerCondition())
				{
				case 0:bFlag = true; break;
				case 1:if (m_nHP < nParama) bFlag = true; break;
				case 2:if (m_nHP < m_nMaxHp * nParama) bFlag = true; break;
				case 3:if (m_nHP > nParama) bFlag = true; break;
				case 4:if (m_nHP > m_nMaxHp * nParama) bFlag = true; break;
				case 5:if (nSrcHp < nParama) bFlag = true; break;
				case 6:if (nSrcHp < nSrcMaxHp * nParama) bFlag = true; break;
				case 7:if (nSrcHp > nParama) bFlag = true; break;
				case 8:if (nSrcHp > nSrcMaxHp * nParama) bFlag = true; break;
				case 9:bFlag = true; break;
				case 10:
				{
					//if (!pTarget->IsRangeUnit())
					bFlag = true;
				}
					break;
				case 11:
				{
					//if (pTarget->IsRangeUnit())
					bFlag = true;
				}
					break;
				case 12:
				{
					if (GetRandom(0, 100) <= nParama * 100)
						bFlag = true;
				}
					break;
				case 13:
				{
				}
					break;
				case 15:
				{
					bFlag = true;
				}
				case 16:
				{
					bFlag = true;
				}
					break;
				default:bFlag = false; break;
				}

				if (false == bFlag)
					continue;

				if (TriggerType_Reduce == pPassSkill->GetTriggerType())
				{
					if (ReduceType & 1)
					{
						if (!di.bIsRangeAtk) bFlag = false;
					}
					if (ReduceType & 2)
					{
						if (di.bIsRangeAtk) bFlag = false;
					}
					if (ReduceType & 4)
					{
						if (AT_PHYSICS != di.nAtkType) bFlag = false;
					}
					if (ReduceType & 8)
					{
						if (AT_MAGIC != di.nAtkType) bFlag = false;
					}
					if (ReduceType & 16)
					{
						if (di.bIsHeroAtk) bFlag = false;
					}
					if (ReduceType & 32)
					{
						if (!di.bIsHeroAtk) bFlag = false;
					}
					if (ReduceType & 64)
					{
						if (di.bIsSkill) bFlag = false;
					}
					if (ReduceType & 128)
					{
						if (!di.bIsSkill) bFlag = false;
					}

					if (false == bFlag)
						continue;

					if (ReduceTypeExcept & 1)
					{
						if (di.bIsRangeAtk) bFlag = false;
					}
					if (ReduceTypeExcept & 2)
					{
						if (!di.bIsRangeAtk) bFlag = false;
					}
					if (ReduceTypeExcept & 4)
					{
						if (AT_PHYSICS == di.nAtkType) bFlag = false;
					}
					if (ReduceTypeExcept & 8)
					{
						if (AT_MAGIC == di.nAtkType) bFlag = false;
					}
					if (ReduceTypeExcept & 16)
					{
						if (!di.bIsHeroAtk) bFlag = false;
					}
					if (ReduceTypeExcept & 32)
					{
						if (di.bIsHeroAtk) bFlag = false;
					}
					if (ReduceTypeExcept & 64)
					{
						if (!di.bIsSkill) bFlag = false;
					}
					if (ReduceTypeExcept & 128)
					{
						if (di.bIsSkill) bFlag = false;
					}

					if (false == bFlag)
						continue;

					float fReducePercent = pPassSkill->GetReducePercent();
					if (15 == pPassSkill->GetTriggerCondition())
					{
						fReducePercent = 3 * fReducePercent;
						fReducePercent = fReducePercent < 0.20f ? fReducePercent : 0.20f;
					}

					if (di.nDamage && di.nMDamage)
					{
						fDamage = (fDamage - pPassSkill->GetReduceValue() / 2.0f) * (1.0f - fReducePercent);
						fMDamage = (fMDamage - pPassSkill->GetReduceValue() / 2.0f) * (1.0f - fReducePercent);
					}
					else if (di.nDamage)
					{
						fDamage = (fDamage - pPassSkill->GetReduceValue()) * (1.0f - fReducePercent);
					}
					else if (di.nMDamage)
					{
						fMDamage = (fMDamage - pPassSkill->GetReduceValue()) * (1.0f - fReducePercent);
					}
				}
			}
		}
	}
}

void COffLineHero::PrevAttackSkillCalculate(COffLineHero* pTarget)
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pPassSkill = it;

		if (!pPassSkill)
			continue;

		if (TriggerTime_PrevAtk == pPassSkill->GetTriggerTime())
		{
			if (0 == it->GetTriggerInterval() % pPassSkill->GetTriggerInterval())
			{
				bool bFlag{ false };
				const float nParama{ pPassSkill->GetTriggerConditionParama() };
				const __int64 nTargetHp{ pTarget->GetHP() };
				const __int64 nTargetMaxHp{ pTarget->GetMaxHP() };
				/*
				1.自身HP低于
				2.自身HP百分比低于
				3.自身HP高于
				4.自身HP百分比高于
				5.敌方HP低于
				6.敌方HP百分比低于
				7.敌方HP高于
				8.敌方HP百分比高于
				9.敌方是英雄
				10.敌方是近战
				11.敌方是远程*/

				switch (pPassSkill->GetTriggerCondition())
				{
				case 0:bFlag = true; break;
				case 1:if (m_nHP < nParama) bFlag = true; break;
				case 2:if (m_nHP < m_nMaxHp * nParama) bFlag = true; break;
				case 3:if (m_nHP > nParama) bFlag = true; break;
				case 4:if (m_nHP > m_nMaxHp * nParama) bFlag = true; break;
				case 5:if (nTargetHp < nParama) bFlag = true; break;
				case 6:if (nTargetHp < nTargetMaxHp * nParama) bFlag = true; break;
				case 7:if (nTargetHp > nParama) bFlag = true; break;
				case 8:if (nTargetHp > nTargetMaxHp * nParama) bFlag = true; break;
				case 9: bFlag = true; break;
				case 10:
				{
					if (!pTarget->IsRangeUnit())
						bFlag = true;
				}
					break;
				case 11:
				{
					if (pTarget->IsRangeUnit())
						bFlag = true;
				}
					break;
				case 16:
				{
					bFlag = true;
				}
					break;
				default:bFlag = false; break;
				}

				if (false == bFlag)
					continue;

				if (TriggerType_Skill == pPassSkill->GetTriggerType())
				{
					const CSkill* pSkill = CSkill::GetSkill(pPassSkill->GetTriggerSkillBuffID());

					if (!pSkill)
						continue;
				}
			}
		}
	}
}

void COffLineHero::AfterAttackSkillCalculate(COffLineHero* pTarget)
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pPassSkill = it;

		if (!pPassSkill)
			continue;


		if (TriggerTime_AfterAtk == pPassSkill->GetTriggerTime())
		{
			if (0 == it->GetTriggerInterval() % pPassSkill->GetTriggerInterval())
			{
				bool bFlag{ false };
				const float nParama{ pPassSkill->GetTriggerConditionParama() };
				const __int64 nTargetHp{ pTarget->GetHP() };
				const __int64 nTargetMaxHp{ pTarget->GetMaxHP() };
				/*
				1.自身HP低于
				2.自身HP百分比低于
				3.自身HP高于
				4.自身HP百分比高于
				5.敌方HP低于
				6.敌方HP百分比低于
				7.敌方HP高于
				8.敌方HP百分比高于
				9.敌方是英雄
				10.敌方是近战
				11.敌方是远程
				12.几率*/

				switch (pPassSkill->GetTriggerCondition())
				{
				case 0:bFlag = true; break;
				case 1:if (m_nHP < nParama) bFlag = true; break;
				case 2:if (m_nHP < m_nMaxHp * nParama) bFlag = true; break;
				case 3:if (m_nHP > nParama) bFlag = true; break;
				case 4:if (m_nHP > m_nMaxHp * nParama) bFlag = true; break;
				case 5:if (nTargetHp < nParama) bFlag = true; break;
				case 6:if (nTargetHp < nTargetMaxHp * nParama) bFlag = true; break;
				case 7:if (nTargetHp > nParama) bFlag = true; break;
				case 8:if (nTargetHp > nTargetMaxHp * nParama) bFlag = true; break;
				case 9:bFlag = true; break;
				case 10:
				{
					if (!pTarget->IsRangeUnit())
						bFlag = true;
				}
					break;
				case 11:
				{
					if (pTarget->IsRangeUnit())
						bFlag = true;
				}
					break;
				case 12:
				{
					if (GetRandom(1, 100) < nParama * 100)
						bFlag = true;
				}
					break;
				default:bFlag = false; break;
				}

				if (false == bFlag)
					continue;

				if (TriggerType_Skill == pPassSkill->GetTriggerType())
				{
					const CSkill* pSkill = CSkill::GetSkill(pPassSkill->GetTriggerSkillBuffID());

					if (!pSkill)
						continue;
				}

				//else if (TriggerType_SelfBuff == pPassSkill->GetTriggerType())
				//{
				//	SetBuff(pPassSkill->GetTriggerSkillBuffID(), nullptr);
				//}
			}
		}
	}
}

void COffLineHero::GetAlwaysPassSkill()
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pSkill = it;

		if (!pSkill)
			continue;

		if (TriggerTime_Always == pSkill->GetTriggerTime())
		{
			switch (pSkill->GetTriggerType())
			{
			case 0:
				break;
			default:
				break;
			}

			for (int i = 0; i < pSkill->GetBaseSize(); i++)
			{
				switch (pSkill->GetBaseType(i))
				{
				case 0:
					break;
				default:
					break;
				}
			}
		}
	}
}

void COffLineHero::Heal(int heal_hp)
{
	m_nHP += heal_hp;
	if (m_nHP > m_nMaxHp)
		m_nHP = m_nMaxHp;
}

void COffLineHero::__GetSuitAddtion(const CHeroCard* hero_card, COffLinePlayer* pPlayer, SuitAddition* suit_addition)
{
	std::map<int, int> suits;
	std::vector<int> suit_id;
	for (auto it = pPlayer->GetAllEquip()->begin(); it != pPlayer->GetAllEquip()->cend(); it++)
	{
		if (abs(it->second) != hero_card->hero_template()->GetID())
			continue;
		auto it_suit = suits.find(it->first->suit_id());
		if (suits.cend() == it_suit)
		{
			suits.insert(std::make_pair(it->first->suit_id(), 1));
			suit_id.push_back(it->first->suit_id());
		}
		else
			it_suit->second++;
	}

	int level = (pPlayer->GetLevel() / 20 + 1);
	for (size_t i = 0; i < suit_id.size(); i++)
	{
		const CSuitTP* suit_tp = CSuitTP::GetSuitTP(suit_id.at(i));

		if (suit_tp)
		{
			auto it_suit = suits.find(suit_id.at(i));
			if (it_suit != suits.cend())
			{
				if (it_suit->second >= 2)
				{
					float attribute = suit_tp->stage(2) * level + suit_tp->attribute(2);
					switch (suit_tp->attribute_type(2))
					{
					case kSuitAtk:			suit_addition->atk_ += (int)attribute;			break;
					case kSuitDef:			suit_addition->def_ += (int)attribute;			break;
					case kSuitHP:			suit_addition->hp_ += (int)attribute;			break;
					case kSuitMoveSpeed:	suit_addition->move_speed_ += (int)attribute;	break;
					case kSuitCritOdds:		suit_addition->crit_odds_ += (int)attribute;		break;
					case kSuitPreventCrit:	suit_addition->prevent_crit_ += (int)attribute;	break;
					case kSuitStr:			suit_addition->str_ += (int)attribute;			break;
					case kSuitCmd:			suit_addition->cmd_ += (int)attribute;			break;
					case kSuitInt:			suit_addition->int_ += (int)attribute;			break;
					case kSuitAtkSpeed:		suit_addition->atk_speed_ += (int)attribute;		break;
					case kSuitMaxTime:		suit_addition->max_time_ += (int)attribute;		break;
					}
				}

				if (it_suit->second >= 4)
				{
					float attribute = suit_tp->stage(4) * level + suit_tp->attribute(4);
					switch (suit_tp->attribute_type(4))
					{
					case kSuitAtk:			suit_addition->atk_ += (int)attribute;			break;
					case kSuitDef:			suit_addition->def_ += (int)attribute;			break;
					case kSuitHP:			suit_addition->hp_ += (int)attribute;			break;
					case kSuitMoveSpeed:	suit_addition->move_speed_ += (int)attribute;	break;
					case kSuitCritOdds:		suit_addition->crit_odds_ += (int)attribute;	break;
					case kSuitPreventCrit:	suit_addition->prevent_crit_ += (int)attribute;	break;
					case kSuitStr:			suit_addition->str_ += (int)attribute;			break;
					case kSuitCmd:			suit_addition->cmd_ += (int)attribute;			break;
					case kSuitInt:			suit_addition->int_ += (int)attribute;			break;
					case kSuitAtkSpeed:		suit_addition->atk_speed_ += (int)attribute;	break;
					case kSuitMaxTime:		suit_addition->max_time_ += (int)attribute;		break;
					}
				}

				if (it_suit->second >= 6)
					suit_addition->skill_id = it_suit->first;
			}
		}
	}
}