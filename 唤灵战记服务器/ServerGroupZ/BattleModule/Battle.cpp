#include "stdafx.h"
#include "StageBattle.h"
#include "Master.h"
#include "Building.h"
#include "Hero.h"
#include "Bullet.h"
#include "Gate.h"
#include "CustomBullet.h"
#include "BattleManager.h"

CBattleServer* CBattle::ms_pServerManager{ nullptr };

CBattle::CBattle()
{
	for (auto &it : m_arrProtoPackages)
	{
		it = new CProtoPackage();
		it->NewPackage();
	}
	m_arrMasters.fill(nullptr);
}

CBattle::~CBattle()
{
	for (auto &it : m_arrMasters)
		delete it;
	for (auto &it : m_mapAtkUnits)
		delete it.second;
	for (auto &it : m_mapDefUnits)
		delete it.second;
	for (auto &it : m_mapDeadUnits)
		delete it.second;
	for (auto &it : m_mapAtkBuildings)
		delete it.second;
	for (auto &it : m_mapDefBuildings)
		delete it.second;
	for (auto &it : m_mapNeutralBuildings)
		delete it.second;
	for (auto &it : m_setBullets)
		delete it;
	for (auto &it : m_arrProtoPackages)
		delete it;
	for (auto &it : m_mapGates)
		delete it.second;
	for (auto &it : m_setWallBullets)
		delete it;
}

void CBattle::SetBattleByPto(const pto_BATTLE_S2B_REQ_CreateBattle_Ex* pPto)
{
	m_nSceneID = pPto->map();

	m_nAtkPoint = pPto->atk_point();
	m_nDefPoint = pPto->def_point();

	for (int i = 0; i < pPto->atk_masters_size(); i++)
	{
		CMaster* pMaster = new CMaster(this);
		pMaster->SetMasterID(i + 1);
		pMaster->SetMasterByPto(&pPto->atk_masters(i), Force::ATK);
		m_arrMasters[i] = pMaster;

		int nProcess{ 0 };
		if (pMaster->IsAI())
			nProcess = 100;
		m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = nProcess;
	}

	for (int i = 0; i < pPto->def_masters_size(); i++)
	{
		CMaster* pMaster = new CMaster(this);
		pMaster->SetMasterID(i + 4);
		pMaster->SetMasterByPto(&pPto->def_masters(i), Force::DEF);
		m_arrMasters[i + 3] = pMaster;

		int nProcess{ 0 };
		if (pMaster->IsAI())
			nProcess = 100;

		m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = nProcess;
	}
}

void CBattle::SetAIBattleByPto(const pto_BATTLE_S2B_REQ_CreateBattle_Ex* pPto)
{
	m_nSceneID = pPto->map();

	m_nAtkPoint = pPto->atk_point();
	m_nDefPoint = pPto->def_point();

	for (int i = 0; i < pPto->atk_masters_size(); i++)
	{
		CMaster* pMaster = new CMaster(this);
		pMaster->SetMasterID(i + 1);
		pMaster->SetMasterByPto(&pPto->atk_masters(i), Force::ATK);
		m_arrMasters[i] = pMaster;

		int nProcess{ 0 };
		if (pMaster->IsAI())
			nProcess = 100;
		m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = nProcess;
	}

	if ((BattleMode)pPto->mode() == BattleMode::ARENA1v1Easy || (BattleMode)pPto->mode() == BattleMode::ARENA1v1Normal)
	{
		CMaster* pMaster = new CMaster(this);
		pMaster->SetIsAI(true);
		pMaster->SetMasterID(4);
		pMaster->SetForce(Force::DEF);
		m_arrMasters[3] = pMaster;
		m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = 100;
		pMaster->SetArenaAI((BattleMode)pPto->mode());
	}
	if ((BattleMode)pPto->mode() == BattleMode::ARENA3v3Easy || (BattleMode)pPto->mode() == BattleMode::ARENA3v3Normal)
	{
		for (size_t i = 0; i < 3; i++)
		{
			CMaster* pMaster = new CMaster(this);
			pMaster->SetIsAI(true);
			pMaster->SetMasterID(4 + i);
			pMaster->SetForce(Force::DEF);
			m_arrMasters[3 + i] = pMaster;
			m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = 100;
			pMaster->SetArenaAI((BattleMode)pPto->mode());
		}
	}
}

void CBattle::SetBattleState(BattleState enState)
{
	if (BtSt_Over == m_enState)
		return;

	m_enState = enState;
}

bool CBattle::AddObject(CUnit* pUnit, bool bSend /*=true*/)
{
	if (!pUnit)
		return false;

	bool bFlag{ false };

	pUnit->SetID(GetUnitID());

	if (Force::ATK == pUnit->GetForce())
	{
		auto it = m_mapAtkUnits.insert(std::make_pair(pUnit->GetID(), pUnit));
		bFlag = it.second;
	}
	else if (Force::DEF == pUnit->GetForce())
	{
		auto it = m_mapDefUnits.insert(std::make_pair(pUnit->GetID(), pUnit));
		bFlag = it.second;
	}

	if (bFlag)
	{

		if (bSend)
		{
			if (pUnit->IsBoss())
				pUnit->SendCustomUnitToClient();
			else
				pUnit->SendUnitToClient();
		}

		return true;
	}

	return false;
}

bool CBattle::AddObject(CBuilding *pBuilding)
{
	std::pair<std::map<int, CBuilding*>::iterator, bool> res;

	pBuilding->SetID(GetBulidingNum() + 1);

	switch (pBuilding->GetForce())
	{
	case Force::ATK:
		res = m_mapAtkBuildings.insert(std::make_pair(pBuilding->GetID(), pBuilding));
		break;
	case Force::DEF:
		res = m_mapDefBuildings.insert(std::make_pair(pBuilding->GetID(), pBuilding));
		break;
	case Force::NEUTRAL:
		res = m_mapNeutralBuildings.insert(std::make_pair(pBuilding->GetID(), pBuilding));
		break;
	default:
		return false;
	}

	return res.second;
}

bool CBattle::AddObject(CBullet* pBullet, bool bSend /* = true*/)
{
	pBullet->SetID(GetBulletID());

	std::pair<std::set<CBullet*>::iterator, bool> it;

	if (BF_WALL == pBullet->GetBulletForm())
		it = m_setWallBullets.insert(pBullet);
	else
		it = m_setBullets.insert(pBullet);

	if (pBullet->GetEffect() == 9997)
	{
		if (pBullet->GetForce() == Force::ATK)
			m_pAtkWarpGate = pBullet;
		else if (pBullet->GetForce() == Force::DEF)
			m_pDefWarpGate = pBullet;
	}

	if (bSend && it.second)
	{
		pto_BATTLE_Struct_Bullet ptoBullet;

		pBullet->SetBulletProtocol(&ptoBullet);

		GetProtoPackage(Force::ATK)->CreateBullet(&ptoBullet);
		GetProtoPackage(Force::DEF)->CreateBullet(&ptoBullet);
	}

	return it.second;
}

bool CBattle::TimeCounts(int nTime)
{
	if (0 == (m_nTimes % nTime))
		return true;
	else
		return false;
}

void CBattle::ProcessMessage(const CMessage& msg)
{
	int nProtocol{ msg.GetProtocolID() };

	CMaster* pMaster = FindMasterByPID(msg.GetID());

	if (!pMaster)
	{
		RECORD_WARNING("找不到召唤师");
		return;
	}

	switch (nProtocol)
	{
	case BATTLE_C2S_REQ_ClickSoldierCard:
		_ClickSoldierCard(pMaster, msg);
		break;
	case BATTLE_C2S_REQ_ClickHeroCard:
		_ClickHeroCard(pMaster, msg);
		break;
	case BATTLE_C2S_NTF_HeroMove:
		_HeroMove(pMaster, msg);
		break;
	case BATTLE_C2S_NTF_HeroMoveTo:
		_HeroMoveTo(pMaster, msg);
		break;
	case BATTLE_C2S_NTF_HeroStand:
		_HeroStand(pMaster, msg);
		break;
	case BATTLE_C2S_REQ_UseActiveSkill:
		_UseActiveSkill(pMaster, msg);
		break;
	case BATTLE_C2S_REQ_Upgradelevel:
		_UpgradeLevel(pMaster, msg);
		break;
	case BATTLE_C2S_NTF_LoadProcess:
		_LoadProcess(pMaster, msg);
		break;
	case BATTLE_C2S_REQ_Pause:
		_Pause(pMaster, true);
		break;
	case BATTLE_C2S_REQ_Continue:
		_Pause(pMaster, false);
		break;
	case BATTLE_C2S_NTF_Surrender:
		_Surrender(pMaster);
		break;
	case BATTLE_C2S_NTF_MasterLeave:
		_MasterLeave(pMaster);
		break;
	case BATTLE_C2S_REQ_UseBaseGun:
		_UseBaseGun(pMaster, msg);
		break;
	default:
		RECORD_WARNING(FormatString("未知的Battle协议号", nProtocol).c_str());
		break;
	}
}

float CBattle::CalculateDistance(CBattleObject* pObj, CBattleObject* pTarget, bool bHasAtkRange, bool bHasAlertRange)
{
	float x1 = 0;
	float x2 = 0;

	float y1 = pTarget->GetPos() - pTarget->GetVolume();
	float y2 = pTarget->GetPos() + pTarget->GetVolume();

	float fAtkRange{ 0 };
	float fAlertRange{ 0 };

	if (bHasAtkRange)
		fAtkRange = ((CUnit*)pObj)->GetAtkDistance();
	if (bHasAlertRange)
		fAlertRange = ((CUnit*)pObj)->GetActiveRange();

	float fRes = 0;
	if (pObj->GetDirection())
	{
		x1 = pObj->GetPos();
		x2 = pObj->GetPos() + pObj->GetVolume() + fAtkRange + fAlertRange;
		fRes = IntersectionLine(x1, x2, y1, y2);
		if (fRes > 0 && y2 < x2)
			return x2 - y1;
	}
	else
	{
		x1 = pObj->GetPos() - pObj->GetVolume() - fAtkRange - fAlertRange;
		x2 = pObj->GetPos();
		fRes = IntersectionLine(x1, x2, y1, y2);
		if (fRes > 0 && y1 > x1)
			return y2 - x1;
	}

	return fRes;
}

