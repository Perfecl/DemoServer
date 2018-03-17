#pragma once
#include "OffLineHero.h"

class COffLineBattle;
class COffLineUnit
{
public:
	COffLineUnit();
	~COffLineUnit();

	void WorldBossBattleLoop(COffLineUnit* m_pTargetUnit, __int64& nBossHP, int& nDmg, bool& bIsOver, pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG, bool& bWinForce);
	void Loop(COffLineUnit* pTargetUnit, std::vector<COffLineUnit*>* all_unit, int& nBulidingHp, bool& bIsOver, pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG, bool& bWinForce);

	bool UseSuitSkill(COffLineUnit* target_unit, std::vector<COffLineUnit*>* all_unit, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round);

	void SetOffLineHero(COffLineHero* pHero){ m_pOffLineHero = pHero; }
	void SetID(int nID){ m_nID = nID; }
	void SetPos(int nPos){ pos_ = nPos; }
	void SetForce(Force enForce){ m_enForce = enForce; }
	void SetHeroTP(const CHeroTP* pHeroTP){ m_pHeroTP = pHeroTP; }

	int  GetID() const { return m_nID; }
	int  GetPos() const { return pos_; }

	COffLineHero*		GetOffLineHero() const { return m_pOffLineHero; }
	Force				GetForce()const { return m_enForce; }
	const CHeroTP*		GetHeroTP() const { return m_pHeroTP; }

	int   GetRange();
	bool  AbleAtkBuliding();
	void  FakeAtkBuliding(pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG);

	void  ChangeEnergy(int energy);
	void  set_energy(int energy){ energy_ = energy; }
	int   energy() const { return energy_; }

	void  set_suit_id(int suit_id) { suit_id_ = suit_id; }

	void  StopMove() { stop_move_ = 2; }
	void  Frozen() { frozen_ = 1; }
	void  FallBack();
	void  DmgUp(){ if (!m_pOffLineHero) m_pOffLineHero->set_dmg_up(3); }
	void  ArmorWeak() { if (!m_pOffLineHero) m_pOffLineHero->set_armor_weak(2); }
	void  BeHeal();

private:
	COffLineHero*			m_pOffLineHero{ nullptr };
	int						m_nID{ 0 };
	int						pos_{ 0 };
	Force					m_enForce{ Force::ATK };
	const CHeroTP*			m_pHeroTP{ nullptr };
	int						energy_{ 0 };

	int						suit_id_{ 0 };		//套装ID

	int						stop_move_{ 0 };	//停止移动回合
	int						frozen_{ 0 };		//停止行动回合


	void ChangeState(pto_OFFLINEBATTLE_Struct_UnitRound* unit_round);
	bool __HasTarget(COffLineUnit* pTargetUnit);
	int __AtkUnit(COffLineUnit* pTargetUnit, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round);
	void __Attack(COffLineUnit* pTargetUnit, pto_OFFLINEBATTLE_Struct_UnitRound* pOSUR);
	int __AtkWorldBoss(COffLineUnit* pBoss, __int64 & nBossHp, bool& bIsOver, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round, bool& bWinForce);
	void __AtkBuliding(int& nBulidingHp, bool& bIsOver, pto_OFFLINEBATTLE_Struct_UnitRound* unit_round, bool& bWinForce);
	void __Move(COffLineUnit* pTargetUnit, pto_OFFLINEBATTLE_Struct_UnitRoundGroup* pOSURG);

	void UseHealSkill(std::vector<COffLineUnit*>* all_unit);
	void UseDmgUpSkill(std::vector<COffLineUnit*>* all_unit);
	void UseArmorWeakSkill(std::vector<COffLineUnit*>* all_unit);
	void UseEnergySkill(std::vector<COffLineUnit*>* all_unit);
};
