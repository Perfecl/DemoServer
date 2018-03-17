#pragma once

const size_t SIZE_0{ 0 };

//��Ӫ
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
	NORMAL,			//��ͨ�ؿ�
	HARD,			//���ѹؿ�
	NIGHTMARE,		//ج�ιؿ�
	ELITE,			//��Ӣ�ؿ�
	MULTI			//���˹ؿ�
};

//ս��ģʽ
enum class BattleMode
{
	NORMAL,				//��ͨս��  (��ƽ)

	ARENA1v1,			//������1v1 (��ƽ)
	ARENA3v3 = 3,		//������3v3	(��ƽ)

	EXERCISE,			//�����ϰ
	WAR,				//�������

	STAGE,				//�ؿ�
	MULTISTAGE,			//���˹ؿ�
	ELITESTAGE,			//��Ӣ�ؿ�
	SPEED,				//������
	ARENA1v1Easy,		//�״����,1300������
	ARENA1v1Normal,		//1600���£��ȴ�30�루Ĭ��1500�֣�
	ARENA3v3Easy,		//�״����,1300������
	ARENA3v3Normal		//1600���£��ȴ�30�루Ĭ��1500�֣�
};

//�Ƿ��ǹ�ƽģʽ
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
//�Ƿ���
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

//�Ƿ��ǹؿ�
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

//ʿ���Ƽ�
enum SoldierTechnology
{
	kSoldierTechAtk,			//�����Ƽ�
	kSoldierTechMatk,			//ħ���Ƽ�
	kSoldierTechDef,			//�����Ƽ�
	kSoldierTechMdef,			//ħ���Ƽ�
	kSoldierTechHP,				//Ѫ���Ƽ�
	kSoldierTechCount
};

//װ����ɫ
enum EquipRankColor
{
	kEquipRankWhite,
	kEquipRankGreen,
	kEquipRankBlue,
	kEquipRnakPurple,
	kEquipRankOrange,
	kEquipRankRed,
};