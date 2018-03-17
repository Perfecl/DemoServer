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
	virtual void		SetUnitProtocol(pto_BATTLE_Struct_Unit* ptoUnit);	//���õĵ�λЭ��

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

	TARGET_TYPE			GetNormalAtkMode() const { return m_enNormalAtk; }		//��ͨ״̬ʱ����Ŀ��ѡ��
	TARGET_TYPE			GetHideAtkMode() const { return m_enHideAtk; }			//����״̬ʱ����Ŀ��ѡ��
	TARGET_TYPE			GetSpecialAtkMode() const { return m_enSpecialAtk; }	//����״̬ʱ����Ŀ��ѡ��
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
	bool				IsImmuneSlowDown() const { return m_bImmuneSlowDown; }				//���߼��ٽ���
	bool				IsImmunePush() const{ return m_bImmunePush; }						//���߻�������
	bool				IsImmuneFear() const{ return m_bImmuneFear; }

	virtual bool		Fallback(float fPos);					//����

	void				Heal(int nNum);							//�ظ�
	void				Retreat();								//����
	void				StopRetreat();							//ֹͣ����
	void				Fear();									//�־�
	void				StopFear();								//ֹͣ�־�

	virtual void		Stun();									//ѣ��

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
	std::string			m_strName;								//����
	int					level_{ 0 };							//�ȼ�

	bool				m_bIsSummonUnit;						//�ǲ��Ǽ����ٻ�������

	int					m_nAtk{ 0 };							//��������
	int					m_nDef{ 0 };							//��������
	int					m_nMatk{ 0 };							//����ħ��
	int					m_nMdef{ 0 };							//����ħ��

	bool				m_bIsRange{ false };					//�Ƿ���Զ�̵�λ
	bool				m_bCanHide{ false };					//�Ƿ��������

	float				m_fAtkDistance{ 0 };					//��������
	int					m_nAtkPrev{ 0 };						//����ǰ��
	int					m_nAtkInterval{ 0 };					//�������

	float				m_fAtkSpeed{ 0 };

	ATK_TYPE			m_enAtkType{ AT_PHYSICS };				//��������

	int					m_nAtkBuildingDamage{ 0 };				//���������˺�

	TARGET_TYPE			m_enNormalAtk{ TT_NORMAL };				//��ͨ״̬����Ŀ��
	TARGET_TYPE			m_enHideAtk{ TT_NORMAL };				//����״̬����Ŀ��
	TARGET_TYPE			m_enSpecialAtk{ TT_NORMAL };			//����״̬����Ŀ��

	bool				m_bIsBreakHero{ false };				//�Ƿ��赲Ӣ��

	int					m_nStr{ 0 };							//����
	int					m_nCmd{ 0 };							//ͳ˧
	int					m_nInt{ 0 };							//����

	int					m_nPosIndex{ 0 };						//��λλ��

	bool				m_bBeDamaged{ false };                  //�Ƿ񱻹���

	std::vector<std::tuple<const CPassSkill*, int, int>> m_vctPasSkills;	//��������(����,CD,���)

	//----------------------------------------------------------------------------

	int				m_nAtkEx{ 0 };							//���⹥��
	int				m_nDefEx{ 0 };							//�������
	int				m_nMatkEx{ 0 };							//����ħ��
	int				m_nMdefEx{ 0 };							//����ħ��

	int				m_nStrEx{ 0 };							//��������
	int				m_nCmdEx{ 0 };							//����ͳ˧
	int				m_nIntEx{ 0 };							//��������

	float			m_nMoveSpeedEx{ 0 };					//�����ƶ��ٶ�
	short			m_nAtkIntervalEx{ 0 };					//���⹥�����
	short			m_nAtkPrevEx{ 0 };						//���⹥��ǰ��

	UnitState		m_enState{ US_STAND };					//��λ״̬
	CBattleObject*  m_pTarget{ nullptr };					//����Ŀ��

	bool			m_bIsHiding{ false };					//�Ƿ�����״̬
	bool			m_bIsSpecialMode{ false };				//�Ƿ�����״̬

	short			m_nAtkIntervalCounts{ 0 };				//�������ʱ��
	short			m_nAtkPrevCounts{ 0 };					//����ǰ��ʱ��

	short			m_nBeDamageCount{ 0 };					//�ܻ��˺��ۼ�
	short			m_nResetBeDamagedTime{ 0 };				//�ܻ��˺�����ʱ��
	short			m_nBeDamageProtected{ 0 };				//�ܻ�����ʱ��
	short			m_nSpasticityCountBeDamage{ 0 };		//���ܿ���ʱ��(����)
	short			m_nSpasticityCountSkill{ 0 };			//���ܿ���ʱ��(����)

	int				m_pUsingPassSkillIndex{ -1 };			//����ʹ�õı�������

	float			m_fSuckBloodValue{ 0 };					//%������Ѫ
	float			m_fPreventCrit{ 0 };			//�Ⱪ����

	//*********************************Custom************************************

	int				m_nMasterID{ 0 };						//������Masterλ��
	AIType			m_enAIType{ AI_NORMAL };				//AIģʽ()
	int				m_nNormalAtkBulletID{ 0 };				//��λ�չ��ӵ�
	float			m_fActiveRange{ 0 };					//���䷶Χ
	float			m_fPatrolPoint{ 0 };					//Ѳ�ߵ�
	bool			m_bIsBackup{ false };					//�Ƿ��
	int				m_nStartBuffID{ 0 };					//��ʼbuff
	bool            m_bChase{ true };
	bool            m_bStartChase{ false };
	bool			m_bSeeHide{ false };					//������
	bool			m_bImmuneStun{ false };					//����ѣ��
	bool			m_bImmuneSlowDown{ false };				//���߼��ٽ���
	bool			m_bImmunePush{ false };					//���߻�������
	bool			m_bImmuneFear{ false };					//���߿־�

	bool			not_attack_{ false };					//������
	bool			double_attack_{ false };					//�����˺�


	//****************************************************************************

	virtual void	_Act();									//�ж�
	virtual void	_Die(CBattleObject* pSrc);				//����
	void			_RemoveUnit();							//����λ

	void			ProduceLootBullet();					//���ɶ�������ӵ�
	void			StartPatrol();							//�����ͼ��Ե��ʼ��Χ����

	virtual void	_MovingLoop();							//�ƶ�Loop
	void			_StartMove();							//��ʼ�ƶ�
	virtual void	_StopMove(bool send);					//�����ƶ�

	void			_PatrolLoop();							//Ѳ��Loop

	void			_AttackLoop();							//����loop	
	void			_StartAttack();							//��ʼ����
	void			_Attacking();							//������
	void			_StopAttack();							//��������

	void			_UsingSkill();							//ʹ�ü���Loop
	virtual void	_StartUseSkill();						//��ʼʹ�ü���
	virtual void	_StopUseSkill();						//ֹͣʹ�ü���

	bool			_IsSpasticity();						//�Ƿ�ֱ
	void			_AdjustPos();							//��������
	void			_CalculateState();						//����״̬
	virtual void	_CalculateSkillCD();					//���㼼��CD

	void			_BreakHide();							//��������

	virtual void	_CalculateBuff() override;

	float			_DamageFormula(int src_Str_Int, int dest_Str_Int, int dest_cmd);						//�˺����㹫ʽ

	void			_PrevBeDamagedSkillCalculate(const DamageInfo& di, float &fDamage, float &fMDamage);
	void			_PrevAttackSkillCalculate();
	void			_AfterAttackSkillCalculate();

	virtual void	_GetAlwaysPassSkill();

	void			_BaseHeal();

	std::vector<std::pair<CBuff*, int>> _FindDotEffect(BUFF_BASIC_EFFECT bbe);

private:
	const CSoldierTP*	m_pSoldierTP{ nullptr };				//ʿ��ģ��
	SOLDIER_TYPE		m_enSoldierType{ SOLDIER_TYPE::NORMAL };//ʿ������
	int					m_nPopulation{ 0 };						//��ռ�˿�
};
