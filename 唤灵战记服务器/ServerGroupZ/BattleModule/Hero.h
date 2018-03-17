#pragma once
#include "Unit.h"

class CHero : public CUnit
{
public:
	static const int KILL_HERO_AWARD{ 150 };
	static const int OUT_COMBAT_TIME{ 3000 };	//����ս��ʱ��

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
	bool				m_bIsAI{ true };				//�Ƿ���AI
	bool				m_bIsBoss{ false };				//�Ƿ���Boss
	HERO_TYPE			m_enHeroType{ HT_ATK };			//Ӣ������
	const CHeroTP*		m_pHeroTP{ nullptr };			//Ӣ��ģ��
	int					m_nQuality{ 0 };				//Ʒ��
	int					m_nMaxTime{ 0 };				//�����ʱ��
	float				m_fMoveToPos{ -1 };				//Ӣ���ƶ���
	float				m_fCritOdds{ 0.2f };			//��������
	float				m_fCritOddsEx{ 0 };				//���ⱬ����
	float				m_fCritValue{ 1.5f };			//����ֵ

	int					m_pUsingSkillIndex{ -1 };		//����ʹ�õļ���
	float				m_fUsingSkillPos{ -1 };			//����ʹ�ü���λ��
	int					m_nReleaseIntervalCounts{ 0 };	//�ͷ��ӵ��ļ��
	int					m_nReleaseTimes{ 0 };			//�ͷ��ӵ�����
	bool				m_bFirstRelease{ false };		//�Ƿ����ͷ��ӵ��ĵ�һ��
	bool				m_bUnrestriction{ false };		//�����������

	float				m_fAutoHeal{ 0.07f };			//5���Զ���Ѫ
	int					m_nOutCombat{ 0 };				//����ս��ʱ��
	int					suit_effect_{ 0 };				//��װ��ЧID


	std::array<std::pair<const CSkill*, int>, 4> m_arrSkills;		//��������(����,CD)

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
