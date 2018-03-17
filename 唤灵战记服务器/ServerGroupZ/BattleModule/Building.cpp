#include "stdafx.h"
#include "Building.h"
#include "Battle.h"
#include "Bullet.h"

CBuilding::CBuilding(CBattle*	pBattle) :
CBattleObject{ pBattle }
{

}

CBuilding::~CBuilding()
{

}

void CBuilding::Loop()
{
	CBattleObject::Loop();

	if (BUILDING_TYPE_CRYSTAL_TOWER == m_enType)
		__ProduceLoop();
}

CBuilding* CBuilding::CreateNormalBuilding(CBattle* pBattle, BuildingType enType, Force enForce, float fX, int nHpMul /* = 1*/)
{
	CBuilding *pBuilding = new CBuilding{ pBattle };

	pBuilding->m_enForce = enForce;
	pBuilding->m_enType = enType;
	pBuilding->m_nResourceID = enType;
	pBuilding->m_fPos = fX;

	if (nHpMul < 1)
		nHpMul = 1;

	switch (enType)
	{
	case BUILDING_TYPE_ATKCASTLE:
	case BUILDING_TYPE_DEFCASTLE:
		pBuilding->m_fVolume = 350;
		pBuilding->m_nHP = pBuilding->m_nMaxHp = 7500;
		break;
	case BUILDING_TYPE_ATKTOWER:
	case BUILDING_TYPE_DEFTOWER:
		pBuilding->m_fVolume = 100;
		pBuilding->m_nHP = pBuilding->m_nMaxHp = 2000;
		pBuilding->m_nDestroyedGetResrc = 300;
		break;
	case BUILDING_TYPE_CRYSTAL_TOWER:
		pBuilding->m_nHP = pBuilding->m_nMaxHp = 1;
		pBuilding->m_fVolume = 100;
		pBuilding->m_nProduceTime = 30000 + GetRandom(0, 10000) - GetRandom(0, 10000);
		pBuilding->m_nTowerPerIncrease = 100;
		break;
	}

	return pBuilding;
}

const CBuilding* CBuilding::CreateCustomBuilding(const Buliding& building)
{
	CBuilding* pBuilding = new CBuilding(nullptr);

	pBuilding->m_nHP = pBuilding->m_nMaxHp = building.hp();
	if (pBuilding->m_nHP == 0) pBuilding->m_nHP = pBuilding->m_nMaxHp = 1000;
	pBuilding->m_fPos = (float)building.pos();
	pBuilding->m_nResourceID = building.modelresourceid();
	pBuilding->m_nGroupID = building.group();
	pBuilding->m_enForce = (Force)building.belong();
	pBuilding->m_fVolume = (float)building.volume();
	pBuilding->m_nProduceTime = building.producetime();
	pBuilding->m_nTowerPerIncrease = building.perincrease();

	switch (building.type())
	{
	case 1:
		if (building.belong() == Force::ATK)
			pBuilding->m_enType = BUILDING_TYPE_ATKCASTLE;
		if (building.belong() == Force::DEF)
			pBuilding->m_enType = BUILDING_TYPE_DEFCASTLE;
		if (building.belong() == Force::NEUTRAL)
			pBuilding->m_enType = BULIDING_TYPE_NEUTRAL_BULIDING;
		pBuilding->m_fVolume = 350;
		break;
	case 2:
		if (building.belong() == Force::ATK)
			pBuilding->m_enType = BUILDING_TYPE_ATKTOWER;
		if (building.belong() == Force::DEF)
			pBuilding->m_enType = BUILDING_TYPE_DEFTOWER;
		if (building.belong() == Force::NEUTRAL)
			pBuilding->m_enType = BULIDING_TYPE_NEUTRAL_BULIDING;
		pBuilding->m_fVolume = 130;
		break;
	case 3:
		pBuilding->m_enType = BUILDING_TYPE_CRYSTAL_TOWER;
		pBuilding->m_enForce = Force::NEUTRAL;
		break;
	default:
		pBuilding->m_fVolume = 100;
	}

	return pBuilding;
}

void CBuilding::SetProtocol(pto_BATTLE_Struct_Building *ptoBuilding)
{
	ptoBuilding->set_enbldtype(m_enType);
	ptoBuilding->set_enbldstate(m_enState);
	ptoBuilding->set_ntowerperincrease(m_nTowerPerIncrease);
	ptoBuilding->set_nproducetime(m_nProduceTime);
	ptoBuilding->set_nrepairtime(m_nRepairTime);
	ptoBuilding->set_nrepairneedtime(m_nRepairNeedTime);
	ptoBuilding->set_nid(m_nID);
	ptoBuilding->set_nx(m_fPos);
	ptoBuilding->set_enforce(m_enForce);
	ptoBuilding->set_nhp(m_nHP);
	ptoBuilding->set_nmaxhp(m_nMaxHp);
	ptoBuilding->set_nbodyrange(m_fVolume);

	//----ÁÙÊ±-----
	ptoBuilding->set_nresourcesmoduleclass(1);
	ptoBuilding->set_nresourcesmoduletype(m_nResourceID);

	int id = m_enType;
	if (BUILDING_TYPE_CRYSTAL_TOWER == m_enType)
		id = 0;

	ptoBuilding->set_nresourcesmoduleid(id);
	//-------------
}

