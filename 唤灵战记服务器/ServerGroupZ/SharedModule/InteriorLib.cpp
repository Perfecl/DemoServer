#include "stdafx.h"
#include "InteriorLib.h"

map<int, const CInteriorLib*> CInteriorLib::ms_mapInteriorLib;
map<int, const SUpgradeCost*> CInteriorLib::ms_mapUpgradeCost;
float	CInteriorLib::castle_atk_mul_;	//基地物攻每级加成
float	CInteriorLib::castle_matk_mul_;	//基地魔攻每级加成
float	CInteriorLib::castle_hp_mul_;	//基地血量每级加成

void CInteriorLib::Load()
{
	castle_atk_mul_ = 12;	
	castle_matk_mul_ = 12;	
	castle_hp_mul_ = 50;

	Interior_Library libInterior;
	libInterior.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Interior.txt"));
	 
	for (int i = 0; i < libInterior.upgrad_cost_size(); i++)
	{
		SUpgradeCost* pCost = new SUpgradeCost;
		pCost->m_nLevel = libInterior.upgrad_cost(i).level();
		pCost->m_nGold = libInterior.upgrad_cost(i).gold();
		pCost->m_nPlayerLevel = libInterior.upgrad_cost(i).player_level();
		pCost->m_nSilver = libInterior.upgrad_cost(i).silver();

		ms_mapUpgradeCost.insert(make_pair(pCost->m_nLevel, pCost));
	}

	for (int i = 0; i < libInterior.levy_library_size(); i++)
	{
		CInteriorLib* pInteriorLib = new CInteriorLib;
		pInteriorLib->m_nLevel = libInterior.levy_library(i).player_lv();
		pInteriorLib->m_nLevy = libInterior.levy_library(i).levy_silver();

		pInteriorLib->m_nFarmMoney = libInterior.produce_library(i).farm_money();
		pInteriorLib->m_fFarmAddition = libInterior.produce_library(i).farm_addition();
		pInteriorLib->m_nHuntMoney = libInterior.produce_library(i).hunt_money();
		pInteriorLib->m_fHuntAddition = libInterior.produce_library(i).hunt_addition();
		pInteriorLib->m_nSilverMoney = libInterior.produce_library(i).silver_money();
		pInteriorLib->m_fSilverAddition = libInterior.produce_library(i).silver_addition();
		pInteriorLib->m_nCrystalMoney = libInterior.produce_library(i).crystal_money();
		pInteriorLib->m_fCrystalAddition = libInterior.produce_library(i).crystal_addition();

		pInteriorLib->m_nPrimaryExp = libInterior.train_library(i).primary_exp();
		pInteriorLib->m_fPrimaryAddition = libInterior.train_library(i).primary_addition();
		pInteriorLib->m_nMiddleExp = libInterior.train_library(i).middle_exp();
		pInteriorLib->m_fMiddleAddition = libInterior.train_library(i).middle_addition();
		pInteriorLib->m_nSeniorExp = libInterior.train_library(i).senior_exp();
		pInteriorLib->m_fSeniorAddition = libInterior.train_library(i).senior_addition();
		pInteriorLib->m_nMaseterExp = libInterior.train_library(i).maseter_exp();
		pInteriorLib->m_fMasterAddition = libInterior.train_library(i).master_addition();
		pInteriorLib->m_nLVUpExp = libInterior.train_library(i).lv_up_exp();

		pInteriorLib->m_nResidenceReputation = libInterior.search_library(i).residence_reputation();
		pInteriorLib->m_nResidenceAddition = libInterior.search_library(i).residence_addition();
		pInteriorLib->m_nVillaReputation = libInterior.search_library(i).villa_reputation();
		pInteriorLib->m_fVillaAddition = libInterior.search_library(i).villa_addition();
		pInteriorLib->m_nCelebritiesReputation = libInterior.search_library(i).celebrities_reputation();
		pInteriorLib->m_fCelebritiesAddition = libInterior.search_library(i).celebrities_addition();
		pInteriorLib->m_nRoReputation = libInterior.search_library(i).ro_reputation();
		pInteriorLib->m_fRoAddition = libInterior.search_library(i).ro_addition();

		ms_mapInteriorLib.insert(make_pair(pInteriorLib->m_nLevel, pInteriorLib));
	}
}

int CInteriorLib::__GetMaxSize(Interior_Library* pLibInterior)
{
	int nResult = pLibInterior->produce_library_size();
	if (pLibInterior->train_library_size() > nResult)
		nResult = pLibInterior->train_library_size();
	if (pLibInterior->search_library_size() > nResult)
		nResult = pLibInterior->search_library_size();
	return nResult;
}

const CInteriorLib* CInteriorLib::GetInteriotLib(int nLV)
{
	auto it = ms_mapInteriorLib.find(nLV);
	if (ms_mapInteriorLib.cend() == it)
		return nullptr;
	else
		return it->second;
}

const SUpgradeCost* CInteriorLib::GetUpgradeCost(int nLevel)
{
	auto it = ms_mapUpgradeCost.find(nLevel);
	if (ms_mapUpgradeCost.cend() == it)
		return nullptr;
	else
		return it->second;
}