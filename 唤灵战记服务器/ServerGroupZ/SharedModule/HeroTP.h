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
	static float		m_fCrit;			//������
	static float		m_fCritValue;		//��������

	static float		m_fATKHero[5];		//�����ͼӳ� Ѫ���﹥�������ħ����ħ��
	static float		m_fDEFHero[5];		//ͳ���ͼӳ�
	static float		m_fINTHero[5];		//�����ͼӳ�
	static float		m_fSKILLHero[5];	//�����ͼӳ�

	static int			ms_ActiveSkill[3];	//����������
	static int			ms_PassSkill[3];	//����������

	static const int	HERO_DURATION{ 60000 }; //Ӣ�۳���ʱ��

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
	int			m_nStr{ 0 };				//���� 
	int			m_nCmd{ 0 };				//ͳ˧
	int			m_nInt{ 0 };				//����

	HERO_TYPE	m_enHeroType{ HT_ATK };		//Ӣ������
	int			m_nQuality{ 0 };			//Ʒ��

	float m_fGrowupAtk{ 0 };
	float m_fGrowupDef{ 0 };
	float m_fGrowupMAtk{ 0 };
	float m_fGrowupMDef{ 0 };
	float m_fGrowupHP{ 0 };

	std::vector<int> m_vctSkills;			//����
};
