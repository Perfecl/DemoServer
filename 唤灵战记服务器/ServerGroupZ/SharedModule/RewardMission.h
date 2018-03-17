#pragma once

enum RewardMissionType
{
	RMT_Null = 0,
	RMT_SweepStage,			//扫荡任务：玩家当前推图进度最高的6个关卡，随机抽取一个扫荡一次；
	RMT_UpgradeEquip,       //强化任务：任意装备强化一次；
	RMT_Train,				//洗将任务：任意武将洗属性一次；
	RMT_UpgradeTechnology,  //科技任务：任意科技升级一次；
	RMT_UseGold,            //消费任务：消耗5黄金；
	RMT_OfflineBattle,		//天梯任务：进行一次天梯战斗；
	RMT_Escort,             //押镖任务：进行一次押镖；
	RMT_Arena,				//竞技任务：进行一次竞技场战斗；
	RMT_Guild,              //帮会任务：获取帮会贡献10点；
};

enum RewardMissionState
{
	RMS_New = 0,				//未接
	RMS_Accept,					//已接
	RMS_Complete,				//已交
};

struct SRewardMission
{
	int id_{ 0 };
	RewardMissionType m_enType{ RMT_Null };
	int mission_target_num_{ 0 };
	int m_nRewardType{ 0 };
	float m_fRewardMul{ 0 };
	int condition_{ 0 };
	int value_{ 0 };
};

struct SRewardMissionRank
{
	int		m_nRank{ 0 };
	float	m_fProbability{ 0 };
	float	m_fMul{ 0 };
};

struct SRewardMissionResource
{
	int m_nType{ 0 };
	int m_nFixedValue{ 0 };
};

class CRewardMission
{
public:
	CRewardMission() = default;
	~CRewardMission() = default;

	static void Load();
	static const SRewardMission* GetRewardMission(int id);
	static int ProduceRewardMissionRank();
	static RewardMissionType ProduceRewardMissionType(int level, int main_progress);
	static const SRewardMission* GetRewardMissionStruce(RewardMissionType enType);
	static int GetRewardMissionTargetNum(RewardMissionType enType);
	static float GetRewardMissionMul(RewardMissionType enType);
	static float GetRankMul(int nRank);
	static int GetFixedValue(int type);

private:
	static std::map<int, const  SRewardMission*> ms_mapRewardMission;
	static std::map<int, const  SRewardMissionRank*> ms_mapRewardMissionRank;
	static std::map<int, const  SRewardMissionResource*> ms_mapRewardMissionResource;
	static std::vector<int>	ms_vctMissionType;
};

