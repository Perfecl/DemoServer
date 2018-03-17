#pragma once

class CPassSkill
{
public:
	static void Load();
	static const CPassSkill* GetPassSkill(int nID);

private:
	static  std::map<int, const CPassSkill*> ms_mapPassSkills;

public:
	CPassSkill() = default;
	~CPassSkill() = default;

	dat_SKILL_ENUM_TriggerTime		GetTriggerTime() const { return m_enTriggerTime; }
	dat_SKILL_ENUM_TriggerType		GetTriggerType() const { return m_enTriggerType; }
	dat_SKILL_ENUM_ProductionType	GetProductionType() const { return m_enProductionType; }
	int								GetTriggerInterval() const { return m_nTriggerInterval; }
	int								GetTriggerCondition() const { return m_nTriggerCondition; }
	float							GetTriggerConditionParama() const { return m_nTriggerConditionParama; }
	int								GetReduceType() const { return m_nReduceType; }
	int								GetReduceTypeExpect() const { return m_nReduceTypeExcept; }
	int								GetReduceValue() const { return m_nReduceValue; }
	float							GetReducePercent() const { return m_fReducePercent; }
	int								GetTriggerSkillBuffID() const { return m_nTriggerSkillBuffID; }
	float							GetProductionValue() const { return m_fProductionValue; }

	int								GetBaseSize() const { return m_vctBase.size(); }
	dat_SKILL_ENUM_BaseType			GetBaseType(int nIndex) const { return m_vctBase.at(nIndex).first; }
	float							GetBaseValue(int nIndex) const { return m_vctBase.at(nIndex).second; }
	int								GetSkillID() const { return m_nID; }

	inline const std::vector<std::pair<int, float>>& GetRestriction() const { return restrictions_; }
	
private:
	int								m_nID{ 0 };
	std::string						m_strName;
	std::string						m_strDescribe;
	int								m_nSmallIcon{ 0 };
	dat_SKILL_ENUM_TriggerTime		m_enTriggerTime{ TriggerTime_Null };
	int								m_nTriggerInterval{ 0 };
	dat_SKILL_ENUM_TriggerType		m_enTriggerType{ TriggerType_Null };
	int								m_nTriggerCondition{ 0 };
	float							m_nTriggerConditionParama{ 0 };
	int								m_nReduceValue{ 0 };
	float							m_fReducePercent{ 0 };
	int								m_nReduceType{ 0 };
	int								m_nReduceTypeExcept{ 0 };

	std::vector<std::pair<dat_SKILL_ENUM_BaseType, float>> m_vctBase;

	int								m_nTriggerSkillBuffID{ 0 };
	dat_SKILL_ENUM_ProductionType	m_enProductionType{ ProductionType_Null };
	float							m_fProductionValue{ 0 };

	std::vector<std::pair<int, float>>  restrictions_;
};
