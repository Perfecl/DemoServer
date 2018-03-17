#include "stdafx.h"
#include "Unit.h"
#include "Master.h"
#include "Battle.h"
#include "Hero.h"
#include "Bullet.h"
#include "Building.h"

CUnit::CUnit(CBattle* pBattle) :
CBattleObject{ pBattle }
{

}

CUnit::~CUnit()
{

}

const CUnit* CUnit::CreateCustomUnit(const Soldier& soldier, const CMaster* master)
{
	const CSoldierTP* soldier_tp = CSoldierTP::GetSoldierTP(soldier.id());

	if (!soldier_tp)
		return nullptr;

	if (!master)
		return nullptr;

	CUnit* pUnit = new CUnit{ nullptr };

	pUnit->m_pSoldierTP = soldier_tp;
	pUnit->m_nMasterID = soldier.master();
	pUnit->m_enForce = (Force)soldier.belong();

	if (Force::ATK == pUnit->m_enForce)
		pUnit->m_bDirection = true;
	else if (Force::DEF == pUnit->m_enForce)
		pUnit->m_bDirection = false;

	pUnit->m_strName = soldier_tp->m_strName;
	pUnit->m_nPopulation = soldier_tp->m_nPopulation;

	pUnit->m_nHP = pUnit->m_nMaxHp = (int)((soldier_tp->m_nBaseHP + master->GetTechnologyHP()) * soldier_tp->m_fHP_mul);
	pUnit->m_nAtk = (int)((soldier_tp->m_nBaseATK + master->GetTechnologyAtk()) * soldier_tp->m_fATK_mul);
	pUnit->m_nDef = (int)((soldier_tp->m_nBaseDEF + master->GetTechnologyDef()) * soldier_tp->m_fDEF_mul);
	pUnit->m_nMatk = (int)((soldier_tp->m_nBaseMATK + master->GetTechnologyMAtk()) * soldier_tp->m_fMATK_mul);
	pUnit->m_nMdef = (int)((soldier_tp->m_nBaseMDEF + master->GetTechnologyMDef()) * soldier_tp->m_fMDEF_mul);
	pUnit->m_nStr = master->GetInteralStr();
	pUnit->m_nCmd = master->GetInteralCmd();
	pUnit->m_nInt = master->GetInteralInt();

	for (auto &it : soldier_tp->m_vctPassSkills)
	{
		const CPassSkill* pPassSkill = CPassSkill::GetPassSkill(it);
		if (pPassSkill)
			pUnit->m_vctPasSkills.push_back(std::make_tuple(pPassSkill, 0, 0));
	}

	pUnit->m_nResourceID = soldier_tp->m_nResourceID;
	pUnit->m_fVolume = soldier_tp->m_fVolume;
	pUnit->m_fMoveSpeed = soldier_tp->m_fMoveSpeed;

	pUnit->m_bIsRange = soldier_tp->m_bIsRange;
	pUnit->m_bIsHiding = pUnit->m_bCanHide = soldier_tp->m_bCanHide;

	pUnit->m_fAtkDistance = soldier_tp->m_fAtkDistance;
	pUnit->m_nAtkPrev = soldier_tp->m_nAtkPrev;
	pUnit->m_nAtkInterval = soldier_tp->m_nAtkInterval;

	pUnit->m_enAtkType = soldier_tp->m_enAtkType;
	pUnit->m_nAtkBuildingDamage = soldier_tp->m_nAtkBuildingDamage;

	pUnit->m_enNormalAtk = soldier_tp->m_enNormalAtk;
	pUnit->m_enHideAtk = soldier_tp->m_enHideAtk;
	pUnit->m_enSpecialAtk = soldier_tp->m_enSpecialAtk;

	pUnit->m_enSoldierType = soldier_tp->m_enSoldierType;

	pUnit->m_bIsBreakHero = soldier_tp->m_bIsBreakHero;


	pUnit->m_nMasterID = soldier.master();
	pUnit->m_fActiveRange = (float)soldier.active_range();

	pUnit->m_fPos = pUnit->m_fPatrolPoint = (float)soldier.pos();
	pUnit->m_bIsBackup = soldier.isbackup();
	pUnit->m_nStartBuffID = soldier.buff_id();
	pUnit->m_enAIType = (AIType)soldier.aitype();
	pUnit->m_nGroupID = soldier.group();
	pUnit->_GetAlwaysPassSkill();
	return pUnit;
}

void CUnit::Loop()
{
	_CalculateBuff();

	_CalculateState();
	_CalculateSkillCD();

	if (m_pBattle->TimeCounts(1000))
		_BaseHeal();

	if (_IsSpasticity())
		return;

	_Act();
}

void CUnit::InitUnit(const CSoldierTP *pSoldierTP, CMaster *pMaster, bool isSummonUnit/*=false*/)
{
	m_pSoldierTP = pSoldierTP;
	m_pMaster = pMaster;
	m_bIsSummonUnit = isSummonUnit;
	if (!pMaster)
		return;

	m_strName = pSoldierTP->m_strName;
	m_nPopulation = pSoldierTP->m_nPopulation;

	m_nHP = m_nMaxHp = (int)((pSoldierTP->m_nBaseHP + pMaster->GetTechnologyHP() + pMaster->GetSoldierExHP()) * pSoldierTP->m_fHP_mul);
	m_nAtk = (int)((pSoldierTP->m_nBaseATK + pMaster->GetTechnologyAtk() + pMaster->GetSoldierExAtk()) * pSoldierTP->m_fATK_mul);
	m_nDef = (int)((pSoldierTP->m_nBaseDEF + pMaster->GetTechnologyDef() + pMaster->GetSoldierExDef()) * pSoldierTP->m_fDEF_mul);
	m_nMatk = (int)((pSoldierTP->m_nBaseMATK + pMaster->GetTechnologyMAtk() + pMaster->GetSoldierExMAtk()) * pSoldierTP->m_fMATK_mul);
	m_nMdef = (int)((pSoldierTP->m_nBaseMDEF + pMaster->GetTechnologyMDef() + pMaster->GetSoldierExMDef()) * pSoldierTP->m_fMDEF_mul);

	if (m_pMaster->IsAI())
	{
		m_nStr = pMaster->GetInteralStr();
		m_nCmd = pMaster->GetInteralCmd();
		m_nInt = pMaster->GetInteralInt();
	}
	else
	{
		SoldierSoul* soldier_soul = m_pMaster->FindSoldierSoul(pSoldierTP->GetID());
		if (soldier_soul)
		{
			m_nStr = pSoldierTP->ms_nStr + soldier_soul->train_level_;
			m_nCmd = pSoldierTP->ms_nCmd + soldier_soul->train_level_;
			m_nInt = pSoldierTP->ms_nInt + soldier_soul->train_level_;
		}
		else
		{
			m_nStr = pSoldierTP->ms_nStr;
			m_nCmd = pSoldierTP->ms_nCmd;
			m_nInt = pSoldierTP->ms_nInt;
		}
	}

	m_nResourceID = pSoldierTP->m_nResourceID;
	m_fVolume = pSoldierTP->m_fVolume;
	m_fMoveSpeed = pSoldierTP->m_fMoveSpeed;

	m_bIsRange = pSoldierTP->m_bIsRange;
	m_bIsHiding = m_bCanHide = pSoldierTP->m_bCanHide;

	m_fAtkDistance = pSoldierTP->m_fAtkDistance;
	m_nAtkPrev = pSoldierTP->m_nAtkPrev;
	m_nAtkInterval = pSoldierTP->m_nAtkInterval;

	m_enAtkType = pSoldierTP->m_enAtkType;
	m_nAtkBuildingDamage = pSoldierTP->m_nAtkBuildingDamage;

	m_enNormalAtk = pSoldierTP->m_enNormalAtk;
	m_enHideAtk = pSoldierTP->m_enHideAtk;
	m_enSpecialAtk = pSoldierTP->m_enSpecialAtk;

	m_enSoldierType = pSoldierTP->m_enSoldierType;

	m_bIsBreakHero = pSoldierTP->m_bIsBreakHero;

	m_enForce = pMaster->GetForce();

	for (auto &it : pSoldierTP->m_vctPassSkills)
	{
		const CPassSkill* pPassSkill = CPassSkill::GetPassSkill(it);
		if (pPassSkill)
			m_vctPasSkills.push_back(make_tuple(pPassSkill, 0, 0));
	}

	if (ATK == m_enForce)
	{
		m_bDirection = true;
		m_fPos = 1;
	}
	else
	{
		m_bDirection = false;
		m_fPos = m_pBattle->GetWidth() - 1;
	}

	_GetAlwaysPassSkill();
}

