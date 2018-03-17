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
	int  nDamage{ 0 };						//�����˺�			
	int	 nMDamage{ 0 };						//ħ���˺�

	bool bIsRangeAtk{ false };				//�Ƿ�Զ��
	bool bIsHeroAtk{ false };				//�Ƿ�Ӣ��
	bool bIsSkill{ false };					//�Ƿ���
	bool bIsArea{ false };					//�Ƿ�Χ

	int	 nAtkType{ 0 };						//��������

	int	 nStr{ 0 };							//����
	int	 nCmd{ 0 };							//ͳ˧
	int	 nInt{ 0 };							//����

	float fRatio{ 1.0f };					//�˺�����

	float fPhysicsRatio{ 1.0f };			//�����˺���
	float fMagicRatio{ 1.0f };				//ħ���˺���	  
	COffLineHero* pSrcHero{ nullptr };      //�˺���Դ
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
	int					level_{ 0 };					//�ȼ�
	__int64				m_nHP{ 0 };						//��ǰHP
	__int64				m_nMaxHp{ 0 };					//���HP

	HERO_TYPE			m_enHeroType{ HT_ATK };			//Ӣ������
	const CHeroTP*		m_pHeroTP{ nullptr };			//Ӣ��ģ��
	int					m_nQuality{ 0 };				//Ʒ��

	float				m_fCritOdds{ 0.2f };			//��������
	float				m_fPreventCrit{ 0 };			//���������
	float				m_fCritOddsEx{ 0 };				//���ⱬ����
	float				m_fCritValue{ 1.5f };			//����ֵ

	int				    m_nAtk{ 0 };					//��������
	int				    m_nDef{ 0 };					//��������
	int				    m_nMatk{ 0 };					//����ħ��
	int				    m_nMdef{ 0 };					//����ħ��

	bool			    m_bIsRange{ false };			//�Ƿ���Զ�̵�λ
	float			    m_fAtkDistance{ 0 };			//��������
	ATK_TYPE		    m_enAtkType{ AT_PHYSICS };		//��������
	int				    m_nStr{ 0 };					//����
	int				    m_nCmd{ 0 };					//ͳ˧
	int				    m_nInt{ 0 };					//����
	int				    m_nAtkEx{ 0 };					//���⹥��
	int				    m_nDefEx{ 0 };					//�������
	int				    m_nMatkEx{ 0 };					//����ħ��
	int				    m_nMdefEx{ 0 };					//����ħ��
	int				    m_nStrEx{ 0 };					//��������
	int				    m_nCmdEx{ 0 };					//����ͳ˧
	int				    m_nIntEx{ 0 };					//��������
	UnitState		    m_enState{ US_STAND };			//��λ״̬
	int					m_nAtkBuildingDamage{ 0 };		//�Խ����˺�

	int					dmg_up_{ 0 };					//10%�˺������غ�
	int					armor_weak_{ 0 };				//���������غ�		
	bool				double_attack_{ false };

	std::vector<const CPassSkill*>	m_vctPasSkills;

	float			    _DamageFormula(int src_Str_Int, int dest_Str_Int, int dest_cmd);		//�˺����㹫ʽ
	void                _Die();

	int					rank_{ 0 };
	int					suit_id_{ 0 };

	static void __GetSuitAddtion(const CHeroCard* hero_card, COffLinePlayer* pPlayer, SuitAddition* suit_addition);
};

