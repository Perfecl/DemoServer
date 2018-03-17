#include "stdafx.h"
#include "Offline.h"
#include "GameServer.h"
#include "Army.h"
#include "Mission.h"
#include "OffLinePlayer.h"
#include "OffLineBattle.h"
#include "Mail.h"
#include "NeedReset.h"
#include "WorldBossBattle.h"
#include "Player.h"

COffline::COffline(CPlayer& player) :
player_{ player }
{
	__LoadFromDatabase();
}

COffline::~COffline()
{

}

void COffline::SendInitialMessage()
{
	__UpdateEscortWhenLogin();

	pto_OFFLINEBATTLE_S2C_RES_UpdatePlayerOfflineBattleData pto_data;
	for (auto &it : player_.army()->GetFormationHero())
		pto_data.add_hero_id(it);
	pto_data.set_ranking(offline_rank_);
	pto_data.set_times(player_.need_reset()->surplus_times());
	pto_data.set_continuous(win_streak_);
	if (last_time_ < 0)
		pto_data.set_last_times(0);
	else
		pto_data.set_last_times(last_time_);
	pto_data.set_buy_times(player_.need_reset()->offline_buy_times());

	player_.SendToAgent(pto_data, PLAYER_S2C_RES_UpdatePlayerOfflineBattleData);
}

void COffline::ProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case OFFLINEBATTLE_C2S_REQ_StartOfflineBattle:
		__StartOfflineBattle(msg);
		break;
	case OFFLINEBATTLE_C2S_NTF_PlayerEnter:
		__SetPlayerIsInOffline(msg);
		break;
	case OFFLINEBATTLE_C2S_REQ_ClearOfflineBattleCd:
		__ClearOfflineBattleCD();
		break;
	case OFFLINEBATTLE_C2S_REQ_BuyOfflineBattleTimes:
		__BuyOfflineBattleTimes();
		break;
	case OFFLINEBATTLE_C2S_REQ_ReplayOfflineBattle:
		__ReplayOfflineBattle(msg);
		break;
	case OFFLINEBATTLE_C2S_REQ_GetEscortCar:
		__GetEscortCar(msg);
		break;
	case OFFLINEBATTLE_C2S_REQ_StartEscort:
		__StartEscort();
		break;
	case OFFLINEBATTLE_C2S_REQ_UpdateEscort:
		__UpdateEscort();
		break;
	case OFFLINEBATTLE_C2S_NTF_EnterEscort:
		__EnterEscort(msg);
		break;
	case OFFLINEBATTLE_C2S_REQ_RobEscort:
		__RobEscort(msg);
		break;
	case OFFLINEBATTLE_C2S_NTF_SetAutoProtect:
		__SetAutoProtect(msg);
		break;
	case OFFLINEBATTLE_C2S_REQ_InviteProtect:
		__InviteProtect(msg);
		break;
	case OFFLINEBATTLE_C2S_RES_AskProtect:
		__ResAskProtect(msg);
		break;
	case OFFLINEBATTLE_C2S_REQ_UpdateProtecterList:
		__UpdateProtecterList();
		break;
	case PLAYER_C2S_REQ_UpdatePlayerOfflineBattleData:
		GAME_WORLD()->UpdatePlayerOfflineBattleData(&player_);
		break;
	case OFFLINEBATTLE_C2S_REQ_OfflineList:
		GAME_WORLD()->GetOfflineList(&player_);
		break;
	case OFFLINEBATTLE_C2S_REQ_BeginWorldBoss:
		__BeginWorldBossBattle();
		break;
	case OFFLINEBATTLE_C2S_REQ_UpdateWorldBoss:
		GAME_WORLD()->UpdateWorldBoss(&player_);
		break;
	default:RECORD_WARNING(FormatString("未知的Army协议号:", msg.GetProtocolID()).c_str()); break;
	}
}

void COffline::SaveToDatabase()
{
	std::ostringstream sql;

	sql << "update tb_player_offline set win_streak = " << win_streak_
		<< ", last_time = " << last_time_
		<< ", dartcar_lv = " << temp_dartcar_level_
		<< ", protect_mode = " << protect_mode_
		<< ", bodyguard = " << bodyguard_
		<< ", last_rob_time = " << last_rob_time_
		<< " where pid = " << player_.pid();

	MYSQL_UPDATE(sql.str());
}

