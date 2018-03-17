#pragma once
#include "UnitTP.h"

enum HERO_TYPE
{
	HT_ATK = 1,
	HT_DEF,
	HT_INT,
	HT_SKILL
};

class CHeroTP : public CUnitTP
{
	friend class CHero;
	friend class COffLineHero;
public:
	static void  Load();
	static const CHeroTP* GetHeroTP(int nID);
	static int   GetMaxTime(int nQuality);
	static int   GetLibSize(){ return ms_mapHeroTPs.size(); }
	static int   RandomHeroID();
	static void GetAllHeroID(std::vector<int>* vctHero);

private:
	static std::map<int, const CHeroTP*>	ms_mapHeroTPs;
	static std::vector<int>					ms_vctHeroID;
	static float		m_fCrit;			//暴击率
	static float		m_fCritValue;		//暴击倍率

	static float		m_fATKHero[5];		//力量型加成 血，物攻，物防，魔攻，魔防
	static float		m_fDEFHero[5];		//统御型加成
	static float		m_fINTHero[5];		//智力型加成
	static float		m_fSKILLHero[5];	//技巧型加成

	static int			ms_ActiveSkill[3];	//主动技开放
	static int			ms_PassSkill[3];	//被动技开放

	static const int	HERO_DURATION{ 60000 }; //英雄持续时间

public:
	CHeroTP() = default;
	~CHeroTP() = default;

	void SetProtocol(pto_BATTLE_Struct_HeroCard* ptoHero, int nTrainRank, int nLevel) const;
	int  GetStr() const { return m_nStr; }
	int  GetCmd() const { return m_nCmd; }
	int  GetInt() const { return m_nInt; }

	float GetGrowupAtk() const { return m_fGrowupAtk; }
	float GetGrowupDef() const { return m_fGrowupDef; }
	float GetGrowupMAtk() const { return m_fGrowupMAtk; }
	float GetGrowupMDef() const { return m_fGrowupMDef; }
	float GetGrowupHP() const { return m_fGrowupHP; }

	float crit() const { return CHeroTP::m_fCrit; }
	float crit_value() const { return CHeroTP::m_fCritValue; }

	HERO_TYPE  GetType() const { return m_enHeroType; }
	int	 GetExMaxTime(int nQuality, int nTrainRank, int nLevel) const;
	int  GetPassSkillExTime(int level) const;

	static float GetHeroTypeAddition(HERO_TYPE enHeroType, int nType);
	static int GetActiveSkillOpenLevel(int nIndex){ if (0 <= nIndex && nIndex <= 2) return ms_ActiveSkill[nIndex]; return 100; }
	static int GetPassSkillOpenLevel(int nIndex){ if (0 <= nIndex && nIndex <= 2)  return ms_PassSkill[nIndex]; return 100; }

private:
	int			m_nStr{ 0 };				//力量 
	int			m_nCmd{ 0 };				//统帅
	int			m_nInt{ 0 };				//智力

	HERO_TYPE	m_enHeroType{ HT_ATK };		//英雄类型
	int			m_nQuality{ 0 };			//品阶

	float m_fGrowupAtk{ 0 };
	float m_fGrowupDef{ 0 };
	float m_fGrowupMAtk{ 0 };
	float m_fGrowupMDef{ 0 };
	float m_fGrowupHP{ 0 };

	std::vector<int> m_vctSkills;			//技能
};
