#include "stdafx.h"
#include "NeedReset.h"
#include "Player.h"
#include "HeroCard.h"
#include "Army.h"
#include "Mission.h"

CNeedReset::CNeedReset(CPlayer& player) :
player_{ player }
{
	__LoadFromDatabase();
}

CNeedReset::~CNeedReset()
{

}

void CNeedReset::ProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case PLAYER_C2S_REQ_RandCard:__TradeDraw();												break;
	case PLAYER_C2S_REQ_BuyRandCard:__TradeRefreshCards();									break;
	case PLAYER_C2S_REQ_GetPrayCard:__TradeGetOneCard(msg);									break;
	case PLAYER_C2S_REQ_AcceptRewardMission:__AcceptRewardMission(msg);						break;
	case PLAYER_C2S_REQ_CommitRewardMission:__CommitRewardMission(msg);						break;
	case PLAYER_C2S_REQ_BuyRewardTimes:__BuyRewardMissionTimes();							break;
	case PLAYER_C2S_REQ_AbandonRewardMission:__AbandonRewardMission(msg);					break;
	case PLAYER_C2S_REQ_RefreshRewardMission:__BuyRefreshRewardMission(msg);				break;
	case PLAYER_C2S_REQ_BuyStamina:__BuyStamina();											break;
	case PLAYER_C2S_REQ_UseInterior:__UseInternal(msg);										break;
	case PLAYER_C2S_REQ_InteriorLvUp:__InternalLvUp(msg);									break;
	case PLAYER_C2S_REQ_ClearInteriotCD:__ClearInternalCD(msg);								break;
	case PLAYER_C2S_REQ_UseLevy:__UseLevy(msg);												break;
	case PLAYER_C2S_REQ_UpgradeCastle:__UpgradeCastle(msg);									break;
	case STAGE_C2S_REQ_BuyStageTimes:__BuyStage(msg);										break;
	case PLAYER_C2S_REQ_MiniGameReward:__MiniGameReward(msg);								break;
	case PLAYER_C2S_REQ_VIPEveryday:__GetVIPEveryDay();										break;
	case PLAYER_C2S_REQ_GetTimeBox:__GetOnlineBox();								break;
	default:RECORD_WARNING(FormatString("未知的NeedReset协议号:", msg.GetProtocolID()));	break;
	}
}

void CNeedReset::SendOnlineBoxInfo()
{
	if (CTimeBox::GetTimeBox(online_box_award_))
	{
		pto_PLAYER_S2C_NTF_UptateTimeBox pto_box;
		pto_box.set_box_id(online_box_award_);
		player_.SendToAgent(pto_box, PLAYER_S2C_NTF_UptateTimeBox);
	}
}

void CNeedReset::__GetOnlineBox()
{
	const CTimeBox* online_box = CTimeBox::GetTimeBox(online_box_award_);
	if (!online_box)
		return;

	pto_PLAYER_S2C_RES_GetTimeBox pto;

	int result{ 0 };

	__int64 seconds{ static_cast<int>(GetDurationCount(time(0), online_box_start_time_, DURATION_TYPE::SECOND)) };

	if (online_box->GetTime() * 60 > seconds)
	{
		result = 1;
	}
	else
	{
		online_box_award_++;
		online_box_start_time_ = time(0);

		player_.ChangeSilver(online_box->GetSilver());
		player_.ChangeHonor(online_box->GetHonour());
	}

	pto.set_res(result);
	player_.SendToAgent(pto, PLAYER_S2C_RES_GetTimeBox);
}

void CNeedReset::WeekResetTrade()
{
	LOCK_BLOCK(trade_lock_);

	trade_hands_.clear();
	trade_is_got_reward_.reset();

	__SendTradeInfo();
}

void CNeedReset::SendInitialMessage()
{
	__SendTradeInfo();
	__SendRewardMissionInfo();
	__SendInternal();
}

void CNeedReset::SaveToDatabase()
{
	CSerializer<int> trade_cards_ser;
	for (auto &it : trade_cards_)
		trade_cards_ser.AddValue(it);

	CSerializer<int> trade_hands_ser;
	for (auto &it : trade_hands_)
		trade_hands_ser.AddValue(it);

	CSerializer<int> elite_already_ser;
	for (auto &it : elite_already_stages_)
		elite_already_ser.AddValue(it);

	CSerializer<int> multi_already_ser;
	for (auto &it : multi_already_stages_)
		multi_already_ser.AddValue(it);

	CSerializer<int> multi_buy_ser;
	for (auto &it : multi_buy_stages_)
		multi_buy_ser.AddPair(it.first, it.second);

	std::ostringstream sql;
	sql << "update tb_player_need_reset set snake_award_d = " << snake_award_ <<
		", puzzle_award_d = " << puzzle_award_ <<
		", buy_stamina_times_d = " << today_buy_stamina_times_ <<
		", trade_draw_times_d = " << today_draw_times_ <<
		", trade_got_award_w = " << trade_is_got_reward_.to_ulong() <<
		", trade_cards = '" << trade_cards_ser.SerializerToString().c_str() <<
		"',trade_hands = '" << trade_hands_ser.SerializerToString().c_str() <<
		"',rm_refresh = from_unixtime(" << mission_reward_refresh_time_ <<
		"), rm_times = " << mission_today_reward_complete_times_ <<
		", rm_buy_times = " << mission_today_reward_buy_times_ <<
		", elite_already_done ='" << elite_already_ser.SerializerToString().c_str() <<
		"',elite_buy_times = " << elite_buy_times_ <<
		" ,multi_already_times = '" << multi_already_ser.SerializerToString().c_str() <<
		"',multi_buy_times = '" << multi_buy_ser.SerializerPairToString().c_str() <<
		"',speed_times = " << speed_times_ <<
		" ,speed_today_best_time = " << speed_today_best_time_ <<
		" ,is_got_battle_reward = " << is_got_battle_reward_ <<
		" ,week_war_times = " << week_war_times_ <<
		" ,week_1v1_times = " << week_1v1_times_ <<
		" ,week_3v3_times = " << week_3v3_times_ <<
		" ,escort_times = " << escort_times_ <<
		" ,rob_times = " << rob_times_ <<
		" ,surplus_times = " << surplus_times_ <<
		" ,protect_times = " << protect_times_ <<
		" ,offline_buy_times = " << offline_buy_times_ <<
		" ,today_attendance = " << today_attendance_ <<
		" ,vip_every_day = " << vip_every_day_ <<
		" ,guild_contribute = " << today_guild_contribute_ <<
		" ,online_box_award = " << online_box_award_ <<
		" where pid = " << player_.pid();
	MYSQL_UPDATE(sql.str());

	__SaveRewardMission();
	__SaveInternal();
}

void CNeedReset::RoutineReset()
{
	online_box_award_ = 0;
	escort_times_ = 3;
	rob_times_ = 5;
	surplus_times_ = 10;
	protect_times_ = 0;
	offline_buy_times_ = 0;

	snake_award_ = 0;
	puzzle_award_ = 0;

	today_buy_stamina_times_ = 0;

	elite_buy_times_ = 0;
	elite_already_stages_.clear();

	multi_already_stages_.clear();
	multi_buy_stages_.clear();

	is_got_battle_reward_ = false;

	speed_times_ = 5;

	today_attendance_ = false;
	vip_every_day_ = false;

	{
		LOCK_BLOCK(trade_lock_);
		today_draw_times_ = 12;
		trade_cards_.clear();
	}

	mission_today_reward_complete_times_ = 0;
	mission_today_reward_buy_times_ = 0;

	levy_times_ = 5;
	gold_point_ = 0;

	internal_cells[0].times_ = 5;
	internal_cells[1].times_ = 5;
	internal_cells[2].times_ = 5;

	today_guild_contribute_ = 0;

	online_box_award_ = 1;

	player_.army()->ResetHeroInternal();
}

