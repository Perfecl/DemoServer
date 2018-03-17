#pragma once

enum VIPFunType
{
	VFT_VIPLevel,					//等级
	VFT_Recharge,					//需要充钱
	VFT_BuyStaminaTimes,			//体力购买次数
	VFT_GoldPointing,				//点金次数
	VFT_Bag,						//额外背包格子
	VFT_EscortProtectTimes,			//额外保护次数
	VFT_BuyRewardTimes,				//购买悬赏次数
	VFT_MeditationTime,				//冥想时间
	VFT_DonationGold,				//公会捐献次数
	VFT_BuyOfflinebattleTimes,		//天梯购买次数
	VFT_VipPackage,					//VIP礼包
	VFT_OpenRandCard,				//刷新祈福
	VFT_TrainPet,					//宠物金币培养
	VFT_RelievedInteriorCD,			//内政免CD
	VFT_OpenTrainPlatinum,			//白金培养
	VFT_FullRuneEngery,				//熔炼符文
	VFT_RelievedHeroInteriorCD,		//英雄内政无需休息
	VFT_SkipOfflinebattle,			//跳过离线战斗
	VFT_UpgradeEquipCrit,			//强化暴击
	VFT_ExLottery,					//宝箱额外抽取
	VFT_QuickTrainPet,				//宠物一键金币培养
	VFT_OpenTrainExtreme,			//至尊培养
	VFT_RelievedOfflinebattleCD,	//离线竞技场免CD
	VFT_LowerTechnologyPrice,		//科技升级折扣15%
	VFT_QuickChange,				//一键金币兑换10次
	VFT_GoldReward,					//悬赏任务只出金色
	VFT_AutoMeltingRune,			//金币自动熔炼
	VFT_BuyExercisePlatformTimes,	//购买练功台次数
};

class CVIPFun
{
public:
	CVIPFun();
	~CVIPFun();
	static void Load();
	static const CVIPFun* GetVIPFun(int nLV);
	static int GetVIPFun(int nVIPLevel, VIPFunType enType);
	static int GetVIPLevel(int money);

	int  GetVIPLevel() const { return m_nVIPLevel; }
	int  GetRecharge() const { return m_nRecharge; }
	int  GetBuyStaminaTimes() const { return m_nBuyStaminaTimes; }
	int  GetGoldPointing() const { return m_nGoldPointing; }
	int  GetBag() const { return m_nBag; }
	int  GetEscortProtectTimes() const { return m_nEscortProtectTimes; }
	int  GetBuyReward() const { return m_nBuyRewardTimes; }
	int  GetMeditationTime() const { return m_nMeditationTime; }
	int  GetDonationGold() const { return m_nDonationGold; }
	int  GetBuyOfflinebattleTimes() const { return m_nBuyOfflinebattleTimes; }
	bool VipPackage() const { return m_bVipPackage; }
	bool OpenRandCard() const { return m_bOpenRandCard; }
	bool TrainPet() const { return m_bTrainPet; }
	bool RelievedInteriorCD() const { return m_bRelievedInteriorCD; }
	bool OpenTrainPlatinum() const { return m_bOpenTrainPlatinum; }
	bool FullRuneEngery() const { return m_bFullRuneEngery; }
	bool RelievedHeroInteriorCD() const { return m_bRelievedHeroInteriorCD; }
	bool SkipOfflinebattle() const { return m_bSkipOfflinebattle; }
	bool UpgradeEquipCrit() const { return m_bUpgradeEquipCrit; }
	bool ExLottery() const { return m_bExLottery; }
	bool QuickTrainPet() const { return m_bQuickTrainPet; }
	bool OpenTrainExtreme() const { return m_bOpenTrainExtreme; }
	bool RelievedOfflinebattleCD() const { return m_bRelievedOfflinebattleCD; }
	bool LowerTechnologyPrice() const { return m_bLowerTechnologyPrice; }
	bool QuickChange() const { return m_bQuickChange; }
	bool GoldReward() const { return m_bGoldReward; }
	bool AutoMeltingRune() const { return m_bAutoMeltingRune; }

private:
	static std::map<int, const CVIPFun*> ms_mapVIPFun;
	int  m_nVIPLevel{ 0 };					//等级
	int  m_nRecharge{ 0 };					//需要充钱
	int  m_nBuyStaminaTimes{ 0 };			//体力购买次数
	int  m_nGoldPointing{ 0 };				//点金次数
	int  m_nBag{ 0 };						//额外背包格子
	int  m_nEscortProtectTimes{ 0 };		//额外保护次数
	int  m_nBuyRewardTimes{ 0 };			//购买悬赏次数
	int  m_nMeditationTime{ 0 };			//冥想时间
	int  m_nDonationGold{ 0 };				//公会捐献次数
	int  m_nBuyOfflinebattleTimes{ 0 };		//天梯购买次数
	bool m_bVipPackage{ 0 };				//VIP礼包
	bool m_bOpenRandCard{ 0 };				//刷新祈福
	bool m_bTrainPet{ 0 };					//宠物金币培养
	bool m_bRelievedInteriorCD{ 0 };		//内政免CD
	bool m_bOpenTrainPlatinum{ 0 };			//白金培养
	bool m_bFullRuneEngery{ 0 };			//熔炼符文
	bool m_bRelievedHeroInteriorCD{ 0 };	//英雄内政无需休息
	bool m_bSkipOfflinebattle{ 0 };			//跳过离线战斗
	bool m_bUpgradeEquipCrit{ 0 };			//强化暴击
	bool m_bExLottery{ 0 };					//宝箱额外抽取
	bool m_bQuickTrainPet{ 0 };				//宠物一键金币培养
	bool m_bOpenTrainExtreme{ 0 };			//至尊培养
	bool m_bRelievedOfflinebattleCD{ 0 };	//离线竞技场免CD
	bool m_bLowerTechnologyPrice{ 0 };		//科技升级折扣15%
	bool m_bQuickChange{ 0 };				//一键金币兑换10次
	bool m_bGoldReward{ 0 };				//悬赏任务只出金色
	bool m_bAutoMeltingRune{ 0 };			//金币自动熔炼
	int	 buy_exercise_platform_times_{ 0 };	//购买练功台次数
};
