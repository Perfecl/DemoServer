#include "stdafx.h"
#include "EditMap.h"
#include "Hero.h"
#include "Building.h"
#include "Master.h"
#include "Incident.h"
#include "Gate.h"
#include "CustomBullet.h"
#include <io.h>  

std::map<int, const CEditMap*> CEditMap::ms_mapEditMaps;

CEditMap::CEditMap()
{
	m_arrMasterType.fill(0);
}

void CEditMap::Load()
{
	std::vector<std::string> vctPath;

	_finddata_t finder;

	auto file = _findfirst(GAME_EDIT_MAP_PATH"*.map", &finder);

	if (-1 == file)
	{
		printf("¶Á²»µ½µØÍ¼");
	}
	else
	{
		do
		{
			vctPath.push_back(GAME_EDIT_MAP_PATH);
			vctPath[vctPath.size() - 1] += finder.name;
		} while (_findnext(file, &finder) == 0);
	}

	_findclose(file);

	for (auto &it : vctPath)
	{
		Map ptoEditMaps;
		ptoEditMaps.ParseFromString(GetDataFromFile(it.c_str()));

		CEditMap* pEditMap = new CEditMap;

		pEditMap->m_nMapID = ptoEditMaps.mapid();
		pEditMap->m_nSceneID = ptoEditMaps.backgroundid();
		pEditMap->m_nBeginStory = ptoEditMaps.begin_story();

		pEditMap->m_strStageName = ptoEditMaps.map_name();
		pEditMap->m_strVictorDescribe = ptoEditMaps.victor_describe();
		pEditMap->m_strLostDescribe = ptoEditMaps.lost_describe();

		pEditMap->m_bAtkCastleOver = ptoEditMaps.atk_castle_over();
		pEditMap->m_bDefCastleOver = ptoEditMaps.def_castle_over();

		pEditMap->atk_base_gun_ = ptoEditMaps.atk_base_gun();
		pEditMap->def_base_gun_ = ptoEditMaps.def_base_gun();

		pEditMap->try_hero_ = ptoEditMaps.try_hero();
		pEditMap->try_hero_id_ = ptoEditMaps.try_hero_id();
		pEditMap->try_hero_level_ = ptoEditMaps.try_hero_level();

		pEditMap->m_fWidth = (float)ptoEditMaps.width();

		for (int i = 0; i < ptoEditMaps.masters_type_size() && i < (int)pEditMap->m_arrMasterType.size(); i++)
			pEditMap->m_arrMasterType[i] = ptoEditMaps.masters_type(i);

		for (int i = 0; i < ptoEditMaps.master_info_size(); i++)
			pEditMap->m_arrMasters[i] = CMaster::CreateCustomMaster(ptoEditMaps.master_info(i));

		for (int i = 0; i < ptoEditMaps.soldiers_size(); i++)
		{
			int master_id = ptoEditMaps.soldiers(i).master();
			if (master_id < 1)
				master_id = 1;
			pEditMap->m_vctUnits.push_back(CUnit::CreateCustomUnit(ptoEditMaps.soldiers(i), pEditMap->m_arrMasters[master_id - 1]));
		}

		for (int i = 0; i < ptoEditMaps.heros_size(); i++)
			pEditMap->m_vctHeroes.push_back(CHero::CreateCustomHero(ptoEditMaps.heros(i)));

		for (int i = 0; i < ptoEditMaps.bulidings_size(); i++)
			pEditMap->m_vctBuildings.push_back(CBuilding::CreateCustomBuilding(ptoEditMaps.bulidings(i)));

		for (int i = 0; i < ptoEditMaps.map_variables_size(); i++)
			pEditMap->m_vctMapVariable.push_back(std::make_pair(ptoEditMaps.map_variables(i).id(), ptoEditMaps.map_variables(i).variable()));
		for (int i = 0; i < ptoEditMaps.incidents_size(); i++)
		{
			CIncident* pIncident = new CIncident;
			pIncident->SetIncidentByProto(ptoEditMaps.incidents(i));
			pEditMap->m_vctIncidents.push_back(pIncident);
		}

		for (int i = 0; i < ptoEditMaps.gates_size(); i++)
		{
			CGate* pGate = new CGate;
			pGate->SetGateByProto(ptoEditMaps.gates(i));
			pEditMap->m_vctGate.push_back(pGate);
		}

		for (int i = 0; i < ptoEditMaps.bullets_size(); i++)
		{
			CCustomBullet* pCustomBullet = new CCustomBullet;
			pCustomBullet->SetCustomBulletByProto(ptoEditMaps.bullets(i));
			pEditMap->m_vctCustomBullet.push_back(pCustomBullet);
		}

		auto it = ms_mapEditMaps.insert(std::make_pair(pEditMap->m_nMapID, pEditMap));
		if (false == it.second)
			delete pEditMap;
	}
}

const CEditMap* CEditMap::GetEditMap(int nID)
{
	auto it = ms_mapEditMaps.find(nID);
	if (ms_mapEditMaps.cend() == it)
		return nullptr;
	else
		return it->second;
}