void COffline::GetEnemyData(pto_OFFLINEBATTLE_S2C_RES_UpdatePlayerOfflineBattleData* pto)
{
	int nEnemyRank = 0;

	//测试排名
#ifdef _RANK_TEST
	for (int i = m_nOfflineRank - 1; i > m_nOfflineRank - 7; i--)
	{
		pto_OFFLINEBATTLE_Struct_EnemyData* pEnemyData = pONUPOBD->add_enemy_data();
		int nPID = GetPIDByRank(i);
		if (0 > nPID)
		{
			COffLinePlayer* pOfflinePlayer = COffLinePlayer::FindOffLinePlayerByPID(nPID);
			__GetEnemyPto(pOfflinePlayer, i, pEnemyData);
		}
		else if (nullptr != FIND_PLAYER(nPID))
		{
			__GetEnemyPto(FIND_PLAYER(nPID), i, pEnemyData);
		}
		else
		{
			SPOfflineData data = GAME_WORLD.GetOfflinePlayerData(nPID);
			if (nullptr != data)
			{
				pEnemyData->set_lv(data->level);
				pEnemyData->set_name(data->name);
				pEnemyData->set_sex(data->sex);
				pEnemyData->set_pid(nPID);
				pEnemyData->set_ranking(i);
			}
		}
	}
#else
	if (offline_rank_ <= 6)
	{
		for (size_t i = 6; i >= 1; i--)
		{
			if (i != offline_rank_)
			{
				auto pEnemyData = pto->add_enemy_data();
				int  pid = GAME_WORLD()->GetPIDByRank(i);
				if (0 > pid)
				{
					COffLinePlayer* pOfflinePlayer = COffLinePlayer::FindOffLinePlayerByPID(pid);
					__GetEnemyPto(pOfflinePlayer, i, pEnemyData);
				}
				else
				{
					SPIPlayerData data = PLAYER_DATA(pid);
					if (data)
					{
						pEnemyData->set_lv(data->level());
						pEnemyData->set_name(data->name());
						pEnemyData->set_sex(data->sex());
						pEnemyData->set_pid(pid);
						pEnemyData->set_ranking(i);
						pEnemyData->set_moduleid(data->clothes());
					}
				}
			}
		}
	}
	else
	{
		int nRange = 0;
		if (1000 < offline_rank_)
			nRange = 100;
		else if (1000 >= offline_rank_ && 500 < offline_rank_)
			nRange = 50;
		else if (500 >= offline_rank_ && 200 < offline_rank_)
			nRange = 20;
		else if (200 >= offline_rank_ && 100 < offline_rank_)
			nRange = 5;
		else if (100 >= offline_rank_ && 6 < offline_rank_)
			nRange = 1;

		nEnemyRank = offline_rank_ - nRange;

		for (int i = 0; i < 5; i++)
		{
			int nFinalEnemyRank = nEnemyRank;

			if (1000 < offline_rank_)
			{
				nFinalEnemyRank = nEnemyRank + 45;
				nFinalEnemyRank -= GetRandom(0, 90);
			}
			else if (1000 >= offline_rank_ && 500 < offline_rank_)
			{
				nFinalEnemyRank = nEnemyRank + 10;
				nFinalEnemyRank -= GetRandom(0, 20);
			}

			auto pEnemyData = pto->add_enemy_data();
			int  pid = GAME_WORLD()->GetPIDByRank(nFinalEnemyRank);

			if (0 > pid)
			{
				COffLinePlayer* pOfflinePlayer = COffLinePlayer::FindOffLinePlayerByPID(pid);
				__GetEnemyPto(pOfflinePlayer, nFinalEnemyRank, pEnemyData);
			}
			else
			{
				SPIPlayerData data = PLAYER_DATA(pid);
				if (data)
				{
					pEnemyData->set_lv(data->level());
					pEnemyData->set_name(data->name());
					pEnemyData->set_sex(data->sex());
					pEnemyData->set_pid(pid);
					pEnemyData->set_ranking(nFinalEnemyRank);
					pEnemyData->set_moduleid(data->clothes());
				}
			}
			nEnemyRank -= nRange;
		}
	}
#endif
}

void COffline::SetPlayerEscort(SPPlayer player, pto_OFFLINEBATTLE_S2C_RES_UpdateEscort* pto, PlayerEscort* escort)
{
	pto_OFFLINEBATTLE_S2C_NTF_PlayereEscort* pESPD = pto->add_player_escort();
	pESPD->set_hp(escort->m_nHp);
	pESPD->set_y(escort->m_nY);
	pESPD->set_p_id(escort->m_nPId);
	pESPD->set_car_lv(escort->level);
	pESPD->set_lv(player->level());
	pESPD->set_name(player->name());
	pESPD->set_time(escort->m_nStartTime);

	SPIPlayerData player_bodyguard{ nullptr };

	if (0 == player->offline()->bodyguard_)
		pESPD->set_protecter_name("");
	else if (player_bodyguard = PLAYER_DATA(player->offline()->bodyguard_))
		pESPD->set_protecter_name(player_bodyguard->name());
	else
		pESPD->set_protecter_name("");
}

void COffline::SetPlayerEscort(COffLinePlayer* offline_player, pto_OFFLINEBATTLE_S2C_RES_UpdateEscort* pto, PlayerEscort* escort)
{
	pto_OFFLINEBATTLE_S2C_NTF_PlayereEscort* pESPD = pto->add_player_escort();
	pESPD->set_hp(escort->m_nHp);
	pESPD->set_y(escort->m_nY);
	pESPD->set_p_id(escort->m_nPId);
	pESPD->set_car_lv(offline_player->GetDartCarLevel());
	pESPD->set_lv(offline_player->GetLevel());
	pESPD->set_name(offline_player->GetName());
	pESPD->set_time(escort->m_nStartTime);

	SPIPlayerData player_bodyguard{ nullptr };

	if (0 == offline_player->GetBodyguard())
		pESPD->set_protecter_name("");
	else if (player_bodyguard = PLAYER_DATA(offline_player->GetBodyguard()))
		pESPD->set_protecter_name(player_bodyguard->name());
}

int	COffline::GetEscortReputation(SPIPlayerData data, PlayerEscort* escort)
{
	if (!data)
		return 0;

	float fCarMul = 0;

	switch (escort->level)
	{
	case 1:	fCarMul = 0.75; break;
	case 2:	fCarMul = 1.0; break;
	case 3: fCarMul = 1.25; break;
	case 4: fCarMul = 1.5; break;
	case 5: fCarMul = 2.0; break;
	default:break;
	}

	return (int)(CRootData::GetRootData(data->level(), RDT_EscortReputation) * fCarMul);
}

