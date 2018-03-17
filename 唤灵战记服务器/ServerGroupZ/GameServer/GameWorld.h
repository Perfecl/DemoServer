#pragma once
#include "WorldBossBattle.h"
class CGameWorld
{
public:
	static const int		kTimeLoop{ 60000 };								//ʱ��ѭ��
	static const size_t		kTradeCardsNum{ 9 };							//��������������
	static const size_t		kTradeRewardsNum{ 3 };							//����������

	static CGameWorld*		GetInstance();									//��ȡ��Ϸ����ʵ��
	static void				InitializeGameWorld(CGameServer& game_svr);		//��ʼ����Ϸ����
	static void				CloseGameWorld();								//�ر���Ϸ����

	static int				watch_pid_;										//���ӵ����ID

private:
	static CGameWorld* instance_;											//��Ϸ����ʵ��

public:
	~CGameWorld();

	void SendToAgentServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, networkz::ISession* session);		//������Ϣ�����������
	void SendToAgentServer(CMessage msg, int pid, networkz::ISession* session);																	//������Ϣ�����������
	void SendToBattleServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid);									//������Ϣ��ս��������
	void SendToBattleServer(CMessage msg, int pid);																								//������Ϣ��ս��������
	void SendToAllPlayers(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid = 0);						//���͸��������

	bool HasBattleServerConnect();

	int											sid();																			//��ȡSID
	void										OnClock(int clock_id, const tm& now_datetime);									//ʱ��
	void										ProcessMessage(const CMessage& msg);											//������Ϣ

	void										Annunciate(const std::string& name, int arg0, int arg1, AnnunciateType type);	//����ͨ��

	Concurrency::task<void>&					GetTask(int pid);																//��ȡ����
	template<typename FunctionZ> void			CommitTask(int pid, FunctionZ fun)
	{
		Concurrency::task<void>& task = GetTask(pid);
		task = task.then(fun);
	}

	SPPlayer									FindPlayer(int pid);															//�������
	void										ErasePlayer(int pid);															//�������
	size_t										PlayerNumber(){ LOCK_BLOCK(players_lock_); return  online_players_.size(); }	//�������
	SPIPlayerData								GetPlayerData(int pid);															//��ȡ������ݽӿ�
	void										OfflineBySession(networkz::ISession* session);									//���Ự����
	bool										UpdateMaxLevel(int level);														//���·�������ߵȼ�
	int											FindPIDByName(const char* name);												//�����ֲ���pid

	CTown*										FindTown(int town_id);															//Ѱ�ҳ���
	inline size_t								TownNum(){ return towns_.size(); }												//��������

	int											CreateGuild(const std::string &name, int pid);									//��������
	int											GetGuildRanking(int guild_id);													//��ȡ��������
	std::vector<CGuild*>						FindGuild(const std::string& guild_name);										//���ҹ���
	CGuild*										FindGuild(int guild_id);														//���ҹ���

#pragma region ����
	SPRoomZ										CreateNewRoom(int pid, BattleMode mode, int map_id, const char* name, const char* password);	//����һ���·���
	void										DeleteRoom(int room_id);														//ɾ������
	SPRoomZ										FindRoom(int room_id);															//���ҷ���
	void										SetRoomListProtocol(pto_ROOM_S2C_RES_RoomList* pto, int map_id, int mode);		//���÷����б�Э��
#pragma endregion

#pragma region ��
	inline int									GetTradeCard(size_t index){ return trade_cards_[index]; }						//������
	inline __int64								GetTradeReward(size_t index){ return trade_rewards_[index]; }					//������
#pragma endregion

#pragma region ������
	inline int									GetSpeedStageRankingsNum(){ LOCK_BLOCK(speed_stage_lock_); return week_speed_ranking_.size(); }	//��ȡ��������������
	int											GetSpeedStageRank(int pid);														//��ȡ������������
	const SpeedRankInfo*						GetSpeedStageRankingDataByPID(int nPID);										//��þ��ٹؿ����������Ϣ
	const SpeedRankInfo*						GetSpeedStageRankingDataByRank(int nRank);										//��þ��ٹؿ����������Ϣ
	int											ChangeSpeedStageRank(int pid);													//�ı���Ҿ���������
	inline int									speed_stage_id() const{ return speed_stage_id_; }
	inline int									speed_stage_max_level() const { return speed_stage_max_level_; }
#pragma endregion

#pragma region ����̨
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
	void										SendExercisePlatform(const std::string& content, unsigned char protocol_type, int protocol_id);			//���͸��������
#pragma endregion

	ResultSetPtr								MySQLQuery(const std::string& sql);												//���ݿ��ѯ����
	__int64										MySQLUpdate(const std::string& sql);											//���ݿ���·���
	bool										MySQLExecute(const std::string& sql);											//���ݿ�ִ�з���

	void										GiveNewMail(SPMail mail, int pid);												//�����µ��ʼ�

	int											StartBattle(int map_id, BattleMode mode, std::vector<int> atkmasters, std::vector<int> defmasters, int stage_id = 0, int stage_diff = 0, int box_num = 0);	//��ʼս��

	void										ResetDay();
private:
	bool													is_open_{ false };						//�Ƿ�����Ϸ����
	CGameServer&											game_server_;							//��Ϸ������

	std::unordered_map<int, SPPlayer>						online_players_;						//�����������(PID,���ָ��)
	CCriticalSectionZ										players_lock_;							//����߳���
	std::unordered_map<int, Concurrency::task<void>>		player_saving_tasks_;					//��ұ�������(PID,����)
	CCriticalSectionZ										saving_lock_;							//�����߳���
	std::unordered_map<int, SPPlayerCache>					playerdata_cache_;						//������ݻ���
	CCriticalSectionZ										player_cache_lock_;						//������ݻ����߳���

	std::unordered_map<int, CGuild*>						guilds_;								//���еĹ���
	std::vector<int>										guild_ranking_;							//��������
	CCriticalSectionZ										guilds_lock_;							//�����߳���

	std::map<int, SPRoomZ>									rooms_;									//���еķ���
	int														room_id_allocator_{ 0 };				//����ID������
	CCriticalSectionZ										rooms_lock_;							//�����߳���

	std::unordered_map<int, CTown*>							towns_;									//���еĳ���

	std::array<int, kTradeCardsNum>							trade_cards_;							//Ŀ�꿨��
	std::array<__int64, kTradeRewardsNum>					trade_rewards_;							//���齱��������

	int														speed_stage_id_{ 0 };					//�������ؿ�
	int														speed_stage_max_level_{ 1 };			//�����������ߵȼ�
	std::list<SpeedRankInfo>								week_speed_ranking_;					//���������а�
	CCriticalSectionZ										speed_stage_lock_;						//�������߳���

	std::map<int, SPExercisePlatform>						exercise_platforms_;					//����̨ (̨��ID)
	std::map<int, SPExpPill>								exp_pills_;								//���鵤 (̨��ID��
	std::map<int, SPOfflineExerciseExp>						offline_exp_bank_;						//���߾���ֵ (PID)
	std::map<int, SPDownFromPlatform>						down_msg_;								//������Ϣ (PID)
	CCriticalSectionZ										exercise_lock_;							//����̨�߳���

	int														max_level_in_server_{ 1 };				//�����������ߵȼ�

	time_t													last_close_time_{ 0 };					//�ϴιط�ʱ��

	std::bitset<32>											event_hour_;							//ʱ��ÿСʱ��־

	std::vector<std::string>								illlegal_words_;						//���δ�

	CGameWorld(CGameServer& game_svr);

	void __LoadFromDatabase();																		//�����ݿ��ж�ȡ����
	void __SaveToDatabase();																		//�������ݿ�

	void __InitializeTowns();																		//��ʼ������
	void __GenerateTradeData();																		//����������
	void __GenerateSpeedStageData();																//���ɾ���������

	void __EnterGame(const CMessage& msg);															//������Ϸ
	void __LeaveGame(const CMessage& msg);															//�뿪��Ϸ

	void __AddPlayer(int pid, networkz::ISession* session);											//���һ�����
	void __DeletePlayer(int pid);																	//ɾ��һ�����

	void __CancelMatch(const CMessage &msg);														//ȡ��ƥ��
	void __OnCreateBattle(const CMessage& msg);														//��ս��������ʱ
	void __OnDeleteBattle(const CMessage& msg);
	void __GameOver(const CMessage& msg);

	void __ExercisePlatformLoop();																	//����̨
	void __CreateExercise();																		//��������̨�����ݿ�

