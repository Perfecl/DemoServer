#pragma once

enum ATK_TYPE
{
	AT_PHYSICS = 1,		//物理
	AT_MAGIC,			//魔法
	AT_BLEND,			//混合
	AT_BaseGun,
};

enum TARGET_TYPE
{
	TT_NORMAL,			//普通
	TT_RANGE,			//远程
	TT_HERO,			//英雄
	TT_BUILDING			//建筑
};

class CUnitTP
{
public:
	static void Load();
	static int GetBaseHP() { return m_nBaseHP; }
	static int GetBaseATK() { return m_nBaseATK; }
	static int GetBaseDEF() { return m_nBaseDEF; }
	static int GetBaseMATK() { return m_nBaseMATK; }
	static int GetBaseMDEF() { return m_nBaseMDEF; }
	int GetID() const { return m_nID; }
	int GetPassSkillSize() const { return m_vctPassSkills.size(); }
	int GetPassSkill(int nIndex) const { return m_vctPassSkills.at(nIndex); }
	float GetAtkDistance() const { return m_fAtkDistance; }
	int GetAtkBuildingDamage() const { return m_nAtkBuildingDamage; }

protected:
	static int	m_nBaseHP;				//基础血量
	static int	m_nBaseATK;				//基础攻击
	static int	m_nBaseDEF;				//基础防御
	static int	m_nBaseMATK;			//基础魔攻
	static int	m_nBaseMDEF;			//基础魔防

public:
	CUnitTP() = default;
	~CUnitTP() = default;

	bool		IsRange() const { return m_bIsRange; }
	float		move_speed() const { return m_fMoveSpeed; }
	int			atk_prev() const { return m_nAtkPrev; }
	int			atk_interval() const { return m_nAtkInterval; }

protected:
	int			m_nID{ 0 };						//ID
	std::string	m_strName;						//名字
	int			m_nResourceID{ 0 };				//资源ID

	float		m_fVolume{ 0 };					//体积
	float		m_fMoveSpeed{ 0 };				//移动速度

	bool		m_bIsRange{ false };			//是否远程单位

	float		m_fAtkDistance{ 0 };			//攻击距离
	int			m_nAtkPrev{ 0 };				//攻击前置
	int			m_nAtkInterval{ 0 };			//攻击间隔

	ATK_TYPE	m_enAtkType{ AT_PHYSICS };		//攻击类型

	int			m_nAtkBuildingDamage{ 0 };		//攻击建筑伤害

	TARGET_TYPE m_enNormalAtk{ TT_NORMAL };		//普通状态攻击目标
	TARGET_TYPE m_enHideAtk{ TT_NORMAL };		//隐身状态攻击目标
	TARGET_TYPE m_enSpecialAtk{ TT_NORMAL };	//特殊状态攻击目标

	std::vector<int> m_vctPassSkills;			//被动技能
};