void CNeedReset::ImplementRewardMission(RewardMissionType type, int target_id, int num)
{
	bool bImplement{ false };

	pto_PLAYER_S2C_NTF_ImplementRewardMission pto;

	for (auto &it : reward_mission_)
	{
		if (RewardMissionState::RMS_Accept == it.state && type == it.type && target_id == it.target_id)
		{
			it.target_num += num;
			if (it.target_num > CRewardMission::GetRewardMissionTargetNum(it.type))
				it.target_num = CRewardMission::GetRewardMissionTargetNum(it.type);
			bImplement = true;
			pto_PLAYER_STRUCT_RewardMissionData* pData = pto.add_mission_data();
			pData->set_id(it.rmid - 1);
			pData->set_target_num(it.target_num);
		}
	}

	if (bImplement)
		player_.SendToAgent(pto, PLAYER_S2C_NTF_ImplementRewardMission);
}

void CNeedReset::RefreshRewardMission()
{
	mission_reward_refresh_time_ = time(0);

	for (auto &it : reward_mission_)
	{
		if (RMS_Accept != it.state)
		{
			it.target_num = 0;

			//if (CVIPFun::GetVIPFun(player_.vip_level(), VFT_GoldReward))
			//	it.rank = 4;
			//else
			it.rank = CRewardMission::ProduceRewardMissionRank();
			it.type = CRewardMission::ProduceRewardMissionType(player_.level(), player_.main_progress());

			it.target_id = 0;

			if (RMT_SweepStage == it.type)
				it.target_id = __ProduceRewardMissionStageLevel();

			it.state = RMS_New;
		}
	}
}

bool CNeedReset::IsMultiStageClear(int stage_level)
{
	auto it = multi_already_stages_.find(stage_level);
	if (it == multi_already_stages_.cend())
		return false;
	return true;
}

void CNeedReset::UpdateMultiStage()
{
	pto_STAGE_S2C_NTF_UpdateMultiStage pto_ntf;

	pto_ntf.set_progress(player_.mission()->stage_multi_progress());
	for (auto &it : multi_already_stages_)
		pto_ntf.add_already_done(it);
	for (auto &it : multi_buy_stages_)
	{
		pto_STAGE_STRUCT_MultiStageBuy* buy = pto_ntf.add_buy_times();
		buy->set_stage_id(it.first);
		buy->set_buy_times(it.second);
	}
	player_.SendToAgent(pto_ntf, STAGE_S2C_NTF_UpdateMultiStage);
}

void CNeedReset::UpdateEliteStage()
{
	pto_STAGE_S2C_NTF_UpdateEliteStage pto_elite_stage;
	pto_elite_stage.set_progress(player_.mission()->stage_elite_progress());
	pto_elite_stage.set_buy_times(elite_buy_times_);
	for (auto &it : elite_already_stages_)
		pto_elite_stage.add_already_done(it);
	player_.SendToAgent(pto_elite_stage, STAGE_S2C_NTF_UpdateEliteStage);
}

void CNeedReset::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select *,unix_timestamp(rm_refresh) as rrt from tb_player_need_reset where pid = " << player_.pid();
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (result && result->next())
	{
		today_buy_stamina_times_ = result->getInt("buy_stamina_times_d");
		today_draw_times_ = result->getInt("trade_draw_times_d");

		std::vector<int> temp_vct = CSerializer<int>::ParseFromString(result->getString("trade_cards").c_str());
		for (auto &it : temp_vct)
			trade_cards_.insert(it);

		temp_vct = CSerializer<int>::ParseFromString(result->getString("trade_hands").c_str());
		for (auto &it : temp_vct)
			trade_hands_.push_back(it);

		trade_is_got_reward_ = result->getInt("trade_got_award_w");
		snake_award_ = result->getInt("snake_award_d");
		puzzle_award_ = result->getInt("puzzle_award_d");

		mission_reward_refresh_time_ = result->getInt64("rrt");
		mission_today_reward_complete_times_ = result->getInt("rm_times");
		mission_today_reward_buy_times_ = result->getInt("rm_buy_times");

		temp_vct = CSerializer<int>::ParseFromString(result->getString("elite_already_done").c_str());
		for (auto &it : temp_vct)
			elite_already_stages_.insert(it);

		temp_vct = CSerializer<int>::ParseFromString(result->getString("multi_already_times").c_str());
		for (auto &it : temp_vct)
			multi_already_stages_.insert(it);

		auto temp_pair = CSerializer<int>::ParsePairFromString(result->getString("multi_buy_times").c_str());
		for (auto &it : multi_buy_stages_)
			multi_buy_stages_.insert(std::make_pair(it.first, it.second));

		speed_times_ = result->getInt("speed_times");
		speed_today_best_time_ = result->getInt("speed_today_best_time");

		is_got_battle_reward_ = result->getBoolean("is_got_battle_reward");

		week_war_times_ = result->getInt("week_war_times");
		week_1v1_times_ = result->getInt("week_1v1_times");
		week_3v3_times_ = result->getInt("week_3v3_times");

		escort_times_ = result->getInt("escort_times");
		rob_times_ = result->getInt("rob_times");
		surplus_times_ = result->getInt("surplus_times");
		protect_times_ = result->getInt("protect_times");
		offline_buy_times_ = result->getInt("offline_buy_times");

		today_attendance_ = result->getBoolean("today_attendance");
		vip_every_day_ = result->getBoolean("vip_every_day");

		today_guild_contribute_ = result->getInt("guild_contribute");
		online_box_award_ = result->getInt("online_box_award");
	}
	else
	{
		throw 3;
	}

	__LoadRewardMission();
	__LoadInternal();
}

void CNeedReset::__LoadInternal()
{
	std::ostringstream sql;
	sql.str("");
	sql << "select * from tb_player_internal where pid =" << player_.pid();

	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (result && result->next())
	{
		levy_times_ = result->getInt("levy_times");
		gold_point_ = result->getInt("gold_point");

		castle_upgrade_[kCastleUpgradeAtk] = result->getInt("castle_atk");
		castle_upgrade_[kCastleUpgradeMAtk] = result->getInt("castle_matk");
		castle_upgrade_[kCastleUpgradeHP] = result->getInt("castle_hp");

		internal_cells[kCrystalCave].hero_id_ = result->getInt("diggings_hero_id");
		internal_cells[kCrystalCave].last_use_ = result->getInt("diggings_last_use");
		internal_cells[kCrystalCave].level_ = result->getInt("diggings_level");
		internal_cells[kCrystalCave].times_ = result->getInt("diggings_times");

		internal_cells[kRathaus].hero_id_ = result->getInt("train_hero_id");
		internal_cells[kRathaus].last_use_ = result->getInt("train_last_use");
		internal_cells[kRathaus].level_ = result->getInt("train_level");
		internal_cells[kRathaus].times_ = result->getInt("train_times");

		internal_cells[kMilitaryHall].hero_id_ = result->getInt("guild_hero_id");
		internal_cells[kMilitaryHall].last_use_ = result->getInt("guild_last_use");
		internal_cells[kMilitaryHall].level_ = result->getInt("guild_level");
		internal_cells[kMilitaryHall].times_ = result->getInt("guild_times");
	}
	else
	{
		throw 6;
	}
}

