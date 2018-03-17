#pragma once

const int base_gun_active_time = 3000;

enum UpgradeLevelType
{
	kMilitary,
	kEconomy,
};

struct HeroCard
{
public:
	const CHeroTP*	m_pHeroTP{ nullptr };

	bool			m_bUsed{ false };

	int		level_{ 0 };				//等级
	int		color_{ 0 };				//颜色
	int		atk_{ 0 };					//攻击
	int		matk_{ 0 };					//魔攻
	int		def_{ 0 };					//防御
	int		mdef_{ 0 };					//魔防
	int		hp_{ 0 };					//血量
	float	move_speed_{ 0 };			//移动速度
	int		prev_atk_{ 0 };				//攻击前摇
	int		atk_interval_{ 0 };			//攻击间隔
	int		exsits_time_{ 0 };			//持续时间
	float	crit_odds_{ 0 };			//爆击几率
	float	crit_value_{ 0 };			//爆击伤害
	float	crit_resistance_{ 0 };		//爆击抗性
	int		strength_{ 0 };				//力量
	int		command_{ 0 };				//统率
	int		intelligence_{ 0 };			//智力
	int		building_atk_{ 0 };			//建筑攻击力
	int		suit_effect_{ 0 };			//套装效果
};

struct SoldierCard
{
	//(ID, CD, NUM)
public:
	int				m_nCardID{ 0 };
	int				m_nCoolDown{ 0 };
	int				m_nCanUseNum{ 0 };
};

struct SoldierSoul
{
	int soldier_id_;	//士兵ID
	int soul_level_;	//士兵灵能等级
	int soul_exp_;		//士兵灵能层数
	int train_level_;	//士兵训练等级
};

class CMaster
{
public:
	static const CMaster*  CreateCustomMaster(const MasterInfoDat& info);

public:
	CMaster(CBattle *pBattle);
	~CMaster();

	void		Loop();

	void		SetProtocol(pto_BATTLE_Struct_Master *pPtoMaster);

	void		SetMasterID(int nID){ m_nID = nID; }
	void		SetMasterByPto(const pto_BATTLE_STRUCT_Master_Ex* pPtoMaster, Force force);
	void		SetArenaAI(BattleMode enMode);

	void		SetForce(Force enForce){ m_enForce = enForce; }
	void		SetUsingHero(CHero* pHero){ m_pUsingHero = pHero; }
	void		SetIsLeave(bool bIsLeave){ m_bIsLeave = bIsLeave; }
	void		SetIsAI(bool bIsAI){ m_bIsAI = bIsAI; }
	void		AddMark(int x){ m_nMark |= x; }

	Force		GetForce() const { return m_enForce; }
	CBattle*	GetBattle() const { return m_pBattle; }
	int			GetPID() const { return m_nPID; }
	int			GetSID() const { return m_nSID; }
	int			GetMasterID() const { return m_nID; }
	CHero*		GetUsingHero() const { return m_pUsingHero; }
	int			GetMark() const { return m_nMark; }
	int			GetMarkNum() const;

	int			GetAllResource() const { return m_nAllResource; }
	int			GetAllHitSoldier() const { return m_nAllHitSoldier; }
	int			GetAllKillHeroes() const { return m_nAllKillHeroes; }
	int			GetAllKillSoldiers() const { return m_nAllKillSoldiers; }
	int			GetAllMakeDamage() const { return m_nAllMakeDamage; }
	int			GetAllGetCrystal() const { return m_nAllGetCrystal; }

	const std::string& GetName() const { return m_strName; }
	int			GetHeroNum() const;
	int			GetCanUseHeroNum() const;
	int			GetSoldierNum() const;
	bool		IsLeave() const { return m_bIsLeave; }
	bool		IsAI() const { return m_bIsAI; }
	bool		IsValid() const { return !m_bIsLeave && !m_bIsAI; }

