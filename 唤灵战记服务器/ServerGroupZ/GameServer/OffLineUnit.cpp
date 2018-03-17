#include "stdafx.h"
#include "OffLineUnit.h"

COffLineUnit::COffLineUnit()
{
}

COffLineUnit::~COffLineUnit()
{
}

void COffLineUnit::WorldBossBattleLoop(COffLineUnit* pBoss, __int64& nBossHP, int& nDmg, bool& bIsOver, pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG, bool& bWinForce)
{
	if (m_pOffLineHero->IsDead())
		return;
	std::vector<COffLineUnit*> all_unit;
	all_unit.push_back(pBoss);
	if (true == AbleAtkBuliding())
	{
		pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
		if (!UseSuitSkill(pBoss, &all_unit, unit_round))
			ChangeEnergy(10);
		nDmg += __AtkWorldBoss(pBoss, nBossHP, bIsOver, unit_round, bWinForce);
		return;
	}
	else
	{
		__Move(nullptr, pOSURG);
		if (true == AbleAtkBuliding())
		{
			pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
			if (!UseSuitSkill(pBoss, &all_unit, unit_round))
				ChangeEnergy(10);
			nDmg += __AtkWorldBoss(pBoss, nBossHP, bIsOver, unit_round, bWinForce);
			return;
		}
	}
}

void COffLineUnit::Loop(COffLineUnit* m_pTargetUnit, std::vector<COffLineUnit*>* all_unit, int& m_nBulidingHp, bool& m_bIsOver, pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG, bool& m_bWinForce)
{
	if (m_pOffLineHero->IsDead())
		return;
	if (frozen_ > 0)
	{
		pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
		unit_round->set_type(3);
		unit_round->set_unit_id(m_nID);
		unit_round->set_energy(energy_);
		ChangeState(unit_round);
	}
	else if (true == __HasTarget(m_pTargetUnit))
	{
		pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
		if (!UseSuitSkill(m_pTargetUnit, all_unit, unit_round))
			ChangeEnergy(10);
		__AtkUnit(m_pTargetUnit, unit_round);
		ChangeState(unit_round);
	}
	else if (true == AbleAtkBuliding())
	{
		pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
		if (!UseSuitSkill(m_pTargetUnit, all_unit, unit_round))
			ChangeEnergy(10);
		__AtkBuliding(m_nBulidingHp, m_bIsOver, unit_round, m_bWinForce);
		ChangeState(unit_round);
	}
	else if (stop_move_ > 0)
	{
		pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
		unit_round->set_type(4);
		unit_round->set_unit_id(m_nID);
		unit_round->set_energy(energy_);
		ChangeState(unit_round);
	}
	else if (stop_move_ <= 0)
	{
		__Move(m_pTargetUnit, pOSURG);
		if (true == __HasTarget(m_pTargetUnit))
		{
			pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
			if (!UseSuitSkill(m_pTargetUnit, all_unit, unit_round))
				ChangeEnergy(10);
			__AtkUnit(m_pTargetUnit, unit_round);
			ChangeState(unit_round);
		}
		else if (true == AbleAtkBuliding())
		{
			pto_OFFLINEBATTLE_Struct_UnitRound* unit_round = pOSURG->add_unit_round();
			if (!UseSuitSkill(m_pTargetUnit, all_unit, unit_round))
				ChangeEnergy(10);
			__AtkBuliding(m_nBulidingHp, m_bIsOver, unit_round, m_bWinForce);
			ChangeState(unit_round);
		}
	}
}

bool COffLineUnit::__HasTarget(COffLineUnit* m_pTargetUnit)
{
	if (nullptr == m_pTargetUnit->m_pOffLineHero)
		return false;
	if (true == m_pTargetUnit->m_pOffLineHero->IsDead())
		return false;
	if (Force::ATK == m_enForce)
	{
		if (GetRange() >= (m_pTargetUnit->pos_ - pos_))
			return true;
		else
			return false;
	}
	else
	{
		if (GetRange() >= pos_ - m_pTargetUnit->pos_)
			return true;
		else
			return false;
	}
}

