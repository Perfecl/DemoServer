#pragma once
#include "IPlayerData.h"

class CPlayer : public IPlayerData
{
public:
	enum PlayerAccess{ kAccessNormal, kAccessGM };								//权限

	static const int kMaxStamina{ 200 };										//最大体力值
	static const int kMaxVIPLevel{ 12 };										//最大vip等级
	static const int kMaxLevel{ 200 };											//玩家最高等级

	static const int kRecentsMaxNum{ 30 };										//最近联系人数量

public:
	CPlayer(int pid, networkz::ISession* session);
	~CPlayer();

	void			SendToAgent(const ::google::protobuf::Message& pto, int protocol_id, unsigned char protocol_type = MSG_S2C);		//发送到代理服务器

	void			ProcessMessage(const CMessage& msg);																				//处理消息

	void			SaveToDatabase(bool is_offline);																					//保存到数据库

	void			ChangeUser(networkz::ISession* session);																			//更改用户会话
	void			ReadyToOffline();																									//准备下线

	void			CheckOrder();																										//检查充值订单

	void			OnGameOver(pto_BATTLE_S2C_NTF_Game_Over& pto);																		//当战斗结束时
	bool			GetBoxState(int town_id, StageType type);																			//取关卡奖励状态

	std::vector<int>	GetFriendList();
	bool			IsFreind(int pid);

	void			ChangeVIPValue(int num);																							//改变VIP值
	void			ChangeSilver(__int64 num);																							//改变银币
	void			ChangeHonor(__int64 num);																							//改变荣誉
	void			ChangeGold(int num,int change_pos);																					//改变金币
	void			ChangeReputation(int num);																							//改变声望
	void			ChangeExp(__int64 num);																								//改变经验
	void			ChangeStamina(int num);																								//改变体力
	void			ItemChangeStamina(int num);
	bool			IsOpenGuide(int guild_id);																							//是否达到开启条件

	bool			GiveNewTitle(int title_id);																							//给与新的称号

	void			AddRecents(int contact_pid);																						//添加最近联系人

	void			ImplementMission(EMissionTargetType target_type, int target_id, int target_id_ex);									//完成任务
	void			ImplementRewardMission(RewardMissionType type, int target_id, int num);												//完成悬赏任务判断

	void			LevelUp();																											//升级
	void			SetLevel(int level);																								//设置等级
	void			SetMaxLevel(int max_level);																							//设置最大等级

	void			SetBattleMasterProto(pto_BATTLE_STRUCT_Master_Ex *pto, bool is_fair);												//设置Master协议
	void			StageBattleWin(const CStage* stage);																				//关卡胜利
	void			EliteStageBattleWin(int box_num, const CEliteStage* stage);															//精英关卡胜利
	void			MultiStageBattleWin(int box_num, const CEliteStage* stage);															//多人关卡胜利
	void			SpeedStageWin(int time);																							//竞速赛胜利

	void			OnRoomKick();

	virtual inline  int			pid() override{ return pid_; }
	virtual inline  int			sid() override{ return sid_; }
	virtual inline  const char* name() override{ return name_.c_str(); }
	virtual inline  bool		sex() override{ return sex_; }
	virtual inline  int			town_id() override{ return town_id_; }
	virtual inline  int			level() override{ return level_; }
	virtual inline  int			guild_id() override{ return guild_id_; }
	virtual inline  void		set_speed_best_rank(int rank){ if (rank < speed_stage_record_) speed_stage_record_ = rank; }
	virtual time_t				offline_time() override { return last_offline_time_; }

	virtual inline	int			exercise_time() override;
	virtual int					clothes() override;

	virtual std::string			guild_name() override;
	virtual int					guild_postion() override;

	virtual inline int			reputation() override{ return reputation_; }

	virtual inline int			offline_battle_win_streak() override{ return 0; }
	virtual int					bodygurand() override;

	virtual CHeroCard*			hero(int index) override;
	virtual std::vector<const CEquip*> hero_equip(int hero_id) override;
	virtual int					technology(int index) override;
	virtual inline int			speed_best_rank() override{ return speed_stage_record_; }
	virtual inline time_t		offline_last_time() override;
	virtual inline const BattleRecord&	battle_record(BattleRecordType type) override{ return battle_record_[type]; }
	virtual int					battle_week_times(BattleRecordType type) override;

	virtual inline int			using_title() override { return using_title_; }

	virtual inline  void		guild_id(int guild_id) override { guild_id_ = guild_id; };

	inline __int64				exp() const { return exp_; }
	inline __int64				silver() const { return silver_; }
	inline __int64				honor() const { return honor_; }
	inline int					stamina() const { return stamina_; }
	inline int					room_id() const{ return room_id_; }
	inline bool					is_in_battle() const { return PlayerState::BATTLE == state_; }
	inline int					vip_level() const { return vip_level_; }
	inline int					gold() const{ return gold_; }
	inline int					max_level() const { return max_level_; }
	inline PlayerState			state() const { return state_; }

