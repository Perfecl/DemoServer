#pragma once
class CLotteryChance
{
public:
	CLotteryChance() = default;
	~CLotteryChance() = default;
	static void Load();
	static const CLotteryChance* GetLotteryChance(int nID);
	static int GetLotteryChanceSize(){ return ms_mapLotteryChanceLibrary.size(); }
	static const CLotteryChance* GetLottertChanceByIndex(int nIndex){ return ms_mapLotteryChanceLibrary.at(nIndex); }
	int GetID() const { return m_nID; }
	int GetType() const { return m_nType; }
	int GetBaseNum() const {return m_nBaseNum;}
	int GetMul() const {return m_nMul;};
	float GetChance() const { return m_fChance; }

private:
	static std::map<int, const CLotteryChance*> ms_mapLotteryChanceLibrary;
	int m_nID{ 0 };
	int m_nType{ 0 };
	int m_nBaseNum{ 0 };
	int m_nMul{ 0 };
	float m_fChance{ 0 };
};