float CBattle::IntersectionLine(float x1, float x2, float y1, float y2)
{
	float  z1 = x1 > y1 ? x1 : y1;
	float  z2 = x2 < y2 ? x2 : y2;

	if (z1 > z2)
		return 0;
	else
		return z2 - z1;
}

void CBattle::SendToAllMaster(const std::string& strContent, unsigned char nProType, int nProID)
{
	for (auto &it : m_arrMasters)
	{
		if (it && it->IsValid())
		{
			int nSID{ 0 };
			if (it->GetForce() == Force::ATK)
				nSID = m_nAtkSID;
			else if (it->GetForce() == Force::DEF)
				nSID = m_nDefSID;
			SEND_TO_GAME(strContent, nProType, nProID, it->GetPID(), nSID);
		}
	}
}

void CBattle::SendToForce(Force enForce, const std::string& strContent, unsigned char nProType, int nProID)
{
	for (auto &it : m_arrMasters)
	{
		if (it && it->IsValid() && it->GetForce() == enForce)
		{
			int nSID{ 0 };
			if (it->GetForce() == Force::ATK)
				nSID = m_nAtkSID;
			else if (it->GetForce() == Force::DEF)
				nSID = m_nDefSID;
			SEND_TO_GAME(strContent, nProType, nProID, it->GetPID(), nSID);
		}
	}
}

void CBattle::SendToMaster(CMaster* pMaster, const std::string& strContent, unsigned char nProType, int nProID, bool include_leave)
{
	if (pMaster && 0 != pMaster->GetPID())
	{
		if (false == include_leave && pMaster->IsLeave())
			return;

		int nSID{ 0 };
		if (pMaster->GetForce() == Force::ATK)
			nSID = m_nAtkSID;
		else if (pMaster->GetForce() == Force::DEF)
			nSID = m_nDefSID;
		SEND_TO_GAME(strContent, nProType, nProID, pMaster->GetPID(), nSID);
	}
}

void CBattle::SendOverMessage(Force enLoseForce)
{
	pto_BATTLE_B2S_NTF_DeleteBattle pto;

	for (auto it : m_arrMasters)
	{
		if (it && !it->IsAI())
		{
			dat_BATTLE_STRUCT_StageLootEx* stage_loot_ex = pto.add_stage_loot_ex();
			it->SetStageLootDeletePto(stage_loot_ex);
		}
	}

	for (int i = 0; i < 3; i++)
	{
		if (m_arrMasters[i] && m_arrMasters[i]->GetPID() && false == m_arrMasters[i]->IsLeave())
		{
			if (Force::DEF == enLoseForce)
				pto.add_win_pids(m_arrMasters[i]->GetPID());
			else
				pto.add_lose_pids(m_arrMasters[i]->GetPID());
		}
	}

	for (int i = 3; i < 6; i++)
	{
		if (m_arrMasters[i] && m_arrMasters[i]->GetPID() && false == m_arrMasters[i]->IsLeave())
		{
			if (Force::ATK == enLoseForce)
				pto.add_win_pids(m_arrMasters[i]->GetPID());
			else
				pto.add_lose_pids(m_arrMasters[i]->GetPID());
		}
	}

	pto.set_mode((int)m_enMode);

	CStageBattle* pStageBattle{ dynamic_cast<CStageBattle*>(this) };

	if (pStageBattle)
		pto.set_mapid(pStageBattle->GetMapID());
	pto.set_box_num(m_nBoxNum);
	pto.set_use_time(m_nTimes / 1000);



	if (Force::DEF == enLoseForce)
	{
		pto.set_win_sid(m_nAtkSID);
		pto.set_lose_sid(m_nDefSID);
	}
	else
	{
		pto.set_win_sid(m_nDefSID);
		pto.set_lose_sid(m_nAtkSID);
	}


	std::string strPto;
	pto.SerializePartialToString(&strPto);

	SEND_TO_GAME(strPto, MSG_B2S, BATTLE_B2S_NTF_DeleteBattle, -1, m_nAtkSID);
	if (m_nAtkSID != m_nDefSID)
		SEND_TO_GAME(strPto, MSG_B2S, BATTLE_B2S_NTF_DeleteBattle, -1, m_nDefSID);
}

CBattle* CBattle::NewBattle(BattleMode enMode)
{
	CBattle* pBattle{ nullptr };

	if (IsStage(enMode))
		pBattle = new CStageBattle;
	else
		pBattle = new CBattle;

	pBattle->SetMode(enMode);

	return pBattle;
}

void CBattle::Initialize()
{
	InitBuilding();
}

void CBattle::InitBuilding()
{
	int nAtkNum = GetMasterNum(Force::ATK);
	int nDefNum = GetMasterNum(Force::DEF);

	m_pAtkCastle = CBuilding::CreateNormalBuilding(this, BUILDING_TYPE_ATKCASTLE, Force::ATK, 300, nDefNum);
	if (false == AddObject(m_pAtkCastle))
		delete m_pAtkCastle;
	else
	{
		for (auto it : m_arrMasters)
		{
			if (it && it->GetForce() == Force::ATK)
			{
				m_pAtkCastle->AddMasterCasletUpgrade(it->GetCastleHPEx());
				break;
			}
		}
	}
	/*CBuilding *pAtkTower = CBuilding::CreateNormalBuilding(this, BUILDING_TYPE_ATKTOWER, Force::ATK, 1600, nDefNum);
	if (false == AddObject(pAtkTower))
	delete pAtkTower;*/

	m_pDefCastle = CBuilding::CreateNormalBuilding(this, BUILDING_TYPE_DEFCASTLE, Force::DEF, GetWidth() - 300, nAtkNum);
	if (false == AddObject(m_pDefCastle))
		delete m_pDefCastle;
	else
	{
		for (auto it : m_arrMasters)
		{
			if (it && it->GetForce() == Force::DEF)
			{
				m_pDefCastle->AddMasterCasletUpgrade(it->GetCastleHPEx());
				break;
			}
		}
	}

	/*CBuilding *pDefTower = CBuilding::CreateNormalBuilding(this, BUILDING_TYPE_DEFTOWER, Force::DEF, GetWidth() - 1600, nAtkNum);
	if (false == AddObject(pDefTower))
	delete pDefTower;*/

	CBuilding *pCrystalTower = CBuilding::CreateNormalBuilding(this, BUILDING_TYPE_CRYSTAL_TOWER, Force::NEUTRAL, GetWidth() / 2);
	if (false == AddObject(pCrystalTower))
		delete pCrystalTower;
}

void CBattle::AddResource(Force enForce, int nNum, bool bAverage /*= false*/, bool is_crystal /* = false*/)
{
	if (Force::ATK == enForce)
	{
		if (bAverage)
			nNum /= GetMasterNum(Force::ATK, false, false);
		for (auto &it : m_arrMasters)
		{
			if (it && it->GetForce() == Force::ATK)
			{
				if (it->IsAI() && is_crystal)
					it->ChangeResource(-nNum / 2);
				else
					it->ChangeResource(nNum);
			}
		}
	}
	else if (Force::DEF == enForce)
	{
		if (bAverage)
			nNum /= GetMasterNum(Force::DEF, false);
		for (auto &it : m_arrMasters)
		{
			if (it && it->GetForce() == Force::DEF)
			{
				if (it->IsAI() && is_crystal)
					it->ChangeResource(-nNum / 2);
				else
					it->ChangeResource(nNum);
			}

		}
	}
}

int CBattle::GetMasterNum(Force enForce, bool bIncludeLeave /* = true */, bool bIncludeAI /*= true*/) const
{
	int nNum{ 0 };

	if (Force::ATK == enForce)
	{
		for (auto &it : m_arrMasters)
		{
			if (!it || it->GetForce() != Force::ATK)
				continue;
			if (false == bIncludeLeave && it->IsLeave())
				continue;
			if (false == bIncludeAI && it->IsAI())
				continue;
			nNum++;
		}
	}
	else if (Force::DEF == enForce)
	{
		for (auto &it : m_arrMasters)
		{
			if (!it || it->GetForce() != Force::DEF)
				continue;
			if (false == bIncludeLeave && it->IsLeave())
				continue;
			if (false == bIncludeAI && it->IsAI())
				continue;
			nNum++;
		}
	}

	return nNum;
}

int	CBattle::GetMasterNum() const
{
	int nNum{ 0 };

	for (auto &it : m_arrMasters)
		if (it)
			nNum++;

	return nNum;
}

int	CBattle::GetBulidingNum() const
{
	return m_mapAtkBuildings.size() + m_mapDefBuildings.size() + m_mapNeutralBuildings.size();
}

int CBattle::GetBulidingNum(Force enForce) const
{
	switch (enForce)
	{
	case Force::ATK:return m_mapAtkBuildings.size();
	case Force::DEF:return m_mapDefBuildings.size();
	case Force::NEUTRAL:return m_mapNeutralBuildings.size();
	default:return 0;
	}
}

int	CBattle::GetUnitNum(bool bIncludeDead/*= true*/) const
{
	int nNum = m_mapAtkUnits.size() + m_mapDefUnits.size();

	if (bIncludeDead)
		nNum += m_mapDeadUnits.size();

	return nNum;
}

int	CBattle::GetUnitNum(Force enForce) const
{
	if (Force::ATK == enForce)
		return m_mapAtkUnits.size();
	else if (Force::DEF == enForce)
		return m_mapDefUnits.size();

	return 0;
}

float CBattle::GetRangeUnitRatio(Force enForce) const
{
	const std::map<int, CUnit*>* nList{ nullptr };

	if (Force::ATK == enForce)
		nList = &m_mapAtkUnits;
	else if (Force::DEF == enForce)
		nList = &m_mapDefUnits;

	if (!nList)
		return 0;

	int nAllNum{ 0 };
	int nRangeNum{ 0 };

	for (auto &it : *nList)
	{
		nAllNum++;
		if (it.second->IsRangeUnit())
			nRangeNum++;
	}

	if (0 == nAllNum)
		return 0;

	return (nRangeNum * 1.0f) / nAllNum;
}

std::map<int, CUnit*>* CBattle::GetUnitList(Force enForce)
{
	if (Force::ATK == enForce)
		return &m_mapAtkUnits;
	else if (Force::DEF == enForce)
		return &m_mapDefUnits;

	return nullptr;
}

