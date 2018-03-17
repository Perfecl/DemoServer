#pragma once

enum EMissionTargetType
{
	MTT_TalkToNpc = 1,		//1.与NPC对话
	MTT_CompleteMap,		//2.完成关卡
	MTT_EquipLvUpTimes,		//3.强化装备次数
	MTT_UseHero,			//4.武将出阵
	MTT_UseEquip,			//5.穿戴装备
	MTT_EquipLvUpLv,		//6.强化装备等级
	MTT_UseSoldier,			//7.士兵出阵
	MTT_UnitTechnology,		//8.强化士兵
	MTT_TrainHero,			//9.武将培养
	MTT_Enchanting,			//10.装备精炼
	MTT_RewardMission,		//11.完成悬赏
	MTT_UseProduceCell,		//12.完成耕作
	MTT_UseBarracksCell,	//13.完成训练
	MTT_UseSearchCell,		//14.完成探访
	MTT_CreateOrJoinGuild,	//15.创建或加入公会
	MTT_GuildContribute,	//16.募捐银币
	MTT_RecruitHero,		//17.招募武将
	MTT_DIYLvUp,			//18.手动升级
	MTT_TechnologyLv,		//19.升级某项科技到XX级
	MTT_PetLevel,			//20.升级宠物到N级
	MTT_OfflineBattle,		//21.参加天梯竞技场N次
	MTT_EliteStage,			//22.完成精英关卡
	MTT_MultiStage,			//23.完成多人关卡
	MTT_SpeedStage,			//24.参加竞速赛
	MTT_FairArena,			//25.参加跨服竞技场
	MTT_Escort,				//26.商队战运送一次
	MTT_RobEscort,			//27.商队战拦截一次
	MTT_AllEquipLevel,		//28.强化所有装备到N级
	MTT_AllTechnologyLevel,	//29.强化所有科技到N级
	MTT_GetTownBox,			//30.领取关卡宝箱
	MTT_TrainHeroRank,		//31.指定英雄培养到阶级
	MTT_HeroLevel,			//32.武将升到NN级
	MTT_WashRune,			//33.符文洗炼
	MTT_HeroLevelUp,		//34.武将升级
	MTT_UseLevy,			//35,完成征收
	MTT_ExchangeFashion,	//36,穿时装
	MTT_Shopping,			//37,购物
	MTT_Trade,				//38,祈福
	MTT_War,				//39,争霸
	MTT_Security,			//40,内政治安
	MTT_AC,
};

class CMissionTP
{
	friend class CMission;
public:
	static void Load();
	static const CMissionTP* GetMission(int nID);
	static const CMissionTP* GetMissionByMainProgress(int nProgress);

private:
	static std::map<int, const CMissionTP*> m_mapMissions;

public:
	CMissionTP() = default;
	~CMissionTP() = default;

	int GetMissionID() const { return m_nID; }
	int GetNeedTargetNum() const { return m_nTargetNeedNum; }
	int GetTargetType()const { return m_nTargetType; }
	int GetTargetID() const { return m_nTargetID; }
	int GetTargetIDEx() const { return m_nTargetIDEx; }
	int GetNeedMainProgress() const { return m_nNeedMainProgress; }
	int GetType() const { return m_nType; }
	int GetNeedLevel() const { return m_nNeedLevel; }

	static void AvailableBranchList(std::vector<int>* branch, int player_level, int main_progress);

private:
	int m_nID{ 0 };							//任务ID
	int m_nType{ 0 };						//任务类型 1主线 2支线 3日常 4悬赏
	int m_nNeedMainProgress{ 0 };			//需求主线进度
	int m_nNeedLevel{ 0 };					//需求等级
	int m_nFollowMisssionID{ 0 };			//后续任务ID 

	int m_nAcceptNPC{ 0 };					//接任务NPC ID
	int m_nCommitNPC{ 0 };					//交任务NPC ID

	int m_nTargetType{ 0 };					//目的类型 1与NPC对话，2完成关卡，3完成副本，4完成多人副本，5进入竞技场，6赢得竞技场,7指引型任务
	int m_nTargetID{ 0 };					//目标ID（对话NPC ID,完成地图ID） 
	int m_nTargetIDEx{ 0 };					//目标ID附加值（关卡难度）            
	int m_nTargetNeedNum{ 0 };				//目标数量       

	long long m_nRewardExp{ 0 };			//奖励经验
	long long m_nRewardSilver{ 0 };			//奖励金钱
	int m_nRewardHonor{ 0 };				//奖励军功
	int m_nRewardReputation{ 0 };			//奖励声望
	int m_nRewardGold{ 0 };					//奖励点券
	int m_nRewardStamina{ 0 };				//奖励体力

	int m_nRewardItem1{ 0 };				//奖励道具1 ID
	int m_nRewardItem1Num{ 0 };				//奖励道具1 数量
	int m_nRewardItem2{ 0 };				//奖励道具1 ID
	int m_nRewardItem2Num{ 0 };				//奖励道具1 数量
	int m_nRewardItem3{ 0 };				//奖励道具1 ID
	int m_nRewardItem3Num{ 0 };				//奖励道具1 数量
	int m_nRewardItem4{ 0 };				//奖励道具1 ID
	int m_nRewardItem4Num{ 0 };				//奖励道具1 数量
	int m_nRewardItem5{ 0 };				//奖励道具1 ID
	int m_nRewardItem5Num{ 0 };				//奖励道具1 数量
	int m_nRewardItem6{ 0 };				//奖励道具1 ID
	int m_nRewardItem6Num{ 0 };				//奖励道具1 数量

	int m_nRewardEquip1{ 0 };
	int m_nRewardEquip2{ 0 };
	int m_nRewardEquip3{ 0 };
	int m_nRewardEquip4{ 0 };

	int m_nRewardHeroID{ 0 };				//奖励英雄ID
	int m_nRewardSoldier{ 0 };				//奖励士兵ID

	int m_nBeginStoryID{ 0 };				//关卡前剧情ID
	int m_nFinishStoryID{ 0 };				//关卡后剧情ID
	int m_nOpenFunction{ 0 };				//开启功能

	int m_nOfferSilver{ 0 };				//接任务提供银币
	int m_nOfferHonor{ 0 };
	int m_nOfferReputation{ 0 };
	int m_nOfferStamina{ 0 };
	int m_nOfferEquip1{ 0 };
	int m_nOfferEquip2{ 0 };
	int m_nOfferSoldier{ 0 };
	int m_nOfferHero{ 0 };
	int m_nOfferEx{ 0 };
};
