#pragma once
#include "LotteryChance.h"

struct ChanceLoot
{
	int m_nID{ 0 };
	int num_{ 0 };
	float m_fChance{ 0 };
};

struct CertainLoot
{
	int id_{ 0 };
	int num_{ 0 };
};

class CLottery
{
public:
	CLottery();
	~CLottery();
	static void Load();
	static const CLottery* GetLottery(int nID);
	int		GetCertainLootID(int nIndex) const { return m_arrCertainLoot.at(nIndex).id_; }
	int		GetCertainLootNum(int index) const { return m_arrCertainLoot.at(index).num_; }
	float	GetChanceLootChance(int nIndex) const { return m_arrChanceLoot.at(nIndex).m_fChance; }
	int		GetChanceLootID(int nIndex) const { return m_arrChanceLoot.at(nIndex).m_nID; }
	int		GetChanceLottNum(int index) const { return m_arrChanceLoot.at(index).num_; }
	int		GetLootLevel() const { return m_nLootLevel; }

private:
	static std::map<int, const CLottery*> ms_mapLotteryLibrary;
	int							m_nID{ 0 };
	int							m_nExpendItemID{ 0 };
	int							m_nExpendItemNum{ 0 };
	int							m_nLootLevel{ 0 };
	std::array<CertainLoot, 6>	m_arrCertainLoot;
	std::array<ChanceLoot, 8>	m_arrChanceLoot;
};