	inline void					state(PlayerState p_state){ state_ = p_state; }
	inline CNeedReset*			need_reset(){ return need_reset_; }
	inline CKnapsack*			knapsack(){ return knapsack_; }
	inline CArmy*				army() { return army_; }
	inline COffline*			offline() { return offline_; }
	inline CMission*			mission() { return mission_; }
	inline CPlayerExercise*		exercise(){ return exercise_; }

	inline int					town_x() const { return town_x_; }
	inline int					town_y() const { return town_y_; }

	inline int					speed_stage_best_rank() const { return speed_stage_record_; }

	inline void					room_id(int room_id){ room_id_ = room_id; }
	inline void					SetTownID(int town_id){ town_id_ = town_id; }
	inline void					SetTownPosition(int x, int y){ town_x_ = x; town_y_ = y; }

	inline int 					point_war() const { return battle_record_[0].point; }
	inline int 					ponit1v1() const { return battle_record_[1].point; }
	inline int 					ponit3v3() const { return battle_record_[2].point; }
	inline bool					Is1v1FisrBattle() const { return 0 == (battle_record_[1].win + battle_record_[1].lose); }

	inline int					pet_id(){ return 0; }
	inline int					pet_level(){ return 0; }
	int							main_progress();

	inline networkz::ISession*	session() const { return agent_session_; }

private:
	const int						pid_;										//玩家ID
	int								sid_;										//ServerID
	std::string						name_;										//姓名
	bool							sex_{ false };								//性别
	int								level_{ 0 };								//等级
	__int64							exp_{ 0 };									//经验
	int								max_level_{ 20 };							//等级上限
	int								vip_level_{ 0 };							//VIP等级
	int								vip_value_{ 0 };							//VIP值
	__int64							silver_{ 0 };								//银币
	int								gold_{ 0 };									//金币
	__int64							honor_{ 0 };								//荣誉
	int								reputation_{ 0 };							//声望
	int								stamina_{ 0 };								//体力
	int								using_title_{ 0 };							//使用的称号
	std::set<int>					have_titles_;								//拥有的称号
	CCriticalSectionZ				title_lock_;								//称号线程锁
	PlayerAccess					access_{ PlayerAccess::kAccessNormal };		//权限
	time_t							last_offline_time_{ 0 };					//上次离线时间
	const time_t					login_time_;								//本次登陆时间

	CCriticalSectionZ				order_lock_;
	std::set <std::string>			orders_;									//订单

	int								town_id_{ 0 };								//城镇ID
	int								town_x_{ 0 };								//城镇x坐标
	int								town_y_{ 0 };								//城镇y坐标

	int								guild_id_{ 0 };								//公会ID							

	time_t							meditation_time_{ 0 };						//上次冥想时间

	int								online_day_award_{ 0 };						//在线天数奖励领取进度
	std::bitset<32>					vip_level_award_{ 0 };						//vip等级奖励领取进度 (0.首冲 1.vip1 2.vip2 ....)

	int								box_normal_progress_{ 0 };					//通关普通宝箱领取进度
	int								box_hard_progress_{ 0 };					//通关困难宝箱领取进度
	int								box_nightmare_progress_{ 0 };				//通关噩梦宝箱领取进度

	time_t							sweep_elite_stage_time_{ 0 };				//扫荡精英关卡时间

	time_t							last_world_chat_time_{ 0 };					//最后全屏聊天

	networkz::ISession*				agent_session_;								//Agent会话

	PlayerState						state_{ PlayerState::NORMAL };				//玩家状态

	std::array<BattleRecord, kBattleRecordCount> battle_record_;				//战斗记录

	int								speed_stage_record_;						//竞速赛最好记录(名次)

	std::string						proto_buffer_;								//协议发送缓冲
	CCriticalSectionZ				proto_buffer_lock_;							//协议发送锁

	void __LoadFromDatabase();													//从数据库读取数据
	void __LoadAwards();														//读取奖励表
	void __LoadRecord();														//读取战斗记录

	void __SaveAwards();														//保存奖励表
	void __SaveRecord();														//保存战斗记录
	void __SaveOrder();															//保存订单

	void __EnterTown(const CMessage& msg);										//进入城镇
	void __LeaveTown();															//离开城镇
	void __TownMove(const CMessage& msg);										//城镇移动

	void __GetMeditation();														//获取冥想

	void __InitMaxLevel();														//初始化最高等级

	void __GetAllTitle();														//获取所有称号
	void __UseTitle(const CMessage& msg);										//使用称号

	void __OnPlayerInfo();														//获取玩家初始信息
	void __SendOnlineBoxInfo();													//发送在线宝箱消息
	void __SendStageInfo();														//发送关卡消息

