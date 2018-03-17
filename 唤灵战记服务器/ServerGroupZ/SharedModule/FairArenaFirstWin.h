#pragma once
class CFairArenaFirstWin
{
public:
	CFairArenaFirstWin();
	~CFairArenaFirstWin();
	static void Load();
	static const CFairArenaFirstWin* GetFairArenaFirstWin(int nLevel);
	int GetSilver() const { return m_nSilver; }
	int GetHonor() const { return m_nHonor; }

private:
	static std::map<int, const CFairArenaFirstWin*> ms_mapFirstWin;
	int m_nLevel{ 0 };
	int m_nSilver{ 0 };
	int m_nHonor{ 0 };
};

