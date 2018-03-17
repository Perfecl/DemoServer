#include "stdafx.h"
#include "TownRankBox.h"

std::map<int, const CTownRankBox*> CTownRankBox::ms_mapTownRankBox;

CTownRankBox::CTownRankBox()
{

}

CTownRankBox::~CTownRankBox()
{

}

void CTownRankBox::Load()
{
	dat_STAGE_STRUCT_TownBoxLibrary libTownBox;
	libTownBox.ParseFromString(GetDataFromFile(GAME_DATA_PATH"TownBox.txt"));

	for (int i = 0; i < libTownBox.town_rank_box_size(); i++)
	{
		CTownRankBox* pBox = new CTownRankBox;
		pBox->m_nTownID = libTownBox.town_rank_box(i).town_id();
		pBox->m_nSilver = libTownBox.town_rank_box(i).silver();
		pBox->m_nGold = libTownBox.town_rank_box(i).gold();
		pBox->m_nHonour = libTownBox.town_rank_box(i).honour();
		pBox->m_nStamina = libTownBox.town_rank_box(i).stamina();
		pBox->m_nPlayerTitle = libTownBox.town_rank_box(i).player_title();

		ms_mapTownRankBox.insert(std::make_pair(pBox->m_nTownID, pBox));
	}
}

const CTownRankBox* CTownRankBox::GetTownRankBox(int nTownID)
{
	auto it = ms_mapTownRankBox.find(nTownID);
	if (it == ms_mapTownRankBox.cend())
		return nullptr;
	else
		return it->second;
}