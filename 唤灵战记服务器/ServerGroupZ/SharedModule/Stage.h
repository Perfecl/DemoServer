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
	int			m_nLevel{ 0 };					//�ؿ��ȼ�
	int			m_nDifficulty{ 0 };				//�Ѷȵȼ�
	int			m_nMapID{ 0 };					//��ͼID
	std::string	m_nName;						//�ؿ�����
	std::string	m_nInfo;						//�ؿ���Ϣ
	int			m_nSceneID{ 0 };				//����ID
	int			m_nBossID{ 0 };					//bossͷ��ID
	int			m_nMusicID{ 0 };				//��������

	int			m_nFightingCapacity{ 0 };		//����ս����
	int         m_nUnlockLevel{ 0 };            //�����ȼ�����

	int			m_nRewardExp{ 0 };				//��������
	int			m_nRewardSilver{ 0 };			//������Ǯ
	int			m_nRewardHonor{ 0 };			//��������

	int			m_nRewardItem1ID{ 0 };			//��������1 ID
	int			m_nRewardItem1Num{ 0 };			//��������1 ����

	int			m_nRewardItem2ID{ 0 };			//��������2 ID
	int			m_nRewardItem2Num{ 0 };			//��������2 ����

	int			m_nRewardItem3ID{ 0 };			//��������3 ID
	int			m_nRewardItem3Num{ 0 };			//��������3 ����

	int			m_nRewardItem4ID{ 0 };			//��������4 ID
	int			m_nRewardItem4Num{ 0 };			//��������4 ����

	int			m_nRewardItem5ID{ 0 };			//��������5 ID
	int			m_nRewardItem5Num{ 0 };			//��������5 ����

	int			m_nRewardItem6ID{ 0 };			//��������6 ID
	int			m_nRewardItem6Num{ 0 };			//��������6 ����

	int			m_nBeginStoryID{ 0 };			//�ؿ�ǰ����ID
	int			m_nFinishStoryID{ 0 };			//�ؿ������ID
	int			m_nOpenFunctionID{ 0 };			//��������
};
