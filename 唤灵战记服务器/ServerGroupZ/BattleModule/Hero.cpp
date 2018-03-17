#include "stdafx.h"
#include "Hero.h"
#include "Master.h"
#include "Battle.h"
#include "Bullet.h"
#include "Building.h"

CHero::CHero(CBattle *pBattle) :
CUnit{ pBattle }
{

}

CHero::~CHero()
{

}

const CHero* CHero::CreateCustomHero(const Hero& hero)
{
	CHero* pHero = new CHero{ nullptr };

	for (int i = 0; i < hero.passkills_size(); i++)
	{
		const CPassSkill* pPassSkill = CPassSkill::GetPassSkill(hero.passkills(i));
		if (pPassSkill)
			pHero->m_vctPasSkills.push_back(std::make_tuple(pPassSkill, 0, 0));
	}
	for (int i = 0; i < hero.actkills_size(); i++)
	{
		const CSkill* pActSkill = CSkill::GetSkill(hero.actkills(i));
		if (pActSkill)
		{
			pHero->m_arrSkills[i].first = pActSkill;
			pHero->m_arrSkills[i].second = 0;
		}
	}

	pHero->m_nHP = pHero->m_nMaxHp = hero.hp();
	pHero->m_enForce = (Force)hero.belong();
	pHero->level_ = hero.level();
	if (Force::ATK == pHero->m_enForce)
		pHero->m_bDirection = true;
	else if (Force::DEF == pHero->m_enForce)
		pHero->m_bDirection = false;

	pHero->not_attack_ = hero.not_attack();
	pHero->m_nAtk = hero.atk();
	pHero->m_nDef = hero.def();
	pHero->m_nMatk = hero.matk();
	pHero->m_nMdef = hero.mdef();
	pHero->m_fPos = pHero->m_fPatrolPoint = (float)hero.pos();
	pHero->m_fMoveSpeed = (float)hero.movespeed();
	pHero->m_bIsRange = hero.israngeunit();
	pHero->m_fAtkDistance = (float)hero.atkdistance();
	pHero->m_nAtkPrev = hero.atkpretime();
	pHero->m_nAtkInterval = hero.atkintervaltime();
	pHero->m_enNormalAtk = (TARGET_TYPE)hero.atkmode_normal();
	pHero->m_enHideAtk = (TARGET_TYPE)hero.atkmode_hiding();
	pHero->m_enSpecialAtk = (TARGET_TYPE)hero.atkmode_holder();
	pHero->m_nResourceID = hero.modelresourceid();
	pHero->m_bCanHide = hero.ishideunit();
	pHero->m_nAtkBuildingDamage = hero.buildingatk();
	pHero->m_nStr = hero.str();
	pHero->m_nInt = hero.intelligence();
	pHero->m_nCmd = hero.cmd();
	pHero->m_nGroupID = hero.group();
	pHero->m_nQuality = hero.quality();
	pHero->m_strName = hero.name();
	pHero->m_enAIType = (AIType)hero.aitype();
	pHero->m_bIsBackup = hero.isbackup();
	pHero->m_nMasterID = hero.master();
	pHero->m_fActiveRange = (float)hero.active_range();
	pHero->m_enAtkType = (ATK_TYPE)hero.atk_type();
	pHero->m_nNormalAtkBulletID = hero.bullet_id();
	pHero->m_fVolume = (float)hero.volume();
	pHero->m_pHeroTP = CHeroTP::GetHeroTP(hero.id());
	pHero->m_bIsBoss = hero.is_boss();
	pHero->_GetAlwaysPassSkill();
	pHero->m_nMaxTime = 0;
	return pHero;
}

