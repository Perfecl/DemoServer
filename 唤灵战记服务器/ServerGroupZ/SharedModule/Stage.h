#pragma once

class CStage
{
	friend class CBattle;
	friend class CPlayerBattle;
	friend class CPlayer;
public:
	static void				Load();
	static int				GetStagesNum(){ return ms_vctStages.size(); }
	static const CStage*	GetStage(int nMapID);
	static const CStage*	GetStage(int nLv, int nDifficult);
	static const CStage*	GetLastStage(){ return ms_vctStages[ms_vctStages.size() - 1]; }

private:
	static std::vector<const CStage*> ms_vctStages;

public:
	CStage() = default;
	~CStage() = default;

	int		GetLevel() const { return m_nLevel; }
	int     GetDifficulty() const { return m_nDifficulty; }
	int		GetUnlockLevel() const { return m_nUnlockLevel; }

private:
	int			m_nLevel{ 0 };					//关卡等级
	int			m_nDifficulty{ 0 };				//难度等级
	int			m_nMapID{ 0 };					//地图ID
	std::string	m_nName;						//关卡名称
	std::string	m_nInfo;						//关卡信息
	int			m_nSceneID{ 0 };				//背景ID
	int			m_nBossID{ 0 };					//boss头像ID
	int			m_nMusicID{ 0 };				//背景音乐

	int			m_nFightingCapacity{ 0 };		//建议战斗力
	int         m_nUnlockLevel{ 0 };            //解锁等级上限

	int			m_nRewardExp{ 0 };				//奖励经验
	int			m_nRewardSilver{ 0 };			//奖励金钱
	int			m_nRewardHonor{ 0 };			//奖励军功

	int			m_nRewardItem1ID{ 0 };			//奖励道具1 ID
	int			m_nRewardItem1Num{ 0 };			//奖励道具1 数量

	int			m_nRewardItem2ID{ 0 };			//奖励道具2 ID
	int			m_nRewardItem2Num{ 0 };			//奖励道具2 数量

	int			m_nRewardItem3ID{ 0 };			//奖励道具3 ID
	int			m_nRewardItem3Num{ 0 };			//奖励道具3 数量

	int			m_nRewardItem4ID{ 0 };			//奖励道具4 ID
	int			m_nRewardItem4Num{ 0 };			//奖励道具4 数量

	int			m_nRewardItem5ID{ 0 };			//奖励道具5 ID
	int			m_nRewardItem5Num{ 0 };			//奖励道具5 数量

	int			m_nRewardItem6ID{ 0 };			//奖励道具6 ID
	int			m_nRewardItem6Num{ 0 };			//奖励道具6 数量

	int			m_nBeginStoryID{ 0 };			//关卡前剧情ID
	int			m_nFinishStoryID{ 0 };			//关卡后剧情ID
	int			m_nOpenFunctionID{ 0 };			//开启功能
};
