#include "stdafx.h"
#include "StageBattle.h"
#include "EditMap.h"
#include "Master.h"
#include "Building.h"
#include "Hero.h"
#include "Incident.h"
#include "Gate.h"
#include "Bullet.h"

CStageBattle::CStageBattle()
{

}

CStageBattle::~CStageBattle()
{

}

void CStageBattle::SetBattleByPto(const pto_BATTLE_S2B_REQ_CreateBattle_Ex* pPtoMaster)
{
	m_pEditMap = CEditMap::GetEditMap(pPtoMaster->map());
	if (!m_pEditMap)
		return;
	m_nSceneID = m_pEditMap->m_nSceneID;

	if (m_pEditMap->m_fWidth > 0)
		m_fWidth = m_pEditMap->m_fWidth;

	int nAktUser{ 0 };
	int nDefUser{ 0 };

	int nAtkAI{ 0 };
	int nDefAI{ 0 };

	if (m_enMode != BattleMode::MULTISTAGE)
	{
		for (size_t i = 0; i < m_pEditMap->m_arrMasterType.size(); i++)
		{
			CMaster* pMaster{ nullptr };

			if (1 == m_pEditMap->m_arrMasterType[i])
			{
				pMaster = new CMaster{ this };

				if (i <= 2)
				{
					pMaster->SetResIncMul(m_pEditMap->m_arrMasters[i]->GetResIncMul());
					pMaster->SetMasterByPto(&pPtoMaster->atk_masters(nAktUser), Force::ATK);
					if (m_pEditMap->try_hero_)
						pMaster->SetTryHero(m_pEditMap->try_hero_id_, m_pEditMap->try_hero_level_);
					nAktUser++;
				}
				else
				{
					if (nDefUser < pPtoMaster->def_masters_size())
					{
						pMaster->SetMasterByPto(&pPtoMaster->def_masters(nDefUser), Force::DEF);
						if (m_pEditMap->try_hero_)
							pMaster->SetTryHero(m_pEditMap->try_hero_id_, m_pEditMap->try_hero_level_);
						nDefUser++;
					}
				}

				m_arrMasters[i] = pMaster;
				pMaster->SetMasterID(i + 1);

				m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = 0;
			}
			else if (2 == m_pEditMap->m_arrMasterType[i])
			{
				if (i <= 2)
				{
					pMaster = m_pEditMap->m_arrMasters[i]->Clone(this);
					pMaster->SetForce(Force::ATK);
					nAtkAI++;
				}
				else
				{
					pMaster = m_pEditMap->m_arrMasters[i]->Clone(this);
					pMaster->SetForce(Force::DEF);
					nDefAI++;
				}

				m_arrMasters[i] = pMaster;
				pMaster->SetMasterID(i + 1);
				pMaster->SetIsAI(true);

				m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = 100;
			}
		}
	}

	else
	{
		for (int i = 0; i < pPtoMaster->atk_masters_size(); i++)
		{
			CMaster* pMaster{ nullptr };
			pMaster = new CMaster{ this };
			pMaster->SetResIncMul(m_pEditMap->m_arrMasters[i]->GetResIncMul());
			pMaster->SetMasterByPto(&pPtoMaster->atk_masters(nAktUser), Force::ATK);
			nAktUser++;

			m_arrMasters[i] = pMaster;
			pMaster->SetMasterID(i + 1);

			m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = 0;
		}
		for (size_t i = 3; i < m_pEditMap->m_arrMasterType.size(); i++)
		{
			CMaster* pMaster{ nullptr };
			if (2 == m_pEditMap->m_arrMasterType[i])
			{
				if (i <= 2)
				{
					pMaster = m_pEditMap->m_arrMasters[i]->Clone(this);
					pMaster->SetForce(Force::ATK);
					nAtkAI++;
				}
				else
				{
					pMaster = m_pEditMap->m_arrMasters[i]->Clone(this);
					pMaster->SetForce(Force::DEF);
					nDefAI++;
				}

				m_arrMasters[i] = pMaster;
				pMaster->SetMasterID(i + 1);
				pMaster->SetIsAI(true);

				m_maSPPlayerLoadProcess[pMaster->GetMasterID()] = 100;
			}
		}
	}
}

