#pragma once

enum ATK_TYPE
{
	AT_PHYSICS = 1,		//����
	AT_MAGIC,			//ħ��
	AT_BLEND,			//���
	AT_BaseGun,
};

enum TARGET_TYPE
{
	TT_NORMAL,			//��ͨ
	TT_RANGE,			//Զ��
	TT_HERO,			//Ӣ��
	TT_BUILDING			//����
};

class CUnitTP
{
public:
	static void Load();
	static int GetBaseHP() { return m_nBaseHP; }
	static int GetBaseATK() { return m_nBaseATK; }
	static int GetBaseDEF() { return m_nBaseDEF; }
	static int GetBaseMATK() { return m_nBaseMATK; }
	static int GetBaseMDEF() { return m_nBaseMDEF; }
	int GetID() const { return m_nID; }
	int GetPassSkillSize() const { return m_vctPassSkills.size(); }
	int GetPassSkill(int nIndex) const { return m_vctPassSkills.at(nIndex); }
	float GetAtkDistance() const { return m_fAtkDistance; }
	int GetAtkBuildingDamage() const { return m_nAtkBuildingDamage; }

protected:
	static int	m_nBaseHP;				//����Ѫ��
	static int	m_nBaseATK;				//��������
	static int	m_nBaseDEF;				//��������
	static int	m_nBaseMATK;			//����ħ��
	static int	m_nBaseMDEF;			//����ħ��

public:
	CUnitTP() = default;
	~CUnitTP() = default;

	bool		IsRange() const { return m_bIsRange; }
	float		move_speed() const { return m_fMoveSpeed; }
	int			atk_prev() const { return m_nAtkPrev; }
	int			atk_interval() const { return m_nAtkInterval; }

protected:
	int			m_nID{ 0 };						//ID
	std::string	m_strName;						//����
	int			m_nResourceID{ 0 };				//��ԴID

	float		m_fVolume{ 0 };					//���
	float		m_fMoveSpeed{ 0 };				//�ƶ��ٶ�

	bool		m_bIsRange{ false };			//�Ƿ�Զ�̵�λ

	float		m_fAtkDistance{ 0 };			//��������
	int			m_nAtkPrev{ 0 };				//����ǰ��
	int			m_nAtkInterval{ 0 };			//�������

	ATK_TYPE	m_enAtkType{ AT_PHYSICS };		//��������

	int			m_nAtkBuildingDamage{ 0 };		//���������˺�

	TARGET_TYPE m_enNormalAtk{ TT_NORMAL };		//��ͨ״̬����Ŀ��
	TARGET_TYPE m_enHideAtk{ TT_NORMAL };		//����״̬����Ŀ��
	TARGET_TYPE m_enSpecialAtk{ TT_NORMAL };	//����״̬����Ŀ��

	std::vector<int> m_vctPassSkills;			//��������
};

