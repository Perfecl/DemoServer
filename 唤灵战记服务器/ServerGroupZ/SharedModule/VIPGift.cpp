#include "stdafx.h"
#include "VIPGift.h"

std::map<int, SPRechargeReward> CVIPGift::recharge_reward_lib_;
std::map<int, SPCheckInGift> CVIPGift::check_in_gift_lib_;
std::map<int, SPDailyGift> CVIPGift::daily_gift_lib_;

CVIPGift::CVIPGift()
{
}

CVIPGift::~CVIPGift()
{
}

void CVIPGift::Load()
{
	dat_VIP_STRUCT_VIPLibrary libVIP;

	libVIP.ParseFromString(GetDataFromFile(GAME_DATA_PATH"VIP.txt"));

	for (int i = 0; i < libVIP.recharge_reward_size(); i++)
	{
		SPRechargeReward reward = std::make_shared<RechargeReward>();
		reward->vip_level = libVIP.recharge_reward(i).vip_level();
		for (int k = 0; k < libVIP.recharge_reward(i).item_size(); k++)
		{
			int id = libVIP.recharge_reward(i).item(k).id();
			if (id)
			{
				int num = libVIP.recharge_reward(i).item(k).num();
				reward->item.push_back(std::make_pair(id, num));
			}
		}
		for (int k = 0; k < libVIP.recharge_reward(i).equip_size(); k++)
		{
			int id = libVIP.recharge_reward(i).equip(k).id();
			if (id)
			{
				int num = libVIP.recharge_reward(i).equip(k).num();
				reward->equip.push_back(std::make_pair(id, num));
			}	
		}
		reward->hero = libVIP.recharge_reward(i).hero();
		reward->soldier = libVIP.recharge_reward(i).soldier();
		reward->fashion = libVIP.recharge_reward(i).fashion();
		recharge_reward_lib_.insert(std::make_pair(reward->vip_level, reward));
	}

	for (int i = 0; i < libVIP.check_in_gift_size(); i++)
	{
		SPCheckInGift gift = std::make_shared<CheckInGift>();
		gift->day = libVIP.check_in_gift(i).day();
		for (int k = 0; k < libVIP.check_in_gift(i).item_size(); k++)
		{
			int id = libVIP.check_in_gift(i).item(k).id();
			if (id)
			{
				int num = libVIP.check_in_gift(i).item(k).num();
				gift->item.push_back(std::make_pair(id, num));
			}
		}
		gift->hero = libVIP.check_in_gift(i).hero();
		check_in_gift_lib_.insert(std::make_pair(gift->day, gift));
	}

	for (int i = 0; i < libVIP.daily_gift_size(); i++)
	{
		SPDailyGift gift = std::make_shared<DailyGift>();
		gift->vip_level = libVIP.daily_gift(i).vip_level();
		gift->gold = libVIP.daily_gift(i).gold();
		gift->silver = libVIP.daily_gift(i).silver();
		daily_gift_lib_.insert(std::make_pair(gift->vip_level, gift));
	}
}

SPRechargeReward CVIPGift::GetRechargeReward(int vip_level)
{
	auto it = recharge_reward_lib_.find(vip_level);
	if (recharge_reward_lib_.cend() == it)
		return nullptr;
	else
		return it->second;
}

SPCheckInGift CVIPGift::GetCheckInGift(int days)
{
	auto it = check_in_gift_lib_.find(days);
	if (check_in_gift_lib_.cend() == it)
		return nullptr;
	else
		return it->second;
}

SPDailyGift CVIPGift::GetDailyGift(int vip_level)
{
	auto it = daily_gift_lib_.find(vip_level);
	if (daily_gift_lib_.cend() == it)
		return nullptr;
	else
		return it->second;
}