void CHero::InitHero(const HeroCard* pHeroCard, CMaster *pMaster)
{
	m_pHeroTP = pHeroCard->m_pHeroTP;
	m_pMaster = pMaster;
	if (!m_pHeroTP)
		return;

	m_enHeroType = m_pHeroTP->m_enHeroType;
	level_ = pHeroCard->level_;

	level_ = pHeroCard->level_;
	m_nHP = m_nMaxHp = pHeroCard->hp_;
	m_nAtk = pHeroCard->atk_;
	m_nDef = pHeroCard->def_;
	m_nMatk = pHeroCard->matk_;
	m_nMdef = pHeroCard->mdef_;
	m_fMoveSpeed = static_cast<float>(pHeroCard->move_speed_);
	m_fCritOdds = pHeroCard->crit_odds_;
	m_fPreventCrit = pHeroCard->crit_resistance_;
	m_fCritValue = pHeroCard->crit_value_;
	m_nAtkInterval = pHeroCard->atk_interval_;
	m_nAtkPrev = pHeroCard->prev_atk_;
	m_nAtkBuildingDamage = pHeroCard->building_atk_;
	m_nStr = pHeroCard->strength_;
	m_nCmd = pHeroCard->command_;
	m_nInt = pHeroCard->intelligence_;
	m_nMaxTime = pHeroCard->exsits_time_;

	m_nResourceID = m_pHeroTP->m_nResourceID;
	m_fVolume = m_pHeroTP->m_fVolume;

	m_bIsRange = m_pHeroTP->m_bIsRange;
	m_fAtkDistance = m_pHeroTP->m_fAtkDistance;
	m_enAtkType = m_pHeroTP->m_enAtkType;
	m_enNormalAtk = m_pHeroTP->m_enNormalAtk;
	m_enHideAtk = m_pHeroTP->m_enHideAtk;
	m_enSpecialAtk = m_pHeroTP->m_enSpecialAtk;


	m_nQuality = m_pHeroTP->m_nQuality;

	m_enForce = pMaster->GetForce();
	suit_effect_ = pHeroCard->suit_effect_;

	for (size_t i = 0; i < m_pHeroTP->m_vctSkills.size(); i++)
	{
		const CSkill* pSkill = CSkill::GetSkill(m_pHeroTP->m_vctSkills[i]);
		if (pSkill && pHeroCard->level_ >= CHeroTP::GetActiveSkillOpenLevel(i))
		{
			m_arrSkills[i].first = pSkill;
			m_arrSkills[i].second = 0;
		}

	}
	if (!m_pMaster->IsAI())
	{
		const CSkill* pSkill = CSkill::GetSkill(911);
		if (pSkill)
		{
			m_arrSkills[3].first = pSkill;
			m_arrSkills[3].second = 0;
		}
	}

	int nIndex{ 0 };
	for (auto &it : m_pHeroTP->m_vctPassSkills)
	{
		const CPassSkill* pPassSkill = CPassSkill::GetPassSkill(it);
		if (pPassSkill && pHeroCard->level_ >= CHeroTP::GetPassSkillOpenLevel(nIndex))
		{
			m_vctPasSkills.push_back(std::make_tuple(pPassSkill, 0, 0));
		}
		nIndex++;
	}

	if (Force::ATK == m_enForce)
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

void CHero::MoveTo(float fPos)
{
	if (_IsSpasticity())
	{
		CancelHeroMove();
		return;
	}

	if (US_SKILL == m_enState)
		_StopUseSkill();

	if (HasAdvanceBuffEffect(BAE_FEAR) || HasAdvanceBuffEffect(BAE_MOVE_HOLD))
	{
		CancelHeroMove();
		return;
	}

	m_fMoveToPos = fPos;

	if (m_fMoveToPos < m_fPos)
		m_bDirection = false;
	else
		m_bDirection = true;

	if (US_ATK == m_enState)
		_StopAttack();

	_StartMove();
}

void CHero::Move(bool bDirection)
{
	if (_IsSpasticity())
	{
		//发送校验，通知客户端马上停下
		CancelHeroMove();
		return;
	}

	if (US_SKILL == m_enState)
		_StopUseSkill();

	if (HasAdvanceBuffEffect(BAE_FEAR) || HasAdvanceBuffEffect(BAE_MOVE_HOLD))
	{
		//发送校验通知，通知客户端马上停下
		CancelHeroMove();
		return;
	}

	m_bDirection = bDirection;

	if (US_ATK == m_enState)
		_StopAttack();

	_StartMove();
}

void CHero::UseSkill(int nIndex, float nStandPos, float fTargetPos)
{
	if (_IsSpasticity())
		return;

	if (HasAdvanceBuffEffect(BAE_FEAR))
		return;

	if (nIndex < 0 || nIndex >= (int)m_arrSkills.size())
		return;

	if (m_pUsingSkillIndex >= 0)
		return;

	if (!m_arrSkills[nIndex].first)
		return;

	if (m_arrSkills[nIndex].second > 0)
		return;

	//校验站立位置(100码内以客户端当前位置为准，把角色硬拉过去)
	if (abs(m_fPos - nStandPos) < 100)
	{
		//但是超过50的话要记录一次，频繁超过50就不对了
		m_fPos = nStandPos;
	}
	//发送校验位置的信息，把客户端拉回来
	else
	{
		_StopMove(false);
		return;
	}

	if (m_arrSkills[nIndex].first->GetCastRange() > 0 && abs(m_fPos - fTargetPos) > m_arrSkills[nIndex].first->GetCastRange())
		return;

	if (US_ATK == m_enState)
		_StopAttack();
	if (US_MOVING == m_enState)
		_StopMove(false);

	m_pUsingSkillIndex = nIndex;
	if (m_arrSkills[nIndex].first->GetCastRange() == 0)	fTargetPos = m_fPos;	//Update By Lux 2014.11.25

	m_fUsingSkillPos = fTargetPos;

	_StartUseSkill();
}

void CHero::Stand()
{
	if (_IsSpasticity())
		return;

	if (HasAdvanceBuffEffect(BAE_FEAR))
		return;

	if (US_MOVING == m_enState)
		_StopMove(true);
}

void CHero::Disappear()
{
	//额外发送英雄消失协议
	pto_BATTLE_S2C_NTF_OnHeroDisappear pto;
	pto.set_nheroid(m_nID);

	std::string strPto;
	pto.SerializeToString(&strPto);
	m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_OnHeroDisappear);

	if (!m_pMaster->IsAI())
		m_pBattle->SetHeroDisappear(true);

	if (!IsBoss())
	{
		for (size_t i = 1; i <= 6; i++)
		{
			CMaster* pMaster = m_pBattle->FindMasterByMID(i);
			if (nullptr != pMaster)
			{
				if (nullptr != pMaster->GetUsingHero())
				{
					pMaster->GetUsingHero()->_HeroDeadSkillCalculate(HDT_Disappear, m_enForce, this);
				}
			}
		}
	}

	_Die(nullptr);
}

