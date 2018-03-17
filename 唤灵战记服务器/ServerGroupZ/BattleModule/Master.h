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

	int		level_{ 0 };				//�ȼ�
	int		color_{ 0 };				//��ɫ
	int		atk_{ 0 };					//����
	int		matk_{ 0 };					//ħ��
	int		def_{ 0 };					//����
	int		mdef_{ 0 };					//ħ��
	int		hp_{ 0 };					//Ѫ��
	float	move_speed_{ 0 };			//�ƶ��ٶ�
	int		prev_atk_{ 0 };				//����ǰҡ
	int		atk_interval_{ 0 };			//�������
	int		exsits_time_{ 0 };			//����ʱ��
	float	crit_odds_{ 0 };			//��������
	float	crit_value_{ 0 };			//�����˺�
	float	crit_resistance_{ 0 };		//��������
	int		strength_{ 0 };				//����
	int		command_{ 0 };				//ͳ��
	int		intelligence_{ 0 };			//����
	int		building_atk_{ 0 };			//����������
	int		suit_effect_{ 0 };			//��װЧ��
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
	int soldier_id_;	//ʿ��ID
	int soul_level_;	//ʿ�����ܵȼ�
	int soul_exp_;		//ʿ�����ܲ���
	int train_level_;	//ʿ��ѵ���ȼ�
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

	short		military_level(){ return military_level_; }						//���µȼ� �����ӳ�
	short		economy_level(){ return economy_level_; }						//���õȼ� ˮ���˿�

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

	void		AddCommonNum();				//������ͨ������
	void		AddEliteNum();				//���Ӿ�Ӣ������
	void		UpdateSoldierCanUseNum();

	void		SetHeroDurationTime(int time){ m_nHeroDurationTime = time; }

	bool		MultiStageClear(){ return multi_stage_clear_; }

	void		SetTryHero(int hero_id, int hero_level);

private:
	CBattle*		m_pBattle;									//ս��ָ��
	Force			m_enForce{ Force::UNKNOWN };				//��Ӫ

	bool			m_bIsLeave{ false };						//�Ƿ��뿪

	int				m_nPID{ 0 };								//PID
	short			m_nID{ 0 };									//MID
	short			m_nSID{ 0 };								//������ID
	std::string		m_strName;									//����
	bool			m_bSex{ false };							//�Ա�
	int				title_id_{ 0 };								//�ƺ�

	short			m_nPlayerLevel{ 0 };						//��ҵȼ�

	short			military_level_{ 0 };						//���µȼ� �����ӳ�
	short			economy_level_{ 0 };						//���õȼ� ˮ���˿�

	short			m_nResource{ 0 };							//������Դ
	short			m_nResourceMax{ MAX_RESOURCE_BASE };		//�����Դ
	short			m_nResIncBase{ RESOURCE_INCREASE_BASE };	//��Դ��������
	float			m_fResIncMul{ 1.0f };						//��Դ��������

	short			m_nPopulation{ 0 };							//�����˿�
	short			m_nPopulationMax{ MAX_POPULATION_BASE };	//����˿�

	std::array<HeroCard, MAX_HERO_NUM>	m_arrHeroes;			//Ӣ����
	CHero*			m_pUsingHero{ nullptr };					//����ʹ�õ�Ӣ��
	int				m_nHeroDurationTime{ 0 };					//Ӣ��ʣ��ʱ��

	std::array<SoldierCard, MAX_SOLDIER_NUM>	m_arrSoldiers;		//ʿ����(ID,CD,NUM)
	std::vector<SoldierSoul> soldiers_soul_;						//ʿ������
	int				technology_[5];								//ʿ���Ƽ�(��������ħ����������,ħ����HP)
	int				soldier_ex_[5];								//ʿ����������(��������ħ����������,ħ����HP)
	int				interal_value_[3];							//ʿ������ֵ
	int				castle_upgrade_[3];							//���������ӳ�(�����﹥������ħ��,Ѫ���ӳ�)

	bool			m_bIsAI{ false };							//�Ƿ���AI

	bool			m_bHeroLock{ false };                       //Ӣ������
	bool			m_bSoldierLock{ false };                    //��λ����

	std::map<int, int> get_stage_loot_;							//��ʰȡ���⽱��<id,num>

	int				remain_base_gun_{ 1 };
	int				base_gun_timer_{ 0 };
	bool			using_base_gun_{ false };

	bool			multi_stage_clear_{ false };				//�ܷ��ö��˹ؿ�����
	//****************************�Զ����ٻ�ʦ************************************
	int         m_nAIType{ 0 };                             //AI����
	bool        m_bShow{ true };							//��̨����ʾMASTER
	int			m_nModelID{ 0 };							//ģ��ID
	int			m_nNextClickSoldierIndex{ -1 };				//��һ����������


	//**********************************************************************

	int			m_nAllResource{ 0 };						//�ܹ���õ���Դ
	int			m_nAllHitSoldier{ 0 };						//�ܹ�����ʿ������
	int			m_nAllKillHeroes{ 0 };						//�ܹ���ɱӢ������
	int			m_nAllKillSoldiers{ 0 };					//�ܹ���ɱʿ������
	int			m_nAllMakeDamage{ 0 };						//���˺���
	int			m_nAllGetCrystal{ 0 };						//�ܹ����ˮ������
	int			m_nMark{ 0 };								//�ɼ�

	void		__IncreaseResources();						//��Դ����
	void		__CalculateHeroExistTime();					//����Ӣ��ʱ��
	void		__CalculateSoldierCD();						//����ʿ��CD

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
