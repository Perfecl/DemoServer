#pragma once

enum RootDataType
{
	RDT_TrainSpendHonour,
	RDT_RewardMissionExp,
	RDT_RewardMissionSilver,
	RDT_RewardMissionHonour,

	RDT_OfflineBattleWeekSilver,		//����ÿ�ܽ�������
	RDT_OfflineBattleWeekReputation,	//����ÿ�ܽ�������
	RDT_OfflineBattleSingleSilver,		//���ݵ���ʤ������

	RDT_EscortSilver,			//�̶�ս����
	RDT_EscortReputation,		//���ս����

	RDT_MeditationExp,		//��������

	RDT_ArenaSilver,	//���ս����
	RDT_ArenaHonour,	//���ս����

	RDT_FairSingleSilver,	//���ս��������
	RDT_FairSingleHonour,	//���ս��������

	RDT_SpeedSilver,		//����������
	RDT_SpeedHonour,		//����������

	RDT_WashRuneHonour,		//ϴ�����Ļ�������
	RDT_HeroLevelHonour,	//Ӣ��������������

	RDT_TrainSoldierSilver,//ѵ��ʿ������

	RDT_ExercisePlatformExp,

	RDT_MissionExp,
};

enum PlayerResourceType
{
	PRT_Silver = 1,
	PRT_Gold,
	PRT_Exp,
	PRT_Honour,
	PRT_Reputation,
	PRT_Stamina,
};

class CRootData
{
public:
	CRootData();
	~CRootData();
	static void Load();
	static const CRootData* GetRootData(int nLevel);
	static __int64 GetRootData(int nLevel, RootDataType enRootDataType);
	int GetTrainSpendHonour() const { return m_nTrainSpendHonour; }
	static int GetRewardMissionResource(int nLevel, int type);
	static __int64 GetTrainSoldierSilver(int level);
	__int64 levy_stage() const { return levy_stage_; }
	__int64 gold_stone_silver() const { return gold_stone_silver_; }
	__int64 castle_update_silver() const { return castle_update_silver_; }
	float exercise_platform_exp() const { return exercise_platform_exp_; }

private:
	static std::map<int, const CRootData*> ms_mapRootData;
	int m_nLevel{ 0 };
	int m_nTrainSpendHonour{ 0 };
	int m_nRewardMissionExp{ 0 };
	int m_nRewardMissionSilver{ 0 };
	int m_nRewardMissionHonour{ 0 };

	int m_nOfflineBattleWeekSilver{ 0 };		//����ÿ�ս�������
	int m_nOfflineBattleWeekReputation{ 0 };	//����ÿ�ս�������
	int m_nOfflineBattleSingleSilver{ 0 };		//���ݵ���ʤ������

	int m_nEscortSilver{ 0 };			//�̶�ս����
	int m_nEscortProtectHonour{ 0 };	//�̶�ս����

	int m_nMeditationExp{ 0 };			//��������

	__int64 m_nArenaSilver { 0 };	//���ս����
	int m_nArenaHonour { 0 };		//���ս����
	int m_nFairSingleSilver{ 0 };	//�����������
	int m_nFairSingleHonour{ 0 };	//�����������
	int m_nSpeedSilver { 0 };	//����������
	int m_nSpeedHonour{ 0 };	//����������

	int m_nWashRuneHonour{ 0 };		//ϴ�����Ļ�������
	int m_nHeroLevelHonour{ 0 };	//Ӣ��������������

	__int64 train_soldier_silver_{ 0 };	//ѵ��ʿ������

	__int64 levy_stage_{ 0 };
	__int64 gold_stone_silver_{ 0 };	//���������
	__int64 castle_update_silver_{ 0 };	//����������Ҫ��Ǯ

	float exercise_platform_exp_{ 0 };

	__int64 mission_exp_{ 0 };
};

