#pragma once
#include "ProtoPackage.h"
#include "CustomBullet.h"

class CBattle
{
public:
	static CBattle*			NewBattle(BattleMode enMode);

	static CBattleServer*	GetBattleServer(){ return ms_pServerManager; }
	static void				SetBattleServer(CBattleServer* ptr){ ms_pServerManager = ptr; }

	static float			CalculateDistance(CBattleObject* pObj, CBattleObject* pTarget, bool bHasAtkRange, bool bHasAlertRange);		//����������λ֮��ľ���
	static float			IntersectionLine(float x1, float x2, float y1, float y2);													//�����ཻ 

protected:
	static CBattleServer*	ms_pServerManager;

	static void			_ClickHeroCard(CMaster *pMaster, const CMessage& msg);
	static void			_ClickSoldierCard(CMaster *pMaster, const CMessage& msg);
	static void			_HeroMove(CMaster *pMaster, const CMessage& msg);
	static void			_HeroMoveTo(CMaster *pMaster, const CMessage& msg);
	static void			_HeroStand(CMaster *pMaster, const CMessage& msg);
	static void			_UseActiveSkill(CMaster *pMaster, const CMessage& msg);
	static void			_UpgradeLevel(CMaster *pMaster, const CMessage& msg);
	static void			_LoadProcess(CMaster *pMaster, const CMessage& msg);
	static void			_Pause(CMaster *pMaster, bool bIsPause);
	static void			_Surrender(CMaster *pMaster);
	static void			_MasterLeave(CMaster *pMaster);
	static void			_UseBaseGun(CMaster *pMaster, const CMessage& msg);

public:
	CBattle();
	~CBattle();

	void  Loop();																				//ѭ��

	void					ProcessMessage(const CMessage& msg);

	virtual void			Initialize();														//��ʼ��	
	virtual void			GameOver(Force enLoseForce, float fPos);							//��Ϸ����
	void					InitBuilding();														//��ʼ������

	bool					TimeCounts(int nTime);												//������

	bool					AddObject(CUnit* pUnit, bool bSend = true);							//��ӵ�λ
	bool					AddObject(CBullet* pBullet, bool bSend = true);						//��ӵ�λ
	bool					AddObject(CBuilding *pBuilding);									//��ӽ���
	void					AddResource(Force enForce, int nNum, bool bAverage = false, bool is_crystal = false);		//�����Ŷ���Դ

	CMaster*				FindMasterByPID(int nPID) const;									//�����ٻ�ʦ
	CMaster*				FindMasterByMID(int nMasterID) const;								//�����ٻ�ʦ1-6
	CBattleObject*			FindEnemy(CUnit* pUnit, bool bHasAlertRange = false) const;			//Ѱ�ҹ���Ŀ��
	CBattleObject*			FindEnemyInRange(float x1, float x2, Force enForce) const;			//Ѱ�ҷ�Χ�еĵ�λ
	float					FindSoldiersLinePos(Force enForce, bool bIncludeHide = false, bool bIncludeHero = false, bool bIncludeUnit = false) const;	//Ѱ�ұ���
	float					FindBuidlingLinePos(Force enForce) const;							//Ѱ�ҽ�����
	float					FindHeroLinePos(Force enForce) const;

	float					FindSkillSoldierLinePos(CHero* pHero) const;
	float                   FindSkillHeroLinePos(CHero* pHero) const;
	float                   FindSkillBulidingLinePos(CHero* pHero) const;
	int                     GetSkillEnemyNum(float fRange, CHero* pHero) const;
	float					BreakHeroSkill(float fRange, CHero* pHero)const;

	bool					HitTestEnemyHero(CBattleObject *pObj) const;						//�ͶԷ�Ӣ������ײ���
	bool					HitTestEnemyBuilding(CBattleObject *pObj) const;					//�ͶԷ���������ײ���
	int						HitTestWallBullet(CBattleObject *pObj) const;						//��ǽ���ӵ�����ײ��� (0,δ��ײ  1.��ǽ���� 2.��ǽ����)

	void					SendBattleData();
	virtual void			SendStageData(pto_BATTLE_S2C_NTF_SendBattleGroundData *pto){};
	void					SendToMaster(CMaster* pMaster, const std::string& strContent, unsigned char nProType, int nProID, bool include_leave = false);
	void					SendToAllMaster(const std::string& strContent, unsigned char nProType, int nProID);
	void					SendOverMessage(Force enLoseForce);
	void					SendToForce(Force enForce, const std::string& strContent, unsigned char nProType, int nProID);

	virtual void			SetBattleByPto(const pto_BATTLE_S2B_REQ_CreateBattle_Ex* pPto);
	virtual void			SetAIBattleByPto(const pto_BATTLE_S2B_REQ_CreateBattle_Ex* pPto);
	void					SetBattleState(BattleState enState);

