#pragma once
struct RechargeReward
{
	int vip_level{ 0 };
	std::vector<std::pair<int, int>>	item;
	std::vector<std::pair<int, int>>	equip;
	int hero{ 0 };
	int soldier{ 0 };
	int fashion{ 0 };
};

struct CheckInGift
{
	int day{ 0 };
	std::vector<std::pair<int, int>> item;
	int hero{ 0 };
};

struct DailyGift
{
	int vip_level{ 0 };
	int gold{ 0 };
	int silver{ 0 };
};

typedef std::shared_ptr<RechargeReward>		SPRechargeReward;
typedef std::shared_ptr<CheckInGift>		SPCheckInGift;
typedef std::shared_ptr<DailyGift>			SPDailyGift;

class CVIPGift
{
public:
	CVIPGift();
	~CVIPGift();
	static void Load();
	static SPRechargeReward GetRechargeReward(int vip_level);
	static SPCheckInGift GetCheckInGift(int days);
	static SPDailyGift GetDailyGift(int vip_level);
private:
	static std::map<int, SPRechargeReward> recharge_reward_lib_;
	static std::map<int, SPCheckInGift> check_in_gift_lib_;
	static std::map<int, SPDailyGift> daily_gift_lib_;
};


