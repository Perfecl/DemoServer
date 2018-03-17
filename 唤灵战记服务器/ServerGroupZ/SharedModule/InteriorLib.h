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

	static float		castle_atk_mul_;	//基地物攻每级加成
	static float		castle_matk_mul_;	//基地魔攻每级加成
	static float		castle_hp_mul_;		//基地血量每级加成

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
	int		m_nFarmMoney{ 0 };				//农场获得银币
	float	m_fFarmAddition{ 0 };			//农场力量加成
	int		m_nHuntMoney{ 0 };				//狩猎获得银币
	float	m_fHuntAddition{ 0 };			//狩猎力量加成
	int		m_nSilverMoney{ 0 };			//银矿获得银币
	float	m_fSilverAddition{ 0 };			//银矿力量加成
	int		m_nCrystalMoney{ 0 };			//水晶获得银币
	float	m_fCrystalAddition{ 0 };		//水晶力量加成

	int		m_nPrimaryExp{ 0 };				//初级获得经验
	float	m_fPrimaryAddition{ 0 };		//初级统率加成
	int		m_nMiddleExp{ 0 };				//中级获得经验
	float	m_fMiddleAddition{ 0 };			//中级统率经验
	int		m_nSeniorExp{ 0 };				//高级获得银币
	float	m_fSeniorAddition{ 0 };			//高级统率加成
	int		m_nMaseterExp{ 0 };				//大师获得经验
	float	m_fMasterAddition{ 0 };			//大师统率加成
	int		m_nLVUpExp{ 0 };				//升级所需经验

	int		m_nResidenceReputation{ 0 };    //民居获得银币
	float	m_nResidenceAddition{ 0 };		//民居智力加成
	int		m_nVillaReputation{ 0 };		//豪宅获得银币
	float	m_fVillaAddition{ 0 };			//豪宅智力加成
	int		m_nCelebritiesReputation{ 0 };  //名士居所获得银币
	float	m_fCelebritiesAddition{ 0 };	//名师居所智力加成
	int		m_nRoReputation{ 0 };           //奇境仙踪获得银币
	float	m_fRoAddition{ 0 };				//奇境仙踪智力加成

	__int64 m_nLevy{ 0 };                   //征收
};

