#include "stdafx.h"
#include "RuneLibrary.h"

std::vector<const Rune*> CRuneLibrary::ms_vctRune;
std::vector<const RunePage*> CRuneLibrary::ms_vctRunePage;
std::vector<const RuneCell*> CRuneLibrary::ms_vctRuneCell;
std::array<int, 4> CRuneLibrary::ms_arrNormalWash;
std::array<int, 4> CRuneLibrary::ms_arrStrengthenWash;

CRuneLibrary::CRuneLibrary()
{

}

CRuneLibrary::~CRuneLibrary()
{

}

void CRuneLibrary::Load()
{
	//碎50 % 1级37 % 2级10 % 3级0 %
	ms_arrNormalWash[0] = 50;
	ms_arrNormalWash[1] = 40;
	ms_arrNormalWash[2] = 10;
	ms_arrNormalWash[3] = 0;

	//强化洗 碎0 % 1级60 % 2级30 % 3级10 %
	ms_arrStrengthenWash[0] = 0;
	ms_arrStrengthenWash[1] = 60;
	ms_arrStrengthenWash[2] = 30;
	ms_arrStrengthenWash[3] = 10;

	dat_STRUCT_RuneDataLibrary libData;
	libData.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Rune.txt"));

	for (int i = 0; i < libData.rune_library_size(); i++)
	{
		Rune* pRune = new Rune;
		pRune->m_nID = libData.rune_library(i).id();
		pRune->m_nQuality = libData.rune_library(i).quality();
		pRune->m_fChance = libData.rune_library(i).chance();
		pRune->m_nAtk = libData.rune_library(i).atk();
		pRune->m_nDef = libData.rune_library(i).def();
		pRune->m_nMAtk = libData.rune_library(i).matk();
		pRune->m_nMDef = libData.rune_library(i).mdef();
		pRune->m_nHP = libData.rune_library(i).hp();
		pRune->m_nMaxTime = (float)libData.rune_library(i).max_time();
		pRune->m_fCrit = libData.rune_library(i).crit();
		pRune->m_fPreventCrit = libData.rune_library(i).prevent_crit();
		pRune->m_fMoveSpeed = libData.rune_library(i).move_speed();
		pRune->m_fAtkSpeed = libData.rune_library(i).atk_speed();
		pRune->m_nBulidingAtk = libData.rune_library(i).buliding_atk();
		pRune->m_nAddStr = libData.rune_library(i).add_str();
		pRune->m_nAddInt = libData.rune_library(i).add_int();
		pRune->m_nAddCmd = libData.rune_library(i).add_cmd();

		ms_vctRune.push_back(pRune);
	}

	for (int i = 0; i < libData.page_addition_library_size(); i++)
	{
		RunePage* pPage = new RunePage;
		pPage->m_nPageID = libData.page_addition_library(i).page();
		pPage->m_nRuneNum = libData.page_addition_library(i).rune_num();
		pPage->m_nAtk = libData.page_addition_library(i).atk();
		pPage->m_nDef = libData.page_addition_library(i).def();
		pPage->m_nMAtk = libData.page_addition_library(i).matk();
		pPage->m_nMDef = libData.page_addition_library(i).mdef();
		pPage->m_nHP = libData.page_addition_library(i).hp();
		pPage->m_nMaxTime = (int)libData.page_addition_library(i).max_time();
		pPage->m_fCrit = libData.page_addition_library(i).crit();
		pPage->m_fPreventCrit = libData.page_addition_library(i).prevent_crit();
		pPage->m_fMoveSpeed = libData.page_addition_library(i).move_speed();
		pPage->m_fAtkSpeed = libData.page_addition_library(i).atk_speed();
		pPage->m_nBulidingAtk = libData.page_addition_library(i).buliding_atk();
		pPage->m_nAddStr = libData.page_addition_library(i).add_str();
		pPage->m_nAddInt = libData.page_addition_library(i).add_int();
		pPage->m_nAddCmd = libData.page_addition_library(i).add_cmd();

		ms_vctRunePage.push_back(pPage);
	}

	for (int i = 0; i < libData.page_cell_library_size(); i++)
	{
		RuneCell* pCell = new RuneCell;
		pCell->m_nPage = libData.page_cell_library(i).page();
		pCell->m_nNum = libData.page_cell_library(i).num();
		pCell->m_nColour = libData.page_cell_library(i).colour();

		ms_vctRuneCell.push_back(pCell);
	}
}