void CNeedReset::__LoadRewardMission()
{
	std::ostringstream sql;
	sql.str("");
	sql << "select rmid,rank,type,target_id,state,target_num from tb_player_rewardMission where pid = " << player_.pid();
	ResultSetPtr result_reward{ MYSQL_QUERY(sql.str()) };
	if (!result_reward)
		throw 3;
	if (result_reward->rowsCount() == 0)
	{
		for (size_t i = 0; i < reward_mission_.size(); i++)
		{
			reward_mission_[i].rmid = i + 1;
			reward_mission_[i].rank = CRewardMission::ProduceRewardMissionRank();
			reward_mission_[i].type = CRewardMission::ProduceRewardMissionType(player_.level(), player_.main_progress());
			sql.str("");
			sql << "insert into tb_player_rewardMission values(" << player_.pid() << "," << reward_mission_[i].rmid << "," << reward_mission_[i].rank << "," << reward_mission_[i].type << "," << "default,default,default)";
			MYSQL_UPDATE(sql.str());
		}
	}
	else
	{
		int index{ 0 };

		while (result_reward->next())
		{
			reward_mission_[index].rmid = result_reward->getInt("rmid");
			reward_mission_[index].rank = result_reward->getInt("rank");
			reward_mission_[index].type = (RewardMissionType)result_reward->getInt("type");
			reward_mission_[index].target_id = result_reward->getInt("target_id");
			reward_mission_[index].target_num = result_reward->getInt("target_num");
			reward_mission_[index].state = (RewardMissionState)result_reward->getInt("state");
			index++;
		}
	}
}

void CNeedReset::__TradeDraw()
{
	if (!player_.IsOpenGuide(22))
		return;

	pto_PLAYER_S2C_RES_RandCard pto;

	int result{ 0 };

	LOCK_BLOCK(trade_lock_);

	if (today_draw_times_ <= 0)
	{
		result = 1;
	}
	else if (!trade_cards_.empty())
	{
		result = 2;
	}
	else
	{
		__TradeRandomCards();
		for (auto &it : trade_cards_)
			pto.add_rand_card(it);
	}

	pto.set_res(result);
	player_.SendToAgent(pto, PLAYER_S2C_RES_RandCard);
}

void CNeedReset::__TradeRefreshCards()
{
	if (!player_.IsOpenGuide(22))
		return;

	pto_PLAYER_S2C_RES_BuyRandCard pto;

	int result{ 0 };

	int need_gold{ 0 };

	if (!CVIPFun::GetVIPFun(player_.vip_level(), VFT_OpenRandCard))
	{
		result = 1;
	}
	else if ((need_gold = CGoldSpend::GetGoldSpend(GST_RandTradeCard)) > player_.gold())
	{
		result = 2;
	}
	else
	{
		player_.ChangeGold(-need_gold, 14);

		__TradeRandomCards();

		for (auto &it : trade_cards_)
			pto.add_rand_card(it);
	}

	pto.set_res(result);
	player_.SendToAgent(pto, PLAYER_S2C_RES_BuyRandCard);
}

void CNeedReset::__TradeGetOneCard(const CMessage& msg)
{
	if (!player_.IsOpenGuide(22))
		return;

	pto_PLAYER_C2S_REQ_GetPrayCard pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_GetPrayCard pto_res;
	pto_res.set_card_id(pto.card_id());
	pto_res.set_res(0);

	int result{ 0 };

	bool is_in_rand_card{ false };

	LOCK_BLOCK(trade_lock_);

	for (auto &it : trade_cards_)
	{
		if (pto.card_id() == it)
			is_in_rand_card = true;
	}

	if (false == is_in_rand_card)
		result = 1;

	const CTradeCard* card_model{ CTradeCard::GetTradeCard(pto.card_id()) };
	if (nullptr == card_model)
		result = 1;

	if (0 == result)
	{
		//手牌是不是满了
		if (trade_hands_.size() >= kMaxTradeHandsNum)
			trade_hands_.pop_front();
		trade_hands_.push_back(pto.card_id());

		today_draw_times_--;

		int reward = card_model->GetRewardBase() + (player_.level() * card_model->GetRewardMul());

		switch (card_model->GetRewardType())
		{
		case 1:player_.ChangeSilver(reward);		break;
		case 4:player_.ChangeHonor(reward);			break;
		case 5:player_.ChangeReputation(reward);	break;
		case 6:player_.ChangeStamina(reward);		break;
		}

		for (int i = 2; i >= 0; i--)
			__TradeGetReward(&pto_res, i);

		trade_cards_.clear();

		for (size_t i = 0; i < kMaxTradeHandsNum; i++)
		{
			if (i < trade_hands_.size())
				pto_res.add_own_card(trade_hands_[i]);
			else
				pto_res.add_own_card(0);
		}

		player_.ImplementMission(EMissionTargetType::MTT_Trade, 0, 0);
	}

	pto_res.set_res(result);
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_GetPrayCard);
}

void CNeedReset::__TradeRandomCards()
{
	LOCK_BLOCK(trade_lock_);
	trade_cards_.clear();
	while (trade_cards_.size() < kMaxTradeCardsNum)
		trade_cards_.insert(GetRandom(1, 12));
}

void CNeedReset::__TradeGetReward(pto_PLAYER_S2C_RES_GetPrayCard* pto, int level)
{
	if (!pto)
		return;

	if (static_cast<size_t>(level) >= trade_is_got_reward_.size())
	{
		RECORD_WARNING("祈福奖励等级错误");
		return;
	}

	int need_cards_num{ level + 2 };

	LOCK_BLOCK(trade_lock_);

	if (trade_hands_.size() < static_cast<size_t>(need_cards_num))
		return;

	size_t	hand_index{ trade_hands_.size() - 1 };
	size_t	card_reverse_start_index{ 0 };

	switch (level)
	{
	case 0:card_reverse_start_index = 1; break;
	case 1:card_reverse_start_index = 4; break;
	case 2:card_reverse_start_index = 8; break;
	}

	bool is_reward{ true };

	for (int i = 0; i < need_cards_num; i++)
	{
		if (trade_hands_[hand_index--] != GAME_WORLD()->GetTradeCard(card_reverse_start_index--))
		{
			is_reward = false;
			break;
		}
	}

	if (is_reward && false == trade_is_got_reward_[level])
	{
		for (int i = 0; i < need_cards_num; i++)
			trade_hands_.pop_back();

		GAME_WORLD()->Annunciate(player_.name(), level, 0, AnnunciateType::WinTrade);

		player_.ChangeSilver(GAME_WORLD()->GetTradeReward(level));

		trade_is_got_reward_[level] = true;

		pto->set_is_reward(true);
		pto->set_reward_id(level);
	}
}

void CNeedReset::__SaveRewardMission()
{
	std::ostringstream sql;
	for (auto &it : reward_mission_)
	{
		sql.str("");
		sql << "update tb_player_rewardMission set rank = " << it.rank
			<< ", type = " << it.type
			<< ", target_id = " << it.target_id
			<< ", target_num = " << it.target_num
			<< ", state = " << it.state
			<< " where pid = " << player_.pid() << " and rmid = " << it.rmid;
		MYSQL_UPDATE(sql.str());
	}
}

void CNeedReset::__SaveInternal()
{
	std::ostringstream sql;
	sql << "update tb_player_internal set levy_times = " << levy_times_
		<< ", gold_point =" << gold_point_
		<< ", diggings_times = " << internal_cells[kCrystalCave].times_
		<< ", diggings_last_use = " << internal_cells[kCrystalCave].last_use_
		<< ", diggings_hero_id = " << internal_cells[kCrystalCave].hero_id_
		<< ", diggings_level = " << internal_cells[kCrystalCave].level_
		<< ", train_times = " << internal_cells[kRathaus].times_
		<< ", train_last_use = " << internal_cells[kRathaus].last_use_
		<< ", train_hero_id = " << internal_cells[kRathaus].hero_id_
		<< ", train_level = " << internal_cells[kRathaus].level_
		<< ", guild_times = " << internal_cells[kMilitaryHall].times_
		<< ", guild_last_use = " << internal_cells[kMilitaryHall].last_use_
		<< ", guild_hero_id = " << internal_cells[kMilitaryHall].hero_id_
		<< ", guild_level = " << internal_cells[kMilitaryHall].level_
		<< ", castle_atk = " << castle_upgrade_[kCastleUpgradeAtk]
		<< ", castle_matk = " << castle_upgrade_[kCastleUpgradeMAtk]
		<< ", castle_hp = " << castle_upgrade_[kCastleUpgradeHP]
		<< " where pid = " << player_.pid();

	MYSQL_UPDATE(sql.str());
}