std::map<int, CUnit*>* CBattle::GetPlayerUnitList()
{
	m_maSPPlayerUnits.clear();
	for (auto &it : m_mapAtkUnits)
	{
		if (it.second->GetMaster() && !it.second->GetMaster()->IsAI())
		{
			m_maSPPlayerUnits.insert(std::make_pair(it.first, it.second));
		}
	}
	return &m_maSPPlayerUnits;
}

std::map<int, CUnit*>* CBattle::GetPlayerHeroUnitList()
{
	m_maSPPlayerHeroUnits.clear();
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetMaster() &&
			!it.second->GetMaster()->IsAI() &&
			it.second->IsHero())
		{
			m_maSPPlayerHeroUnits.insert(make_pair(it.first, it.second));
		}
	}
	return &m_maSPPlayerHeroUnits;
}

std::map<int, CBuilding*>* CBattle::GetBulidingList(Force enForce)
{
	if (Force::ATK == enForce)
	{
		return &m_mapAtkBuildings;
	}
	else if (Force::DEF == enForce)
	{
		return &m_mapDefBuildings;
	}
	else if (Force::NEUTRAL == enForce)
	{
		return &m_mapNeutralBuildings;
	}

	return nullptr;
}

void CBattle::SendBattleData()
{
	pto_BATTLE_S2C_NTF_SendBattleGroundData pto;
	pto.set_nsceneid(m_nSceneID);
	pto.set_mode(int(GetBattleMode()));
	pto.set_nwidth(GetWidth());
	pto.set_atk_base_gun(atk_base_gun_);
	pto.set_def_base_gun(def_base_gun_);

	SendStageData(&pto);

	if (BattleMode::STAGE != GetBattleMode() &&
		BattleMode::ELITESTAGE != GetBattleMode() &&
		BattleMode::SPEED != GetBattleMode() &&
		BattleMode::MULTISTAGE != GetBattleMode())
	{
		if (m_pAtkCastle)
		{
			pto.set_nattackforcecastlehp(m_pAtkCastle->GetHP());
			pto.set_nattackforcecastlemaxhp(m_pAtkCastle->GetMaxHP());
		}
		if (m_pDefCastle)
		{
			pto.set_ndefensforcecastlehp(m_pDefCastle->GetHP());
			pto.set_ndefensforcecastlemaxhp(m_pDefCastle->GetMaxHP());
		}
	}

	//召唤师数据
	for (auto &it : m_arrMasters)
		if (it && it->GetForce() == Force::ATK)
			it->SetProtocol(pto.add_vctattackmasters());

	for (auto &it : m_arrMasters)
		if (it && it->GetForce() == Force::DEF)
			it->SetProtocol(pto.add_vctdefensmasters());

	//建筑数据
	for (auto &it : m_mapAtkBuildings)
		it.second->SetProtocol(pto.add_vctattackbuildings());
	for (auto &it : m_mapDefBuildings)
		it.second->SetProtocol(pto.add_vctdefensbuildings());
	for (auto &it : m_mapNeutralBuildings)
		it.second->SetProtocol(pto.add_vctneutralbuildings());

	for (auto &it : m_mapGates)
		it.second->SetProtocol(pto.add_gates());

	for (auto &it : m_setBullets)
		it->SetBulletProtocol(pto.add_bullet());

	std::string strPto;
	pto.SerializeToString(&strPto);
	SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_SendData);
}

CMaster* CBattle::FindMasterByMID(int nMasterID) const
{
	if (nMasterID > (int)m_arrMasters.size() || nMasterID < 1)
		return nullptr;

	return m_arrMasters[nMasterID - 1];
}

CMaster* CBattle::FindMasterByPID(int nPID) const
{
	for (auto &it : m_arrMasters)
		if (it && it->GetPID() == nPID)
			return it;

	return nullptr;
}

CBattleObject*	CBattle::FindEnemy(CUnit* pUnit, bool bHasAlertRange /*= false*/) const
{
	const map<int, CUnit*>*		mapUnit{ nullptr };
	const map<int, CBuilding*>*	mapBuilding{ nullptr };

	if (Force::ATK == pUnit->GetForce())
	{
		mapUnit = &m_mapDefUnits;
		mapBuilding = &m_mapDefBuildings;
	}
	else if (Force::DEF == pUnit->GetForce())
	{
		mapUnit = &m_mapAtkUnits;
		mapBuilding = &m_mapAtkBuildings;
	}

	float fDistance{ 0 };
	CBattleObject* pObject{ nullptr };

	for (auto &it : *mapUnit)
	{
		if (it.second->IsHiding() || it.second->IsDead())
			continue;

		if (pUnit->IsHiding() && !it.second->IsSeeHide())
		{
			switch (pUnit->GetHideAtkMode())
			{
			case TT_RANGE:
				if (false == it.second->IsRangeUnit() || it.second->IsHero())
					continue;
				break;
			case TT_HERO:
				if (false == it.second->IsHero())
					continue;
				break;
			case TT_BUILDING:
				goto point1;
			}
		}
		else if (pUnit->IsSpecialMode())
		{
			switch (pUnit->GetSpecialAtkMode())
			{
			case TT_RANGE:
				if (false == it.second->IsRangeUnit() || it.second->IsHero())
					continue;
				break;
			case TT_HERO:
				if (false == it.second->IsHero())
					continue;
				break;
			case TT_BUILDING:
				goto point1;
			}
		}
		else
		{
			switch (pUnit->GetNormalAtkMode())
			{
			case TT_RANGE:
				if ((false == it.second->IsRangeUnit() || it.second->IsHero()) && !it.second->IsSeeHide())
					continue;
				break;
			case TT_HERO:
				if (false == it.second->IsHero() && !it.second->IsSeeHide())
					continue;
				break;
			case TT_BUILDING:
				goto point1;
			}
		}

		float fDis = CalculateDistance(pUnit, it.second, true, bHasAlertRange);

		if (fDis > 0 && fDis > fDistance)
		{
			fDistance = fDis;
			pObject = it.second;
		}
	}

point1:
	for (auto &it : *mapBuilding)
	{
		if (BUILDING_STATE_DESTROYED == it.second->GetBuildingState())
			continue;

		float fDis = CalculateDistance(pUnit, it.second, true, bHasAlertRange);

		if (fDis > 0 && fDis > fDistance)
		{
			fDistance = fDis;
			pObject = it.second;
		}
	}

	return pObject;
}

CBattleObject*	CBattle::FindEnemyInRange(float x1, float x2, Force enForce) const
{
	float f1 = min(x1, x2);
	float f2 = max(x1, x2);

	const map<int, CUnit*>*		mapUnit{ nullptr };
	const map<int, CBuilding*>*	mapBuilding{ nullptr };

	if (Force::ATK == enForce)
	{
		mapUnit = &m_mapDefUnits;
		mapBuilding = &m_mapDefBuildings;
	}
	else if (Force::DEF == enForce)
	{
		mapUnit = &m_mapAtkUnits;
		mapBuilding = &m_mapAtkBuildings;
	}

	float fDistance{ 0 };
	CBattleObject* pObject{ nullptr };

	for (auto &it : *mapUnit)
	{
		if (it.second->IsHiding())
			continue;

		if (0 != IntersectionLine(f1, f2, it.second->GetPos() - it.second->GetVolume(), it.second->GetPos() + it.second->GetVolume()))
		{
			if (Force::ATK == enForce &&
				it.second->GetPos() > x1)
			{
				return it.second;
			}

			else if (Force::DEF == enForce &&
				it.second->GetPos() < x2)
			{
				return it.second;
			}
		}
	}
	for (auto &it : *mapBuilding)
	{
		if (0 != IntersectionLine(f1, f2, it.second->GetPos() - it.second->GetVolume(), it.second->GetPos() + it.second->GetVolume()))
			return it.second;
	}

	return nullptr;
}

float CBattle::FindSoldiersLinePos(Force enForce, bool bIncludeHide /*= false*/, bool bIncludeHero/*=false*/, bool bIncludeUnit/*= false*/) const
{
	float fPos{ -1 };

	const map<int, CUnit*>*		pMapUnits{ nullptr };
	const map<int, CBuilding*>*	pMapBuildings{ nullptr };

	if (Force::ATK == enForce)
	{
		fPos = 1000;
		pMapUnits = &m_mapAtkUnits;
		pMapBuildings = &m_mapAtkBuildings;
	}
	else if (Force::DEF == enForce)
	{
		fPos = GetWidth() - 1000;
		pMapUnits = &m_mapDefUnits;
		pMapBuildings = &m_mapDefBuildings;
	}

	for (auto &it : *pMapUnits)
	{
		if (false == bIncludeHero && it.second->IsHero())
			continue;

		if (false == bIncludeHide && it.second->IsHiding())
			continue;

		if (false == bIncludeUnit && !it.second->IsHero())
			continue;

		float fUintPos{ 0 };

		if (Force::ATK == enForce)
		{
			fUintPos = it.second->GetPos() + it.second->GetVolume();
			fPos = fPos > fUintPos ? fPos : fUintPos;
		}
		else if (Force::DEF == enForce)
		{
			fUintPos = it.second->GetPos() - it.second->GetVolume();
			fPos = fPos < fUintPos ? fPos : fUintPos;
		}

	}
	for (auto &it : *pMapBuildings)
	{
		if (BUILDING_STATE_DESTROYED == it.second->GetBuildingState())
			continue;

		float fbuidligPos{ 0 };

		if (Force::ATK == enForce)
		{
			fbuidligPos = it.second->GetPos() + it.second->GetVolume();
			fPos = fPos > fbuidligPos ? fPos : fbuidligPos;
		}
		else if (Force::DEF == enForce)
		{
			fbuidligPos = it.second->GetPos() - it.second->GetVolume();
			fPos = fPos < fbuidligPos ? fPos : fbuidligPos;
		}
	}

	if (Force::ATK == enForce)
	{
		fPos += 100;
	}
	else if (Force::DEF == enForce)
	{
		fPos -= 100;
	}

	return fPos;
}