int	COffline::GetEscortReward(int player_level, int dartcar_level, PlayerEscort* escort)
{
	float fHPMul = 0;

	switch (escort->m_nHp)
	{
	case 2:fHPMul = 1; break;
	case 1:fHPMul = 0.85f; break;
	case 0:fHPMul = 0.7f; break;
	default:break;
	}

	float fCarMul = 0;
	switch (dartcar_level)
	{
	case 1:	fCarMul = 0.75f; break;
	case 2:	fCarMul = 1.0f; break;
	case 3: fCarMul = 1.25f; break;
	case 4: fCarMul = 1.5f; break;
	case 5: fCarMul = 2.0f; break;
	default:break;
	}

	return (int)(CRootData::GetRootData(player_level, RDT_EscortSilver) * fHPMul * fCarMul);
}

int	COffline::GetEscortReputation(int player_level, int dartcar_level, PlayerEscort* escort)
{
	float fHPMul = 0;

	switch (escort->m_nHp)
	{
	case 2:fHPMul = 1; break;
	case 1:fHPMul = 0.85f; break;
	case 0:fHPMul = 0.7f; break;
	default:break;
	}

	float fCarMul = 0;
	switch (dartcar_level)
	{
	case 1:	fCarMul = 0.75f; break;
	case 2:	fCarMul = 1.0f; break;
	case 3: fCarMul = 1.25f; break;
	case 4: fCarMul = 1.5f; break;
	case 5: fCarMul = 2.0f; break;
	default:break;
	}

	return (int)(CRootData::GetRootData(player_level, RDT_EscortReputation) * fHPMul * fCarMul);
}

void COffline::SendBeRobbed(CPlayer* robber, bool win)
{
	pto_OFFLINEBATTLE_S2C_NTF_BeRobbed pto_ntf;
	pto_ntf.set_pid(robber->pid());
	pto_ntf.set_name(robber->name());
	pto_ntf.set_win(win);

	player_.SendToAgent(pto_ntf, OFFLINEBATTLE_S2C_NTF_BeRobbed);
}

bool COffline::ExercisePlatformBattle(CPlayer* player, int pid, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle)
{
	if (player)
		return player->offline()->__ExercisePlatformBattle(pid, pPtoBattle);

	return true;
}

void COffline::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select * from tb_player_offline where pid =" << player_.pid();

	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };

	if (result && result->next())
	{
		last_time_ = result->getInt64("last_time");
		win_streak_ = result->getInt("win_streak");
		temp_dartcar_level_ = result->getInt("dartcar_lv");
		protect_mode_ = result->getInt("protect_mode");
		bodyguard_ = result->getInt("bodyguard");
		last_rob_time_ = result->getInt64("last_rob_time");
	}
	else
	{
		RECORD_WARNING("查询离线失败");
	}
}

void COffline::__StartOfflineBattle(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_REQ_StartOfflineBattle pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	__StartOfflineBattle(pto.enemy_pid());
}

void COffline::__StartOfflineBattle(int pid)
{
	pto_OFFLINEBATTLE_S2C_RES_StartOfflineBattle pto;

	if (600 > time(0) - last_time_)
		pto.set_res(1);
	else if (0 >= player_.need_reset()->surplus_times())
		pto.set_res(2);
	else if (player_.army()->GetFormationHero().empty())
		pto.set_res(4);

	if (0 == pto.res())
	{
		if (0 > pid)
		{
			__EnterOffLineBattle(pid, &pto);
		}
		else
		{
			SPIPlayerData data = PLAYER_DATA(pid);

			if (data)
			{
				COffLinePlayer nTempPlayer;
				nTempPlayer.SetOffLinePlayerByData(data);
				__EnterOffLineBattle(&nTempPlayer, &pto);
			}
		}
	}

	if (0 != pto.res())
		player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_StartOfflineBattle);
}

void COffline::__EnterOffLineBattle(CPlayer* pTargetPlayer, pto_OFFLINEBATTLE_S2C_RES_StartOfflineBattle* pto)
{
	COffLinePlayer nOffLinePlayer;
	nOffLinePlayer.SetOffLinePlayerByPlayer(pTargetPlayer);
	__EnterOffLineBattle(&nOffLinePlayer, pto);
}