int COffLineUnit::__AtkUnit(COffLineUnit* pTargetUnit, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round)
{
	int make_damage{ 0 };
	if (Force::ATK == m_enForce)
	{
		m_pOffLineHero->Attack(pTargetUnit->m_pOffLineHero, unit_round);
		unit_round->set_unit_id(m_nID);
		unit_round->set_target_id(pTargetUnit->m_nID);
		unit_round->set_type(1);
		if (pTargetUnit->m_pOffLineHero->IsDead())
		{
			unit_round->set_type(0);
		}
		unit_round->set_energy(energy_);
		make_damage += (int)unit_round->num();
		return make_damage;
	}
	else
	{
		m_pOffLineHero->Attack(pTargetUnit->m_pOffLineHero, unit_round);
		unit_round->set_unit_id(m_nID);
		unit_round->set_target_id(pTargetUnit->m_nID);
		unit_round->set_type(1);
		if (pTargetUnit->m_pOffLineHero->IsDead())
		{
			unit_round->set_type(0);
		}
		unit_round->set_energy(energy_);
		make_damage += (int)unit_round->num();
		return make_damage;
	}
}

bool COffLineUnit::AbleAtkBuliding()
{
	if (nullptr == m_pOffLineHero)
		return false;

	if (Force::ATK == m_enForce)
	{
		if (GetRange() > (8 - pos_))
			return true;
		else
			return false;
	}
	else
	{
		if (GetRange() > (pos_))
			return true;
		else
			return false;
	}
}

int COffLineUnit::__AtkWorldBoss(COffLineUnit* pBoss, __int64& nBossHp, bool& bIsOver, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round, bool& bWinForce)
{
	int nDmg = m_pOffLineHero->Attack(pBoss->m_pOffLineHero, unit_round);
	if (Force::ATK == m_enForce)
	{
		unit_round->set_type(1);
		unit_round->set_unit_id(m_nID);
		unit_round->set_num(nDmg);
		unit_round->set_target_id(7);
		nBossHp -= nDmg;
		if (nBossHp <= 0)
		{
			unit_round->set_type(0);
			bIsOver = true;
			bWinForce = Force::ATK;
		}
	}
	else
	{
		unit_round->set_type(1);
		unit_round->set_unit_id(m_nID);
		unit_round->set_num(nDmg);
		unit_round->set_target_id(6);
		nBossHp -= nDmg;
		if (nBossHp <= 0)
		{
			unit_round->set_type(0);
			bIsOver = true;
			bWinForce = Force::DEF;
		}
	}
	return nDmg;
}

void COffLineUnit::__AtkBuliding(int& nBulidingHp, bool& bIsOver, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round, bool& bWinForce)
{
	if (Force::ATK == m_enForce)
	{
		nBulidingHp -= m_pOffLineHero->GetAtkBuildingDamage();
		if (nBulidingHp <= 0)
		{
			bIsOver = true;
			bWinForce = Force::ATK;
		}

		//printf("单位%d攻击对方基地，剩余血量%d\n", m_nId, m_nBulidingHp);
		unit_round->set_type(1);
		unit_round->set_unit_id(m_nID);
		unit_round->set_num(m_pOffLineHero->GetAtkBuildingDamage());
		unit_round->set_target_id(7);
		unit_round->set_energy(energy_);
	}
	else
	{
		nBulidingHp -= m_pOffLineHero->GetAtkBuildingDamage();
		if (nBulidingHp <= 0)
		{
			bIsOver = true;
			bWinForce = Force::DEF;
		}

		//printf("单位%d攻击对方基地，剩余血量%d\n", m_nId, m_nBulidingHp);
		unit_round->set_type(1);
		unit_round->set_unit_id(m_nID);
		unit_round->set_num(m_pOffLineHero->GetAtkBuildingDamage());
		unit_round->set_target_id(6);
		unit_round->set_energy(energy_);
	}
}