float CBattle::FindBuidlingLinePos(Force enForce) const
{
	float fPos{ 0 };
	const std::map<int, CBuilding*>*	pMapBuildings{ nullptr };

	if (Force::ATK == enForce)
	{
		pMapBuildings = &m_mapAtkBuildings;
	}
	else if (Force::DEF == enForce)
	{
		fPos = GetWidth();
		pMapBuildings = &m_mapDefBuildings;
	}

	for (auto &it : *pMapBuildings)
	{
		if (BUILDING_STATE_DESTROYED == it.second->GetBuildingState())
			continue;

		float fbuidligPos{ 0 };

		if (Force::ATK == enForce)
		{
			fbuidligPos = it.second->GetPos() + it.second->GetVolume();
			fPos = fPos > fbuidligPos ? fPos : fbuidligPos;
		}
		else if (Force::DEF == enForce)
		{
			fbuidligPos = it.second->GetPos() - it.second->GetVolume();
			fPos = fPos < fbuidligPos ? fPos : fbuidligPos;
		}
	}

	return fPos;
}

float CBattle::FindHeroLinePos(Force enForce) const
{
	float fPos{ -1 };

	const map<int, CUnit*>*		pMapUnits{ nullptr };

	if (Force::ATK == enForce)
	{
		fPos = 0;
		pMapUnits = &m_mapAtkUnits;
	}
	else if (Force::DEF == enForce)
	{
		fPos = GetWidth();
		pMapUnits = &m_mapDefUnits;
	}

	for (auto &it : *pMapUnits)
	{
		if (!it.second->IsHero())
			continue;

		float fUintPos{ 0 };

		if (Force::ATK == enForce)
		{
			fUintPos = it.second->GetPos() + it.second->GetVolume();
			fPos = fPos > fUintPos ? fPos : fUintPos;
		}
		else if (Force::DEF == enForce)
		{
			fUintPos = it.second->GetPos() - it.second->GetVolume();
			fPos = fPos < fUintPos ? fPos : fUintPos;
		}

	}
	return fPos;
}

float CBattle::FindSkillHeroLinePos(CHero* pHero) const
{
	float fPos{ -1 };

	const map<int, CUnit*>*		pMapUnits{ nullptr };

	Force enForce;
	if (pHero->GetForce() == Force::ATK)
		enForce = Force::DEF;
	else if (pHero->GetForce() == Force::DEF)
		enForce = Force::ATK;

	if (Force::ATK == enForce)
	{
		pMapUnits = &m_mapAtkUnits;
	}
	else if (Force::DEF == enForce)
	{
		fPos = GetWidth();
		pMapUnits = &m_mapDefUnits;
	}

	for (auto &it : *pMapUnits)
	{
		if (!it.second->IsHero())
			continue;

		if (it.second->IsHiding())
			continue;

		float fUnitPos{ 0 };

		if (Force::ATK == enForce)
		{
			fUnitPos = it.second->GetPos() + it.second->GetVolume();
			if (fPos < fUnitPos && fUnitPos < pHero->GetPos())
			{
				fPos = fUnitPos;
			}
		}
		else if (Force::DEF == enForce)
		{
			fUnitPos = it.second->GetPos() - it.second->GetVolume();
			if (fPos > fUnitPos && fUnitPos > pHero->GetPos())
			{
				fPos = fUnitPos;
			}
		}
	}

	return fPos;
}

float CBattle::FindSkillSoldierLinePos(CHero* pHero) const
{
	float fPos{ -1 };

	const map<int, CUnit*>*		pMapUnits{ nullptr };

	Force enForce;
	if (pHero->GetForce() == Force::ATK)
		enForce = Force::DEF;
	else if (pHero->GetForce() == Force::DEF)
		enForce = Force::ATK;

	if (Force::ATK == enForce)
	{
		pMapUnits = &m_mapAtkUnits;
	}
	else if (Force::DEF == enForce)
	{
		fPos = GetWidth();
		pMapUnits = &m_mapDefUnits;
	}

	for (auto &it : *pMapUnits)
	{
		if (it.second->IsHiding())
			continue;

		float fUnitPos{ 0 };

		if (Force::ATK == enForce)
		{
			fUnitPos = it.second->GetPos() + it.second->GetVolume();
			if (fPos < fUnitPos && fUnitPos < pHero->GetPos())
			{
				fPos = fUnitPos;
			}
		}
		else if (Force::DEF == enForce)
		{
			fUnitPos = it.second->GetPos() - it.second->GetVolume();
			if (fPos > fUnitPos && fUnitPos > pHero->GetPos())
			{
				fPos = fUnitPos;
			}
		}
	}

	return fPos;
}

float CBattle::FindSkillBulidingLinePos(CHero* pHero) const
{
	float fPos{ -1 };

	const map<int, CBuilding*>*	pMapBuildings{ nullptr };

	Force enForce;
	if (pHero->GetForce() == Force::ATK)
		enForce = Force::DEF;
	else if (pHero->GetForce() == Force::DEF)
		enForce = Force::ATK;

	if (Force::ATK == enForce)
	{
		pMapBuildings = &m_mapAtkBuildings;
	}
	else if (Force::DEF == enForce)
	{
		fPos = GetWidth();
		pMapBuildings = &m_mapDefBuildings;
	}

	for (auto &it : *pMapBuildings)
	{
		if (BUILDING_STATE_DESTROYED == it.second->GetBuildingState())
			continue;

		float fbuidligPos{ 0 };

		if (Force::ATK == enForce)
		{
			fbuidligPos = it.second->GetPos() - it.second->GetVolume();
			if (fPos < fbuidligPos && fbuidligPos < pHero->GetPos())
			{
				fPos = fbuidligPos;
			}
		}


		else if (Force::DEF == enForce)
		{
			fbuidligPos = it.second->GetPos() - it.second->GetVolume();
			if (fPos > fbuidligPos && fbuidligPos > pHero->GetPos())
			{
				fPos = fbuidligPos;
			}
		}
	}

	return fPos;
}

CMaster* CBattle::FindGetMostResourceMaster(int &nNum) const
{
	CMaster* pMaster{ nullptr };

	nNum = 0;

	for (auto &it : m_arrMasters)
	{
		if (!it)
			continue;

		if (it->GetAllResource() > nNum)
		{
			nNum = it->GetAllResource();
			pMaster = it;
		}
		else if (pMaster && it->GetAllResource() == nNum  && it->GetAllMakeDamage() > pMaster->GetAllMakeDamage())
		{
			nNum = it->GetAllResource();
			pMaster = it;
		}
	}

	return pMaster;
}

CMaster* CBattle::FindHitMostSoldierMaster(int &nNum) const
{
	CMaster* pMaster{ nullptr };

	nNum = 0;

	for (auto &it : m_arrMasters)
	{
		if (!it)
			continue;

		if (it->GetAllHitSoldier() > nNum)
		{
			nNum = it->GetAllHitSoldier();
			pMaster = it;
		}
		else if (pMaster && it->GetAllHitSoldier() == nNum  && it->GetAllMakeDamage() > pMaster->GetAllMakeDamage())
		{
			nNum = it->GetAllHitSoldier();
			pMaster = it;
		}
	}

	return pMaster;
}

CMaster* CBattle::FindKillMostHeroesMaster(int &nNum) const
{
	CMaster* pMaster{ nullptr };

	nNum = 0;

	for (auto &it : m_arrMasters)
	{
		if (!it)
			continue;

		if (it->GetAllKillHeroes() > nNum)
		{
			nNum = it->GetAllKillHeroes();
			pMaster = it;
		}
		else if (pMaster && it->GetAllKillHeroes() == nNum  && it->GetAllMakeDamage() > pMaster->GetAllMakeDamage())
		{
			nNum = it->GetAllKillHeroes();
			pMaster = it;
		}
	}

	return pMaster;
}

CMaster* CBattle::FindKillMostSoldiersMaster(int &nNum) const
{
	CMaster* pMaster{ nullptr };

	nNum = 0;

	for (auto &it : m_arrMasters)
	{
		if (!it)
			continue;

		if (it->GetAllKillSoldiers() > nNum)
		{
			nNum = it->GetAllKillSoldiers();
			pMaster = it;
		}
		else if (pMaster && it->GetAllKillSoldiers() == nNum  && it->GetAllMakeDamage() > pMaster->GetAllMakeDamage())
		{
			nNum = it->GetAllKillSoldiers();
			pMaster = it;
		}
	}

	return pMaster;
}

CMaster* CBattle::FindGetMostCrystalMaster(int &nNum) const
{
	CMaster* pMaster{ nullptr };

	nNum = 0;

	for (auto &it : m_arrMasters)
	{
		if (!it)
			continue;

		if (it->GetAllGetCrystal() > nNum)
		{
			nNum = it->GetAllGetCrystal();
			pMaster = it;
		}
		else if (pMaster && it->GetAllGetCrystal() == nNum  && it->GetAllMakeDamage() > pMaster->GetAllMakeDamage())
		{
			nNum = it->GetAllGetCrystal();
			pMaster = it;
		}
	}

	return pMaster;
}

CMaster* CBattle::FindMakeMostDamageMaster(Force enForce, int &nNum) const
{
	CMaster* pMaster{ nullptr };

	nNum = 0;

	for (auto &it : m_arrMasters)
	{
		if (it && enForce == it->GetForce())
		{
			if (it->GetAllMakeDamage() > nNum)
			{
				nNum = it->GetAllMakeDamage();
				pMaster = it;
			}
		}
	}

	return pMaster;
}

int CBattle::GetAverageDamage(Force enForce) const
{
	int nNum{ 0 };
	int nCount{ 0 };

	for (auto &it : m_arrMasters)
	{
		if (it)
		{
			nNum += it->GetAllMakeDamage();
			nCount++;
		}
	}

	if (0 == nCount)
		return 0;

	return nNum / nCount;
}

