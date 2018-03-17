#pragma once
#include "BuffTP.h"

class CBuff
{
public:
	CBuff(const CBuffTP* pBuffTP, CBattle* pBattle);
	~CBuff() = default;

	void	SetSrcObject(CBattleObject* pObject){ m_pObject = pObject; }
	bool	IsDebuff() const { return m_pBuffTP->m_bIsDebuff; }
	bool	HasAdvanceEffect(BUFF_ADVANCE_EFFECT enEffect) const;
	int		GetBasicEffect(BUFF_BASIC_EFFECT enEffect) const;
	int		GetID() const { return m_nID; }
	int		GetBuffID() const { return m_pBuffTP->m_nID; }
	int		GetTime() const { return m_nTimes; }
	void	ChangeTime(int nNum){ m_nTimes += nNum; }
	void	ResetTime(){ m_nTimes = m_pBuffTP->m_nTime; }
	CBattleObject* GetBattleObject() { return m_pObject; }

private:
	const CBuffTP*	m_pBuffTP{ nullptr };

	int				m_nID{ 0 };					//唯一ID
	int				m_nTimes{ 0 };				//剩余时间

	CBattle*		m_pBattle{ nullptr };		//战场
	CBattleObject*	m_pObject{ nullptr };		//释放者指针
};
