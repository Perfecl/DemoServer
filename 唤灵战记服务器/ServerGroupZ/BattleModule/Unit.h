#pragma once
#include "BattleObject.h" 

class CUnit : public CBattleObject
{
public:
	static const CUnit* CreateCustomUnit(const Soldier& soldier,const CMaster* master);

public:
	CUnit(CBattle* pBattle);
	virtual ~CUnit();

	virtual void		Loop() override;

	virtual void		BeDamaged(const DamageInfo& di) override;
	virtual ObjectType	GetObjectType() const override{ return OT_UNIT; }

	void				InitUnit(const CSoldierTP *pSoldierTP, CMaster *pMaster, bool isSummonUnit = false);
	void				InitBrithBuff(const CSoldierTP *pSoldierTP);
	void				SendUnitToClient(Force enForce = Force::UNKNOWN);
	void				SendCustomUnitToClient(Force enForce = Force::UNKNOWN);
	void				SetDirection(bool bDirection){ m_bDirection = bDirection; }
	void				SetPosIndex(int nIndex){ m_nPosIndex = nIndex; }
	virtual void		SetUnitProtocol(pto_BATTLE_Struct_Unit* ptoUnit);	//设置的单位协议

	bool				GetIsSummonUnit() const { return m_bIsSummonUnit; }

	int					GetAtk() const { return m_nAtk + m_nAtkEx; }
	int					GetDef() const { return m_nDef + m_nDefEx; }
	int					GetMatk()const { return m_nMatk + m_nMatkEx; }
	int					GetMdef()const { return m_nMdef + m_nMdefEx; }
	int					GetStr() const { return m_nStr + m_nStrEx; }
	int					GetCmd() const { return m_nCmd + m_nCmdEx; }
	int					GetInt() const { return m_nInt + m_nIntEx; }
	int					GetAtkPrevTime() const { return m_nAtkPrev + m_nAtkPrevEx; }
	int					GetAtkIntervalTime() const { return m_nAtkInterval + m_nAtkIntervalEx; }
	int					GetBaseAtkPrevTime() const { return m_nAtkPrev; }
	int					GetBaseAktIntervalTime() const { return m_nAtkInterval; }
	virtual float		GetMoveSpeed() const override{ return m_fMoveSpeed + m_nMoveSpeedEx; }
	float				GetPreventCrit() const { return m_fPreventCrit; }

	TARGET_TYPE			GetNormalAtkMode() const { return m_enNormalAtk; }		//普通状态时攻击目标选择
	TARGET_TYPE			GetHideAtkMode() const { return m_enHideAtk; }			//隐身状态时攻击目标选择
	TARGET_TYPE			GetSpecialAtkMode() const { return m_enSpecialAtk; }	//特殊状态时攻击目标选择
	float				GetAtkDistance();
	float				GetActiveRange() const { return m_fActiveRange; }
	CBattleObject*		GetTarget() const { return m_pTarget; }

	bool				CanHide() const { return m_bCanHide; }
	bool				IsHiding() const { return m_bIsHiding; }
	bool				IsSpecialMode() const { return m_bIsSpecialMode; }
	bool				IsRangeUnit() const { return m_bIsRange; }
	bool				IsSoldier() const;
	bool				IsDead() const { if (US_DEAD == m_enState) return true; return false; }
	virtual bool		IsBoss() const { return false; }
	bool				IsBackup() const { return m_bIsBackup; }
	bool				IsBreakHero() const { return m_bIsBreakHero; }
	virtual bool		IsUnrestriction() const { return false; }
	bool				IsImmuneStun() const { return m_bImmuneStun; }
	bool				IsImmuneSlowDown() const { return m_bImmuneSlowDown; }				//免疫减速禁锢
	bool				IsImmunePush() const{ return m_bImmunePush; }						//免疫击退拉扯
	bool				IsImmuneFear() const{ return m_bImmuneFear; }

	virtual bool		Fallback(float fPos);					//击退

