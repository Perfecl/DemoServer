#include "stdafx.h"
#include "WorldBossBattle.h"

CWorldBossBattle::CWorldBossBattle()
{
}

CWorldBossBattle::~CWorldBossBattle()
{
	for (auto it : m_vctOffLineHero)
	{
		if (nullptr != it)
			delete it;
	}
}

void CWorldBossBattle::Start(COffLinePlayer* pPlayer, SWorldBoss* pBoss, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto)
{
	m_vctOffLineHero.clear();
	m_vctOffLineUnit.clear();

	m_nBossHP = pBoss->m_nHP;

	pPto->add_base_hp(100);
	pPto->add_base_max_hp(100);
	pPto->add_base_hp(pBoss->m_nHP);
	pPto->add_base_max_hp(pBoss->m_nMaxHP);

	InitUnit(pPlayer, pPto, Force::ATK);
	InitBoss(pBoss, pPto);
}

void CWorldBossBattle::Loop(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto)
{
	if (nullptr == m_vctOffLineUnit.at(0).GetOffLineHero() &&
		nullptr == m_vctOffLineUnit.at(1).GetOffLineHero() &&
		nullptr == m_vctOffLineUnit.at(2).GetOffLineHero())
	{
		pPto->set_win_force(Force::DEF);
	}
	while (false == m_bIsOver)
	{
		pto_OFFLINEBATTLE_Struct_BattleRound* pOSBR = pPto->add_rounds();

		if (false == m_bIsOver && nullptr != m_vctOffLineUnit.at(0).GetOffLineHero())
		{
			pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_atk_unit_rounds();
			m_vctOffLineUnit.at(0).WorldBossBattleLoop(&m_vctOffLineUnit.at(3), m_nBossHP, m_nDamage, m_bIsOver, pOSURG, m_bWinForce);
			if (true == m_bIsOver && m_bWinForce == Force::ATK)
			{
				if (true == m_vctOffLineUnit.at(1).AbleAtkBuliding())
				{
					pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_atk_unit_rounds();
					m_vctOffLineUnit.at(1).FakeAtkBuliding(pOSURG);
				}
				if (true == m_vctOffLineUnit.at(2).AbleAtkBuliding())
				{
					pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_atk_unit_rounds();
					m_vctOffLineUnit.at(2).FakeAtkBuliding(pOSURG);
				}
			}
		}

		if (false == m_bIsOver && nullptr != m_vctOffLineUnit.at(1).GetOffLineHero())
		{
			pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_atk_unit_rounds();
			m_vctOffLineUnit.at(1).WorldBossBattleLoop(&m_vctOffLineUnit.at(3), m_nBossHP, m_nDamage, m_bIsOver, pOSURG, m_bWinForce);
			if (true == m_bIsOver && m_bWinForce == Force::ATK)
			{
				if (true == m_vctOffLineUnit.at(2).AbleAtkBuliding())
				{
					pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_atk_unit_rounds();
					m_vctOffLineUnit.at(2).FakeAtkBuliding(pOSURG);
				}
			}
		}

		if (false == m_bIsOver && nullptr != m_vctOffLineUnit.at(2).GetOffLineHero())
		{
			pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_atk_unit_rounds();
			m_vctOffLineUnit.at(2).WorldBossBattleLoop(&m_vctOffLineUnit.at(3), m_nBossHP, m_nDamage, m_bIsOver, pOSURG, m_bWinForce);
		}

		if (m_nRound >= 20 && !m_bIsOver)
		{
			m_bIsOver = true;
			m_bWinForce = Force::DEF;
		}
		m_nRound++;
	}
}

int CWorldBossBattle::Over(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto)
{
	pPto->set_win_force(m_bWinForce);
	return m_nDamage;
}