void CStageBattle::Initialize()
{
	m_bAtkBaseOver = m_pEditMap->m_bAtkCastleOver;
	m_bDefBaseOver = m_pEditMap->m_bDefCastleOver;
	atk_base_gun_ = m_pEditMap->atk_base_gun_;
	def_base_gun_ = m_pEditMap->def_base_gun_;

	try_hero_ = m_pEditMap->try_hero_;
	try_hero_id_ = m_pEditMap->try_hero_id_;

	for (auto &it : m_pEditMap->m_vctBuildings)
		AddObject(it->Clone(this));

	for (auto it_buliding : m_mapAtkBuildings)
	{
		if (it_buliding.second->GetBuidlingType() == BuildingType::BUILDING_TYPE_ATKCASTLE)
		{
			m_pAtkCastle = it_buliding.second;
			for (auto it : m_arrMasters)
			{
				if (it && it->GetForce() == Force::ATK && !it->IsAI())
				{
					it_buliding.second->AddMasterCasletUpgrade(it->GetCastleHPEx());
				}
			}
		}
	}

	for (auto it_buliding : m_mapDefBuildings)
	{
		if (it_buliding.second->GetBuidlingType() == BuildingType::BUILDING_TYPE_DEFCASTLE)
		{
			m_pDefCastle = it_buliding.second;
		}
	}

	for (auto &it : m_pEditMap->m_vctHeroes)
		if (false == it->IsBackup())
			AddObject(it->Clone(this), false);

	for (auto &it : m_pEditMap->m_vctUnits)
		if (false == it->IsBackup())
			AddObject(it->Clone(this), false);

	for (auto &it : m_pEditMap->m_vctIncidents)
		m_vctIncidents.push_back(std::make_pair(it, false));

	for (auto &it : m_pEditMap->m_vctMapVariable)
		m_vctMapVariable.push_back(std::make_pair(it.first, it.second));

	for (auto &it : m_pEditMap->m_vctGate)
		AddGate(it);

	for (auto &it : m_pEditMap->m_vctCustomBullet)
		ProduceCustomBullet(it, false);
}

void CStageBattle::SendStageData(pto_BATTLE_S2C_NTF_SendBattleGroundData *pto)
{
	pto->set_battle_name(ANSI_to_UTF8(m_pEditMap->m_strStageName));
	pto->set_victor_describe(ANSI_to_UTF8(m_pEditMap->m_strVictorDescribe));
	pto->set_lost_describe(ANSI_to_UTF8(m_pEditMap->m_strLostDescribe));
	pto->set_map_id(GetMapID());
	pto->set_atk_base_gun(atk_base_gun_);
	pto->set_def_base_gun(def_base_gun_);

	std::vector<const 	std::map<int, CUnit*>*> vctList;
	vctList.push_back(&m_mapAtkUnits);
	vctList.push_back(&m_mapDefUnits);

	for (auto &itList : vctList)
	{
		for (auto &itSoldier : *itList)
		{
			CUnit* pUnit{ itSoldier.second };

			if (pUnit->IsBoss())
				pUnit->SendCustomUnitToClient();
			else
				pUnit->SetUnitProtocol(pto->add_vctunit());
		}
	}

	for (auto it : m_setWallBullets)
		it->SetBulletProtocol(pto->add_bullet());

	for (auto &it : m_setBullets)
		it->SetBulletProtocol(pto->add_bullet());
}

int	CStageBattle::GetMapID() const
{
	return m_pEditMap->m_nMapID;
}

