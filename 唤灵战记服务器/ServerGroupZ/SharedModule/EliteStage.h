#pragma once

class CEliteStage
{
	friend class CPlayerBattle;
	friend class CPlayer;
public:
	static void Load();
	static const CEliteStage*	GetEliteStageByMapID(int nMapID);
	static const CEliteStage* GetEliteStageByLevel(int nLevel);
	static const CEliteStage*	GetMultiStageByMapID(int nMapID);
	static const CEliteStage* GetMultiStageByLevel(int nLevel);

private:
	static std::map<int, const CEliteStage*> ms_mapEliteStages;
	static std::map<int, const CEliteStage*> ms_mapMultiStages;

public:
	CEliteStage();
	~CEliteStage();

	int GetLevel() const { return m_nLevel; }
	int GetOpenLevel() const { return m_nOpenLevel; }
	int GetMapID() const { return m_nMapID; }

	int ProduceBoxNum() const;
	int RewardBox() const{ return m_nRewardBox; }

private:
	std::array<float, 2>    m_arrProbability;
	int			       m_nLevel{ 0 };			   //关卡等级
	int				   m_nOpenLevel{ 0 };          //开放等级
	int			       m_nMapID{ 0 };			   //地图ID

	int                m_nRewardExp{ 0 };          //奖励经验
	int                m_nRewardSilver{ 0 };       //奖励金钱
	int                m_nRewardHonour{ 0 };       //奖励军功
	int                m_nRewardBox{ 0 };		
};

