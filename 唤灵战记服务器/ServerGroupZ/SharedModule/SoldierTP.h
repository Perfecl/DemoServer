#pragma once
#include "UnitTP.h"

enum class SOLDIER_TYPE
{
	NORMAL = 1,	//��ͨ��
	ELITE,		//��Ӣ��
	STRATEGY,	//���Ա�
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

	static int	ms_nStr;				//����
	static int	ms_nCmd;				//ͳ˧
	static int	ms_nInt;				//����

public:
	CSoldierTP() = default;
	~CSoldierTP() = default;

	void			SetProtocol(pto_BATTLE_Struct_SoldierCard* ptoSoldier) const;

	SOLDIER_TYPE	GetType() const { return m_enSoldierType; }
	int				GetPopulation() const { return m_nPopulation; }
	int				GetPrice() const { return m_nPrice; }
	int				GetCD() const { return m_nCD; }
	int				GetCanUseNumMax() const { return m_nCanUseNumMax; }
	float			GetHPMul() const { return m_fHP_mul; }					//Ѫ������
	float			GetATKMul() const { return m_fATK_mul; }				//��������
	float			GetDEFMul() const { return m_fDEF_mul; }				//��������
	float			GetMATKMul() const { return m_fMATK_mul; }				//ħ������
	float			GetMDEFMul() const { return m_fMDEF_mul; }				//ħ������

	int				GetStr() const { return CSoldierTP::ms_nStr; }
	int				GetInt() const { return CSoldierTP::ms_nInt; }
	int				GetCmd() const { return CSoldierTP::ms_nCmd; }

	int				strategy_id() const { return strategy_id_; }
	bool			cross_line() const { return cross_line_; }


private:
	SOLDIER_TYPE	m_enSoldierType{ SOLDIER_TYPE::NORMAL };	//ʿ������
	int				m_nCanUseNumMax{ 0 };			//��ʹ������
	bool			m_bCanHide{ false };			//�Ƿ����γ���

	int				m_nPrice{ 0 };					//�۸�
	int				m_nPopulation{ 0 };				//�˿�
	int				m_nCD{ 0 };						//��ȴʱ��

	float			m_fHP_mul{ 0 };					//Ѫ������
	float			m_fATK_mul{ 0 };				//��������
	float			m_fDEF_mul{ 0 };				//��������
	float			m_fMATK_mul{ 0 };				//ħ������
	float			m_fMDEF_mul{ 0 };				//ħ������

	bool			m_bIsBreakHero{ false };		//�Ƿ��赲Ӣ��

	int				m_nBirthBuff{ 0 };				//����Buff

	int				strategy_id_{ 0 };
	bool			cross_line_{ 0 };
};