void COffline::__EnterOffLineBattle(COffLinePlayer* pTargetPlayer, pto_OFFLINEBATTLE_S2C_RES_StartOfflineBattle* pto)
{
	pto_OFFLINEBATTLE_Struct_SaveOfflineBattle ptoSaveOffLineBattle;

	ptoSaveOffLineBattle.set_time(time(0));
	ptoSaveOffLineBattle.set_id(GAME_WORLD()->offline_record_id());
	GAME_WORLD()->ChangeOfflineRecordID(1);

	auto pPlayOffLineBattle = ptoSaveOffLineBattle.mutable_play_data();
	pPlayOffLineBattle->set_type(OBT_OfflineRank);
	player_.need_reset()->change_surplus_times(-1);
	pto->set_res(0);

	COffLineBattle m_nOfflineBattle;
	bool m_bWin = false;

	COffLinePlayer nOffLinePlayer;
	nOffLinePlayer.SetOffLinePlayerByPlayer(&player_);

	player_.ImplementRewardMission(RMT_OfflineBattle, 0, 1);
	player_.ImplementMission(MTT_OfflineBattle, 0, 0);

	m_nOfflineBattle.Start(&nOffLinePlayer, pTargetPlayer, pPlayOffLineBattle);
	m_nOfflineBattle.Loop(pPlayOffLineBattle);
	m_bWin = m_nOfflineBattle.Over(pPlayOffLineBattle);

	GAME_WORLD()->AddOffLineBattleSaveData(&ptoSaveOffLineBattle);

	auto m_pMsgPOB = pto->mutable_play_data();
	*m_pMsgPOB = *pPlayOffLineBattle;

	int m_nAtkRank = 1;
	int m_nDefRank = 1;
	bool m_bResourcesFound = GAME_WORLD()->GetPlayersRank(m_nAtkRank, m_nDefRank, &player_, pTargetPlayer->GetPID());

	SPPlayer pTempPlayer{ nullptr };
	if (pTargetPlayer->GetPID() > 0)
		pTempPlayer = GAME_WORLD()->FindPlayer(pTargetPlayer->GetPID());

	if (true == m_bWin)
	{
		pto->set_win(true);
		GAME_WORLD()->ResetOfflineRank(m_nAtkRank, m_nDefRank, &player_, pTargetPlayer->GetPID(), m_bResourcesFound);

		win_streak_++;

		if (win_streak_ >= 5)
			player_.GiveNewTitle(8);

		__int64 silver = CRootData::GetRootData(player_.level(), RDT_OfflineBattleSingleSilver);
		player_.ChangeSilver(silver);
		player_.ChangeHonor(silver);

		if (m_nDefRank <= 10 && m_nAtkRank > m_nDefRank)
			GAME_WORLD()->Annunciate(player_.name(), m_nDefRank, 0, AnnunciateType::OfflineBattleRank);

		GAME_WORLD()->SavePlayerOfflineBattleData(1, ptoSaveOffLineBattle.id(), time(0), 1, player_.pid(), pTargetPlayer->GetPID());
		GAME_WORLD()->SavePlayerOfflineBattleData(0, ptoSaveOffLineBattle.id(), time(0), 0, pTargetPlayer->GetPID(), player_.pid());
		last_time_ = 0;
	}
	else
	{
		pto->set_win(false);
		win_streak_ = 0;

		GAME_WORLD()->SavePlayerOfflineBattleData(1, ptoSaveOffLineBattle.id(), time(0), 0, player_.pid(), pTargetPlayer->GetPID());
		GAME_WORLD()->SavePlayerOfflineBattleData(0, ptoSaveOffLineBattle.id(), time(0), 1, pTargetPlayer->GetPID(), player_.pid());

		last_time_ = time(0);
	}

	player_.SendToAgent(*pto, OFFLINEBATTLE_S2C_RES_StartOfflineBattle);

	if (nullptr != pTempPlayer)
		__SendBeAttackedMessage(m_bWin, m_nAtkRank, &player_, pTempPlayer);
}

void COffline::__EnterOffLineBattle(int pid, pto_OFFLINEBATTLE_S2C_RES_StartOfflineBattle* pto)
{
	COffLinePlayer* pAIPlayer = COffLinePlayer::FindOffLinePlayerByPID(pid);
	if (nullptr == pAIPlayer)
	{
		pto->set_res(3);
		player_.SendToAgent(*pto, OFFLINEBATTLE_S2C_RES_StartOfflineBattle);
	}
	else
	{
		__EnterOffLineBattle(pAIPlayer, pto);
	}
}

void COffline::__SendBeAttackedMessage(bool& bAtkWin, int nNewRank, CPlayer* pAtkPlayer, SPPlayer pDefPlayer)
{
	if (nullptr == pAtkPlayer || nullptr == pDefPlayer)
	{
		printf("未能找到玩家");
		return;
	}

	pto_OFFLINEBATTLE_S2C_NTF_BeAttacked pto;
	pto.set_name(pAtkPlayer->name());
	pto.set_pid(pAtkPlayer->pid());

	if (bAtkWin)
	{
		pto.set_win(0);
		pto.set_rank(nNewRank);
	}
	else
	{
		pto.set_win(1);
	}

	pDefPlayer->SendToAgent(pto, OFFLINEBATTLE_S2C_NTF_BeAttacked);
}

void COffline::__SetPlayerIsInOffline(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_NTF_PlayerEnter pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	is_in_offline_ = pto.enter();
}

void COffline::__ClearOfflineBattleCD()
{
	pto_OFFLINEBATTLE_S2C_RES_ClearOfflineBattleCd pto;

	pto.set_res(0);

	time_t nowTime;
	time(&nowTime);

	if (600 <= (nowTime - last_time_))
		pto.set_res(2);

	int nCD = (int)(600 - (nowTime - last_time_));
	int nGold = ((nCD - 1) / 60 + 1);

	if (nGold > player_.gold())
		pto.set_res(1);

	if (0 == pto.res())
	{
		player_.ChangeGold(-nGold, 10);
		last_time_ = 0;
	}

	player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_ClearOfflineBattleCd);
}

void COffline::__BuyOfflineBattleTimes()
{
	pto_OFFLINEBATTLE_S2C_RES_BuyOfflineBattleTimes pto;
	pto.set_res(0);

	if (player_.need_reset()->offline_buy_times() >= CVIPFun::GetVIPFun(player_.vip_level(), VFT_BuyOfflinebattleTimes))
	{
		pto.set_res(2);
	}

	int nGold = CGoldSpend::GetGoldSpend(player_.need_reset()->offline_buy_times() + 1, GST_BuyOfflineBattleSpend);
	if (player_.gold() < nGold)
	{
		pto.set_res(1);
	}

	if (0 == pto.res())
	{
		player_.ChangeGold(-nGold, 11);
		player_.need_reset()->change_offline_buy_times(1);
		player_.need_reset()->change_surplus_times(1);
	}

	player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_BuyOfflineBattleTimes);
}

