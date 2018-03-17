#include "stdafx.h"
#include "BulletTP.h"

std::map<int, const CBulletTP*> CBulletTP::ms_mapBullets;

void CBulletTP::Load()
{
	dat_SKILL_Library_All libSkill;
	libSkill.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Skill.txt"));

	auto itBullets = libSkill.bullets();

	for (int i = 0; i < itBullets.bullet_size(); i++)
	{
		auto itBullet = itBullets.bullet(i);

		if (0 == itBullet.id())
			continue;

		CBulletTP *pBulletTP = new CBulletTP;

		pBulletTP->m_nBulletID = itBullet.id();
		pBulletTP->m_strName = UTF8_to_ANSI(itBullet.name());
		pBulletTP->m_fVolumn = itBullet.volumn();
		pBulletTP->m_enForm = (BulletForm)itBullet.form();

		pBulletTP->m_bIsRange = itBullet.is_range();
		pBulletTP->m_bIsArea = itBullet.is_area();
		pBulletTP->m_nType = itBullet.type();

		pBulletTP->m_nValidForce = itBullet.valid_force();
		pBulletTP->m_cVaildRange = itBullet.valid_range();
		pBulletTP->m_cVaildUnit = itBullet.valid_unit();

		pBulletTP->m_nBasePhysicsDamage = itBullet.base_physics();
		pBulletTP->m_nBaseMagicDamage = itBullet.base_magic();
		pBulletTP->m_fPercentPhysicsDamage = itBullet.percent_physics();
		pBulletTP->m_fPercentMagicDamage = itBullet.percent_magic();

		pBulletTP->m_fPercentDamageSoldier = itBullet.percent_soldier();
		pBulletTP->m_fPercentDamageHero = itBullet.percent_hero();
		pBulletTP->m_fPercentDamageBuilding = itBullet.percent_building();

		pBulletTP->m_bHasTargetObj = itBullet.has_target();
		pBulletTP->m_bHasFlyPath = itBullet.has_fly_path();

		pBulletTP->m_fPosOffset = itBullet.pos_offset();

		pBulletTP->m_nMoveLeadTime = itBullet.move_lead_time();
		pBulletTP->m_fMoveSpeed = itBullet.move_speed();
		pBulletTP->m_fMaxMoveDistance = itBullet.max_move_distance();

		pBulletTP->m_nSettlementLeadTime = itBullet.settlement_lead_time();
		pBulletTP->m_nSettlementIntervalTime = itBullet.settlement_interval();
		pBulletTP->m_nSettlementTimes = itBullet.settlement_times();

		pBulletTP->m_nAttackSingleTimes = itBullet.attack_single_times();
		pBulletTP->m_nAttackSingleIntervalTime = itBullet.attack_single_interval();

		pBulletTP->m_nHitTimes = itBullet.hit_times();

		pBulletTP->m_fFallbackDistance = itBullet.fall_back_distance();

		pBulletTP->m_nLifeTime = itBullet.life_time();

		pBulletTP->m_cDeadManner = itBullet.dead_manner();

		for (int i = 0; i < itBullet.buff_id_size(); i++)
			pBulletTP->m_vctBuffEffect.push_back(itBullet.buff_id(i));

		pBulletTP->m_fProcced = itBullet.proceed();
		pBulletTP->m_fMinProceed = itBullet.min_proceed();
		pBulletTP->m_fMaxProceed = itBullet.max_proceed();

		pBulletTP->m_nEffect = itBullet.effect();
		pBulletTP->m_nEffectValue = itBullet.effect_value();

		pBulletTP->m_nLayer1ResourceID = itBullet.layer1();
		pBulletTP->m_nLayer2ResourceID = itBullet.layer2();

		pBulletTP->m_nNewBulletID = itBullet.new_bullet_id();
		pBulletTP->m_bIsOpposite = itBullet.opposite();
		pBulletTP->m_nDead_SummonedID = itBullet.dead_summoned_id();
		pBulletTP->m_nDead_SummonedNum = itBullet.dead_summoned_num();

		pBulletTP->invalid_target_ = itBullet.invalid_target();

		auto it = ms_mapBullets.insert(std::make_pair(pBulletTP->m_nBulletID, pBulletTP));
		if (false == it.second)
			delete pBulletTP;
	}
}

const CBulletTP* CBulletTP::GetBulletTP(int nID)
{
	auto it = ms_mapBullets.find(nID);
	if (ms_mapBullets.cend() == it)
		return nullptr;
	else
		return it->second;
}