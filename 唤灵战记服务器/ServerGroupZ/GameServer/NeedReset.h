#pragma once

struct TRewardMission
{
	int					rmid{ 0 };							//悬赏任务ID
	int					rank{ 0 };							//星级
	RewardMissionType	type{ RMT_Null };					//悬赏任务类型
	int					target_id{ 0 };						//任务ID
	int					target_num{ 0 };					//任务数量
	RewardMissionState	state{ RMS_New };					//任务状态
};

class CNeedReset
{
public:
	static const int kMaxTradeCardsNum{ 3 };									//抽牌数量
	static const int kMaxTradeHandsNum{ 5 };									//手牌数量

	static const int kRewardMissionNum{ 5 };									//悬赏任务数量

public:
	CNeedReset(CPlayer& player);
	~CNeedReset();
	void		SendOnlineBoxInfo();								//发送在线宝箱消息

	void		ProcessMessage(const CMessage& msg);

	void		SendInitialMessage();

	void		SaveToDatabase();

	void		WeekResetTrade();

	void		RoutineReset();
	
	void		RefreshRewardMission();																		//刷新悬赏任务  
	void		AutoRefreshRewardMission();																	//自动刷新悬赏任务
	void		ImplementRewardMission(RewardMissionType type, int target_id, int num);						//完成悬赏任务判断

	void		ChangeInternalExp(int nNum);
	inline int	castle_upgrade(CastleUpgrade type) const { return castle_upgrade_[type]; }

	inline int	today_buy_stamina_times() const { return today_buy_stamina_times_; }

	bool		IsMultiStageClear(int stage_level);

	void		UpdateMultiStage();
	void		UpdateEliteStage();
	inline bool	IsEliteAlreadyFighted(int elite_stage) const { return elite_already_stages_.find(elite_stage) != elite_already_stages_.cend(); }
	bool		AddEliteAlready(int lv){ return elite_already_stages_.insert(lv).second; }
	bool		AddMultiAlready(int lv){ return  multi_already_stages_.insert(lv).second; }

	inline bool is_got_battle_reward() const { return is_got_battle_reward_; }
	inline void is_got_battle_reward(bool flag){ is_got_battle_reward_ = flag; }

	inline void	ResetBattleWeekTimes(){ week_war_times_ = week_1v1_times_ = week_3v3_times_ = 0; }
	inline int 	week_war_times()const{ return week_war_times_; }
	inline int	week_1v1_times()const{ return week_1v1_times_; }
	inline int	week_3v3_times()const{ return week_3v3_times_; }

	inline void add_week_war_times(){ week_war_times_++; }
	inline void add_week_1v1_times(){ week_1v1_times_++; }
	inline void add_week_3v3_times(){ week_3v3_times_++; }

	inline int 	snake_award()const{ return snake_award_; }
	inline int	puzzle_award()const{ return puzzle_award_; }

	inline int  speed_times() const{ return speed_times_; }

	inline void ChangeSpeedTimes(int num){ speed_times_ += num; }

	inline int  speed_today_best_time() const { return speed_today_best_time_; }
	inline void speed_today_best_time(int time) { speed_today_best_time_ = time; }

	inline int	escort_times()const { return escort_times_; }
	inline int	rob_times() const { return rob_times_; }
	inline int	surplus_times() const { return surplus_times_; }
	inline int	protect_times()const { return protect_times_; }
	inline int	offline_buy_times() const { return offline_buy_times_; }
	inline bool	today_attendance()const { return today_attendance_; }
	inline bool	vip_every_day() const { return vip_every_day_; }
	inline int today_guild_contribute() const { return today_guild_contribute_; }

	inline void	change_escort_times(int num){ escort_times_ += num; }
	inline void	change_rob_times(int num){ rob_times_ += num; }
	inline void	change_surplus_times(int num){ surplus_times_ += num; }
	inline void	change_protect_times(int num){ protect_times_ += num; }
	inline void	change_offline_buy_times(int num){ offline_buy_times_ += num; }
	inline void change_today_guild_contribute(int num){ today_guild_contribute_ += num; }

	inline void	today_attendance(bool flag){ today_attendance_ = flag; }

private:
	CPlayer&			player_;
	time_t				online_box_start_time_{ time(0) };			//在线宝箱起始时间点
	int					online_box_award_{ 0 };						//在线宝箱领取进度

	int					escort_times_{ 0 };							//剩余押镖次数
	int					rob_times_{ 0 };							//剩余抢劫次数
	int					surplus_times_{ 0 };						//剩余挑战次数
	int					protect_times_{ 0 };						//已经保护次数
	int					offline_buy_times_{ 0 };					//离线战斗购买次数

	int					snake_award_{ 0 };							//贪吃蛇奖励
	int					puzzle_award_{ 0 };							//拼图奖励

	int					today_buy_stamina_times_{ 0 };				//今日购买体力次数

	int 				elite_buy_times_{ 0 };						//精英关卡购买次数
	std::set<int>		elite_already_stages_;						//今天精英关卡已经打过的

