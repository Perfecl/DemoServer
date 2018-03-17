#pragma once
class COfflineRewardStage
{
public:
	COfflineRewardStage();
	~COfflineRewardStage();
	static void Load();
	static const COfflineRewardStage* GetOfflineRewardStage(int nRank);

	int		GetStage() const { return m_nStage; }
	int		GetRank() const { return m_nRank;}
	float	GetStageReduce() const { return m_fStageReduce; }
	float	GetCoefficient() const { return m_fCoefficient; }

private:	
	static std::vector<COfflineRewardStage*> ms_vctOfflineRewardStage;
	int		m_nMinRank{ 0 };
	int		m_nMaxRank{ 0 };
	int		m_nStage{ 0 };
	int		m_nRank{ 0 };
	float	m_fStageReduce{ 0 };
	float	m_fCoefficient{ 0 };
};