void CBuilding::BeDamaged(const DamageInfo& di)
{
	int nValue = di.nDamage + di.nMDamage;
	m_nHP -= nValue;

	int			nSrcObjID{ 0 };
	ObjectType	enSrcObjType{ OT_NULL };

	if (di.pSrcObject)
	{
		nSrcObjID = di.pSrcObject->GetID();
		enSrcObjType = di.pSrcObject->GetObjectType();
	}

	m_pBattle->GetProtoPackage(Force::ATK)->BeDamage(m_nID, nValue, GetObjectType(), nSrcObjID, enSrcObjType, false, 0, !di.bIsSkill);
	m_pBattle->GetProtoPackage(Force::DEF)->BeDamage(m_nID, nValue, GetObjectType(), nSrcObjID, enSrcObjType, false, 0, !di.bIsSkill);

	if (m_nHP <= 0)
	{
		m_enState = BUILDING_STATE_DESTROYED;

		pto_BATTLE_S2C_NTF_BuildingDead pto;
		pto.set_nid(m_nID);
		string strPto;
		pto.SerializeToString(&strPto);
		m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_BuildingDead);

		if (Force::ATK == m_enForce)
			m_pBattle->AddResource(Force::DEF, m_nDestroyedGetResrc);
		else if (Force::DEF == m_enForce)
			m_pBattle->AddResource(Force::ATK, m_nDestroyedGetResrc);

		if (BUILDING_TYPE_ATKCASTLE == m_enType || BUILDING_TYPE_DEFCASTLE == m_enType)
		{
			if (Force::ATK == m_enForce && !m_pBattle->GetAtkBaseOver())
			{
				m_pBattle->GameOver(m_enForce, m_fPos);
			}
			else if (Force::DEF == m_enForce && !m_pBattle->GetDefBaseOver())
			{
				m_pBattle->GameOver(m_enForce, m_fPos);
			}

		}
	}
}

void CBuilding::BeDamaged(int nNum)
{
	if (nNum <= 0)
		nNum = 1;

	m_nHP -= nNum;

	m_pBattle->GetProtoPackage(Force::ATK)->BeDamage(m_nID, nNum, GetObjectType());
	m_pBattle->GetProtoPackage(Force::DEF)->BeDamage(m_nID, nNum, GetObjectType());

	if (m_nHP <= 0)
	{
		m_enState = BUILDING_STATE_DESTROYED;

		pto_BATTLE_S2C_NTF_BuildingDead pto;
		pto.set_nid(m_nID);
		std::string strPto;
		pto.SerializeToString(&strPto);
		m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_BuildingDead);

		if (Force::ATK == m_enForce)
			m_pBattle->AddResource(Force::DEF, m_nDestroyedGetResrc);
		else if (Force::DEF == m_enForce)
			m_pBattle->AddResource(Force::ATK, m_nDestroyedGetResrc);

		if (BUILDING_TYPE_ATKCASTLE == m_enType || BUILDING_TYPE_DEFCASTLE == m_enType)
		{
			if (Force::ATK == m_enForce && !m_pBattle->GetAtkBaseOver())
			{
				m_pBattle->GameOver(m_enForce, m_fPos);
			}
			else if (Force::DEF == m_enForce && !m_pBattle->GetDefBaseOver())
			{
				m_pBattle->GameOver(m_enForce, m_fPos);
			}

		}
	}
}

CBuilding*	CBuilding::Clone(CBattle* pBattle) const
{
	CBuilding* pBuilding = new CBuilding{ *this };

	pBuilding->m_pBattle = pBattle;

	return pBuilding;
}

void CBuilding::Repair(int add_hp)
{
	if (add_hp + m_nHP > m_nMaxHp)
		add_hp = m_nMaxHp - m_nHP;
	m_nHP += add_hp;

	m_pBattle->GetProtoPackage(Force::ATK)->Heal(m_nID, add_hp, m_nID, GetObjectType(), 0, GetObjectType(), false);
	m_pBattle->GetProtoPackage(Force::DEF)->Heal(m_nID, add_hp, m_nID, GetObjectType(), 0, GetObjectType(), false);
}

void CBuilding::__ProduceLoop()
{
	if (0 == m_nProduceTimeCount)
	{
		int ran = GetRandom(1, 3);

		m_nNextResource = m_nTowerPerIncrease * ran;

		int nMasterNum{ m_pBattle->GetMasterNum(Force::ATK, true, false) > m_pBattle->GetMasterNum(Force::DEF, true, false) ? m_pBattle->GetMasterNum(Force::ATK, true, false) : m_pBattle->GetMasterNum(Force::DEF, true, false) };

		if (2 == nMasterNum)
			m_nNextResource = int(m_nNextResource * 1.6f);
		else if (3 == nMasterNum)
			m_nNextResource = int(m_nNextResource * 2.0f);

		std::string strPto;
		pto_BATTLE_S2C_NTF_StartProduce pto;

		pto.set_nid(m_nID);
		pto.set_nextresource(m_nNextResource);
		pto.set_nproducetime(m_nProduceTime);
		pto.SerializeToString(&strPto);
		m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_StartProduce);
	}

	m_nProduceTimeCount += BATTLE_LOOP_TIME;

	if (m_nProduceTimeCount >= m_nProduceTime)
	{
		m_nProduceTime = 30000 + GetRandom(0, 10000) - GetRandom(0, 10000);
		m_nProduceTimeCount = 0;

		m_enState = BUILDING_STATE_DESTROYED;

		std::string strPto;
		pto_BATTLE_S2C_NTF_SetNeutralBuildingState pto;
		pto.set_nid(m_nID);
		pto.set_nstate(BUILDING_STATE_DESTROYED);
		pto.SerializeToString(&strPto);
		m_pBattle->SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_StartProduce);

		const CBulletTP* pBulletTP = CBulletTP::GetBulletTP(999);

		CBullet *pBullet = new CBullet(m_pBattle);

		pBullet->InitBullet(pBulletTP, this, m_fPos);

		pBullet->SetEffectValueEx(m_nNextResource);

		m_pBattle->AddObject(pBullet);

		m_nProduceTimes++;
	}
}
