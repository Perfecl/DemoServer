#pragma once

struct HeroCard;
class  CBattleServer;
class  CHero;
class  CBattleObject;
class  CMaster;
class  CBattle;
class  CBuilding;
class  CUnit;
class  CBullet;
class  CBulletTP;
class  CIncident;
class  CEditMap;
class  CIncident;
class  CBuffTP;
class  CBuff;
class  CGate;

const int BATTLE_LOOP_TIME = 10;

const int RESOURCE_INCREASE_BASE = 30;				//资源增长基数
const int MAX_RESOURCE_BASE = 1000;					//资源基数
const int MAX_POPULATION_BASE = 8;					//人口基数
const int POPULATION_INCREASE_BASE = 1;				//人口增长基数

const int BE_DAMAGED_PROTECTED_TIME = 5000;			//受击保护时间
const int BE_DAMAGED_RESET_TIME = 3000;				//受击伤害重置时间

const size_t MAX_SOLDIER_NUM{ 6 };					//最大军阵士兵数量
const size_t MAX_HERO_NUM{ 3 };						//最大军阵英雄数量
const size_t MAX_AI_POPULATION{ 20 };				//最大AI人口

enum UnitState
{
	US_STAND,
	US_MOVING,
	US_ATK,
	US_SKILL,
	US_DEAD
};

enum BattleState
{
	BtSt_NULL,
	BtSt_Ready,
	BtSt_Run,
	BtSt_Pause,
	BtSt_Over
};

enum BuildingType
{
	BUILDING_TYPE_NULL = -1,
	BUILDING_TYPE_ATKCASTLE,
	BUILDING_TYPE_DEFCASTLE,
	BUILDING_TYPE_ATKTOWER,
	BUILDING_TYPE_DEFTOWER,
	BUILDING_TYPE_CRYSTAL_TOWER,
	BULIDING_TYPE_NEUTRAL_BULIDING,
};

enum BulidingState
{
	BUILDING_STATE_NORMAL,
	BUILDING_STATE_DESTROYED,
	BUILDING_STATE_REPAIRING
};

enum ObjectType
{
	OT_NULL = -1,
	OT_UNIT,
	OT_BUILDING,
	OT_BULLET
};

enum SkillType
{
	ST_ACT_SKILL,
	ST_PASS_SKILL
};

enum AIType
{
	AI_NOT_MOVE,							//不移动
	AI_MOVE_AFTER_BE_ATK,					//受到攻击后移动
	AI_ALERT,								//警戒，遇敌后前进
	AI_NORMAL,								//普通
	AI_PATROL,								//巡逻
	AI_CHASE,                               //追击
	AI_RETREAT,								//撤退
};

struct DamageInfo
{
	int  nDamage = 0;						//物理伤害			
	int	 nMDamage = 0;						//魔法伤害

	int	 nAtkType = 0;						//攻击类型

	int	 nStr = 0;							//力量
	int	 nCmd = 0;							//统帅
	int	 nInt = 0;							//智力

	float fRatio = 1;						//伤害比率
	float fMagicRatio = 1;					//魔法伤害比
	float fPhysicsRatio = 1;				//物理伤害比

	CBattleObject* pSrcObject = nullptr;	//伤害来源
	CMaster* master_ = nullptr;

	bool bIsRangeAtk = false;				//是否远程
	bool bIsHeroAtk = false;				//是否英雄
	bool bIsSkill = false;					//是否技能
	bool bIsArea = false;					//是否范围
	bool bIsBuliding = false;				//是否建筑
};

struct BulletSingleInfo
{
	CBattleObject* pObject{ nullptr };
	int	nInterval{ 0 };
	int nTimes{ 0 };
};

enum HeroDeadType
{
	HDT_Dead = 0,	//死亡
	HDT_Disappear,	//消失
	HDT_DeadAndDisappear,
};