#pragma region ����ģ��
public:
	std::list<int>								offline_battle_ranking_;								//������������
	std::multimap<int, OffLineBattleData*>		offline_datas_;											//���߾���������
	std::map<int, PlayerEscort*>				offline_escorts_;										//Ѻ��
	std::map<int, pto_OFFLINEBATTLE_Struct_SaveOfflineBattle*> offLine_battle_save_data;				//���߾�����¼��ID,¼��
	int											offline_record_id_{ 0 };								//����¼��ID
	SWorldBoss									world_boss_;											//����BOSS
	std::list<SWorldBossRank*>					world_boss_rankings_;									//����BOSS�˺�����
	bool										world_boss_start_;										//����BOSS��ʼ
	CCriticalSectionZ							offline_battle_lock_;									//�����߳���

	void					SendEscortPlayer(const std::string& content, unsigned char protocol_type, int protocol_id);				//���͸�Ѻ�����

	int						AddOfflineBattleRanking(int pid);																		//�������ս���������
	inline void				CleatOfflineRanking(){ LOCK_BLOCK(offline_battle_lock_); offline_battle_ranking_.clear(); }				//�������ս������
	int						GetPIDByRank(int rank);																					//��������ȡ���
	int						GetOfflinePlayerRank(int pid);																			//����Ƿ������а�
	void					AddOffLineBattleSaveData(pto_OFFLINEBATTLE_Struct_SaveOfflineBattle* save_data);						//�������ս��¼��
	bool					GetPlayersRank(int& nAtkRank, int& nDefRank, CPlayer* pAtkPlayer, int nDefPID);							//����������
	void					ResetOfflineRank(int& nAtkRank, int& nDefRank, CPlayer* pAtkPlayer, int nDefUId, bool bResourcesFound);	//������������
	void					SavePlayerOfflineBattleData(bool bChallenge, int id, time_t nTime, bool bWin, int nPlayerPID, int pid);	//���������������
	void					UpdatePlayerOfflineBattleData(CPlayer* player);															//�������
	void					GetOfflineList(CPlayer* player);																		//��������б�
	const pto_OFFLINEBATTLE_Struct_SaveOfflineBattle* GetOfflineBattleSavedata(int nID);											//�������ս��¼��
	void					StartWorldBoss();																						//��ʼ����BOSS
	void					EndWorldBoss();																							//��������BOSS
	SWorldBoss&				GetWorldBoss(){ return world_boss_; }																	//�������BOSS
	SWorldBossRank*			FindWorldBossRanking(int pid);																			//Ѱ������BOSS����
	void					ChangeWorldBossRank(int pid, int damage);																//�ı�����BOSS����
	void					UpdateWorldBoss(CPlayer* player);																		//��������BOSS
	const SWorldBossRank*	GetWorldRankingDataByRank(int rank);																	//�������BOSS������Ϣ
	PlayerEscort*			FindEscort(int nPID);																					//Ѱ��Ѻ��
	void					InserEscort(PlayerEscort* pescort);																		//����Ѻ��
	void					DeleteEscort(int nPID);																					//ɾ��Ѻ��
	void					UpdateEscort(pto_OFFLINEBATTLE_S2C_RES_UpdateEscort* pto);												//����Ѻ��
	void					SendOfflineBattleReward();																				//��������ս������

	inline int				offline_record_id() const { return offline_record_id_; }
	inline void				ChangeOfflineRecordID(int num){ offline_record_id_ += num; }
private:
	void					__EraseOfflineRanking(int nRank);																		//ɾ������
	void					__InsertOfflineRanking(int nRank, int nPID);															//��������
	void					__ProduceWorldBoss();																					//��������BOSS
	void					__SendOfflineBattleRankRewardMail(int pid, int rank, __int64 reputation, __int64 silver);				//��������ս�������ʼ�
	void					__FinishEscort();																						//Ѻ�����
	void					__CloseFinishEscort();																					//�ط�Ѻ�����
#pragma endregion

#pragma region ����ģ��
public:
	void	__WindUpTrade();																			//������
	void	__WindUpArenaReward();																		//���㾺���������ʼ�
	void	__WindUpSpeedStage();																		//���㾺����
	void	__RoutineReset();																			//��������

	void	__SendSpeedStageReward();																	//���;���������
	__int64	__GetBaseReward(LotteryType type);															//��û�������

	void	__ResetSpeedStageData();																	//���þ�����
	void	__SendSpeedStageRankRewardMail(int pid, int rank, __int64 silver, __int64 honour);			//���;���������

	void	__SendFairReward(int level, int point, int type, int pid);									//���Ź�ƽ����
	void	__SendWarReward(int level, int point, int pid);												//�������Խ���
#pragma endregion
};

inline CGameWorld*		GAME_WORLD(){ return CGameWorld::GetInstance(); }
inline SPPlayer			FIND_PLAYER(int pid){ return GAME_WORLD()->FindPlayer(pid); }
inline SPIPlayerData	PLAYER_DATA(int pid){ return GAME_WORLD()->GetPlayerData(pid); }
inline ResultSetPtr		MYSQL_QUERY(const std::string& sql){ return std::move(GAME_WORLD()->MySQLQuery(sql)); }
inline __int64			MYSQL_UPDATE(const std::string& sql){ return GAME_WORLD()->MySQLUpdate(sql); }
inline bool				MYSQL_EXECUTE(const std::string& sql){ return GAME_WORLD()->MySQLExecute(sql); }