void CStageBattle::_EventLoop()
{
	for (size_t i = 0; i < m_vctIncidents.size(); i++)
	{
		//判断条件
		if (true == m_vctIncidents.at(i).second)
			continue;
		if (true == m_vctIncidents.at(i).first->m_bIsConditionTime)
		{
			if (m_nTimes / 1000 < m_vctIncidents.at(i).first->m_nConditionTime)
				continue;
		}
		if (true == m_vctIncidents.at(i).first->m_bIsConditionPos)
		{
			if (m_vctIncidents.at(i).first->m_nConditionPosCamp == 0)
			{
				if (FindStageSoldierLinePos(Force::ATK, true, true) < m_vctIncidents.at(i).first->m_fConditionPos)
					continue;
				else
					m_fGameOverPos = m_vctIncidents.at(i).first->m_fConditionPos;
			}
			if (m_vctIncidents.at(i).first->m_nConditionPosCamp == 1)
			{
				if (FindStageSoldierLinePos(Force::DEF, true, true) > m_vctIncidents.at(i).first->m_fConditionPos)
					continue;
				else
					m_fGameOverPos = m_vctIncidents.at(i).first->m_fConditionPos;
			}
		}
		if (true == m_vctIncidents.at(i).first->m_bIsConditionResource)
		{
			CMaster* pMaster = FindMasterByMID(m_vctIncidents.at(i).first->m_nConditionResourceCamp);
			if (pMaster == nullptr)
				continue;
			else if (pMaster->GetResource() < m_vctIncidents.at(i).first->m_nConditionResource)
				continue;
			else
				m_fGameOverPos = FindSoldiersLinePos(Force::ATK, true, true, true);
		}

		if (m_vctIncidents.at(i).first->m_bHeroUnavailable)
		{
			if (!__HeroUnavailable())
				continue;
		}

		if (true == m_vctIncidents.at(i).first->m_bIsBaseLv)
		{
			CMaster* pMaster = FindMasterByMID(m_vctIncidents.at(i).first->m_nBaseLvMaster);
			if (pMaster->military_level() < m_vctIncidents.at(i).first->m_nBaseLv + 1)
				continue;
			else
			{
				switch (pMaster->GetForce())
				{
				case Force::ATK:
					m_fGameOverPos = 0;
					m_fGameOverPos = FindSoldiersLinePos(Force::ATK, false, true, true);
					break;
				case Force::DEF:
					m_fGameOverPos = GetWidth();
					m_fGameOverPos = FindSoldiersLinePos(Force::ATK, false, true, true);
					break;
				default:
					break;
				}
			}
		}

		if (true == m_vctIncidents.at(i).first->m_bIsConditionHP)
		{
			if (false == __IsConditionHp(m_vctIncidents.at(i).first))
				continue;
		}

		if (true == m_vctIncidents.at(i).first->m_bIsConditionUnitPos)
		{
			if (false == __IsConditionUnitPos(m_vctIncidents.at(i).first))
				continue;
		}

		if (m_vctIncidents.at(i).first->m_bNeedTimer)
		{
			if (!__IsNeedTimer(m_vctIncidents.at(i).first->m_nNeedTimerID, m_vctIncidents.at(i).first->m_nNeedTimerTime))
				continue;
		}

		if (true == m_vctIncidents.at(i).first->m_bHeroDead)
		{
			if (false == m_bPlayerHeroDead)
				continue;
		}

		if (true == m_vctIncidents.at(i).first->m_bHeroDisappear)
		{
			if (false == m_bPlayerHeroDisappear)
				continue;
		}

		if (m_vctIncidents.at(i).first->is_call_hero_)
		{
			bool call_hero{ false };
			for (auto &it : m_arrMasters)
			{
				if (it && !it->IsAI() && !it->UsingHero())
					call_hero = true;
			}
			if (call_hero)
				continue;
		}

		bool bIsMapVariableRight = true;
		for (size_t n = 0; n < m_vctIncidents.at(i).first->m_vctConditionTrueVariableID.size(); n++)
		{
			for (size_t k = 0; k < m_vctMapVariable.size(); k++)
			{
				if (m_vctIncidents.at(i).first->m_vctConditionTrueVariableID.at(n) == m_vctMapVariable.at(k).first)
				{
					if (m_vctMapVariable.at(k).second == false)
					{
						bIsMapVariableRight = false;
						break;
					}
				}
			}
			if (bIsMapVariableRight == false)
				break;
		}
		if (bIsMapVariableRight == false)
			continue;
		else
			m_fGameOverPos = FindSoldiersLinePos(Force::ATK, false, true, true);

		for (size_t n = 0; n < m_vctIncidents.at(i).first->m_vctConditionFalseVariableID.size(); n++)
		{
			for (size_t k = 0; k < m_vctMapVariable.size(); k++)
			{
				if (m_vctIncidents.at(i).first->m_vctConditionFalseVariableID.at(n) == m_vctMapVariable.at(k).first)
				{
					if (m_vctMapVariable.at(k).second == true)
					{
						bIsMapVariableRight = false;
						break;
					}
				}
			}
			if (bIsMapVariableRight == false)
				break;
		}
		if (bIsMapVariableRight == false)
			continue;
		else
			m_fGameOverPos = FindSoldiersLinePos(Force::ATK, false, true, true);

		//	执行事件

		if (true == m_vctIncidents.at(i).first->m_bIsPlayeStory)
		{
			if (m_vctIncidents.at(i).first->m_bIsResultVictor ||
				m_vctIncidents.at(i).first->m_bIsResultLose)
				m_nEndStory = m_vctIncidents.at(i).first->m_nStoryId;
			else
				__PlayStory(m_vctIncidents.at(i).first->m_nStoryId);
		}

		if (m_vctIncidents.at(i).first->change_base_level_)
			__ChangeBaseLevel(m_vctIncidents.at(i).first->change_base_level_maseter_, m_vctIncidents.at(i).first->change_base_level_lv_);

		if (m_vctIncidents.at(i).first->m_bIsResultVictor)  //胜利
			GameOver(Force::DEF, m_fGameOverPos);

		if (m_vctIncidents.at(i).first->m_bIsResultLose)    //失败
			GameOver(Force::ATK, m_fGameOverPos);

		if (m_vctIncidents.at(i).first->set_backup_mark_)
			__SetBackupMark(m_vctIncidents.at(i).first->backup_mark_pos_);

		if (m_vctIncidents.at(i).first->cancel_backup_mark_)
			__CancelBackupMark();

		if (m_vctIncidents.at(i).first->m_bIsCancelMark)
			__SendCancelMark();

		if (m_vctIncidents.at(i).first->open_timer_)
			OpenTimer(m_vctIncidents.at(i).first->timer_time_, m_vctIncidents.at(i).first->timer_describe_);

		if (m_vctIncidents.at(i).first->close_timer_)
			CloseTimer();

		if (m_vctIncidents.at(i).first->m_bOpenGate)
			__OpenGate(m_vctIncidents.at(i).first->m_nOpenGateID);

		if (m_vctIncidents.at(i).first->m_bBeginTimer)
			m_mapTimer.insert(std::make_pair(m_vctIncidents.at(i).first->m_nBeginTimerID, 0));

		if (true == m_vctIncidents.at(i).first->m_bIsChangResourceOverride)
		{
			__ChangeMasterResourceOverride(m_vctIncidents.at(i).first->m_nMasterId, m_vctIncidents.at(i).first->m_fResourceOverride);
		}

		if (true == m_vctIncidents.at(i).first->m_bIsResultGetBuff)
			__UnitGetBuff(m_vctIncidents.at(i).first->m_nResultGetBuffGroup, m_vctIncidents.at(i).first->m_nResultGetBuffID);

		if (true == m_vctIncidents.at(i).first->m_bIsResultRemoveBuff)
			__RemoveUnitBuff(m_vctIncidents.at(i).first->m_nResultRemoveBuffGroup, m_vctIncidents.at(i).first->m_nResultRemoveBuffID);

		if (true == m_vctIncidents.at(i).first->m_bIsResultGetResource)   //添加资源
			__GetResource((Force)m_vctIncidents.at(i).first->m_nResultGetResourceCamp, m_vctIncidents.at(i).first->m_nResultGetResource);

		if (true == m_vctIncidents.at(i).first->m_bIsResultBackupArrive)
			__AddBackUp(m_vctIncidents.at(i).first->m_nResultBackupArriveGroup, m_vctIncidents.at(i).first->m_bBossEnter);

		if (true == m_vctIncidents.at(i).first->m_bIsResultShowNotice)
			__ShowNotice(m_vctIncidents.at(i).first->m_strShowNotice);

		if (true == m_vctIncidents.at(i).first->m_bIsResultMasterTalk)
			__MasterTalk(m_vctIncidents.at(i).first->m_nMasterNum, m_vctIncidents.at(i).first->m_strMasterTalk);

		if (true == m_vctIncidents.at(i).first->m_bIsResultHeroTalk)
			__HeroTalk(__GetUnitID(m_vctIncidents.at(i).first->m_nHeroGroup), m_vctIncidents.at(i).first->m_strHeroTalk);

		if (true == m_vctIncidents.at(i).first->m_bIsResultUnitRetreat)
			__UnitRetreat(m_vctIncidents.at(i).first->m_nUnitGroup);

		if (true == m_vctIncidents.at(i).first->m_bIsStopRetreat)
			__StopRetreat(m_vctIncidents.at(i).first->m_nStopRetratGroup);

		if (m_vctIncidents.at(i).first->m_bLockHero)
			__LockPlayerHero();

		if (m_vctIncidents.at(i).first->m_bUnlockHero)
			__UnlockPlayerHero();

		if (m_vctIncidents.at(i).first->m_bLockSoldier)
			__LockPlayerSoldier();

		if (m_vctIncidents.at(i).first->m_bUnlockSoldier)
			__UnlockPlayerSoldier();

		if (m_vctIncidents.at(i).first->m_bRemoveUnit)
			__RemoveUnit(m_vctIncidents.at(i).first->m_nRemoveUnitGroup);

		if (m_vctIncidents.at(i).first->m_bResetHero)
			__ResetHero();

		if (m_vctIncidents.at(i).first->m_bChangeAI)
			__ChangeUnitAIType(m_vctIncidents.at(i).first->m_nChangeAIGroup, m_vctIncidents.at(i).first->m_nChangeAIType, (float)m_vctIncidents[i].first->m_nChangeAIRange);

		if (m_vctIncidents.at(i).first->m_bMasterDisappear)
			__MasterDisappear(m_vctIncidents.at(i).first->m_nMasterDisappearID + 1);

		if (m_vctIncidents.at(i).first->m_bUnitDropBullet)
			__UnitDroupBullet(m_vctIncidents.at(i).first->m_nDropBulletGroup, m_vctIncidents.at(i).first->m_nDropBulletId, m_vctIncidents.at(i).first->m_nBulletValue);

		if (m_vctIncidents.at(i).first->m_bDeleteBullet)
			__DeleteBullet(m_vctIncidents.at(i).first->m_nDeleteBulletCustomID);

		for (size_t n = 0; n < m_vctIncidents.at(i).first->m_vctChangeTrueVariableID.size(); n++)
		{
			for (size_t k = 0; k < m_vctMapVariable.size(); k++)
			{
				if (m_vctMapVariable.at(k).first == m_vctIncidents.at(i).first->m_vctChangeTrueVariableID.at(n))
				{
					m_vctMapVariable.at(k).second = true;
				}
			}
		}

		for (size_t n = 0; n < m_vctIncidents.at(i).first->m_vctChangeFalseVariableID.size(); n++)
		{
			for (size_t k = 0; k < m_vctMapVariable.size(); k++)
			{
				if (m_vctMapVariable.at(k).first == m_vctIncidents.at(i).first->m_vctChangeFalseVariableID.at(n))
				{
					m_vctMapVariable.at(k).second = false;
				}
			}
		}

		if (!m_bPlayerHeroDead && !m_bPlayerHeroDisappear)
			m_vctIncidents.at(i).second = true;  //事件标记为已执行
	}
	m_bPlayerHeroDead = false;
	m_bPlayerHeroDisappear = false;
	_SendProtoPackage();
}

