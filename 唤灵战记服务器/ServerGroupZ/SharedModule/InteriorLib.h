#pragma once

struct SUpgradeCost
{
	int m_nLevel{ 0 };
	int m_nGold{ 0 };
	int m_nPlayerLevel{ 0 };
	int m_nSilver{ 0 };
};

class CInteriorLib
{
public:
	static void Load();
	static const CInteriorLib* GetInteriotLib(int nLV);
	static const SUpgradeCost* GetUpgradeCost(int nLevel);

	static float	castle_atk_mul(){ return castle_atk_mul_; }
	static float	castle_matk_mul(){ return castle_matk_mul_; }
	static float	castle_hp_mul(){ return castle_hp_mul_; }

private:
	static std::map<int, const CInteriorLib*> ms_mapInteriorLib;
	static std::map<int, const SUpgradeCost*> ms_mapUpgradeCost;

	static float		castle_atk_mul_;	//�����﹥ÿ���ӳ�
	static float		castle_matk_mul_;	//����ħ��ÿ���ӳ�
	static float		castle_hp_mul_;		//����Ѫ��ÿ���ӳ�

	static int __GetMaxSize(Interior_Library* pLibInterior);

public:
	CInteriorLib() = default;
	~CInteriorLib() = default;

	int		GetLevel() const { return m_nLevel; }
	int		GetFarmMoney() const { return m_nFarmMoney; }
	float	GetFarmAddition() const { return m_fFarmAddition; }
	int		GetHuntMoney() const { return m_nHuntMoney; }
	float	GetHuntAddition() const { return m_fHuntAddition; }
	int		GetSilverMoney() const { return m_nSilverMoney; }
	float	GetSilverAddition() const { return m_fSilverAddition; }
	int		GetCrystalMoney() const { return m_nCrystalMoney; }
	float	GetCrystalAddition() const { return m_fCrystalAddition; }
	int		GetPrimaryExp() const { return m_nPrimaryExp; }
	float	GetPrimaryAddition() const { return m_fPrimaryAddition; }
	int		GetMiddleExp() const { return m_nMiddleExp; }
	float	GetMiddleAddition() const { return m_fMiddleAddition; }
	int		GetSeniorExp() const { return m_nSeniorExp; }
	float	GetSeniorAddition() const { return m_fSeniorAddition; }
	int		GetMaseterExp() const { return m_nMaseterExp; }
	float	GetMasterAddition() const { return m_fMasterAddition; }
	int		GetLVUpExp() const { return m_nLVUpExp; }
	int		GetResidenceReputation() const { return m_nResidenceReputation; }
	float	GetResidenceAddition() const { return m_nResidenceAddition; }
	int		GetVillaReputation() const { return m_nVillaReputation; }
	float	GetVillaAddition() const { return m_fVillaAddition; }
	int		GetCelebritiesReputation() const { return m_nCelebritiesReputation; }
	float	GetCelebritiesAddition() const { return m_fCelebritiesAddition; }
	int		GetRoReputation() const { return m_nRoReputation; }
	float	GetRoAddition() const { return m_fRoAddition; }
	__int64 GetLevy() const { return m_nLevy; }


private:
	int		m_nLevel{ 0 };
	int		m_nFarmMoney{ 0 };				//ũ���������
	float	m_fFarmAddition{ 0 };			//ũ�������ӳ�
	int		m_nHuntMoney{ 0 };				//���Ի������
	float	m_fHuntAddition{ 0 };			//���������ӳ�
	int		m_nSilverMoney{ 0 };			//����������
	float	m_fSilverAddition{ 0 };			//���������ӳ�
	int		m_nCrystalMoney{ 0 };			//ˮ���������
	float	m_fCrystalAddition{ 0 };		//ˮ�������ӳ�

	int		m_nPrimaryExp{ 0 };				//������þ���
	float	m_fPrimaryAddition{ 0 };		//����ͳ�ʼӳ�
	int		m_nMiddleExp{ 0 };				//�м���þ���
	float	m_fMiddleAddition{ 0 };			//�м�ͳ�ʾ���
	int		m_nSeniorExp{ 0 };				//�߼��������
	float	m_fSeniorAddition{ 0 };			//�߼�ͳ�ʼӳ�
	int		m_nMaseterExp{ 0 };				//��ʦ��þ���
	float	m_fMasterAddition{ 0 };			//��ʦͳ�ʼӳ�
	int		m_nLVUpExp{ 0 };				//�������辭��

	int		m_nResidenceReputation{ 0 };    //��ӻ������
	float	m_nResidenceAddition{ 0 };		//��������ӳ�
	int		m_nVillaReputation{ 0 };		//��լ�������
	float	m_fVillaAddition{ 0 };			//��լ�����ӳ�
	int		m_nCelebritiesReputation{ 0 };  //��ʿ�����������
	float	m_fCelebritiesAddition{ 0 };	//��ʦ���������ӳ�
	int		m_nRoReputation{ 0 };           //�澳���ٻ������
	float	m_fRoAddition{ 0 };				//�澳���������ӳ�

	__int64 m_nLevy{ 0 };                   //����
};

