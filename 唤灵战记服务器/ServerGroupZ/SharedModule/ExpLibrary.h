#pragma once

class CExpLibrary
{
public:
	static void		Load();
	static __int64	GetSoldierExp(int nLV, SoldierTechnology type);
	static const CExpLibrary* GetExpLibrary(int nLV);
	
private:
	static std::map<int, const CExpLibrary*> ms_mapExpLibrary;

	static float ms_nSoldierAllExp;		//总科技
	static float ms_nSoldierAtkExp;		//士兵攻击
	static float ms_nSoldierMAtkExp;	//士兵魔攻
	static float ms_nSoldierDefExp;		//士兵防御
	static float ms_nSoldierMDefExp;	//士兵魔防
	static float ms_nSoldierHPExp;	    //士兵血量

public:
	CExpLibrary() = default;
	~CExpLibrary() = default;

	__int64		GetNeedExp() const { return m_nExp; }
	int			GetNeedStage() const { return m_nStage; }

private:
	int			m_nLevel{ 0 };				//所升等级
	int			m_nStage{ 0 };
	__int64		m_nExp{ 0 };				//升级所需经验
	__int64		m_nSoldierExp{ 0 };
};

