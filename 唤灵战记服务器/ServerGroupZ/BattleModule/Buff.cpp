#include "stdafx.h"
#include "Buff.h"
#include "Battle.h"

CBuff::CBuff(const CBuffTP* pBuffTP, CBattle* pBattle) :
m_pBuffTP{ pBuffTP },
m_pBattle{ pBattle }
{
	if (m_pBuffTP)
		m_nTimes = m_pBuffTP->m_nTime;

	if (m_pBattle)
		m_nID = m_pBattle->GetBuffID();
}

bool CBuff::HasAdvanceEffect(BUFF_ADVANCE_EFFECT enEffect) const
{
	if (enEffect < 1000)
		return false;

	for (auto &it : m_pBuffTP->m_vctAdvanceEffect)
	if (it == enEffect)
		return true;

	return false;
}

int	CBuff::GetBasicEffect(BUFF_BASIC_EFFECT enEffect) const
{
	if (enEffect >= 1000)
		return 0;

	for (auto &it : m_pBuffTP->m_vctEffects)
	{
		if (it.first == enEffect)
		{
			if (m_pBuffTP->m_nInterval)
			{
				if (m_nTimes % m_pBuffTP->m_nInterval)
					return 0;
				else
					return it.second;
			}
			else
			{
				return it.second;
			}
		}
	}

	return 0;
}