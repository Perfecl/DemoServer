#include "stdafx.h"
#include "OffLinePlayer.h"
#include "Player.h"
#include "Offline.h"
#include "GameServer.h"
#include "Army.h"
#include "Knapsack.h"
#include "HeroCard.h"

using namespace std;

map<int, COffLinePlayer*> COffLinePlayer::ms_mapOfflineBattleAI;

COffLinePlayer::COffLinePlayer()
{
	heroes.fill(nullptr);					//Ó¢ÐÛID
}

COffLinePlayer::~COffLinePlayer()
{

}

void COffLinePlayer::DeleteOfflineAI()
{
	for (auto &it : ms_mapOfflineBattleAI)
		delete it.second;
}

COffLinePlayer::COffLinePlayer(int nPID)
{
	std::array<int, 3>              m_arrHeroesID;

	m_arrHeroesID[0] = 0;
	m_arrHeroesID[1] = 0;
	m_arrHeroesID[2] = 0;
	m_nPID = nPID;

	if (0 == rand() % 2)
		m_bSex = true;
	else
		m_bSex = false;

	m_strName = CNameLibrary::GetRandomName(m_bSex);

	m_nLevel = 15 + (GetRandom(0, 5));

	int nMaxHeroId = CHeroTP::GetLibSize();

	m_arrHeroesID[0] = CHeroTP::RandomHeroID();

	while (m_arrHeroesID[1] == 0 || m_arrHeroesID[1] == m_arrHeroesID[0])
		m_arrHeroesID[1] = CHeroTP::RandomHeroID();

	while (m_arrHeroesID[2] == 0 || m_arrHeroesID[2] == m_arrHeroesID[0] || m_arrHeroesID[2] == m_arrHeroesID[1])
		m_arrHeroesID[2] = CHeroTP::RandomHeroID();

	for (size_t i = 0; i < 3; i++)
	{
		CHeroCard* hero_card = new CHeroCard(m_arrHeroesID[i]);
		heroes[i] = hero_card;
	}
}

void COffLinePlayer::ProduceOfflineBattleAI()
{
	for (size_t i = 1; i <= 200; i++)
	{
		COffLinePlayer* pOfflinePlayer = new COffLinePlayer(0 - i);
		ms_mapOfflineBattleAI.insert(make_pair(pOfflinePlayer->m_nPID, pOfflinePlayer));
	}
}

void COffLinePlayer::InsertAIIntoRanking()
{
	for (auto &it : ms_mapOfflineBattleAI)
		GAME_WORLD()->AddOfflineBattleRanking(it.second->m_nPID);
}

COffLinePlayer* COffLinePlayer::FindOffLinePlayerByPID(int nPID)
{
	auto it = ms_mapOfflineBattleAI.find(nPID);

	if (ms_mapOfflineBattleAI.cend() == it)
		return nullptr;
	else
		return it->second;
}

void COffLinePlayer::SetOffLinePlayerByPlayer(CPlayer* pPlayer)
{
	m_nPID = pPlayer->pid();
	m_strName = pPlayer->name();
	m_bSex = pPlayer->sex();
	m_nLevel = pPlayer->level();
	m_nDressID = pPlayer->clothes();

	for (size_t i = 0; i < 3; i++)
	{
		pPlayer->army()->GetFormationHero();
	}

	int i = 0;
	for (auto &it_hero : pPlayer->army()->GetFormationHero())
	{
		CHeroCard* hero_card = pPlayer->army()->FindHero(it_hero);
		if (hero_card)
		{
			heroes[i] = hero_card;
			for (auto it_equip : pPlayer->knapsack()->GetHeroEquip(it_hero))
			{
				equips.insert(std::make_pair(it_equip, it_hero));
			}
		}
		i++;
	}
}

void COffLinePlayer::SetOffLinePlayerByData(SPIPlayerData data)
{
	if (!data)
		return;

	m_nPID = data->pid();
	m_strName = data->name();
	m_bSex = data->sex();
	m_nLevel = data->level();
	m_nDressID = data->clothes();

	for (int i = 0; i < 3; i++)
	{
		heroes[i] = data->hero(i);

		if (heroes[i])
		{
			for (auto &it : data->hero_equip(heroes[i]->hero_id()))
			{
				equips.insert(std::make_pair(it, heroes[i]->hero_id()));
			}
		}
	}
}
