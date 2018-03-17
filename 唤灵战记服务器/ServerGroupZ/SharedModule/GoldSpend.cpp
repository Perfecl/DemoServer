#include "stdafx.h"
#include "GoldSpend.h"

std::map<int, const CGoldSpend*> CGoldSpend::ms_mapGoldSpend;
int CGoldSpend::m_nRandTradeCard;
int CGoldSpend::m_nRandEscortCar;
int CGoldSpend::m_nTrainHero;
int CGoldSpend::m_nTrainPlatinumHero;
int CGoldSpend::m_nTrainExtremeHero;

CGoldSpend::CGoldSpend()
{
}

CGoldSpend::~CGoldSpend()
{
}

void CGoldSpend::Load()
{
	dat_VIP_STRUCT_VIPLibrary libVIP;
	libVIP.ParseFromString(GetDataFromFile(GAME_DATA_PATH"VIP.txt"));

	m_nRandTradeCard = libVIP.rand_trade_card();
	m_nRandEscortCar = libVIP.rand_escort_car();
	m_nTrainHero = libVIP.train_hero();
	m_nTrainPlatinumHero = libVIP.train_hero_platinum();
	m_nTrainExtremeHero = libVIP.train_hero_extreme();

	for (int i = 0; i < libVIP.gold_spend_size(); i++)
	{
		CGoldSpend* pGoldSpend = new CGoldSpend;
		pGoldSpend->m_nTimes = libVIP.gold_spend(i).times();
		pGoldSpend->m_nBuyStamina = libVIP.gold_spend(i).buy_stamina();
		pGoldSpend->m_nBuyOfflineBattleSpend = libVIP.gold_spend(i).buy_offline_battle_spend();
		pGoldSpend->m_nBuyEliteStageSpend = libVIP.gold_spend(i).buy_elite_stage_spend();
		pGoldSpend->m_nBuyRewardMissionSpend = libVIP.gold_spend(i).buy_reward_mission_spend();
		pGoldSpend->m_nGoldPointingSpend = libVIP.gold_spend(i).gold_pointing_spend();
		pGoldSpend->m_nBuyMultiStageSpend = libVIP.gold_spend(i).buy_multi_stage_spend();
		pGoldSpend->buy_exercise_platform_spend_ = libVIP.gold_spend(i).buy_exercise_platform_spend();
		pGoldSpend->guild_contribute_spend_ = libVIP.gold_spend(i).guild_contribute_spend();
		ms_mapGoldSpend.insert(std::make_pair(pGoldSpend->m_nTimes, pGoldSpend));
	}
}

int CGoldSpend::GetGoldSpend(int nTimes, GoldSpendType enType)
{
	auto it = ms_mapGoldSpend.find(nTimes);
	if (ms_mapGoldSpend.cend() == it)
		return 0;
	else
	{
		switch (enType)
		{
		case (int)GST_RandTradeCard:
			return m_nRandTradeCard;
		case (int)GST_RandEscortCar:
			return m_nRandEscortCar;
		case (int)GST_TrainHero:
			return m_nTrainHero;
		case (int)GST_TrainPlatinum:
			return m_nTrainPlatinumHero;
		case (int)GST_TrainExtreme:
			return m_nTrainExtremeHero;

		case (int)GST_BuyStamina:
			return it->second->m_nBuyStamina;
		case (int)GST_BuyOfflineBattleSpend:
			return it->second->m_nBuyOfflineBattleSpend;
		case (int)GST_BuyEliteStageSpend:
			return it->second->m_nBuyEliteStageSpend;
		case (int)GST_BuyRewardMissionSpend:
			return it->second->m_nBuyRewardMissionSpend;
		case (int)GST_GoldPointingSpend:
			return it->second->m_nGoldPointingSpend;
		case (int)GST_BuyMultiStageSpend:
			return it->second->m_nBuyMultiStageSpend;
		case(int)GST_BuyExercisePlatformSpend:
			return it->second->buy_exercise_platform_spend_;
		case (int)GST_GuildContributeSpend:
			return it->second->guild_contribute_spend_;
		default:
			return 0;
		}
	}
}

int CGoldSpend::GetGoldSpend(GoldSpendType enType)
{
	switch (enType)
	{
	case (int)GST_RandTradeCard:
		return m_nRandTradeCard;
	case (int)GST_RandEscortCar:
		return m_nRandEscortCar;
	case (int)GST_TrainHero:
		return m_nTrainHero;
	case (int)GST_TrainPlatinum:
		return m_nTrainPlatinumHero;
	case (int)GST_TrainExtreme:
		return m_nTrainExtremeHero;
	default:
		return 0;
	}
}