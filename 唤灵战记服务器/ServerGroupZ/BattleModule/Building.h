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
	BuildingType		m_enType{ BUILDING_TYPE_NULL };			//建筑类型
	int					m_nResourceID{ 0 };						//资源ID
	int					m_nRepairTime{ 0 };						//修复时间
	int					m_nRepairNeedTime{ 0 };					//需要修复的时间
	int					m_nDestroyedGetResrc{ 0 };				//被击杀敌对可获得的资源量
	int					m_nTowerPerIncrease{ 0 };				//每次生产的资源量
	int					m_nProduceTime{ 0 };					//生产周期
	int					m_nProduceTimeCount{ 0 };				//资源生产计时器
	int					m_nProduceTimes{ 0 };					//资源生产次数
	int					m_nNextResource{ 0 };					//下次要生产的资源数
	BulidingState		m_enState{ BUILDING_STATE_NORMAL };		//建筑状态

	void __ProduceLoop();
};

