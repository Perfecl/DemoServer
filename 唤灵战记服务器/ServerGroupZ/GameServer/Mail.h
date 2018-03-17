#pragma once

enum MailModelID
{
	MMI_FairArena = 1,		//1	跨服竞技赛奖励发放
	MMI_Escort,				//2	商队战奖励发放
	MMI_EscortProtect,		//3	商队战保护奖励发放
	MMI_SpeedStage,			//4	竞速赛奖励发放
	MMI_TerritoryBattle,	//5	争霸赛奖励发放
	MMI_OfflinBattle,		//6	天梯通关奖励发放
	MMI_GuildBattle,		//7	工会战奖励发放
	MMI_BossBattle,			//8	世界boss奖励发放
	MMI_TownStageRank,		//9	大关首破前十奖励
	MMI_DayWinAward,		//10公平竞技首胜
	MMI_Recharge,			//11充值奖励
};

class CMail
{
	friend class CKnapsack;
public:
	static void InsertToDatabase(SPMail mail, int pid);

public:
	CMail();
	~CMail();

	void		GetReward(SPPlayer player);												//获取奖励
	void		SetProtocol(dat_PLAYER_STRUCT_Mail *pto);								//设置邮件协议

	void		SetModelID(int nModelID){ model_id_ = nModelID; }

	void		GetRward(SPPlayer SPPlayer);

	void		SetIsRead(bool bIsRead){ is_read_ = bIsRead; }
	bool		IsRead(){ return is_read_; }

	void		SetRewardHeroID(int nHeroID){ reward_hero_id_ = nHeroID; }
	void		SetRewardSoldierID(int nSoldierID){ reward_soldier_id_ = nSoldierID; }
	void		AddRewardItem(int nID, __int64 nNum){ reward_items_.push_back(std::make_pair(nID, nNum)); }
	void		AddRewardEquip(int nEquipID){ reward_equips_.push_back(nEquipID); }
	void		SetRewardTitle(int title){ reward_title_ = title; }

	void		SetName(std::string strName){ name_ = strName; }
	const char* GetName() const { return name_.c_str(); }

	void		SetParam0(int nParam){ param0_ = nParam; }
	int			GetParam0() const { return param0_; }

	void		SetParam1(int nParam){ param1_ = nParam; }
	int			GetParam1()const{ return param1_; }

private:
	int											id_{ 0 };							//邮件ID
	int											model_id_{ 1 };						//内容模版ID
	int											param0_{ 0 };						//参数1
	int											param1_{ 0 };						//参数2
	std::string									name_;								//姓名
	time_t										recv_time_{ time(0) };				//接受时间
	bool										is_read_{ false };					//是否读取过
	int											reward_hero_id_{ 0 };				//奖励武将
	int											reward_soldier_id_{ 0 };			//奖励士兵
	std::vector<std::pair<int, __int64>>		reward_items_;						//奖励道具
	std::vector<int>							reward_equips_;						//奖励装备
	int											reward_title_{ 0 };					//奖励称号
};

