#pragma once
class CCombinationReward
{
public:
	CCombinationReward();
	~CCombinationReward();
	static void Load();
	static const long long GetCombinationReward(int nLV, int nCardNum);

private:
	static std::map<int, const CCombinationReward*> ms_mapCombinationReward;
	int		m_nLevel{ 0 };
	__int64 m_nCards_2{ 0 };
	__int64 m_nCards_3{ 0 };
	__int64 m_nCards_4{ 0 };
};

