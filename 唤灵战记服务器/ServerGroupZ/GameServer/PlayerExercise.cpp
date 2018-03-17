#include "stdafx.h"
#include "PlayerExercise.h"
#include "GameWorld.h"
#include "Offline.h"
#include "Player.h"

CPlayerExercise::CPlayerExercise(CPlayer& player) :
player_{ player }
{
	__LoadFromDatabase();
}

CPlayerExercise::~CPlayerExercise()
{

}

void CPlayerExercise::SendInitialMessage()
{
	pto_PLAYER_S2C_NTF_UpdatePlatform pto_ntf;

	pto_ntf.set_contend_times(contend_times_);
	pto_ntf.set_buy_contend(buy_contend_);
	pto_ntf.set_exp_pill_times(exp_pill_times_);
	pto_ntf.set_today_exercise_time(today_exercise_time_);
	pto_ntf.set_today_exercise_exp(today_exercise_exp_);
	pto_ntf.set_last_exercise_time(last_exercise_time_);
	pto_ntf.set_last_exercise_exp(last_exercise_exp_);
	pto_ntf.set_exp_box(exp_box_);
	pto_ntf.set_contend_cd(contend_cd_);
	pto_ntf.set_exp_pill_cd(exp_pill_cd_);

	for (int i = 0; i < GAME_WORLD()->ExercisePlatformsSize(); i++)
	{
		auto platform = GAME_WORLD()->GetExercisePlatform(i + 1);
		if (platform)
		{
			pto_PLAYER_STRUCT_Platform* pto_platform = pto_ntf.add_platform();
			pto_platform->set_id(platform->id);
			pto_platform->set_pid(platform->pid);
			pto_platform->set_start_time(platform->start_time);
			if (!platform->pid)
			{
				pto_platform->set_level(0);
				pto_platform->set_guild_id(0);
			}
			else
			{
				SPIPlayerData data = PLAYER_DATA(platform->pid);
				if (data)
				{
					pto_platform->set_level(data->level());
					pto_platform->set_guild_id(data->guild_id());
				}
			}
		}

		auto pill = GAME_WORLD()->GetExpPill(i + 1);
		if (pill && pill->times > 0)
		{
			pto_PLAYER_STRUCT_ExpPill* pto_pill = pto_ntf.add_exp_pill();
			pto_pill->set_id(pill->id);
			if (pill->times > 0)
				pto_pill->set_collect(true);
			else
				pto_pill->set_collect(false);
			for (size_t k = 0; k < pill->taken_player.size(); k++)
			{
				pto_pill->add_pid(pill->taken_player.at(k));
			}
		}
	}
	player_.SendToAgent(pto_ntf, PLAYER_S2C_NTF_UpdatePlatform);

	auto down_msg = GAME_WORLD()->GetDownMsg(player_.pid());
	if (down_msg)
	{
		SendDownFromPlatform(down_msg->type, down_msg->pid, down_msg->time);
		GAME_WORLD()->DeleteDownMsg(player_.pid());
	}
}

void CPlayerExercise::ProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case PLAYER_C2S_REQ_ContendPlatform:		__ContendPlatform(msg);			break;
	case PLAYER_C2S_REQ_TakePlatformExp:		__TakePlatformExp();			break;
	case PLAYER_C2S_REQ_TakeExpPill:			__TakeExpPill(msg);				break;
	case PLAYER_C2S_REQ_BuyContendTimes:		__BuyContendTimes();			break;
	case PLAYER_C2S_NTF_EnterExercisePlatform:	__EnterExercisePlatform();		break;
	case PLAYER_C2S_REQ_ClearExercisePlatformCD:__ClearExercisePlatformCD(msg);	break;
	}
}

