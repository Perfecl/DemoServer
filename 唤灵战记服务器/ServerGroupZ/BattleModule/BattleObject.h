#pragma once
#include "Buff.h"

class CBattleObject
{
public:
	CBattleObject(CBattle*	pBattle);
	virtual ~CBattleObject();

	virtual void Loop(){}
	virtual void BeDamaged(const DamageInfo& di){};

	virtual float		GetMoveSpeed() const { return m_fMoveSpeed; }
	virtual ObjectType	GetObjectType() const { return OT_NULL; }

	CMaster*		GetMaster() const { return m_pMaster; }
	int				GetID() const { return m_nID; }
	Force			GetForce() const { return m_enForce; }
	bool			GetDirection() const { return m_bDirection; }
	float			GetPos() const { return m_fPos; }
	float			GetVolume() const { return m_fVolume; }
	int				GetHP() const { return m_nHP; }
	int				GetMaxHP() const { return m_nMaxHp; }
	int             GetGroupID() const{ return m_nGroupID; }

	virtual bool	IsHero() const { return false; }
	virtual bool	IsBoss() const { return false; }

	void			SetForce(Force enForce){ m_enForce = enForce; }
	void			SetPos(float fPos){ m_fPos = fPos; }
	void			SetID(int nID){ m_nID = nID; }
	void			SetMaster(CMaster* pMaster){ m_pMaster = pMaster; }

	bool			HitTest(CBattleObject* pObj);

	void			SetBuff(int nBuffID, CBattleObject* pSrcObj);

	bool			HasAdvanceBuffEffect(BUFF_ADVANCE_EFFECT enEffect);
	float			GetBaseBuffEffect(BUFF_BASIC_EFFECT enEffect);
	void			ClearDebuff();

protected:
	int				m_nHP{ 0 };								//当前HP
	int				m_nMaxHp{ 0 };							//最大HP
	CMaster*		m_pMaster{ nullptr };					//召唤师指针
	CBattle*		m_pBattle;
	int				m_nID{ 0 };
	int				m_nResourceID{ 0 };
	Force			m_enForce{ Force::UNKNOWN };
	float			m_fPos{ 0 };
	float			m_fVolume{ 0 };
	float			m_fMoveSpeed{ 0 };
	bool			m_bDirection{ false };
	int				m_nGroupID{ 0 };

	std::vector<CBuff*>	m_vctBuff;
	std::vector<CBuff*>	m_vctDebuff;

	virtual void	_CalculateBuff();
};