void COffLineUnit::__Move(COffLineUnit* pTargetUnit, pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG)
{
	if (Force::ATK == m_enForce)
	{
		int m_nTempPos = pos_;
		if (1 == m_pHeroTP->GetType() || 4 == m_pHeroTP->GetType())
			pos_ += 2;
		else
			pos_ += 1;
		if (pTargetUnit)
		{
			if (nullptr != pTargetUnit->m_pOffLineHero)
			{
				if (pos_ >= pTargetUnit->pos_ &&
					pTargetUnit->m_pOffLineHero->IsDead() == false)
					pos_ = pTargetUnit->pos_ - 1;
			}
		}
		if (pos_ > 8)
			pos_ = 8;
		if (m_nTempPos != pos_)
		{
			pto_OFFLINEBATTLE_Struct_UnitRound* pOSUR = pOSUR = pOSURG->add_unit_round();
			pOSUR->set_type(2);
			pOSUR->set_unit_id(m_nID);
			pOSUR->set_num(pos_);
		}
	}
	else
	{
		int m_nTempPos = pos_;
		if (1 == m_pHeroTP->GetType())
			pos_ -= 2;
		else
			pos_ -= 1;
		if (pTargetUnit)
		{
			if (nullptr != pTargetUnit->m_pOffLineHero)
			{
				if (pos_ <= pTargetUnit->pos_ && pTargetUnit->m_pOffLineHero->IsDead() == false)
					pos_ = pTargetUnit->pos_ + 1;
			}
		}
		if (pos_ < 0)
			pos_ = 0;
		if (m_nTempPos != pos_)
		{
			pto_OFFLINEBATTLE_Struct_UnitRound* pOSUR = pOSUR = pOSURG->add_unit_round();
			pOSUR->set_type(2);
			pOSUR->set_unit_id(m_nID);
			pOSUR->set_num(pos_);
		}
	}
}

int COffLineUnit::GetRange()
{
	int m_nResult = int(m_pOffLineHero->GetAtkDistance() / 200);
	if (m_nResult < 1)
		return 1;
	else
		return m_nResult;
}

void COffLineUnit::FakeAtkBuliding(pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG)
{
	if (true == AbleAtkBuliding() && Force::ATK == m_enForce)
	{
		pto_OFFLINEBATTLE_Struct_UnitRound* pOSUR = pOSURG->add_unit_round();
		pOSUR->set_type(1);
		pOSUR->set_unit_id(m_nID);
		pOSUR->set_num(m_pOffLineHero->GetAtkBuildingDamage());
		pOSUR->set_target_id(7);
	}
	else if (true == AbleAtkBuliding() && Force::DEF == m_enForce)
	{
		pto_OFFLINEBATTLE_Struct_UnitRound* pOSUR = pOSURG->add_unit_round();
		pOSUR->set_type(1);
		pOSUR->set_unit_id(m_nID);
		pOSUR->set_num(m_pOffLineHero->GetAtkBuildingDamage());
		pOSUR->set_target_id(6);
	}
}

void COffLineUnit::ChangeEnergy(int energy)
{
	if (!CSuitTP::GetSuitTP(suit_id_))
		return;
	energy_ += energy;
	if (energy_ > 30)
		energy_ = 30;
}

bool COffLineUnit::UseSuitSkill(COffLineUnit* target_unit, std::vector<COffLineUnit*>* all_unit, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round)
{
	if (energy_ < 30 ||
		frozen_ > 0)
		return false;
	if (target_unit == nullptr || unit_round == nullptr) return false;
	switch (suit_id_)
	{
	case 1:
		energy_ = 0;
		m_pOffLineHero->set_double_attack(true);
		unit_round->set_skill_id(1);
		return true;
	case 2:
		if (target_unit->GetOffLineHero() != nullptr && !target_unit->GetOffLineHero()->IsDead())
		{
			energy_ = 0;
			target_unit->FallBack();
			unit_round->set_skill_id(2);
			return true;
		}
		return false;
	case 3:
		if (target_unit->GetOffLineHero() != nullptr && !target_unit->GetOffLineHero()->IsDead())
		{
			energy_ = 0;
			target_unit->StopMove();
			unit_round->set_skill_id(3);
			return true;
		}
		return false;
	case 4:
		if (target_unit->GetOffLineHero() != nullptr && !target_unit->GetOffLineHero()->IsDead())
		{
			energy_ = 0;
			target_unit->Frozen();
			unit_round->set_skill_id(4);
			energy_ = 0;
			return true;
		}
		return false;
	case 5:
	{
			  energy_ = 0;
			  UseHealSkill(all_unit);
			  unit_round->set_skill_id(5);
			  return true;
	}
	case 6:
	{
			  energy_ = 0;
			UseDmgUpSkill(all_unit);
			  unit_round->set_skill_id(6);
			  return true;
	}
	case 7:
		if (target_unit->GetOffLineHero() != nullptr && !target_unit->GetOffLineHero()->IsDead())
		{
			energy_ = 0;
			target_unit->ArmorWeak();
			unit_round->set_skill_id(7);
			return true;
		}
		return false;
	case 8:
	{
			  energy_ = 0;
			  UseEnergySkill(all_unit);
			  unit_round->set_skill_id(8);
			  return true;
	}
	default:
		return false;
	}
}

