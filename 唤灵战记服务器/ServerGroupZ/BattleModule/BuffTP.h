#pragma once

enum BUFF_BASIC_EFFECT
{
	BBE_CMD = 1,					//统帅
	BBE_STR,						//力量
	BBE_INT,						//智力
	BBE_ATK_SPEED_UP,				//加攻击速度
	BBE_ATK_SPEED_DOWN,				//减攻击速度
	BBE_HP_V, 						//HP百分比
	BBE_HP_P,						//HP值
	BBE_DISTANCE,					//攻击距离
	BBE_CRIT,						//暴击率
	BBE_CRIT_VALUE, 				//暴击倍率
	BBE_ATK,						//造成物理Dot伤害
	BBE_MATK,						//造成魔法Dot伤害
	BBE_DEF,						//承受物理伤害
	BBE_MDEF,						//承受魔法伤害
	BBE_RICH,						//获得水晶变多
	BBE_EX_ATK =17,					//造成的物理伤害增加
	BBE_EX_MATK,					//造成的魔法伤害增加
	BEE_NERF_DEF,					//减少护甲
	BBE_END = 999					//结束
};

enum BUFF_ADVANCE_EFFECT
{
	BAE_INVINCIBLE = 1000,			//无敌
	BAE_STUN,						//眩晕
	BAE_FEAR,						//恐惧
	BAE_SILENCE,					//沉默
	BAE_HIDING,						//隐身
	BAE_MOVE_SPEED_UP,				//加速
	BAE_MOVE_SPEED_DOWN,			//减速
	BAE_MOVE_HOLD,					//禁锢
	BAE_SHUTTLE,					//穿梭
	BAE_BLIND,						//致盲
	BAE_END = 1999
};

class CBuffTP
{
	friend class CBuff;
public:
	static void Load();
	static const CBuffTP* GetBuffTP(int nID);

private:
	static std::map<int, const  CBuffTP*> ms_mapBuffs;

public:
	CBuffTP() = default;
	~CBuffTP() = default;

private:
	int									m_nID{ 0 };
	std::string							m_strName;
	int									m_nIcon{ 0 };
	std::vector<std::pair<int, int>>	m_vctEffects;
	std::vector<int>					m_vctAdvanceEffect;
	bool								m_bIsDebuff{ false };
	int									m_nInterval{ 0 };
	int									m_nTime{ 0 };
};
