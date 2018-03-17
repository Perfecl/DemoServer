#include "stdafx.h"
#include "RecastLib.h"

map<int, const CRecastLib*>  CRecastLib::ms_mapRecastLib;

CRecastLib::CRecastLib()
{
}


CRecastLib::~CRecastLib()
{
}

void CRecastLib::Load()
{
	Equip_Recast_Library libRecast;
	libRecast.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Recast.txt"));

	for (int i = 0; i < libRecast.recat_library_size(); i++)
	{
		CRecastLib* pRecastLib = new CRecastLib;
		pRecastLib->m_nEquipID = libRecast.recat_library(i).equip_id();
		pRecastLib->m_nStrengthenLV = libRecast.recat_library(i).strengthen_lv();
		pRecastLib->m_nMaterialEquipID = libRecast.recat_library(i).material_equip_id();
		pRecastLib->m_nDrawingID = libRecast.recat_library(i).drawing_id();
		pRecastLib->m_nDrawingNum = libRecast.recat_library(i).drawing_num();
		pRecastLib->m_nMoney = libRecast.recat_library(i).money();
		pRecastLib->m_nMaterial1ID = libRecast.recat_library(i).material_item_1_id();
		pRecastLib->m_nMaterial1Num = libRecast.recat_library(i).material_item_1_num();
		pRecastLib->m_nMaterial2ID = libRecast.recat_library(i).material_item_2_id();
		pRecastLib->m_nMaterial2Num = libRecast.recat_library(i).material_item_2_num();
		pRecastLib->m_nMaterial3ID = libRecast.recat_library(i).material_item_3_id();
		pRecastLib->m_nMaterial3Num = libRecast.recat_library(i).material_item_3_num();
		pRecastLib->m_nMaterial4ID = libRecast.recat_library(i).material_item_4_id();
		pRecastLib->m_nMaterial4Num = libRecast.recat_library(i).material_item_4_num();

		ms_mapRecastLib.insert(make_pair(pRecastLib->m_nMaterialEquipID, pRecastLib));
	}
}

const CRecastLib*  CRecastLib::GetRecastLib(int nID)
{
	auto it = ms_mapRecastLib.find(nID);
	if (ms_mapRecastLib.cend() == it)
		return nullptr;
	else
		return it->second;
}