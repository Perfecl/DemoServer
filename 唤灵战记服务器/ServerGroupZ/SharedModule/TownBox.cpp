#include "stdafx.h"
#include "TownBox.h"

std::vector<const CTownBox*> CTownBox::ms_vctTownBox;

void CTownBox::Load()
{
	dat_STAGE_STRUCT_TownBoxLibrary libTownBox;
	libTownBox.ParseFromString(GetDataFromFile(GAME_DATA_PATH"TownBox.txt"));

	for (int i = 0; i < libTownBox.town_box_size(); i++)
	{
		CTownBox* pBox = new CTownBox;
		pBox->m_nTownID = libTownBox.town_box(i).town_id();
		pBox->m_nStageLevel = libTownBox.town_box(i).stage_level();
		pBox->m_nStageDifficulty = libTownBox.town_box(i).stage_difficulty();
		pBox->m_nSilver = libTownBox.town_box(i).silver();
		pBox->m_nGold = libTownBox.town_box(i).gold();
		pBox->m_nHonour = libTownBox.town_box(i).honour();
		pBox->m_nStamina = libTownBox.town_box(i).stamina();
		ms_vctTownBox.push_back(pBox);
		
	}
}

const CTownBox* CTownBox::GetTownBox(int nTownID, int nDifficult)
{
	for (auto &it : ms_vctTownBox)
	{
		if (nTownID == it->m_nTownID && nDifficult == it->m_nStageDifficulty)
			return it;
	}

	return nullptr;
}

const CTownBox* CTownBox::GetTownBoxByStageLevel(int nStageLevel)
{
	for (auto &it : ms_vctTownBox)
	if (nStageLevel == it->m_nStageLevel)
		return it;

	return nullptr;
}