#pragma once

enum RootDataType
{
	RDT_TrainSpendHonour,
	RDT_RewardMissionExp,
	RDT_RewardMissionSilver,
	RDT_RewardMissionHonour,

	RDT_OfflineBattleWeekSilver,		//天梯每周奖励银币
	RDT_OfflineBattleWeekReputation,	//天梯每周奖励声望
	RDT_OfflineBattleSingleSilver,		//天梯单场胜利银币

	RDT_EscortSilver,			//商队战银币
	RDT_EscortReputation,		//跨服战荣誉

	RDT_MeditationExp,		//打坐经验

	RDT_ArenaSilver,	//跨服战银币
	RDT_ArenaHonour,	//跨服战荣誉

	RDT_FairSingleSilver,	//跨服战单场银币
	RDT_FairSingleHonour,	//跨服战单场荣誉

	RDT_SpeedSilver,		//竞速赛银币
	RDT_SpeedHonour,		//竞速赛荣誉

	RDT_WashRuneHonour,		//洗练符文花费荣誉
	RDT_HeroLevelHonour,	//英雄升级花费荣誉

	RDT_TrainSoldierSilver,//训练士兵银币

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

	int m_nOfflineBattleWeekSilver{ 0 };		//天梯每日奖励银币
	int m_nOfflineBattleWeekReputation{ 0 };	//天梯每日奖励声望
	int m_nOfflineBattleSingleSilver{ 0 };		//天梯单场胜利银币

	int m_nEscortSilver{ 0 };			//商队战银币
	int m_nEscortProtectHonour{ 0 };	//商队战声望

	int m_nMeditationExp{ 0 };			//打坐经验

	__int64 m_nArenaSilver { 0 };	//跨服战银币
	int m_nArenaHonour { 0 };		//跨服战荣誉
	int m_nFairSingleSilver{ 0 };	//跨服单场银币
	int m_nFairSingleHonour{ 0 };	//跨服单场荣誉
	int m_nSpeedSilver { 0 };	//竞速赛银币
	int m_nSpeedHonour{ 0 };	//竞速赛荣誉

	int m_nWashRuneHonour{ 0 };		//洗练符文花费荣誉
	int m_nHeroLevelHonour{ 0 };	//英雄升级花费荣誉

	__int64 train_soldier_silver_{ 0 };	//训练士兵银币

	__int64 levy_stage_{ 0 };
	__int64 gold_stone_silver_{ 0 };	//点金获得银币
	__int64 castle_update_silver_{ 0 };	//升级基地需要的钱

	float exercise_platform_exp_{ 0 };

	__int64 mission_exp_{ 0 };
};

