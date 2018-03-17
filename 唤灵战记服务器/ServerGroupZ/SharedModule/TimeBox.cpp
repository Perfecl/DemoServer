#include "stdafx.h"
#include "TimeBox.h"

std::map<int, const CTimeBox*> CTimeBox::ms_mapTimeBox;

CTimeBox::CTimeBox()
{
}


CTimeBox::~CTimeBox()
{
}

void CTimeBox::Load()
{
	pto_BOX_STRUCT_BoxLibrary libBox;
	libBox.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Box.txt"));

	for (int i = 0; i < libBox.time_box_library_size(); i++)
	{
		CTimeBox* pTimeBox = new CTimeBox;

		pTimeBox->m_nID = libBox.time_box_library(i).id();
		pTimeBox->m_nTime = libBox.time_box_library(i).time();
		pTimeBox->m_nSilver = libBox.time_box_library(i).silver();
		pTimeBox->m_nHonour = libBox.time_box_library(i).honour();

		ms_mapTimeBox.insert(std::make_pair(pTimeBox->m_nID, pTimeBox));
	}
}

const CTimeBox* CTimeBox::GetTimeBox(int nID)
{
	auto it = ms_mapTimeBox.find(nID);
	if (ms_mapTimeBox.cend() == it)
		return nullptr;
	else
		return it->second;
}