CHero*	CHero::Clone(CBattle* pBattle) const
{
	CHero* pHero = new CHero{ *this };

	pHero->m_pBattle = pBattle;
	pHero->m_pMaster = pBattle->FindMasterByMID(m_nMasterID);

	if (nullptr != m_pMaster)
		pHero->m_pMaster->SetUsingHero(pHero);

	return pHero;
}

void CHero::SetUnitProtocol(pto_BATTLE_Struct_Unit* ptoUnit)
{
	CUnit::SetUnitProtocol(ptoUnit);

	for (auto &it : m_arrSkills)
	{
		if (it.first)
		{
			ptoUnit->add_vctactskillid(it.first->GetID());
			ptoUnit->add_vctactskillcd(it.first->GetCD());

		}
		else
		{
			ptoUnit->add_vctactskillid(0);
			ptoUnit->add_vctactskillcd(0);
		}
	}
}

void CHero::StopMoveWhenPause()
{
	m_fMoveToPos = -1;

	if (HasAdvanceBuffEffect(BAE_FEAR))
		return;

	if (US_MOVING == m_enState)
		m_enState = US_STAND;
}

void CHero::BeDamaged(const DamageInfo& di)
{
	m_nOutCombat = OUT_COMBAT_TIME;
	CUnit::BeDamaged(di);
}

