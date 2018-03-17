#pragma once
#include "ProtoPackage.h"
#include "CustomBullet.h"

class CBattle
{
public:
	static CBattle*			NewBattle(BattleMode enMode);

	static CBattleServer*	GetBattleServer(){ return ms_pServerManager; }
	static void				SetBattleServer(CBattleServer* ptr){ ms_pServerManager = ptr; }

	static float			CalculateDistance(CBattleObject* pObj, CBattleObject* pTarget, bool bHasAtkRange, bool bHasAlertRange);		//计算两个单位之间的距离
	static float			IntersectionLine(float x1, float x2, float y1, float y2);													//两线相交 

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

	void  Loop();																				//循环

	void					ProcessMessage(const CMessage& msg);

	virtual void			Initialize();														//初始化	
	virtual void			GameOver(Force enLoseForce, float fPos);							//游戏结束
	void					InitBuilding();														//初始化建筑

	bool					TimeCounts(int nTime);												//计数器

	bool					AddObject(CUnit* pUnit, bool bSend = true);							//添加单位
	bool					AddObject(CBullet* pBullet, bool bSend = true);						//添加单位
	bool					AddObject(CBuilding *pBuilding);									//添加建筑
	void					AddResource(Force enForce, int nNum, bool bAverage = false, bool is_crystal = false);		//增加团队资源

	CMaster*				FindMasterByPID(int nPID) const;									//查找召唤师
	CMaster*				FindMasterByMID(int nMasterID) const;								//查找召唤师1-6
	CBattleObject*			FindEnemy(CUnit* pUnit, bool bHasAlertRange = false) const;			//寻找攻击目标
	CBattleObject*			FindEnemyInRange(float x1, float x2, Force enForce) const;			//寻找范围中的单位
	float					FindSoldiersLinePos(Force enForce, bool bIncludeHide = false, bool bIncludeHero = false, bool bIncludeUnit = false) const;	//寻找兵线
	float					FindBuidlingLinePos(Force enForce) const;							//寻找建筑线
	float					FindHeroLinePos(Force enForce) const;

	float					FindSkillSoldierLinePos(CHero* pHero) const;
	float                   FindSkillHeroLinePos(CHero* pHero) const;
	float                   FindSkillBulidingLinePos(CHero* pHero) const;
	int                     GetSkillEnemyNum(float fRange, CHero* pHero) const;
	float					BreakHeroSkill(float fRange, CHero* pHero)const;

	bool					HitTestEnemyHero(CBattleObject *pObj) const;						//和对方英雄做碰撞检测
	bool					HitTestEnemyBuilding(CBattleObject *pObj) const;					//和对方建筑做碰撞检测
	int						HitTestWallBullet(CBattleObject *pObj) const;						//和墙类子弹做碰撞检测 (0,未碰撞  1.在墙靠左 2.在墙靠右)

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

	float					GetWarpGate(Force enForce);			//获取传送门坐标
	void					ClearWarpGate(CBullet* pInstance);	//消除传送门

	bool					DefCastleHP(float remain_hp);
	bool					PrisonHP(Force force, float ramain_hp);

	int						stage_loot_times(){ return stage_loot_times_; }
	void					ChangeStageLootTimes(int times){ stage_loot_times_ += times; }
	void					GetRemainStageLoot();		//拾取战场上剩余掉落

	bool					UnitHold(CUnit* unit);

	CBuilding*				GetAtkCastle() { return m_pAtkCastle; }			//进攻方主堡
	CBuilding*				GetDefCastle() { return	m_pDefCastle; }			//防守方主堡

	bool					atk_base_gun(){ return atk_base_gun_; }
	bool					def_base_gun(){ return def_base_gun_; }

	int						GetStageID(){ return m_nStageID; }

	void					ChargeBulletDie(CBattleObject* owner);

protected:
	const short				m_nReadyTime{ 1500 };				//准备时间
	bool					m_bIsReadyBeforePause{ false };		//暂停之前是否是准备状态