void COffLineUnit::ChangeState(pto_OFFLINEBATTLE_Struct_UnitRound* unit_round)
{
	if (stop_move_ > 0)
	{
		stop_move_--;
		if (stop_move_ == 0)
			unit_round->add_buff_id(3);
	}

	if (frozen_ > 0)
	{
		frozen_--;
		if (frozen_ == 0)
			unit_round->add_buff_id(4);
	}

	if (m_pOffLineHero)
	{
		if (m_pOffLineHero->dmg_up() > 0)
		{
			m_pOffLineHero->ChangeDmgUp(-1);
			if (m_pOffLineHero->dmg_up() == 0)
				unit_round->add_buff_id(6);
		}


		if (m_pOffLineHero->armor_weak() > 0)
		{
			m_pOffLineHero->ChangeArmorWeak(-1);
			if (m_pOffLineHero->armor_weak() == 0)
				unit_round->add_buff_id(7);
		}
	}
}

void COffLineUnit::FallBack()
{
	if (Force::ATK == m_enForce)
	{
		int m_nTempPos = pos_;
		pos_ -= 2;
		if (pos_ < 0)
			pos_ = 0;
	}
	else
	{
		int m_nTempPos = pos_;
		pos_ += 2;
		if (pos_ > 8)
			pos_ = 8;
	}
}

void COffLineUnit::UseHealSkill(std::vector<COffLineUnit*>* all_unit)
{
	int begin_num = 0;
	if (m_enForce == Force::DEF)
		begin_num = 3;

	for (int i = begin_num; i < begin_num + 3; i++)
	{
		if (all_unit->at(i)->GetOffLineHero() &&
			!all_unit->at(i)->GetOffLineHero()->IsDead())
		{
			all_unit->at(begin_num)->BeHeal();
		}
	}
}

void COffLineUnit::UseDmgUpSkill(std::vector<COffLineUnit*>* all_unit)
{
	int begin_num = 0;
	if (m_enForce == Force::DEF)
		begin_num = 3;

	for (int i = begin_num; i < begin_num + 3; i++)
	{
		if (all_unit->at(i)->GetOffLineHero() &&
			!all_unit->at(i)->GetOffLineHero()->IsDead())
		{
			all_unit->at(begin_num)->DmgUp();
		}
	}
}

void COffLineUnit::UseArmorWeakSkill(std::vector<COffLineUnit*>* all_unit)
{
	int begin_num = 0;
	if (m_enForce == Force::ATK)
		begin_num = 3;

	for (int i = begin_num; i < begin_num + 3; i++)
	{
		if (all_unit->at(i)->GetOffLineHero() &&
			!all_unit->at(i)->GetOffLineHero()->IsDead())
		{
			all_unit->at(i)->ArmorWeak();
		}
	}
}

void COffLineUnit::UseEnergySkill(std::vector<COffLineUnit*>* all_unit)
{
	int begin_num = 0;
	if (m_enForce == Force::DEF)
		begin_num = 3;

	for (int i = begin_num; i < begin_num + 3; i++)
	{
		if (all_unit->at(i)->GetOffLineHero() &&
			!all_unit->at(i)->GetOffLineHero()->IsDead())
		{
			all_unit->at(begin_num)->ChangeEnergy(10);
		}
	}
}

void COffLineUnit::BeHeal()
{
	if (!GetOffLineHero())
		return;
	if (GetOffLineHero()->IsDead())
		return;

	int heal_hp = int(GetOffLineHero()->GetMaxHP() * 0.5f);
	GetOffLineHero()->Heal(heal_hp);
}