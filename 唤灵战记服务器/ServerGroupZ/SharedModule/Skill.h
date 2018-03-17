#pragma once

enum SKILL_MOVE_EFFECT
{
	SME_NULL,
	SME_JUMP,
	SME_FOLLOW
};

enum SKILL_SEPECIAL_EFFECT
{
	SSE_NULL,
	SSE_CLEAR_SOLDIER_CARD_CD,		//士兵卡牌CD清零
	SSE_JUMP_BACK,					//武将后跳
	SSE_ADD_RESOURCE_300,			//增加300资源
	SSE_ENTER_HIDE,					//进入隐身状态
	SSE_KILL_SELF,					//杀死自己
	SSE_ENEMY_ADD_RESOURCE,			//对手增加资源
	SSE_ADD_COMMON_NUM,				//增加普通兵数量
	SSE_ADD_ELITE_NUM,				//增加精英兵数量
	SSE_RESET_HERO,					//重置英雄使用状态
	SSE_HERO_DISAPPEAR,				//英雄消失
	SSE_REPAIR_BASE,				//修理基地
	SSE_AMBUSH,						//伏兵
	SSE_DOUBLE_ATTACK = 14,			//两次攻击
	SSE_ADD_RESOURCE_25,			//增加25资源
};

class CSkill
{
public:
	static void  Load();
	static const CSkill* GetSkill(int nID);

private:
	static std::map<int, const CSkill*> ms_mapSkills;

public:
	CSkill() = default;
	~CSkill() = default;

	int		GetID() const { return m_nSkillID; }
	int		GetCD() const { return m_nCooldown; }
	int		GetPrevTime() const { return m_nLeadTime; }
	int		GetSpasticTime() const { return m_nSpasticTime; }
	size_t	GetBulletSize() const { return m_vctBullets.size(); }
	int		GetBulletID(int nIndex) const { return m_vctBullets[nIndex]; }
	size_t	GetBuffSize() const { return m_vctSelfBuffs.size(); }
	int		GetBuffID(int nIndex) const { return m_vctSelfBuffs[nIndex]; }
	float	GetCastRange() const { return m_fCastRange; }
	int		GetCastTime() const { return m_nCastTimes; }
	int		GetSkillPrevTime() const { return m_nLeadTime; }
	int		GetCastInterval() const { return m_nCastInterval; }
	int		GetChantTime() const { return m_nChantTime; }
	bool    IsRepeatSameBullet() const { return m_bRepeatSameBullet; }
	bool	IsNeedStandChannel() const { return m_bStandChannel; }
	SKILL_MOVE_EFFECT		GetMoveEffect() const { return m_enMoveEffect; }
	SKILL_SEPECIAL_EFFECT	GetSepecialEffect() const { return m_enSepecialEffect; }
	float	GetHealthEffect() const { return m_fHealthEffect; }
	int     GetAIRule() const { return m_nAIRule; }
	const  std::vector<int>* GetBullets() const { return &m_vctBullets; }
	bool    IsNeedClick() const { return m_bIsNeedClick; }

	int		GetSummonedID() const{ return m_nSummonedID; }
	int		GetSunnonedNum() const{ return m_nSummonedNum; }

private:
	int				m_nSkillID{ 0 };									//技能唯一ID
	int				m_nSmallIconID{ 0 };								//小图标ID
	int				m_nCooldown{ 0 };									//冷却时间
	std::string		m_strName;											//技能名称

	float					m_fHealthEffect{ 0 };						//回血效果
	SKILL_MOVE_EFFECT		m_enMoveEffect{ SME_NULL };					//技能释放移动效果
	SKILL_SEPECIAL_EFFECT	m_enSepecialEffect{ SSE_NULL };				//特殊效果

	int				m_nSummonedID{ 0 };									//召唤物ID
	int				m_nSummonedNum{ 0 };								//召唤物数量

	int				m_nLeadTime{ 0 };									//技能前摇时间
	int				m_nChantTime{ 0 };									//技能咏唱时间
	int				m_nSpasticTime{ 0 };								//技能后摇时间

	bool			m_bStandChannel{ false };							//是否需要持续施法
	bool			m_bRepeatSameBullet{ false };						//是否重复施放同一波子弹(true的话把vector里的子弹全部放一遍,false的话一个一个放)

	int				m_nCastTimes{ 0 };									//施放次数
	int				m_nCastInterval{ 0 };								//施放间隔
	float			m_fCastRange{ 0 };									//施放距离
	int				m_nCastArea{ 0 };									//施放范围

	std::vector<int>		m_vctBullets;								//释放出的子弹ID
	std::vector<int>		m_vctSelfBuffs;								//对自身释放的BuffID

	int				m_nAIRule{ 0 };										//AI规则
	bool            m_bIsNeedClick{ false };                            //是否需要点击
};