void CPlayerExercise::SaveToDatabase()
{
	std::ostringstream sql;
	sql << "update tb_player_exercise set contend_times = " << contend_times_ <<
		", buy_contend = " << buy_contend_ <<
		", exp_pill_times = " << exp_pill_times_ <<
		", today_exercise_time = " << today_exercise_time_ <<
		", today_exercise_exp = " << today_exercise_exp_ <<
		", last_exercise_time = " << last_exercise_time_ <<
		", last_exercise_exp = " << last_exercise_exp_ <<
		", exp_box = " << exp_box_ <<
		", exp_pill_cd = " << last_exercise_time_ <<
		", contend_cd = " << last_exercise_exp_ <<
		"  where pid = " << player_.pid();
	MYSQL_UPDATE(sql.str());
}

void CPlayerExercise::SendDownFromPlatform(int type, int pid, time_t time)
{
	pto_PLAYER_S2C_NTF_DownFromPlatform pto_ntf;
	pto_ntf.set_type(type);
	pto_ntf.set_pid(pid);
	pto_ntf.set_time(time);

	player_.SendToAgent(pto_ntf, PLAYER_S2C_NTF_DownFromPlatform);
}

void CPlayerExercise::RoutineReset()
{
	contend_times_ = 5;
	buy_contend_ = 0;
	exp_pill_times_ = 10;

	last_exercise_time_ = today_exercise_time_;
	last_exercise_exp_ = today_exercise_exp_;

	today_exercise_time_ = 0;
	today_exercise_exp_ = 0;
}

void CPlayerExercise::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select contend_times,buy_contend,exp_pill_times,today_exercise_time,today_exercise_exp,last_exercise_time,last_exercise_exp,exp_box,exp_pill_cd,contend_cd from tb_player_exercise where pid = " << player_.pid();
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (result && result->next())
	{
		contend_times_ = result->getInt("contend_times");
		buy_contend_ = result->getInt("buy_contend");
		exp_pill_times_ = result->getInt("exp_pill_times");
		today_exercise_time_ = result->getInt("today_exercise_time");
		today_exercise_exp_ = result->getInt64("today_exercise_exp");
		last_exercise_time_ = result->getInt("last_exercise_time");
		last_exercise_exp_ = result->getInt64("last_exercise_exp");
		exp_box_ = result->getInt64("exp_box");
		exp_pill_cd_ = result->getInt64("exp_pill_cd");
		contend_cd_ = result->getInt64("contend_cd");
	}

	auto offline_exp = GAME_WORLD()->GetOfflineExp(player_.pid());
	if (offline_exp)
	{
		exp_box_ += offline_exp->exp_;
		today_exercise_time_ += offline_exp->exercise_time_;
		GAME_WORLD()->DeleteOfflineExp(player_.pid());
	}
}