void CUnit::InitBrithBuff(const CSoldierTP *pSoldierTP)
{
	if (pSoldierTP->m_nBirthBuff != 0)
		SetBuff(pSoldierTP->m_nBirthBuff, this);
}

void CUnit::SendUnitToClient(Force enForce/*= Force::UNKNOWN*/)
{
	pto_BATTLE_Struct_Unit pPtoUnit;

	SetUnitProtocol(&pPtoUnit);

	/*if (IsHiding())
		enForce = m_enForce;*/

	if (Force::UNKNOWN == enForce)
	{
		m_pBattle->GetProtoPackage(Force::ATK)->AddUnit(&pPtoUnit);
		m_pBattle->GetProtoPackage(Force::DEF)->AddUnit(&pPtoUnit);
	}
	else
	{
		m_pBattle->GetProtoPackage(enForce)->AddUnit(&pPtoUnit);
	}
}

void CUnit::SendCustomUnitToClient(Force enForce /*= Force::UNKNOWN*/)
{
	pto_BATTLE_S2C_NTF_AddCustomUnit ptoAddCustomUnit;
	auto ptoSoldier = ptoAddCustomUnit.mutable_soldier();

	ptoAddCustomUnit.set_name(ANSI_to_UTF8(m_strName));
	ptoAddCustomUnit.set_nlevel(level_);
	SetUnitProtocol(ptoSoldier);

	string strPto;
	ptoAddCustomUnit.SerializeToString(&strPto);

	/*if (IsHiding())
		enForce = m_enForce;*/

	if (Force::UNKNOWN == enForce)
		m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_AddCustomUnit);
	else
		m_pBattle->SendToForce(enForce, strPto, MSG_S2C, BATTLE_S2C_NTF_AddCustomUnit);
}

