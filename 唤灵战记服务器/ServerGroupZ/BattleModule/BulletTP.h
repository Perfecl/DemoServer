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
	int				m_nBulletID{ 0 };					//�ӵ����ID
	std::string		m_strName;
	float			m_fVolumn{ 0 };						//���
	BulletForm		m_enForm{ BF_NORMAL };				//�Ƿ���ǽ��
	int				m_nType{ 0 };						//�Ƿ���ħ���ӵ� (0������ 1ħ������ 2��Ϲ���)

	bool			m_bIsRange{ false };				//�Ƿ���Զ���ӵ�
	bool			m_bIsArea{ false };					//�Ƿ��Ƿ�Χ�ӵ�

	int				m_nValidForce{ 0 };					//��Ч��Ӫ(1.�з� 2.�ҷ� 3.˫�� 4.������)
	char			m_cVaildRange{ 0 };					//��ЧĿ��(1.��ս 2.Զ��)
	char			m_cVaildUnit{ 0 };					//Ŀ������(1.ʿ�� 2.Ӣ�� 4.Boss 8.����)

	int				m_nBasePhysicsDamage{ 0 };			//���ܵ������˺��ӳ�Baseֵ
	int				m_nBaseMagicDamage{ 0 };			//���ܵķ����˺��ӳ�Baseֵ
	float			m_fPercentPhysicsDamage{ 0 };		//���ܵ������˺��ӳɰٷֱ�
	float			m_fPercentMagicDamage{ 0 };			//���ܵķ����˺��ӳɰٷֱ�

	float			m_fPercentDamageSoldier{ 0 };		//��ʿ�����˺�����
	float			m_fPercentDamageHero{ 0 };			//��Ӣ�۵��˺�����
	float			m_fPercentDamageBuilding{ 0 };		//�Խ������˺�����

	bool			m_bHasTargetObj{ false };			//�Ƿ���Ŀ�굥λ
	bool			m_bHasFlyPath{ false };				//�Ƿ��з��й켣

	float			m_fPosOffset{ 0 };					//��ʼλ��ƫ��

	int				m_nMoveLeadTime{ 0 };				//�ӵ��ƶ�ǰ��ʱ��
	float			m_fMoveSpeed{ 0 };					//�ƶ��ٶ�
	float			m_fMaxMoveDistance{ 0 };			//�ӵ�����ƶ�����

	int				m_nSettlementLeadTime{ 0 };			//����ǰ��ʱ��
	int				m_nSettlementIntervalTime{ 0 };		//������ʱ��
	int				m_nSettlementTimes{ 0 };			//�����ܴ���

	int				m_nAttackSingleTimes{ 0 };			//�ӵ��Ե�һĿ��Ĺ�������
	int				m_nAttackSingleIntervalTime{ 0 };	//�ӵ��Ե�һĿ��Ĺ������

	int				m_nHitTimes{ 0 };					//�����ܴ���

	float			m_fFallbackDistance{ 0 };			//�Ƿ��л���Ч��

	int				m_nLifeTime{ 0 };					//�ӵ����ʱ��

	char			m_cDeadManner{ 0 };					//�������㷽ʽ(1.LifeTime  2.SettlementTimes  4.Move 8.���д���)

	float			m_fProcced{ 0 };					//����
	float			m_fMinProceed{ 0 };					//��С����
	float			m_fMaxProceed{ 0 };					//�������

	int				m_nEffect{ 0 };						//����Ч��
	int				m_nEffectValue{ 0 };				//����Ч��ֵ

	int				m_nLayer1ResourceID{ 0 };			//ǰ����ԴID
	int				m_nLayer2ResourceID{ 0 };			//�����ԴID

	std::vector<int>	m_vctBuffEffect;				//�Ե�����ɵ�BuffID

	int				m_nNewBulletID{ 0 };				//���ӵ���ID
	bool			m_bIsOpposite{ false };				//�Ƿ񷴷���

	int				m_nDead_SummonedID{ 0 };			//�ٻ���ID
	int				m_nDead_SummonedNum{ 0 };			//�ٻ�������

	int				invalid_target_{ 0 };				//��ЧĿ��
};

