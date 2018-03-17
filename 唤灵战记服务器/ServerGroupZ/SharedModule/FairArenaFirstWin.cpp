#include "stdafx.h"
#include "FairArenaFirstWin.h"  

map<int, const CFairArenaFirstWin*> CFairArenaFirstWin::ms_mapFirstWin;

CFairArenaFirstWin::CFairArenaFirstWin()
{
}

CFairArenaFirstWin::~CFairArenaFirstWin()
{
}

void CFairArenaFirstWin::Load()
{
	pto_FAIRARENA_STRUCT_FairArena libFairArena;
	libFairArena.ParseFromString(GetDataFromFile(GAME_DATA_PATH"FairArena.txt"));

	for (int i = 0; i < libFairArena.first_win_library_size(); i++)
	{
		CFairArenaFirstWin* pFirstWin = new CFairArenaFirstWin;

		pFirstWin->m_nLevel = libFairArena.first_win_library(i).lv();
		pFirstWin->m_nSilver = libFairArena.first_win_library(i).silver();
		pFirstWin->m_nHonor = libFairArena.first_win_library(i).honor();

		ms_mapFirstWin.insert(make_pair(pFirstWin->m_nLevel, pFirstWin));
	}
}

const CFairArenaFirstWin* CFairArenaFirstWin::GetFairArenaFirstWin(int nLevel)
{
	auto it = ms_mapFirstWin.find(nLevel);
	if (ms_mapFirstWin.cend() == it)
		return nullptr;
	else
		return it->second;
}
