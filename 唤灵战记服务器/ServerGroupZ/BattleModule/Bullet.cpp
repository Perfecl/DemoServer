#include "stdafx.h"
#include "Bullet.h"
#include "Battle.h"
#include "Hero.h"
#include "Master.h"

CBullet::CBullet(CBattle*	pBattle) :
CBattleObject{ pBattle }
{

}

CBullet::~CBullet()
{
	for (auto &it : m_mapBulletSingleInfos)
		delete it.second;
}

void CBullet::Loop()
{
	if (m_bIsDead)
		return;

	CBattleObject::Loop();

	m_nLifeTime += BATTLE_LOOP_TIME;

	for (auto &it : m_mapBulletSingleInfos)
		if (it.second->nInterval > 0)
		{
		it.second->nInterval -= BATTLE_LOOP_TIME;
		if (it.second->nInterval < 0)
			it.second->nInterval = 0;
		}

	if (m_pBulletTP->m_bHasFlyPath)
	{
		if (0 == m_cMoveState)
		{
			m_nMoveLeadTime += BATTLE_LOOP_TIME;

			if (m_nMoveLeadTime >= m_pBulletTP->m_nMoveLeadTime)
			{
				m_cMoveState = 1;
				m_pBattle->GetProtoPackage(Force::ATK)->BulletMove(m_nID, m_pBulletTP->m_enForm, true);
				m_pBattle->GetProtoPackage(Force::DEF)->BulletMove(m_nID, m_pBulletTP->m_enForm, true);
			}
		}
		else if (1 == m_cMoveState)
		{
			__OnMove();
		}
	}

	if (__CanSettlement())
	{
		if (m_pBulletTP->m_bIsArea)
			__MultiSettlement();
		else
			__SingleSettlement();
	}

	//生存周期结算
	if (__IsGoingToDie())
		Die();
}

void CBullet::InitBullet(const CBulletTP* pBulletTP, CBattleObject* pOwner, float fPos)
{
	if (!pBulletTP)
		return;

	m_pBulletTP = pBulletTP;
	m_pOwner = pOwner;
	if (pOwner->GetForce() == Force::ATK)
		m_bDirection = true;
	else
		m_bDirection = false;
	if (pBulletTP->m_bIsOpposite)
		m_bDirection = !m_bDirection;
	m_enForce = pOwner->GetForce();
	m_fVolume = pBulletTP->m_fVolumn;
	m_fMoveSpeed = pBulletTP->m_fMoveSpeed;
	m_pMaster = pOwner->GetMaster();

	if (pBulletTP->m_bHasTargetObj && false == pBulletTP->m_bHasFlyPath)
	{
		CUnit *pUnit = dynamic_cast<CUnit*>(pOwner);
		if (pUnit)
		{
			CBattleObject* pObj = pUnit->GetTarget();
			if (pObj)
				fPos = pObj->GetPos();
		}
	}

	if (pOwner->GetForce() == Force::ATK)
		m_fPos = m_fStartPos = fPos + pBulletTP->m_fPosOffset;
	else
		m_fPos = m_fStartPos = fPos - pBulletTP->m_fPosOffset;

	if (pBulletTP->m_bHasTargetObj)
	{
		CUnit* pUnit = dynamic_cast<CUnit*>(pOwner);
		if (pUnit)
			m_pTarget = pUnit->GetTarget();
	}
}

void CBullet::InitBullet(const CBulletTP* pBulletTP, Force enForce, float fPos)
{
	m_pBulletTP = pBulletTP;
	m_pOwner = nullptr;
	if (enForce == Force::ATK ||
		enForce == Force::UNKNOWN)
		m_bDirection = true;
	else
		m_bDirection = false;
	if (pBulletTP->m_bIsOpposite)
		m_bDirection = !m_bDirection;
	m_enForce = enForce;
	m_fVolume = pBulletTP->m_fVolumn;
	m_fMoveSpeed = pBulletTP->m_fMoveSpeed;
	m_pMaster = nullptr;

	if (m_bDirection)
		m_fPos = m_fStartPos = fPos + pBulletTP->m_fPosOffset;
	else
		m_fPos = m_fStartPos = fPos - pBulletTP->m_fPosOffset;
}

