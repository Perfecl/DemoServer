#include "stdafx.h"
#include "InteriorEvent.h"

map<int, const CInteriorEvent*> CInteriorEvent::ms_mapProduceEvents;
map<int, const CInteriorEvent*> CInteriorEvent::ms_mapTrainEvents;
map<int, const CInteriorEvent*> CInteriorEvent::ms_mapSearchEvents;

void CInteriorEvent::Load()
{
	Interior_Library libInterior;
	libInterior.ParseFromString(GetDataFromFile(GAME_DATA_PATH"Interior.txt"));

	for (int i = 0; i < libInterior.produce_events_size(); i++)
	{
		CInteriorEvent* pEvent = new CInteriorEvent;
		pEvent->m_nID = libInterior.produce_events(i).id();
		pEvent->m_fChance = libInterior.produce_events(i).chance();
		pEvent->m_enRewardType = (InteriorEventRewardType)libInterior.produce_events(i).reward_type();
		pEvent->m_nRewardBase = libInterior.produce_events(i).reward_base();
		pEvent->m_nRewardAddition = libInterior.produce_events(i).reward_addition();
		
		ms_mapProduceEvents.insert(make_pair(pEvent->m_nID, pEvent));
	}

	for (int i = 0; i < libInterior.train_events_size(); i++)
	{
		CInteriorEvent* pEvent = new CInteriorEvent;
		pEvent->m_nID = libInterior.train_events(i).id();
		pEvent->m_fChance = libInterior.train_events(i).chance();
		pEvent->m_enRewardType = (InteriorEventRewardType)libInterior.train_events(i).reward_type();
		pEvent->m_nRewardBase = libInterior.train_events(i).reward_base();
		pEvent->m_nRewardAddition = libInterior.train_events(i).reward_addition();

		ms_mapTrainEvents.insert(make_pair(pEvent->m_nID, pEvent));
	}

	for (int i = 0; i < libInterior.search_events_size(); i++)
	{
		CInteriorEvent* pEvent = new CInteriorEvent;
		pEvent->m_nID = libInterior.search_events(i).id();
		pEvent->m_fChance = libInterior.search_events(i).chance();
		pEvent->m_enRewardType = (InteriorEventRewardType)libInterior.search_events(i).reward_type();
		pEvent->m_nRewardBase = libInterior.search_events(i).reward_base();
		pEvent->m_nRewardAddition = libInterior.search_events(i).reward_addition();

		ms_mapSearchEvents.insert(make_pair(pEvent->m_nID, pEvent));
	}
}

const CInteriorEvent* CInteriorEvent::RandomProduceEvents()
{
	if (!ms_mapProduceEvents.size())
		return nullptr;

	int nMax = ms_mapProduceEvents.size();
	int nK = GetRandom(1, nMax);

	if (GetRandom(0, 100) <= ms_mapProduceEvents.at(nK)->m_fChance * 100)
		return ms_mapProduceEvents.at(nK);

	return nullptr;
}

const CInteriorEvent* CInteriorEvent::RandomTrainEvents()
{
	if (!ms_mapTrainEvents.size())
		return nullptr;

	int nMax = ms_mapTrainEvents.size();
	int nK = GetRandom(1, nMax);

	if (GetRandom(0, 100) <= ms_mapTrainEvents.at(nK)->m_fChance * 100)
		return ms_mapTrainEvents.at(nK);

	return nullptr;
}

const CInteriorEvent* CInteriorEvent::RandomSearchEvents()
{
	if (!ms_mapSearchEvents.size())
		return nullptr;

	int nMax = ms_mapSearchEvents.size();
	int nK = GetRandom(1, nMax);

	if (GetRandom(0, 100) <= ms_mapSearchEvents.at(nK)->m_fChance * 100)
		return ms_mapTrainEvents.at(nK);

	return nullptr;
}