#include "stdafx.h"
#include "OffLineBattle.h"
#include "OffLinePlayer.h"

COffLineBattle::COffLineBattle()
{
	m_arrBulidingHP.fill(100);
}

COffLineBattle::~COffLineBattle()
{
	for (auto it : m_vctOffLineHero)
	{
		if (nullptr != it)
			delete it;
	}
}

void COffLineBattle::Start(COffLinePlayer* pPlayer, COffLinePlayer* pTargetPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto)
{
	m_arrBulidingHP[0] = 1000;
	m_arrBulidingHP[1] = 1000;

	pPto->add_base_hp(m_arrBulidingHP[0]);
	pPto->add_base_max_hp(m_arrBulidingHP[0]);
	pPto->add_base_hp(m_arrBulidingHP[1]);
	pPto->add_base_max_hp(m_arrBulidingHP[1]);

	m_vctOffLineHero.clear();
	m_vctOffLineUnit.clear();

	InitUnit(pPlayer, pPto, Force::ATK);
	InitUnit(pTargetPlayer, pPto, Force::DEF);
}

void COffLineBattle::InitUnit(COffLinePlayer* pOffLinePlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto, Force enForce)
{
	pto_OFFLINEBATTLE_Struct_PlayerData* pOSPD = pPto->add_player_data();
	pOSPD->set_pid(pOffLinePlayer->GetPID());
	pOSPD->set_sex(pOffLinePlayer->GetSex());
	pOSPD->set_lv(pOffLinePlayer->GetLevel());
	pOSPD->set_name(pOffLinePlayer->GetName());
	pOSPD->set_dress_id(pOffLinePlayer->GetDressID());
	if (enForce)
		pOSPD->set_force(true);
	else
		pOSPD->set_force(false);

	for (size_t i = 0; i < 3; i++)
	{
		COffLineHero* pOfflineHero = COffLineHero::InitHero(pOffLinePlayer->GerHeroCard(i), pOffLinePlayer);
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
			nOffLineUnit.set_suit_id(pOfflineHero->suit_id());
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
			//英雄品质和等级
			pUnitData->set_nrank(nOffLineUnit.GetOffLineHero()->rank());
			pUnitData->set_suit_id(nOffLineUnit.GetOffLineHero()->suit_id());
		}
	}
}

void COffLineBattle::Loop(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto)
{
	if (nullptr == m_vctOffLineUnit.at(0).GetOffLineHero() &&
		nullptr == m_vctOffLineUnit.at(1).GetOffLineHero() &&
		nullptr == m_vctOffLineUnit.at(2).GetOffLineHero() &&
		nullptr == m_vctOffLineUnit.at(3).GetOffLineHero() &&
		nullptr == m_vctOffLineUnit.at(4).GetOffLineHero() &&
		nullptr == m_vctOffLineUnit.at(5).GetOffLineHero())
	{
		m_bIsOver = true;
		m_bWinForce = Force::DEF;
		pPto->set_win_force(Force::DEF);
	}

	std::vector<COffLineUnit*> offline_unit;
	for (size_t i = 0; i < m_vctOffLineUnit.size(); i++)
	{
		offline_unit.push_back(&m_vctOffLineUnit.at(i));
	}

	while (false == m_bIsOver)
	{
		pto_OFFLINEBATTLE_Struct_BattleRound* pOSBR = pPto->add_rounds();

		if (false == m_bIsOver && nullptr != m_vctOffLineUnit.at(0).GetOffLineHero())
		{
			pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_atk_unit_rounds();
			m_vctOffLineUnit.at(0).Loop(&m_vctOffLineUnit.at(3), &offline_unit, m_arrBulidingHP[1], m_bIsOver, pOSURG, m_bWinForce);
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
			m_vctOffLineUnit.at(1).Loop(&m_vctOffLineUnit.at(4), &offline_unit, m_arrBulidingHP[1], m_bIsOver, pOSURG, m_bWinForce);
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
			m_vctOffLineUnit.at(2).Loop(&m_vctOffLineUnit.at(5), &offline_unit, m_arrBulidingHP[1], m_bIsOver, pOSURG, m_bWinForce);
		}

		if (false == m_bIsOver && nullptr != m_vctOffLineUnit.at(3).GetOffLineHero())
		{
			pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_def_unit_rounds();
			m_vctOffLineUnit.at(3).Loop(&m_vctOffLineUnit.at(0), &offline_unit, m_arrBulidingHP[0], m_bIsOver, pOSURG, m_bWinForce);
			if (true == m_bIsOver && m_bWinForce == Force::DEF)
			{
				if (true == m_vctOffLineUnit.at(4).AbleAtkBuliding())
				{
					pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_def_unit_rounds();
					m_vctOffLineUnit.at(4).FakeAtkBuliding(pOSURG);
				}
				if (true == m_vctOffLineUnit.at(5).AbleAtkBuliding())
				{
					pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_def_unit_rounds();
					m_vctOffLineUnit.at(5).FakeAtkBuliding(pOSURG);
				}
			}
		}

		if (false == m_bIsOver && nullptr != m_vctOffLineUnit.at(4).GetOffLineHero())
		{
			pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_def_unit_rounds();
			m_vctOffLineUnit.at(4).Loop(&m_vctOffLineUnit.at(1), &offline_unit, m_arrBulidingHP[0], m_bIsOver, pOSURG, m_bWinForce);
			if (true == m_bIsOver && m_bWinForce == Force::DEF)
			{
				if (true == m_vctOffLineUnit.at(5).AbleAtkBuliding())
				{
					pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_def_unit_rounds();
					m_vctOffLineUnit.at(5).FakeAtkBuliding(pOSURG);
				}
			}
		}

		if (false == m_bIsOver && nullptr != m_vctOffLineUnit.at(5).GetOffLineHero())
		{
			pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG = pOSBR->add_def_unit_rounds();
			m_vctOffLineUnit.at(5).Loop(&m_vctOffLineUnit.at(2), &offline_unit, m_arrBulidingHP[0], m_bIsOver, pOSURG, m_bWinForce);
		}
	}
}

bool COffLineBattle::Over(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pOSPOB)
{
	if (0 >= m_arrBulidingHP[0])
	{
		pOSPOB->set_win_force(Force::DEF);
		return false;
	}
	else if (0 >= m_arrBulidingHP[1])
	{
		pOSPOB->set_win_force(Force::ATK);
		return true;
	}

	return false;
}