	void				Heal(int nNum);							//回复
	void				Retreat();								//撤退
	void				StopRetreat();							//停止撤退
	void				Fear();									//恐惧
	void				StopFear();								//停止恐惧

	virtual void		Stun();									//眩晕

	AIType              GetAIType(){ return m_enAIType; }
	void				ChangeAIType(AIType enAIType){ _StopMove(true); m_enAIType = enAIType; }
	void                SetAIType(AIType enAIType){ m_enAIType = enAIType; }
	void                SetActiveRange(float fUnitChangeAIRange){ m_fActiveRange = fUnitChangeAIRange; }
	void				SetPatrolPoint(float fPos){ m_fPatrolPoint = fPos; }
	void				SetState(UnitState enState){ m_enState = enState; }

	CUnit*				Clone(CBattle* pBattle) const;

	bool                GetBeDamaged(){ return m_bBeDamaged; }
	void                SetBeDamaged(bool bBeDamaged){ m_bBeDamaged = bBeDamaged; }

	bool				IsSeeHide(){ return m_bSeeHide; }

	virtual	void		Remove();
	void				SuckHPSkillCalculate(int nFinalDamage);
	void				BramblesDmgSkillCalculate(int nFinalDamage, CUnit* pTarget);
	void				BuffSkillCalculate(CBuff* pBuff);
	void				BeBramblesDmg(int nDamage, CBattleObject* pSrc);

	bool				double_attack(){ return double_attack_; }
	void				set_double_attack(bool double_attack){ double_attack_ = double_attack; }

protected:
	std::string			m_strName;								//名字
	int					level_{ 0 };							//等级

	bool				m_bIsSummonUnit;						//是不是技能召唤出来的

	int					m_nAtk{ 0 };							//基础攻击
	int					m_nDef{ 0 };							//基础防御
	int					m_nMatk{ 0 };							//基础魔攻
	int					m_nMdef{ 0 };							//基础魔防

	bool				m_bIsRange{ false };					//是否是远程单位
	bool				m_bCanHide{ false };					//是否可以隐身

	float				m_fAtkDistance{ 0 };					//攻击距离
	int					m_nAtkPrev{ 0 };						//攻击前置
	int					m_nAtkInterval{ 0 };					//攻击间隔

	float				m_fAtkSpeed{ 0 };

	ATK_TYPE			m_enAtkType{ AT_PHYSICS };				//攻击类型

	int					m_nAtkBuildingDamage{ 0 };				//攻击建筑伤害

	TARGET_TYPE			m_enNormalAtk{ TT_NORMAL };				//普通状态攻击目标
	TARGET_TYPE			m_enHideAtk{ TT_NORMAL };				//隐身状态攻击目标
	TARGET_TYPE			m_enSpecialAtk{ TT_NORMAL };			//特殊状态攻击目标

	bool				m_bIsBreakHero{ false };				//是否阻挡英雄

	int					m_nStr{ 0 };							//力量
	int					m_nCmd{ 0 };							//统帅
	int					m_nInt{ 0 };							//智力

	int					m_nPosIndex{ 0 };						//栏位位置

	bool				m_bBeDamaged{ false };                  //是否被攻击

	std::vector<std::tuple<const CPassSkill*, int, int>> m_vctPasSkills;	//被动技能(技能,CD,间隔)

	//----------------------------------------------------------------------------

	int				m_nAtkEx{ 0 };							//额外攻击
	int				m_nDefEx{ 0 };							//额外防御
	int				m_nMatkEx{ 0 };							//额外魔攻
	int				m_nMdefEx{ 0 };							//额外魔防

	int				m_nStrEx{ 0 };							//额外力量
	int				m_nCmdEx{ 0 };							//额外统帅
	int				m_nIntEx{ 0 };							//额外智力

	float			m_nMoveSpeedEx{ 0 };					//额外移动速度
	short			m_nAtkIntervalEx{ 0 };					//额外攻击间隔
	short			m_nAtkPrevEx{ 0 };						//额外攻击前置

	UnitState		m_enState{ US_STAND };					//单位状态
	CBattleObject*  m_pTarget{ nullptr };					//攻击目标