const Rune* CRuneLibrary::GetRuneByID(int nID)
{
	for (auto it : ms_vctRune)
	{
		if (nID == it->m_nID)
			return it;
	}
	return nullptr;
}

unsigned CRuneLibrary::WashRune(WashRuneType enWashType, int nColour, int nBrokeRune)
{
	int nRuneLevel{ 0 };
	int nRuneColour{ 1 };
	int nRuneID{ 0 };

	switch (enWashType)
	{
	case WRT_Nomal:
		nRuneLevel = __ProduceRuneLevel(ms_arrNormalWash);
		while (nRuneLevel == 3 && enWashType == WashRuneType::WRT_Nomal)
			nRuneLevel = __ProduceRuneLevel(ms_arrNormalWash);
		while (!nRuneLevel && nBrokeRune == 3)
		{
			nRuneLevel = __ProduceRuneLevel(ms_arrNormalWash);
		}
		nRuneColour = GetRandom(1, 4);
		break;
	case WRT_Strengthen:
		nRuneLevel = __ProduceRuneLevel(ms_arrStrengthenWash);
		nRuneColour = GetRandom(1, 4);
		//nRuneColour = nColour;
		break;
	case WRT_Item:
		nRuneLevel = 3;
		nRuneColour = nColour;
		break;
	default:
		break;
	}
	if (nRuneLevel)
		nRuneID = __ProduceRuneID(nRuneLevel);
	else
		nRuneID = Broke_Rune_ID;
	return SetRune(nRuneColour, nRuneID);
}

unsigned CRuneLibrary::TestWash(int nColour, int nID)
{
	for (size_t i = 0; i < ms_vctRune.size(); i++)
	{
		if (ms_vctRune.at(i)->m_nID == nID)
		{
			return SetRune(nColour, nID);
		}
	}
	return 0;
}

unsigned CRuneLibrary::SetRune(int nColor, int nID)
{
	unsigned nRune = nID;

	switch (nColor)
	{
	case 2:nRune |= 536870912; break;
	case 3:nRune |= 1073741824; break;
	case 4:nRune |= 1610612736; break;
	}
	return nRune;
}

void CRuneLibrary::SetLock(unsigned& nRune, bool isLock)
{
	if (isLock)
		nRune |= 2147483648;
	else
		nRune &= ~2147483648;
}

int	CRuneLibrary::GetRuneColor(unsigned nRune)
{
	nRune <<= 1;
	nRune >>= 30;

	return nRune + 1;
}

int	CRuneLibrary::GetRuneID(unsigned nRune)
{
	nRune <<= 3;
	nRune >>= 3;
	return nRune;
}

bool CRuneLibrary::HasLock(unsigned nRune)
{
	if (nRune & 2147483648)
		return true;
	else
		return false;
}

int CRuneLibrary::GetRuneCellColour(int nPos)
{
	for (auto it : ms_vctRuneCell)
	{
		if (nPos == it->m_nNum)
			return it->m_nColour;
	}
	return 1;
}

const RunePage* CRuneLibrary::GetRunePage(int nPage, int num)
{
	for (auto it : ms_vctRunePage)
	{
		if (it->m_nPageID == nPage &&
			it->m_nRuneNum == num)
		{
			return it;
		}
	}
	return nullptr;
}