void CStageBattle::_StartStory()
{
	__PlayStory(m_pEditMap->m_nBeginStory);
}

bool CStageBattle::__IsConditionHp(const CIncident* pIncident)
{
	int nResult{ 0 };
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetGroupID() == pIncident->m_nConditionHPGroup)
		{
			m_fGameOverPos = it.second->GetPos();
			if (it.second->GetHP() > (pIncident->m_nConditionHP * it.second->GetMaxHP() / 100))
			{
				return false;
			}
			else if (pIncident->m_nConditionHP <= 0 &&
				!it.second->IsDead())
			{
				return false;
			}
			else
				nResult++;
		}
	}

	for (auto it : m_mapDefUnits)
	{
		if (it.second->GetGroupID() == pIncident->m_nConditionHPGroup)
		{
			m_fGameOverPos = it.second->GetPos();
			if (it.second->GetHP() > (pIncident->m_nConditionHP * it.second->GetMaxHP() / 100))
			{
				return false;
			}
			else if (pIncident->m_nConditionHP <= 0 &&
				!it.second->IsDead())
			{
				return false;
			}
			else
				nResult++;
		}
	}

	for (auto it : m_mapDeadUnits)
	{
		if (it.second->GetGroupID() == pIncident->m_nConditionHPGroup)
		{
			m_fGameOverPos = it.second->GetPos();
			if (it.second->GetHP() > (pIncident->m_nConditionHP * it.second->GetMaxHP() / 100))
			{
				return false;
			}
			else if (pIncident->m_nConditionHP <= 0 &&
				!it.second->IsDead())
			{
				return false;
			}
			else
				nResult++;
		}
	}

	for (auto it : m_mapAtkBuildings)
	{
		if (it.second->GetGroupID() == pIncident->m_nConditionHPGroup)
		{
			m_fGameOverPos = it.second->GetPos();
			if (it.second->GetHP() > (pIncident->m_nConditionHP * it.second->GetMaxHP() / 100))
			{
				return false;
			}
			else
				nResult++;
		}
	}

	for (auto it : m_mapDefBuildings)
	{
		if (it.second->GetGroupID() == pIncident->m_nConditionHPGroup)
		{
			m_fGameOverPos = it.second->GetPos();
			if (it.second->GetHP() > (pIncident->m_nConditionHP * it.second->GetMaxHP() / 100))
			{
				return false;
			}
			else
				nResult++;
		}
	}

	for (auto it : m_mapNeutralBuildings)
	{
		if (it.second->GetGroupID() == pIncident->m_nConditionHPGroup)
		{
			m_fGameOverPos = it.second->GetPos();
			if (it.second->GetHP() > (pIncident->m_nConditionHP * it.second->GetMaxHP() / 100))
			{
				return false;
			}
			else
				nResult++;
		}
	}

	if (nResult < __AllGroupUnit(pIncident->m_nConditionHPGroup))
		return false;

	return true;
}

