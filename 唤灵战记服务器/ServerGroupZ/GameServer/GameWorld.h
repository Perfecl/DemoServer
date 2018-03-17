#pragma once
#include "WorldBossBattle.h"
class CGameWorld
{
public:
	static const int		kTimeLoop{ 60000 };								//时间循环
	static const size_t		kTradeCardsNum{ 9 };							//祈福奖励牌组数量
	static const size_t		kTradeRewardsNum{ 3 };							//祈福奖励数量

	static CGameWorld*		GetInstance();									//获取游戏世界实例
	static void				InitializeGameWorld(CGameServer& game_svr);		//初始化游戏世界
	static void				CloseGameWorld();								//关闭游戏世界

	static int				watch_pid_;										//监视的玩家ID

private:
	static CGameWorld* instance_;											//游戏世界实例

public:
	~CGameWorld();

	void SendToAgentServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, networkz::ISession* session);		//发送消息给代理服务器
	void SendToAgentServer(CMessage msg, int pid, networkz::ISession* session);																	//发送消息给代理服务器
	void SendToBattleServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid);									//发送消息给战斗服务器
	void SendToBattleServer(CMessage msg, int pid);																								//发送消息给战斗服务器
	void SendToAllPlayers(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid = 0);						//发送给所有玩家

	bool HasBattleServerConnect();

	int											sid();																			//获取SID
	void										OnClock(int clock_id, const tm& now_datetime);									//时钟
	void										ProcessMessage(const CMessage& msg);											//处理消息

	void										Annunciate(const std::string& name, int arg0, int arg1, AnnunciateType type);	//发出通告

	Concurrency::task<void>&					GetTask(int pid);																//获取任务
	template<typename FunctionZ> void			CommitTask(int pid, FunctionZ fun)
	{
		Concurrency::task<void>& task = GetTask(pid);
		task = task.then(fun);
	}

	SPPlayer									FindPlayer(int pid);															//查找玩家
	void										ErasePlayer(int pid);															//擦出玩家
	size_t										PlayerNumber(){ LOCK_BLOCK(players_lock_); return  online_players_.size(); }	//玩家数量
	SPIPlayerData								GetPlayerData(int pid);															//获取玩家数据接口
	void										OfflineBySession(networkz::ISession* session);									//按会话离线
	bool										UpdateMaxLevel(int level);														//更新服务器最高等级
	int											FindPIDByName(const char* name);												//用名字查找pid

	CTown*										FindTown(int town_id);															//寻找城镇
	inline size_t								TownNum(){ return towns_.size(); }												//城镇数量

	int											CreateGuild(const std::string &name, int pid);									//创建公会
	int											GetGuildRanking(int guild_id);													//获取公会排名
	std::vector<CGuild*>						FindGuild(const std::string& guild_name);										//查找公会
	CGuild*										FindGuild(int guild_id);														//查找公会

#pragma region 房间
	SPRoomZ										CreateNewRoom(int pid, BattleMode mode, int map_id, const char* name, const char* password);	//创建一个新房间
	void										DeleteRoom(int room_id);														//删除房间
	SPRoomZ										FindRoom(int room_id);															//查找房间
	void										SetRoomListProtocol(pto_ROOM_S2C_RES_RoomList* pto, int map_id, int mode);		//设置房间列表协议
#pragma endregion

#pragma region 祈福
	inline int									GetTradeCard(size_t index){ return trade_cards_[index]; }						//祈福卡牌
	inline __int64								GetTradeReward(size_t index){ return trade_rewards_[index]; }					//祈福奖励
#pragma endregion

#pragma region 竞速赛
	inline int									GetSpeedStageRankingsNum(){ LOCK_BLOCK(speed_stage_lock_); return week_speed_ranking_.size(); }	//获取竞速赛排名数量
	int											GetSpeedStageRank(int pid);														//获取竞速赛排名次
	const SpeedRankInfo*						GetSpeedStageRankingDataByPID(int nPID);										//获得竞速关卡玩家排名信息
	const SpeedRankInfo*						GetSpeedStageRankingDataByRank(int nRank);										//获得竞速关卡玩家排名信息
	int											ChangeSpeedStageRank(int pid);													//改变玩家竞速赛排名
	inline int									speed_stage_id() const{ return speed_stage_id_; }
	inline int									speed_stage_max_level() const { return speed_stage_max_level_; }
#pragma endregion

#pragma region 练功台
	SPExercisePlatform							GetExercisePlatform(int id);
	SPExpPill									GetExpPill(int id);
	int											ExercisePlatformsSize(){ return exercise_platforms_.size(); }
	void										AddOfflinExpBank(int pid, __int64 exp);
	SPOfflineExerciseExp						GetOfflineExp(int pid);
	void										DeleteOfflineExp(int pid);
	void										ResetDayOfflineExp();
	void										AddDownFromPlatform(int pid, int type, int enemy_id, time_t time);
	SPDownFromPlatform							GetDownMsg(int pid);
	void										DeleteDownMsg(int pid);
	void										SendExercisePlatform(const std::string& content, unsigned char protocol_type, int protocol_id);			//发送给练功玩家