void CBullet::SetBulletProtocol(pto_BATTLE_Struct_Bullet* ptoBullet)
{
	ptoBullet->set_nid(m_nID);
	ptoBullet->set_nmodelid(m_pBulletTP->GetBulletID());
	ptoBullet->set_bisdead(m_bIsDead);
	if (m_pOwner)
		ptoBullet->set_ncasterid(m_pOwner->GetID());
	ptoBullet->set_bdirection(m_bDirection);

	if (m_pTarget)
		ptoBullet->set_nsingletargetid(m_pTarget->GetID());

	ptoBullet->set_neffectvalue(GetEffectValue());
	ptoBullet->set_nstartposx(m_fStartPos);
	ptoBullet->set_nx(m_fPos);

	//位移技能 (随子弹移动 0没有 1跳跃 2冲锋 3闪现)
	if (m_pSkill)
	{
		if (m_pSkill->GetMoveEffect() == 1)
			ptoBullet->set_m_bhasjumpeffect(true);
		else
			ptoBullet->set_m_bhasjumpeffect(false);

		if (m_pSkill->GetMoveEffect() == 2)
			ptoBullet->set_m_bhaschargeeffect(true);
		else
			ptoBullet->set_m_bhaschargeeffect(false);

		if (m_pSkill->GetMoveEffect() == 3)
			ptoBullet->set_m_bhasblinkeffect(true);
		else
			ptoBullet->set_m_bhasblinkeffect(false);
	}
}

void CBullet::RemainStageLoot(CMaster* master)
{
	if (9994 == m_pBulletTP->m_nEffect)
	{
		if (master)
		{
			const CStageLoot* loop = CStageLoot::GetStageLootByBulletID(m_pBulletTP->m_nBulletID);
			if (loop)
				master->GetStageLoot(m_pBulletTP->m_nEffectValue, loop->num());
		}
	}
}

