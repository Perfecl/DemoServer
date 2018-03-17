#pragma once

class CIncident
{
public:
	CIncident() = default;
	~CIncident() = default;

	void	SetIncidentByProto(const Incident& pto);

	//事件开启条件
	bool	m_bIsConditionTime{ false };			//按时间开启
	int		m_nConditionTime{ 0 };					//开启时间

	bool	m_bIsConditionPos{ false };				//到达位置开启
	int		m_nConditionPosCamp{ 0 };				//需要到达位置的阵营
	float	m_fConditionPos{ 0 };					//需要到达的位置

	bool	m_bIsConditionResource{ false };		//按资源达要求开启
	int		m_nConditionResourceCamp{ 0 };			//资源要求的阵营
	int		m_nConditionResource{ 0 };				//需要的资源量

	bool	m_bIsConditionHP{ false };				//按血量开启
	int		m_nConditionHPGroup{ 0 };				//血量要求的单位
	int		m_nConditionHP{ 0 };					//要求的血量

	bool	is_call_hero_{ false };					//玩家召唤英雄
	//事件结果
	bool	m_bIsResultVictor{ false };				//取得胜利
	bool	m_bIsResultLose{ false };				//失败

	bool	m_bIsResultGetBuff{ false };			//获得BUFF
	int		m_nResultGetBuffGroup{ 0 };				//获得BUFF单位
	int		m_nResultGetBuffID{ 0 };				//获得BUFF ID

	bool	m_bIsResultRemoveBuff{ false };			//取消BUFF
	int		m_nResultRemoveBuffGroup{ 0 };			//取消BUFF单位
	int		m_nResultRemoveBuffID{ 0 };				//取消BUFF ID

	bool	m_bIsResultGetResource{ false };		//获得资源
	int		m_nResultGetResourceCamp{ 0 };			//获得资源阵营
	int		m_nResultGetResource{ 0 };				//获得资源量

	bool	m_bIsResultBackupArrive{ false };		//增援部队抵达
	int		m_nResultBackupArriveGroup{ 0 };		//增援部队号

	string	m_strIncidentName;

	bool	m_bIsResultShowNotice{ false };			//显示公告
	string	m_strShowNotice;						//公告内容

	bool	m_bIsResultMasterTalk{ false };			//指挥官谈话
	int		m_nMasterNum{ 0 };                      //指挥官位置
	string	m_strMasterTalk;						//指挥官谈话内容

	bool	m_bIsResultHeroTalk{ false };			//英雄谈话
	int		m_nHeroGroup{ 0 };                      //英雄组
	string	m_strHeroTalk;							//英雄谈话内容

	bool	m_bIsResultUnitRetreat{ false };		//单位撤退
	int		m_nUnitGroup{ 0 };                      //单位组
	float	m_fRetreatSpeed{ 0 };					//撤退速度

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

	bool		open_timer_{ false };		//开启计时器
	int			timer_time_{ 0 };		//计时器时间
	std::string	timer_describe_;	//计时器描述
	bool		close_timer_{ false };		//关闭计时器

	bool	set_backup_mark_{ false };
	int		backup_mark_pos_{ 0 };
	bool	cancel_backup_mark_{ false };

	bool	change_base_level_{ false };
	int		change_base_level_maseter_{ 0 };
	int		change_base_level_lv_{ 0 };
};