#pragma endregion

	ResultSetPtr								MySQLQuery(const std::string& sql);												//数据库查询方法
	__int64										MySQLUpdate(const std::string& sql);											//数据库更新方法
	bool										MySQLExecute(const std::string& sql);											//数据库执行方法

	void										GiveNewMail(SPMail mail, int pid);												//给予新的邮件

	int											StartBattle(int map_id, BattleMode mode, std::vector<int> atkmasters, std::vector<int> defmasters, int stage_id = 0, int stage_diff = 0, int box_num = 0);	//开始战斗

	void										ResetDay();
private:
	bool													is_open_{ false };						//是否开启游戏世界
	CGameServer&											game_server_;							//游戏服务器

	std::unordered_map<int, SPPlayer>						online_players_;						//所有在线玩家(PID,玩家指针)
	CCriticalSectionZ										players_lock_;							//玩家线程锁
	std::unordered_map<int, Concurrency::task<void>>		player_saving_tasks_;					//玩家保存任务(PID,任务)
	CCriticalSectionZ										saving_lock_;							//保存线程锁
	std::unordered_map<int, SPPlayerCache>					playerdata_cache_;						//玩家数据缓存
	CCriticalSectionZ										player_cache_lock_;						//玩家数据缓存线程锁

	std::unordered_map<int, CGuild*>						guilds_;								//所有的公会
	std::vector<int>										guild_ranking_;							//公会排行
	CCriticalSectionZ										guilds_lock_;							//公会线程锁

	std::map<int, SPRoomZ>									rooms_;									//所有的房间
	int														room_id_allocator_{ 0 };				//房间ID分配器
	CCriticalSectionZ										rooms_lock_;							//房间线程锁

	std::unordered_map<int, CTown*>							towns_;									//所有的城镇

	std::array<int, kTradeCardsNum>							trade_cards_;							//目标卡组
	std::array<__int64, kTradeRewardsNum>					trade_rewards_;							//卡组奖励银币数

	int														speed_stage_id_{ 0 };					//竞速赛关卡
	int														speed_stage_max_level_{ 1 };			//竞速赛玩家最高等级
	std::list<SpeedRankInfo>								week_speed_ranking_;					//竞速赛排行榜
	CCriticalSectionZ										speed_stage_lock_;						//竞速赛线程锁

	std::map<int, SPExercisePlatform>						exercise_platforms_;					//练功台 (台子ID)
	std::map<int, SPExpPill>								exp_pills_;								//经验丹 (台子ID）
	std::map<int, SPOfflineExerciseExp>						offline_exp_bank_;						//离线经验值 (PID)
	std::map<int, SPDownFromPlatform>						down_msg_;								//离线消息 (PID)
	CCriticalSectionZ										exercise_lock_;							//练功台线程锁

	int														max_level_in_server_{ 1 };				//服务器玩家最高等级

	time_t													last_close_time_{ 0 };					//上次关服时间

	std::bitset<32>											event_hour_;							//时间每小时标志

	std::vector<std::string>								illlegal_words_;						//屏蔽词

	CGameWorld(CGameServer& game_svr);

	void __LoadFromDatabase();																		//从数据库中读取数据
	void __SaveToDatabase();																		//存入数据库

	void __InitializeTowns();																		//初始化城镇
	void __GenerateTradeData();																		//生成祈福数据
	void __GenerateSpeedStageData();																//生成竞速赛数据

	void __EnterGame(const CMessage& msg);															//进入游戏
	void __LeaveGame(const CMessage& msg);															//离开游戏

	void __AddPlayer(int pid, networkz::ISession* session);											//添加一个玩家
	void __DeletePlayer(int pid);																	//删除一个玩家

	void __CancelMatch(const CMessage &msg);														//取消匹配
	void __OnCreateBattle(const CMessage& msg);														//有战斗被创建时
	void __OnDeleteBattle(const CMessage& msg);
	void __GameOver(const CMessage& msg);

	void __ExercisePlatformLoop();																	//练功台
	void __CreateExercise();																		//创建练功台到数据库

