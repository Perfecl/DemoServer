#pragma once
#include "BattleObject.h"
#include "BulletTP.h"

class CBullet : public CBattleObject
{
public:
	CBullet(CBattle* pBattle);
	virtual ~CBullet();

	void Loop() override;
	virtual ObjectType GetObjectType() const override{ return OT_BULLET; }

	void		InitBullet(const CBulletTP* pBulletTP, CBattleObject* pOwner, float m_fUsingSkillPos);
	void		InitBullet(const CBulletTP* pBulletTP, Force enForce, float fPos);
	void		SetSkill(const CSkill* pSkill){ m_pSkill = pSkill; }
	void		SetBulletProtocol(pto_BATTLE_Struct_Bullet* pBullet);
	void		SetEffectValueEx(int nNum){ m_nEffectValueEx = nNum; }
	void		SetInfinite(bool bInfinite){ m_bInfinite = bInfinite; }
	void		SetCustomID(int nCustomID){ m_nCustomID = nCustomID; }
	void		set_master(CMaster* master){ master_ = master; }
	CMaster*	master() const { return master_; }

	int			GetEffectValue() const { return  m_pBulletTP->m_nEffectValue + m_nEffectValueEx; }
	BulletForm  GetBulletForm() const { return m_pBulletTP->m_enForm; }
	int			GetCustomID() const { return m_nCustomID; }
	bool		IsDead() const { return m_bIsDead; }
	void		Die();

	int			GetEffect() const { return m_pBulletTP->m_nEffect; }
	void		RemainStageLoot(CMaster* master);
	bool		IsChargeBullet();
	CBattleObject* owner(){ return m_pOwner; }

private:
	const CBulletTP*	m_pBulletTP{ nullptr };							//子弹模版指针
	const CSkill*		m_pSkill{ nullptr };							//关联的技能
	CBattleObject*		m_pOwner{ nullptr };							//释放者

	bool				m_bIsDead{ false };								//是否死亡

	float				m_fStartPos{ 0 };								//起始位置

	CBattleObject*		m_pTarget{ nullptr };							//目标单位

	char				m_cMoveState{ 0 };								//移动状态(0还未移动  1移动中  2移动完成)
	int					m_nMoveLeadTime{ 0 };							//移动前置时间

	int					m_nLifeTime{ 0 };								//存活时间
	int					m_nSettlementIntervalTime{ 0 };					//结算间隔时间
	int					m_nSettlementTimes{ 0 };						//结算总次数
	bool                m_bInfinite{ false };                           //是否一直存在

	int					m_nHitTimes{ 0 };								//命中次数

	int					m_nEffectValueEx{ 0 };							//特殊效果加成

	int					m_nDeadByWho{ 0 };								//死于谁
	int					m_nCustomID{ 0 };								//自定义ID

	CMaster*			master_{ nullptr };								//玩家

	std::map<CBattleObject*, BulletSingleInfo*> m_mapBulletSingleInfos;		//信息组

	void				__OnMove();										//移动
	void				__MoveComplete();								//移动结束

	bool				__CanSettlement();								//是否可以结算
	void				__MultiSettlement();							//群体结算
	void				__SingleSettlement();							//单体结算
	void				__Settlement(CUnit* pUnit);						//结算
	void				__Settlement(CBuilding* pBuilding);				//结算

	bool				__CheckSingleInfoValid(CBattleObject *pUnit);	//检查单个单位信息
	bool				__IsGoingToDie();								//是否去死

	BulletSingleInfo*	__GetBulletSingleInfo(CBattleObject* pObj);		//获取子弹单个单位信息
};