void CNeedReset::__SendTradeInfo()
{
	pto_PLAYER_S2C_NTF_UpdateTrade pto;

	for (size_t i = 0; i < CGameWorld::kTradeCardsNum; i++)
		pto.add_target_card(GAME_WORLD()->GetTradeCard(i));
	for (size_t i = 0; i < CGameWorld::kTradeRewardsNum; i++)
		pto.add_reward(GAME_WORLD()->GetTradeReward(i));
	for (auto &it : trade_cards_)
		pto.add_rand_card(it);

	for (size_t i = 0; i < kMaxTradeHandsNum; i++)
	{
		if (i < trade_hands_.size())
			pto.add_own_card(trade_hands_[i]);
		else
			pto.add_own_card(0);
	}

	pto.add_get_reward(trade_is_got_reward_[0]);
	pto.add_get_reward(trade_is_got_reward_[1]);
	pto.add_get_reward(trade_is_got_reward_[2]);

	pto.set_use_times(today_draw_times_);

	player_.SendToAgent(pto, PLAYER_S2C_NTF_UpdataTrade);
}

void CNeedReset::__SendInternal()
{
	pto_PLAYER_S2C_NTF_UpdateInterior pto_internal;
	pto_PLAYER_Struck_Interiot_Cell_Model* ptoProduce = pto_internal.add_interiot_cells();
	ptoProduce->set_type((size_t)InternalBuilding::kCrystalCave);
	ptoProduce->set_cell_lv(internal_cells[(size_t)InternalBuilding::kCrystalCave].level_);
	ptoProduce->set_time(internal_cells[(size_t)InternalBuilding::kCrystalCave].last_use_);
	ptoProduce->set_use_times(internal_cells[(size_t)InternalBuilding::kCrystalCave].times_);
	ptoProduce->set_hero_id(internal_cells[(size_t)InternalBuilding::kCrystalCave].hero_id_);
	pto_PLAYER_Struck_Interiot_Cell_Model* ptoTrain = pto_internal.add_interiot_cells();
	ptoTrain->set_type((size_t)InternalBuilding::kRathaus);
	ptoTrain->set_cell_lv(internal_cells[(size_t)InternalBuilding::kRathaus].level_);
	ptoTrain->set_time(internal_cells[(size_t)InternalBuilding::kRathaus].last_use_);
	ptoTrain->set_use_times(internal_cells[(size_t)InternalBuilding::kRathaus].times_);
	ptoTrain->set_hero_id(internal_cells[(size_t)InternalBuilding::kRathaus].hero_id_);
	pto_PLAYER_Struck_Interiot_Cell_Model* ptoSearch = pto_internal.add_interiot_cells();
	ptoSearch->set_type((size_t)InternalBuilding::kMilitaryHall);
	ptoSearch->set_cell_lv(internal_cells[(size_t)InternalBuilding::kMilitaryHall].level_);
	ptoSearch->set_time(internal_cells[(size_t)InternalBuilding::kMilitaryHall].last_use_);
	ptoSearch->set_use_times(internal_cells[(size_t)InternalBuilding::kMilitaryHall].times_);
	ptoSearch->set_hero_id(internal_cells[(size_t)InternalBuilding::kMilitaryHall].hero_id_);
	pto_internal.set_levy_times(levy_times_);
	pto_internal.set_use_gold_pointing(gold_point_);
	pto_internal.add_castle_level(castle_upgrade_[CastleUpgrade::kCastleUpgradeAtk]);
	pto_internal.add_castle_level(castle_upgrade_[CastleUpgrade::kCastleUpgradeMAtk]);
	pto_internal.add_castle_level(castle_upgrade_[CastleUpgrade::kCastleUpgradeHP]);
	player_.SendToAgent(pto_internal, PLAYER_S2C_NTF_UpdataInterior);
}

void CNeedReset::__SendRewardMissionInfo()
{
	//if (time(0) - mission_reward_refresh_time_ >= 1800)
	//	RefreshRewardMission();
	pto_PLAYER_S2C_NTF_UpdateRewardMission pto;
	pto.set_refresh_time(mission_reward_refresh_time_);
	pto.set_complete_times(mission_today_reward_complete_times_);
	pto.set_buy_times(mission_today_reward_buy_times_);
	for (auto &it : reward_mission_)
	{
		pto_PLAYER_STRUCT_RewardMission* reward_mission = pto.add_reward_missions();
		reward_mission->set_type(it.type);
		reward_mission->set_rank(it.rank);
		reward_mission->set_target_id(it.target_id);
		reward_mission->set_target_num(it.target_num);
		reward_mission->set_state(it.state);
	}

	player_.SendToAgent(pto, PLAYER_S2C_NTF_UpdateRewardMission);
}

void CNeedReset::__AcceptRewardMission(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_AcceptRewardMission pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_AcceptRewardMission pto_res;
	pto_res.set_id(pto.id());

	if (pto.id() >= 5 || pto.id() < 0)
	{
		pto_res.set_res(1);
	}
	else if (RMS_New != reward_mission_[pto.id()].state)
	{
		pto_res.set_res(2);
	}
	else
	{
		pto_res.set_res(0);
		reward_mission_[pto.id()].state = RMS_Accept;
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_AcceptRewardMission);
}

void CNeedReset::__CommitRewardMission(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_CommitRewardMission pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_CommitRewardMission pto_res;
	pto_res.set_id(pto.id());
	pto_res.set_res(0);

	TRewardMission* pMission{ nullptr };

	if (pto.id() < 0 || pto.id() > 4)
	{
		pto_res.set_res(1);
		player_.SendToAgent(pto_res, PLAYER_S2C_RES_CommitRewardMission);
		return;
	}
	else
	{
		pMission = &reward_mission_[pto.id()];
	}

	if (10 + mission_today_reward_buy_times_ <= mission_today_reward_complete_times_)
	{
		pto_res.set_res(4);
	}

	else if (RMS_Accept != pMission->state)
	{
		pto_res.set_res(1);
	}
	else if (0 == pto.type())
	{
		if (player_.gold() < 10)
		{
			pto_res.set_res(3);
		}

		else
		{
			pto_res.set_res(0);
			pMission->state = RMS_Complete;
			mission_today_reward_complete_times_++;
			player_.ChangeGold(-10, 15);
		}
	}
	else if (1 == pto.type())
	{
		int nTargetNeedNum = CRewardMission::GetRewardMissionTargetNum(pMission->type);
		if (nTargetNeedNum > pMission->target_num)
		{
			pto_res.set_res(2);
		}
		else
		{
			pto_res.set_res(0);
			pMission->state = RMS_Complete;
			mission_today_reward_complete_times_++;
		}
	}

	if (!pto_res.res())
	{
		__GetRewardMissionResource(pMission);
		player_.ImplementMission(EMissionTargetType::MTT_RewardMission, 0, 0);
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_CommitRewardMission);
}

void CNeedReset::__BuyRewardMissionTimes()
{
	pto_PLAYER_S2C_RES_BuyRewardMissionTimes pto;

	int need_gold{ CGoldSpend::GetGoldSpend(mission_today_reward_buy_times_ + 1, GST_BuyRewardMissionSpend) };

	if (mission_today_reward_buy_times_ >= CVIPFun::GetVIPFun(player_.vip_level(), VFT_BuyRewardTimes))
	{
		pto.set_res(1);
	}
	else if (need_gold > player_.gold())
	{
		pto.set_res(2);
	}
	else
	{
		pto.set_res(0);
		player_.ChangeGold(-need_gold, 16);
		mission_today_reward_buy_times_++;
	}

	player_.SendToAgent(pto, PLAYER_S2C_RES_BuyRewardTimes);
}