void COffline::__GetEnemyPto(COffLinePlayer* pOffLinePlayer, int nRank, pto_OFFLINEBATTLE_Struct_EnemyData* pPto)
{
	if (pOffLinePlayer)
	{
		pPto->set_lv(pOffLinePlayer->GetLevel());
		pPto->set_ranking(nRank);
		pPto->set_name(pOffLinePlayer->GetName().c_str());
		pPto->set_pid(pOffLinePlayer->GetPID());
		pPto->set_sex(pOffLinePlayer->GetSex());
		pPto->set_moduleid(0);
	}
	else
	{
		RECORD_WARNING("找不到离线玩家数据");
	}

}

void COffline::__GetEnemyPto(SPPlayer pPlayer, int nRank, pto_OFFLINEBATTLE_Struct_EnemyData* pPto)
{
	if (pPlayer)
	{
		pPto->set_lv(pPlayer->level());
		pPto->set_ranking(nRank);
		pPto->set_name(pPlayer->name());
		pPto->set_pid(pPlayer->pid());
		pPto->set_sex(pPlayer->sex());
		pPto->set_moduleid(pPlayer->clothes());
	}
	else
	{
		RECORD_WARNING("找不到离线玩家数据");
	}
}

void COffline::__ReplayOfflineBattle(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_REQ_ReplayOfflineBattle pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_OFFLINEBATTLE_S2C_RES_ReplayOfflineBattle pto_res;

	auto pOSSOB = GAME_WORLD()->GetOfflineBattleSavedata(pto.id());

	if (nullptr != pOSSOB)
	{
		pto_res.set_res(0);
		pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPlayData = pto_res.mutable_play_data();
		*pPlayData = pOSSOB->play_data();
	}
	else
	{
		pto_res.set_res(1);
	}

	player_.SendToAgent(pto_res, OFFLINEBATTLE_S2C_RES_ReplayOfflineBattle);
}

void COffline::__BeginWorldBossBattle()
{
	pto_OFFLINEBATTLE_S2C_RES_BeginWorldBossBattle pto;
	pto.set_res(0);

	int nFinalDmg = 0;

	if (GAME_WORLD()->GetWorldBoss().m_nHP <= 0)
		GAME_WORLD()->StartWorldBoss();


	SWorldBossRank* rank = GAME_WORLD()->FindWorldBossRanking(player_.pid());
	if (rank)
		nFinalDmg += rank->m_nDmg;

	if (!pto.res())
	{
		COffLinePlayer nOffLinePlayer;
		nOffLinePlayer.SetOffLinePlayerByPlayer(&player_);

		pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pBattle = pto.mutable_battle();

		pBattle->set_type(OfflineBattleType::OBT_WorldBoss);

		bool bFirst{ false };
		if (GAME_WORLD()->GetWorldBoss().m_nMaxHP - GAME_WORLD()->GetWorldBoss().m_nHP <= 0)
			bFirst = true;

		CWorldBossBattle nBattle;
		nBattle.Start(&nOffLinePlayer, &GAME_WORLD()->GetWorldBoss(), pBattle);
		nBattle.Loop(pBattle);

		int nDmg = nBattle.Over(pBattle);

		//第一个造成伤害
		if (bFirst && nDmg > 0)
		{
			pto.set_first_dmg(true);
		}

		if (GAME_WORLD()->GetWorldBoss().m_nHP <= 0)
		{
			pto.set_last_dmg(true);
			GAME_WORLD()->StartWorldBoss();
		}

		nFinalDmg += nDmg;

		GAME_WORLD()->GetWorldBoss().m_nHP -= nDmg;

		//伤害排名
		GAME_WORLD()->ChangeWorldBossRank(player_.pid(), nFinalDmg);
	}

	player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_BeginWorldBoss);
}

void COffline::__GetEscortCar(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_REQ_GetEscortCar pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_OFFLINEBATTLE_S2C_RES_GetEscortCar pto_res;
	pto_res.set_res(0);

	if (0 == pto.type())
	{
		if (0 != temp_dartcar_level_)
			pto_res.set_res(1);

		if (0 == pto_res.res())
		{
			__RandCar();
			pto_res.set_lv(temp_dartcar_level_);
		}

		player_.SendToAgent(pto_res, OFFLINEBATTLE_S2C_RES_GetEscortCar);
	}
	else if (1 == pto.type())
	{
		int nGold = CGoldSpend::GetGoldSpend(GoldSpendType::GST_RandEscortCar);

		if (nGold > player_.gold())
			pto_res.set_res(2);
		else if (GAME_WORLD()->FindEscort(player_.pid()))
			pto_res.set_res(1);

		if (0 == pto_res.res())
		{
			player_.ChangeGold(-nGold, 12);
			__RandCar();
			pto_res.set_lv(temp_dartcar_level_);
		}

		player_.SendToAgent(pto_res, OFFLINEBATTLE_S2C_RES_GetEscortCar);
	}
}

