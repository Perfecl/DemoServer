#include "stdafx.h"
#include "ExpLibrary.h"

std::map<int, const CExpLibrary*> CExpLibrary::ms_mapExpLibrary;

float CExpLibrary::ms_nSoldierAllExp{ 0 };
float CExpLibrary::ms_nSoldierAtkExp{ 0 };
float CExpLibrary::ms_nSoldierMAtkExp{ 0 };
float CExpLibrary::ms_nSoldierDefExp{ 0 };
float CExpLibrary::ms_nSoldierMDefExp{ 0 };
float CExpLibrary::ms_nSoldierHPExp{ 0 };

void CExpLibrary::Load()
{
	Exp_Library libExp;
	libExp.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Exp.txt"));

	if (libExp.has_tech_exp())
	{
		auto itTechExp = libExp.tech_exp();
		ms_nSoldierAllExp = itTechExp.all();
		ms_nSoldierAtkExp = itTechExp.atk();
		ms_nSoldierMAtkExp = itTechExp.m_atk();
		ms_nSoldierDefExp = itTechExp.def();
		ms_nSoldierMDefExp = itTechExp.m_def();
		ms_nSoldierHPExp = itTechExp.hp();
	}

	for (int i = 0; i < libExp.exp_librarys_size(); i++)
	{
		auto it = libExp.exp_librarys(i);

		CExpLibrary *pExpLib = new CExpLibrary;
		pExpLib->m_nLevel = it.lv();
		pExpLib->m_nExp = it.exp();
		pExpLib->m_nStage = it.stage();
		pExpLib->m_nSoldierExp = it.soldier_exp();

		ms_mapExpLibrary.insert(std::make_pair(pExpLib->m_nLevel, pExpLib));
	}
}

const CExpLibrary* CExpLibrary::GetExpLibrary(int nLV)
{
	auto it = ms_mapExpLibrary.find(nLV);
	if (ms_mapExpLibrary.cend() == it)
		return nullptr;
	else
		return it->second;
}

__int64 CExpLibrary::GetSoldierExp(int nLV, SoldierTechnology type)
{
	const CExpLibrary* pExpLib = GetExpLibrary(nLV + 1);

	if (!pExpLib)
		return -1;

	float fTypeMul{ 0 };

	switch (type)
	{
	case kSoldierTechAtk:
		fTypeMul = ms_nSoldierAtkExp; 
		break;
	case kSoldierTechMatk:
		fTypeMul = ms_nSoldierMAtkExp; 
		break;
	case kSoldierTechDef:
		fTypeMul = ms_nSoldierDefExp; 
		break;
	case kSoldierTechMdef:
		fTypeMul = ms_nSoldierMDefExp; 
		break;
	case kSoldierTechHP:
		fTypeMul = ms_nSoldierHPExp ; 
		break;
	}

	return static_cast<__int64>(pExpLib->m_nSoldierExp * fTypeMul / ms_nSoldierAllExp);
}