void CRuneLibrary::CalculateRuneAddition(RuneAddition* pAddition, std::array<unsigned, 16>* runes)
{
 	int nPos{ 0 };
	std::array<int, 4> arrPageCounter;
	arrPageCounter.fill(0);

	for (auto it : *runes)
	{
		const Rune* pRune = CRuneLibrary::GetRuneByID(CRuneLibrary::GetRuneID(it));
		if (pRune)
		{
			pAddition->m_nAtk += pRune->m_nAtk;
			pAddition->m_nDef += pRune->m_nDef;
			pAddition->m_nMAtk += pRune->m_nMAtk;
			pAddition->m_nMDef += pRune->m_nMDef;
			pAddition->m_nHP += pRune->m_nHP;

			pAddition->m_nMaxTime += pRune->m_nMaxTime;
			pAddition->m_fCrit += pRune->m_fCrit / 100;
			pAddition->m_fPreventCrit += pRune->m_fPreventCrit / 100;
			pAddition->m_fMoveSpeed += pRune->m_fMoveSpeed;
			pAddition->m_fAtkSpeed += pRune->m_fAtkSpeed / 100;
			pAddition->m_nBulidingAtk += pRune->m_nBulidingAtk;

			pAddition->m_nAddStr += pRune->m_nAddStr;
			pAddition->m_nAddCmd += pRune->m_nAddCmd;
			pAddition->m_nAddInt += pRune->m_nAddInt;

			if (CRuneLibrary::GetRuneColor(it) == GetRuneCellColour(nPos))
			{
				if (nPos >= 0 && nPos <= 3)
					arrPageCounter[0]++;
				else if (nPos >= 4 && nPos <= 7)
					arrPageCounter[1]++;
				else if (nPos >= 8 && nPos <= 11)
					arrPageCounter[2]++;
				else if (nPos >= 12 && nPos <= 15)
					arrPageCounter[3]++;
			}
		}
		nPos++;
	}

	for (size_t k = 0; k < 4; k++)
	{
		for (int i = 1; i <= arrPageCounter[k]; i++)
		{
			const RunePage* pPage = CRuneLibrary::GetRunePage(k + 1, i);
			if (pPage)
			{
				pAddition->m_nAtk += pPage->m_nAtk;
				pAddition->m_nDef += pPage->m_nDef;
				pAddition->m_nMAtk += pPage->m_nMAtk;
				pAddition->m_nMDef += pPage->m_nMDef;
				pAddition->m_nHP += pPage->m_nHP;

				pAddition->m_nMaxTime += pPage->m_nMaxTime;
				pAddition->m_fCrit += pPage->m_fCrit / 100;
				pAddition->m_fPreventCrit += pPage->m_fPreventCrit / 100;
				pAddition->m_fMoveSpeed += pPage->m_fMoveSpeed;
				pAddition->m_fAtkSpeed += pPage->m_fAtkSpeed / 100;
				pAddition->m_nBulidingAtk += pPage->m_nBulidingAtk;

				pAddition->m_nAddStr += pPage->m_nAddStr;
				pAddition->m_nAddCmd += pPage->m_nAddCmd;
				pAddition->m_nAddInt += pPage->m_nAddInt;
			}
		}
	}
	if (16 == arrPageCounter[0] +
		arrPageCounter[1] +
		arrPageCounter[2] +
		arrPageCounter[3])
	{
		const RunePage* pPage = CRuneLibrary::GetRunePage(999, 999);
		if (pPage)
		{
			pAddition->m_nAtk += pPage->m_nAtk;
			pAddition->m_nDef += pPage->m_nDef;
			pAddition->m_nMAtk += pPage->m_nMAtk;
			pAddition->m_nMDef += pPage->m_nMDef;
			pAddition->m_nHP += pPage->m_nHP;

			pAddition->m_nMaxTime += pPage->m_nMaxTime;
			pAddition->m_fCrit += pPage->m_fCrit / 100;
			pAddition->m_fPreventCrit += pPage->m_fPreventCrit / 100;
			pAddition->m_fMoveSpeed += pPage->m_fMoveSpeed;
			pAddition->m_fAtkSpeed += pPage->m_fAtkSpeed / 100;
			pAddition->m_nBulidingAtk += pPage->m_nBulidingAtk;

			pAddition->m_nAddStr += pPage->m_nAddStr;
			pAddition->m_nAddCmd += pPage->m_nAddCmd;
			pAddition->m_nAddInt += pPage->m_nAddInt;
		}
	}
}

int CRuneLibrary::__ProduceRuneID(int nRuneLevel)
{
	if (ms_vctRune.size() <= 0)
		return 0;

	while (true)
	{
		auto it = ms_vctRune[GetRandom<int>(0, ms_vctRune.size() - 1)];
		if (it->m_nQuality != nRuneLevel)
			continue;
		if (GetRandom(0, 100) < (int)(it->m_fChance * 100))
			return it->m_nID;
	}
}

int CRuneLibrary::__ProduceRuneLevel(std::array<int, 4> arrChance)
{
	int nChance = GetRandom(0, 100);
	if (nChance < arrChance[0])
		return 0;
	if (nChance < (arrChance[0] + arrChance[1]))
		return 1;
	if (nChance < (arrChance[0] + arrChance[1] + arrChance[2]))
		return 2;
	return 3;
}
