#pragma once

enum BulletForm
{
	BF_NORMAL,
	BF_WALL,
	BF_TRAP
};

class CBulletTP
{
	friend class CBullet;
public:
	static void Load();
	static const CBulletTP* GetBulletTP(int nID);

private:
	static std::map<int, const  CBulletTP*> ms_mapBullets;

public:
	CBulletTP() = default;
	~CBulletTP() = default;
	float GetVolumn() const { return m_fVolumn; }
	float GetMaxMoveDistance() const { return m_fMaxMoveDistance; }
	float GetPosOffset() const { return m_fPosOffset; }
	int GetBulletID() const { return m_nBulletID; }

private:
	int				m_nBulletID{ 0 };					//子弹配表ID
	std::string		m_strName;
	float			m_fVolumn{ 0 };						//体积
	BulletForm		m_enForm{ BF_NORMAL };				//是否是墙体
	int				m_nType{ 0 };						//是否是魔法子弹 (0物理攻击 1魔法攻击 2混合攻击)

	bool			m_bIsRange{ false };				//是否是远程子弹
	bool			m_bIsArea{ false };					//是否是范围子弹

	int				m_nValidForce{ 0 };					//有效阵营(1.敌方 2.我方 3.双方 4.进攻方)
	char			m_cVaildRange{ 0 };					//有效目标(1.近战 2.远程)
	char			m_cVaildUnit{ 0 };					//目标类型(1.士兵 2.英雄 4.Boss 8.建筑)

	int				m_nBasePhysicsDamage{ 0 };			//技能的物理伤害加成Base值
	int				m_nBaseMagicDamage{ 0 };			//技能的法术伤害加成Base值
	float			m_fPercentPhysicsDamage{ 0 };		//技能的物理伤害加成百分比
	float			m_fPercentMagicDamage{ 0 };			//技能的法术伤害加成百分比

	float			m_fPercentDamageSoldier{ 0 };		//对士兵的伤害比例
	float			m_fPercentDamageHero{ 0 };			//对英雄的伤害比例
	float			m_fPercentDamageBuilding{ 0 };		//对建筑的伤害比例

	bool			m_bHasTargetObj{ false };			//是否有目标单位
	bool			m_bHasFlyPath{ false };				//是否有飞行轨迹

	float			m_fPosOffset{ 0 };					//起始位置偏移

	int				m_nMoveLeadTime{ 0 };				//子弹移动前置时间
	float			m_fMoveSpeed{ 0 };					//移动速度
	float			m_fMaxMoveDistance{ 0 };			//子弹最大移动距离

	int				m_nSettlementLeadTime{ 0 };			//结算前置时间
	int				m_nSettlementIntervalTime{ 0 };		//结算间隔时间
	int				m_nSettlementTimes{ 0 };			//结算总次数

	int				m_nAttackSingleTimes{ 0 };			//子弹对单一目标的攻击次数
	int				m_nAttackSingleIntervalTime{ 0 };	//子弹对单一目标的攻击间隔

	int				m_nHitTimes{ 0 };					//命中总次数

	float			m_fFallbackDistance{ 0 };			//是否有击飞效果

	int				m_nLifeTime{ 0 };					//子弹存活时间

	char			m_cDeadManner{ 0 };					//死亡结算方式(1.LifeTime  2.SettlementTimes  4.Move 8.命中次数)

	float			m_fProcced{ 0 };					//逐步率
	float			m_fMinProceed{ 0 };					//最小逐步率
	float			m_fMaxProceed{ 0 };					//最大逐步率

	int				m_nEffect{ 0 };						//特殊效果
	int				m_nEffectValue{ 0 };				//特殊效果值

	int				m_nLayer1ResourceID{ 0 };			//前层资源ID
	int				m_nLayer2ResourceID{ 0 };			//后层资源ID

	std::vector<int>	m_vctBuffEffect;				//对敌人造成的BuffID

	int				m_nNewBulletID{ 0 };				//新子弹的ID
	bool			m_bIsOpposite{ false };				//是否反方向

	int				m_nDead_SummonedID{ 0 };			//召唤物ID
	int				m_nDead_SummonedNum{ 0 };			//召唤物数量

	int				invalid_target_{ 0 };				//无效目标
};