void CNeedReset::__AbandonRewardMission(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_AbandonRewardMission pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_AbandonRewardMission pto_res;
	pto_res.set_id(pto.id());
	pto_res.set_res(0);

	if (0 > pto.id() || 4 < pto.id())
	{
		pto_res.set_res(1);
	}
	else
	{
		TRewardMission* reward_mission = &reward_mission_[pto.id()];
		if (RMS_New == reward_mission->state || RMS_Complete == reward_mission->state)
		{
			pto_res.set_res(1);
		}
		else
		{
			reward_mission->state = RMS_New;
			reward_mission->target_num = 0;
		}
	}

	player_.SendToAgent(pto_res, PLAYER_S2C_RES_AbandonRewardMission);
}

void CNeedReset::__BuyRefreshRewardMission(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_RefreshRewardMission pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_RefreshRewardMission pto_res;

	int need_gold{ 0 };

	int remain_time = int(1800 - (time(0) - mission_reward_refresh_time_));

	need_gold = (remain_time - (remain_time % 60)) / 60 + 1;

	bool in_cd{ false };
	if (remain_time > 0)
		in_cd = true;

	if (player_.gold() < need_gold && pto_req.type() && in_cd)
	{
		pto_res.set_res(1);
	}
	else if (in_cd && !pto_req.type())
		pto_res.set_res(2);
	else
	{
		pto_res.set_res(0);
		if (pto_req.type() && in_cd)
			player_.ChangeGold(-need_gold, 17);
	}

	if (!pto_res.res())
	{
		RefreshRewardMission();

		for (auto &it : reward_mission_)
		{
			pto_PLAYER_STRUCT_RewardMission* reward_mission = pto_res.add_reward_missions();
			reward_mission->set_type(it.type);
			reward_mission->set_rank(it.rank);
			reward_mission->set_target_id(it.target_id);
			reward_mission->set_target_num(it.target_num);
			reward_mission->set_state(it.state);
		}
	}
	pto_res.set_time(mission_reward_refresh_time_);
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_RefreshRewardMission);
}

void CNeedReset::AutoRefreshRewardMission()
{
	if (time(0) - mission_reward_refresh_time_ < 18)
		return;

	RefreshRewardMission();
	pto_PLAYER_S2C_RES_RefreshRewardMission pto_res;
	pto_res.set_res(0);
	for (auto &it : reward_mission_)
	{
		pto_PLAYER_STRUCT_RewardMission* reward_mission = pto_res.add_reward_missions();
		reward_mission->set_type(it.type);
		reward_mission->set_rank(it.rank);
		reward_mission->set_target_id(it.target_id);
		reward_mission->set_target_num(it.target_num);
		reward_mission->set_state(it.state);
	}
}

void CNeedReset::__GetRewardMissionResource(const TRewardMission* reward_mission)
{
	const SRewardMission* pMissionStruct = CRewardMission::GetRewardMissionStruce(reward_mission->type);
	if (nullptr == pMissionStruct)
		return;

	//int nFixExp = CRootData::GetRewardMissionResource(player_.level(), PRT_Exp);
	__int64 nFixExp = CRootData::GetRootData(player_.level(), RDT_MissionExp);
	__int64 nResultExp = __int64(nFixExp * CRewardMission::GetRankMul(reward_mission->rank));
	player_.ChangeExp(nResultExp);

	int nFixValue = CRewardMission::GetFixedValue(pMissionStruct->m_nRewardType);

	if (!nFixValue)
		nFixValue = CRootData::GetRewardMissionResource(player_.level(), pMissionStruct->m_nRewardType);

	int nResult = int(nFixValue * CRewardMission::GetRankMul(reward_mission->rank) * pMissionStruct->m_fRewardMul);

	switch (pMissionStruct->m_nRewardType)
	{
	case 1:
		player_.ChangeSilver(nResult);
		break;
	case 2:
		player_.ChangeGold(nResult, 18);
		break;
	case 3:
		player_.ChangeExp(nResult);
		break;
	case 4:
		player_.ChangeHonor(nResult);
		break;
	case 5:
		player_.ChangeReputation(nResult);
		break;
	case 6:
		player_.ChangeStamina(nResult);
		break;
	}
}

int  CNeedReset::__ProduceRewardMissionStageLevel()
{
	int stage_progress{ player_.mission()->stage_normal_progress() };

	if (!stage_progress)
		return 0;

	int nMinStageLevel{ 1 };

	if (stage_progress >= 6)
		nMinStageLevel = stage_progress - 5;

	return GetRandom(nMinStageLevel, stage_progress);
}

void CNeedReset::__BuyStamina()
{
	pto_PLYAER_S2C_RES_BuyStamina pto;

	int max_times{ CVIPFun::GetVIPFun(player_.vip_level(), VFT_BuyStaminaTimes) };
	int need_gold{ CGoldSpend::GetGoldSpend(today_buy_stamina_times_ + 1, GST_BuyStamina) };

	int result{ 0 };

	if (player_.stamina() >= CPlayer::kMaxStamina)
	{
		result = 2;
	}
	else if (today_buy_stamina_times_ >= max_times)
	{
		result = 3;
	}
	else if (player_.gold() < need_gold)
	{
		result = 1;
	}
	else
	{
		player_.ChangeStamina(25);
		player_.ChangeGold(-need_gold, 19);
		today_buy_stamina_times_++;
	}

	pto.set_stamina(player_.stamina());
	pto.set_res(result);

	player_.SendToAgent(pto, PLAYER_S2C_RES_BuyStamina);
}

void CNeedReset::__UseInternal(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_UseInterior ptoC2S;
	ptoC2S.ParseFromArray(msg.body_c_str(), msg.body_size());

	int nRes{ 0 };
	int nResource{ 0 };

	pto_PLAYER_S2C_RES_UseInterior ptoS2C;

	auto it_hero = player_.army()->FindHero(ptoC2S.hero_id());
	if (it_hero && it_hero->today_is_internal_affair())
		nRes = 4;
	else
	{
		switch (ptoC2S.type())
		{
		case 0:
			__UseInternalProduce(it_hero, nRes, nResource, &ptoS2C);
			break;
		case 1:
			__UseInternalTrain(it_hero, nRes, nResource, &ptoS2C);
			break;
		case 2:
			__UseInternalSearch(it_hero, nRes, nResource, &ptoS2C);
			break;
		default:
			nRes = 2;
			break;
		}
	}

	ptoS2C.set_res(nRes);
	ptoS2C.set_resources(nResource);

	player_.SendToAgent(ptoS2C, PLAYER_S2C_RES_UseInterior);
}

