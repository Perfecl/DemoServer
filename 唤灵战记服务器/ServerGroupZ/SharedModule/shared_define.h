#pragma once

const size_t SIZE_0{ 0 };

//阵营
enum Force
{
	UNKNOWN = -1,
	ATK,
	DEF,
	NEUTRAL
};

enum class StageType
{
	UNKNOWN,
	NORMAL,			//普通关卡
	HARD,			//困难关卡
	NIGHTMARE,		//噩梦关卡
	ELITE,			//精英关卡
	MULTI			//多人关卡
};

//战斗模式
enum class BattleMode
{
	NORMAL,				//普通战斗  (公平)

	ARENA1v1,			//竞技场1v1 (公平)
	ARENA3v3 = 3,		//竞技场3v3	(公平)

	EXERCISE,			//跨服练习
	WAR,				//跨服争霸

	STAGE,				//关卡
	MULTISTAGE,			//多人关卡
	ELITESTAGE,			//精英关卡
	SPEED,				//竞速赛
	ARENA1v1Easy,		//首次秒进,1300分以下
	ARENA1v1Normal,		//1600以下，等待30秒（默认1500分）
	ARENA3v3Easy,		//首次秒进,1300分以下
	ARENA3v3Normal		//1600以下，等待30秒（默认1500分）
};

//是否是公平模式
inline bool IsFairMode(BattleMode mode)
{
	if ((BattleMode::NORMAL == mode) ||
		(BattleMode::EXERCISE == mode) ||
		(BattleMode::ARENA1v1 == mode) ||
		(BattleMode::ARENA3v3 == mode) ||
		(BattleMode::SPEED == mode))
	{
		return true;
	}

	return false;
}
//是否跨服
inline bool IsCrossRealm(BattleMode mode)
{
	if ((BattleMode::ARENA1v1 == mode) ||
		(BattleMode::ARENA3v3 == mode) ||
		(BattleMode::ARENA1v1Easy == mode) ||
		(BattleMode::ARENA1v1Normal == mode) ||
		(BattleMode::ARENA3v3Easy == mode) ||
		(BattleMode::ARENA3v3Normal == mode) ||
		(BattleMode::EXERCISE == mode) ||
		(BattleMode::WAR == mode))
	{
		return true;
	}

	return false;
}

//是否是关卡
inline bool IsStage(BattleMode mode)
{
	if ((BattleMode::STAGE == mode) ||
		(BattleMode::MULTISTAGE == mode) ||
		(BattleMode::ELITESTAGE == mode) ||
		(BattleMode::SPEED == mode))
	{
		return true;
	}

	return false;
}

//士兵科技
enum SoldierTechnology
{
	kSoldierTechAtk,			//攻击科技
	kSoldierTechMatk,			//魔攻科技
	kSoldierTechDef,			//防御科技
	kSoldierTechMdef,			//魔防科技
	kSoldierTechHP,				//血量科技
	kSoldierTechCount
};

//装备颜色
enum EquipRankColor
{
	kEquipRankWhite,
	kEquipRankGreen,
	kEquipRankBlue,
	kEquipRnakPurple,
	kEquipRankOrange,
	kEquipRankRed,
};