void CPlayerExercise::__ContendPlatform(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ContendPlatform pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_ContendPlatform pto_res;
	pto_res.set_res(0);
	pto_res.set_platform_id(pto_req.platform_id());

	auto platform = GAME_WORLD()->GetExercisePlatform(pto_req.platform_id());

	if (!platform)
		pto_res.set_res(3);
	else if (platform->pid == player_.pid())
		pto_res.set_res(5);
	else if (time(0) - contend_cd_ < 600)
		pto_res.set_res(6);
	else if (time(0) - platform->start_time < ExercisePlatformProtectTime)
		pto_res.set_res(2);
	else if (contend_times_ <= 0)
		pto_res.set_res(1);
	else if (GAME_WORLD()->GetOfflinePlayerRank(player_.pid()) > 100 ||
		!GAME_WORLD()->GetOfflinePlayerRank(player_.pid()))
		pto_res.set_res(4);

	if (!pto_res.res())
	{
		contend_cd_ = time(0);
		pto_res.set_time(contend_cd_);
		contend_times_--;
		bool win{ false };
		if (!platform->pid)
			win = true;
		else
		{
			pto_OFFLINEBATTLE_S2C_NTF_ExercisePlatformBattle pto_battle;
			pto_OFFLINEBATTLE_Struct_PlayOfflineBattle* pto_play_battle = pto_battle.mutable_battle();
			pto_play_battle->set_type(4);
			win = COffline::ExercisePlatformBattle(&player_, platform->pid, pto_play_battle);
			player_.SendToAgent(pto_battle, OFFLINEBATTLE_S2C_NTF_ExercisePlatformBattle);
		}

		pto_res.set_win(win);

		if (win)
		{
			SPPlayer enemy = FIND_PLAYER(platform->pid);
			if (enemy)
				enemy->exercise()->SendDownFromPlatform(2, player_.pid(), time(0));
			else if (platform->pid)
				GAME_WORLD()->AddDownFromPlatform(platform->pid, 2, player_.pid(), time(0));

			pto_PLAYER_S2C_NTF_ChangePlatformOwner pto_ntf;
			pto_PLAYER_STRUCT_Platform* pto_platform = pto_ntf.mutable_platform();

			for (int i = 0; i < GAME_WORLD()->ExercisePlatformsSize(); i++)
			{
				SPExercisePlatform old_platform = GAME_WORLD()->GetExercisePlatform(i + 1);
				if (old_platform && old_platform->pid == player_.pid())
				{
					old_platform->pid = 0;
					old_platform->start_time = 0;

					pto_PLAYER_S2C_NTF_ChangePlatformOwner pto_ntf_old;
					pto_PLAYER_STRUCT_Platform* pto_platform_old = pto_ntf_old.mutable_platform();

					pto_platform_old->set_id(old_platform->id);
					pto_platform_old->set_pid(old_platform->pid);
					pto_platform_old->set_start_time(old_platform->start_time);
					pto_platform_old->set_guild_id(0);
					pto_platform_old->set_level(0);

					std::string str_pto_old;
					pto_ntf_old.SerializeToString(&str_pto_old);
					GAME_WORLD()->SendExercisePlatform(str_pto_old, MSG_S2C, PLAYER_S2C_NTF_ChangePlatformOwner);
				}
			}
			platform->pid = player_.pid();
			platform->start_time = time(0);

			pto_platform->set_id(platform->id);
			pto_platform->set_pid(platform->pid);
			pto_platform->set_start_time(platform->start_time);
			pto_platform->set_level(player_.level());
			pto_platform->set_guild_id(0);

			std::string str_pto;
			pto_ntf.SerializeToString(&str_pto);
			GAME_WORLD()->SendExercisePlatform(str_pto, MSG_S2C, PLAYER_S2C_NTF_ChangePlatformOwner);

			SendInitialMessage();

			if (platform->level == 1)
			{
				GAME_WORLD()->Annunciate(player_.name(), 0, 0, AnnunciateType::ExercisePlatform);
			}
		}
		else
		{
			SPPlayer enemy = GAME_WORLD()->FindPlayer(platform->pid);
			if (enemy)
				enemy->exercise()->SendDownFromPlatform(3, player_.pid(), time(0));
		}
	}
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ContendPlatform);
}

void CPlayerExercise::__TakePlatformExp()
{
	pto_PLAYER_S2C_RES_TakePlatformExp pto_res;
	pto_res.set_get_exp(0);
	if (exp_box_ <= 0)
		pto_res.set_res(1);
	else
	{
		pto_res.set_res(0);
		pto_res.set_get_exp(exp_box_);
		player_.ChangeExp(exp_box_);
		AddexerciseExp(exp_box_);
		exp_box_ = 0;
	}
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_TakePlatformExp);
}