void CUnit::BeDamaged(const DamageInfo& di)
{
	if (US_DEAD == m_enState)
	{
		CUnit* unit = dynamic_cast<CUnit*>(di.pSrcObject);
		if (unit && unit->double_attack())
		{
			unit->set_double_attack(false);
		}
		return;
	}

	float fFinalDamage{ 0 };
	float fDamage = static_cast<float>(di.nDamage);
	float fMDamage = static_cast<float>(di.nMDamage);

	CUnit* di_unit = dynamic_cast<CUnit*>(di.pSrcObject);
	if (di_unit)
	{
		fDamage = (1 + di_unit->GetBaseBuffEffect(BUFF_BASIC_EFFECT::BBE_EX_ATK)) * fDamage;
		fMDamage = (1 + di_unit->GetBaseBuffEffect(BUFF_BASIC_EFFECT::BBE_EX_MATK)) * fMDamage;
	}

	int def = GetDef();
	int mdef = GetMdef();

	def -= static_cast<int>(GetBaseBuffEffect(BUFF_BASIC_EFFECT::BEE_NERF_DEF));
	mdef -= static_cast<int>(GetBaseBuffEffect(BUFF_BASIC_EFFECT::BEE_NERF_DEF));

	//伤害公式
	if (fDamage > 0)
	{
		if (di.nAtkType == ATK_TYPE::AT_BaseGun)
			fDamage = (fDamage - def) * di.fPhysicsRatio;
		else
			fDamage = (fDamage - def) * _DamageFormula(di.nStr, GetStr(), GetCmd()) * di.fPhysicsRatio;

		if (fDamage < 0)
			fDamage = 0;
	}
	if (fMDamage > 0)
	{
		if (di.nAtkType == ATK_TYPE::AT_BaseGun)
			fMDamage = (fDamage - mdef) * di.fPhysicsRatio;
		else
			fMDamage = (fMDamage - mdef) * _DamageFormula(di.nInt, GetInt(), GetCmd()) * di.fMagicRatio;

		if (fMDamage < 0)
			fMDamage = 0;
	}

	float fCritOdds{ 0 };
	float fCritValue{ 0 };

	_PrevBeDamagedSkillCalculate(di, fDamage, fMDamage);

	CHero* pAtkHero = dynamic_cast<CHero*>(di.pSrcObject);
	if (pAtkHero /*&& false == pAtkHero->m_bIsAI*/)
	{
		fCritOdds = pAtkHero->GetCritOdds();
		fCritValue = pAtkHero->GetCritValue();
		pAtkHero->AttackingSkillCalculate(this, fDamage, fMDamage, fCritOdds, fCritValue);
	}

	fDamage *= GetBaseBuffEffect(BBE_DEF);
	fMDamage *= GetBaseBuffEffect(BBE_MDEF);

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
		else if (AT_BaseGun == di.nAtkType)
		{
			fFinalDamage = fDamage + fMDamage;
			if (IsHero() || IsBoss())
				fFinalDamage = fFinalDamage / 2;
		}
	}

	fFinalDamage *= di.fRatio;

	bool		bIsCrit{ false };
	CMaster*	pMaster{ nullptr };
	int			nSrcObjID{ 0 };
	ObjectType	enSrcObjType{ OT_NULL };

	if (di.pSrcObject && di.nAtkType != ATK_TYPE::AT_BaseGun)
	{
		pMaster = di.pSrcObject->GetMaster();
		nSrcObjID = di.pSrcObject->GetID();
		enSrcObjType = di.pSrcObject->GetObjectType();

		//英雄护甲 近战减免15% 远程减免30%
		if (IsHero() && false == di.pSrcObject->IsHero())
		{
			CUnit* pUnit = dynamic_cast<CUnit*>(di.pSrcObject);
			if (pUnit && false == pUnit->IsBoss())
			{
				/*int nMasterNum;

				if (m_enForce == Force::ATK)
				{
				nMasterNum = m_pBattle->GetMasterNum(Force::DEF);
				}
				else
				{
				nMasterNum = m_pBattle->GetMasterNum(Force::ATK);
				}*/

				//近战减免
				if (false == pUnit->IsRangeUnit())
					fFinalDamage *= 0.85f;
				//fFinalDamage *= (0.9f - (0.1f*nMasterNum));
				//远程减免
				else
					fFinalDamage *= 0.7f;
				//fFinalDamage *= (0.85f - (0.15f*nMasterNum));
			}
		}

		//计算暴击率
		if (di.bIsSkill == false && (di.pSrcObject->IsHero() || di.pSrcObject->IsBoss()))	//BOSS也会爆击了 by LuX 2014.8.21
		{
			CHero* pHero = dynamic_cast<CHero*>(di.pSrcObject);
			if (pHero)
			{
				fCritOdds = fCritOdds - m_fPreventCrit;
				if (GetRandom(1, 100) <= (100 * fCritOdds))
				{
					fFinalDamage *= fCritValue;
					bIsCrit = true;
				}
			}
		}

		//英雄相克
		if (IsHero() && di.pSrcObject->IsHero() && !IsUnrestriction() && !IsBoss() && !di.pSrcObject->IsBoss())
		{
			CHero* pHero = dynamic_cast<CHero*>(this);
			CHero* pHeroSrc = dynamic_cast<CHero*>(di.pSrcObject);

			if (pHero && pHeroSrc)
			{
				if (HT_ATK == pHero->GetHeroType() && HT_DEF == pHeroSrc->GetHeroType())
					fFinalDamage *= 1.25f;
				else if (HT_DEF == pHero->GetHeroType() && HT_INT == pHeroSrc->GetHeroType())
					fFinalDamage *= 1.25f;
				else if (HT_DEF == pHero->GetHeroType() && HT_SKILL == pHeroSrc->GetHeroType())
					fFinalDamage *= 1.25f;
				else if (HT_INT == pHero->GetHeroType() && HT_ATK == pHeroSrc->GetHeroType())
					fFinalDamage *= 1.25f;
				else if (HT_SKILL == pHero->GetHeroType() && HT_ATK == pHeroSrc->GetHeroType())
					fFinalDamage *= 1.25f;
				else if (HT_ATK == pHero->GetHeroType() && HT_INT == pHeroSrc->GetHeroType())
					fFinalDamage *= 0.75f;
				else if (HT_ATK == pHero->GetHeroType() && HT_SKILL == pHeroSrc->GetHeroType())
					fFinalDamage *= 0.75f;
				else if (HT_DEF == pHero->GetHeroType() && HT_ATK == pHeroSrc->GetHeroType())
					fFinalDamage *= 0.75f;
				else if (HT_INT == pHero->GetHeroType() && HT_DEF == pHeroSrc->GetHeroType())
					fFinalDamage *= 0.75f;
				else if (HT_SKILL == pHero->GetHeroType() && HT_DEF == pHeroSrc->GetHeroType())
					fFinalDamage *= 0.75f;
			}
		}
	}

	//计算主基地等级加成(称为能量增幅)
	if (di.nAtkType != ATK_TYPE::AT_BaseGun)
	{
		if (pMaster != nullptr && m_pMaster != nullptr)
		{
			float rat = 1.0f + (pMaster->military_level() - m_pMaster->military_level()) * 0.05f;
			fFinalDamage *= rat;
		}
	}

	int min_damage = int((di.nDamage + di.nMDamage) * 0.15f);
	int nFinalDamage = static_cast<int>(fFinalDamage) > min_damage ? static_cast<int>(fFinalDamage) : min_damage;

	if (HasAdvanceBuffEffect(BAE_INVINCIBLE) && di.nAtkType != ATK_TYPE::AT_BaseGun)
		nFinalDamage = 0;

	if (nFinalDamage > 0)
		m_bBeDamaged = true;

	if (di.pSrcObject && di.nAtkType != ATK_TYPE::AT_BaseGun)
	{
		if (!di.bIsSkill)
		{
			if (di.pSrcObject->IsHero())
			{
				CUnit* pUnit = dynamic_cast<CUnit*>(di.pSrcObject);
				if (false == pUnit->IsBoss())
					nFinalDamage = nFinalDamage * (pUnit->GetBaseAtkPrevTime() + pUnit->GetBaseAktIntervalTime()) / 1000;
			}
			else
			{
				CUnit* pUnit = dynamic_cast<CUnit*>(di.pSrcObject);
				nFinalDamage = nFinalDamage * (pUnit->GetBaseAtkPrevTime() + pUnit->GetBaseAktIntervalTime()) / 1500;
			}
		}

		CUnit* pUnit = dynamic_cast<CUnit*>(di.pSrcObject);
		if (pUnit)
		{
			pUnit->SuckHPSkillCalculate(nFinalDamage);
			if (!di.bIsSkill && !di.bIsRangeAtk)
			{
				BramblesDmgSkillCalculate(nFinalDamage, pUnit);
			}
		}
	}

	if (false == IsHero() && di.pSrcObject && di.bIsBuliding == false)
	{
		for (auto &it : ((CUnit*)di.pSrcObject)->m_vctPasSkills)
		{
			for (auto &it_skill : std::get<0>(it)->GetRestriction())
			{
				if (it_skill.first == m_pSoldierTP->GetID())
					nFinalDamage = int(nFinalDamage * (1 + it_skill.second / 100));
			}
		}
	}

	int attack_times = 1;
	CUnit* unit = dynamic_cast<CUnit*>(di.pSrcObject);
	if (unit && unit->double_attack())
	{
		attack_times = 2;
		unit->set_double_attack(false);
	}

	m_nHP -= nFinalDamage * attack_times;

	if (m_enAIType == AI_MOVE_AFTER_BE_ATK)
		m_enAIType = AI_NORMAL;

	if (pMaster)
		pMaster->ChangeMakeDamage(nFinalDamage * attack_times);

	else if (di.master_)
		di.master_->ChangeMakeDamage(nFinalDamage * attack_times);

	for (int i = 0; i < attack_times; i++)
	{
		m_pBattle->GetProtoPackage(Force::ATK)->BeDamage(m_nID, nFinalDamage, GetObjectType(), nSrcObjID, enSrcObjType, bIsCrit, 0, !di.bIsSkill);
		m_pBattle->GetProtoPackage(Force::DEF)->BeDamage(m_nID, nFinalDamage, GetObjectType(), nSrcObjID, enSrcObjType, bIsCrit, 0, !di.bIsSkill);
	}

	if (m_nHP <= 0)
	{
		_Die(di.pSrcObject);
	}
	else
	{
		if (false == IsHero() && m_nBeDamageProtected <= 0)
		{
			m_nResetBeDamagedTime = 0;

			m_nBeDamageCount += nFinalDamage * attack_times;

			if (m_nBeDamageCount > int(m_nMaxHp / 4))
			{
				m_nBeDamageCount = m_nResetBeDamagedTime = 0;

				float fFallbackPos{ m_fPos };
				if (Force::ATK == m_enForce)
					fFallbackPos -= (120 + GetRandom(1, 100));
				else if (Force::DEF == m_enForce)
					fFallbackPos += (120 + GetRandom(1, 100));
				Fallback(fFallbackPos);
			}
		}
	}
}

bool CUnit::IsSoldier() const
{
	if (false == IsHero() && false == IsBoss())
		return true;
	else
		return false;
}

bool CUnit::Fallback(float fPos)
{
	if (US_DEAD == m_enState)
		return false;

	if (HasAdvanceBuffEffect(BAE_INVINCIBLE))
		return false;

	if (m_bImmunePush)
		return false;

	_StopAttack();
	_StopUseSkill();

	float fOldPos{ m_fPos };

	m_fPos = fPos;

	_AdjustPos();

	m_nBeDamageProtected = BE_DAMAGED_PROTECTED_TIME;
	m_nSpasticityCountBeDamage = 600;

	float fFallbackDistance = abs(fOldPos - m_fPos);
	if (fFallbackDistance > 300)
		m_nSpasticityCountBeDamage += int((fFallbackDistance - 300) / 50 * 100);

	if (m_enForce == Force::ATK &&
		m_fPos < m_pBattle->GetGatePos(this))
		m_fPos = m_pBattle->GetGatePos(this);

	else if (m_enForce == Force::DEF&&
		m_fPos > m_pBattle->GetGatePos(this))
		m_fPos = m_pBattle->GetGatePos(this);

	m_pBattle->GetProtoPackage(Force::ATK)->FallBack(m_nID, m_fPos);
	m_pBattle->GetProtoPackage(Force::DEF)->FallBack(m_nID, m_fPos);

	m_pBattle->ChargeBulletDie(this);

	return true;
}

