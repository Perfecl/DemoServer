#include "stdafx.h"
#include "UnitTP.h"

int	CUnitTP::m_nBaseHP{ 0 };
int	CUnitTP::m_nBaseATK{ 0 };
int	CUnitTP::m_nBaseDEF{ 0 };
int	CUnitTP::m_nBaseMATK{ 0 };
int	CUnitTP::m_nBaseMDEF{ 0 };

void CUnitTP::Load()
{
	dat_UNIT_Library_All datAll;
	datAll.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Unit.txt"));

	auto baseValues = datAll.base_value();

	m_nBaseHP = baseValues.hp();
	m_nBaseATK = baseValues.atk();
	m_nBaseDEF = baseValues.def();
	m_nBaseMATK = baseValues.matk();
	m_nBaseMDEF = baseValues.mdef();
}