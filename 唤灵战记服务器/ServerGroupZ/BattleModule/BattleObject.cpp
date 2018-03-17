#include "stdafx.h"
#include "BattleObject.h"
#include "Battle.h"
#include "BuffTP.h"
#include "Unit.h"
#include "Hero.h"

CBattleObject::CBattleObject(CBattle* pBattle) :
m_pBattle{ pBattle }
{

}

CBattleObject::~CBattleObject()
{
	for (auto &it : m_vctDebuff)
		delete it;
	for (auto &it : m_vctBuff)
		delete it;
}

bool CBattleObject::HitTest(CBattleObject* pObj)
{
	float x1 = pObj->m_fPos - pObj->m_fVolume;
	float x2 = pObj->m_fPos + pObj->m_fVolume;

	float y1 = m_fPos - m_fVolume;
	float y2 = m_fPos + m_fVolume;

	if (0 == m_pBattle->IntersectionLine(x1, x2, y1, y2))
		return false;

	return true;
}

void CBattleObject::SetBuff(int nBuffID, CBattleObject* pSrcObj)
{
	const CBuffTP* pBuffTP = CBuffTP::GetBuffTP(nBuffID);
	bool bCheckHas = false;

	if (!pBuffTP)
	{
		int(FormatString("未知的BuffID", nBuffID).c_str());
		return;
	}

	CBuff* pBuff = new CBuff{ pBuffTP, m_pBattle };
	pBuff->SetSrcObject(pSrcObj);

	if (pBuff->IsDebuff())
	{
		if (HasAdvanceBuffEffect(BAE_INVINCIBLE))
			return;

		CUnit* pUnit = dynamic_cast<CUnit*>(this);
		if (pUnit)
		{
			pUnit->BuffSkillCalculate(pBuff);
			if (pBuff->HasAdvanceEffect(BAE_STUN) &&
				pUnit->IsImmuneStun())
				return;
			if (pBuff->HasAdvanceEffect(BAE_MOVE_SPEED_DOWN) &&
				pUnit->IsImmuneSlowDown())
				return;
			if (pBuff->HasAdvanceEffect(BAE_MOVE_HOLD) &&
				pUnit->IsImmuneSlowDown())
				return;
			if (pBuff->HasAdvanceEffect(BAE_FEAR) &&
				pUnit->IsImmuneFear())
				return;
		}
		m_vctDebuff.push_back(pBuff);

	}
	else
	{
		for (auto &itBuffs : m_vctBuff)
		{
			if (itBuffs->GetBuffID() == pBuff->GetBuffID())
			{
				//如果有一样的，刷新一下时间
				itBuffs->ResetTime();
				bCheckHas = true;
			}
		}
		if (bCheckHas == false)
		{
			m_vctBuff.push_back(pBuff);
		}
	}

	if (pBuff->HasAdvanceEffect(BAE_STUN) || pBuff->HasAdvanceEffect(BAE_FEAR))
	{
		CUnit* pUnit = dynamic_cast<CUnit*>(this);
		if (pUnit)
		{
			if (pUnit->IsHero())
				pBuff->ChangeTime(-pBuff->GetTime() / 2);
			pUnit->Stun();
		}
	}

	if (pBuff->HasAdvanceEffect(BAE_FEAR))
	{
		CUnit* pUnit = dynamic_cast<CUnit*>(this);
		if (pUnit)
			pUnit->Fear();
	}

	if (pBuff->HasAdvanceEffect(BAE_INVINCIBLE))
		ClearDebuff();

	if (OT_UNIT == GetObjectType() && bCheckHas == false)
	{
		m_pBattle->GetProtoPackage(Force::ATK)->SetBuff(pBuff->GetID(), m_nID, pBuff->GetBuffID());
		m_pBattle->GetProtoPackage(Force::DEF)->SetBuff(pBuff->GetID(), m_nID, pBuff->GetBuffID());
	}
}

bool CBattleObject::HasAdvanceBuffEffect(BUFF_ADVANCE_EFFECT enEffect)
{
	for (auto &it : m_vctBuff)
		if (it->HasAdvanceEffect(enEffect))
			return true;

	for (auto &it : m_vctDebuff)
		if (it->HasAdvanceEffect(enEffect))
			return true;

	return false;
}