void CUnit::Heal(int nNum)
{
	m_nHP += nNum;
	if (m_nHP > m_nMaxHp)
		m_nHP = m_nMaxHp;

	m_pBattle->GetProtoPackage(Force::ATK)->Heal(m_nID, nNum, m_nID, GetObjectType(), 0, GetObjectType(), false);
	m_pBattle->GetProtoPackage(Force::DEF)->Heal(m_nID, nNum, m_nID, GetObjectType(), 0, GetObjectType(), false);
}

CUnit* CUnit::Clone(CBattle* pBattle) const
{
	CUnit* pUnit = new CUnit{ *this };

	pUnit->m_pBattle = pBattle;

	pUnit->m_pMaster = pBattle->FindMasterByMID(m_nMasterID);

	return pUnit;
}

void CUnit::SetUnitProtocol(pto_BATTLE_Struct_Unit* ptoUnit)
{
	ptoUnit->set_bdirection(m_bDirection);
	ptoUnit->set_nlevel(level_);
	if (m_pMaster)
		ptoUnit->set_nmasterid(m_pMaster->GetMasterID());

	ptoUnit->set_bishero(IsHero());
	ptoUnit->set_fmovespeed(m_fMoveSpeed);
	bool bIsDead{ false };
	if (US_DEAD == m_enState)
		bIsDead = true;
	ptoUnit->set_bisdead(bIsDead);
	ptoUnit->set_bishide(m_bCanHide);
	ptoUnit->set_nid(m_nID);
	ptoUnit->set_nx(m_fPos);
	ptoUnit->set_enforce(m_enForce);
	ptoUnit->set_nhp(m_nHP);
	ptoUnit->set_nmaxhp(m_nMaxHp);
	ptoUnit->set_nbodyrange(m_fVolume);
	ptoUnit->set_nattackrange((int)m_fAtkDistance);
	ptoUnit->set_nattackpretime(m_nAtkPrev);
	ptoUnit->set_nattackintervaltime(m_nAtkInterval);
	ptoUnit->set_bissummonunit(m_bIsSummonUnit);

	//------临时----
	ptoUnit->set_nresourceid(m_nResourceID);
	ptoUnit->set_bisrangeunit(m_bIsRange);

	ptoUnit->set_nposindex(m_nPosIndex);

	ptoUnit->set_enattackpettern_normal(m_enNormalAtk);
	ptoUnit->set_enattackpettern_ishiding(m_enHideAtk);
	ptoUnit->set_enattackpettern_support(m_enSpecialAtk);

	ptoUnit->set_bisbreakhero(m_bIsBreakHero);

	//------ 技能 ------
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pPassSkill = std::get<0>(it);

		if (!pPassSkill)
			continue;
		ptoUnit->add_vctpasskillid(pPassSkill->GetSkillID());
	}
}

void CUnit::Retreat()
{
	m_enState = US_STAND;
	if (Force::ATK == m_enForce)
		m_bDirection = false;
	else if (Force::DEF == m_enForce)
		m_bDirection = true;

	m_enAIType = AI_RETREAT;
	m_enState = US_STAND;
}

void CUnit::StopRetreat()
{
	m_enAIType = AI_NORMAL;
	_StopMove(true);
}

void CUnit::Fear()
{
	if (US_DEAD == m_enState)
		return;

	if (HasAdvanceBuffEffect(BAE_INVINCIBLE))
		return;

	if (m_bImmuneFear)
		return;

	if (US_SKILL == m_enState)
		_StopUseSkill();
	else if (US_ATK == m_enState)
		_StopAttack();

	if (Force::ATK == m_enForce)
		m_bDirection = false;
	else if (Force::DEF == m_enForce)
		m_bDirection = true;

	if (!HasAdvanceBuffEffect(BAE_STUN))
		_StartMove();
}

void CUnit::StopFear()
{
	if (m_enAIType != AI_RETREAT)
	{
		if (Force::ATK == m_enForce)
			m_bDirection = true;
		else if (Force::DEF == m_enForce)
			m_bDirection = false;
	}
	_StopMove(true);
}

void CUnit::Stun()
{
	if (HasAdvanceBuffEffect(BAE_INVINCIBLE))
		return;

	if (US_MOVING == m_enState)
		_StopMove(false);
	else if (US_SKILL == m_enState)
		_StopUseSkill();
	else if (US_ATK == m_enState)
		_StopAttack();
}

void CUnit::Remove()
{
	m_enState = US_DEAD;

	if (false == IsHero())
	{
		if (m_pMaster && !m_bIsSummonUnit)
			m_pMaster->ChangePopulation(-m_nPopulation);
	}
}

void CUnit::SuckHPSkillCalculate(int nFinalDamage)
{
	for (size_t i = 0; i < m_vctPasSkills.size(); ++i)
	{
		const CPassSkill *pPassSkill = get<0>(m_vctPasSkills[i]);

		if (!pPassSkill)
			continue;

		if (m_pTarget && m_pTarget->GetObjectType() == OT_BUILDING)
			continue;

		if (TriggerTime_Always == pPassSkill->GetTriggerTime())
		{
			if (TriggerType_Base == pPassSkill->GetTriggerType())
			{
				for (int i = 0; i < pPassSkill->GetBaseSize(); i++)
				{
					if (dat_SKILL_ENUM_BaseType::BaseType_SuckHP == pPassSkill->GetBaseType(i))
					{
						Heal(int(nFinalDamage * pPassSkill->GetBaseValue(i) / 100));
					}
				}
			}
		}
	}
}

void CUnit::BramblesDmgSkillCalculate(int nFinalDamage, CUnit* pTarget)
{
	for (size_t i = 0; i < m_vctPasSkills.size(); ++i)
	{
		const CPassSkill *pPassSkill = get<0>(m_vctPasSkills[i]);

		if (!pPassSkill)
			continue;

		if (m_pTarget && m_pTarget->GetObjectType() == OT_BUILDING)
			continue;

		if (TriggerTime_Always == pPassSkill->GetTriggerTime())
		{
			if (TriggerType_Base == pPassSkill->GetTriggerType())
			{
				for (int i = 0; i < pPassSkill->GetBaseSize(); i++)
				{
					if (dat_SKILL_ENUM_BaseType::BaseType_BramblesDmg == pPassSkill->GetBaseType(i))
					{
						int nDamage = int(nFinalDamage * pPassSkill->GetBaseValue(i) / 100);
						if (nDamage > GetMaxHP() * 0.5f)
							nDamage = int(GetMaxHP() * 0.5f);
						if (nDamage < 5) nDamage = 5;
						pTarget->BeBramblesDmg(nDamage, this);
					}
				}
			}
		}
	}
}

void CUnit::BeBramblesDmg(int nDamage, CBattleObject* pSrc)
{
	m_nHP -= nDamage;

	if (m_enAIType == AI_MOVE_AFTER_BE_ATK)
		m_enAIType = AI_NORMAL;

	if (pSrc)
	{
		if (pSrc->GetMaster())
			pSrc->GetMaster()->ChangeMakeDamage(nDamage);

		m_pBattle->GetProtoPackage(Force::ATK)->BeDamage(m_nID, nDamage, GetObjectType(), pSrc->GetID(), pSrc->GetObjectType(), false, 0, false);
		m_pBattle->GetProtoPackage(Force::DEF)->BeDamage(m_nID, nDamage, GetObjectType(), pSrc->GetID(), pSrc->GetObjectType(), false, 0, false);

		if (m_nHP <= 0)
			_Die(pSrc);
	}
}