	float					m_fWidth{ 6300.0f };				//战场宽度

	int						m_nID{ 0 };							//战斗ID
	int						m_nSceneID{ 0 };					//场景ID
	int						m_nTimes{ 0 };						//战斗经过时间
	BattleMode				m_enMode{ BattleMode::NORMAL };    //战斗模式
	BattleState				m_enState{ BtSt_NULL };				//战斗状态
	BattleState             m_enTempState{ BtSt_NULL };			//保存前战斗状态
	bool					m_bDeadTime{ false };

	int                     m_nStageID{ 0 };                    //关卡ID
	int                     m_nDifficulty{ 0 };                 //关卡难度
	int                     m_nBoxNum{ 0 };                     //精英关卡奖励宝箱数量

	Force					m_enLoseForce{ Force::UNKNOWN };		//失败方阵营

	std::map<int, int>			m_maSPPlayerLoadProcess;				//玩家加载进度(MasterID,进度百分比)

	std::array<CMaster*, 6>		m_arrMasters;						//召唤师
	int							m_nAtkSID{ 0 };
	int							m_nDefSID{ 0 };
	int							m_nAtkPoint{ 0 };
	int							m_nDefPoint{ 0 };

	std::map<int, CUnit*>		m_mapAtkUnits;						//进攻方单位
	std::map<int, CUnit*>		m_mapDefUnits;						//防守方单位
	std::map<int, CUnit*>		m_mapDeadUnits;						//死亡单位
	std::map<int, CUnit*>		m_maSPPlayerUnits;					//玩家单位
	std::map<int, CUnit*>		m_maSPPlayerHeroUnits;				//玩家英雄单位

	CBuilding*					m_pAtkCastle{ nullptr };			//进攻方主堡
	CBuilding*					m_pDefCastle{ nullptr };			//防守方主堡
	std::map<int, CBuilding*>	m_mapAtkBuildings;					//进攻方建筑
	std::map<int, CBuilding*>	m_mapDefBuildings;					//防守方建筑
	std::map<int, CBuilding*>	m_mapNeutralBuildings;				//中立建筑
	std::map<int, CGate*>		m_mapGates;							//门

	std::set<CBullet*>			m_setBullets;						//子弹
	std::set<CBullet*>			m_setWallBullets;					//墙类子弹

	CBullet*					m_pAtkWarpGate{ nullptr };			//进攻方传送门
	CBullet*					m_pDefWarpGate{ nullptr };			//防守方传送门

	int							m_nUnitCounts{ 0 };					//单位计数
	int							m_nBulletCounts{ 0 };				//子弹计数
	int							m_nBuffCounts{ 0 };					//buff计数0

	float						m_fGameOverPos{ 0 };				//游戏结束坐标

	bool						m_bPlayerHeroDead{ false };			//玩家英雄死亡
	bool						m_bPlayerHeroDisappear{ false };	//玩家英雄消失

	bool						m_bAtkBaseOver{ false };
	bool						m_bDefBaseOver{ false };

	bool						atk_base_gun_{ true };
	bool						def_base_gun_{ true };

	bool						try_hero_{ false };
	int							try_hero_id_{ false };

	int							m_nEndStory{ -1 };
	std::map<int, int>			m_mapTimer;
	int							stage_loot_times_{ 0 };				//掉落次数

	std::array<CProtoPackage*, 2> m_arrProtoPackages;				//大协议包([0]攻方单位 [1]守方单位)

	virtual bool _CheckMaster();								//检查召唤师


	void _UnitLoop();											//单位Loop
	void _BulletLoop();											//子弹Loop
	void _CheckLoadProcess();									//检查加载进度
	void _Fighting();											//进行战斗
	void _SendProtoPackage();									//发送大协议包
	void _MasterSurrender(CMaster* master);

	virtual void _EventLoop(){}									//事件循环
	virtual void _StartStory(){};								//开始故事
};