void CNeedReset::__InternalLvUp(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_InteriorLvUp ptoC2S;
	ptoC2S.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (0 > ptoC2S.type() || 2 < ptoC2S.type())
		return;

	pto_PLAYER_S2C_RES_InteriorLvUp pto_s2c;
	pto_s2c.set_type(ptoC2S.type());
	pto_s2c.set_res(0);

	InternalBuilding type = (InternalBuilding)ptoC2S.type();
	int nResource = 0;
	const SUpgradeCost* pUpgradeCost = CInteriorLib::GetUpgradeCost(internal_cells[ptoC2S.type()].level_ + 1);

	if (nullptr == pUpgradeCost)
		pto_s2c.set_res(1);

	else if (4 <= internal_cells[ptoC2S.type()].level_)
		pto_s2c.set_res(1);


	else if (0 == ptoC2S.buy_type())
	{
		nResource = pUpgradeCost->m_nGold;
		if (1 > player_.vip_level())
			pto_s2c.set_res(3);

		else if (nResource > player_.gold())
			pto_s2c.set_res(2);

		if (!pto_s2c.res())
		{
			internal_cells[ptoC2S.type()].level_++;
			player_.ChangeGold(-nResource,20);
		}
	}

	else if (1 == ptoC2S.buy_type())
	{
		nResource = pUpgradeCost->m_nSilver;
		if (player_.level() < pUpgradeCost->m_nPlayerLevel)
			pto_s2c.set_res(4);

		else if (nResource > player_.silver())
			pto_s2c.set_res(2);

		if (!pto_s2c.res())
		{
			internal_cells[ptoC2S.type()].level_++;
			player_.ChangeSilver(-nResource);
		}
	}

	player_.SendToAgent(pto_s2c, PLAYER_S2C_RES_InteriorLvUp);
}

void CNeedReset::__ClearInternalCD(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_ClearInteriotCD ptoREQ;
	ptoREQ.ParseFromArray(msg.body_c_str(), msg.body_size());

	InternalCell* pCell{ nullptr };

	switch (ptoREQ.type())
	{
	case 0:
		pCell = &internal_cells[0];
		break;
	case 1:
		pCell = &internal_cells[1];
		break;
	case 2:
		pCell = &internal_cells[2];
		break;
	default:
		return;
	}

	pto_PLAYER_S2C_RES_ClearInteriotCD ptoRES;
	ptoRES.set_type(ptoREQ.type());

	time_t nTime = time(0) - pCell->last_use_;

	int nCD = (int)(600 - nTime);
	int nGold = ((nCD - 1) / 60 + 1);

	if (0 > nTime || nCD <= 0)
	{
		ptoRES.set_res(2);
	}
	else if (player_.gold() < nGold)
	{
		ptoRES.set_res(1);
	}
	else
	{
		ptoRES.set_res(0);
		player_.ChangeGold(-nGold,21);
		pCell->last_use_ = 0;
	}

	player_.SendToAgent(ptoRES, PLAYER_S2C_RES_ClearInteriotCD);
}

void CNeedReset::__UseLevy(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_UseLevy ptoREQ;
	ptoREQ.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (0 > ptoREQ.type() || 2 < ptoREQ.type())
		return;

	if (0 == ptoREQ.type() && !player_.IsOpenGuide(13))
		return;

	else if (!player_.IsOpenGuide(34))
		return;

	pto_PLAYER_S2C_RES_UseLevy ptoRES;
	ptoRES.set_res(0);
	ptoRES.set_type(ptoREQ.type());

	int nGold = 0;
	int nTimes = 1;

	if (0 == ptoREQ.type() && 0 >= levy_times_)
		ptoRES.set_res(1);

	if (1 == ptoREQ.type())
	{
		nGold = CGoldSpend::GetGoldSpend(gold_point_ + 1, GST_GoldPointingSpend);

		if (gold_point_ >= CVIPFun::GetVIPFun(player_.vip_level(), VFT_GoldPointing))
			ptoRES.set_res(3);

		if (player_.gold() < nGold)
			ptoRES.set_res(2);
	}
	else if (2 == ptoREQ.type())
	{
		nTimes = CVIPFun::GetVIPFun(player_.vip_level(), VFT_GoldPointing) - gold_point_;
		if (0 >= nTimes)
		{
			ptoRES.set_res(3);
		}
		else
		{
			if (nTimes > 10)
				nTimes = 10;

			for (int i = 1; i <= nTimes; i++)
				nGold += CGoldSpend::GetGoldSpend(gold_point_ + i, GST_GoldPointingSpend);

			if (nGold > player_.gold())
				ptoRES.set_res(2);
		}
	}

	if (0 == ptoRES.res())
	{
		__int64 nSilver = 0;
		__int64 single_silver{ 0 };
		const CRootData* data = CRootData::GetRootData(player_.level());
		if (data)
		{
			if (0 == ptoREQ.type())
			{
				single_silver += data->levy_stage() * player_.mission()->stage_normal_progress();
				single_silver += data->levy_stage() * player_.mission()->stage_hard_progress();
				single_silver += data->levy_stage() * player_.mission()->stage_nightmare_progress();
			}
			else
			{
				single_silver = data->gold_stone_silver();
			}

			for (int i = 1; i <= nTimes; i++)
			{
				nSilver += single_silver;
			}
		}

		player_.ChangeSilver(nSilver);
		player_.ChangeGold(-nGold,22);

		switch (ptoREQ.type())
		{
		case 0:
			levy_times_--;
			player_.ImplementMission(EMissionTargetType::MTT_UseLevy, 0, 0);
			break;
		case 1:
			gold_point_++;
			break;
		case 2:
			gold_point_ += nTimes;
			break;
		default:
			break;
		}
		ptoRES.set_times(nTimes);
	}

	player_.SendToAgent(ptoRES, PLAYER_S2C_RES_UseLevy);
}

void CNeedReset::__UpgradeCastle(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_UpgradeCastle pto_c2s;
	pto_c2s.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (pto_c2s.type() < 0 || pto_c2s.type() > 2)
		return;

	pto_PLAYER_S2C_RES_UpgradeCastle pto_s2c;
	pto_s2c.set_res(0);

	CastleUpgrade type = (CastleUpgrade)pto_c2s.type();
	pto_s2c.set_type(type);

	if (castle_upgrade_[type] >= player_.level())
		pto_s2c.set_res(2);

	const CRootData* data = CRootData::GetRootData(castle_upgrade_[type]);
	long long silver = data->castle_update_silver();

	if (player_.silver() < silver)
		pto_s2c.set_res(1);

	if (!pto_s2c.res())
	{
		castle_upgrade_[type]++;
		player_.ChangeSilver(-silver);
	}

	player_.SendToAgent(pto_s2c, PLAYER_S2C_RES_UpgradeCastle);
}

void CNeedReset::__UseInternalProduce(CHeroCard* hero_card, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto)
{
	if (!player_.IsOpenGuide(8))
	{
		res = 8;
		return;
	}

	time_t nNowTime;
	time(&nNowTime);

	if (600 >= (nNowTime - internal_cells[0].last_use_) && (nNowTime - internal_cells[0].last_use_) > 0 && !CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_RelievedInteriorCD))
	{
		res = 3;
	}
	else if (0 >= internal_cells[0].times_)
	{
		res = 6;
	}
	else
	{
		if (!CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_RelievedInteriorCD))
			internal_cells[0].last_use_ = nNowTime;
		internal_cells[0].times_--;

		pto_PLAYER_Struck_Interiot_Cell_Model *pCell = pto->mutable_cell();
		pCell->set_cell_lv(internal_cells[0].level_);
		pCell->set_time(internal_cells[0].last_use_);
		pCell->set_use_times(internal_cells[0].times_);
		pCell->set_type(0);

		if (hero_card)
		{
			internal_cells[0].hero_id_ = hero_card->hero_id();
			hero_card->set_today_is_internal_affair(true);
		}

		else
			internal_cells[0].hero_id_ = 0;

		resource = __GetProduceResource(hero_card);


		pCell->set_hero_id(internal_cells[0].hero_id_);

		player_.ImplementMission(EMissionTargetType::MTT_UseProduceCell, 0, 0);


		pto->set_event_id(0);
		const CInteriorEvent* pEvent = CInteriorEvent::RandomProduceEvents();
		if (nullptr != pEvent && hero_card)
		{
			pto->set_event_id(pEvent->GetID());
			int nResult = pEvent->GetRewardBase() + (pEvent->GetRewardAddition() * player_.level());
			pto->set_reward_type(pEvent->GetRewardType());
			pto->set_event_reward(nResult);

			switch (pto->reward_type())
			{
			case 1:
				player_.ChangeSilver(nResult);
				break;
			case 2:
				player_.ChangeGold(nResult,23);
				break;
			case 3:
				player_.ChangeHonor(nResult);
				break;
			case 4:
				player_.ChangeReputation(nResult);
				break;
			default:
				break;
			}
		}
	}
}

