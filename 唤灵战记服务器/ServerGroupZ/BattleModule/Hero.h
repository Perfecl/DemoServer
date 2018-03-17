#pragma once
#include "Unit.h"

class CHero : public CUnit
{
public:
	static const int KILL_HERO_AWARD{ 150 };
	static const int OUT_COMBAT_TIME{ 3000 };	//脱离战斗时间

	static const CHero* CreateCustomHero(const Hero& soldier);

public:
	CHero(CBattle *pBattle);
	virtual ~CHero();

	virtual ObjectType	GetObjectType() const override{ return OT_UNIT; }

	void				InitHero(const HeroCard* pHeroCard, CMaster *pMaster);

	int					GetQuality() const { return m_nQuality; }
	int					GetMaxTime() const { return m_nMaxTime; }
	HERO_TYPE			GetHeroType() const { return m_enHeroType; }
	float				GetCritOdds() const { return m_fCritOdds + m_fCritOddsEx; }
	float				GetCritValue() const { return m_fCritValue; }
	size_t				GetSkillSize() const { return m_arrSkills.size(); }
	const CSkill*		GetSkill(int nIndex) const{ return m_arrSkills[nIndex].first; }
	int                 GetState() const { return  m_enState; }
	int                 GetUsintSkill()const{ return m_pUsingSkillIndex; }
	int					IsOutCombat()const{ return m_nOutCombat; }
	void				SetOutCombat(int nTime){ m_nOutCombat = nTime; }
	std::array<std::pair<const CSkill*, int>, 4>* GetSkills(){ return &m_arrSkills; }

	virtual void		SetUnitProtocol(pto_BATTLE_Struct_Unit* ptoUnit) override;

	virtual bool		IsHero() const override{ return true; }
	virtual bool		IsBoss() const override{ return m_bIsBoss; }

	void				SetIsAI(bool bIsAI){ m_bIsAI = bIsAI; }

	void				MoveTo(float fPos);
	void				Move(bool bDirection);
	void				UseSkill(int nIndex, float nStandPos, float fTargetPos);
	void				Stand();
	void				Disappear();

	void				StopMoveWhenPause();

	virtual bool		Fallback(float fPos) override;

	CHero*				Clone(CBattle* pBattle) const;
	const CHeroTP*      GetHeroTP() const { return m_pHeroTP; }
	bool				IsUnrestriction() const { return m_bUnrestriction; }

	float				GetAutoHeal() const { return m_fAutoHeal; }

	virtual void        Loop() override;
	void				CancelHeroMove();

public:
	bool				m_bIsAI{ true };				//是否是AI
	bool				m_bIsBoss{ false };				//是否是Boss
	HERO_TYPE			m_enHeroType{ HT_ATK };			//英雄类型
	const CHeroTP*		m_pHeroTP{ nullptr };			//英雄模版
	int					m_nQuality{ 0 };				//品阶
	int					m_nMaxTime{ 0 };				//最长持续时间
	float				m_fMoveToPos{ -1 };				//英雄移动点
	float				m_fCritOdds{ 0.2f };			//爆击几率
	float				m_fCritOddsEx{ 0 };				//额外爆击率
	float				m_fCritValue{ 1.5f };			//爆击值

	int					m_pUsingSkillIndex{ -1 };		//正在使用的技能
	float				m_fUsingSkillPos{ -1 };			//正在使用技能位置
	int					m_nReleaseIntervalCounts{ 0 };	//释放子弹的间隔
	int					m_nReleaseTimes{ 0 };			//释放子弹次数
	bool				m_bFirstRelease{ false };		//是否是释放子弹的第一波
	bool				m_bUnrestriction{ false };		//无视类型相克

	float				m_fAutoHeal{ 0.07f };			//5秒自动回血
	int					m_nOutCombat{ 0 };				//脱离战斗时间
	int					suit_effect_{ 0 };				//套装特效ID


	std::array<std::pair<const CSkill*, int>, 4> m_arrSkills;		//主动技能(技能,CD)

	void				SetSuitSkill(int skill_id);
	virtual void		BeDamaged(const DamageInfo& di);
	virtual void		Fear();
	virtual void		Stun();
	virtual void		Remove();
	void				AttackingSkillCalculate(CUnit* pTarget, float& fDamage, float& fMDamage, float& fCritOdds, float& fCritValue);

private:
	void			__AIUseSkill();
	virtual void	_Act() override;
	virtual void	_MovingLoop() override;


	bool			_HeroUsingSkill();
	virtual void	_StartUseSkill() override;
	virtual void	_StopUseSkill() override;
	void			_BreakUseSkill();

	virtual void	_StopMove(bool send) override;

	virtual void	_Die(CBattleObject* pSrc) override;
	virtual void	_CalculateSkillCD() override;

	virtual void	_CalculateBuff() override;
	virtual void	_GetAlwaysPassSkill() override;
	void			_HeroDeadSkillCalculate(HeroDeadType enDeadType, Force enForce, CHero* pHero);
};