void CBullet::Die()
{
	m_bIsDead = true;

	m_pBattle->ClearWarpGate(this);

	if (m_pBulletTP->m_nNewBulletID)
	{
		const CBulletTP* pBulletTP = CBulletTP::GetBulletTP(m_pBulletTP->m_nNewBulletID);

		if (pBulletTP)
		{
			CBullet *pBullet = new CBullet(m_pBattle);

			pBullet->InitBullet(pBulletTP, m_pOwner, m_fPos);
			pBullet->SetSkill(m_pSkill);
			m_pBattle->AddObject(pBullet);
		}
	}

	for (int i = 0; i < m_pBulletTP->m_nDead_SummonedNum; ++i)
	{
		const CSoldierTP* pSoldierTP{ CSoldierTP::GetSoldierTP(m_pBulletTP->m_nDead_SummonedID) };

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

	m_pBattle->GetProtoPackage(Force::ATK)->BulletDead(m_nID, m_pBulletTP->m_enForm, m_nDeadByWho);
	m_pBattle->GetProtoPackage(Force::DEF)->BulletDead(m_nID, m_pBulletTP->m_enForm, m_nDeadByWho);
}

void CBullet::__OnMove()
{
	float fMoveSpeed{ m_pBulletTP->m_fMoveSpeed };

	if (false == m_bDirection)
		fMoveSpeed *= -1;

	m_fPos += fMoveSpeed;

	if (m_pSkill)
	{
		if (m_pSkill->GetMoveEffect() == 2)
		{
			if (m_pBattle->HitTestEnemyBuilding(this) || m_pBattle->HitTestWallBullet(this) || m_pBattle->HitTestGate(this))
			{
				__MoveComplete();
				return;
			}
			m_pOwner->SetPos(m_fPos);
		}
	}

	if (abs(m_fPos - m_fStartPos) >= m_pBulletTP->m_fMaxMoveDistance)
	{
		fMoveSpeed = m_pBulletTP->m_fMaxMoveDistance;
		if (false == m_bDirection)
			fMoveSpeed *= -1;
		m_fPos = m_fStartPos + fMoveSpeed;
		__MoveComplete();
	}

	if (m_fPos > m_pBattle->GetWidth())
	{
		m_fPos = m_pBattle->GetWidth();
		__MoveComplete();
	}
	else if (m_fPos < 0)
	{
		m_fPos = 0;
		__MoveComplete();
	}
}

void CBullet::__MoveComplete()
{
	m_cMoveState = 2;

	m_pBattle->GetProtoPackage(Force::ATK)->BulletMoveComplete(m_nID, m_pBulletTP->m_enForm, true);
	m_pBattle->GetProtoPackage(Force::DEF)->BulletMoveComplete(m_nID, m_pBulletTP->m_enForm, true);
}

bool CBullet::__IsGoingToDie()
{
	if (m_pBulletTP->m_cDeadManner & 1)
	{
		if (m_pBulletTP->m_nLifeTime != -1 && m_nLifeTime >= m_pBulletTP->m_nLifeTime && !m_bInfinite)
			return true;
	}

	if (m_pBulletTP->m_cDeadManner & 2)
	{
		if (m_nSettlementTimes >= m_pBulletTP->m_nSettlementTimes)
			return true;
	}

	if (m_pBulletTP->m_cDeadManner & 4)
	{
		if (2 == m_cMoveState)
			return true;
	}

	if (m_pBulletTP->m_cDeadManner & 8)
	{
		if (m_nHitTimes >= m_pBulletTP->m_nHitTimes)
			return true;
	}

	return false;
}

bool CBullet::__CanSettlement()
{
	if (m_nLifeTime < m_pBulletTP->m_nSettlementLeadTime)
		return false;

	if (m_nSettlementIntervalTime > 0)
	{
		m_nSettlementIntervalTime -= BATTLE_LOOP_TIME;

		if (m_nSettlementIntervalTime < 0)
			m_nSettlementIntervalTime = 0;

		return false;
	}

	return true;
}

void CBullet::__MultiSettlement()
{
	std::vector<std::map<int, CUnit*>*>		m_vctTargetList;
	std::vector<std::map<int, CBuilding*>*>	m_vctTargetBuildingList;

	if (1 == m_pBulletTP->m_nValidForce)
	{
		Force enForce{ Force::ATK };
		if (Force::ATK == m_enForce)
			enForce = Force::DEF;

		m_vctTargetList.push_back(m_pBattle->GetUnitList(enForce));

		if (m_pBulletTP->m_cVaildUnit & 8)
			m_vctTargetBuildingList.push_back(m_pBattle->GetBulidingList(enForce));
	}
	else if (2 == m_pBulletTP->m_nValidForce)
	{
		m_vctTargetList.push_back(m_pBattle->GetUnitList(m_enForce));
		if (m_pBulletTP->m_cVaildUnit & 8)
			m_vctTargetBuildingList.push_back(m_pBattle->GetBulidingList(m_enForce));
	}
	else if (3 == m_pBulletTP->m_nValidForce)
	{
		m_vctTargetList.push_back(m_pBattle->GetUnitList(Force::ATK));
		if (m_pBulletTP->m_cVaildUnit & 8)
			m_vctTargetBuildingList.push_back(m_pBattle->GetBulidingList(Force::ATK));

		m_vctTargetList.push_back(m_pBattle->GetUnitList(Force::DEF));
		if (m_pBulletTP->m_cVaildUnit & 8)
			m_vctTargetBuildingList.push_back(m_pBattle->GetBulidingList(Force::DEF));
	}
	else if (4 == m_pBulletTP->m_nValidForce)
	{
		m_vctTargetList.push_back(m_pBattle->GetUnitList(Force::ATK));
		if (m_pBulletTP->m_cVaildUnit & 8)
			m_vctTargetBuildingList.push_back(m_pBattle->GetBulidingList(Force::ATK));
	}

	else if (5 == m_pBulletTP->m_nValidForce)
	{
		m_vctTargetList.push_back(m_pBattle->GetPlayerUnitList());
		if (m_pBulletTP->m_cVaildUnit & 8)
			m_vctTargetBuildingList.push_back(m_pBattle->GetBulidingList(Force::ATK));
	}

	else if (6 == m_pBulletTP->m_nValidForce)
	{
		m_vctTargetList.push_back(m_pBattle->GetPlayerHeroUnitList());
		if (m_pBulletTP->m_cVaildUnit & 8)
			m_vctTargetBuildingList.push_back(m_pBattle->GetBulidingList(Force::ATK));
	}

	if (m_pBulletTP->m_nEffect == 9999)
	{
		//英雄先判断吃水晶
		std::deque<CUnit*> vct;

		for (auto &itList : m_vctTargetList)
		{
			for (auto &itUnit : *itList)
			{
				if (itUnit.second->IsHero())
					vct.push_front(itUnit.second);
				else
					vct.push_back(itUnit.second);
			}
		}

		for (auto &it : vct)
			__Settlement(it);
	}
	else
	{
		for (auto &itList : m_vctTargetList)
			for (auto &itUnit : *itList)
				__Settlement(itUnit.second);

		for (auto &itList : m_vctTargetBuildingList)
			for (auto &itBuild : *itList)
				__Settlement(itBuild.second);
	}

	m_nSettlementTimes++;

	m_nSettlementIntervalTime = m_pBulletTP->m_nSettlementIntervalTime;
}

void CBullet::__SingleSettlement()
{
	__Settlement(dynamic_cast<CUnit*>(m_pTarget));
}

void CBullet::__Settlement(CUnit* pUnit)
{
	if (false == __CheckSingleInfoValid(pUnit))
		return;

	if (m_pBulletTP->m_nHitTimes > 0 && m_nHitTimes >= m_pBulletTP->m_nHitTimes)
		return;

	if (pUnit->IsHiding() && m_pBulletTP->m_nValidForce == 1)
		return;

	if (!pUnit->IsRangeUnit())
		if (!(m_pBulletTP->m_cVaildRange & 1))
			return;

	if (pUnit->IsRangeUnit())
		if (!(m_pBulletTP->m_cVaildRange & 2))
			return;

	if (pUnit->IsSoldier())
		if (!(m_pBulletTP->m_cVaildUnit & 1))
			return;

	if (pUnit->IsHero() && !pUnit->IsBoss())
		if (!(m_pBulletTP->m_cVaildUnit & 2))
			return;

	if (pUnit->IsBoss())
		if (!(m_pBulletTP->m_cVaildUnit & 4))
			return;

	if (pUnit == m_pOwner)
		if (m_pBulletTP->invalid_target_ & 1)
			return;

	if (!pUnit->HitTest(this))
		return;

	if (9999 == m_pBulletTP->m_nEffect)
	{
		m_nDeadByWho = pUnit->GetID();

		m_pBattle->AddResource(pUnit->GetForce(), GetEffectValue(), true, true);

		pUnit->GetMaster()->ChangeGetCrystal(1);
	}
	else if (9994 == m_pBulletTP->m_nEffect)
	{
		CMaster* master = pUnit->GetMaster();
		if (master)
		{
			const CStageLoot* loop = CStageLoot::GetStageLootByBulletID(m_pBulletTP->m_nBulletID);
			if (loop)
				master->GetStageLoot(m_pBulletTP->m_nEffectValue, loop->num());
		}
	}
	else if (9998 == m_pBulletTP->m_nEffect)
	{
		if (pUnit->IsHero())
			pUnit->Heal(static_cast<int>(0.15f * pUnit->GetMaxHP()));
	}
	else if (9995 == m_pBulletTP->m_nEffect)
	{
		CMaster* master = pUnit->GetMaster();
		if (master)
			master->ChangeHeroDurationTime(5000);
	}
	else if (10001 == m_pBulletTP->m_nEffect)
	{
		pUnit->Heal(int(pUnit->GetMaxHP() * 0.5f));
	}
	else if (1000 == m_pBulletTP->m_nEffect && master_)
	{
		DamageInfo di;

		if (m_enForce == Force::ATK)
			di.pSrcObject = (CBattleObject*)m_pBattle->GetAtkCastle();

		if (m_enForce == Force::DEF)
			di.pSrcObject = (CBattleObject*)m_pBattle->GetDefCastle();

		di.bIsArea = m_pBulletTP->m_bIsArea;
		di.bIsRangeAtk = m_pBulletTP->m_bIsRange;
		di.bIsSkill = false;
		di.nAtkType = ATK_TYPE::AT_BaseGun;
		di.nStr = pUnit->GetStr();
		di.nCmd = pUnit->GetCmd();
		di.nInt = pUnit->GetInt();
		di.nDamage = master_->GetCastleGunAtk();
		di.nMDamage = master_->GetCastleGunMAtk();
		di.master_ = master_;
		di.bIsBuliding = true;
		pUnit->BeDamaged(di);
	}
	else
	{
		DamageInfo di;

		di.bIsArea = m_pBulletTP->m_bIsArea;
		di.bIsRangeAtk = m_pBulletTP->m_bIsRange;
		di.bIsSkill = true;
		if (nullptr != m_pOwner)
		{
			di.pSrcObject = m_pOwner;
			di.bIsHeroAtk = m_pOwner->IsHero();
		}
		di.nAtkType = m_pBulletTP->m_nType;

		di.nDamage = m_pBulletTP->m_nBasePhysicsDamage;
		di.nMDamage = m_pBulletTP->m_nBaseMagicDamage;

		CUnit* pSrcUnit = dynamic_cast<CUnit*>(m_pOwner);
		if (pSrcUnit)
		{
			di.nDamage += pSrcUnit->GetAtk();
			di.nMDamage += pSrcUnit->GetMatk();
			di.nStr = pSrcUnit->GetStr();
			di.nInt = pSrcUnit->GetInt();
			di.nCmd = pSrcUnit->GetCmd();
		}

		di.fPhysicsRatio = m_pBulletTP->m_fPercentPhysicsDamage;
		di.fMagicRatio = m_pBulletTP->m_fPercentMagicDamage;

		if (pUnit->IsHero())
			di.fRatio *= m_pBulletTP->m_fPercentDamageHero;
		else
			di.fRatio *= m_pBulletTP->m_fPercentDamageSoldier;

		//逐步率判断
		float fProceed = 1.0f + m_nHitTimes * m_pBulletTP->m_fProcced;
		if (fProceed > m_pBulletTP->m_fMaxProceed)
			fProceed = m_pBulletTP->m_fMaxProceed;
		if (fProceed < m_pBulletTP->m_fMinProceed)
			fProceed = m_pBulletTP->m_fMinProceed;

		di.fRatio *= fProceed;

		if (di.fPhysicsRatio + di.fMagicRatio > 0)
			pUnit->BeDamaged(di);
	}

	for (auto &it : m_pBulletTP->m_vctBuffEffect)
		pUnit->SetBuff(it, m_pOwner);

	if (m_pBulletTP->m_fFallbackDistance > 0)
	{
		if (m_bDirection)
			pUnit->Fallback(pUnit->GetPos() + m_pBulletTP->m_fFallbackDistance);
		else
			pUnit->Fallback(pUnit->GetPos() - m_pBulletTP->m_fFallbackDistance);
	}
	else if (m_pBulletTP->m_fFallbackDistance == -1)
	{
		if (m_bDirection)
			pUnit->Fallback(m_fStartPos + m_pBulletTP->m_fVolumn + m_pBulletTP->m_fMaxMoveDistance);
		else
			pUnit->Fallback(m_fStartPos - m_pBulletTP->m_fVolumn - m_pBulletTP->m_fMaxMoveDistance);
	}


	m_nHitTimes++;

	BulletSingleInfo* info = __GetBulletSingleInfo(pUnit);
	info->nInterval = m_pBulletTP->m_nAttackSingleIntervalTime;
	info->nTimes++;
}

void CBullet::__Settlement(CBuilding* pBuilding)
{

}

BulletSingleInfo* CBullet::__GetBulletSingleInfo(CBattleObject* pObj)
{
	auto it = m_mapBulletSingleInfos.find(pObj);
	if (m_mapBulletSingleInfos.cend() == it)
	{
		BulletSingleInfo* pInfo = new BulletSingleInfo;
		pInfo->pObject = pObj;
		m_mapBulletSingleInfos.insert(std::make_pair(pInfo->pObject, pInfo));
		return pInfo;
	}
	else
	{
		return it->second;
	}
}

bool CBullet::__CheckSingleInfoValid(CBattleObject *pUnit)
{
	if (!pUnit)
		return false;

	BulletSingleInfo* pInfo = __GetBulletSingleInfo(pUnit);

	if (m_pBulletTP->m_nAttackSingleTimes > 0 && pInfo->nTimes >= m_pBulletTP->m_nAttackSingleTimes)
		return false;

	if (pInfo->nInterval > 0)
		return false;

	return true;
}

bool CBullet::IsChargeBullet()
{
	if (!m_pSkill)
		return false;

	if (m_pSkill->GetMoveEffect() == 2)
		return true;

	return false;
}