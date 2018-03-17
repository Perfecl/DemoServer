#pragma once
#include "UnitTP.h"

enum class SOLDIER_TYPE
{
	NORMAL = 1,	//普通兵
	ELITE,		//精英兵
	STRATEGY,	//策略兵
	BOSS		//BOSS
};

class CSoldierTP : public CUnitTP
{
	friend class CUnit;
public:
	static void Load();
	static const CSoldierTP* GetSoldierTP(int nID);
	static void GetAllSoldierID(std::vector<int>* vctSoldier);

	static int base_str(){ return ms_nStr; }
	static int base_cmd(){ return ms_nCmd; }
	static int base_int(){ return ms_nInt; }
private:
	static std::map<int, const CSoldierTP*> ms_mapSoldierTPs;

	static int	ms_nStr;				//力量
	static int	ms_nCmd;				//统帅
	static int	ms_nInt;				//智力

public:
	CSoldierTP() = default;
	~CSoldierTP() = default;

	void			SetProtocol(pto_BATTLE_Struct_SoldierCard* ptoSoldier) const;

	SOLDIER_TYPE	GetType() const { return m_enSoldierType; }
	int				GetPopulation() const { return m_nPopulation; }
	int				GetPrice() const { return m_nPrice; }
	int				GetCD() const { return m_nCD; }
	int				GetCanUseNumMax() const { return m_nCanUseNumMax; }
	float			GetHPMul() const { return m_fHP_mul; }					//血量倍率
	float			GetATKMul() const { return m_fATK_mul; }				//攻击倍率
	float			GetDEFMul() const { return m_fDEF_mul; }				//防御倍率
	float			GetMATKMul() const { return m_fMATK_mul; }				//魔攻倍率
	float			GetMDEFMul() const { return m_fMDEF_mul; }				//魔防倍率

	int				GetStr() const { return CSoldierTP::ms_nStr; }
	int				GetInt() const { return CSoldierTP::ms_nInt; }
	int				GetCmd() const { return CSoldierTP::ms_nCmd; }

	int				strategy_id() const { return strategy_id_; }
	bool			cross_line() const { return cross_line_; }


private:
	SOLDIER_TYPE	m_enSoldierType{ SOLDIER_TYPE::NORMAL };	//士兵类型
	int				m_nCanUseNumMax{ 0 };			//可使用数量
	bool			m_bCanHide{ false };			//是否隐形出场

	int				m_nPrice{ 0 };					//价格
	int				m_nPopulation{ 0 };				//人口
	int				m_nCD{ 0 };						//冷却时间

	float			m_fHP_mul{ 0 };					//血量倍率
	float			m_fATK_mul{ 0 };				//攻击倍率
	float			m_fDEF_mul{ 0 };				//防御倍率
	float			m_fMATK_mul{ 0 };				//魔攻倍率
	float			m_fMDEF_mul{ 0 };				//魔防倍率

	bool			m_bIsBreakHero{ false };		//是否阻挡英雄

	int				m_nBirthBuff{ 0 };				//出生Buff

	int				strategy_id_{ 0 };
	bool			cross_line_{ 0 };
};
