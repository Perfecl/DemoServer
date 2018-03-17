#pragma once

class COffLineHero;
class CHeroCard;
enum UnitState
{
	US_STAND,
	US_MOVING,
	US_ATK,
	US_SKILL,
	US_DEAD
};

struct OfflineDamageInfo
{
	int  nDamage{ 0 };						//物理伤害			
	int	 nMDamage{ 0 };						//魔法伤害

	bool bIsRangeAtk{ false };				//是否远程
	bool bIsHeroAtk{ false };				//是否英雄
	bool bIsSkill{ false };					//是否技能
	bool bIsArea{ false };					//是否范围

	int	 nAtkType{ 0 };						//攻击类型

	int	 nStr{ 0 };							//力量
	int	 nCmd{ 0 };							//统帅
	int	 nInt{ 0 };							//智力

	float fRatio{ 1.0f };					//伤害比率

	float fPhysicsRatio{ 1.0f };			//物理伤害比
	float fMagicRatio{ 1.0f };				//魔法伤害比	  
	COffLineHero* pSrcHero{ nullptr };      //伤害来源
};

class COffLinePlayer;
class COffLineHero
{
public:
	COffLineHero();
	~COffLineHero();

	static COffLineHero*	InitHero(const CHeroCard* hero_card, COffLinePlayer* pPlayer);

	int						Attack(COffLineHero* pHero, pto_OFFLINEBATTLE_Struct_UnitRound* pUnitRound);
	__int64					BeDamaged(OfflineDamageInfo di, pto_OFFLINEBATTLE_Struct_UnitRound* pUnitRound);

	bool				IsDead() const { if (US_DEAD == m_enState) return true; return false; }
	int					GetQuality() const { return m_nQuality; }
	HERO_TYPE			GetHeroType() const { return m_enHeroType; }
	float				GetCritOdds() const { return m_fCritOdds + m_fCritOddsEx; }
	float				GetPreventCrit() const { return m_fPreventCrit; }
	float				GetCritValue() const { return m_fCritValue; }

	int					level() const { return level_; }
	__int64				GetHP() const { return m_nHP; }
	__int64				GetMaxHP() const { return m_nMaxHp; }

	int					GetAtk() const { return m_nAtk + m_nAtkEx; }
	int					GetDef() const { return m_nDef + m_nDefEx; }
	int					GetMatk()const { return m_nMatk + m_nMatkEx; }
	int					GetMdef()const { return m_nMdef + m_nMdefEx; }
	int					GetStr() const { return m_nStr + m_nStrEx; }
	int					GetCmd() const { return m_nCmd + m_nCmdEx; }
	int					GetInt() const { return m_nInt + m_nIntEx; }

	void				SetStr(int nStr) { m_nStr = m_nStr; }
	void				SetCmd(int nCmd) { m_nCmd = m_nCmd; }
	void				SetInt(int nInt) { m_nInt = m_nInt; }
	void				SetMaxHP(__int64 nMaxHP){ m_nMaxHp = nMaxHP; }
	void				SetHP(__int64 nHP){ m_nHP = nHP; }

	const CHeroTP*      GetHeroTP() const { return m_pHeroTP; }
	float				GetAtkDistance() const { return m_fAtkDistance; }

	bool				IsRangeUnit() const { return m_bIsRange; }

	int					GetAtkBuildingDamage() const { return m_nAtkBuildingDamage; }

	void				AttackingSkillCalculate(COffLineHero* pTarget, float& fDamage, float& fMDamage, float& fCritOdds, float& fCritValue);
	void				PrevBeDamagedSkillCalculate(const OfflineDamageInfo& di, float &fDamage, float &fMDamage);
	void				PrevAttackSkillCalculate(COffLineHero* pTarget);
	void				AfterAttackSkillCalculate(COffLineHero* pTarget);
	void				GetAlwaysPassSkill();
	void				Heal(int heal_hp);

	int					dmg_up() { return dmg_up_; }
	int					armor_weak(){ return armor_weak_; }
	bool				double_attack() { return double_attack_; }

	void				set_dmg_up(int dmg_up){ dmg_up_ = dmg_up; }
	void				set_armor_weak(int armor_weak){ armor_weak_ = armor_weak; }
	void				set_double_attack(bool double_attack){ double_attack_ = double_attack; }

	void				ChangeDmgUp(int num){ dmg_up_ += num; }
	void				ChangeArmorWeak(int num){ armor_weak_ += num; }

	int					rank(){ return rank_; }
	int					suit_id(){ return suit_id_; }

private:
	int					level_{ 0 };					//等级
	__int64				m_nHP{ 0 };						//当前HP
	__int64				m_nMaxHp{ 0 };					//最大HP

	HERO_TYPE			m_enHeroType{ HT_ATK };			//英雄类型
	const CHeroTP*		m_pHeroTP{ nullptr };			//英雄模版
	int					m_nQuality{ 0 };				//品阶

	float				m_fCritOdds{ 0.2f };			//爆击几率
	float				m_fPreventCrit{ 0 };			//免除暴几率
	float				m_fCritOddsEx{ 0 };				//额外爆击率
	float				m_fCritValue{ 1.5f };			//爆击值

	int				    m_nAtk{ 0 };					//基础攻击
	int				    m_nDef{ 0 };					//基础防御
	int				    m_nMatk{ 0 };					//基础魔攻
	int				    m_nMdef{ 0 };					//基础魔防

	bool			    m_bIsRange{ false };			//是否是远程单位
	float			    m_fAtkDistance{ 0 };			//攻击距离
	ATK_TYPE		    m_enAtkType{ AT_PHYSICS };		//攻击类型
	int				    m_nStr{ 0 };					//力量
	int				    m_nCmd{ 0 };					//统帅
	int				    m_nInt{ 0 };					//智力
	int				    m_nAtkEx{ 0 };					//额外攻击
	int				    m_nDefEx{ 0 };					//额外防御
	int				    m_nMatkEx{ 0 };					//额外魔攻
	int				    m_nMdefEx{ 0 };					//额外魔防
	int				    m_nStrEx{ 0 };					//额外力量
	int				    m_nCmdEx{ 0 };					//额外统帅
	int				    m_nIntEx{ 0 };					//额外智力
	UnitState		    m_enState{ US_STAND };			//单位状态
	int					m_nAtkBuildingDamage{ 0 };		//对建筑伤害

	int					dmg_up_{ 0 };					//10%伤害提升回合
	int					armor_weak_{ 0 };				//护甲虚弱回合		
	bool				double_attack_{ false };

	std::vector<const CPassSkill*>	m_vctPasSkills;

	float			    _DamageFormula(int src_Str_Int, int dest_Str_Int, int dest_cmd);		//伤害计算公式
	void                _Die();

	int					rank_{ 0 };
	int					suit_id_{ 0 };

	static void __GetSuitAddtion(const CHeroCard* hero_card, COffLinePlayer* pPlayer, SuitAddition* suit_addition);
};

