#pragma once

enum BUFF_BASIC_EFFECT
{
	BBE_CMD = 1,					//ͳ˧
	BBE_STR,						//����
	BBE_INT,						//����
	BBE_ATK_SPEED_UP,				//�ӹ����ٶ�
	BBE_ATK_SPEED_DOWN,				//�������ٶ�
	BBE_HP_V, 						//HP�ٷֱ�
	BBE_HP_P,						//HPֵ
	BBE_DISTANCE,					//��������
	BBE_CRIT,						//������
	BBE_CRIT_VALUE, 				//��������
	BBE_ATK,						//�������Dot�˺�
	BBE_MATK,						//���ħ��Dot�˺�
	BBE_DEF,						//���������˺�
	BBE_MDEF,						//����ħ���˺�
	BBE_RICH,						//���ˮ�����
	BBE_EX_ATK =17,					//��ɵ������˺�����
	BBE_EX_MATK,					//��ɵ�ħ���˺�����
	BEE_NERF_DEF,					//���ٻ���
	BBE_END = 999					//����
};

enum BUFF_ADVANCE_EFFECT
{
	BAE_INVINCIBLE = 1000,			//�޵�
	BAE_STUN,						//ѣ��
	BAE_FEAR,						//�־�
	BAE_SILENCE,					//��Ĭ
	BAE_HIDING,						//����
	BAE_MOVE_SPEED_UP,				//����
	BAE_MOVE_SPEED_DOWN,			//����
	BAE_MOVE_HOLD,					//����
	BAE_SHUTTLE,					//����
	BAE_BLIND,						//��ä
	BAE_END = 1999
};

class CBuffTP
{
	friend class CBuff;
public:
	static void Load();
	static const CBuffTP* GetBuffTP(int nID);

private:
	static std::map<int, const  CBuffTP*> ms_mapBuffs;

public:
	CBuffTP() = default;
	~CBuffTP() = default;

private:
	int									m_nID{ 0 };
	std::string							m_strName;
	int									m_nIcon{ 0 };
	std::vector<std::pair<int, int>>	m_vctEffects;
	std::vector<int>					m_vctAdvanceEffect;
	bool								m_bIsDebuff{ false };
	int									m_nInterval{ 0 };
	int									m_nTime{ 0 };
};