void CBattle::GameOver(Force enLoseForce, float fPos)
{
	if (m_enState == BtSt_Over)
		return;
	GetRemainStageLoot();
	SetBattleState(BtSt_Over);

	m_enLoseForce = enLoseForce;

	int nTempTime = m_nTimes / 1000;

#ifdef _TEST_
	cout << "战斗" << m_nID << "结束,战斗时间:" << nTempTime / 60 << "分" << nTempTime % 60 << "秒" << endl << endl;
#endif

	int nTemp{ 0 };
	CMaster* pKillHeroMaster = FindKillMostHeroesMaster(nTemp);
	CMaster* pKillSoldierMaster = FindKillMostSoldiersMaster(nTemp);
	CMaster* pGetResourceMaster = FindGetMostResourceMaster(nTemp);
	CMaster* pGetCrystalMaster = FindGetMostCrystalMaster(nTemp);
	CMaster* pAtkMostDamageMaster = FindMakeMostDamageMaster(Force::ATK, nTemp);
	CMaster* pDefMostDamageMaster = FindMakeMostDamageMaster(Force::DEF, nTemp);

	int nAtkAverageDamage = GetAverageDamage(Force::ATK);
	int nDefAverageDamage = GetAverageDamage(Force::DEF);

	pto_BATTLE_S2C_NTF_Game_Over ptoTemp;

	for (auto &it : m_arrMasters)
	{
		if (!it)
			continue;

		auto datPlayer = ptoTemp.add_player_date();
		datPlayer->set_master_id(it->GetMasterID());
		datPlayer->set_kill_soldier_num(it->GetAllKillSoldiers());
		datPlayer->set_kill_hero_num(it->GetAllKillHeroes());
		datPlayer->set_get_all_resource(it->GetAllResource());
		datPlayer->set_get_crystal_times(it->GetAllGetCrystal());
		datPlayer->set_pid(it->GetPID());
		if (it->GetForce() == enLoseForce)
			datPlayer->set_is_win(false);
		else
			datPlayer->set_is_win(true);
		datPlayer->set_is_ai(it->IsAI());
		datPlayer->set_server_id(it->GetSID());

		if (it == pKillHeroMaster)
			it->AddMark(1);
		if (it == pKillSoldierMaster)
			it->AddMark(2);
		if (it == pGetResourceMaster)
			it->AddMark(4);
		if (it == pGetCrystalMaster)
			it->AddMark(8);
		if (enLoseForce != it->GetForce())
		{
			if (it->GetForce() == Force::ATK && it == pAtkMostDamageMaster)
				it->AddMark(16);
			else if (it->GetForce() == Force::DEF && it == pDefMostDamageMaster)
				it->AddMark(16);
		}
		else
		{
			if (it->GetForce() == Force::ATK && it == pAtkMostDamageMaster)
				it->AddMark(32);
			else if (it->GetForce() == Force::DEF && it == pDefMostDamageMaster)
				it->AddMark(32);
		}
		datPlayer->set_player_mark(it->GetMark());
	}

	for (auto &it : m_arrMasters)
	{
		if (!it || it->IsAI())
			continue;

#ifdef _TEST_
		std::cout << "玩家" << it->GetName() << "的战绩:\t" << endl;
		std::cout << "击杀士兵数量:\t" << it->GetAllKillSoldiers() << endl;
		std::cout << "击杀英雄数量:\t" << it->GetAllKillHeroes() << endl;
		std::cout << "出击的士兵数量:\t" << it->GetAllHitSoldier() << endl;
		std::cout << "获得的总资源量:\t" << it->GetAllResource() << endl << endl;
#endif

		pto_BATTLE_S2C_NTF_Game_Over ptoGameOver;
		ptoGameOver.set_time(nTempTime);
		ptoGameOver.set_is_exception(false);

		float fScore{ 0 };

		int nOppositeAverageDamage{ 0 };

		if (it->GetForce() == Force::ATK)
			nOppositeAverageDamage = nDefAverageDamage;
		else if (it->GetForce() == Force::DEF)
			nOppositeAverageDamage = nAtkAverageDamage;

		fScore = (float)it->GetAllMakeDamage();
		if (nOppositeAverageDamage)
			fScore /= nOppositeAverageDamage;

		fScore += it->GetAllKillHeroes() *0.03f;
		fScore += it->GetMarkNum() * 0.05f;

		if (enLoseForce == it->GetForce())
		{
			ptoGameOver.set_is_win(false);
			ptoGameOver.set_stage_reward(false);
		}
		else
		{
			fScore += 0.1f;
			ptoGameOver.set_is_win(true);
			ptoGameOver.set_stage_reward(true);
			it->GetExtraBlueprint();
			ptoGameOver.set_box_num(m_nBoxNum);
			if (GetBattleMode() == BattleMode::MULTISTAGE && it->MultiStageClear())
			{
				ptoGameOver.set_stage_reward(false);
				ptoGameOver.set_box_num(0);
			}
			it->SetStageLootPto(&ptoGameOver);
		}

		if (Force::ATK == it->GetForce())
			ptoGameOver.set_enemy_point(m_nDefPoint);
		else
			ptoGameOver.set_enemy_point(m_nAtkPoint);

		for (int i = 0; i < ptoTemp.player_date_size(); i++)
		{
			auto datPlayer = ptoGameOver.add_player_date();
			datPlayer->set_master_id(ptoTemp.player_date(i).master_id());
			datPlayer->set_kill_soldier_num(ptoTemp.player_date(i).kill_soldier_num());
			datPlayer->set_kill_hero_num(ptoTemp.player_date(i).kill_hero_num());
			datPlayer->set_get_all_resource(ptoTemp.player_date(i).get_all_resource());
			datPlayer->set_get_crystal_times(ptoTemp.player_date(i).get_crystal_times());
			datPlayer->set_player_mark(ptoTemp.player_date(i).player_mark());
			datPlayer->set_pid(ptoTemp.player_date(i).pid());
			datPlayer->set_is_win(ptoTemp.player_date(i).is_win());
			datPlayer->set_is_ai(ptoTemp.player_date(i).is_ai());
			datPlayer->set_server_id(ptoTemp.player_date(i).server_id());
		}

		if (fScore > 1.5f)
			ptoGameOver.set_player_grade(0);
		else if (fScore > 1.25f)
			ptoGameOver.set_player_grade(1);
		else if (fScore < 0.7f)
			ptoGameOver.set_player_grade(3);
		else
			ptoGameOver.set_player_grade(2);

		ptoGameOver.set_pos((int)fPos);
		ptoGameOver.set_mode(int(m_enMode));
		ptoGameOver.set_id(m_nStageID);
		ptoGameOver.set_difficulty(m_nDifficulty);
		ptoGameOver.set_story_id(m_nEndStory);

		std::string str;
		ptoGameOver.SerializeToString(&str);

		SendToMaster(it, str, MSG_S2C, BATTLE_S2C_NTF_GameOver, true);
	}
}

CProtoPackage* CBattle::GetProtoPackage(Force enForce)
{
	if (Force::ATK == enForce)
	{
		return m_arrProtoPackages[0];
	}
	else if (Force::DEF == enForce)
	{
		return m_arrProtoPackages[1];
	}

	return nullptr;
}

bool CBattle::HitTestEnemyHero(CBattleObject *pObj) const
{
	if (pObj->HasAdvanceBuffEffect(BAE_SHUTTLE))
		return false;

	const map<int, CUnit*>* pList{ nullptr };

	if (pObj->GetForce() == Force::ATK)
		pList = &m_mapDefUnits;
	else if (pObj->GetForce() == Force::DEF)
		pList = &m_mapAtkUnits;

	if (!pList)
		return false;

	for (auto &it : *pList)
	{
		if (it.second->IsHero() || it.second->IsBreakHero())
			if (CalculateDistance(pObj, it.second, false, false) > 0)
				return true;
	}

	return false;
}

bool CBattle::HitTestEnemyBuilding(CBattleObject *pObj) const
{
	const std::map<int, CBuilding*>* pBuidlings{ nullptr };

	if (Force::ATK == pObj->GetForce())
		pBuidlings = &m_mapDefBuildings;
	else if (Force::DEF == pObj->GetForce())
		pBuidlings = &m_mapAtkBuildings;

	for (auto &it : *pBuidlings)
	{
		if (BUILDING_STATE_DESTROYED != it.second->GetBuildingState())
			if (CalculateDistance(pObj, it.second, false, false) > 0)
				return true;
	}

	return false;
}

int CBattle::HitTestWallBullet(CBattleObject *pObj) const
{
	for (auto &it : m_setWallBullets)
	{
		if (CalculateDistance(pObj, it, false, false) > 0)
		{
			if (pObj->GetPos() <= it->GetPos())
				return 1;
			else
				return 2;
		}
	}

	return 0;
}

int CBattle::GetSkillEnemyNum(float fRange, CHero* pHero) const
{
	int nResult = 0;

	const std::map<int, CUnit*>*		pMapUnits{ nullptr };

	Force enForce;
	if (pHero->GetForce() == Force::ATK)
		enForce = Force::DEF;
	else if (pHero->GetForce() == Force::DEF)
		enForce = Force::ATK;

	if (Force::ATK == enForce)
	{
		pMapUnits = &m_mapAtkUnits;
	}
	else if (Force::DEF == enForce)
	{
		pMapUnits = &m_mapDefUnits;
	}

	for (auto &it : *pMapUnits)
	{
		if (it.second->IsHiding())
			continue;

		float fUnitPos{ 0 };

		if (Force::ATK == enForce)
		{
			if (it.second->GetPos() >= pHero->GetPos() - fRange &&
				it.second->GetPos() < pHero->GetPos())
			{
				nResult++;
			}
		}
		else if (Force::DEF == enForce)
		{
			if (it.second->GetPos() <= pHero->GetPos() + fRange &&
				it.second->GetPos() > pHero->GetPos())
			{
				nResult++;
			}
		}
	}
	return nResult;
}

float CBattle::BreakHeroSkill(float fRange, CHero* pHero) const
{
	float fResult = -1;
	const std::map<int, CUnit*>*		pMapUnits{ nullptr };

	Force enForce;
	if (pHero->GetForce() == Force::ATK)
		enForce = Force::DEF;
	else if (pHero->GetForce() == Force::DEF)
		enForce = Force::ATK;

	if (Force::ATK == enForce)
		pMapUnits = &m_mapAtkUnits;
	else if (Force::DEF == enForce)
		pMapUnits = &m_mapDefUnits;

	for (auto &it : *pMapUnits)
	{
		if (!it.second->IsHero())
			continue;

		if (it.second->IsHiding())
			continue;

		float fUnitPos{ 0 };

		if (Force::ATK == enForce)
		{
			if (it.second->GetPos() >= pHero->GetPos() - fRange &&
				it.second->GetPos() < pHero->GetPos() &&
				((CHero*)it.second)->GetUsintSkill() != -1)
			{
				fResult = it.second->GetPos();
				return fResult;
			}
		}
		else if (Force::DEF == enForce)
		{
			if (it.second->GetPos() <= pHero->GetPos() - fRange &&
				it.second->GetPos() > pHero->GetPos() &&
				((CHero*)it.second)->GetUsintSkill() != -1)
			{
				fResult = it.second->GetPos();
				return fResult;
			}
		}
	}
	return fResult;
}