float CBattleObject::GetBaseBuffEffect(BUFF_BASIC_EFFECT enEffect)
{
	std::vector<const std::vector<CBuff*>*> vctList;
	vctList.push_back(&m_vctBuff);
	vctList.push_back(&m_vctDebuff);

	if (BBE_DEF == enEffect || BBE_MDEF == enEffect)
	{
		float fValue{ 1 };
		for (auto &itList : vctList)
		{
			for (auto &it : *itList)
			{
				float fReduce = (100 - it->GetBasicEffect(enEffect)) / 100.0f;
				if (fReduce < 0)
					fReduce = 0;
				fValue *= fReduce;
			}
		}
		return fValue;
	}
	else if (BBE_CRIT == enEffect || BBE_STR == enEffect || BBE_CMD == enEffect || BBE_INT == enEffect)
	{
		float fValue{ 0 };
		for (auto &itList : vctList)
			for (auto &it : *itList)
				fValue += it->GetBasicEffect(enEffect);

		return fValue;
	}
	else if (BBE_ATK_SPEED_UP == enEffect || BBE_ATK_SPEED_DOWN == enEffect)
	{
		float fValue{ 0 };
		for (auto &itList : vctList)
		{
			for (auto &it : *itList)
			{
				float fTemp = (float)it->GetBasicEffect(enEffect);
				if (fTemp > fValue)
					fValue = fTemp;
			}
		}
		return fValue;
	}
	else if (enEffect == BBE_EX_ATK)
	{
		float fValue{ 0 };
		for (auto &itList : vctList)
		{
			for (auto &it : *itList)
			{
				float fTemp = (float)it->GetBasicEffect(enEffect);
				if (fTemp > fValue)
					fValue = fTemp;
			}
		}
		return fValue / 100;
	}
	else if (enEffect == BBE_EX_MATK)
	{
		float fValue{ 0 };
		for (auto &itList : vctList)
		{
			for (auto &it : *itList)
			{
				float fTemp = (float)it->GetBasicEffect(enEffect);
				if (fTemp > fValue)
					fValue = fTemp;
			}
		}
		return fValue / 100;
	}
	else if (enEffect == BEE_NERF_DEF)
	{
		float fValue{ 0 };
		for (auto &itList : vctList)
		{
			for (auto &it : *itList)
			{
				float fTemp = (float)it->GetBasicEffect(enEffect);
				if (fTemp > fValue)
					fValue = fTemp;
			}
		}
		return fValue;
	}

	return 0;
}

void CBattleObject::ClearDebuff()
{
	bool bHasFear = false;
	for (auto &it : m_vctDebuff)
	{
		if (OT_UNIT == GetObjectType())
		{
			m_pBattle->GetProtoPackage(Force::ATK)->EndBuff(it->GetID(), m_nID);
			m_pBattle->GetProtoPackage(Force::DEF)->EndBuff(it->GetID(), m_nID);
		}
	

		if (it->HasAdvanceEffect(BAE_FEAR))
			bHasFear = true;


		delete it;
	}

	m_vctDebuff.clear();

	if (bHasFear)
	{
		CUnit* pUnit = dynamic_cast<CUnit*>(this);
		if (pUnit)
			pUnit->StopFear();
	}
}

void CBattleObject::_CalculateBuff()
{
	std::vector<std::vector<CBuff*>*> vctList;
	vctList.push_back(&m_vctBuff);
	vctList.push_back(&m_vctDebuff);

	for (auto &itBuffs : vctList)
	{
		for (auto it = itBuffs->cbegin(); it != itBuffs->cend();)
		{
			if ((*it)->GetTime() <= 0)
			{
				if (OT_UNIT == GetObjectType())
				{
					m_pBattle->GetProtoPackage(Force::ATK)->EndBuff((*it)->GetID(), m_nID);
					m_pBattle->GetProtoPackage(Force::DEF)->EndBuff((*it)->GetID(), m_nID);
				}

				bool bHasFear = false;
				if ((*it)->HasAdvanceEffect(BAE_FEAR))
					bHasFear = true;

				delete *it;

				it = itBuffs->erase(it);

				if (bHasFear)
				{
					CUnit* pUnit = dynamic_cast<CUnit*>(this);
					if (pUnit && !pUnit->HasAdvanceBuffEffect(BAE_FEAR))
						pUnit->StopFear();
				}
			}
			else
			{
				(*it)->ChangeTime(-BATTLE_LOOP_TIME);
				++it;
			}
		}
	}
}