void CUnit::BuffSkillCalculate(CBuff* pBuff)
{
	for (size_t i = 0; i < m_vctPasSkills.size(); ++i)
	{
		const CPassSkill *pPassSkill = std::get<0>(m_vctPasSkills[i]);

		if (!pPassSkill)
			continue;

		if (m_pTarget && m_pTarget->GetObjectType() == OT_BUILDING)
			continue;

		if (TriggerTime_Always == pPassSkill->GetTriggerTime())
		{
			if (TriggerType_Base == pPassSkill->GetTriggerType())
			{
				for (int i = 0; i < pPassSkill->GetBaseSize(); i++)
				{
					if (dat_SKILL_ENUM_BaseType::BaseType_DebuffTimePercent == pPassSkill->GetBaseType(i))
					{
						pBuff->ChangeTime(-(int)(pBuff->GetTime() * pPassSkill->GetBaseValue(i) / 100));
					}
				}
			}
		}
	}
}

void CUnit::ProduceLootBullet()
{
	if (m_pBattle->GetBattleMode() != BattleMode::ELITESTAGE &&
		m_pBattle->GetBattleMode() != BattleMode::STAGE)
		return;
	if (m_enForce != Force::DEF)
		return;
	CMaster* master = m_pBattle->FindMasterByMID(m_nMasterID);
	if (master && !master->IsAI())
		return;
	if (m_pBattle->stage_loot_times() >= 3)
		return;

	if (IsBoss())
	{
		if (30 < GetRandom(0, 100))
			return;
	}
	else if (6 < GetRandom(0, 100))
		return;

	int bullet_id = CStageLoot::ProduceBulletID();
	float pos = m_fPos;
	for (auto it : *m_pBattle->GetBulidingList(Force::DEF))
	{
		if (pos >= it.second->GetPos() - it.second->GetVolume() &&
			pos <= it.second->GetPos() + it.second->GetVolume())
		{
			pos = it.second->GetPos() - it.second->GetVolume();
		};
	}
	m_pBattle->ProuduceBullet(pos, bullet_id, 10);
	m_pBattle->ChangeStageLootTimes(1);
}

void CUnit::StartPatrol()
{
	if (m_enAIType != AI_NORMAL)
		return;

	float patrol_point{ 0 };
	if (m_enForce == Force::ATK)
	{
		patrol_point = m_pBattle->GetWidth() - m_fAtkDistance - 100;
		if (m_fPos < patrol_point)
			return;
	}

	if (m_enForce == Force::DEF)
	{
		patrol_point = m_fAtkDistance + 100;
		if (m_fPos > patrol_point)
			return;
	}

	ChangeAIType(AI_PATROL);
	SetActiveRange(1200);
	SetPatrolPoint(patrol_point);
}

bool CUnit::_IsSpasticity()
{
	if (m_nSpasticityCountBeDamage > 0 || m_nSpasticityCountSkill > 0)
		return true;

	if (HasAdvanceBuffEffect(BAE_STUN))
		return true;

	return false;
}

void CUnit::_Act()
{
	if (HasAdvanceBuffEffect(BAE_FEAR))
	{
		_MovingLoop();
		return;
	}

	if (not_attack_ && m_enAIType == AI_NORMAL)
	{
		if (m_enState == US_MOVING && m_pBattle->UnitHold(this))
			_StopMove(true);
		else if (m_enState == US_STAND && !m_pBattle->UnitHold(this) && m_enAIType != AI_NOT_MOVE)
			_StartMove();
		else if (m_enState == US_MOVING && !m_pBattle->UnitHold(this))
			_MovingLoop();
		return;
	}

	if (US_ATK == m_enState)
	{
		_AttackLoop();
		return;
	}

	if (AI_RETREAT == m_enAIType)
	{
		_MovingLoop();
		return;
	}
	else if (AI_ALERT == m_enAIType)
	{
		if (m_pBattle->FindEnemy(this, true))
			m_enAIType = AI_NORMAL;
	}
	else if (AI_PATROL == m_enAIType)
	{
		_PatrolLoop();
		return;
	}

	m_pTarget = m_pBattle->FindEnemy(this);

	if (m_pTarget)
		_AttackLoop();
	else
		_MovingLoop();
}

void CUnit::_PatrolLoop()
{
	float f1{ 0 }, f2{ 0 };

	if (Force::ATK == m_enForce)
	{
		f1 = m_fPos;
		f2 = m_fPatrolPoint + m_fActiveRange;
	}
	else if (Force::DEF == m_enForce)
	{
		f1 = m_fPatrolPoint - m_fActiveRange;
		f2 = m_fPos;
	}

	if (m_pBattle->FindEnemyInRange(f1, f2, m_enForce))
	{
		if ((Force::ATK == m_enForce && false == m_bDirection) || (Force::DEF == m_enForce && true == m_bDirection))
			_StopMove(true);

		m_pTarget = m_pBattle->FindEnemy(this);

		if (m_pTarget)
			_AttackLoop();
		else
			_MovingLoop();
	}
	else
	{
		if (US_MOVING == m_enState)
			_StopMove(true);

		if (Force::ATK == m_enForce)
		{
			if (m_fPos > m_fPatrolPoint + m_fMoveSpeed)
			{
				m_bDirection = false;
				_MovingLoop();
			}
			if (m_fPos < m_fPatrolPoint - m_fMoveSpeed)
			{
				m_bDirection = true;
				_MovingLoop();
			}
		}
		else if (Force::DEF == m_enForce)
		{
			if (m_fPos < m_fPatrolPoint - m_fMoveSpeed)
			{
				m_bDirection = true;
				_MovingLoop();
			}
			if (m_fPos > m_fPatrolPoint + m_fMoveSpeed)
			{
				m_bDirection = false;
				_MovingLoop();
			}
		}
	}
}

void CUnit::_MovingLoop()
{
	if (false == IsHero() && m_nAtkIntervalCounts > 0)
		return;

	if (US_ATK == m_enState)
		_StopAttack();

	if (AI_NOT_MOVE == m_enAIType)
		return;
	if (AI_PATROL != m_enAIType && AI_RETREAT != m_enAIType && AI_NORMAL != m_enAIType && !not_attack_)
		return;

	if (US_MOVING != m_enState)
		_StartMove();

	int wall_hit_test_result = m_pBattle->HitTestWallBullet(this);

	if (m_bDirection)
		m_fPos += GetMoveSpeed();
	else
		m_fPos -= GetMoveSpeed();

	if (m_pBattle->HitTestWallBullet(this))
	{
		if (true == m_bDirection)
		{
			if (wall_hit_test_result == 0 || wall_hit_test_result == 1)
				m_fPos -= GetMoveSpeed();
		}
		else if (false == m_bDirection)
		{
			if (wall_hit_test_result == 0 || wall_hit_test_result == 2)
				m_fPos += GetMoveSpeed();
		}
	}

	if (m_pBattle->HitTestGate(this))
	{
		if (Force::ATK == m_enForce && true == m_bDirection)
		{
			m_fPos -= GetMoveSpeed();
			_StopMove(true);
		}
		else if (Force::DEF == m_enForce && false == m_bDirection)
		{
			m_fPos += GetMoveSpeed();
			_StopMove(true);
		}
	}

	_AdjustPos();
	StartPatrol();
}

void CUnit::_StartMove()
{
	m_enState = US_MOVING;

	/*if (m_bIsHiding)
	{
	m_pBattle->GetProtoPackage(m_enForce)->UnitStartMove(m_nID, m_bDirection);
	}
	else
	{*/
	m_pBattle->GetProtoPackage(Force::ATK)->UnitStartMove(m_nID, m_bDirection);
	m_pBattle->GetProtoPackage(Force::DEF)->UnitStartMove(m_nID, m_bDirection);
	//}
}

