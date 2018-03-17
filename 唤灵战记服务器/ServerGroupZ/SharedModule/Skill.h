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
	SSE_CLEAR_SOLDIER_CARD_CD,		//ʿ������CD����
	SSE_JUMP_BACK,					//�佫����
	SSE_ADD_RESOURCE_300,			//����300��Դ
	SSE_ENTER_HIDE,					//��������״̬
	SSE_KILL_SELF,					//ɱ���Լ�
	SSE_ENEMY_ADD_RESOURCE,			//����������Դ
	SSE_ADD_COMMON_NUM,				//������ͨ������
	SSE_ADD_ELITE_NUM,				//���Ӿ�Ӣ������
	SSE_RESET_HERO,					//����Ӣ��ʹ��״̬
	SSE_HERO_DISAPPEAR,				//Ӣ����ʧ
	SSE_REPAIR_BASE,				//�������
	SSE_AMBUSH,						//����
	SSE_DOUBLE_ATTACK = 14,			//���ι���
	SSE_ADD_RESOURCE_25,			//����25��Դ
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
	int				m_nSkillID{ 0 };									//����ΨһID
	int				m_nSmallIconID{ 0 };								//Сͼ��ID
	int				m_nCooldown{ 0 };									//��ȴʱ��
	std::string		m_strName;											//��������

	float					m_fHealthEffect{ 0 };						//��ѪЧ��
	SKILL_MOVE_EFFECT		m_enMoveEffect{ SME_NULL };					//�����ͷ��ƶ�Ч��
	SKILL_SEPECIAL_EFFECT	m_enSepecialEffect{ SSE_NULL };				//����Ч��

	int				m_nSummonedID{ 0 };									//�ٻ���ID
	int				m_nSummonedNum{ 0 };								//�ٻ�������

	int				m_nLeadTime{ 0 };									//����ǰҡʱ��
	int				m_nChantTime{ 0 };									//����ӽ��ʱ��
	int				m_nSpasticTime{ 0 };								//���ܺ�ҡʱ��

	bool			m_bStandChannel{ false };							//�Ƿ���Ҫ����ʩ��
	bool			m_bRepeatSameBullet{ false };						//�Ƿ��ظ�ʩ��ͬһ���ӵ�(true�Ļ���vector����ӵ�ȫ����һ��,false�Ļ�һ��һ����)

	int				m_nCastTimes{ 0 };									//ʩ�Ŵ���
	int				m_nCastInterval{ 0 };								//ʩ�ż��
	float			m_fCastRange{ 0 };									//ʩ�ž���
	int				m_nCastArea{ 0 };									//ʩ�ŷ�Χ

	std::vector<int>		m_vctBullets;								//�ͷų����ӵ�ID
	std::vector<int>		m_vctSelfBuffs;								//�������ͷŵ�BuffID

	int				m_nAIRule{ 0 };										//AI����
	bool            m_bIsNeedClick{ false };                            //�Ƿ���Ҫ���
};
