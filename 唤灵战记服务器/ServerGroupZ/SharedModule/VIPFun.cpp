#include "stdafx.h"
#include "VIPFun.h"

std::map<int, const CVIPFun*> CVIPFun::ms_mapVIPFun;

CVIPFun::CVIPFun()
{

}

CVIPFun::~CVIPFun()
{

}

void CVIPFun::Load()
{
	dat_VIP_STRUCT_VIPLibrary libVIP;
	libVIP.ParseFromString(GetDataFromFile(GAME_DATA_PATH"VIP.txt"));

	for (int i = 0; i < libVIP.vip_fun_size(); i++)
	{
		CVIPFun* pVIPFun = new CVIPFun;
		pVIPFun->m_nVIPLevel = libVIP.vip_fun(i).vip_lv();
		pVIPFun->m_nRecharge = libVIP.vip_fun(i).recharge();
		pVIPFun->m_nBuyStaminaTimes = libVIP.vip_fun(i).buy_stamina();
		pVIPFun->m_nGoldPointing = libVIP.vip_fun(i).gold_pointing();
		pVIPFun->m_nBag = libVIP.vip_fun(i).bag();
		pVIPFun->m_nEscortProtectTimes = libVIP.vip_fun(i).protect();
		pVIPFun->m_nBuyRewardTimes = libVIP.vip_fun(i).buy_reward();
		pVIPFun->m_nMeditationTime = libVIP.vip_fun(i).meditation_time();
		pVIPFun->m_nDonationGold = libVIP.vip_fun(i).donation_gold();
		pVIPFun->m_nBuyOfflinebattleTimes = libVIP.vip_fun(i).buy_offlinebattle();
		pVIPFun->m_bVipPackage = libVIP.vip_fun(i).vip_package();
		pVIPFun->m_bOpenRandCard = libVIP.vip_fun(i).rand_card();
		pVIPFun->m_bTrainPet = libVIP.vip_fun(i).train_pet();
		pVIPFun->m_bRelievedInteriorCD = libVIP.vip_fun(i).interior_cd();
		pVIPFun->m_bOpenTrainPlatinum = libVIP.vip_fun(i).train_platinum();
		pVIPFun->m_bFullRuneEngery = libVIP.vip_fun(i).full_rune_engery();
		pVIPFun->m_bRelievedHeroInteriorCD = libVIP.vip_fun(i).hero_interior_cd();
		pVIPFun->m_bSkipOfflinebattle = libVIP.vip_fun(i).skip_offlinebattle();
		pVIPFun->m_bUpgradeEquipCrit = libVIP.vip_fun(i).crit();
		pVIPFun->m_bExLottery = libVIP.vip_fun(i).ex_box();
		pVIPFun->m_bQuickTrainPet = libVIP.vip_fun(i).quick_train_pet();
		pVIPFun->m_bOpenTrainExtreme = libVIP.vip_fun(i).train_extreme();
		pVIPFun->m_bRelievedOfflinebattleCD = libVIP.vip_fun(i).offlinebattle_cd();
		pVIPFun->m_bLowerTechnologyPrice = libVIP.vip_fun(i).technology_price();
		pVIPFun->m_bQuickChange = libVIP.vip_fun(i).quick_change();
		pVIPFun->m_bGoldReward = libVIP.vip_fun(i).gold_reward();
		pVIPFun->m_bAutoMeltingRune = libVIP.vip_fun(i).auto_melting_rune();
		pVIPFun->buy_exercise_platform_times_ = libVIP.vip_fun(i).buy_exercise_platform_times();

		ms_mapVIPFun.insert(std::make_pair(pVIPFun->m_nVIPLevel, pVIPFun));
	}
}

const CVIPFun* CVIPFun::GetVIPFun(int nLV)
{
	auto it = ms_mapVIPFun.find(nLV);
	if (ms_mapVIPFun.cend() == it)
		return nullptr;
	else
		return it->second;
}

int CVIPFun::GetVIPFun(int nVIPLevel, VIPFunType enType)
{
	auto it = ms_mapVIPFun.find(nVIPLevel);

	if (ms_mapVIPFun.cend() == it)
	{
		return 0;
	}
	else
	{
		switch (enType)
		{
		case (int)VFT_VIPLevel:
			return it->second->m_nVIPLevel;

		case (int)VFT_Recharge:
			return it->second->m_nRecharge;

		case (int)VFT_BuyStaminaTimes:
			return it->second->m_nBuyStaminaTimes;

		case (int)VFT_GoldPointing:
			return it->second->m_nGoldPointing;

		case (int)VFT_Bag:
			return it->second->m_nBag;

		case (int)VFT_EscortProtectTimes:
			return it->second->m_nEscortProtectTimes;

		case (int)VFT_BuyRewardTimes:
			return it->second->m_nBuyRewardTimes;

		case (int)VFT_MeditationTime:
			return it->second->m_nMeditationTime;

		case (int)VFT_DonationGold:
			return it->second->m_nDonationGold;

		case (int)VFT_BuyOfflinebattleTimes:
			return it->second->m_nBuyOfflinebattleTimes;

		case (int)VFT_VipPackage:
			return it->second->m_bVipPackage;

		case (int)VFT_OpenRandCard:
			return it->second->m_bOpenRandCard;

		case (int)VFT_TrainPet:
			return it->second->m_bTrainPet;

		case (int)VFT_RelievedInteriorCD:
			return it->second->m_bRelievedInteriorCD;

		case (int)VFT_OpenTrainPlatinum:
			return it->second->m_bOpenTrainPlatinum;

		case (int)VFT_FullRuneEngery:
			return it->second->m_bFullRuneEngery;

		case (int)VFT_RelievedHeroInteriorCD:
			return it->second->m_bRelievedHeroInteriorCD;

		case (int)VFT_SkipOfflinebattle:
			return it->second->m_bSkipOfflinebattle;

		case (int)VFT_UpgradeEquipCrit:
			return it->second->m_bUpgradeEquipCrit;

		case (int)VFT_ExLottery:
			return it->second->m_bExLottery;

		case (int)VFT_QuickTrainPet:
			return it->second->m_bQuickTrainPet;

		case (int)VFT_OpenTrainExtreme:
			return it->second->m_bOpenTrainExtreme;

		case (int)VFT_RelievedOfflinebattleCD:
			return it->second->m_bRelievedOfflinebattleCD;

		case (int)VFT_LowerTechnologyPrice:
			return it->second->m_bLowerTechnologyPrice;

		case (int)VFT_QuickChange:
			return it->second->m_bQuickChange;

		case (int)VFT_GoldReward:
			return it->second->m_bGoldReward;

		case (int)VFT_AutoMeltingRune:
			return it->second->m_bAutoMeltingRune;

		case (int)VFT_BuyExercisePlatformTimes:
			return it->second->buy_exercise_platform_times_;

		default:
			return 0;
		}
	}
}

int CVIPFun::GetVIPLevel(int money)
{
	for (size_t i = 12; i >0 ; i--)
	{
		const CVIPFun* vip_fun = CVIPFun::GetVIPFun(i);
		if (vip_fun && money >= vip_fun->GetRecharge())
			return i;
	}
	return 0;
}