void CUnit::_StopMove(bool send)
{
	m_enState = US_STAND;

	if (m_enAIType != AI_RETREAT && !HasAdvanceBuffEffect(BAE_FEAR))
	{
		if (Force::ATK == m_enForce)
			m_bDirection = true;
		else if (Force::DEF == m_enForce)
			m_bDirection = false;
	}

	if (send)
	{
		m_pBattle->GetProtoPackage(Force::ATK)->StopMove(m_nID, 0, m_fPos);
		m_pBattle->GetProtoPackage(Force::DEF)->StopMove(m_nID, 0, m_fPos);
	}
}

void CUnit::_AttackLoop()
{
	if (US_MOVING == m_enState)
		_StopMove(false);

	if (m_nAtkIntervalCounts > 0)
		return;

	if (US_ATK != m_enState)
		_StartAttack();

	m_nAtkPrevCounts += BATTLE_LOOP_TIME;

	if (m_pUsingPassSkillIndex >= 0)
		_UsingSkill();
	else
		_Attacking();
}

void CUnit::_StartAttack()
{
	m_enState = US_ATK;

	if (m_bIsHiding)
		_BreakHide();

	_PrevAttackSkillCalculate();

	if (m_pUsingPassSkillIndex >= 0)
	{
		_StartUseSkill();
	}
	else
	{
		m_pBattle->GetProtoPackage(Force::ATK)->StartAttack(m_nID, m_pTarget->GetID(), m_pTarget->GetObjectType(), m_fPos);
		m_pBattle->GetProtoPackage(Force::DEF)->StartAttack(m_nID, m_pTarget->GetID(), m_pTarget->GetObjectType(), m_fPos);
	}
}

void CUnit::_Attacking()
{
	if (m_nAtkPrevCounts < GetAtkPrevTime())
		return;

	if (m_pTarget && m_pTarget->GetObjectType() == ObjectType::OT_UNIT && ((CUnit*)m_pTarget)->m_enState == US_DEAD)
		m_pTarget = m_pBattle->FindEnemy(this);

	if (m_pTarget)
	{
		DamageInfo damgeInfo;
		damgeInfo.bIsArea = false;
		damgeInfo.bIsSkill = false;
		damgeInfo.bIsHeroAtk = IsHero();
		damgeInfo.bIsRangeAtk = IsRangeUnit();
		damgeInfo.nStr = GetStr();
		damgeInfo.nInt = GetInt();
		damgeInfo.nCmd = GetCmd();

		if (OT_BUILDING == m_pTarget->GetObjectType())
		{
			if (m_enForce == Force::DEF &&
				IsBoss() && m_nAtkBuildingDamage < 200)
			{
				m_nAtkBuildingDamage = 200;
			}
			if (AT_MAGIC == m_enAtkType)
			{
				damgeInfo.nDamage = m_nAtkBuildingDamage;
			}
			else if (AT_PHYSICS == m_enAtkType)
			{
				damgeInfo.nMDamage = m_nAtkBuildingDamage;
			}
			else if (AT_BLEND == m_enAtkType)
			{
				damgeInfo.nDamage = damgeInfo.nMDamage = m_nAtkBuildingDamage / 2;
				damgeInfo.nDamage += (m_nAtkBuildingDamage % 2);
			}


		}
		else if (OT_UNIT == m_pTarget->GetObjectType())
		{
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
		}

		damgeInfo.nAtkType = m_enAtkType;
		damgeInfo.pSrcObject = this;

		m_pTarget->BeDamaged(damgeInfo);
		_AfterAttackSkillCalculate();
	}

	_StopAttack();
	m_nAtkIntervalCounts = GetAtkIntervalTime();
}

void CUnit::_StopAttack()
{
	m_nAtkPrevCounts = 0;

	if (m_enState != US_DEAD)
		m_enState = US_STAND;

	if (false == IsHero() || m_pMaster->IsAI())
		_StopUseSkill();
}

void CUnit::_UsingSkill()
{
	const CSkill* pSkill{ nullptr };

	if (HasAdvanceBuffEffect(BAE_SILENCE))
		return;

	if (m_pUsingPassSkillIndex >= 0 && (size_t)m_pUsingPassSkillIndex < m_vctPasSkills.size())
	{
		const CPassSkill* pPassSkill = std::get<0>(m_vctPasSkills[m_pUsingPassSkillIndex]);
		pSkill = CSkill::GetSkill(pPassSkill->GetTriggerSkillBuffID());
	}

	if (!pSkill)
	{
		int("找不到使用的技能");
		return;
	}

	if (m_nAtkPrevCounts < pSkill->GetPrevTime())
		return;

	if (pSkill->GetHealthEffect())
		Heal(int(m_nMaxHp* pSkill->GetHealthEffect()));

	for (size_t i = 0; i < pSkill->GetBulletSize(); i++)
	{
		const CBulletTP* pBulletTP = CBulletTP::GetBulletTP(pSkill->GetBulletID(i));
		if (!pBulletTP)
			continue;

		CBullet *pBullet = new CBullet(m_pBattle);
		pBullet->InitBullet(pBulletTP, this, m_fPos);
		pBullet->SetSkill(pSkill);
		m_pBattle->AddObject(pBullet);
	}

	SKILL_SEPECIAL_EFFECT enSSE = pSkill->GetSepecialEffect();
	if (enSSE == SSE_CLEAR_SOLDIER_CARD_CD)
	{
		m_pMaster->ClearSoldierCardCD();
	}
	else if (enSSE == SSE_ADD_RESOURCE_300)
	{
		m_pMaster->ChangeResource(300);
	}
	else if (enSSE == SSE_ADD_RESOURCE_25)
	{
		m_pMaster->ChangeResource(25);
	}
	else if (enSSE == SSE_JUMP_BACK)
	{
		if (Force::ATK == m_enForce)
			m_fPos -= 400;
		else if (Force::DEF == m_enForce)
			m_fPos += 400;

		_AdjustPos();
	}
	else if (enSSE == SSE_ENTER_HIDE)
	{
		m_bIsHiding = true;
	}
	else if (enSSE == SSE_KILL_SELF)
	{
		_Die(this);
	}
	else if (enSSE == SSE_DOUBLE_ATTACK)
	{
		set_double_attack(true);
	}

	if (SSE_NULL != enSSE)
	{
		pto_BATTLE_S2C_NTF_SkillSepecialEffect pto;
		pto.set_id(m_nID);
		pto.set_effect(enSSE);
		std::string strPto;
		pto.SerializeToString(&strPto);
		if (enSSE == SSE_JUMP_BACK || enSSE == SSE_ENTER_HIDE)
			m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_SkillSepecialEffect);
		else
			m_pBattle->SendToMaster(m_pMaster, strPto, MSG_S2C, BATTLE_S2C_NTF_SkillSepecialEffect);
	}

	for (size_t i = 0; i < pSkill->GetBuffSize(); i++)
		SetBuff(pSkill->GetBuffID(i), this);

	std::get<1>(m_vctPasSkills[m_pUsingPassSkillIndex]) = pSkill->GetCD();
	std::get<2>(m_vctPasSkills[m_pUsingPassSkillIndex])++;

	m_nSpasticityCountSkill = pSkill->GetSpasticTime();

	m_pUsingPassSkillIndex = -1;
	_StopAttack();

	m_nAtkIntervalCounts = GetAtkIntervalTime();
}

void CUnit::_StopUseSkill()
{
	m_nAtkPrevCounts = 0;
	m_pUsingPassSkillIndex = -1;

	if (m_enState != US_DEAD)
		m_enState = US_STAND;
}

void CUnit::_StartUseSkill()
{
	m_pBattle->GetProtoPackage(Force::ATK)->StartUseSkill(m_nID, m_pUsingPassSkillIndex, ST_PASS_SKILL, m_fPos);
	m_pBattle->GetProtoPackage(Force::DEF)->StartUseSkill(m_nID, m_pUsingPassSkillIndex, ST_PASS_SKILL, m_fPos);
}