bool CStageBattle::__IsConditionUnitPos(const CIncident* pIncident)
{
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() != AIType::AI_RETREAT &&
			it.second->GetPos() >= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
		else if (it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() == AIType::AI_RETREAT &&
			it.second->GetPos() <= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
	}

	for (auto it : m_mapDefUnits)
	{
		if (it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() == AIType::AI_RETREAT &&
			it.second->GetPos() >= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
		else if (it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() != AIType::AI_RETREAT &&
			it.second->GetPos() <= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
	}

	for (auto it : m_mapDeadUnits)
	{
		if (it.second->GetForce() == Force::ATK &&
			it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() != AIType::AI_RETREAT &&
			it.second->GetPos() >= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
		else if (it.second->GetForce() == Force::ATK &&
			it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() == AIType::AI_RETREAT &&
			it.second->GetPos() <= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
		if (it.second->GetForce() == Force::DEF &&
			it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() == AIType::AI_RETREAT &&
			it.second->GetPos() >= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
		else if (it.second->GetForce() == Force::DEF &&
			it.second->GetGroupID() == pIncident->m_nUnitPosGroup &&
			it.second->GetAIType() != AIType::AI_RETREAT &&
			it.second->GetPos() <= pIncident->m_fUnitPos)
		{
			m_fGameOverPos = it.second->GetPos();
			return true;
		}
	}

	return false;
}

void CStageBattle::__PlayStory(int nStoryId)
{
	if (nStoryId >= 0)
	{
		pto_BATTLE_S2C_NTF_PlayStory m_nPtoBSNPS;
		m_nPtoBSNPS.set_id(nStoryId);

		std::string msgStr;
		m_nPtoBSNPS.SerializeToString(&msgStr);

		for (auto &it : m_arrMasters)
		{
			if (it && !it->IsAI() && !it->IsLeave())
			{
				SendToMaster(it, msgStr, MSG_S2C, BATTLE_S2C_NTF_PlayStory);
			}
		}

		if (BtSt_Ready == GetBattleState())
			m_bIsReadyBeforePause = true;

		if (BattleMode::MULTISTAGE != m_enMode)
		{
			Pause();
		}
	}
}

void CStageBattle::__SendCancelMark()
{
	for (auto &it : m_arrMasters)
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, "", MSG_S2C, BATTLE_S2C_NTF_CancelMark);
}

void CStageBattle::__ChangeMasterResourceOverride(int nMasterId, float fResourceOverride)
{
	CMaster* pMaster = FindMasterByMID(nMasterId + 1);
	if (nullptr != pMaster)
		pMaster->SetResIncMul(fResourceOverride);
}

void CStageBattle::__UnitGetBuff(int nGroupID, int nBuffID)
{
	for (auto it : m_mapAtkUnits)
	{
		if (nGroupID == it.second->GetGroupID())
			it.second->SetBuff(nBuffID, nullptr);
	}

	for (auto it : m_mapDefUnits)
	{
		if (nGroupID == it.second->GetGroupID())
			it.second->SetBuff(nBuffID, nullptr);
	}

	for (auto it : m_mapDeadUnits)
	{
		if (nGroupID == it.second->GetGroupID())
			it.second->SetBuff(nBuffID, nullptr);
	}
}

void CStageBattle::__RemoveUnitBuff(int nGroupID, int nBuffID)
{
	for (auto it : m_mapAtkUnits)
	{
		if (nGroupID == it.second->GetGroupID())
			int a = nBuffID;
	}

	for (auto it : m_mapDefUnits)
	{
		if (nGroupID == it.second->GetGroupID())
			int a = nBuffID;
	}

	for (auto it : m_mapDeadUnits)
	{
		if (nGroupID == it.second->GetGroupID())
			int a = nBuffID;
	}
}

void CStageBattle::__GetResource(Force enForce, int nResource)
{
	if (enForce == 0)
		for (auto &it : m_arrMasters)
		{
		if (nullptr != it &&
			Force::ATK == it->GetForce())
		{
			it->ChangeResource(nResource);
		}
		}
	if (enForce == 1)
		for (auto &it : m_arrMasters)
		{
		if (nullptr != it &&
			Force::DEF == it->GetForce())
		{
			it->ChangeResource(nResource);
		}
		}
}

void CStageBattle::__AddBackUp(int nGroupID, bool bBossEnter)
{
	for (auto &it : m_pEditMap->m_vctUnits)
		if (true == it->IsBackup() &&
			it->GetGroupID() == nGroupID)
		{
		AddObject(it->Clone(this), true);
		}

	for (auto &it : m_pEditMap->m_vctHeroes)
		if (true == it->IsBackup() &&
			it->GetGroupID() == nGroupID)
		{
		AddObject(it->Clone(this), true);
		}

	if (bBossEnter)
	{
		for (auto &it : m_arrMasters)
		{
			if (it && !it->IsAI() && !it->IsLeave())
				SendToMaster(it, "", MSG_S2C, BATTLE_S2C_NTF_BossEnter);
		}
	}
}

void CStageBattle::__ShowNotice(std::string strNotice)
{
	pto_BATTLE_S2C_NTF_ShowNotice pBSNSN;
	pBSNSN.set_notice(strNotice);
	std::string msgStr;
	pBSNSN.SerializeToString(&msgStr);
	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, msgStr, MSG_S2C, BATTLE_S2C_NTF_ShowNotice);
	}
}

void CStageBattle::__MasterTalk(int nMasterID, const std::string strTalk)
{
	pto_BATTLE_S2C_NTF_MasterTalk pBSNMT;
	pBSNMT.set_master_id(nMasterID);
	pBSNMT.set_master_talk(strTalk);
	std::string msgStr;
	pBSNMT.SerializeToString(&msgStr);

	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, msgStr, MSG_S2C, BATTLE_S2C_NTF_MasterTalk);
	}
}