void CHero::Fear()
{
	if (HasAdvanceBuffEffect(BAE_INVINCIBLE))
		return;

	if (US_SKILL == m_enState)
		_BreakUseSkill();

	m_fMoveToPos = -1;

	CUnit::Fear();
}

void CHero::Stun()
{
	if (HasAdvanceBuffEffect(BAE_INVINCIBLE))
		return;

	if (US_SKILL == m_enState)
	{
		_BreakUseSkill();
	}

	CUnit::Stun();
}

bool CHero::Fallback(float fPos)
{
	if (CUnit::Fallback(fPos))
	{
		if (US_SKILL == m_enState)
		{
			_BreakUseSkill();
		}
	}
	else
		return false;
	return true;
}

void CHero::Remove()
{
	m_enState = US_DEAD;

	if (m_pMaster)
		m_pMaster->SetUsingHero(nullptr);
}

void CHero::AttackingSkillCalculate(CUnit* pTarget, float& fDamage, float& fMDamage, float& fCritOdds, float& fCritValue)
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pPassSkill = std::get<0>(it);

		if (!pPassSkill)
			continue;

		bool  bFlag{ false };
		const float nParama{ pPassSkill->GetTriggerConditionParama() };
		const int nSrcHp{ pTarget->GetHP() };
		const int nSrcMaxHp{ pTarget->GetMaxHP() };
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
			if (pTarget->HasAdvanceBuffEffect(BUFF_ADVANCE_EFFECT::BAE_STUN) || pTarget->HasAdvanceBuffEffect(BUFF_ADVANCE_EFFECT::BAE_MOVE_HOLD))
				bFlag = true;
		}
			break;
		case 15:
		{
			if (m_pBattle->GetFriendlyUnitNum(m_enForce))
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

		for (int i = 0; i < pPassSkill->GetBaseSize(); i++)
		{
			float fAtkValue = pPassSkill->GetBaseValue(i) / 100;
			if (15 == pPassSkill->GetTriggerCondition())
			{
				fAtkValue = m_pBattle->GetFriendlyUnitNum(m_enForce) * fAtkValue;
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

void CHero::Loop()
{
	if (m_bIsAI)
		__AIUseSkill();

	_CalculateBuff();

	_CalculateState();
	_CalculateSkillCD();

	if (m_pBattle->TimeCounts(1000))
		_BaseHeal();

	if (_IsSpasticity())
		return;

	_Act();
}

void CHero::_Act()
{
	if (m_bIsAI)
	{
		if (US_SKILL == m_enState)
		{
			if (_HeroUsingSkill())
				_StopUseSkill();
		}
		else
			CUnit::_Act();

		return;
	}

	if (HasAdvanceBuffEffect(BAE_FEAR))
	{
		_MovingLoop();
		return;
	}

	if (US_MOVING == m_enState)
	{
		_MovingLoop();
	}
	else if (US_SKILL == m_enState)
	{
		if (_HeroUsingSkill())
			_StopUseSkill();
	}
	else
	{
		if (US_ATK != m_enState)
			m_pTarget = m_pBattle->FindEnemy(this);

		if (m_pTarget)
			_AttackLoop();
	}
}

void CHero::_MovingLoop()
{
	if (m_nAtkIntervalCounts > 0 && true == IsBoss())
		return;

	CUnit::_MovingLoop();

	if (m_pBattle->HitTestEnemyHero(this) || m_pBattle->HitTestEnemyBuilding(this))
	{
		if (Force::ATK == m_enForce && true == m_bDirection)
		{
			m_fPos -= GetMoveSpeed();
			if (m_fMoveToPos > 0)
				_StopMove(true);
		}
		else if (Force::DEF == m_enForce && false == m_bDirection)
		{
			m_fPos += GetMoveSpeed();
			if (m_fMoveToPos > 0)
				_StopMove(true);
		}

		_AdjustPos();
	}

	if (m_fMoveToPos > 0)
	{
		if (m_bDirection)
		{
			if (m_fPos >= m_fMoveToPos)
				_StopMove(true);
		}
		else
		{
			if (m_fPos <= m_fMoveToPos)
				_StopMove(true);
		}
	}
}

void CHero::_Die(CBattleObject* pSrc)
{
	CUnit::_Die(nullptr);

	pto_BATTLE_S2C_NTF_OnHeroDead pto;
	pto.set_nheroid(m_nID);

	int nMasterID{ 0 };
	int nKillerID{ 0 };

	if (pSrc)
	{
		nKillerID = pSrc->GetID();
		CMaster *pSrcMaster{ pSrc->GetMaster() };
		if (pSrcMaster)
		{
			nMasterID = pSrcMaster->GetMasterID();
			pSrcMaster->ChangeKillHeroes(1);
		}

		if (!IsBoss())
		{
			for (size_t i = 1; i <= 6; i++)
			{
				CMaster* pMaster = m_pBattle->FindMasterByMID(i);
				if (nullptr != pMaster)
				{
					if (nullptr != pMaster->GetUsingHero())
					{
						pMaster->GetUsingHero()->_HeroDeadSkillCalculate(HDT_Dead, m_enForce, this);
					}
				}
			}
		}
	}

	if (m_pMaster)
		m_pMaster->SetUsingHero(nullptr);

	pto.set_nmasterid(nMasterID);
	pto.set_nkillerid(nKillerID);

	string strPto;
	pto.SerializeToString(&strPto);
	m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_OnHeroDead);

	if (pSrc)
	{
		if (m_enForce == Force::ATK)
			m_pBattle->AddResource(Force::DEF, KILL_HERO_AWARD);
		else if (m_enForce == Force::DEF)
			m_pBattle->AddResource(Force::ATK, KILL_HERO_AWARD);

		if (!m_pMaster->IsAI())
			m_pBattle->SetHeroDead(true);
	}
}

void CHero::_CalculateSkillCD()
{
	CUnit::_CalculateSkillCD();

	for (auto &it : m_arrSkills)
	{
		if (it.second > 0)
		{
			it.second -= BATTLE_LOOP_TIME;
			if (it.second < 0)
				it.second = 0;
		}
	}
}

void CHero::_StartUseSkill()
{
	if (m_pUsingSkillIndex >= 0)
	{
		const CSkill* pSkill{ m_arrSkills[m_pUsingSkillIndex].first };
		if (!pSkill)
			return;

		m_nSpasticityCountSkill = pSkill->GetSkillPrevTime();

		if (US_MOVING == m_enState)
			_StopMove(false);
		if (US_ATK == m_enState)
			_StopAttack();

		m_enState = US_SKILL;

		m_pBattle->GetProtoPackage(Force::ATK)->StartUseSkill(m_nID, m_pUsingSkillIndex, ST_ACT_SKILL, m_fPos);
		m_pBattle->GetProtoPackage(Force::DEF)->StartUseSkill(m_nID, m_pUsingSkillIndex, ST_ACT_SKILL, m_fPos);
	}

	if (m_pUsingPassSkillIndex >= 0 )
	{
		const CSkill* pSkill{ m_arrSkills[m_pUsingPassSkillIndex].first };
		if (!pSkill)
			return;

		m_pBattle->GetProtoPackage(Force::ATK)->StartUseSkill(m_nID, m_pUsingPassSkillIndex, ST_PASS_SKILL, m_fPos);
		m_pBattle->GetProtoPackage(Force::DEF)->StartUseSkill(m_nID, m_pUsingPassSkillIndex, ST_PASS_SKILL, m_fPos);
	}
}

bool CHero::_HeroUsingSkill()
{
	if (HasAdvanceBuffEffect(BAE_SILENCE))
		return true;

	if (m_pUsingSkillIndex < 0)
		return true;

	const CSkill* pSkill{ m_arrSkills[m_pUsingSkillIndex].first };
	if (!pSkill)
		return true;

	if (false == m_bFirstRelease && pSkill->GetCastInterval() > 0)
	{
		m_nReleaseIntervalCounts = pSkill->GetCastInterval();
		m_bFirstRelease = true;
		return false;
	}

	if (m_nReleaseIntervalCounts > 0)
	{
		m_nReleaseIntervalCounts -= BATTLE_LOOP_TIME;

		if (m_nReleaseIntervalCounts > 0)
			return false;
		else
			m_nReleaseIntervalCounts = 0;
	}

	if (pSkill->GetHealthEffect())
		Heal(int(m_nMaxHp* pSkill->GetHealthEffect()));

	for (int i = 0; i < pSkill->GetSunnonedNum(); ++i)
	{
		const CSoldierTP* pSoldierTP{ CSoldierTP::GetSoldierTP(pSkill->GetSummonedID()) };

		if (!pSoldierTP)
			continue;

		CUnit *pUnit = new CUnit{ m_pBattle };
		pUnit->InitUnit(pSoldierTP, m_pMaster, true);
		pUnit->SetPosIndex(0);
		pUnit->SetPos(m_fPos + GetRandom(1, 100));

		if (m_pBattle->AddObject(pUnit))
			pUnit->InitBrithBuff(pSoldierTP);
		else
			delete pUnit;
	}


	for (size_t i = 0; i < pSkill->GetBuffSize(); i++)
		SetBuff(pSkill->GetBuffID(i), this);

	if (pSkill->GetBulletSize() > 0)
	{
		/*int nIndex{ m_nReleaseTimes };

		if (pSkill->IsRepeatSameBullet())
		nIndex = 0;*/

		for (size_t i = 0; i < pSkill->GetBulletSize(); i++)
		{
			/*if (nIndex >= (int)pSkill->GetBulletSize())
				return true;*/

			const CBulletTP* pBulletTP = CBulletTP::GetBulletTP(pSkill->GetBulletID(i));
			if (!pBulletTP)
				return true;

			if (pSkill->GetMoveEffect() == 1)
			{
				float fBulidingPos{ 0 };
				float fGatePos{ 0 };
				if (Force::ATK == m_enForce)
				{
					fBulidingPos = m_pBattle->FindBuidlingLinePos(Force::DEF);
					m_fUsingSkillPos = (fBulidingPos < m_fUsingSkillPos ? fBulidingPos : m_fUsingSkillPos) - m_fVolume;

					fGatePos = m_pBattle->FindGateLinePos(Force::ATK);
					m_fUsingSkillPos = (fGatePos < m_fUsingSkillPos ? fGatePos : m_fUsingSkillPos);
				}
				else if (Force::DEF == m_enForce)
				{
					fBulidingPos = m_pBattle->FindBuidlingLinePos(Force::ATK);
					m_fUsingSkillPos = (fBulidingPos > m_fUsingSkillPos ? fBulidingPos : m_fUsingSkillPos) + m_fVolume;

					fGatePos = m_pBattle->FindGateLinePos(Force::DEF);
					m_fUsingSkillPos = (fGatePos > m_fUsingSkillPos ? fGatePos : m_fUsingSkillPos);
				}

				SetPos(m_fUsingSkillPos);
			}

			CBullet *pBullet = new CBullet(m_pBattle);

			pBullet->InitBullet(pBulletTP, this, m_fUsingSkillPos);
			pBullet->SetSkill(pSkill);
			m_pBattle->AddObject(pBullet);
		}
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
	else if (enSSE == SSE_ADD_ELITE_NUM)
		m_pMaster->AddEliteNum();
	else if (enSSE == SSE_ADD_COMMON_NUM)
		m_pMaster->AddCommonNum();
	else if (enSSE == SSE_RESET_HERO)
		m_pMaster->ResetHero();
	else if (enSSE == SSE_KILL_SELF)
	{
		_Die(this);
		
	}
	else if (enSSE == SSE_HERO_DISAPPEAR)
	{
		m_pMaster->SetHeroDurationTime(m_nMaxTime);
	}
	else if (enSSE == SSE_REPAIR_BASE)
	{
		if (m_enForce == Force::ATK && m_pBattle->GetAtkCastle())
			m_pBattle->GetAtkCastle()->Repair(1000);
		else if (m_pBattle->GetDefCastle())
			m_pBattle->GetDefCastle()->Repair(1000);
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

	if (m_bIsHiding)
		_BreakHide();

	m_nReleaseTimes++;
	m_nReleaseIntervalCounts = pSkill->GetCastInterval();

	if (m_nReleaseTimes >= pSkill->GetCastTime())
	{
		m_arrSkills[m_pUsingSkillIndex].second = pSkill->GetCD();
		m_nSpasticityCountSkill = pSkill->GetSpasticTime();

		m_pBattle->GetProtoPackage(Force::ATK)->UseSkillComplete(m_nID, m_pUsingSkillIndex);
		m_pBattle->GetProtoPackage(Force::DEF)->UseSkillComplete(m_nID, m_pUsingSkillIndex);

		return true;
	}

	return false;
}

void CHero::_StopUseSkill()
{
	if (m_pUsingSkillIndex == -1) return;

	const CSkill* pSkill{ m_arrSkills[m_pUsingSkillIndex].first };
	if (pSkill == nullptr)
		return;

	m_arrSkills[m_pUsingSkillIndex].second = pSkill->GetCD();

	m_pBattle->GetProtoPackage(Force::ATK)->UseSkillComplete(m_nID, m_pUsingSkillIndex);
	m_pBattle->GetProtoPackage(Force::DEF)->UseSkillComplete(m_nID, m_pUsingSkillIndex);

	m_nReleaseIntervalCounts = 0;
	m_bFirstRelease = false;
	m_nReleaseTimes = 0;
	m_pUsingSkillIndex = -1;
	m_fUsingSkillPos = -1;
	m_pUsingPassSkillIndex = -1;
	m_enState = US_STAND;
}

void CHero::_StopMove(bool send)
{
	m_fMoveToPos = -1;
	CUnit::_StopMove(send);
}

void CHero::_CalculateBuff()
{
	CUnit::_CalculateBuff();

	m_fCritOddsEx = GetBaseBuffEffect(BBE_CRIT) / 100;
}

void CHero::_GetAlwaysPassSkill()
{
	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pSkill = get<0>(it);

		if (!pSkill)
			continue;

		if (TriggerTime_Always == pSkill->GetTriggerTime())
		{
			switch (pSkill->GetTriggerType())
			{
			case TriggerType_Unrestriction:
				m_bUnrestriction = true;
				break;
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
				break;
			default:
				break;
			}


			for (int i = 0; i < pSkill->GetBaseSize(); i++)
			{
				switch (pSkill->GetBaseType(i))
				{
					//case BaseType_MaxTime:
					//	m_nMaxTime += int(pSkill->GetBaseValue(i));
					//	break;
				case BaseType_AutoHeal:
					m_fAutoHeal = pSkill->GetBaseValue(i) / 100;
					break;
				default:
					break;
				}
			}
		}
	}
}

void CHero::_HeroDeadSkillCalculate(HeroDeadType enDeadType, Force enForce, CHero* pHero)
{
	dat_SKILL_ENUM_TriggerTime enTT{ TriggerTime_Null };

	switch (enDeadType)
	{
	case HDT_Dead:
		enTT = TriggerTime_HeroDead;
		break;
	case HDT_Disappear:
		enTT = TriggerTime_HeroDeadDisappear;
		break;
	case HDT_DeadAndDisappear:
		enTT = TriggerTime_HeroDeadDisappear;
		break;
	default:
		break;
	}

	for (auto &it : m_vctPasSkills)
	{
		const CPassSkill *pSkill = std::get<0>(it);

		if (!pSkill)
			continue;
		if (enTT == pSkill->GetTriggerTime() ||
			pSkill->GetTriggerTime() == TriggerTime_HeroDeadDisappear)
		{
			bool  bFlag{ false };
			if (13 == pSkill->GetTriggerCondition())
			{
				switch ((int)pSkill->GetTriggerConditionParama())
				{
				case 4:
					bFlag = true;
					break;
				case 1:
					if (pHero == this)
						bFlag = true;
					break;
				case 2:
					if (enForce == m_enForce)
						bFlag = true;
					break;
				case 3:
					if (enForce != m_enForce)
						bFlag = true;
					break;
				default:
					break;
				}
			}
			if (!bFlag)
				continue;
			for (int i = 0; i < pSkill->GetBaseSize(); i++)
			{
				switch (pSkill->GetBaseType(i))
				{
				case BaseType_MaxTime:
					m_pMaster->ChangeHeroDurationTime(int(pSkill->GetBaseValue(i)));
					break;
				case BaseType_AddResource:
					m_pMaster->ChangeResource(int(pSkill->GetBaseValue(i)));
					break;
				default:
					break;
				}
			}

		}
	}
}

void CHero::_BreakUseSkill()
{
	if (m_pUsingSkillIndex == -1) return;

	const CSkill* pSkill{ m_arrSkills[m_pUsingSkillIndex].first };
	if (pSkill == nullptr)
		return;

	if (m_arrSkills[m_pUsingSkillIndex].first->IsNeedStandChannel() ||
		m_arrSkills[m_pUsingSkillIndex].first->GetChantTime() >= 0)
	{
		m_arrSkills[m_pUsingSkillIndex].second = m_arrSkills[m_pUsingSkillIndex].first->GetCD();
		m_pBattle->GetProtoPackage(Force::ATK)->UseSkillComplete(m_nID, m_pUsingSkillIndex);
		m_pBattle->GetProtoPackage(Force::DEF)->UseSkillComplete(m_nID, m_pUsingSkillIndex);
	}

	m_nReleaseIntervalCounts = 0;
	m_bFirstRelease = false;
	m_nReleaseTimes = 0;
	m_pUsingSkillIndex = -1;
	m_fUsingSkillPos = -1;
	m_enState = US_STAND;
}

void CHero::__AIUseSkill()
{
	for (size_t i = 0; i < GetSkills()->size(); i++)
	{
		if (GetSkills()->at(i).second > 0)
			continue;

		float fUseSkillPos = GetPos();

		if (m_pMaster->UseSkill(GetSkills()->at(i).first, this, fUseSkillPos) &&
			m_enState != US_SKILL)
		{
			UseSkill(i, GetPos(), fUseSkillPos);
			return;
		}
	}
	SetBeDamaged(false);
}

void CHero::SetSuitSkill(int skill_id)
{
	int index = m_vctPasSkills.size();
	const CPassSkill* pPassSkill = CPassSkill::GetPassSkill(skill_id);
	if (pPassSkill)
	{
		m_vctPasSkills.push_back(std::make_tuple(pPassSkill, 0, 0));
	}
}

void CHero::CancelHeroMove()
{
	if (m_pMaster && !m_pMaster->IsAI())
	{
		pto_BATTLE_S2C_NTF_CancelHeroMove pto_ntf;
		pto_ntf.set_nx(m_fPos);

		std::string str_pto;
		pto_ntf.SerializeToString(&str_pto);
		m_pBattle->SendToMaster(m_pMaster, str_pto, MSG_S2C, BATTLE_S2C_NTF_CancelHeroMove);
	}
}