void COffline::__StartEscort()
{
	pto_OFFLINEBATTLE_S2C_RES_StartEscort pto;
	pto.set_res(0);

	if (0 == temp_dartcar_level_)
		pto.set_res(1);
	else if (GAME_WORLD()->FindEscort(player_.pid()))
		pto.set_res(2);
	else if (0 >= player_.need_reset()->escort_times())
		pto.set_res(3);

	if (0 == pto.res())
	{
		player_.need_reset()->change_escort_times(-1);

		PlayerEscort* pPlayerEscort = new PlayerEscort;

		pPlayerEscort->m_nY = GetRandom(0, 100);
		pPlayerEscort->m_nPId = player_.pid();
		pPlayerEscort->m_nHp = 2;
		pPlayerEscort->m_nStartTime = time(0);
		pPlayerEscort->level = temp_dartcar_level_;

		if (5 == temp_dartcar_level_)
		{
			pto_OFFLINEBATTLE_S2C_NTF_Announcement pto_Ann;
			pto_Ann.set_name(player_.name());
			pto_Ann.set_type(5);
			std::string str;
			pto_Ann.SerializeToString(&str);
			GAME_WORLD()->SendToAllPlayers(str, MSG_S2C, OFFLINEBATTLE_S2C_NTF_Announcement);
		}
		temp_dartcar_level_ = 0;

		GAME_WORLD()->InserEscort(pPlayerEscort);

		__SendNewEscort(pPlayerEscort);

		player_.ImplementRewardMission(RMT_Escort, 0, 1);
		player_.ImplementMission(MTT_Escort, 0, 0);

		pto.set_res(0);
		player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_StartEscort);
	}
}

void COffline::__RandCar()
{
	int nResult = 0;

	while (!nResult)
	{
		int nLevel = GetRandom(1, 5);
		int nChance = 0;
		switch (nLevel)
		{
		case 1:	nChance = 100;	break;
		case 2:	nChance = 25;	break;
		case 3:	nChance = 15;	break;
		case 4:	nChance = 10;	break;
		case 5:	nChance = 5;	break;
		default:	break;
		}
		if (GetRandom(0, 100) <= nChance)
			nResult = nLevel;
	}
	temp_dartcar_level_ = nResult;
}

void COffline::__SendNewEscort(PlayerEscort* pPlayerEscort)
{
	pto_OFFLINEBATTLE_S2C_NTF_PlayereEscort pto;
	pto.set_car_lv(pPlayerEscort->level);
	pto.set_hp(pPlayerEscort->m_nHp);
	pto.set_y(pPlayerEscort->m_nY);
	pto.set_lv(player_.level());
	pto.set_name(player_.name());
	pto.set_p_id(pPlayerEscort->m_nPId);
	pto.set_time(pPlayerEscort->m_nStartTime);

	if (0 == bodyguard_)
	{
		pto.set_protecter_name("");
		__SendNewEscort(&pto);
	}
	else
	{
		SPPlayer player = GAME_WORLD()->FindPlayer(bodyguard_);

		if (player)
		{
			pto.set_protecter_name(player->name());
			__SendNewEscort(&pto);
		}
		else
		{
			pto.set_protecter_name("");
			__SendNewEscort(&pto);
		}
	}
}

void COffline::__SendNewEscort(pto_OFFLINEBATTLE_S2C_NTF_PlayereEscort* pto)
{
	std::string str;
	pto->SerializeToString(&str);
	GAME_WORLD()->SendEscortPlayer(str, MSG_S2C, OFFLINEBATTLE_S2C_NTF_PlayereEscort);
}

void COffline::__UpdateEscort()
{
	pto_OFFLINEBATTLE_S2C_RES_UpdateEscort pto;
	pto.set_escort_times(player_.need_reset()->escort_times());
	pto.set_protect_times(CVIPFun::GetVIPFun(player_.vip_level(), VFT_EscortProtectTimes));
	pto.set_rob_times(player_.need_reset()->rob_times());
	PlayerEscort* escort = GAME_WORLD()->FindEscort(player_.pid());
	if (escort)
	{
		pto.set_car_lv(escort->level);
		pto.set_time(escort->m_nStartTime);
	}
	else
	{
		pto.set_car_lv(0);
		pto.set_time(0);
	}
	pto.set_auto_protect(__AutoProtect());
	pto.set_auto_refuse(__AutoRefuse());
	pto.set_last_rob_times(last_rob_time_);

	time_t nNowTime = time(0);

	GAME_WORLD()->UpdateEscort(&pto);

	player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_UpdateEscort);
}

void COffline::__EnterEscort(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_NTF_EnterEscort pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	is_in_escort_ = pto.enter();
}