void CStageBattle::__HeroTalk(int nHeroID, const std::string strTalk)
{
	if (-1 != nHeroID)
	{
		pto_BATTLE_S2C_NTF_UnitTalk pBSNHT;
		pBSNHT.set_unit_nid(nHeroID);
		pBSNHT.set_unit_talk(strTalk);
		std::string msgStr;
		pBSNHT.SerializeToString(&msgStr);

		for (auto &it : m_arrMasters)
		{
			if (it && !it->IsAI() && !it->IsLeave())
				SendToMaster(it, msgStr, MSG_S2C, BATTLE_S2C_NTF_UnitTalk);
		}
	}
}

int CStageBattle::__GetUnitID(int nHeroGroupID)
{
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetGroupID() == nHeroGroupID &&
			!it.second->IsDead())
			return it.second->GetID();
	}

	for (auto it : m_mapDefUnits)
	{
		if (it.second->GetGroupID() == nHeroGroupID &&
			!it.second->IsDead())
			return it.second->GetID();
	}

	for (auto it : m_mapDeadUnits)
	{
		if (it.second->GetGroupID() == nHeroGroupID)
			return it.second->GetID();
	}
	return -1;
}

int  CStageBattle::__GetHeroID(int nHeroGroupID)
{
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetGroupID() == nHeroGroupID &&
			it.second->IsHero())
			return it.second->GetID();
	}

	for (auto it : m_mapDefUnits)
	{
		if (it.second->GetGroupID() == nHeroGroupID &&
			it.second->IsHero())
			return it.second->GetID();
	}

	for (auto it : m_mapDeadUnits)
	{
		if (it.second->GetGroupID() == nHeroGroupID &&
			it.second->IsHero())
			return it.second->GetID();
	}
	return -1;
}

void CStageBattle::__UnitRetreat(int nUnitGroupID)
{
	for (auto it : m_mapAtkUnits)
		if (it.second->GetGroupID() == nUnitGroupID)
			it.second->Retreat();

	for (auto it : m_mapDefUnits)
		if (it.second->GetGroupID() == nUnitGroupID)
			it.second->Retreat();

	for (auto it : m_mapDeadUnits)
		if (it.second->GetGroupID() == nUnitGroupID)
			it.second->Retreat();
}

