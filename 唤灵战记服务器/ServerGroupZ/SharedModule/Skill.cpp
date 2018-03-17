#include "stdafx.h"
#include "Skill.h"

std::map<int, const CSkill*> CSkill::ms_mapSkills;

const CSkill* CSkill::GetSkill(int nID)
{
	auto it = ms_mapSkills.find(nID);
	if (ms_mapSkills.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CSkill::Load()
{
	dat_SKILL_Library_All libSkill;
	libSkill.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Skill.txt"));

	auto itSkills = libSkill.skills();

	for (int i = 0; i < itSkills.skill_size(); i++)
	{
		auto itSkill = itSkills.skill(i);

		if (0 == itSkill.id())
			continue;

		CSkill* pSkill = new CSkill;

		pSkill->m_nSkillID = itSkill.id();
		pSkill->m_nSmallIconID = itSkill.small_icon_id();
		pSkill->m_nCooldown = itSkill.cd();
		pSkill->m_strName = UTF8_to_ANSI(itSkill.name());

		pSkill->m_fHealthEffect = itSkill.health_effect();
		pSkill->m_enMoveEffect = (SKILL_MOVE_EFFECT)itSkill.move_effect();
		pSkill->m_enSepecialEffect = (SKILL_SEPECIAL_EFFECT)itSkill.sepecial_effect();

		pSkill->m_nSummonedID = itSkill.summoned_id();
		pSkill->m_nSummonedNum = itSkill.summoned_num();

		pSkill->m_nLeadTime = itSkill.lead_time();
		pSkill->m_nChantTime = itSkill.chant_time();
		pSkill->m_nSpasticTime = itSkill.spastic_time();

		pSkill->m_bStandChannel = itSkill.is_stand_channel();
		pSkill->m_bRepeatSameBullet = itSkill.repeat_same_bullet();

		pSkill->m_nCastTimes = itSkill.cast_times();
		pSkill->m_nCastInterval = itSkill.cast_interval();
		pSkill->m_fCastRange = (float)itSkill.cast_range();
		pSkill->m_nCastArea = itSkill.cast_area();
		pSkill->m_bIsNeedClick = itSkill.is_need_click();

		for (int i = 0; i < itSkill.bullet_id_size(); i++)
			pSkill->m_vctBullets.push_back(itSkill.bullet_id(i));

		for (int i = 0; i < itSkill.self_buff_id_size(); i++)
			pSkill->m_vctSelfBuffs.push_back(itSkill.self_buff_id(i));

		pSkill->m_nAIRule = itSkill.ai_rule();

		auto it = ms_mapSkills.insert(std::make_pair(pSkill->m_nSkillID, pSkill));
		if (false == it.second)
			delete pSkill;
	}
}
