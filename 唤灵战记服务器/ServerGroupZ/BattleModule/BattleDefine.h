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

const int RESOURCE_INCREASE_BASE = 30;				//��Դ��������
const int MAX_RESOURCE_BASE = 1000;					//��Դ����
const int MAX_POPULATION_BASE = 8;					//�˿ڻ���
const int POPULATION_INCREASE_BASE = 1;				//�˿���������

const int BE_DAMAGED_PROTECTED_TIME = 5000;			//�ܻ�����ʱ��
const int BE_DAMAGED_RESET_TIME = 3000;				//�ܻ��˺�����ʱ��

const size_t MAX_SOLDIER_NUM{ 6 };					//������ʿ������
const size_t MAX_HERO_NUM{ 3 };						//������Ӣ������
const size_t MAX_AI_POPULATION{ 20 };				//���AI�˿�

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
	AI_NOT_MOVE,							//���ƶ�
	AI_MOVE_AFTER_BE_ATK,					//�ܵ��������ƶ�
	AI_ALERT,								//���䣬���к�ǰ��
	AI_NORMAL,								//��ͨ
	AI_PATROL,								//Ѳ��
	AI_CHASE,                               //׷��
	AI_RETREAT,								//����
};

struct DamageInfo
{
	int  nDamage = 0;						//�����˺�			
	int	 nMDamage = 0;						//ħ���˺�

	int	 nAtkType = 0;						//��������

	int	 nStr = 0;							//����
	int	 nCmd = 0;							//ͳ˧
	int	 nInt = 0;							//����

	float fRatio = 1;						//�˺�����
	float fMagicRatio = 1;					//ħ���˺���
	float fPhysicsRatio = 1;				//�����˺���

	CBattleObject* pSrcObject = nullptr;	//�˺���Դ
	CMaster* master_ = nullptr;

	bool bIsRangeAtk = false;				//�Ƿ�Զ��
	bool bIsHeroAtk = false;				//�Ƿ�Ӣ��
	bool bIsSkill = false;					//�Ƿ���
	bool bIsArea = false;					//�Ƿ�Χ
	bool bIsBuliding = false;				//�Ƿ���
};

struct BulletSingleInfo
{
	CBattleObject* pObject{ nullptr };
	int	nInterval{ 0 };
	int nTimes{ 0 };
};

enum HeroDeadType
{
	HDT_Dead = 0,	//����
	HDT_Disappear,	//��ʧ
	HDT_DeadAndDisappear,
};