void CStageBattle::__StopRetreat(int nUnitGroupID)
{
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetGroupID() == nUnitGroupID)
		{
			it.second->StopRetreat();
		}
	}

	for (auto it : m_mapDefUnits)
	{
		if (it.second->GetGroupID() == nUnitGroupID)
		{
			it.second->StopRetreat();
		}
	}

	for (auto it : m_mapDeadUnits)
	{
		if (it.second->GetGroupID() == nUnitGroupID)
		{
			it.second->StopRetreat();
		}
	}
}

void CStageBattle::__LockPlayerHero()
{
	pto_BATTLE_S2C_NTF_LockFunction pto;
	pto.set_type(0);
	std::string msgString;
	pto.SerializeToString(&msgString);

	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI())
		{
			it->LockHero();
			SendToMaster(it, msgString, MSG_S2C, BATTLE_S2C_NTF_LockFunction);
		}
	}
}

void CStageBattle::__UnlockPlayerHero()
{
	pto_BATTLE_S2C_NTF_LockFunction pto;
	pto.set_type(1);
	std::string msgString;
	pto.SerializeToString(&msgString);

	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI())
		{
			it->UnlockHero();
			SendToMaster(it, msgString, MSG_S2C, BATTLE_S2C_NTF_LockFunction);
		}
	}
}

void CStageBattle::__LockPlayerSoldier()
{
	pto_BATTLE_S2C_NTF_LockFunction pto;
	pto.set_type(2);
	std::string msgString;
	pto.SerializeToString(&msgString);
	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI())
		{
			it->LockSoldier();
			SendToMaster(it, msgString, MSG_S2C, BATTLE_S2C_NTF_LockFunction);
		}
	}
}

void CStageBattle::__UnlockPlayerSoldier()
{
	pto_BATTLE_S2C_NTF_LockFunction pto;
	pto.set_type(3);
	std::string msgString;
	pto.SerializeToString(&msgString);

	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI())
		{
			it->UnlockSoldier();
			SendToMaster(it, msgString, MSG_S2C, BATTLE_S2C_NTF_LockFunction);
		}
	}
}

void CStageBattle::__RemoveUnit(const int nUnitGroupID)
{
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetGroupID() == nUnitGroupID)
		{
			GetProtoPackage(Force::ATK)->RemoveUnit(it.second->GetID());
			if (!it.second->IsHiding())
				GetProtoPackage(Force::DEF)->RemoveUnit(it.second->GetID());
			it.second->Remove();
		}
	}

	for (auto it : m_mapDefUnits)
	{
		if (it.second->GetGroupID() == nUnitGroupID)
		{
			GetProtoPackage(Force::DEF)->RemoveUnit(it.second->GetID());
			if (!it.second->IsHiding())
				GetProtoPackage(Force::ATK)->RemoveUnit(it.second->GetID());
			it.second->Remove();
		}
	}
}

void CStageBattle::__ResetHero()
{
	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI())
		{
			it->ResetHero();
		}
	}
}

void CStageBattle::__ChangeUnitAIType(const int nUnitGroupID, const int nUnitAIType, float fUnitChangeAIRange)
{
	for (auto it : m_mapAtkUnits)
	{
		if (nUnitGroupID == it.second->GetGroupID())
		{
			it.second->ChangeAIType((AIType)nUnitAIType);
			it.second->SetActiveRange(fUnitChangeAIRange);
			if (AI_PATROL == (AIType)nUnitAIType)
			{
				it.second->SetPatrolPoint(it.second->GetPos());
			}
		}
	}

	for (auto it : m_mapDefUnits)
	{
		if (nUnitGroupID == it.second->GetGroupID())
		{
			it.second->ChangeAIType((AIType)nUnitAIType);
			it.second->SetActiveRange(fUnitChangeAIRange);
			if (AI_PATROL == (AIType)nUnitAIType)
			{
				it.second->SetPatrolPoint(it.second->GetPos());
			}
		}
	}

	for (auto it : m_mapDeadUnits)
	{
		if (nUnitGroupID == it.second->GetGroupID())
		{
			it.second->SetAIType((AIType)nUnitAIType);
			it.second->SetActiveRange(fUnitChangeAIRange);
		}
	}
}

void CStageBattle::__OpenGate(int nGateID)
{
	for (auto it : m_mapGates)
	{
		if (nGateID == it.second->GetID())
		{
			it.second->OpenGate();
			pto_BATTLE_S2C_NTF_OpenGate pto;
			pto.set_id(it.second->GetID());
			pto.set_state(it.second->IsOpen());

			std::string msgStr;
			pto.SerializeToString(&msgStr);

			for (auto &it : m_arrMasters)
			{
				if (it && !it->IsAI() && !it->IsLeave())
				{
					SendToMaster(it, msgStr, MSG_S2C, BATTLE_S2C_NTF_OpenGate);
				}
			}
		}
	}
}

bool CStageBattle::__HeroUnavailable()
{
	for (auto it : m_arrMasters)
	{
		if (it && !it->IsAI())
		{
			if (!it->HeroUnavailable())
				return false;
		}
	}
	return true;
}

