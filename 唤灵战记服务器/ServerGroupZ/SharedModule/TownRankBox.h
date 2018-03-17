#pragma once

class CTownRankBox
{
	friend class CTown;
public:
	CTownRankBox();
	~CTownRankBox();

	static void Load();

	static const CTownRankBox* GetTownRankBox(int nTownID);

private:
	static std::map<int, const CTownRankBox*> ms_mapTownRankBox;

	int m_nTownID{ 0 };
	int m_nSilver{ 0 };
	int m_nGold{ 0 };
	int m_nHonour{ 0 };
	int m_nStamina{ 0 };
	int m_nPlayerTitle{ 0 };
};

