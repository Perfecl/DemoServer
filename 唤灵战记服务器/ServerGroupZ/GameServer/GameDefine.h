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

const int kStageStamina{ 5 };					//�ؿ�����

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
	TownStageRank = 1,		//�������ǰʮ
	OfflineBattleRank,		//����ǰʮ
	StageSpeedRank,			//����ǰʮ
	TrainHeroOrange,		//�佫��������ɫ(����ɫ����ɫʱ)
	TrainHeroRed,			//�佫��������ɫ(�ӳ�ɫ����ɫʱ)
	TrainHeroGold,			//�佫��������ɫ(�Ӻ�ɫ����ɫʱ)
	FairArenaOpen,			//���PK ����
	FairArenaClose,			//���PK �ر�
	StageSpeedOpen,			//������ ����
	StageSpeedClose,		//������ �ر�
	UpgradeEquipCrit,		//vip4����ǿ������ 
	ResetServer,			//����������֪ͨ
	WinTrade,				//���н�
	RecastEquip,			//����
	BOSSBattleOpen,			//BOSSս����
	BOSSBattleClose,		//BOSSս�ر�
	BOSSBattleFirst,		//BOSSսͨ���һ
	GuildBattleOpen,		//����ս����
	GuildBattleClose,		//����ս�ر�
	GuildBattleResult,		//����ս�ֱ�ͨ��4���ǵĵ���
	TerritoryBattle,		//����ս(���������ֶ�PVP)
	GMMsg = 25,				//GM��Ϣ
	ExercisePlatform,		//��ȡ�ʼ�����̨
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
	int										pid{ 0 };							//���ID
	int										time{ 0 };							//��ʱ
	std::array<int, kMaxHeroNum>			heroes;
	std::array<int, kMaxSoldierMun>			soldiers;
};

struct OffLineBattleData
{
	int		m_nId{ 0 };
	bool	m_bChallenge{ true };  //0����ս  1��ս
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
	int point{ 0 }; 			//����	
	int S{ 0 };					//S
	int A{ 0 };					//A
	int B{ 0 };					//B
	int C{ 0 };					//C
	int win{ 0 };				//ʤ������
	int lose{ 0 };				//ʧ�ܳ���
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

enum InternalBuilding				//��������
{
	kCrystalCave,					//ˮ����
	kRathaus,						//�����
	kMilitaryHall,					//������
	kInternalBuildingConut,			//����
};

enum CastleUpgrade
{
	kCastleUpgradeAtk,		//�����﹥
	kCastleUpgradeMAtk,		//����ħ��
	kCastleUpgradeHP,		//����Ѫ��
	kCastleUpgradeConut,
};

struct InternalCell					//������Ԫ��
{
	int			times_{ 0 };		//ʹ�ô���
	time_t 		last_use_{ 0 };		//���ʹ��ʱ��
	int 		hero_id_{ 0 };		//��Ӣ��ID
	int 		level_{ 1 };		//�ȼ�
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