void CBattle::AddGate(const CGate* pGate)
{
	CGate* pTempGate = new CGate;
	pTempGate->SetGateByGate(pGate);
	m_mapGates.insert(std::make_pair(pTempGate->GetID(), pTempGate));
}

bool CBattle::HitTestGate(CBattleObject *pObj) const
{
	for (auto &it : m_mapGates)
		if (!it.second->IsOpen())
			if (CalculateGateDistance(pObj, it.second, false, false) > 0)
				return true;
	return false;
}

float CBattle::CalculateGateDistance(CBattleObject* pObj, CGate* pGate, bool bHasAtkRange, bool bHasAlertRange) const
{
	float x1 = 0;
	float x2 = 0;

	float y1 = pGate->GetPos() - GateVolume;
	float y2 = pGate->GetPos() + GateVolume;

	float fAtkRange{ 0 };
	float fAlertRange{ 0 };

	if (bHasAtkRange)
		fAtkRange = ((CUnit*)pObj)->GetAtkDistance();
	if (bHasAlertRange)
		fAlertRange = ((CUnit*)pObj)->GetActiveRange();

	float fRes = 0;
	if (pObj->GetDirection())
	{
		x1 = pObj->GetPos();
		x2 = pObj->GetPos() + pObj->GetVolume() + fAtkRange + fAlertRange;
		fRes = IntersectionLine(x1, x2, y1, y2);
		if (fRes > 0 && y2 < x2)
			return x2 - y1;
	}
	else
	{
		x1 = pObj->GetPos() - pObj->GetVolume() - fAtkRange - fAlertRange;
		x2 = pObj->GetPos();
		fRes = IntersectionLine(x1, x2, y1, y2);
		if (fRes > 0 && y1 > x1)
			return y2 - x1;
	}
	return fRes;
}

float CBattle::FindGateLinePos(Force enForce) const
{
	float fPos{ 0 };

	if (Force::ATK == enForce)
		fPos = m_fWidth;
	else if (Force::DEF == enForce)
		fPos = 0;

	for (auto &it : m_mapGates)
	{
		if (!it.second->IsOpen())
		{
			float fGatePos{ 0 };
			if (Force::ATK == enForce)
			{
				fGatePos = it.second->GetPos() - GateVolume;
				if (fPos > fGatePos)
				{
					fPos = fGatePos;
				}
			}
			else if (Force::DEF == enForce)
			{
				fGatePos = it.second->GetPos() + GateVolume;
				if (fPos < fGatePos)
				{
					fPos = fGatePos;
				}
			}
		}
	}
	return fPos;
}

float CBattle::GetGatePos(CUnit* pUnit) const
{
	float fPos{ -1 };

	if (Force::ATK == pUnit->GetForce())
		fPos = 0;
	else if (Force::DEF == pUnit->GetForce())
		fPos = m_fWidth;

	for (auto it : m_mapGates)
	{
		if (!it.second->IsOpen())
		{
			if (Force::ATK == pUnit->GetForce() &&
				fPos < it.second->GetPos())
			{
				fPos = it.second->GetPos();
			}

			else if (Force::DEF == pUnit->GetForce() &&
				fPos > it.second->GetPos())
			{
				fPos = it.second->GetPos();
			}
		}
	}
	return fPos;
}

float CBattle::FindLinePos(Force enForce, bool bIncludeHide, bool bIncludeHero, bool bIncludeUnit) const
{
	float fPos{ -1 };

	const std::map<int, CUnit*>*		pMapUnits{ nullptr };
	const std::map<int, CBuilding*>*	pMapBuildings{ nullptr };

	if (Force::ATK == enForce)
	{
		fPos = 0;
		pMapUnits = &m_mapAtkUnits;
		pMapBuildings = &m_mapAtkBuildings;
	}
	else if (Force::DEF == enForce)
	{
		fPos = GetWidth();
		pMapUnits = &m_mapDefUnits;
		pMapBuildings = &m_mapDefBuildings;
	}

	for (auto &it : *pMapUnits)
	{
		if (false == bIncludeHero && it.second->IsHero())
			continue;

		if (false == bIncludeHide && it.second->IsHiding())
			continue;

		if (false == bIncludeUnit && !it.second->IsHero())
			continue;

		float fUintPos{ 0 };

		if (Force::ATK == enForce)
		{
			fUintPos = it.second->GetPos() + it.second->GetVolume();
			fPos = fPos > fUintPos ? fPos : fUintPos;
		}
		else if (Force::DEF == enForce)
		{
			fUintPos = it.second->GetPos() - it.second->GetVolume();
			fPos = fPos < fUintPos ? fPos : fUintPos;
		}

	}
	for (auto &it : *pMapBuildings)
	{
		if (BUILDING_STATE_DESTROYED == it.second->GetBuildingState())
			continue;

		float fbuidligPos{ 0 };

		if (Force::ATK == enForce)
		{
			fbuidligPos = it.second->GetPos() + it.second->GetVolume();
			fPos = fPos > fbuidligPos ? fPos : fbuidligPos;
		}
		else if (Force::DEF == enForce)
		{
			fbuidligPos = it.second->GetPos() - it.second->GetVolume();
			fPos = fPos < fbuidligPos ? fPos : fbuidligPos;
		}
	}

	return fPos;
}

void CBattle::ProduceCustomBullet(const CCustomBullet* pCustomBullet, bool bSend)
{
	const CBulletTP* pBulletTP = CBulletTP::GetBulletTP(pCustomBullet->GetBulletID());
	if (nullptr == pBulletTP)
	{
		printf("未找到BulletTD\n");
		return;
	}

	CBullet *pBullet = new CBullet(this);

	pBullet->InitBullet(pBulletTP, pCustomBullet->GetForce(), (float)pCustomBullet->GetPos());
	pBullet->SetEffectValueEx(pCustomBullet->GetEffectValue());
	pBullet->SetInfinite(pCustomBullet->GetInfinite());
	pBullet->SetCustomID(pCustomBullet->GetCustomID());

	pBullet->SetID(GetBulletID());

	pair<set<CBullet*>::iterator, bool> it;

	if (BF_WALL == pBullet->GetBulletForm())
		it = m_setWallBullets.insert(pBullet);
	else
		it = m_setBullets.insert(pBullet);

	if (pBullet->GetEffect() == 9997)
	{
		if (pBullet->GetForce() == Force::ATK)
			m_pAtkWarpGate = pBullet;
		else if (pBullet->GetForce() == Force::DEF)
			m_pDefWarpGate = pBullet;
	}
}

float CBattle::GetWarpGate(Force enForce)
{
	float fPos{ 0 };

	if (Force::ATK == enForce)
	{
		fPos = 1;
		if (m_pAtkWarpGate)
			fPos = m_pAtkWarpGate->GetPos();

	}
	else if (Force::DEF == enForce)
	{
		fPos = m_fWidth - 1;
		if (m_pDefWarpGate)
			fPos = m_pDefWarpGate->GetPos();
	}
	return fPos;
}

void CBattle::ClearWarpGate(CBullet* pInstance)
{
	if (pInstance)
	{
		if (pInstance->GetForce() == Force::ATK)
		{
			if (m_pAtkWarpGate == pInstance)
				m_pAtkWarpGate = nullptr;
		}
		else if (pInstance->GetForce() == Force::DEF)
		{
			if (m_pDefWarpGate == pInstance)
				m_pDefWarpGate = nullptr;
		}
	}
}

void CBattle::Pause()
{
	if (m_enState == BtSt_Ready)
		m_bIsReadyBeforePause = true;
	SetBattleState(BtSt_Pause);
}

void CBattle::ProuduceBullet(float fPos, int nBulletID, int nBulletValu)
{
	const CBulletTP* pBulletTP = CBulletTP::GetBulletTP(nBulletID);
	if (nullptr == pBulletTP)
	{
		printf("未找到BulletTD\n");
		return;
	}

	CBullet *pBullet = new CBullet(this);

	pBullet->InitBullet(pBulletTP, Force::UNKNOWN, (float)fPos);
	pBullet->SetEffectValueEx(nBulletValu);
	pBullet->SetInfinite(false);

	AddObject(pBullet, true);
}

void CBattle::ProduceBaseGunBullet(CMaster* master)
{
	const CBulletTP* pBulletTP = CBulletTP::GetBulletTP(1000);
	if (nullptr == pBulletTP)
	{
		printf("未找到BulletTD\n");
		return;
	}

	float pos{ 0 };
	bool castle{ false };
	if (Force::ATK == master->GetForce())
	{
		for (auto it : m_mapAtkBuildings)
		{
			if (it.second->GetBuidlingType() == BuildingType::BUILDING_TYPE_ATKCASTLE)
			{
				pos = it.second->GetPos();
				castle = true;
			}
		}
	}

	if (Force::DEF == master->GetForce())
	{
		for (auto it : m_mapDefBuildings)
		{
			if (it.second->GetBuidlingType() == BuildingType::BUILDING_TYPE_DEFCASTLE)
			{
				pos = it.second->GetPos();
				castle = true;
			}
		}
	}
	if (!castle)
		return;

	CBullet *pBullet = new CBullet(this);
	pBullet->set_master(master);
	pBullet->InitBullet(pBulletTP, master->GetForce(), (float)pos);
	AddObject(pBullet, true);
}

int	CBattle::GetRangeFriendlyUnitNum(CUnit* pUnit, int nRange)
{
	int nResult{ 0 };
	const map<int, CUnit*>*		pMapUnits{ nullptr };
	float fPoint = pUnit->GetPos();

	if (Force::ATK == pUnit->GetForce())
		pMapUnits = &m_mapAtkUnits;
	else if (Force::DEF == pUnit->GetForce())
		pMapUnits = &m_mapDefUnits;

	for (auto &it : *pMapUnits)
	{
		if (it.second->GetPos() - fPoint <= nRange &&
			it.second->GetPos() - fPoint >= 0)
			nResult++;
		else if (fPoint - it.second->GetPos() <= nRange &&
			fPoint - it.second->GetPos() >= 0)
			nResult++;
	}

	return nResult;
}

