#pragma once

struct SpeedStageReward
{
	int m_nRank{ 0 };
	int m_nGold{ 0 };
	float m_fCoefficient{ 0 };
};

class CSpeedStage
{
public:
	CSpeedStage();
	~CSpeedStage();

	static void Load();
	static int  GetGoldReward(int nRank);
	static float GetRewardCoefficient(int nRank);
	static int  ProduceSpeedStageID(int nOldId);
	static int  GetMapID(int nID);

private:
	static std::map<int, SpeedStageReward*> m_mapSpeedStageReward;
	static std::vector<std::pair<int, int>>	m_vctSpeedStage;
};

