#include "stdafx.h"
#include "PassSkill.h"

std::map<int, const CPassSkill*> CPassSkill::ms_mapPassSkills;

void CPassSkill::Load()
{
	dat_SKILL_Library_All libSkill;
	libSkill.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Skill.txt"));

	auto itPassSkills = libSkill.pass_skills();

	for (int i = 0; i < itPassSkills.pass_skill_size(); i++)
	{
		auto itPassSkill = itPassSkills.pass_skill(i);

		if (0 == itPassSkill.id())
			continue;

		CPassSkill* pPassSkill = new CPassSkill;

		pPassSkill->m_nID = itPassSkill.id();
		pPassSkill->m_strName = UTF8_to_ANSI(itPassSkill.name());
		pPassSkill->m_strDescribe = itPassSkill.describe();
		pPassSkill->m_nSmallIcon = itPassSkill.small_icon();
		pPassSkill->m_enTriggerTime = itPassSkill.trigger_time();
		pPassSkill->m_nTriggerInterval = itPassSkill.trigger_interval();
		pPassSkill->m_enTriggerType = itPassSkill.trigger_type();
		pPassSkill->m_nTriggerCondition = itPassSkill.trigger_condition();
		pPassSkill->m_nTriggerConditionParama = itPassSkill.trigger_condition_parama();
		pPassSkill->m_nReduceValue = itPassSkill.reduce_value();
		pPassSkill->m_fReducePercent = itPassSkill.reduce_percent();
		pPassSkill->m_nReduceType = itPassSkill.reduce_type();
		pPassSkill->m_nReduceTypeExcept = itPassSkill.reduce_type_except();

		if (itPassSkill.base_type_size() != itPassSkill.base_value_size())
			throw std::exception("技能基础类型和技能基础类型值数量不匹配");

		for (int i = 0; i < itPassSkill.base_type_size(); i++)
			pPassSkill->m_vctBase.push_back(std::make_pair(itPassSkill.base_type(i), itPassSkill.base_value(i)));

		pPassSkill->m_nTriggerSkillBuffID = itPassSkill.trigger_skill_buff_id();
		pPassSkill->m_enProductionType = itPassSkill.production_type();
		pPassSkill->m_fProductionValue = itPassSkill.production_value();

		if (itPassSkill.restrain_size() != itPassSkill.restrain_value_size())
			throw std::exception("技能相克类型和技能相克类型值数量不匹配");

		for (int i = 0; i < itPassSkill.restrain_size(); i++)
			pPassSkill->restrictions_.push_back(std::make_pair(itPassSkill.restrain(i), itPassSkill.restrain_value(i)));

		auto it = ms_mapPassSkills.insert(std::make_pair(pPassSkill->m_nID, pPassSkill));
		if (false == it.second)
			delete pPassSkill;
	}
}

const CPassSkill* CPassSkill::GetPassSkill(int nID)
{
	auto it = ms_mapPassSkills.find(nID);

	if (ms_mapPassSkills.cend() == it)
		return nullptr;
	else
		return it->second;
}
