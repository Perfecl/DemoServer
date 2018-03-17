#pragma once
#include "BattleObject.h"

class CBuilding : public CBattleObject
{
public:
	static CBuilding* CreateNormalBuilding(CBattle* pBattle, BuildingType enType, Force enForce, float fX, int nHpMul = 1);

	static const CBuilding* CreateCustomBuilding(const Buliding& building);

public:
	CBuilding(CBattle* pBattle);
	virtual ~CBuilding();

	void				Loop() override;
	virtual	void		BeDamaged(const DamageInfo& di) override;
	void				BeDamaged(int nNum);
	virtual ObjectType	GetObjectType() const override{ return OT_BUILDING; }
	BuildingType		GetBuidlingType() const { return m_enType; }

	void				SetProtocol(pto_BATTLE_Struct_Building *pBuilding);

	BulidingState		GetBuildingState() const { return m_enState; }

	CBuilding*			Clone(CBattle* pBattle) const;
	void				AddMasterCasletUpgrade(int add_hp){ m_nHP = m_nMaxHp = m_nHP + add_hp; }
	int					GetResourceID() const { return m_nResourceID; }
	void				Repair(int add_hp);

private:
	BuildingType		m_enType{ BUILDING_TYPE_NULL };			//��������
	int					m_nResourceID{ 0 };						//��ԴID
	int					m_nRepairTime{ 0 };						//�޸�ʱ��
	int					m_nRepairNeedTime{ 0 };					//��Ҫ�޸���ʱ��
	int					m_nDestroyedGetResrc{ 0 };				//����ɱ�жԿɻ�õ���Դ��
	int					m_nTowerPerIncrease{ 0 };				//ÿ����������Դ��
	int					m_nProduceTime{ 0 };					//��������
	int					m_nProduceTimeCount{ 0 };				//��Դ������ʱ��
	int					m_nProduceTimes{ 0 };					//��Դ��������
	int					m_nNextResource{ 0 };					//�´�Ҫ��������Դ��
	BulidingState		m_enState{ BUILDING_STATE_NORMAL };		//����״̬

	void __ProduceLoop();
};