int CStageBattle::__AllGroupUnit(int nGroup)
{
	int nResult = 0;
	for (auto it : m_pEditMap->m_vctHeroes)
	{
		if (it->GetGroupID() == nGroup)
			nResult++;
	}
	for (auto it : m_pEditMap->m_vctBuildings)
	{
		if (it->GetGroupID() == nGroup)
			nResult++;
	}
	for (auto it : m_pEditMap->m_vctUnits)
	{
		if (it->GetGroupID() == nGroup)
			nResult++;
	}
	return nResult;
}

float CStageBattle::FindStageSoldierLinePos(Force enForce, bool bIncludeHide /*= false*/, bool bIncludeHero/*=false*/) const
{
	float fPos{ -1 };

	const 	std::map<int, CUnit*>*		pMapUnits{ nullptr };
	const 	std::map<int, CBuilding*>*	pMapBuildings{ nullptr };

	if (Force::ATK == enForce)
	{
		pMapUnits = &m_mapAtkUnits;
		pMapBuildings = &m_mapAtkBuildings;
	}
	else if (Force::DEF == enForce)
	{
		pMapUnits = &m_mapDefUnits;
		pMapBuildings = &m_mapDefBuildings;
		fPos = 10086;
	}

	for (auto &it : *pMapUnits)
	{
		if (false == bIncludeHero && it.second->IsHero())
			continue;

		if (false == bIncludeHide && it.second->IsHiding())
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

void CStageBattle::__MasterDisappear(int nMasterID)
{
	pto_BATTLE_S2C_NTF_MasterDisappear ptoNTF;
	ptoNTF.set_master_id(nMasterID);

	std::string msgStr;
	ptoNTF.SerializeToString(&msgStr);
	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, msgStr, MSG_S2C, BATTLE_S2C_NTF_MasterDisappear);
	}
}

bool CStageBattle::__IsNeedTimer(int nTimerID, int nTime)
{
	auto it = m_mapTimer.find(nTimerID);
	if (it == m_mapTimer.cend())
		return false;
	else if (it->second >= nTime)
		return true;
	return false;
}

void CStageBattle::__UnitDroupBullet(int nGroup, int nBulletID, int nBulletValue)
{
	for (auto it : m_mapAtkUnits)
	{
		if (it.second->GetGroupID() == nGroup)
			ProuduceBullet(it.second->GetPos(), nBulletID, nBulletValue);
	}

	for (auto it : m_mapDefUnits)
	{
		if (it.second->GetGroupID() == nGroup)
			ProuduceBullet(it.second->GetPos(), nBulletID, nBulletValue);
	}

	for (auto it : m_mapDeadUnits)
	{
		if (it.second->GetGroupID() == nGroup)
			ProuduceBullet(it.second->GetPos(), nBulletID, nBulletValue);
	}

	for (auto it : m_mapAtkBuildings)
	{
		if (it.second->GetGroupID() == nGroup)
			ProuduceBullet(it.second->GetPos(), nBulletID, nBulletValue);
	}

	for (auto it : m_mapDefBuildings)
	{
		if (it.second->GetGroupID() == nGroup)
			ProuduceBullet(it.second->GetPos(), nBulletID, nBulletValue);
	}

	for (auto it : m_mapNeutralBuildings)
	{
		if (it.second->GetGroupID() == nGroup)
			ProuduceBullet(it.second->GetPos(), nBulletID, nBulletValue);
	}
}

void CStageBattle::__DeleteBullet(int nCustomID)
{
	for (auto it = m_setBullets.begin(); it != m_setBullets.cend();)
	{
		if ((*it)->GetCustomID() == nCustomID)
			(*it)->Die();
		++it;
	}
}

void CStageBattle::__SetBackupMark(int pos)
{
	pto_BATTLE_S2C_NTF_SetBackupMark pto_ntf;
	pto_ntf.set_mark_pos(pos);
	std::string str_pto;
	pto_ntf.SerializeToString(&str_pto);

	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, str_pto, MSG_S2C, BATTLE_S2C_NTF_SetBackupMark);
	}
}

void CStageBattle::__CancelBackupMark()
{
	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, "", MSG_S2C, BATTLE_S2C_NTF_CancelBackupMark);
	}
}

void CStageBattle::OpenTimer(int time, std::string describe)
{
	pto_BATTLE_S2C_NTF_OpenTimer pto;
	pto.set_max_time(time);
	pto.set_describe(describe);

	std::string pto_msg;
	pto.SerializeToString(&pto_msg);
	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, pto_msg, MSG_S2C, BATTLE_S2C_NTF_OpenTimer);
	}
}

void CStageBattle::CloseTimer()
{
	for (auto &it : m_arrMasters)
	{
		if (it && !it->IsAI() && !it->IsLeave())
			SendToMaster(it, "", MSG_S2C, BATTLE_S2C_NTF_CloseTimer);
	}
}

void CStageBattle::__ChangeBaseLevel(int master_id, int level)
{
	CMaster* master = FindMasterByMID(master_id);
	if (master)
	{
		master->ChangeLevel(level);
	}
}
