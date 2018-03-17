#pragma once
#include "CustomBullet.h"

class CEditMap
{
	friend class CStageBattle;
public:
	static void Load();
	static const CEditMap* GetEditMap(int nID);

private:
	static std::map<int, const CEditMap*> ms_mapEditMaps;

public:
	CEditMap();
	~CEditMap() = default;

private:
	int				m_nMapID{ 0 };
	int				m_nSceneID{ 0 };
	int				m_nBeginStory{ -1 };
	float           m_fWidth{ 0 };

	std::array<int, 6>	m_arrMasterType;  //(0.ÎÞ 1.Íæ¼Ò 2.µçÄÔ)

	std::string			m_strStageName;
	std::string			m_strVictorDescribe;
	std::string			m_strLostDescribe;

	bool			m_bAtkCastleOver{ false };
	bool			m_bDefCastleOver{ false };

	bool			atk_base_gun_{ false };
	bool			def_base_gun_{ false };

	bool			try_hero_{ false };
	int				try_hero_id_{ 0 };
	int				try_hero_level_{ 0 };

	std::vector<const CUnit*>				m_vctUnits;
	std::vector<const CHero*>				m_vctHeroes;
	std::vector<const CBuilding*>			m_vctBuildings;
	std::array<const CMaster*, 6>			m_arrMasters;
	std::vector<std::pair<int, bool>>		m_vctMapVariable;
	std::vector<const CIncident*>			m_vctIncidents;
	std::vector<const CGate*>               m_vctGate;
	std::vector<const CCustomBullet*>       m_vctCustomBullet;
};

