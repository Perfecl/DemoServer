#pragma once

struct TRewardMission
{
	int					rmid{ 0 };							//��������ID
	int					rank{ 0 };							//�Ǽ�
	RewardMissionType	type{ RMT_Null };					//������������
	int					target_id{ 0 };						//����ID
	int					target_num{ 0 };					//��������
	RewardMissionState	state{ RMS_New };					//����״̬
};

class CNeedReset
{
public:
	static const int kMaxTradeCardsNum{ 3 };									//��������
	static const int kMaxTradeHandsNum{ 5 };									//��������

	static const int kRewardMissionNum{ 5 };									//������������

public:
	CNeedReset(CPlayer& player);
	~CNeedReset();
	void		SendOnlineBoxInfo();								//�������߱�����Ϣ

	void		ProcessMessage(const CMessage& msg);

	void		SendInitialMessage();

	void		SaveToDatabase();

	void		WeekResetTrade();

	void		RoutineReset();
	
	void		RefreshRewardMission();																		//ˢ����������  
	void		AutoRefreshRewardMission();																	//�Զ�ˢ����������
	void		ImplementRewardMission(RewardMissionType type, int target_id, int num);						//������������ж�

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
	time_t				online_box_start_time_{ time(0) };			//���߱�����ʼʱ���
	int					online_box_award_{ 0 };						//���߱�����ȡ����

	int					escort_times_{ 0 };							//ʣ��Ѻ�ڴ���
	int					rob_times_{ 0 };							//ʣ�����ٴ���
	int					surplus_times_{ 0 };						//ʣ����ս����
	int					protect_times_{ 0 };						//�Ѿ���������
	int					offline_buy_times_{ 0 };					//����ս���������

	int					snake_award_{ 0 };							//̰���߽���
	int					puzzle_award_{ 0 };							//ƴͼ����

	int					today_buy_stamina_times_{ 0 };				//���չ�����������

	int 				elite_buy_times_{ 0 };						//��Ӣ�ؿ��������
	std::set<int>		elite_already_stages_;						//���쾫Ӣ�ؿ��Ѿ������

	std::set<int>		multi_already_stages_;						//���˹ؿ��Ѿ����
	std::map<int, int>	multi_buy_stages_;							//���˹ؿ��ѹ���

	bool				is_got_battle_reward_{ false };				//ÿ������ʤ

	int					week_war_times_{ 0 };						//������ս������
	int					week_1v1_times_{ 0 };						//��1v1ս������
	int					week_3v3_times_{ 0 };						//��3v3ս������

	int					speed_times_{ 0 };							//������ÿ��ʣ�����
	int					speed_today_best_time_{ 0 };				//������������óɼ�

	bool				today_attendance_{ false };					//�����Ƿ�ǩ����

	bool				vip_every_day_{ false };					//VIPÿ��С���

	int					today_guild_contribute_{ 0 };				//ÿ�վ���

	void		__LoadFromDatabase();								//�����ݿ��ж�ȡ
	void		__SaveRewardMission();								//������������
	void		__SaveInternal();									//��������
	void		__GetOnlineBox();									//��ȡ���߱���
	void		__BuyStamina();										//��������

	void		__BuyStage(const CMessage& msg);					//����ؿ�����
	void		__BuyEliteStage();									//����Ӣ�ؿ�����	
	void		__BuyMultiStage(int stage_id);						//������˹ؿ�����
	void		__BuySpeedStageTimes();								//�����ٹؿ�����

	void		__MiniGameReward(const CMessage& msg);				//С��Ϸ����

	void		__GetVIPEveryDay();									//��ȡVIPÿ�����

#pragma region ��
	int	 today_draw_times_{ 0 };												//���յ�ʣ����ƴ���

	std::set<int>									trade_cards_;				//δ�鵽�Ŀ���
	std::deque<int>									trade_hands_;				//�鵽�ֵ�����
	std::bitset<3>									trade_is_got_reward_;		//���ܵõ����Ľ���
	CCriticalSectionZ								trade_lock_;

	void __SendTradeInfo();														//��������Ϣ
	void __TradeDraw();															//������
	void __TradeRefreshCards();													//��ˢ�³鵽�Ŀ���
	void __TradeGetOneCard(const CMessage& msg);								//���ӳ���Ŀ����л��һ�ſ�
	void __TradeRandomCards();													//��������ɿ���
	void __TradeGetReward(pto_PLAYER_S2C_RES_GetPrayCard* pto, int level);		//����ý���
#pragma endregion

#pragma region ��������
	time_t													mission_reward_refresh_time_{ 0 };					//�����������ˢ��ʱ��
	int														mission_today_reward_complete_times_{ 0 };			//������������ɴ���
	int														mission_today_reward_buy_times_{ 0 };				//�������������
	std::array<TRewardMission, kRewardMissionNum>			reward_mission_;									//��������

	void __LoadRewardMission();
	void __SendRewardMissionInfo();
	void __AcceptRewardMission(const CMessage& msg);							//������������				
	void __CommitRewardMission(const CMessage& msg);							//�����������
	void __BuyRewardMissionTimes();												//�������ʹ���
	void __AbandonRewardMission(const CMessage& msg);							//������������
	void __BuyRefreshRewardMission(const CMessage& msg);						//ˢ����������
	

	int	 __GetRewardMissionTargetNeedNum();										//��������Ŀ������
	void __GetRewardMissionResource(const TRewardMission* reward_mission);		//���������������Դ
	int	 __ProduceRewardMissionStageLevel();									//������������ؿ�ID
#pragma endregion

#pragma region ����
	int												levy_times_{ 0 };			//ʣ�����մ���
	int												gold_point_{ 0 };			//���ʹ�ô���

	std::array<int, kCastleUpgradeConut> castle_upgrade_;						//��������(�﹥��ħ����Ѫ��)
	std::array<InternalCell, kInternalBuildingConut> internal_cells;			//������������

	void	__LoadInternal();
	void	__SendInternal();
	void	__UseInternal(const CMessage& msg);																		//ʹ������
	void	__InternalLvUp(const CMessage& msg);																	//��������
	void	__ClearInternalCD(const CMessage& msg);																	//�������CD
	void	__UseLevy(const CMessage& msg);																			//ʹ������
	void    __UpgradeCastle(const CMessage& msg);																	//�����Ǳ�

	void    __UseInternalProduce(CHeroCard* hero, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto);	//ʹ����������
	void    __UseInternalTrain(CHeroCard* hero, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto);		//ʹ������ѵ��
	void    __UseInternalSearch(CHeroCard* hero, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto);		//ʹ������Ѱ��
	int     __GetProduceResource(CHeroCard* hero);																	//������Դ
	int     __GetTrainResource(CHeroCard* hero);																	//ѵ����Դ
	int     __GetSearchResource(CHeroCard* hero);																	//Ѱ����Դ
#pragma endregion
};