void CPlayerExercise::__TakeExpPill(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_TakeExpPill pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_TakeExpPill pto_res;
	pto_res.set_res(0);
	pto_res.set_id(pto_req.id());
	pto_res.set_exp(0);
	pto_res.set_time(exp_pill_cd_);

	if (exp_pill_times_ <= 0)
		pto_res.set_res(1);
	else if (time(0) - exp_pill_cd_ < 600)
		pto_res.set_res(4);

	auto pill = GAME_WORLD()->GetExpPill(pto_req.id());
	if (!pill)
		pto_res.set_res(2);
	else if (pill->times <= 0)
		pto_res.set_res(2);
	else
	{
		for (auto it : pill->taken_player)
		{
			if (it == player_.pid())
				pto_res.set_res(3);
		}
	}
	if (!pto_res.res())
	{
		exp_pill_times_--;
		pill->times--;
		pill->taken_player.push_back(player_.pid());
		player_.ChangeExp(pill->exp);
		exp_pill_cd_ = time(0);
		pto_res.set_time(exp_pill_cd_);
		pto_res.set_exp(pill->exp);

		pto_PLAYER_S2C_NTF_UpdateExpPill pto_ntf_pill;
		pto_PLAYER_STRUCT_ExpPill* pto_pill = pto_ntf_pill.mutable_pill();
		pto_pill->set_id(pill->id);
		pto_pill->set_collect(false);

		for (size_t k = 0; k < pill->taken_player.size(); k++)
		{
			pto_pill->add_pid(pill->taken_player.at(k));
		}

		if (pill->times <= 0)
		{
			std::string str_pto_ntf;
			pto_ntf_pill.SerializeToString(&str_pto_ntf);
			GAME_WORLD()->SendExercisePlatform(str_pto_ntf, MSG_S2C, PLAYER_S2C_NTF_UpdateExpPill);
		}
		else
		{
			player_.SendToAgent(pto_ntf_pill, PLAYER_S2C_NTF_UpdateExpPill);
		}
		SendInitialMessage();
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_TakeExpPill);
}

void CPlayerExercise::__BuyContendTimes()
{
	pto_PLAYER_S2C_RES_BuyContendTimes pto_res;
	pto_res.set_res(0);

	if (buy_contend_ >= CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_BuyExercisePlatformTimes))
		pto_res.set_res(2);

	int gold = CGoldSpend::GetGoldSpend(buy_contend_ + 1, GoldSpendType::GST_BuyExercisePlatformSpend);

	if (player_.gold() < gold)
		pto_res.set_res(1);

	if (!pto_res.res())
	{
		player_.ChangeGold(-gold, 1);
		buy_contend_++;
		contend_times_++;
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_BuyContendTimes);
}

void CPlayerExercise::__EnterExercisePlatform()
{
	in_exercise_platform_ = true;
	SendInitialMessage();
}

void CPlayerExercise::__ClearExercisePlatformCD(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ClearExercisePlatformCD pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_ClearExercisePlatformCD pto_res;
	pto_res.set_res(0);
	pto_res.set_type(pto_req.type());

	int gold{ 0 };
	if (pto_req.type() == 1)
	{
		if (time(0) - contend_cd_ > 600)
			pto_res.set_res(2);
		else
		{
			time_t remain_time = 600 - (time(0) - contend_cd_);
			if (!(remain_time % 60))
				gold = int(remain_time / 60);
			else
				gold = int((remain_time - (remain_time % 60)) / 60 + 1);
			if (gold > player_.gold())
				pto_res.set_res(1);
		}

		if (!pto_res.res())
		{
			player_.ChangeGold(-gold, 2);
			contend_cd_ = 0;
		}
	}
	else if (pto_req.type() == 2)
	{
		if (time(0) - exp_pill_cd_ > 600)
			pto_res.set_res(2);
		else
		{
			time_t remain_time = 600 - (time(0) - exp_pill_cd_);
			if (!(remain_time % 60))
				gold = int(remain_time / 60);
			else
				gold = int((remain_time - (remain_time % 60)) / 60 + 1);
			if (gold > player_.gold())
				pto_res.set_res(1);
		}
		if (!pto_res.res())
		{
			player_.ChangeGold(-gold, 3);
			exp_pill_cd_ = 0;
		}
	}
	else
	{
		pto_res.set_res(2);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_ClearExercisePlatformCD);
}