void COffline::__RobEscort(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_REQ_RobEscort pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_OFFLINEBATTLE_S2C_RES_RobEscort pto_res;

	pto_res.set_res(0);

	if (player_.army()->GetFormationHero().empty())
		pto_res.set_res(4);

	else if (player_.pid() == pto.p_id())
		pto_res.set_res(1);
	else if (0 >= player_.need_reset()->rob_times())
		pto_res.set_res(2);
	else if (int(time(0) - last_rob_time_) < 600)
		pto_res.set_res(5);

	PlayerEscort* escort = GAME_WORLD()->FindEscort(pto.p_id());

	if (nullptr == escort)
		pto_res.set_res(3);
	else if (0 >= escort->m_nHp)
		pto_res.set_res(1);

	if (escort)
	{
		SPIPlayerData data = PLAYER_DATA(escort->m_nPId);

		if (!data)
		{
			pto_res.set_res(3);
		}
		else if (data)
		{
			if (player_.pid() == data->bodygurand())
				pto_res.set_res(1);
		}

		if (0 == pto_res.res())
		{
			player_.ImplementMission(MTT_RobEscort, 0, 0);

			pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle = pto_res.mutable_battle();

			pPtoBattle->set_type(OBT_Escort);

			bool bResult{ false };

			SPIPlayerData bodyguard_data = PLAYER_DATA(data->bodygurand());

			if (bodyguard_data)
			{
				bResult = __EscortBattle(data->bodygurand(), pPtoBattle);

				if (!bResult)
					__SendEscortProtectRewardMail(data->bodygurand(), GetEscortReputation(data, escort));
			}
			else
			{
				bResult = __EscortBattle(pto.p_id(), pPtoBattle);
			}

			SPPlayer target = GAME_WORLD()->FindPlayer(pto.p_id());
			if (target)
				target->offline()->SendBeRobbed(&player_, bResult);

			pto_OFFLINEBATTLE_Struct_SaveOfflineBattle ptoOSSOB;
			ptoOSSOB.set_time(time(0));
			ptoOSSOB.set_id(GAME_WORLD()->offline_record_id());
			GAME_WORLD()->ChangeOfflineRecordID(1);
			pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pOSPOB = ptoOSSOB.mutable_play_data();
			*pOSPOB = *pPtoBattle;

			GAME_WORLD()->AddOffLineBattleSaveData(&ptoOSSOB);

			player_.need_reset()->change_rob_times(-1);

			last_rob_time_ = time(0);

			if (bResult)
			{
				escort->m_nHp--;
				__int64 silver = __int64(CRootData::GetRootData(data->level(), RDT_EscortSilver) * 0.15f);
				int reputation = int(CRootData::GetRootData(data->level(), RDT_EscortReputation) * 0.15f);
				player_.ChangeSilver(silver);
				player_.ChangeReputation(reputation);
				pto_res.set_silver(silver);
				pto_res.set_reputation(reputation);
				__UpdataEscortHP(escort->m_nPId, escort->m_nHp);
			}
			else
			{
				pto_res.set_silver(0);
				pto_res.set_reputation(0);
			}
		}
	}

	player_.SendToAgent(pto_res, OFFLINEBATTLE_S2C_RES_RobEscort);
}

bool COffline::__EscortBattle(CPlayer* pTargetPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pto)
{
	COffLinePlayer nOffLinePlayer;
	nOffLinePlayer.SetOffLinePlayerByPlayer(pTargetPlayer);
	return __EscortBattle(&nOffLinePlayer, pto);
}

bool COffline::__EscortBattle(COffLinePlayer* pTargetPlayer, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pto)
{
	COffLineBattle m_nOfflineBattle;

	COffLinePlayer nOffLinePlayer;
	nOffLinePlayer.SetOffLinePlayerByPlayer(&player_);
	m_nOfflineBattle.Start(&nOffLinePlayer, pTargetPlayer, pto);
	m_nOfflineBattle.Loop(pto);
	return m_nOfflineBattle.Over(pto);
}

bool COffline::__EscortBattle(int pid, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pto)
{
	SPIPlayerData data = PLAYER_DATA(pid);
	COffLinePlayer nOfflinePlayer;
	nOfflinePlayer.SetOffLinePlayerByData(data);
	return __EscortBattle(&nOfflinePlayer, pto);
}

void COffline::__SendEscortProtectRewardMail(int pid, int reward)
{
	SPMail mail = std::make_shared<CMail>();
	mail->SetModelID(MMI_EscortProtect);
	mail->AddRewardItem(LT_Reputation, reward);

	GAME_WORLD()->GiveNewMail(mail, pid);
}

void COffline::__SetAutoProtect(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_NTF_SetAutoProtect pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	__SetAutoProtectMode(pto.auto_protect(), pto.auto_refuse());
}

void COffline::__InviteProtect(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_REQ_InviteProtect pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_OFFLINEBATTLE_S2C_RES_InviteProtect pto_res;

	pto_res.set_res(0);

	SPPlayer pProtecter = GAME_WORLD()->FindPlayer(pto.p_id());

	if (nullptr != pProtecter)
	{
		if (0 == temp_dartcar_level_)
			pto_res.set_res(2);		//玩家未获得镖车
		if (CVIPFun::GetVIPFun(pProtecter->vip_level(), VFT_EscortProtectTimes) <= player_.need_reset()->protect_times())
			pto_res.set_res(4);		//保镖次数用完
		if (!player_.IsFreind(pProtecter->pid()))
			pto_res.set_res(3);		//对方未加好友
	}
	else
	{
		pto_res.set_res(6);			//保镖不存在
	}

	player_.SendToAgent(pto_res, OFFLINEBATTLE_S2C_RES_InviteProtect);

	if (0 == pto_res.res())
	{
		if (pProtecter->offline()->__AutoProtect())
		{
			pto_OFFLINEBATTLE_S2C_NTF_ResAskProtect ptoRes;
			ptoRes.set_p_id(pProtecter->pid());
			ptoRes.set_res(1);
			ptoRes.set_name(pProtecter->name());
			bodyguard_ = player_.pid();

			player_.SendToAgent(ptoRes, OFFLINEBATTLE_S2C_NTF_ResAskProtect);
		}
		else if (pProtecter->offline()->__AutoRefuse())
		{
			pto_OFFLINEBATTLE_S2C_NTF_ResAskProtect ptoRes;
			ptoRes.set_p_id(pProtecter->pid());
			ptoRes.set_res(0);
			ptoRes.set_name(pProtecter->name());

			player_.SendToAgent(ptoRes, OFFLINEBATTLE_S2C_NTF_ResAskProtect);
		}
		else if (!pProtecter->offline()->__AutoProtect())
		{
			pto_OFFLINEBATTLE_S2C_REQ_AskProtect m_ptoAP;
			m_ptoAP.set_p_id(player_.pid());
			m_ptoAP.set_name(player_.name());
			m_ptoAP.set_car_lv(temp_dartcar_level_);

			pProtecter->SendToAgent(m_ptoAP, OFFLINEBATTLE_S2C_REQ_AskProtect);
		}
	}
}