	std::set<int>		multi_already_stages_;						//多人关卡已经打过
	std::map<int, int>	multi_buy_stages_;							//多人关卡已购买

	bool				is_got_battle_reward_{ false };				//每天跨服首胜

	int					week_war_times_{ 0 };						//周争霸战斗次数
	int					week_1v1_times_{ 0 };						//周1v1战斗次数
	int					week_3v3_times_{ 0 };						//周3v3战斗次数

	int					speed_times_{ 0 };							//竞速赛每天剩余次数
	int					speed_today_best_time_{ 0 };				//竞速赛本日最好成绩

	bool				today_attendance_{ false };					//今日是否签到过

	bool				vip_every_day_{ false };					//VIP每日小礼包

	int					today_guild_contribute_{ 0 };				//每日捐献

	void		__LoadFromDatabase();								//从数据库中读取
	void		__SaveRewardMission();								//保存悬赏任务
	void		__SaveInternal();									//保存内政
	void		__GetOnlineBox();									//获取在线宝箱
	void		__BuyStamina();										//购买体力

	void		__BuyStage(const CMessage& msg);					//购买关卡次数
	void		__BuyEliteStage();									//购买精英关卡次数	
	void		__BuyMultiStage(int stage_id);						//购买多人关卡次数
	void		__BuySpeedStageTimes();								//购买竞速关卡次数

	void		__MiniGameReward(const CMessage& msg);				//小游戏奖励

	void		__GetVIPEveryDay();									//获取VIP每日礼包

#pragma region 祈福
	int	 today_draw_times_{ 0 };												//本日的剩余抽牌次数

	std::set<int>									trade_cards_;				//未抽到的卡牌
	std::deque<int>									trade_hands_;				//抽到手的手牌
	std::bitset<3>									trade_is_got_reward_;		//这周得到过的奖励
	CCriticalSectionZ								trade_lock_;

	void __SendTradeInfo();														//发送祈福信息
	void __TradeDraw();															//祈福抽牌
	void __TradeRefreshCards();													//祈福刷新抽到的卡牌
	void __TradeGetOneCard(const CMessage& msg);								//祈福从抽出的卡牌中获得一张卡
	void __TradeRandomCards();													//祈福随机生成卡牌
	void __TradeGetReward(pto_PLAYER_S2C_RES_GetPrayCard* pto, int level);		//祈福获得奖励
#pragma endregion

#pragma region 悬赏任务
	time_t													mission_reward_refresh_time_{ 0 };					//悬赏任务最后刷新时间
	int														mission_today_reward_complete_times_{ 0 };			//悬赏任务已完成次数
	int														mission_today_reward_buy_times_{ 0 };				//悬赏任务购买次数
	std::array<TRewardMission, kRewardMissionNum>			reward_mission_;									//悬赏任务

	void __LoadRewardMission();
	void __SendRewardMissionInfo();
	void __AcceptRewardMission(const CMessage& msg);							//接受悬赏任务				
	void __CommitRewardMission(const CMessage& msg);							//完成悬赏任务
	void __BuyRewardMissionTimes();												//购买悬赏次数
	void __AbandonRewardMission(const CMessage& msg);							//放弃悬赏任务
	void __BuyRefreshRewardMission(const CMessage& msg);						//刷新悬赏任务
	

	int	 __GetRewardMissionTargetNeedNum();										//悬赏任务目标数量
	void __GetRewardMissionResource(const TRewardMission* reward_mission);		//获得悬赏任务奖励资源
	int	 __ProduceRewardMissionStageLevel();									//生成悬赏任务关卡ID
#pragma endregion

#pragma region 内政
	int												levy_times_{ 0 };			//剩余征收次数
	int												gold_point_{ 0 };			//点金使用次数

	std::array<int, kCastleUpgradeConut> castle_upgrade_;						//主堡升级(物攻，魔攻，血量)
	std::array<InternalCell, kInternalBuildingConut> internal_cells;			//三个内政大厅

	void	__LoadInternal();
	void	__SendInternal();
	void	__UseInternal(const CMessage& msg);																		//使用内政
	void	__InternalLvUp(const CMessage& msg);																	//内政升级
	void	__ClearInternalCD(const CMessage& msg);																	//清空内政CD
	void	__UseLevy(const CMessage& msg);																			//使用征收
	void    __UpgradeCastle(const CMessage& msg);																	//升级城堡

	void    __UseInternalProduce(CHeroCard* hero, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto);	//使用内政生产
	void    __UseInternalTrain(CHeroCard* hero, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto);		//使用内政训练
	void    __UseInternalSearch(CHeroCard* hero, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto);		//使用内政寻访
	int     __GetProduceResource(CHeroCard* hero);																	//生产资源
	int     __GetTrainResource(CHeroCard* hero);																	//训练资源
	int     __GetSearchResource(CHeroCard* hero);																	//寻访资源
#pragma endregion
};