void CNeedReset::__UseInternalTrain(CHeroCard* hero_card, int& res, int& resource, pto_PLAYER_S2C_RES_UseInterior* pto)
{
	time_t nNowTime;
	time(&nNowTime);

	if (600 >= (nNowTime - internal_cells[1].last_use_) && (nNowTime - internal_cells[1].last_use_) > 0 && !CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_RelievedInteriorCD))
		res = 3;
	else if (0 >= internal_cells[1].times_)
		res = 6;
	else
	{
		if (!CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_RelievedInteriorCD))
			internal_cells[1].last_use_ = nNowTime;
		internal_cells[1].times_--;

		pto_PLAYER_Struck_Interiot_Cell_Model *pCell = pto->mutable_cell();
		pCell->set_cell_lv(internal_cells[1].level_);
		pCell->set_time(internal_cells[1].last_use_);
		pCell->set_use_times(internal_cells[1].times_);
		pCell->set_type(1);

		if (hero_card)
		{
			internal_cells[1].hero_id_ = hero_card->hero_id();
			hero_card->set_today_is_internal_affair(true);
		}
		else
			internal_cells[1].hero_id_ = 0;

		resource = __GetTrainResource(hero_card);

		pCell->set_hero_id(internal_cells[1].hero_id_);

		player_.ImplementMission(EMissionTargetType::MTT_Security, 0, 0);

		pto->set_event_id(0);
		const CInteriorEvent* pEvent = CInteriorEvent::RandomTrainEvents();
		if (nullptr != pEvent && hero_card)
		{
			pto->set_event_id(pEvent->GetID());
			int nResult = pEvent->GetRewardBase() + (pEvent->GetRewardAddition() * player_.level());
			pto->set_reward_type(pEvent->GetRewardType());
			pto->set_event_reward(nResult);

			switch (pto->reward_type())
			{
			case 1:
				player_.ChangeSilver(nResult);
				break;
			case 2:
				player_.ChangeGold(nResult,24);
				break;
			case 3:
				player_.ChangeHonor(nResult);
				break;
			case 4:
				player_.ChangeReputation(nResult);
				break;
			default:
				break;
			}
		}
	}
}

void CNeedReset::__UseInternalSearch(CHeroCard* hero_card, int& nRes, int& nResource, pto_PLAYER_S2C_RES_UseInterior* pto)
{
	if (!player_.IsOpenGuide(36))
	{
		nRes = 8;
		return;
	}

	time_t nNowTime;
	time(&nNowTime);

	if (600 >= (nNowTime - internal_cells[2].last_use_) && (nNowTime - internal_cells[2].last_use_) > 0 && !CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_RelievedInteriorCD))
		nRes = 3;
	else if (0 >= internal_cells[2].times_)
		nRes = 6;
	else
	{
		if (!CVIPFun::GetVIPFun(player_.vip_level(), VIPFunType::VFT_RelievedInteriorCD))
			internal_cells[2].last_use_ = nNowTime;
		internal_cells[2].times_--;

		pto_PLAYER_Struck_Interiot_Cell_Model *pCell = pto->mutable_cell();
		pCell->set_cell_lv(internal_cells[2].level_);
		pCell->set_time(internal_cells[2].last_use_);
		pCell->set_use_times(internal_cells[2].times_);
		pCell->set_type(2);

		if (hero_card)
		{
			hero_card->set_today_is_internal_affair(true);
			internal_cells[2].hero_id_ = hero_card->hero_id();
		}
		else
			internal_cells[2].hero_id_ = 0;

		nResource = __GetSearchResource(hero_card);

		pCell->set_hero_id(internal_cells[2].hero_id_);

		player_.ImplementMission(EMissionTargetType::MTT_UseSearchCell, 0, 0);

		pto->set_event_id(0);
		const CInteriorEvent* pEvent = CInteriorEvent::RandomSearchEvents();
		if (nullptr != pEvent && hero_card)
		{
			pto->set_event_id(pEvent->GetID());
			int nResult = pEvent->GetRewardBase() + (pEvent->GetRewardAddition() * player_.level());
			pto->set_reward_type(pEvent->GetRewardType());
			pto->set_event_reward(nResult);

			switch (pto->reward_type())
			{
			case 1:
				player_.ChangeSilver(nResult);
				break;
			case 2:
				player_.ChangeGold(nResult,25);
				break;
			case 3:
				player_.ChangeHonor(nResult);
				break;
			case 4:
				player_.ChangeReputation(nResult);
				break;
			default:
				break;
			}
		}
	}
}

int CNeedReset::__GetProduceResource(CHeroCard* hero_card)
{
	int nResult{ 0 };

	const CInteriorLib* pInteriorLib = CInteriorLib::GetInteriotLib(player_.level());
	float fAddition{ 0 };

	switch (internal_cells[0].level_)
	{
	case 1:
		fAddition = pInteriorLib->GetFarmAddition();
		nResult = pInteriorLib->GetFarmMoney();
		break;
	case 2:
		fAddition = pInteriorLib->GetHuntAddition();
		nResult = pInteriorLib->GetHuntMoney();
		break;
	case 3:
		fAddition = pInteriorLib->GetSilverAddition();
		nResult = pInteriorLib->GetSilverMoney();
		break;
	case 4:
		fAddition = pInteriorLib->GetCrystalAddition();
		nResult = pInteriorLib->GetCrystalMoney();
		break;
	default:
		break;
	}

	if (hero_card)
	{
		int hero_silver = (int)((hero_card->hero_template()->GetStr() + hero_card->train_str()) * fAddition);

		for (int i = 0; i < hero_card->hero_template()->GetPassSkillSize(); i++)
		{
			int a = hero_card->hero_template()->GetPassSkill(i);
			const CPassSkill* pSkill = CPassSkill::GetPassSkill(a);
			if (nullptr != pSkill && hero_card->hero_level() >= CHeroTP::GetPassSkillOpenLevel(i))
			{
				if (dat_SKILL_ENUM_ProductionType::ProductionType_Produce == pSkill->GetProductionType())
				{
					hero_silver = (int)(hero_silver * (1 + pSkill->GetProductionValue()));
				}
			}
		}
		nResult += hero_silver;
	}
	player_.ChangeSilver(nResult);

	return nResult;
}

int CNeedReset::__GetTrainResource(CHeroCard* hero_card)
{
	int nResult{ 0 };

	const CInteriorLib* pInteriorLib = CInteriorLib::GetInteriotLib(player_.level());
	float fAddition{ 0 };
	switch (internal_cells[1].level_)
	{
	case 1:
		fAddition = pInteriorLib->GetPrimaryAddition();
		nResult = pInteriorLib->GetPrimaryExp();
		break;
	case 2:
		fAddition = pInteriorLib->GetMiddleAddition();
		nResult = pInteriorLib->GetMiddleExp();
		break;
	case 3:
		fAddition = pInteriorLib->GetSeniorAddition();
		nResult = pInteriorLib->GetSeniorExp();
		break;
	case 4:
		fAddition = pInteriorLib->GetMasterAddition();
		nResult = pInteriorLib->GetMaseterExp();
		break;
	default:
		break;
	}

	if (hero_card)
	{
		int hero_honor = (int)((hero_card->hero_template()->GetCmd() + hero_card->train_cmd()) * fAddition);

		for (int i = 0; i < hero_card->hero_template()->GetPassSkillSize(); i++)
		{
			int a = hero_card->hero_template()->GetPassSkill(i);
			const CPassSkill* pSkill = CPassSkill::GetPassSkill(a);
			if (nullptr != pSkill && hero_card->hero_level() >= CHeroTP::GetPassSkillOpenLevel(i))
			{
				if (dat_SKILL_ENUM_ProductionType::ProductionType_Train == pSkill->GetProductionType())
				{
					hero_honor = (int)(hero_honor * (1 + pSkill->GetProductionValue()));
				}
			}
		}
		nResult += hero_honor;
	}

	player_.ChangeHonor(nResult);

	return nResult;
}