void COffline::__ResAskProtect(const CMessage& msg)
{
	pto_OFFLINEBATTLE_C2S_RES_AskProtect pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	SPPlayer player = GAME_WORLD()->FindPlayer(pto.p_id());

	if (player)
	{
		pto_OFFLINEBATTLE_S2C_NTF_ResAskProtect pto_res;
		if (1 == pto.res() && player->offline()->player_.need_reset()->protect_times() < CVIPFun::GetVIPFun(player->vip_level(), VFT_EscortProtectTimes))
		{
			player->offline()->bodyguard_ = player_.pid();
			pto_res.set_res(1);
		}
		else
		{
			pto_res.set_res(0);
		}

		pto_res.set_name(player_.name());
		pto_res.set_p_id(player_.pid());

		player->SendToAgent(pto_res, OFFLINEBATTLE_S2C_NTF_ResAskProtect);
	}
}

void COffline::__SetAutoProtectMode(bool bAutoProtect, bool bAutoRefuse)
{
	if (bAutoProtect)
		protect_mode_ = 1;
	else if (bAutoRefuse)
		protect_mode_ = 2;
	else
		protect_mode_ = 0;
}

void COffline::__UpdateProtecterList()
{
	pto_OFFLINEBATTLE_S2C_RES_UpdateProtecterList pto;

	for (auto &it : player_.GetFriendList())
	{
		if (__PlayerCanProtect(it))
			pto.add_p_id(it);
	}

	player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_UpdateProtecterList);
}

bool COffline::__PlayerCanProtect(int pid)
{
	SPPlayer player = GAME_WORLD()->FindPlayer(pid);

	if (nullptr == player)
		return false;

	if (player_.need_reset()->protect_times() >= CVIPFun::GetVIPFun(player->vip_level(), VFT_EscortProtectTimes))
		return false;
	if (!player_.IsFreind(pid))
		return false;

	return true;
}

void COffline::__UpdateEscortWhenLogin()
{
	pto_OFFLINEBATTLE_S2C_RES_UpdateEscort pto;

	PlayerEscort* escort = GAME_WORLD()->FindEscort(player_.pid());

	if (escort)
	{
		pto.set_time(escort->m_nStartTime);
		pto.set_car_lv(escort->level);
	}
	else
	{
		pto.set_time(0);
		pto.set_car_lv(0);
		bodyguard_ = 0;
	}

	pto.set_escort_times(player_.need_reset()->escort_times());
	pto.set_protect_times(CVIPFun::GetVIPFun(player_.vip_level(), VFT_EscortProtectTimes));
	pto.set_rob_times(player_.need_reset()->rob_times());
	pto.set_last_rob_times(last_rob_time_);

	player_.SendToAgent(pto, OFFLINEBATTLE_S2C_RES_UpdateEscort);
}

void COffline::__ClearWorldBossCD()
{
	pto_OFFLINEBATTLE_S2C_RES_ClearWorldBossCD pto_res;
	pto_res.set_res(0);

	if (GAME_WORLD()->GetWorldBoss().m_nHP <= 0)
		pto_res.set_res(2);

	SWorldBossRank* rank = GAME_WORLD()->FindWorldBossRanking(player_.pid());
	if (!rank)
	{
		pto_res.set_res(2);
	}
	else
	{
		int cd = int(time(0) - rank->m_nLastTime);

		int gold = 0;

		if (cd >= 120)
		{
			pto_res.set_res(2);
		}
		else
		{
			time_t remain_time = 120 - (time(0) - rank->m_nLastTime);
			if (!(remain_time % 60))
				gold = int(remain_time / 60);
			else
				gold = int((remain_time - (remain_time % 60)) / 60 + 1);

			if (gold > player_.gold())
				pto_res.set_res(1);

			if (!pto_res.res())
			{
				rank->m_nLastTime = 0;
				player_.ChangeGold(-gold, 13);
			}
		}
	}
	player_.SendToAgent(pto_res, OFFLINEBATTLE_S2C_RES_ClearWorldBossCD);
}

void COffline::__UpdataEscortHP(int pid, int hp)
{
	pto_OFFLINEBATTLE_S2C_NTF_UpdataEscortHP pto_ntf;
	pto_ntf.set_p_id(pid);
	pto_ntf.set_hp(hp);

	std::string pto_str;
	pto_ntf.SerializeToString(&pto_str);
	GAME_WORLD()->SendEscortPlayer(pto_str, MSG_S2C, OFFLINEBATTLE_S2C_NTF_UpdataEscortHP);
}

bool COffline::__ExercisePlatformBattle(int pid, pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pPtoBattle)
{
	SPIPlayerData data = PLAYER_DATA(pid);
	COffLinePlayer nOfflinePlayer;
	nOfflinePlayer.SetOffLinePlayerByData(data);
	return __EscortBattle(&nOfflinePlayer, pPtoBattle);
}