	bool		ClickHeroCard(int nIndex, float fPos);
	bool		ClickSoldierCard(int nIndex, int fPos);
	void		UpgradeLevel();
	void		ChangeLevel(int level);

	void		ChangeResource(int nNum);
	void		ChangePopulation(int nNum);
	void		ChangeKillSoldiers(int nNum){ m_nAllKillSoldiers += nNum; }
	void		ChangeKillHeroes(int nNum){ m_nAllKillHeroes += nNum; }
	void		ChangeMakeDamage(int nNum){ m_nAllMakeDamage += nNum; }
	void		ChangeGetCrystal(int nNum){ m_nAllGetCrystal += nNum; }
	void		ClearSoldierCardCD(int nIndex = -1);

	CMaster*    Clone(CBattle *pBattle) const;

	float		GetResIncMul()const { return m_fResIncMul; }
	int         GetResource() const { return m_nResource; }

	short		military_level(){ return military_level_; }						//军事等级 攻防加成
	short		economy_level(){ return economy_level_; }						//经济等级 水晶人口

	void        SetResIncMul(float fMul){ m_fResIncMul = fMul; }

	void        LockHero(){ m_bHeroLock = true; }
	void        UnlockHero(){ m_bHeroLock = false; }
	void        LockSoldier(){ m_bSoldierLock = true; }
	void        UnlockSoldier(){ m_bSoldierLock = false; }
	void        ResetHero();
	bool        HeroUnavailable();

	bool		UsingHero();
	void		ChangeHeroDurationTime(int nTime){ m_nHeroDurationTime -= nTime; }

	bool        UseSkill(const CSkill* pSkill, CHero* pHero, float& nUseSkillPos);
	void		GetStageLoot(int id, int num);
	void		SetStageLootPto(pto_BATTLE_S2C_NTF_Game_Over* pto_game_over);
	void		SetStageLootDeletePto(dat_BATTLE_STRUCT_StageLootEx* stage_loot_ex);

	int			GetTechnologyAtk() const { return technology_[0]; }
	int			GetTechnologyMAtk() const { return technology_[1]; }
	int			GetTechnologyDef() const { return technology_[2]; }
	int			GetTechnologyMDef() const { return technology_[3]; }
	int			GetTechnologyHP() const { return technology_[4]; }

	int			GetSoldierExAtk() const { return soldier_ex_[0]; }
	int			GetSoldierExMAtk() const { return soldier_ex_[1]; }
	int			GetSoldierExDef() const { return soldier_ex_[2]; }
	int			GetSoldierExMDef() const { return soldier_ex_[3]; }
	int			GetSoldierExHP() const { return soldier_ex_[4]; }

	int			GetInteralStr() const { return interal_value_[0]; }
	int			GetInteralCmd() const { return interal_value_[1]; }
	int			GetInteralInt() const { return interal_value_[2]; }

	void		SetHeroCard(int hero_id, int index);
	SoldierSoul*	FindSoldierSoul(int soldier_id);

	int			GetCastleGunAtk() const { return castle_upgrade_[0]; }
	int			GetCastleGunMAtk() const { return castle_upgrade_[1]; }
	int			GetCastleHPEx() const { return castle_upgrade_[2]; }

	void		UseBaseGun();
	void		BaseGunActivate();
	void		BaseGunFire();
	void		BaseGunTimer();

	void		GetExtraBlueprint();

	void		AddCommonNum();				//增加普通兵数量
	void		AddEliteNum();				//增加精英兵数量
	void		UpdateSoldierCanUseNum();

	void		SetHeroDurationTime(int time){ m_nHeroDurationTime = time; }

	bool		MultiStageClear(){ return multi_stage_clear_; }

	void		SetTryHero(int hero_id, int hero_level);

private:
	CBattle*		m_pBattle;									//战斗指针
	Force			m_enForce{ Force::UNKNOWN };				//阵营

