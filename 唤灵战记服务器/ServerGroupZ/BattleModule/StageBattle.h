#pragma once
#include "Battle.h"

class CStageBattle : public CBattle
{
public:
	CStageBattle();
	~CStageBattle();

	virtual void SetBattleByPto(const pto_BATTLE_S2B_REQ_CreateBattle_Ex* pPtoMaster) override;
	virtual void SetAIBattleByPto(const pto_BATTLE_S2B_REQ_CreateBattle_Ex* pPto) override{}
	virtual void Initialize() override;

	virtual void SendStageData(pto_BATTLE_S2C_NTF_SendBattleGroundData *pto) override;

	float       FindStageSoldierLinePos(Force enForce, bool bIncludeHide /*= false*/, bool bIncludeHero/*=false*/) const;

	void		OpenTimer(int time, std::string describe);
	void		CloseTimer();

	int	GetMapID() const;

private:
	const CEditMap*  m_pEditMap{ nullptr };

	std::vector<std::pair<const CIncident*, bool>>	m_vctIncidents;
	std::vector<std::pair<const int, bool>>         m_vctMapVariable;

	bool		__IsConditionHp(const CIncident* pIncident);
	bool		__IsConditionUnitPos(const CIncident* pIncident);
	void		__PlayStory(int nStoryId);
	void		__SendCancelMark();
	void		__ChangeMasterResourceOverride(int nMasterId, float fResourceOverride);
	void		__UnitGetBuff(int nGroupID, int nBuffID);
	void		__RemoveUnitBuff(int nGroupID, int nBuffID);
	void		__GetResource(Force enForce, const int nResource);
	void		__AddBackUp(int nGroupID, bool bBossEnter);
	void		__ShowNotice(std::string strNotice);
	void		__MasterTalk(int nMasterID, const std::string strTalk);
	int			__GetUnitID(int nHeroGroupID);
	int			__GetHeroID(int nHeroGroupID);
	void		__HeroTalk(int nHeroID, const std::string strTalk);
	void		__UnitRetreat(int nUnitGroupID);
	void		__StopRetreat(int nUnitGroupID);
	void		__LockPlayerHero();
	void		__UnlockPlayerHero();
	void		__LockPlayerSoldier();
	void		__UnlockPlayerSoldier();
	void		__RemoveUnit(int nUnitGroupID);
	void		__ResetHero();
	void		__ChangeUnitAIType(int nUnitGroupID, int nUnitAIType, float fUnitChangeAIRange);
	void        __OpenGate(int nGateID);
	void		__MasterDisappear(int nMasterID);
	bool		__IsNeedTimer(int nTimerID, int nTime);
	bool        __HeroUnavailable();
	int         __AllGroupUnit(int nGroup);
	void		__UnitDroupBullet(int nGroup, int nBulletID, int nBulletValue);
	void		__ProuduceBullet(int nPos, int nBulletID, int nBulletValu);
	void		__DeleteBullet(int nCustomID);
	void		__SetBackupMark(int pos);
	void		__CancelBackupMark();
	void		__ChangeBaseLevel(int master_id, int level);

	virtual void _EventLoop() override;
	virtual void _StartStory() override;		//¿ªÊ¼¹ÊÊÂ
};
