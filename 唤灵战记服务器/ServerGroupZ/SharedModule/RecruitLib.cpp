#include "stdafx.h"
#include "RecruitLib.h"

std::map<int, const CRecruitLib*> CRecruitLib::ms_mapRecruitLib;
std::map<int, int> CRecruitLib::ms_mapGiftLib;
std::map<int, int> CRecruitLib::ms_mapCellLib;

void  CRecruitLib::Load()
{
	Recruit_Library libRecruit;
	libRecruit.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Recruit.txt"));

	for (int i = 0; i < libRecruit.recruit_librarys_size(); i++)
	{
		CRecruitLib* pRecruitLib = new CRecruitLib;
		pRecruitLib->m_nHeroID = libRecruit.recruit_librarys(i).id();
		pRecruitLib->m_nSilver = libRecruit.recruit_librarys(i).money();
		pRecruitLib->m_nReputation = libRecruit.recruit_librarys(i).reputation();
		pRecruitLib->m_nNeedLevel = libRecruit.recruit_librarys(i).need_lv();

		ms_mapRecruitLib.insert(std::make_pair(pRecruitLib->m_nHeroID, pRecruitLib));
	}

	for (int i = 0; i < libRecruit.gift_library_size(); i++)
		ms_mapGiftLib.insert(std::make_pair(libRecruit.gift_library(i).id(), libRecruit.gift_library(i).ratio()));

	for (int i = 0; i < libRecruit.game_cell_library_size(); i++)
		ms_mapCellLib.insert(std::make_pair(libRecruit.game_cell_library(i).id(), libRecruit.game_cell_library(i).gift_id()));
}

const CRecruitLib* CRecruitLib::GetRecruitLib(int nID)
{
	auto it = ms_mapRecruitLib.find(nID);
	if (ms_mapRecruitLib.cend() == it)
		return nullptr;
	else
		return it->second;
}

int CRecruitLib::GetGiftRatio(int nGiftID)
{
	auto it = ms_mapGiftLib.find(nGiftID);
	if (ms_mapGiftLib.cend() == it)
		return 0;
	else
		return it->second;
}

int CRecruitLib::GetGiftID(int nCellID)
{
	auto it = ms_mapCellLib.find(nCellID);
	if (ms_mapCellLib.cend() == it)
		return 0;
	else
		return it->second;
}

int CRecruitLib::ProduceCellID()
{
	int result = GetRandom(1, (int)ms_mapCellLib.size());
	if (result == 5 && GetRandom(1, 3) != 1)
		result = GetRandom(1, (int)ms_mapCellLib.size());
	else if (result == 17 && GetRandom(1, 3) != 1)
		result = GetRandom(1, (int)ms_mapCellLib.size());
	else if ((result == 1 || result == 21) && GetRandom(1, 2) != 1)
		result = GetRandom(1, (int)ms_mapCellLib.size());
	else if ((result == 9 || result == 13) && GetRandom(1, 2) != 1)
		result = GetRandom(1, (int)ms_mapCellLib.size());
	return result;
}