	bool			m_bIsLeave{ false };						//是否离开

	int				m_nPID{ 0 };								//PID
	short			m_nID{ 0 };									//MID
	short			m_nSID{ 0 };								//服务器ID
	std::string		m_strName;									//名字
	bool			m_bSex{ false };							//性别
	int				title_id_{ 0 };								//称号

	short			m_nPlayerLevel{ 0 };						//玩家等级

	short			military_level_{ 0 };						//军事等级 攻防加成
	short			economy_level_{ 0 };						//经济等级 水晶人口

	short			m_nResource{ 0 };							//现有资源
	short			m_nResourceMax{ MAX_RESOURCE_BASE };		//最大资源
	short			m_nResIncBase{ RESOURCE_INCREASE_BASE };	//资源增长基数
	float			m_fResIncMul{ 1.0f };						//资源增长倍数

	short			m_nPopulation{ 0 };							//现有人口
	short			m_nPopulationMax{ MAX_POPULATION_BASE };	//最大人口

	std::array<HeroCard, MAX_HERO_NUM>	m_arrHeroes;			//英雄组
	CHero*			m_pUsingHero{ nullptr };					//正在使用的英雄
	int				m_nHeroDurationTime{ 0 };					//英雄剩余时间

	std::array<SoldierCard, MAX_SOLDIER_NUM>	m_arrSoldiers;		//士兵组(ID,CD,NUM)
	std::vector<SoldierSoul> soldiers_soul_;						//士兵灵能
	int				technology_[5];								//士兵科技(攻击力，魔攻，防御力,魔防，HP)
	int				soldier_ex_[5];								//士兵额外属性(攻击力，魔攻，防御力,魔防，HP)
	int				interal_value_[3];							//士兵内政值
	int				castle_upgrade_[3];							//基地升级加成(火炮物攻，火炮魔攻,血量加成)

	bool			m_bIsAI{ false };							//是否是AI

	bool			m_bHeroLock{ false };                       //英雄锁定
	bool			m_bSoldierLock{ false };                    //单位锁定

	std::map<int, int> get_stage_loot_;							//已拾取额外奖励<id,num>

	int				remain_base_gun_{ 1 };
	int				base_gun_timer_{ 0 };
	bool			using_base_gun_{ false };

	bool			multi_stage_clear_{ false };				//能否获得多人关卡宝箱
	//****************************自定义召唤师************************************
	int         m_nAIType{ 0 };                             //AI类型
	bool        m_bShow{ true };							//在台子显示MASTER
	int			m_nModelID{ 0 };							//模型ID
	int			m_nNextClickSoldierIndex{ -1 };				//下一个出兵索引


	//**********************************************************************

	int			m_nAllResource{ 0 };						//总共获得的资源
	int			m_nAllHitSoldier{ 0 };						//总共出击士兵数量
	int			m_nAllKillHeroes{ 0 };						//总共击杀英雄数量
	int			m_nAllKillSoldiers{ 0 };					//总共击杀士兵数量
	int			m_nAllMakeDamage{ 0 };						//总伤害量
	int			m_nAllGetCrystal{ 0 };						//总共获得水晶数量
	int			m_nMark{ 0 };								//成绩

	void		__IncreaseResources();						//资源增长
	void		__CalculateHeroExistTime();					//计算英雄时间
	void		__CalculateSoldierCD();						//计算士兵CD

	void		__SetFairMasterValue();

	void		__AILoop();
	void		__AI1();
	void		__AI2();
	void		__AI3();
	//void		__MoveHeroGroup()

	float       __GetSkillRange(const CSkill* pSkill);
	int         __GetEnemyInRange(const CSkill* pSkill, CHero* pHero);

	int			__HeroPassSkillAddResources();

	void		__AIClickHeroCard();
	bool		__EnemyUsingHero();
	bool		UseStrategySoldier(const CSoldierTP* soldier, int pos);
};
