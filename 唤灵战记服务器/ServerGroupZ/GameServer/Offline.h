#pragma once

enum OfflineBattleType
{
	OBT_Null,
	OBT_OfflineRank,	//天梯
	OBT_Escort,			//商队
	OBT_WorldBoss,		//世界BOSS
};

class COffLinePlayer;
class COffline
{
public:
	static void SetPlayerEscort(SPPlayer player, pto_OFFLINEBATTLE_S2C_RES_UpdateEscort* pto, PlayerEscort* escort);
	static void SetPlayerEscort(COffLinePlayer* offline_player, pto_OFFLINEBATTLE_S2C_RES_UpdateEscort* pto, PlayerEscort* escort);
	static int	GetEscortReputation(SPIPlayerData data, PlayerEscort* escort);
	static int	GetEscortReward(int player_level, int dartcar_level, PlayerEscort* escort);
	static int	GetEscortReputation(int player_level, int dartcar_level, PlayerEscort* escort);

public:
	COffline(CPlayer& player);
	~COffline();

	void			ProcessMessage(const CMessage& msg);
	void			SaveToDatabase();
	void			SendInitialMessage();

	void			GetEnemyData(pto_OFFLINEBATTLE_S2C_RES_UpdatePlayerOfflineBattleData* pto);

	inline int		offline_rank() const { return offline_rank_; }
	inline void		offline_rank(int rank){ offline_rank_ = rank; }

	inline int		win_streak() const { return win_streak_; }
	inline time_t	last_time() const { return last_time_; }
	inline int		bodyguard() const { return bodyguard_; }
	inline int		temp_dartcar_level() const { return temp_dartcar_level_; }

	inline bool		IsInEscrot() const { return is_in_escort_; }

	inline void		set_temp_dartcar_level(int level) { temp_dartcar_level_ = level; }
	inline void		set_bodyguard(int id){ bodyguard_ = id; }
	inline void		set_last_time_(time_t time){ last_time_ = time; }

	inline void		UpdateEscort(){ __UpdateEscort(); }
	static bool		ExercisePlatformBattle(CPlayer* player, int pid, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle);


private:
	CPlayer&	player_;

	time_t		last_time_{ 0 };				//最后挑战时间
	int			win_streak_{ 0 };				//连胜次数

	int			temp_dartcar_level_{ 0 };		//镖车等级
	int			protect_mode_{ 0 };				//保护模式
	int			bodyguard_{ 0 };				//保镖ID

	time_t		last_rob_time_{ 0 };			//最后打劫时间

	int			offline_rank_{ 0 };				//离线排名
	bool		is_in_offline_{ false };		//是否在离线战斗界面
	bool		is_in_escort_{ false };			//是否在押镖界面

	void __LoadFromDatabase();

	void __StartOfflineBattle(const CMessage& msg);
	void __SetPlayerIsInOffline(const CMessage& msg);
	void __ClearOfflineBattleCD();
	void __BuyOfflineBattleTimes();
	void __ReplayOfflineBattle(const CMessage& msg);
	void __BeginWorldBossBattle();
	void __GetEscortCar(const CMessage& msg);
	void __StartEscort();
	void __UpdateEscort();
	void __EnterEscort(const CMessage& msg);
	void __RobEscort(const CMessage& msg);
	void __SetAutoProtect(const CMessage& msg);
	void __InviteProtect(const CMessage& msg);
	void __ResAskProtect(const CMessage& msg);
	void __UpdateProtecterList();

	void __StartOfflineBattle(int pid);
	void __EnterOffLineBattle(CPlayer* pTargetPlayer, pto_OFFLINEBATTLE_S2C_RES_StartOfflineBattle* pto);
	void __EnterOffLineBattle(COffLinePlayer* pTargetPlayer, pto_OFFLINEBATTLE_S2C_RES_StartOfflineBattle* pto);
	void __EnterOffLineBattle(int pid, pto_OFFLINEBATTLE_S2C_RES_StartOfflineBattle* pto);

	void __SendBeAttackedMessage(bool& bAtkWin, int nNewRank, CPlayer* pAtkPlayer, SPPlayer pDefPlayer);
	void __GetEnemyPto(COffLinePlayer* pOffLinePlayer, int nRank, pto_OFFLINEBATTLE_Struct_EnemyData* pPto);
	void __GetEnemyPto(SPPlayer pPlayer, int nRank, pto_OFFLINEBATTLE_Struct_EnemyData* pPto);

	void __RandCar();
	void __SendNewEscort(PlayerEscort* player_escort);
	void __SendNewEscort(pto_OFFLINEBATTLE_S2C_NTF_PlayereEscort* pto);
	bool __EscortBattle(CPlayer* pTargetPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle);
	bool __EscortBattle(COffLinePlayer* pTargetPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle);
	bool __EscortBattle(int nPID, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle);
	void __SendEscortProtectRewardMail(int pid, int reward);
	void __SetAutoProtectMode(bool bAutoProtect, bool bAutoRefuse);
	bool __PlayerCanProtect(int pid);
	void __UpdateEscortWhenLogin();

	void __ClearWorldBossCD();
	void __UpdataEscortHP(int pid, int hp);

	void SendBeRobbed(CPlayer* robber, bool win);

	bool		__ExercisePlatformBattle(int pid, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle);

	inline bool __AutoProtect() const { if (1 == protect_mode_)return true; return false; }
	inline bool __AutoRefuse() const { if (2 == protect_mode_)return true; return false; }
	inline void __SetAutoProtect(bool bAuto){ if (bAuto) protect_mode_ = 1; }
	inline void __SetAutoRefuse(bool bAuto){ if (bAuto)protect_mode_ = 2; }
};
