#pragma once
#include "OffLineUnit.h"
#include "OffLineHero.h"

class COffLineBattle
{
public:
	COffLineBattle();
	~COffLineBattle();

public:
	int  GetBulidingHp(int nIndex){ return m_arrBulidingHP[nIndex]; }
	void Start(COffLinePlayer* pPlayer, COffLinePlayer* pTargetPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto);
	void Loop(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto);
	bool Over(pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto);

	void InitUnit(COffLinePlayer* pPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPto, Force enForce);

private:
	std::array<int, 2> m_arrBulidingHP;
	bool m_bIsOver{ false };
	bool m_bWinForce{ false };

	std::vector<COffLineUnit>	m_vctOffLineUnit;
	std::vector<COffLineHero*>	m_vctOffLineHero;
};

