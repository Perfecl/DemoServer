#pragma once

class CGameServer;
class CPlayer;
class CTown;
class CGameWorld;
class CKnapsack;
class CArmy;
class COffline;
class CPlayerCache;
class CGuild;
class CMission;
class CNeedReset;
class CRoomZ;
class CHeroCard;
class CItem;
class CEquip;
class CMail;
class CPlayerExercise;
class CMail;
__interface IPlayerData;

typedef std::shared_ptr<CPlayer>		SPPlayer;
typedef std::shared_ptr<CPlayerCache>	SPPlayerCache;
typedef std::shared_ptr<IPlayerData>	SPIPlayerData;
typedef std::shared_ptr<CRoomZ>			SPRoomZ;
typedef std::shared_ptr<CMail>			SPMail;

const int kMaxHeroNum{ 3 };
const int kMaxSoldierMun{ 6 };

const int kStageStamina{ 5 };					//关卡体力

enum class PlayerState
{
	NORMAL,
	MATCH,
	BATTLE
};

enum TrainValueType
{
	kStr = 1,
	kCmd,
	kInt,
};

struct SoldierData
{
	int soul_exp_{ 0 };
	int soul_level_{ 0 };
	int train_level_{ 0 };
};

enum class AnnunciateType
{
	TownStageRank = 1,		//大关首破前十
	OfflineBattleRank,		//天梯前十
	StageSpeedRank,			//竞速前十
	TrainHeroOrange,		//武将培养到橙色(从紫色到橙色时)
	TrainHeroRed,			//武将培养到红色(从橙色到红色时)
	TrainHeroGold,			//武将培养到金色(从红色到金色时)
	FairArenaOpen,			//跨服PK 开赛
	FairArenaClose,			//跨服PK 关闭
	StageSpeedOpen,			//竞速赛 开启
	StageSpeedClose,		//竞速赛 关闭
	UpgradeEquipCrit,		//vip4武器强化爆击 
	ResetServer,			//服务器重启通知
	WinTrade,				//祈福中奖
	RecastEquip,			//重铸
	BOSSBattleOpen,			//BOSS战开启
	BOSSBattleClose,		//BOSS战关闭
	BOSSBattleFirst,		//BOSS战通告第一
	GuildBattleOpen,		//公会战开启
	GuildBattleClose,		//公会战关闭
	GuildBattleResult,		//公会战分别通告4座城的得主
	TerritoryBattle,		//领土战(本服个人手动PVP)
	GMMsg = 25,				//GM信息
	ExercisePlatform,		//夺取皇家练功台
};

enum LotteryType
{
	LT_Null,
	LT_Silver,
	LT_Gold,
	LT_Exp,
	LT_Honour,
	LT_Reputation,
	LT_Stamina,
	LT_Item,
};

struct TempLottery
{
	LotteryType m_enLotteryType{ LT_Null };
	int m_nID{ 0 };
	int m_nNum{ 0 };
	bool m_bChoose{ true };
};

struct SpeedRankInfo
{
	int										pid{ 0 };							//玩家ID
	int										time{ 0 };							//用时
	std::array<int, kMaxHeroNum>			heroes;
	std::array<int, kMaxSoldierMun>			soldiers;
};

struct OffLineBattleData
{
	int		m_nId{ 0 };
	bool	m_bChallenge{ true };  //0被挑战  1挑战
	time_t	m_nTime{ 0 };
	bool	m_bWin;
	int		m_nEnemyPId;
};

struct PlayerEscort
{
	int		m_nPId{ 0 };
	int		m_nHp{ 2 };
	time_t	m_nStartTime{ 0 };
	int		m_nY{ 0 };
	int		level{ 0 };
};

struct BattleRecord
{
	int point{ 0 }; 			//积分	
	int S{ 0 };					//S
	int A{ 0 };					//A
	int B{ 0 };					//B
	int C{ 0 };					//C
	int win{ 0 };				//胜利场次
	int lose{ 0 };				//失败场次
};

enum BattleRecordType
{
	kBattleRecordWar,
	kBattleRecord1v1,
	kBattleRecord3v3,
	kBattleRecordCount,
};

enum JewelryType
{
	kJewelryNull = -1,
	kJewelryClothes,
	kJewelryRing,
	kJewelryEarrings,
	kJewelryCount
};

enum class ExpenseType
{
	GetGold,
	Stamina,
	Trade,
	SpeedStage,
	EliteStage,
	MultiStage,
	Escort,
	WorldBoss,
	GoldPoint,
	Internal,
	Hero,
	OfflineBattle,
	Equip,
	RewardMission,
	BuyFashion,
	Mall,
	BuyContendTimes,
	ClearExercisePlatformCD,
	MiniGame,
	ClearWorldBossCD,
};

enum InternalBuilding				//内政建筑
{
	kCrystalCave,					//水晶矿场
	kRathaus,						//议会厅
	kMilitaryHall,					//军事厅
	kInternalBuildingConut,			//数量
};

enum CastleUpgrade
{
	kCastleUpgradeAtk,		//火炮物攻
	kCastleUpgradeMAtk,		//火炮魔攻
	kCastleUpgradeHP,		//主堡血量
	kCastleUpgradeConut,
};

struct InternalCell					//内政单元格
{
	int			times_{ 0 };		//使用次数
	time_t 		last_use_{ 0 };		//最后使用时间
	int 		hero_id_{ 0 };		//用英雄ID
	int 		level_{ 1 };		//等级
};

struct ExpPill
{
	int					id{ 0 };
	__int64				exp{ 0 };
	int					times{ 0 };
	std::vector<int>	taken_player;
};

struct OfflineExerciseExp
{
	__int64 exp_{ 0 };
	int		exercise_time_{ 0 };
};

struct DownFromPlatform
{
	int		type{ 0 };
	int		pid{ 0 };
	time_t	time{ 0 };
};

struct CExercisePlatform
{
	int		pid{ 0 };
	time_t	start_time{ 0 };

	int		id{ 0 };
	int		level{ 0 };
	float	exp_mul{ 0 };
};

typedef std::shared_ptr<CExercisePlatform>		SPExercisePlatform;
typedef std::shared_ptr<ExpPill>				SPExpPill;
typedef std::shared_ptr<OfflineExerciseExp>		SPOfflineExerciseExp;
typedef std::shared_ptr<DownFromPlatform>		SPDownFromPlatform;

#define ExercisePlatformProtectTime 600
#define ExercisePlatformMaxOccupyTime 21600
#define EverydayMaxExercisePlatformTimes 720