#pragma region 离线模块
public:
	std::list<int>								offline_battle_ranking_;								//离线天梯排名
	std::multimap<int, OffLineBattleData*>		offline_datas_;											//离线竞技场数据
	std::map<int, PlayerEscort*>				offline_escorts_;										//押运
	std::map<int, pto_OFFLINEBATTLE_Struct_SaveOfflineBattle*> offLine_battle_save_data;				//离线竞技场录像（ID,录像）
	int											offline_record_id_{ 0 };								//离线录像ID
	SWorldBoss									world_boss_;											//世界BOSS
	std::list<SWorldBossRank*>					world_boss_rankings_;									//世界BOSS伤害排名
	bool										world_boss_start_;										//世界BOSS开始
	CCriticalSectionZ							offline_battle_lock_;									//离线线程锁

	void					SendEscortPlayer(const std::string& content, unsigned char protocol_type, int protocol_id);				//发送给押镖玩家

	int						AddOfflineBattleRanking(int pid);																		//添加离线战斗玩家排名
	inline void				CleatOfflineRanking(){ LOCK_BLOCK(offline_battle_lock_); offline_battle_ranking_.clear(); }				//清空离线战斗排名
	int						GetPIDByRank(int rank);																					//用排名获取玩家
	int						GetOfflinePlayerRank(int pid);																			//玩家是否在排行榜
	void					AddOffLineBattleSaveData(pto_OFFLINEBATTLE_Struct_SaveOfflineBattle* save_data);						//添加离线战斗录像
	bool					GetPlayersRank(int& nAtkRank, int& nDefRank, CPlayer* pAtkPlayer, int nDefPID);							//获得玩家排名
	void					ResetOfflineRank(int& nAtkRank, int& nDefRank, CPlayer* pAtkPlayer, int nDefUId, bool bResourcesFound);	//重置离线排名
	void					SavePlayerOfflineBattleData(bool bChallenge, int id, time_t nTime, bool bWin, int nPlayerPID, int pid);	//保存离线玩家数据
	void					UpdatePlayerOfflineBattleData(CPlayer* player);															//更新玩家
	void					GetOfflineList(CPlayer* player);																		//获得离线列表
	const pto_OFFLINEBATTLE_Struct_SaveOfflineBattle* GetOfflineBattleSavedata(int nID);											//获得离线战斗录像
	void					StartWorldBoss();																						//开始世界BOSS
	void					EndWorldBoss();																							//结束世界BOSS
	SWorldBoss&				GetWorldBoss(){ return world_boss_; }																	//获得世界BOSS
	SWorldBossRank*			FindWorldBossRanking(int pid);																			//寻找世界BOSS排名
	void					ChangeWorldBossRank(int pid, int damage);																//改变世界BOSS排名
	void					UpdateWorldBoss(CPlayer* player);																		//更新世界BOSS
	const SWorldBossRank*	GetWorldRankingDataByRank(int rank);																	//获得世界BOSS排名信息
	PlayerEscort*			FindEscort(int nPID);																					//寻找押运
	void					InserEscort(PlayerEscort* pescort);																		//插入押运
	void					DeleteEscort(int nPID);																					//删除押运
	void					UpdateEscort(pto_OFFLINEBATTLE_S2C_RES_UpdateEscort* pto);												//更新押运
	void					SendOfflineBattleReward();																				//发送离线战斗奖励

	inline int				offline_record_id() const { return offline_record_id_; }
	inline void				ChangeOfflineRecordID(int num){ offline_record_id_ += num; }
private:
	void					__EraseOfflineRanking(int nRank);																		//删除排名
	void					__InsertOfflineRanking(int nRank, int nPID);															//插入排名
	void					__ProduceWorldBoss();																					//生成世界BOSS
	void					__SendOfflineBattleRankRewardMail(int pid, int rank, __int64 reputation, __int64 silver);				//发送离线战斗奖励邮件
	void					__FinishEscort();																						//押镖完成
	void					__CloseFinishEscort();																					//关服押镖完成
#pragma endregion

#pragma region 结算模块
public:
	void	__WindUpTrade();																			//结算祈福
	void	__WindUpArenaReward();																		//结算竞技场奖励邮件
	void	__WindUpSpeedStage();																		//结算竞速赛
	void	__RoutineReset();																			//常规重置

	void	__SendSpeedStageReward();																	//发送竞速赛奖励
	__int64	__GetBaseReward(LotteryType type);															//获得基础奖励

	void	__ResetSpeedStageData();																	//重置竞速赛
	void	__SendSpeedStageRankRewardMail(int pid, int rank, __int64 silver, __int64 honour);			//发送竞速赛奖励

	void	__SendFairReward(int level, int point, int type, int pid);									//发放公平奖励
	void	__SendWarReward(int level, int point, int pid);												//发送争霸奖励
#pragma endregion
};

inline CGameWorld*		GAME_WORLD(){ return CGameWorld::GetInstance(); }
inline SPPlayer			FIND_PLAYER(int pid){ return GAME_WORLD()->FindPlayer(pid); }
inline SPIPlayerData	PLAYER_DATA(int pid){ return GAME_WORLD()->GetPlayerData(pid); }
inline ResultSetPtr		MYSQL_QUERY(const std::string& sql){ return std::move(GAME_WORLD()->MySQLQuery(sql)); }
inline __int64			MYSQL_UPDATE(const std::string& sql){ return GAME_WORLD()->MySQLUpdate(sql); }
inline bool				MYSQL_EXECUTE(const std::string& sql){ return GAME_WORLD()->MySQLExecute(sql); }