int CNeedReset::__GetSearchResource(CHeroCard* hero_card)
{
	int nResult{ 0 };

	const CInteriorLib* pInteriorLib = CInteriorLib::GetInteriotLib(player_.level());
	float fAddition{ 0 };

	switch (internal_cells[2].level_)
	{
	case 1:
		fAddition = pInteriorLib->GetResidenceAddition();
		nResult = pInteriorLib->GetResidenceReputation();
		break;
	case 2:
		fAddition = pInteriorLib->GetVillaAddition();
		nResult = pInteriorLib->GetVillaReputation();
		break;
	case 3:
		fAddition = pInteriorLib->GetCelebritiesAddition();
		nResult = pInteriorLib->GetCelebritiesReputation();
		break;
	case 4:
		fAddition = pInteriorLib->GetRoAddition();
		nResult = pInteriorLib->GetRoReputation();
		break;
	default:
		break;
	}

	if (hero_card)
	{
		int hero_reputation = (int)((hero_card->hero_template()->GetInt() + hero_card->train_int()) * fAddition);

		for (int i = 0; i < hero_card->hero_template()->GetPassSkillSize(); i++)
		{
			int a = hero_card->hero_template()->GetPassSkill(i);
			const CPassSkill* pSkill = CPassSkill::GetPassSkill(a);
			if (nullptr != pSkill && hero_card->hero_level() >= CHeroTP::GetPassSkillOpenLevel(i))
			{
				if (dat_SKILL_ENUM_ProductionType::ProductionType_Search == pSkill->GetProductionType())
					hero_reputation = (int)(hero_reputation * (1 + pSkill->GetProductionValue()));
			}
		}
		nResult += hero_reputation;
	}

	player_.ChangeReputation(nResult);

	return nResult;
}

void CNeedReset::__BuyStage(const CMessage& msg)
{
	pto_STAGE_C2S_REQ_BuyStageTimes pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	switch ((BattleMode)pto.type())
	{
	case BattleMode::NORMAL:__BuyEliteStage(); break;
	case BattleMode::MULTISTAGE:__BuyMultiStage(pto.stage_id()); break;
	case BattleMode::ELITESTAGE:__BuyEliteStage(); break;
	case BattleMode::SPEED:__BuySpeedStageTimes(); break;
	}
}

void CNeedReset::__BuyEliteStage()
{
	pto_STAGE_S2C_RES_BuyStageTimes pto;
	pto.set_type((int)BattleMode::ELITESTAGE);
	pto.set_res(0);

	int need_gold{ 0 };

	if (elite_already_stages_.size() <= 0)
	{
		pto.set_res(3);
	}
	else if (3 <= elite_buy_times_)
	{
		pto.set_res(2);
	}
	else
	{
		need_gold = CGoldSpend::GetGoldSpend(elite_buy_times_ + 1, GST_BuyEliteStageSpend);
		if (need_gold > player_.gold())
			pto.set_res(1);
	}

	if (0 == pto.res())
	{
		elite_buy_times_++;
		elite_already_stages_.clear();
		player_.ChangeGold(-need_gold,26);
	}

	player_.SendToAgent(pto, STAGE_S2C_RES_BuyStageTimes);
}

void CNeedReset::__BuyMultiStage(int stage_id)
{
	pto_STAGE_S2C_RES_BuyStageTimes pto;
	pto.set_type((int)BattleMode::MULTISTAGE);
	pto.set_res(0);
	pto.set_stage_id(stage_id);

	int need_gold{ 0 };

	auto it_buy = multi_buy_stages_.find(stage_id);
	auto it_done = multi_already_stages_.find(stage_id);
	if (it_done == multi_already_stages_.cend())
	{
		pto.set_res(3);
	}
	if (it_buy != multi_buy_stages_.cend() &&
		3 <= it_buy->second)
	{
		pto.set_res(2);
	}
	else if (it_buy == multi_buy_stages_.cend())
	{
		need_gold = CGoldSpend::GetGoldSpend(1, GST_BuyMultiStageSpend);
		if (need_gold > player_.gold())
			pto.set_res(1);
	}
	else if (it_buy != multi_buy_stages_.cend())
	{
		need_gold = CGoldSpend::GetGoldSpend(it_buy->second + 1, GST_BuyMultiStageSpend);
		if (need_gold > player_.gold())
			pto.set_res(1);
	}

	if (0 == pto.res())
	{
		multi_already_stages_.erase(stage_id);

		if (it_buy == multi_buy_stages_.cend())
			multi_buy_stages_.insert(std::make_pair(stage_id, 1));
		else
			it_buy->second++;

		player_.ChangeGold(-need_gold,27);
	}

	player_.SendToAgent(pto, STAGE_S2C_RES_BuyStageTimes);
}

void CNeedReset::__BuySpeedStageTimes()
{
	pto_STAGE_S2C_RES_BuyStageTimes pto;
	pto.set_type((int)BattleMode::SPEED);
	pto.set_res(0);

	const int BUY_SPEED_STAGE_GOLD{ 5 };			//购买竞速次数金币

	if (BUY_SPEED_STAGE_GOLD > player_.gold())
		pto.set_res(1);

	if (0 == pto.res())
	{
		speed_times_++;
		player_.ChangeGold(-BUY_SPEED_STAGE_GOLD,28);
	}

	player_.SendToAgent(pto, STAGE_S2C_RES_BuyStageTimes);
}

void CNeedReset::__MiniGameReward(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_MiniGameReward pto_req;
	pto_req.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_MiniGameReward pto_res;
	pto_res.set_res(0);
	pto_res.set_game_type(pto_req.game_type());
	pto_res.set_reward_num(0);

	if (pto_req.game_type() == 1)
	{
		if (snake_award_ >= 30)
		{
			pto_res.set_res(1);
		}
		else
		{
			int get_gold = pto_req.reward_num();
			if (snake_award_ + get_gold >= 30)
			{
				get_gold = 30 - snake_award_;
			}
			player_.ChangeGold(get_gold,29);
			snake_award_ += get_gold;
			pto_res.set_reward_num(get_gold);
		}
	}

	else if (pto_req.game_type() == 2)
	{
		if (puzzle_award_ >= 50)
		{
			pto_res.set_res(1);
		}
		else
		{
			int get_reputation = pto_req.reward_num();
			if (puzzle_award_ + get_reputation >= 50)
			{
				get_reputation = 50 - puzzle_award_;
			}
			player_.ChangeReputation(get_reputation);
			puzzle_award_ += get_reputation;
			pto_res.set_reward_num(get_reputation);
		}
	}
	player_.SendToAgent(pto_res, PLAYER_S2C_RES_MiniGameReward);
}

void CNeedReset::__GetVIPEveryDay()
{
	bool result = false;

	if (!vip_every_day_)
	{
		SPDailyGift gift = CVIPGift::GetDailyGift(player_.vip_level());
		if (gift)
		{
			player_.ChangeGold(gift->gold,30);
			player_.ChangeSilver(gift->silver);
			result = true;
			vip_every_day_ = true;
		}
	}

	pto_PLAYER_S2C_RES_VIPEveryday pto;
	pto.set_result(result);
	player_.SendToAgent(pto, PLAYER_S2C_RES_VIPEveryday);
}