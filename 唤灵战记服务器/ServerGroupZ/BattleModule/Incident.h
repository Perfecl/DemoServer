#pragma once

class CIncident
{
public:
	CIncident() = default;
	~CIncident() = default;

	void	SetIncidentByProto(const Incident& pto);

	//�¼���������
	bool	m_bIsConditionTime{ false };			//��ʱ�俪��
	int		m_nConditionTime{ 0 };					//����ʱ��

	bool	m_bIsConditionPos{ false };				//����λ�ÿ���
	int		m_nConditionPosCamp{ 0 };				//��Ҫ����λ�õ���Ӫ
	float	m_fConditionPos{ 0 };					//��Ҫ�����λ��

	bool	m_bIsConditionResource{ false };		//����Դ��Ҫ����
	int		m_nConditionResourceCamp{ 0 };			//��ԴҪ�����Ӫ
	int		m_nConditionResource{ 0 };				//��Ҫ����Դ��

	bool	m_bIsConditionHP{ false };				//��Ѫ������
	int		m_nConditionHPGroup{ 0 };				//Ѫ��Ҫ��ĵ�λ
	int		m_nConditionHP{ 0 };					//Ҫ���Ѫ��

	bool	is_call_hero_{ false };					//����ٻ�Ӣ��
	//�¼����
	bool	m_bIsResultVictor{ false };				//ȡ��ʤ��
	bool	m_bIsResultLose{ false };				//ʧ��

	bool	m_bIsResultGetBuff{ false };			//���BUFF
	int		m_nResultGetBuffGroup{ 0 };				//���BUFF��λ
	int		m_nResultGetBuffID{ 0 };				//���BUFF ID

	bool	m_bIsResultRemoveBuff{ false };			//ȡ��BUFF
	int		m_nResultRemoveBuffGroup{ 0 };			//ȡ��BUFF��λ
	int		m_nResultRemoveBuffID{ 0 };				//ȡ��BUFF ID

	bool	m_bIsResultGetResource{ false };		//�����Դ
	int		m_nResultGetResourceCamp{ 0 };			//�����Դ��Ӫ
	int		m_nResultGetResource{ 0 };				//�����Դ��

	bool	m_bIsResultBackupArrive{ false };		//��Ԯ���ӵִ�
	int		m_nResultBackupArriveGroup{ 0 };		//��Ԯ���Ӻ�

	string	m_strIncidentName;

	bool	m_bIsResultShowNotice{ false };			//��ʾ����
	string	m_strShowNotice;						//��������

	bool	m_bIsResultMasterTalk{ false };			//ָ�ӹ�̸��
	int		m_nMasterNum{ 0 };                      //ָ�ӹ�λ��
	string	m_strMasterTalk;						//ָ�ӹ�̸������

	bool	m_bIsResultHeroTalk{ false };			//Ӣ��̸��
	int		m_nHeroGroup{ 0 };                      //Ӣ����
	string	m_strHeroTalk;							//Ӣ��̸������

	bool	m_bIsResultUnitRetreat{ false };		//��λ����
	int		m_nUnitGroup{ 0 };                      //��λ��
	float	m_fRetreatSpeed{ 0 };					//�����ٶ�

	vector<int> m_vctConditionTrueVariableID;
	vector<int> m_vctConditionFalseVariableID;

	vector<int> m_vctChangeTrueVariableID;
	vector<int> m_vctChangeFalseVariableID;

	bool	m_bIsConditionUnitPos{ false };
	float	m_fUnitPos{ 0 };
	int		m_nUnitPosGroup{ 0 };

	bool	m_bIsChangResourceOverride{ false };
	int		m_nMasterId{ 0 };
	float	m_fResourceOverride{ 0 };

	bool	m_bIsBaseLv{ false };
	int		m_nBaseLvMaster{ 1 };
	int		m_nBaseLv{ 1 };

	bool	m_bIsStopRetreat{ false };
	int		m_nStopRetratGroup{ -1 };

	bool	m_bIsPlayeStory{ false };
	int		m_nStoryId{ -1 };

	bool	m_bIsCancelMark{ false };

	bool	m_bResetHero{ false };
	bool	m_bLockHero{ false };
	bool	m_bUnlockHero{ false };
	bool	m_bLockSoldier{ false };
	bool	m_bUnlockSoldier{ false };
	bool	m_bRemoveUnit{ false };
	int		m_nRemoveUnitGroup{ false };

	bool	m_bHeroDead{ false };
	bool	m_bHeroDisappear{ false };

	bool	m_bChangeAI{ false };
	int		m_nChangeAIGroup{ 0 };
	int		m_nChangeAIType{ 0 };
	int		m_nChangeAIRange{ 0 };

	bool    m_bOpenGate{ false };
	int     m_nOpenGateID{ 0 };

	bool    m_bHeroUnavailable{ false };

	bool	m_bMasterDisappear{ false };
	int		m_nMasterDisappearID{ 0 };

	bool	m_bBeginTimer{ false };
	int		m_nBeginTimerID{ 0 };
	int		m_bNeedTimer{ false };
	int		m_nNeedTimerID{ 0 };
	int		m_nNeedTimerTime{ 0 };

	bool	m_bUnitDropBullet{ false };
	int		m_nDropBulletGroup{ 0 };
	int		m_nDropBulletId{ 0 };
	int		m_nBulletValue{ 0 };

	bool	m_bDeleteBullet{ false };
	int		m_nDeleteBulletCustomID{ 0 };

	bool	m_bBossEnter{ false };

	bool		open_timer_{ false };		//������ʱ��
	int			timer_time_{ 0 };		//��ʱ��ʱ��
	std::string	timer_describe_;	//��ʱ������
	bool		close_timer_{ false };		//�رռ�ʱ��

	bool	set_backup_mark_{ false };
	int		backup_mark_pos_{ 0 };
	bool	cancel_backup_mark_{ false };

	bool	change_base_level_{ false };
	int		change_base_level_maseter_{ 0 };
	int		change_base_level_lv_{ 0 };
};