bool CBattle::DefCastleHP(float remain_hp)
{
	if (m_pDefCastle)
	{
		if (m_pDefCastle->GetHP() <= m_pDefCastle->GetMaxHP() * remain_hp)
			return true;
	}
	return false;
}

bool CBattle::PrisonHP(Force force, float ramain_hp)
{
	for (auto it : m_mapAtkBuildings)
	{
		if (it.second->GetResourceID() == 4 && it.second->GetHP() <= it.second->GetMaxHP() * ramain_hp)
			return true;
	}

	for (auto it : m_mapDefBuildings)
	{
		if (it.second->GetResourceID() == 4 && it.second->GetHP() <= it.second->GetMaxHP() * ramain_hp)
			return true;
	}
	return false;
}

void CBattle::GetRemainStageLoot()
{
	if (m_enMode != BattleMode::STAGE &&
		m_enMode != BattleMode::ELITESTAGE)
		return;
	if (!m_arrMasters[0])
		return;

	for (auto it : m_setBullets)
	{
		it->RemainStageLoot(m_arrMasters[0]);
	}
}

bool CBattle::UnitHold(CUnit* unit)
{
	if (unit->GetForce() == Force::ATK)
	{
		for (auto it : m_mapDefUnits)
		{
			if (it.second->GetPos() > unit->GetPos() &&
				it.second->GetPos() < unit->GetPos() + unit->GetAtkDistance())
				return true;
		}

		for (auto it : m_mapDefBuildings)
		{
			if (it.second->GetHP() > 0 &&
				it.second->GetPos() > unit->GetPos() &&
				it.second->GetPos() < unit->GetPos() + unit->GetAtkDistance())
				return true;
		}

		return false;
	}

	if (unit->GetForce() == Force::DEF)
	{
		for (auto it : m_mapAtkUnits)
		{
			if (it.second->GetHP() > 0 &&
				it.second->GetPos() < unit->GetPos() &&
				it.second->GetPos() > unit->GetPos() - unit->GetAtkDistance())
				return true;
		}

		for (auto it : m_mapAtkBuildings)
		{
			if (it.second->GetPos() < unit->GetPos() &&
				it.second->GetPos() > unit->GetPos() - unit->GetAtkDistance())
				return true;
		}
		return false;
	}

	return false;
}

void CBattle::Loop()
{
	m_nTimes += BATTLE_LOOP_TIME;

	switch (GetBattleState())
	{
	case BtSt_NULL:
		if (TimeCounts(500))
			_CheckLoadProcess();
		break;
	case BtSt_Ready:
		if (m_nTimes >= m_nReadyTime)
		{
			SendToAllMaster("", MSG_S2C, BATTLE_S2C_NTF_BattleStart);
			m_nTimes = 0;
			m_bIsReadyBeforePause = false;
			SetBattleState(BtSt_Run);
		}
		break;
	case BtSt_Run:
		_Fighting();
		break;
	case BtSt_Pause:
		m_nTimes -= BATTLE_LOOP_TIME;
		break;
	case BtSt_Over:
		break;
	default:
		RECORD_WARNING("未知的战斗状态");
		break;
	}
}

void CBattle::_ClickHeroCard(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	pto_BATTLE_C2S_REQ_ClickHeroCard pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	pMaster->ClickHeroCard(pto.ncardindex(), (float)pto.pos());
}

void CBattle::_ClickSoldierCard(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	pto_BATTLE_C2S_REQ_ClickSoldierCard pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	pMaster->ClickSoldierCard(pto.ncardindex(), pto.pos());
}

void CBattle::_HeroMove(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Pause == pMaster->GetBattle()->GetBattleState())
	{
		CHero* pHero = pMaster->GetUsingHero();
		if (pHero)
			pHero->CancelHeroMove();
	}
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	pto_BATTLE_C2S_NTF_HeroMove pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CHero* pHero = pMaster->GetUsingHero();
	if (pHero)
		pHero->Move(pto.bdirection());
}

void CBattle::_HeroMoveTo(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Pause == pMaster->GetBattle()->GetBattleState())
	{
		CHero* pHero = pMaster->GetUsingHero();
		if (pHero)
			pHero->CancelHeroMove();
	}
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	pto_BATTLE_C2S_NTF_HeroMoveTo pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	//HeroStand已由此命令代替，判断坐标进行延迟同步处理，同时进行延迟修正广播

	CHero* pHero = pMaster->GetUsingHero();
	if (pHero)
		pHero->MoveTo(pto.nx());
}

void CBattle::_HeroStand(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	//此协议已作废

	CHero* pHero = pMaster->GetUsingHero();
	if (pHero)
		pHero->Stand();
}

void CBattle::_UseActiveSkill(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	pto_BATTLE_C2S_REQ_UseActiveSkill pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CHero* pHero = pMaster->GetUsingHero();
	if (pHero)
		pHero->UseSkill(pto.nindex(), pto.nposx(), pto.nx());
}

void CBattle::_UpgradeLevel(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	pMaster->UpgradeLevel();
}

void CBattle::_UseBaseGun(CMaster *pMaster, const CMessage& msg)
{
	if (BtSt_Run != pMaster->GetBattle()->GetBattleState())
		return;

	pMaster->UseBaseGun();
}

void CBattle::_Pause(CMaster *pMaster, bool bIsPause)
{
	CBattle *pBattle{ pMaster->GetBattle() };

	std::string strPto;
	int nProtocolID{ 0 };

	if (pBattle->GetBattleMode() == BattleMode::STAGE ||
		pBattle->GetBattleMode() == BattleMode::ELITESTAGE ||
		pBattle->GetBattleMode() == BattleMode::SPEED)
	{

		if (bIsPause)
		{
			pto_BATTLE_S2C_RES_Pause ptoPause;
			pBattle->Pause();

			for (auto &it : pBattle->m_arrMasters)
			{
				if (it)
				{
					CHero* pHero = it->GetUsingHero();
					if (pHero)
					{
						pHero->StopMoveWhenPause();
						pHero->CancelHeroMove();
					}
				}
			}

			ptoPause.set_res(0);

			nProtocolID = BATTLE_S2C_RES_Pause;
			ptoPause.SerializeToString(&strPto);
		}
		else
		{
			pto_BATTLE_S2C_RES_Continue ptoContinue;

			if (pBattle->m_bIsReadyBeforePause)
			{
				//pBattle->SendToAllMaster("", MSG_S2C, BATTLE_S2C_NTF_BattleStart);
				//pBattle->m_nTimes = 0;
				//pBattle->m_bIsReadyBeforePause = false;
				pBattle->SetBattleState(BtSt_Ready);
			}

			else
			{
				pBattle->SetBattleState(BtSt_Run);

				ptoContinue.set_res(0);

				nProtocolID = BATTLE_S2C_RES_Continue;
				ptoContinue.SerializeToString(&strPto);
			}
		}

		pBattle->SendToMaster(pMaster, strPto, MSG_S2C, nProtocolID);
	}
}

bool CBattle::_CheckMaster()
{
	if (0 == GetMasterNum(Force::ATK, false, false))
	{
		GameOver(Force::ATK, GetWidth() / 2);
		return true;
	}
	else if (0 == GetMasterNum(Force::DEF, false, true))
	{
		GameOver(Force::DEF, GetWidth() / 2);
		return true;
	}

	return false;
}

void CBattle::_CheckLoadProcess()
{
	pto_BATTLE_S2C_NTF_LoadProcess pto;

	bool bResult{ true };
	for (auto &it : m_maSPPlayerLoadProcess)
	{
		pto.add_mid(it.first);
		pto.add_process(it.second);
		if (100 != it.second)
			bResult = false;
	}
	pto.set_result(bResult);

	std::string strPto;
	pto.SerializeToString(&strPto);
	SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_LoadProcess);

	if (bResult)
	{
		SetBattleState(BtSt_Ready);
		m_nTimes = 0;
		_StartStory();
	}

	else if (m_nTimes >= 20000)
	{
		for (auto &it : m_maSPPlayerLoadProcess)
		{
			pto.add_mid(it.first);
			pto.add_process(100);
		}
		pto.set_result(true);

		std::string strPto;
		pto.SerializeToString(&strPto);
		SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_LoadProcess);

		SetBattleState(BtSt_Ready);
		m_nTimes = 0;
		_StartStory();
	}
}

void CBattle::_LoadProcess(CMaster *pMaster, const CMessage& msg)
{
	pto_BATTLE_C2S_NTF_LoadProcess pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CBattle* pBattle = pMaster->GetBattle();
	if (pBattle)
		pBattle->m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = pto.process();
}

void CBattle::_Fighting()
{
	if (BattleMode::STAGE != GetBattleMode() &&
		BattleMode::ELITESTAGE != GetBattleMode() &&
		BattleMode::SPEED != GetBattleMode() &&
		BattleMode::MULTISTAGE != GetBattleMode())
	{
		if (false == m_bDeadTime && m_nTimes > 240000)
		{
			m_bDeadTime = true;
			pto_BATTLE_S2C_NTF_DeathTime pto;
			pto.set_atk(true);
			pto.set_def(true);
			std::string strPto;
			pto.SerializeToString(&strPto);
			SendToAllMaster(strPto, MSG_S2C, BATTLE_S2C_NTF_DeathTime);
		}

		if (m_bDeadTime && TimeCounts(3000))
		{
			for (auto &it : m_mapAtkBuildings)
				if (it.second->GetBuidlingType() == BUILDING_TYPE_ATKCASTLE)
					it.second->BeDamaged(50);

			for (auto &it : m_mapDefBuildings)
				if (it.second->GetBuidlingType() == BUILDING_TYPE_DEFCASTLE)
					it.second->BeDamaged(50);
		}
	}

	for (auto &it : m_arrMasters)
		if (it)
			it->Loop();

	for (auto &it : m_mapAtkBuildings)
		it.second->Loop();
	for (auto &it : m_mapDefBuildings)
		it.second->Loop();
	for (auto &it : m_mapNeutralBuildings)
		it.second->Loop();

	for (auto &it : m_mapTimer)
		it.second += BATTLE_LOOP_TIME;

	_UnitLoop();
	_BulletLoop();

	if (TimeCounts(100))
		_EventLoop();

	if (TimeCounts(20))
		_SendProtoPackage();
}

