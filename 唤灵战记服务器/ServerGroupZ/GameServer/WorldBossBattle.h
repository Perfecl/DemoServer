#pragma once
#include "OffLineUnit.h"
#include "OffLineHero.h"
#include "OffLinePlayer.h"

struct SWorldBoss
{
	__int64 m_nMaxHP{ 0 };
	__int64 m_nHP{ 0 };
	int m_nStr{ 0 };
	int m_nInt{ 0 };
	int m_nCmd{ 0 };
};

struct SWorldBossRank
{
	int m_nPID{ 0 };
	int m_nDmg{ 0 };
	time_t m_nLastTime{ 0 }; //上次打世界BOSS时间
};

class CWorldBossBattle
{
public:
	CWorldBossBattle();
	~CWorldBossBattle();

	void WorldBossBattle();

	void Start(COffLinePlayer* pPlayer, SWorldBoss* pBoss, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto);
	void Loop(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto);
	int  Over(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto);

private:
	void InitUnit(COffLinePlayer* pPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto, Force enForce);
	void InitBoss(SWorldBoss* pBoss, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto);
	std::vector<COffLineUnit> m_vctOffLineUnit;
	std::vector<COffLineHero*> m_vctOffLineHero;

	__int64 m_nBossHP{ 0 };

	int		m_nRound{ 0 };
	int		m_nDamage{ 0 };

	bool	m_bIsOver{ false };
	bool	m_bWinForce{ false };
};

