#pragma once
#define Broke_Rune_ID 9999

enum WashRuneType
{
	WRT_Null,
	WRT_Nomal,
	WRT_Strengthen,
	WRT_Item,
};

struct RuneAddition
{
	int m_nAtk{ 0 };
	int m_nDef{ 0 };
	int m_nMAtk{ 0 };
	int m_nMDef{ 0 };
	int m_nHP{ 0 };
	float m_nMaxTime{ 0 };
	float m_fCrit{ 0 };
	float m_fPreventCrit{ 0 };
	float m_fMoveSpeed{ 0 };
	float m_fAtkSpeed{ 0 };
	int m_nBulidingAtk{ 0 };
	int m_nAddStr{ 0 };
	int m_nAddInt{ 0 };
	int m_nAddCmd{ 0 };
};

struct Rune
{
	int m_nID{ 0 };
	int m_nQuality{ 0 };
	float m_fChance{ 0 };
	int m_nAtk{ 0 };
	int m_nDef{ 0 };
	int m_nMAtk{ 0 };
	int m_nMDef{ 0 };
	int m_nHP{ 0 };
	float m_nMaxTime{ 0 };
	float m_fCrit{ 0 };
	float m_fPreventCrit{ 0 };
	float m_fMoveSpeed{ 0 };
	float m_fAtkSpeed{ 0 };
	int m_nBulidingAtk{ 0 };
	int m_nAddStr{ 0 };
	int m_nAddInt{ 0 };
	int m_nAddCmd{ 0 };
};

struct RunePage
{
	int m_nPageID{ 0 };
	int m_nRuneNum{ 0 };
	int m_nAtk{ 0 };
	int m_nDef{ 0 };
	int m_nMAtk{ 0 };
	int m_nMDef{ 0 };
	int m_nHP{ 0 };
	int m_nMaxTime{ 0 };
	float m_fCrit{ 0 };
	float m_fPreventCrit{ 0 };
	float m_fMoveSpeed{ 0 };
	float m_fAtkSpeed{ 0 };
	int m_nBulidingAtk{ 0 };
	int m_nAddStr{ 0 };
	int m_nAddInt{ 0 };
	int m_nAddCmd{ 0 };
};

struct  RuneCell
{
	int m_nPage{ 0 };
	int m_nNum{ 0 };
	int m_nColour{ 0 };
};

class CRuneLibrary
{
public:
	CRuneLibrary();
	~CRuneLibrary();
	static void Load();
	static const Rune* GetRuneByID(int nID);

	static unsigned	WashRune(WashRuneType enWashType, int nColour, int nBrokeRune);
	static unsigned TestWash(int nColour, int nID);
	static int GetRuneCellColour(int nPos);

	static unsigned	SetRune(int nColor, int nID);
	static void	SetLock(unsigned& nRune, bool isLock);

	static int	GetRuneColor(unsigned nRune);
	static int	GetRuneID(unsigned nRune);
	static bool HasLock(unsigned nRune);

	static void CalculateRuneAddition(RuneAddition* pAddtition, std::array<unsigned, 16>* runes);
	static const RunePage* GetRunePage(int nPage, int num);
private:
	static std::vector<const Rune*> ms_vctRune;
	static std::vector<const RunePage*> ms_vctRunePage;
	static std::vector<const RuneCell*> ms_vctRuneCell;
	static std::array<int, 4> ms_arrNormalWash;
	static std::array<int, 4> ms_arrStrengthenWash;

	static int __ProduceRuneLevel(std::array<int, 4> arrChance);
	static int __ProduceRuneID(int nRuneLevel);
};