	void __OnPlayerBase(const CMessage& msg);									//获取玩家基础信息
	void __OnPlayerBaseEx(const CMessage& msg);									//获取玩家基础信息扩展
	void __OnPLayerDetailInfo(const CMessage& msg);								//获取玩家详细信息

	void __GetTownBox(const CMessage& msg);										//获取大关奖励

	void __SetBoxState(int town_id, StageType type, bool is_get);				//设置关卡奖励状态
	void __UpdateTownTop10(const CMessage& msg);								//更新城镇首破

	void __StartStage(const CMessage& msg);										//开始关卡
	void __StartEliteStage(const CMessage &msg);								//开始精英关卡
	void __StartSpeedStage();													//开始竞速关卡
	void __SweepStage(const CMessage& msg);										//扫荡关卡
	void __SweepEliteStage(const CMessage &msg);								//扫荡精英关卡
	void __StartSweepEliteStage(const CMessage &msg);							//开始扫荡精英关
	void __UpdateSpeedStage();													//更新竞速关卡
	void __CheckSpeedFormation(const CMessage &msg);							//检查竞速赛阵容
	void __QuitBattle();														//退出战斗
	void __GetDayWinAward();													//获得跨服战奖励
	void __GetVIPAward(const CMessage &msg);									//获得vip等级奖励
	void __GetOnlineAward(const CMessage &msg);									//获得在线天数奖励

	void __SpeedRankList(const CMessage &msg);									//竞速赛排行
	void __UpdateBattleRecord();												//更新战斗记录

	void __SetProtobuff(dat_PLAYER_STRUCT_PlayerData* pto);						//设置协议

	void __TestCommand(const CMessage& msg);									//测试命令
	void __GMCommand(std::string& message);										//GM命令

	void __GetGift(const CMessage& msg);										//领取礼包

	void __GMRecharge(int pid, int rmb);										//gm充钱
	void __GmGiveHero(int pid, int hero_id);
	void __GmImplementMission(int pid);

	void __KickPlayer(int pid);													//踢下线
	void __FrozenPlayer(int pid, int hour);										//冻结玩家

	void __GiveArenaSingleBonus(bool bIsWin,int nTime);							//跨服单场奖励

	void SendResetServer(int time);


#pragma region 组件
	CNeedReset*						need_reset_{ nullptr };						//需要重置
	CKnapsack*						knapsack_{ nullptr };						//背包
	CArmy*							army_{ nullptr };							//军队
	COffline*						offline_{ nullptr };						//离线
	CMission*						mission_{ nullptr };						//任务
	CPlayerExercise*				exercise_{ nullptr };						//练功台
#pragma endregion

#pragma region 社交
	std::set<int>					friends_list_;								//好友列表	
	std::set<int>					black_list_;								//黑名单
	std::list<int>					recents_list_;								//最近联系人
	CCriticalSectionZ				social_lock_;								//社交线程锁

	void __LoadFriends();														//读取好友表
	void __SendFriendInfo();													//发送好友信息
	void __AddFriendsOrBlack(const CMessage& msg);								//添加好友或黑名单
	void __DeleteFriendsOrBlack(const CMessage& msg);							//删除好友或黑名单
	void __SetContactsProtocol(int pid, pto_SOCIALStruct_FriendInfo* pto);		//设置联系人协议
	void __ChatMessage(const CMessage& msg);									//聊天消息

	void __CreateGuild(const CMessage& msg);									//创建公会
	void __GetGuildInfo();														//获取公会信息
	void __FindGuild(const CMessage& msg);										//寻找公会
	void __ApplyGuild(const CMessage& msg);										//加入公会
	void __AgreeApply(const CMessage& msg);										//同意加入公会
	void __ChangeGuildAccess(const CMessage& msg);								//改变公会职位
	void __QuitGuild();															//退出公会
	void __GuildKick(const CMessage& msg);										//公会踢人
	void __CessionGuide(const CMessage& msg);									//转让公会
	void __GetGuildName(const CMessage& msg);									//获取公会名
	void __GuildContribute();													//公会捐献
	void __GuildEditNotification(const CMessage& msg);							//改通告
#pragma endregion

#pragma region 房间
	int	 room_id_{ 0 };															//房间ID

	void __RoomCreate(const CMessage& msg);										//创建房间
	void __RoomList(const CMessage& msg);										//显示房间列表
	void __RoomJoin(const CMessage& msg);										//加入房间
	void __RoomQuit();															//退出房间
	void __RoomLock(const CMessage& msg);										//锁住房间
	void __RoomKick(const CMessage& msg);										//房间踢人
	void __RoomReady(const CMessage& msg);										//准备
	void __RoomChangePos(const CMessage& msg);									//交换位置
	void __RoomStart();															//房间开始战斗
	void __StartWar();															//开始争霸战
#pragma endregion
};