void CBattle::_UnitLoop()
{
	for (auto it = m_mapAtkUnits.begin(); it != m_mapAtkUnits.end();)
	{
		if (it->second->IsDead())
		{
			m_mapDeadUnits.insert(std::make_pair(it->second->GetID(), it->second));
			it = m_mapAtkUnits.erase(it);
		}
		else
		{
			it->second->Loop();
			++it;
		}
	}
	for (auto it = m_mapDefUnits.begin(); it != m_mapDefUnits.end();)
	{
		if (it->second->IsDead())
		{
			m_mapDeadUnits.insert(std::make_pair(it->second->GetID(), it->second));
			it = m_mapDefUnits.erase(it);
		}
		else
		{
			it->second->Loop();
			++it;
		}
	}
}

void CBattle::_BulletLoop()
{
	std::vector<std::set<CBullet*>*> vctList;
	vctList.push_back(&m_setBullets);
	vctList.push_back(&m_setWallBullets);

	for (auto &itList : vctList)
	{
		for (auto it = itList->cbegin(); it != itList->cend();)
		{
			if ((*it)->IsDead())
			{
				delete (*it);
				it = itList->erase(it);
			}
			else
			{
				(*it)->Loop();
				++it;
			}
		}
	}
}

void CBattle::_SendProtoPackage()
{
	if (m_arrProtoPackages[0]->IsNull() == false)
		SendToForce(Force::ATK, m_arrProtoPackages[0]->GetMsgStr(), MSG_S2C, BATTLE_S2C_NTF_PerPackage);

	if (m_arrProtoPackages[1]->IsNull() == false)
		SendToForce(Force::DEF, m_arrProtoPackages[1]->GetMsgStr(), MSG_S2C, BATTLE_S2C_NTF_PerPackage);

	for (auto &it : m_arrProtoPackages)
	{
		it->Clear();
		it->NewPackage();
	}
}

void CBattle::_Surrender(CMaster *pMaster)
{
	CBattle* pBattle{ pMaster->GetBattle() };

	if (pBattle)
	{
		pto_BATTLE_S2C_NTF_Surrender pto_ntf;
		pto_ntf.set_master_id(pMaster->GetMasterID());

		std::string str_pto;
		pto_ntf.SerializeToString(&str_pto);
		pBattle->SendToAllMaster(str_pto, MSG_S2C, BATTLE_S2C_NTF_Surrender);
		//pBattle->_MasterSurrender(pMaster);
		pBattle->GameOver(pMaster->GetForce(), pBattle->m_fGameOverPos);
		//_MasterLeave(pMaster);
	}
}

void CBattle::_MasterLeave(CMaster *pMaster)
{
	pMaster->SetIsLeave(true);

	pMaster->GetBattle()->_CheckMaster();

	{
		pto_BATTLE_S2C_NTF_MasterLeave pto_ntf;
		pto_ntf.set_master_id(pMaster->GetMasterID());
		std::string str_pto;
		pto_ntf.SerializeToString(&str_pto);
		pMaster->GetBattle()->SendToAllMaster(str_pto, MSG_S2C, BATTLE_S2C_NTF_MasterLeave);
	}

	{
		pto_BATTLE_S2C_NTF_MasterDisappear pto_ntf;
		pto_ntf.set_master_id(pMaster->GetMasterID());
		std::string str_pto;
		pto_ntf.SerializeToString(&str_pto);
		pMaster->GetBattle()->SendToAllMaster(str_pto, MSG_S2C, BATTLE_S2C_NTF_MasterDisappear);
	}
}

void CBattle::_MasterSurrender(CMaster* master)
{
	m_enLoseForce = master->GetForce();

	int nTempTime = m_nTimes / 1000;

#ifdef _TEST_
	cout << "战斗" << m_nID << "结束,战斗时间:" << nTempTime / 60 << "分" << nTempTime % 60 << "秒" << endl << endl;
#endif

	int nTemp{ 0 };
	CMaster* pKillHeroMaster = FindKillMostHeroesMaster(nTemp);
	CMaster* pKillSoldierMaster = FindKillMostSoldiersMaster(nTemp);
	CMaster* pGetResourceMaster = FindGetMostResourceMaster(nTemp);
	CMaster* pGetCrystalMaster = FindGetMostCrystalMaster(nTemp);
	CMaster* pAtkMostDamageMaster = FindMakeMostDamageMaster(Force::ATK, nTemp);
	CMaster* pDefMostDamageMaster = FindMakeMostDamageMaster(Force::DEF, nTemp);

	int nAtkAverageDamage = GetAverageDamage(Force::ATK);
	int nDefAverageDamage = GetAverageDamage(Force::DEF);

	pto_BATTLE_S2C_NTF_Game_Over ptoTemp;

	for (auto &it : m_arrMasters)
	{
		if (!it)
			continue;

		auto datPlayer = ptoTemp.add_player_date();
		datPlayer->set_master_id(it->GetMasterID());
		datPlayer->set_kill_soldier_num(it->GetAllKillSoldiers());
		datPlayer->set_kill_hero_num(it->GetAllKillHeroes());
		datPlayer->set_get_all_resource(it->GetAllResource());
		datPlayer->set_get_crystal_times(it->GetAllGetCrystal());
		datPlayer->set_pid(it->GetPID());
		if (it->GetForce() == m_enLoseForce)
			datPlayer->set_is_win(false);
		else
			datPlayer->set_is_win(true);

		if (it == pKillHeroMaster)
			it->AddMark(1);
		if (it == pKillSoldierMaster)
			it->AddMark(2);
		if (it == pGetResourceMaster)
			it->AddMark(4);
		if (it == pGetCrystalMaster)
			it->AddMark(8);
		if (m_enLoseForce != it->GetForce())
		{
			if (it->GetForce() == Force::ATK && it == pAtkMostDamageMaster)
				it->AddMark(16);
			else if (it->GetForce() == Force::DEF && it == pDefMostDamageMaster)
				it->AddMark(16);
		}
		else
		{
			if (it->GetForce() == Force::ATK && it == pAtkMostDamageMaster)
				it->AddMark(32);
			else if (it->GetForce() == Force::DEF && it == pDefMostDamageMaster)
				it->AddMark(32);
		}
		datPlayer->set_player_mark(it->GetMark());
	}

#ifdef _TEST_
	std::cout << "玩家" << it->GetName() << "的战绩:\t" << endl;
	std::cout << "击杀士兵数量:\t" << it->GetAllKillSoldiers() << endl;
	std::cout << "击杀英雄数量:\t" << it->GetAllKillHeroes() << endl;
	std::cout << "出击的士兵数量:\t" << it->GetAllHitSoldier() << endl;
	std::cout << "获得的总资源量:\t" << it->GetAllResource() << endl << endl;
#endif

	pto_BATTLE_S2C_NTF_Game_Over ptoGameOver;
	ptoGameOver.set_time(nTempTime);
	ptoGameOver.set_is_exception(false);

	float fScore{ 0 };

	int nOppositeAverageDamage{ 0 };

	if (master->GetForce() == Force::ATK)
		nOppositeAverageDamage = nDefAverageDamage;
	else if (master->GetForce() == Force::DEF)
		nOppositeAverageDamage = nAtkAverageDamage;

	fScore = (float)master->GetAllMakeDamage();
	if (nOppositeAverageDamage)
		fScore /= nOppositeAverageDamage;

	fScore += master->GetAllKillHeroes() *0.03f;
	fScore += master->GetMarkNum() * 0.05f;

	if (m_enLoseForce == master->GetForce())
	{
		ptoGameOver.set_is_win(false);
	}
	else
	{
		fScore += 0.1f;
		ptoGameOver.set_is_win(true);
		master->GetExtraBlueprint();
		ptoGameOver.set_box_num(m_nBoxNum);
		if (GetBattleMode() != BattleMode::MULTISTAGE && master->MultiStageClear())
			ptoGameOver.set_box_num(0);
		master->SetStageLootPto(&ptoGameOver);
	}

	if (Force::ATK == master->GetForce())
		ptoGameOver.set_enemy_point(m_nDefPoint);
	else
		ptoGameOver.set_enemy_point(m_nAtkPoint);

	for (int i = 0; i < ptoTemp.player_date_size(); i++)
	{
		auto datPlayer = ptoGameOver.add_player_date();
		datPlayer->set_master_id(ptoTemp.player_date(i).master_id());
		datPlayer->set_kill_soldier_num(ptoTemp.player_date(i).kill_soldier_num());
		datPlayer->set_kill_hero_num(ptoTemp.player_date(i).kill_hero_num());
		datPlayer->set_get_all_resource(ptoTemp.player_date(i).get_all_resource());
		datPlayer->set_get_crystal_times(ptoTemp.player_date(i).get_crystal_times());
		datPlayer->set_player_mark(ptoTemp.player_date(i).player_mark());
		datPlayer->set_pid(ptoTemp.player_date(i).pid());
		datPlayer->set_is_win(ptoTemp.player_date(i).is_win());
	}

	if (fScore > 1.5f)
		ptoGameOver.set_player_grade(0);
	else if (fScore > 1.25f)
		ptoGameOver.set_player_grade(1);
	else if (fScore < 0.7f)
		ptoGameOver.set_player_grade(3);
	else
		ptoGameOver.set_player_grade(2);

	ptoGameOver.set_pos((int)GetWidth() / 2);
	ptoGameOver.set_mode(int(m_enMode));
	ptoGameOver.set_id(m_nStageID);
	ptoGameOver.set_difficulty(m_nDifficulty);
	ptoGameOver.set_story_id(m_nEndStory);

	std::string str;
	ptoGameOver.SerializeToString(&str);

	SendToMaster(master, str, MSG_S2C, BATTLE_S2C_NTF_GameOver);
}

void CBattle::ChargeBulletDie(CBattleObject* owner)
{
	for (auto &it : m_setBullets)
	{
		if (it && it->IsChargeBullet())
		{
			if (it && it->owner() == owner)
				it->Die();
		}
	}
}