	void					SetSID(int nATK, int nDEF){ m_nAtkSID = nATK; m_nDefSID = nDEF; }

	BattleState				GetBattleState(){ return m_enState; }
	int						GetMasterNum() const;
	int						GetMasterNum(Force enForce, bool bIncludeLeave = true, bool bIncludeAI = true) const;
	BattleMode				GetBattleMode() const { return m_enMode; }
	int						GetBulidingNum() const;
	int						GetBulidingNum(Force enForce) const;
	int						GetUnitNum(bool bIncludeDead = true) const;
	int						GetUnitNum(Force enForce) const;
	float					GetRangeUnitRatio(Force enForce) const;
	int						GetBulletNum() const { return m_setBullets.size(); }
	Force					GetLoseForce() const { return m_enLoseForce; }
	float					GetWidth() const { return m_fWidth; }

	int						GetUnitID(){ return ++m_nUnitCounts; }
	int						GetBulletID(){ return ++m_nBulletCounts; }
	int						GetBuffID(){ return ++m_nBuffCounts; }
	std::map<int, CUnit*>*			GetUnitList(Force enForce);
	std::map<int, CUnit*>*			GetPlayerUnitList();
	std::map<int, CUnit*>*			GetPlayerHeroUnitList();
	std::map<int, CBuilding*>*		GetBulidingList(Force enForce);
	CProtoPackage*					GetProtoPackage(Force enForce);

	int						GetAverageDamage(Force enForce) const;
	CMaster*				FindGetMostResourceMaster(int &nNum) const;
	CMaster*				FindHitMostSoldierMaster(int &nNum) const;
	CMaster*				FindKillMostHeroesMaster(int &nNum) const;
	CMaster*				FindKillMostSoldiersMaster(int &nNum) const;
	CMaster*				FindGetMostCrystalMaster(int &nNum)	const;
	CMaster*				FindMakeMostDamageMaster(Force enForce, int &nNum) const;

	bool                    IsHeroDead(){ return m_bPlayerHeroDead; }
	bool                    IsHeroDisappear(){ return m_bPlayerHeroDisappear; }
	void                    SetHeroDead(bool bFlag){ m_bPlayerHeroDead = bFlag; }
	void                    SetHeroDisappear(bool bFlag){ m_bPlayerHeroDisappear = bFlag; }

	void					Pause();

	void                    AddGate(const CGate* pGate);

	bool					UsingHero(Force enForce);

	bool					HitTestGate(CBattleObject *pObj) const;
	float					CalculateGateDistance(CBattleObject* pObj, CGate* pGate, bool bHasAtkRange, bool bHasAlertRange) const;
	float					FindGateLinePos(Force enForce) const;
	float					GetGatePos(CUnit* pUnit) const;
	float					FindLinePos(Force enForce, bool bIncludeHide, bool bIncludeHero, bool bIncludeUnit) const;

	void					SetStageID(int nID){ m_nStageID = nID; }
	void					SetDifficulty(int nDifficulty){ m_nDifficulty = nDifficulty; }
	void					SetBoxNum(int nBoxNum){ m_nBoxNum = nBoxNum; }
	void 					SetMode(BattleMode enMode){ m_enMode = enMode; }
	void 					SetAtkBasesOver(bool bOver){ m_bAtkBaseOver = bOver; }
	void 					SetDefBasesOver(bool bOver){ m_bDefBaseOver = bOver; }
	bool 					GetAtkBaseOver(){ return m_bAtkBaseOver; }
	bool 					GetDefBaseOver(){ return m_bDefBaseOver; }
	void 					ProduceCustomBullet(const CCustomBullet* pCustomBullet, bool bSend);
	void 					ProuduceBullet(float fPos, int nBulletID, int nBulletValu);
	void					ProduceBaseGunBullet(CMaster* master);
	int 					GetAtkUnitNum(){ return m_mapAtkUnits.size(); }
	int						GetDefUnitNum(){ return m_mapDefUnits.size(); }
	int						GetRangeFriendlyUnitNum(CUnit* pUnit, int nRange);
	int 					GetFriendlyUnitNum(Force enForce){ if (enForce == Force::ATK) return m_mapAtkUnits.size(); if (enForce == Force::DEF) return m_mapDefUnits.size(); return 0; }

	float					GetWarpGate(Force enForce);			//��ȡ����������
	void					ClearWarpGate(CBullet* pInstance);	//����������

	bool					DefCastleHP(float remain_hp);
	bool					PrisonHP(Force force, float ramain_hp);

	int						stage_loot_times(){ return stage_loot_times_; }
	void					ChangeStageLootTimes(int times){ stage_loot_times_ += times; }
	void					GetRemainStageLoot();		//ʰȡս����ʣ�����

	bool					UnitHold(CUnit* unit);

	CBuilding*				GetAtkCastle() { return m_pAtkCastle; }			//����������
	CBuilding*				GetDefCastle() { return	m_pDefCastle; }			//���ط�����

	bool					atk_base_gun(){ return atk_base_gun_; }
	bool					def_base_gun(){ return def_base_gun_; }

	int						GetStageID(){ return m_nStageID; }

	void					ChargeBulletDie(CBattleObject* owner);

protected:
	const short				m_nReadyTime{ 1500 };				//׼��ʱ��
	bool					m_bIsReadyBeforePause{ false };		//��֮ͣǰ�Ƿ���׼��״̬

	float					m_fWidth{ 6300.0f };				//ս�����

	int						m_nID{ 0 };							//ս��ID
	int						m_nSceneID{ 0 };					//����ID
	int						m_nTimes{ 0 };						//ս������ʱ��
	BattleMode				m_enMode{ BattleMode::NORMAL };    //ս��ģʽ
	BattleState				m_enState{ BtSt_NULL };				//ս��״̬
	BattleState             m_enTempState{ BtSt_NULL };			//����ǰս��״̬
	bool					m_bDeadTime{ false };

	int                     m_nStageID{ 0 };                    //�ؿ�ID
	int                     m_nDifficulty{ 0 };                 //�ؿ��Ѷ�
	int                     m_nBoxNum{ 0 };                     //��Ӣ�ؿ�������������

	Force					m_enLoseForce{ Force::UNKNOWN };		//ʧ�ܷ���Ӫ

	std::map<int, int>			m_maSPPlayerLoadProcess;				//��Ҽ��ؽ���(MasterID,���Ȱٷֱ�)

	std::array<CMaster*, 6>		m_arrMasters;						//�ٻ�ʦ
	int							m_nAtkSID{ 0 };
	int							m_nDefSID{ 0 };
	int							m_nAtkPoint{ 0 };
	int							m_nDefPoint{ 0 };

	std::map<int, CUnit*>		m_mapAtkUnits;						//��������λ
	std::map<int, CUnit*>		m_mapDefUnits;						//���ط���λ
	std::map<int, CUnit*>		m_mapDeadUnits;						//������λ
	std::map<int, CUnit*>		m_maSPPlayerUnits;					//��ҵ�λ
	std::map<int, CUnit*>		m_maSPPlayerHeroUnits;				//���Ӣ�۵�λ

	CBuilding*					m_pAtkCastle{ nullptr };			//����������
	CBuilding*					m_pDefCastle{ nullptr };			//���ط�����
	std::map<int, CBuilding*>	m_mapAtkBuildings;					//����������
	std::map<int, CBuilding*>	m_mapDefBuildings;					//���ط�����
	std::map<int, CBuilding*>	m_mapNeutralBuildings;				//��������
	std::map<int, CGate*>		m_mapGates;							//��

	std::set<CBullet*>			m_setBullets;						//�ӵ�
	std::set<CBullet*>			m_setWallBullets;					//ǽ���ӵ�

	CBullet*					m_pAtkWarpGate{ nullptr };			//������������
	CBullet*					m_pDefWarpGate{ nullptr };			//���ط�������

	int							m_nUnitCounts{ 0 };					//��λ����
	int							m_nBulletCounts{ 0 };				//�ӵ�����
	int							m_nBuffCounts{ 0 };					//buff����0

	float						m_fGameOverPos{ 0 };				//��Ϸ��������

	bool						m_bPlayerHeroDead{ false };			//���Ӣ������
	bool						m_bPlayerHeroDisappear{ false };	//���Ӣ����ʧ

	bool						m_bAtkBaseOver{ false };
	bool						m_bDefBaseOver{ false };

	bool						atk_base_gun_{ true };
	bool						def_base_gun_{ true };

	bool						try_hero_{ false };
	int							try_hero_id_{ false };

	int							m_nEndStory{ -1 };
	std::map<int, int>			m_mapTimer;
	int							stage_loot_times_{ 0 };				//�������

	std::array<CProtoPackage*, 2> m_arrProtoPackages;				//��Э���([0]������λ [1]�ط���λ)

	virtual bool _CheckMaster();								//����ٻ�ʦ


	void _UnitLoop();											//��λLoop
	void _BulletLoop();											//�ӵ�Loop
	void _CheckLoadProcess();									//�����ؽ���
	void _Fighting();											//����ս��
	void _SendProtoPackage();									//���ʹ�Э���
	void _MasterSurrender(CMaster* master);

	virtual void _EventLoop(){}									//�¼�ѭ��
	virtual void _StartStory(){};								//��ʼ����
};