void CUnit::_AdjustPos()
{
	if (m_fPos < 0)
		m_fPos = 0;

	if (m_fPos > m_pBattle->GetWidth())
		m_fPos = m_pBattle->GetWidth();
}

void CUnit::_Die(CBattleObject* pSrc)
{
	ProduceLootBullet();
	m_enState = US_DEAD;

	m_pBattle->GetProtoPackage(Force::ATK)->UnitDead(m_nID);
	m_pBattle->GetProtoPackage(Force::DEF)->UnitDead(m_nID);

	if (false == IsHero())
	{
		if (m_pMaster && !m_bIsSummonUnit)
			m_pMaster->ChangePopulation(-m_nPopulation);

		if (pSrc)
		{
			CMaster *pMaster{ pSrc->GetMaster() };
			if (pMaster && pMaster->GetMasterID() != m_nMasterID)
				pMaster->ChangeKillSoldiers(1);
		}
	}
}

void CUnit::_RemoveUnit()
{
	m_enState = US_DEAD;

	if (false == IsHero())
	{
		if (m_pMaster && !m_bIsSummonUnit)
			m_pMaster->ChangePopulation(-m_nPopulation);
	}
}

void CUnit::_CalculateState()
{
	//攻击间隔
	if (m_nAtkIntervalCounts > 0)
	{
		m_nAtkIntervalCounts -= BATTLE_LOOP_TIME;
		if (m_nAtkIntervalCounts < 0)
			m_nAtkIntervalCounts = 0;
	}

	//受击保护
	if (m_nBeDamageProtected > 0)
	{
		m_nBeDamageProtected -= BATTLE_LOOP_TIME;

		if (m_nBeDamageProtected < 0)
			m_nBeDamageProtected = 0;
	}

	//受击累计伤害
	if (m_nBeDamageCount > 0)
	{
		m_nResetBeDamagedTime += BATTLE_LOOP_TIME;

		if (m_nResetBeDamagedTime >= BE_DAMAGED_RESET_TIME)
		{
			m_nBeDamageCount = 0;
			m_nResetBeDamagedTime = 0;
		}
	}

	//受击时间
	if (m_nSpasticityCountBeDamage > 0)
	{
		m_nSpasticityCountBeDamage -= BATTLE_LOOP_TIME;
		if (m_nSpasticityCountBeDamage <= 0)
		{
			m_nSpasticityCountBeDamage = 0;
			if (HasAdvanceBuffEffect(BAE_FEAR))
			{
				Fear();
			}
		}
	}

	//技能硬直时间
	if (m_nSpasticityCountSkill > 0)
	{
		m_nSpasticityCountSkill -= BATTLE_LOOP_TIME;
		if (m_nSpasticityCountSkill < 0)
			m_nSpasticityCountSkill = 0;
	}
}

void CUnit::_CalculateSkillCD()
{
	for (auto &it : m_vctPasSkills)
	{
		if (std::get<1>(it) > 0)
		{
			std::get<1>(it) -= BATTLE_LOOP_TIME;
			if (std::get<1>(it) < 0)
				std::get<1>(it) = 0;
		}
	}
}