void CWorldBossBattle::InitUnit(COffLinePlayer* pPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto, Force enForce)
{
	pto_OFFLINEBATTLE_Struct_PlayerData* pOSPD = pPto->add_player_data();
	pOSPD->set_pid(pPlayer->GetPID());
	pOSPD->set_sex(pPlayer->GetSex());
	pOSPD->set_lv(pPlayer->GetLevel());
	pOSPD->set_name(pPlayer->GetName());
	pOSPD->set_dress_id(pPlayer->GetDressID());
	if (enForce)
		pOSPD->set_force(true);
	else
		pOSPD->set_force(false); 

	for (size_t i = 0; i < 3; i++)
	{
		COffLineHero* pOfflineHero = COffLineHero::InitHero(pPlayer->GerHeroCard(i), pPlayer);
		if (nullptr == pOfflineHero)
		{
			m_vctOffLineHero.push_back(nullptr);
			COffLineUnit nOffLineUnit;
			nOffLineUnit.SetOffLineHero(nullptr);
			m_vctOffLineUnit.push_back(nOffLineUnit);
		}
		else
		{
			m_vctOffLineHero.push_back(pOfflineHero);
			COffLineUnit nOffLineUnit;
			nOffLineUnit.SetForce(enForce);
			nOffLineUnit.SetOffLineHero(pOfflineHero);
			nOffLineUnit.SetHeroTP(pOfflineHero->GetHeroTP());
			if (Force::ATK == enForce)
			{
				nOffLineUnit.SetID(i);
				nOffLineUnit.SetPos(0);
			}
			else
			{
				nOffLineUnit.SetID(i + 3);
				nOffLineUnit.SetPos(8);
			}
			m_vctOffLineUnit.push_back(nOffLineUnit);

			pto_OFFLINEBATTLE_Struct_UnitData* pUnitData = pPto->add_unit_data();
			pUnitData->set_id(nOffLineUnit.GetID());
			pUnitData->set_nlevel(nOffLineUnit.GetOffLineHero()->level());
			pUnitData->set_hero_id(nOffLineUnit.GetHeroTP()->GetID());
			pUnitData->set_pos(nOffLineUnit.GetPos());
			pUnitData->set_hp(nOffLineUnit.GetOffLineHero()->GetMaxHP());
			pUnitData->set_atk(nOffLineUnit.GetOffLineHero()->GetAtk());
			pUnitData->set_def(nOffLineUnit.GetOffLineHero()->GetDef());
			pUnitData->set_matk(nOffLineUnit.GetOffLineHero()->GetMatk());
			pUnitData->set_mdef(nOffLineUnit.GetOffLineHero()->GetMdef());
			pUnitData->set_str(nOffLineUnit.GetOffLineHero()->GetStr());
			pUnitData->set_intelligence(nOffLineUnit.GetOffLineHero()->GetInt());
			pUnitData->set_cmd(nOffLineUnit.GetOffLineHero()->GetCmd());
			pUnitData->set_range(nOffLineUnit.GetRange());
			pUnitData->set_nrank(nOffLineUnit.GetOffLineHero()->rank());
		}
	}
}

void CWorldBossBattle::InitBoss(SWorldBoss* pBoss, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto)
{
	COffLineHero* pOfflineHero = new COffLineHero;
	pOfflineHero->SetStr(pBoss->m_nStr);
	pOfflineHero->SetCmd(pBoss->m_nCmd);
	pOfflineHero->SetInt(pBoss->m_nInt);
	pOfflineHero->SetHP(pBoss->m_nHP);  
	pOfflineHero->SetMaxHP(pBoss->m_nMaxHP);


	m_vctOffLineHero.push_back(pOfflineHero);
	COffLineUnit nOffLineUnit;
	nOffLineUnit.SetForce(Force::DEF);
	nOffLineUnit.SetOffLineHero(pOfflineHero);
	nOffLineUnit.SetHeroTP(pOfflineHero->GetHeroTP());

	nOffLineUnit.SetID(3);
	nOffLineUnit.SetPos(9);
	m_vctOffLineUnit.push_back(nOffLineUnit);
}