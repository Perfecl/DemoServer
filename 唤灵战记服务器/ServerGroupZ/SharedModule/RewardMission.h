#pragma once

enum RewardMissionType
{
	RMT_Null = 0,
	RMT_SweepStage,			//ɨ��������ҵ�ǰ��ͼ������ߵ�6���ؿ��������ȡһ��ɨ��һ�Σ�
	RMT_UpgradeEquip,       //ǿ����������װ��ǿ��һ�Σ�
	RMT_Train,				//ϴ�����������佫ϴ����һ�Σ�
	RMT_UpgradeTechnology,  //�Ƽ���������Ƽ�����һ�Σ�
	RMT_UseGold,            //������������5�ƽ�
	RMT_OfflineBattle,		//�������񣺽���һ������ս����
	RMT_Escort,             //Ѻ�����񣺽���һ��Ѻ�ڣ�
	RMT_Arena,				//�������񣺽���һ�ξ�����ս����
	RMT_Guild,              //������񣺻�ȡ��ṱ��10�㣻
};

enum RewardMissionState
{
	RMS_New = 0,				//δ��
	RMS_Accept,					//�ѽ�
	RMS_Complete,				//�ѽ�
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