	bool			m_bIsHiding{ false };					//是否隐身状态
	bool			m_bIsSpecialMode{ false };				//是否特殊状态

	short			m_nAtkIntervalCounts{ 0 };				//攻击间隔时间
	short			m_nAtkPrevCounts{ 0 };					//攻击前置时间

	short			m_nBeDamageCount{ 0 };					//受击伤害累计
	short			m_nResetBeDamagedTime{ 0 };				//受击伤害重置时间
	short			m_nBeDamageProtected{ 0 };				//受击保护时间
	short			m_nSpasticityCountBeDamage{ 0 };		//不受控制时间(被打)
	short			m_nSpasticityCountSkill{ 0 };			//不受控制时间(技能)

	int				m_pUsingPassSkillIndex{ -1 };			//正在使用的被动技能

	float			m_fSuckBloodValue{ 0 };					//%多少吸血
	float			m_fPreventCrit{ 0 };			//免暴几率

	//*********************************Custom************************************

	int				m_nMasterID{ 0 };						//所处的Master位置
	AIType			m_enAIType{ AI_NORMAL };				//AI模式()
	int				m_nNormalAtkBulletID{ 0 };				//单位普攻子弹
	float			m_fActiveRange{ 0 };					//警戒范围
	float			m_fPatrolPoint{ 0 };					//巡逻点
	bool			m_bIsBackup{ false };					//是否后备
	int				m_nStartBuffID{ 0 };					//起始buff
	bool            m_bChase{ true };
	bool            m_bStartChase{ false };
	bool			m_bSeeHide{ false };					//反隐身
	bool			m_bImmuneStun{ false };					//免疫眩晕
	bool			m_bImmuneSlowDown{ false };				//免疫减速禁锢
	bool			m_bImmunePush{ false };					//免疫击退拉扯
	bool			m_bImmuneFear{ false };					//免疫恐惧

	bool			not_attack_{ false };					//不攻击
	bool			double_attack_{ false };					//两次伤害


	//****************************************************************************

	virtual void	_Act();									//行动
	virtual void	_Die(CBattleObject* pSrc);				//死亡
	void			_RemoveUnit();							//消灭单位

	void			ProduceLootBullet();					//生成额外掉落子弹
	void			StartPatrol();							//到达地图边缘开始范围警戒

	virtual void	_MovingLoop();							//移动Loop
	void			_StartMove();							//开始移动
	virtual void	_StopMove(bool send);					//结束移动

	void			_PatrolLoop();							//巡逻Loop

	void			_AttackLoop();							//攻击loop	
	void			_StartAttack();							//开始攻击
	void			_Attacking();							//攻击中
	void			_StopAttack();							//结束攻击

	void			_UsingSkill();							//使用技能Loop
	virtual void	_StartUseSkill();						//开始使用技能
	virtual void	_StopUseSkill();						//停止使用技能

	bool			_IsSpasticity();						//是否僵直
	void			_AdjustPos();							//调整坐标
	void			_CalculateState();						//计算状态
	virtual void	_CalculateSkillCD();					//计算技能CD

	void			_BreakHide();							//打破隐身

	virtual void	_CalculateBuff() override;

	float			_DamageFormula(int src_Str_Int, int dest_Str_Int, int dest_cmd);						//伤害计算公式

	void			_PrevBeDamagedSkillCalculate(const DamageInfo& di, float &fDamage, float &fMDamage);
	void			_PrevAttackSkillCalculate();
	void			_AfterAttackSkillCalculate();

	virtual void	_GetAlwaysPassSkill();

	void			_BaseHeal();

	std::vector<std::pair<CBuff*, int>> _FindDotEffect(BUFF_BASIC_EFFECT bbe);

private:
	const CSoldierTP*	m_pSoldierTP{ nullptr };				//士兵模版
	SOLDIER_TYPE		m_enSoldierType{ SOLDIER_TYPE::NORMAL };//士兵类型
	int					m_nPopulation{ 0 };						//所占人口
};
