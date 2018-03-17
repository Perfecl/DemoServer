#pragma once

enum GoldSpendType
{
	GST_RandTradeCard = 1,
	GST_RandEscortCar,
	GST_TrainHero,
	GST_TrainPlatinum,
	GST_TrainExtreme,
	GST_BuyStamina,
	GST_BuyOfflineBattleSpend,
	GST_BuyEliteStageSpend,
	GST_BuyRewardMissionSpend,
	GST_GoldPointingSpend,
	GST_BuyMultiStageSpend,
	GST_BuyExercisePlatformSpend,
	GST_GuildContributeSpend,
};

class CGoldSpend
{
public:
	CGoldSpend();
	~CGoldSpend();
	static void Load();
	static int GetGoldSpend(int nTimes, GoldSpendType enType);
	static int GetGoldSpend(GoldSpendType enType);

private:
	static std::map<int, const CGoldSpend*> ms_mapGoldSpend;
	static int m_nRandTradeCard;
	static int m_nRandEscortCar;
	static int m_nTrainHero;
	static int m_nTrainPlatinumHero;
	static int m_nTrainExtremeHero;
	int m_nTimes{ 0 };
	int m_nBuyStamina{ 0 };
	int m_nBuyOfflineBattleSpend{ 0 };
	int m_nBuyEliteStageSpend{ 0 };
	int m_nBuyRewardMissionSpend{ 0 };
	int m_nGoldPointingSpend{ 0 };
	int m_nBuyMultiStageSpend{ 0 };
	int buy_exercise_platform_spend_{ 0 };
	int guild_contribute_spend_{ 0 };
};

