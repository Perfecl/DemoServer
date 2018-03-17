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
	const CBulletTP*	m_pBulletTP{ nullptr };							//�ӵ�ģ��ָ��
	const CSkill*		m_pSkill{ nullptr };							//�����ļ���
	CBattleObject*		m_pOwner{ nullptr };							//�ͷ���

	bool				m_bIsDead{ false };								//�Ƿ�����

	float				m_fStartPos{ 0 };								//��ʼλ��

	CBattleObject*		m_pTarget{ nullptr };							//Ŀ�굥λ

	char				m_cMoveState{ 0 };								//�ƶ�״̬(0��δ�ƶ�  1�ƶ���  2�ƶ����)
	int					m_nMoveLeadTime{ 0 };							//�ƶ�ǰ��ʱ��

	int					m_nLifeTime{ 0 };								//���ʱ��
	int					m_nSettlementIntervalTime{ 0 };					//������ʱ��
	int					m_nSettlementTimes{ 0 };						//�����ܴ���
	bool                m_bInfinite{ false };                           //�Ƿ�һֱ����

	int					m_nHitTimes{ 0 };								//���д���

	int					m_nEffectValueEx{ 0 };							//����Ч���ӳ�

	int					m_nDeadByWho{ 0 };								//����˭
	int					m_nCustomID{ 0 };								//�Զ���ID

	CMaster*			master_{ nullptr };								//���

	std::map<CBattleObject*, BulletSingleInfo*> m_mapBulletSingleInfos;		//��Ϣ��

	void				__OnMove();										//�ƶ�
	void				__MoveComplete();								//�ƶ�����

	bool				__CanSettlement();								//�Ƿ���Խ���
	void				__MultiSettlement();							//Ⱥ�����
	void				__SingleSettlement();							//�������
	void				__Settlement(CUnit* pUnit);						//����
	void				__Settlement(CBuilding* pBuilding);				//����

	bool				__CheckSingleInfoValid(CBattleObject *pUnit);	//��鵥����λ��Ϣ
	bool				__IsGoingToDie();								//�Ƿ�ȥ��

	BulletSingleInfo*	__GetBulletSingleInfo(CBattleObject* pObj);		//��ȡ�ӵ�������λ��Ϣ
};