float CUnit::_DamageFormula(int src_Str_Int, int dest_Str_Int, int dest_cmd)
{
	float fResult{ 0 };

	float fV1 = static_cast<float>(src_Str_Int);
	float fV2 = static_cast<float>(dest_Str_Int);
	float fV3 = static_cast<float>(dest_cmd);

	float fValueDef = fV2 * 0.25f + fV3 * 0.75f;

	if (fV1 >= fValueDef)
	{
		fResult = 1.0f + ((fV1 - fValueDef) * 0.02f);
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

void CUnit::_BreakHide()
{
	m_bIsHiding = false;

	/*if (Force::ATK == m_enForce)
		SendUnitToClient(Force::DEF);
		else if (Force::DEF == m_enForce)
		SendUnitToClient(Force::ATK);*/

	m_pBattle->GetProtoPackage(Force::ATK)->BreakHide(m_nID);
	m_pBattle->GetProtoPackage(Force::DEF)->BreakHide(m_nID);
}

void CUnit::_PrevBeDamagedSkillCalculate(const DamageInfo& di, float &fDamage, float &fMDamage)
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pPassSkill = get<0>(it);

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
			if (0 == get<2>(it) % pPassSkill->GetTriggerInterval())
			{
				bool  bFlag{ false };
				const float nParama{ pPassSkill->GetTriggerConditionParama() };
				const int nSrcHp{ di.pSrcObject->GetHP() };
				const int nSrcMaxHp{ di.pSrcObject->GetMaxHP() };
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
				case 9:if (m_pTarget->IsHero()) bFlag = true; break;
				case 10:
				{
					CUnit* pUnit = dynamic_cast<CUnit*>(m_pTarget);
					if (pUnit && false == pUnit->IsRangeUnit())
						bFlag = true;
				}
					break;
				case 11:
				{
					CUnit* pUnit = dynamic_cast<CUnit*>(m_pTarget);
					if (pUnit && true == pUnit->IsRangeUnit())
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
					if (m_pBattle->GetFriendlyUnitNum(m_enForce) > 0)
						bFlag = true;
				}
				case 16:
				{
					if (m_pBattle->GetRangeFriendlyUnitNum(this, 1000) < nParama)
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
					if (ReduceType & 256)
					{
						if (!di.bIsBuliding) bFlag = false;
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
					if (ReduceTypeExcept & 256)
					{
						if (di.bIsBuliding) bFlag = false;
					}

					if (false == bFlag)
						continue;

					float fReducePercent = pPassSkill->GetReducePercent();
					if (15 == pPassSkill->GetTriggerCondition())
					{
						fReducePercent = m_pBattle->GetFriendlyUnitNum(m_enForce) * fReducePercent;
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
					++get<2>(it);
				}
			}
		}
	}
}

void CUnit::_PrevAttackSkillCalculate()
{
	for (size_t i = 0; i < m_vctPasSkills.size(); ++i)
	{
		const CPassSkill *pPassSkill = std::get<0>(m_vctPasSkills[i]);

		if (!pPassSkill)
			continue;

		if (m_pTarget && m_pTarget->GetObjectType() == OT_BUILDING)
			continue;

		if (TriggerTime_PrevAtk == pPassSkill->GetTriggerTime())
		{
			if ((0 == std::get<2>(m_vctPasSkills[i]) % pPassSkill->GetTriggerInterval()) && (0 == std::get<1>(m_vctPasSkills.at(i))))
			{
				bool bFlag{ false };
				const float nParama{ pPassSkill->GetTriggerConditionParama() };
				const int nTargetHp{ m_pTarget->GetHP() };
				const int nTargetMaxHp{ m_pTarget->GetMaxHP() };
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
				case 9:if (m_pTarget->IsHero()) bFlag = true; break;
				case 10:
				{
					CUnit* pUnit = dynamic_cast<CUnit*>(m_pTarget);
					if (pUnit && false == pUnit->IsRangeUnit())
						bFlag = true;
				}
					break;
				case 11:
				{
					CUnit* pUnit = dynamic_cast<CUnit*>(m_pTarget);
					if (pUnit && true == pUnit->IsRangeUnit())
						bFlag = true;
				}
				case 12:
				{
					if (GetRandom(0, 100) <= (nParama * 100))
						bFlag = true;
				}
					break;
				case 16:
				{
					if (m_pBattle->GetRangeFriendlyUnitNum(this, 1000) < nParama)
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

					m_pUsingPassSkillIndex = i;
				}
			}
		}
	}
}

void CUnit::_CalculateBuff()
{
	CBattleObject::_CalculateBuff();

	float fEffect{ 0 };

	bool bMoveUp = HasAdvanceBuffEffect(BAE_MOVE_SPEED_UP);
	bool bMoveDown = HasAdvanceBuffEffect(BAE_MOVE_SPEED_DOWN);
	if (!bMoveDown) bMoveDown = HasAdvanceBuffEffect(BAE_FEAR);

	if (bMoveUp && bMoveDown)
		m_nMoveSpeedEx = m_fMoveSpeed * -0.25f;
	else if (bMoveUp)
		m_nMoveSpeedEx = m_fMoveSpeed * 0.5f;
	else if (bMoveDown)
		m_nMoveSpeedEx = m_fMoveSpeed * -0.5f;
	else
		m_nMoveSpeedEx = 0;

	m_nStrEx = (int)GetBaseBuffEffect(BBE_STR);
	m_nCmdEx = (int)GetBaseBuffEffect(BBE_CMD);
	m_nIntEx = (int)GetBaseBuffEffect(BBE_INT);

	float fAtkSpeed = (GetBaseBuffEffect(BBE_ATK_SPEED_DOWN) - GetBaseBuffEffect(BBE_ATK_SPEED_UP)) / 100.0f - m_fAtkSpeed;
	if (fAtkSpeed < 0)
	{
		m_nAtkIntervalEx = int(m_nAtkInterval * (fAtkSpeed / (1 + abs(fAtkSpeed))));
		m_nAtkPrevEx = int(m_nAtkPrev * (fAtkSpeed / (1 + abs(fAtkSpeed))));
	}
	else
	{
		m_nAtkIntervalEx = int(m_nAtkInterval * fAtkSpeed);
		m_nAtkPrevEx = int(m_nAtkPrev * fAtkSpeed);
	}


	//减血
	auto vctAtkDot = _FindDotEffect(BBE_ATK);
	for (auto it : vctAtkDot)
	{
		DamageInfo damage;
		damage.pSrcObject = it.first->GetBattleObject();
		CUnit* pSrc = dynamic_cast<CUnit*>(damage.pSrcObject);
		if (pSrc)
		{
			damage.nDamage = pSrc->GetAtk();
			damage.nStr = pSrc->GetStr();
			damage.nCmd = pSrc->GetCmd();
			damage.nInt = pSrc->GetInt();
		}

		damage.nAtkType = 1;
		damage.fRatio = it.second / 100.0f;
		damage.bIsSkill = true;

		BeDamaged(damage);
	}
	auto vctMatkDot = _FindDotEffect(BBE_MATK);
	for (auto it : vctMatkDot)
	{
		DamageInfo damage;
		damage.pSrcObject = it.first->GetBattleObject();
		CUnit* pSrc = dynamic_cast<CUnit*>(damage.pSrcObject);
		if (pSrc)
		{
			damage.nMDamage = pSrc->GetMatk();
			damage.nStr = pSrc->GetStr();
			damage.nCmd = pSrc->GetCmd();
			damage.nInt = pSrc->GetInt();
		}

		damage.nAtkType = 2;
		damage.fRatio = it.second / 100.0f;
		damage.bIsSkill = true;
		BeDamaged(damage);
	}
}

std::vector<std::pair<CBuff*, int>> CUnit::_FindDotEffect(BUFF_BASIC_EFFECT bbe)
{
	std::vector<std::pair<CBuff*, int>> vctPairs;

	if (BBE_ATK == bbe || BBE_MATK == bbe)
	{
		for (auto it : m_vctBuff)
		{
			int nTemp = it->GetBasicEffect(bbe);
			if (nTemp)
				vctPairs.push_back(std::make_pair(it, nTemp));
		}
		for (auto it : m_vctDebuff)
		{
			int nTemp = it->GetBasicEffect(bbe);
			if (nTemp)
				vctPairs.push_back(std::make_pair(it, nTemp));
		}
	}

	return move(vctPairs);
}

void CUnit::_AfterAttackSkillCalculate()
{
	for (size_t i = 0; i < m_vctPasSkills.size(); ++i)
	{
		const CPassSkill *pPassSkill = std::get<0>(m_vctPasSkills[i]);

		if (!pPassSkill)
			continue;

		if (m_pTarget && m_pTarget->GetObjectType() == OT_BUILDING)
			continue;

		if (TriggerTime_AfterAtk == pPassSkill->GetTriggerTime())
		{
			if ((0 == std::get<2>(m_vctPasSkills[i]) % pPassSkill->GetTriggerInterval()) && (0 == std::get<1>(m_vctPasSkills[i])))
			{
				bool bFlag{ false };
				const float nParama{ pPassSkill->GetTriggerConditionParama() };
				const int nTargetHp{ m_pTarget->GetHP() };
				const int nTargetMaxHp{ m_pTarget->GetMaxHP() };
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
				case 9:if (m_pTarget->IsHero()) bFlag = true; break;
				case 10:
				{
					CUnit* pUnit = dynamic_cast<CUnit*>(m_pTarget);
					if (pUnit && false == pUnit->IsRangeUnit())
						bFlag = true;
				}
					break;
				case 11:
				{
					CUnit* pUnit = dynamic_cast<CUnit*>(m_pTarget);
					if (pUnit && true == pUnit->IsRangeUnit())
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

					m_pUsingPassSkillIndex = i;
				}

				else if (TriggerType_SelfBuff == pPassSkill->GetTriggerType())
					SetBuff(pPassSkill->GetTriggerSkillBuffID(), nullptr);
			}
		}
	}
}

void CUnit::_GetAlwaysPassSkill()
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pSkill = std::get<0>(it);

		if (!pSkill)
			continue;

		if (TriggerTime_Always == pSkill->GetTriggerTime())
		{
			switch (pSkill->GetTriggerType())
			{
			case TriggerType_SeeHide:
				m_bSeeHide = true;
				break;
			case TriggerType_ImmuneStun:
				m_bImmuneStun = true;
				break;
			case TriggerType_ImmuneSlowDown:
				m_bImmuneSlowDown = true;
				break;
			case TriggerType_ImmunePush:
				m_bImmunePush = true;
				break;
			case TriggerType_ImmuneFear:
				m_bImmuneFear = true;
			default:
				break;
			}
		}
	}
}

void CUnit::_BaseHeal()
{
	if (m_pBattle->GetBattleMode() == BattleMode::NORMAL ||
		m_pBattle->GetBattleMode() == BattleMode::ARENA1v1 ||
		m_pBattle->GetBattleMode() == BattleMode::ARENA3v3 ||
		m_pBattle->GetBattleMode() == BattleMode::EXERCISE ||
		m_pBattle->GetBattleMode() == BattleMode::WAR ||
		m_pBattle->GetBattleMode() == BattleMode::ARENA1v1Easy ||
		m_pBattle->GetBattleMode() == BattleMode::ARENA1v1Normal ||
		m_pBattle->GetBattleMode() == BattleMode::ARENA3v3Easy ||
		m_pBattle->GetBattleMode() == BattleMode::ARENA3v3Normal)
	{
		if ((m_enForce == Force::ATK && m_fPos <= 1000) ||
			(m_enForce == Force::DEF && m_fPos >= (m_pBattle->GetWidth() - 1000)))
		{
			//int heal_hp = int(0.03f * m_nMaxHp);
			int heal_hp = 500;
			if (heal_hp > (m_nMaxHp - m_nHP))
				heal_hp = (m_nMaxHp - m_nHP);
			Heal(heal_hp);
		}
	}
}

float CUnit::GetAtkDistance()
{
	if (HasAdvanceBuffEffect(BUFF_ADVANCE_EFFECT::BAE_BLIND))
		return -10;
	return m_fAtkDistance;
}
