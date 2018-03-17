#include "stdafx.h"
#include "Player.h"
#include "Town.h"
#include "Guild.h"
#include "Mission.h"
#include "NeedReset.h"
#include "Offline.h"
#include "Knapsack.h"
#include "Army.h"
#include "RoomZ.h"
#include "PlayerExercise.h"
#include "HeroCard.h"
#include "Equip.h"
#include "Mail.h"
#include <regex>

CPlayer::CPlayer(int pid, networkz::ISession* session) :
pid_{ pid },
agent_session_{ session },
login_time_{ time(0) }
{
	try
	{
		__LoadFromDatabase();

		mission_ = new CMission{ *this };
		need_reset_ = new CNeedReset{ *this };
		knapsack_ = new CKnapsack{ *this };
		army_ = new CArmy{ *this };
		offline_ = new COffline{ *this };
		exercise_ = new CPlayerExercise{ *this };

		__InitMaxLevel();
	}
	catch (...)
	{
		delete knapsack_;
		delete army_;
		delete offline_;
		delete mission_;
		delete exercise_;
		delete need_reset_;

		throw std::exception(FormatString("读取玩家数据失败,pid:", pid_).c_str());
	}
}

CPlayer::~CPlayer()
{
	delete knapsack_;
	delete army_;
	delete offline_;
	delete mission_;
	delete exercise_;
	delete need_reset_;
}

void CPlayer::ChangeUser(networkz::ISession* session)
{
	ReadyToOffline();

	agent_session_ = session;
}

void CPlayer::SendToAgent(const ::google::protobuf::Message& pto, int protocol_id, unsigned char protocol_type)
{
	LOCK_BLOCK(proto_buffer_lock_);
	pto.SerializeToString(&proto_buffer_);
	GAME_WORLD()->SendToAgentServer(proto_buffer_, protocol_type, protocol_id, pid_, agent_session_);
}

void CPlayer::ReadyToOffline()
{
	__LeaveTown();
	__RoomQuit();
	__QuitBattle();
}

void CPlayer::SaveToDatabase(bool is_offline)
{
	auto player = FIND_PLAYER(pid());

	GAME_WORLD()->CommitTask(pid_, [is_offline, player]()
	{
		CSerializer<int> titles_ser;
		for (auto &it : player->have_titles_)
			titles_ser.AddValue(it);

		CSerializer<int> recents_ser;
		for (auto &it : player->recents_list_)
			recents_ser.AddValue(it);

		std::ostringstream sql;
		sql << "update tb_player set level = " << player->level_ <<
			", exp = " << player->exp_ <<
			", silver = " << player->silver_ <<
			", gold = " << player->gold_ <<
			", honor = " << player->honor_ <<
			", reputation = " << player->reputation_ <<
			", guild_id = " << player->guild_id_ <<
			", stamina = " << player->stamina_ <<
			", vip_value = " << player->vip_value_ <<
			", using_title = " << player->using_title_ <<
			", have_titles = '" << titles_ser.SerializerToString().c_str() <<
			"',recents_friend = '" << recents_ser.SerializerToString().c_str() <<
			"', town_id = " << player->town_id_ <<
			", town_x = " << player->town_x_ <<
			", town_y = " << player->town_y_ <<
			", offline_time = now()";
		sql << " where pid = " << player->pid();
		MYSQL_UPDATE(sql.str());

		player->__SaveAwards();
		player->__SaveRecord();

		player->need_reset_->SaveToDatabase();
		player->mission_->SaveToDatabase();
		player->knapsack_->SaveToDatabase();
		player->army_->SaveToDatabase();
		player->exercise_->SaveToDatabase();
		player->offline_->SaveToDatabase();

		player->__SaveOrder();

		RECORD_TRACE(FormatString("pid:", player->pid_, " 保存成功"));

		if (is_offline)
		{
			GAME_WORLD()->ErasePlayer(player->pid_);
			RECORD_INFO(FormatString("pid:", player->pid_, " 离开游戏,在线人数:", GAME_WORLD()->PlayerNumber()));
		}
	});
}

void CPlayer::ProcessMessage(const CMessage& msg)
{
	int protocol_id = msg.GetProtocolID();

	if (IS_BATTLE_PROTOCOL(protocol_id))
	{
		if (PlayerState::BATTLE == state_)
			GAME_WORLD()->SendToBattleServer(msg, pid());
		else
			RECORD_TRACE(FormatString("玩家不在战斗中", pid_));
		return;
	}

	switch (protocol_id)
	{
	case TOWN_C2S_REQ_EnterTown:__EnterTown(msg);									break;
	case TOWN_C2S_NTF_Move:__TownMove(msg);											break;

	case CHAT_C2S_NTF_Debug:__TestCommand(msg);										break;
	case PLAYER_C2S_REQ_PlayerInfo:__OnPlayerInfo();								break;
	case PLAYER_C2S_REQ_GetMeditation:__GetMeditation();							break;
	case PLAYER_C2S_REQ_AllTitles:__GetAllTitle();									break;
	case PLAYER_C2S_REQ_UseTitle:__UseTitle(msg);									break;
	case PLAYER_C2S_REQ_PlayerBase:__OnPlayerBase(msg);								break;
	case PLAYER_C2S_REQ_PlayerBaseEx:__OnPlayerBaseEx(msg);							break;
	case PLAYER_C2S_REQ_PlayerDetailInfo:__OnPLayerDetailInfo(msg);					break;
	case PLAYER_C2S_REQ_GetTownBox:__GetTownBox(msg);								break;
	case PLAYER_C2S_REQ_UpdateTownStageRank:__UpdateTownTop10(msg);					break;
	case PLAYER_C2S_REQ_PlayerLvUp:LevelUp();										break;
	case PLAYER_C2S_REQ_GetGift:__GetGift(msg);										break;
	case PLAYER_C2S_REQ_GetVIPAward:__GetVIPAward(msg);								break;
	case PLAYER_C2S_REQ_GetOnlineAward:__GetOnlineAward(msg);						break;


	case SOCIAL_C2S_REQ_AddFriend:__AddFriendsOrBlack(msg);							break;
	case SOCIAL_C2S_REQ_DeleteFriend:__DeleteFriendsOrBlack(msg);					break;
	case CHAT_C2S_NTF_World:__ChatMessage(msg);										break;
	case GUILD_C2S_REQ_CreateGuild:__CreateGuild(msg);								break;
	case GUILD_C2S_REQ_GetGuildInfo:__GetGuildInfo();								break;
	case GUILD_C2S_REQ_FindGuild:__FindGuild(msg);									break;
	case GUILD_C2S_REQ_ApplyGuild:__ApplyGuild(msg);								break;
	case GUILD_C2S_NTF_AgreeApply:__AgreeApply(msg);								break;
	case GUILD_C2S_REQ_ChangeAccess:__ChangeGuildAccess(msg);						break;
	case GUILD_C2S_REQ_QuitGuild:__QuitGuild();										break;
	case GUILD_C2S_REQ_Kick:__GuildKick(msg);										break;
	case GUILD_C2S_REQ_Cession:__CessionGuide(msg);									break;
	case GUILD_C2S_REQ_GuildName:__GetGuildName(msg);								break;
	case GUILD_C2S_REQ_Contribute:__GuildContribute();								break;
	case GUILD_C2S_REQ_ChangeNotification:__GuildEditNotification(msg);				break;

	case ROOM_C2S_REQ_Create:__RoomCreate(msg);										break;
	case ROOM_C2S_REQ_RoomList:__RoomList(msg);										break;
	case ROOM_C2S_REQ_Join:__RoomJoin(msg);											break;
	case ROOM_C2S_NTF_Quit:__RoomQuit();											break;
	case ROOM_C2S_REQ_Lock:__RoomLock(msg);											break;
	case ROOM_C2S_REQ_Kick:__RoomKick(msg);											break;
	case ROOM_C2S_REQ_ChangePos:__RoomChangePos(msg);								break;
	case ROOM_C2S_REQ_Ready:__RoomReady(msg);										break;
	case ROOM_C2S_REQ_Start:__RoomStart();											break;
	case ROOM_C2S_NTF_StartWar:__StartWar();										break;
	case ROOM_C2S_NTF_CancelMatch:GAME_WORLD()->SendToBattleServer(msg, pid());		break;

	case STAGE_C2S_REQ_StartStage:__StartStage(msg);								break;
	case STAGE_C2S_REQ_SweepStage:__SweepStage(msg);								break;
	case STAGE_C2S_REQ_StartEliteStage:__StartEliteStage(msg);						break;

	case STAGE_C2S_REQ_StartSpeedStage:__StartSpeedStage();							break;
	case STAGE_C2S_REQ_UpdateSpeedStage:__UpdateSpeedStage();						break;
	case STAGE_C2S_REQ_CheckSpeedFormation:__CheckSpeedFormation(msg);				break;
	case STAGE_C2S_REQ_SpeedRankList:__SpeedRankList(msg);							break;
	case PLAYER_C2S_REQ_UpdateBattleData:__UpdateBattleRecord();					break;
	case STAGE_C2S_REQ_StartSweepEliteStage:__StartSweepEliteStage(msg);			break;

	case MISSION_C2S_REQ_AcceptMission:
	case MISSION_C2S_REQ_GiveBackMission:mission_->ProcessMessage(msg);				break;

		//军队消息
	case PLAYER_C2S_REQ_SaveFormation:
	case PLAYER_C2S_REQ_TrainHero:
	case PLAYER_C2S_REQ_SaveTrainHero:
	case PLAYER_C2S_REQ_RecruitHero:
	case PLAYER_CS2_REQ_UnitTechnologyLvUp:
	case PLAYER_C2S_REQ_BuyRunePage:
	case PLAYER_C2S_REQ_LockRune:
	case PLAYER_C2S_REQ_FullRuneEnergy:
	case PLAYER_C2S_REQ_WashRune:
	case PLAYER_C2S_REQ_IntimacyGame:
	case PLAYER_C2S_REQ_ExchangeHeroExp:
	case PLAYER_C2S_REQ_ClearExchangeCD:
	case PLAYER_C2S_REQ_UpgradeSoldierTrainLevel:
	case PLAYER_C2S_REQ_CultivateHero:
	{
										 try
										 {
											 army()->ProcessMessage(msg);
										 }
										 catch (std::exception exc)
										 {
											 std::cout << exc.what() << std::endl;
										 }
										 catch (...)
										 {
											 std::cout << "处理玩家军队消息协议时发生异常" << std::endl;
										 }

	}
		break;

		//离线战斗
	case OFFLINEBATTLE_C2S_REQ_StartOfflineBattle:
	case OFFLINEBATTLE_C2S_NTF_PlayerEnter:
	case OFFLINEBATTLE_C2S_REQ_ClearOfflineBattleCd:
	case OFFLINEBATTLE_C2S_REQ_BuyOfflineBattleTimes:
	case OFFLINEBATTLE_C2S_REQ_ReplayOfflineBattle:
	case OFFLINEBATTLE_C2S_REQ_GetEscortCar:
	case OFFLINEBATTLE_C2S_REQ_StartEscort:
	case OFFLINEBATTLE_C2S_REQ_UpdateEscort:
	case OFFLINEBATTLE_C2S_NTF_EnterEscort:
	case OFFLINEBATTLE_C2S_REQ_RobEscort:
	case OFFLINEBATTLE_C2S_NTF_SetAutoProtect:
	case OFFLINEBATTLE_C2S_REQ_InviteProtect:
	case OFFLINEBATTLE_C2S_RES_AskProtect:
	case OFFLINEBATTLE_C2S_REQ_UpdateProtecterList:
	case PLAYER_C2S_REQ_UpdatePlayerOfflineBattleData:
	case OFFLINEBATTLE_C2S_REQ_OfflineList:
	case OFFLINEBATTLE_C2S_REQ_UpdateWorldBoss:
	case OFFLINEBATTLE_C2S_REQ_BeginWorldBoss:{
		try
		{
			offline()->ProcessMessage(msg);
		}
		catch (std::exception exc)
		{
			std::cout << exc.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "处理玩家离线消息协议时发生异常" << std::endl;
		}
	}
		break;

		//物品消息
	case PLAYER_C2S_REQ_ExchangeEquip:
	case PLAYER_C2S_REQ_UseItem:
	case PLAYER_C2S_REQ_RemoveItem:
	case PLAYER_C2S_REQ_SellItem:
	case PLAYER_C2S_REQ_ItemToBackpack:
	case PLAYER_C2S_REQ_EquipLvUp:
	case PLAYER_C2S_REQ_RecastEquip:
	case PLAYER_C2S_NTF_CloseLottery:
	case PLAYER_C2S_REQ_ChooseLottery:
	case PLAYER_C2S_REQ_ReadMail:
	case PLAYER_C2S_REQ_GetMailReward:
	case PLAYER_C2S_REQ_DeleteMail:
	case PLAYER_C2S_REQ_GetMailList:
	case PLAYER_C2S_REQ_MakeEquip:
	case PLAYER_C2S_REQ_ActivateEquipEnchanting:
	case PLAYER_C2S_REQ_Enchanting:
	case PLAYER_C2S_REQ_ExchangeFashion:
	case PLAYER_C2S_REQ_BuyFashion:
	case PLAYER_C2S_REQ_ResolveEquip:
	case PLAYER_C2S_REQ_GetFashion:
	case PLAYER_C2S_REQ_BatchSell:
	case PLAYER_C2S_REQ_BuyInMall:
	case PLAYER_C2S_REQ_Inherit:{
		try
		{
			knapsack()->ProcessMessage(msg);
		}
		catch (std::exception exc)
		{
			std::cout << exc.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "处理玩家物品消息协议时发生异常" << std::endl;
		}
	}
		break;

	case PLAYER_C2S_REQ_GetTimeBox:
	case PLAYER_C2S_REQ_VIPEveryday:
	case STAGE_C2S_REQ_BuyStageTimes:
	case PLAYER_C2S_REQ_MiniGameReward:
	case PLAYER_C2S_REQ_UseInterior:
	case PLAYER_C2S_REQ_InteriorLvUp:
	case PLAYER_C2S_REQ_ClearInteriotCD:
	case PLAYER_C2S_REQ_UseLevy:
	case PLAYER_C2S_REQ_UpgradeCastle:
	case PLAYER_C2S_REQ_BuyStamina:
	case PLAYER_C2S_REQ_BuyRewardTimes:
	case PLAYER_C2S_REQ_CommitRewardMission:
	case PLAYER_C2S_REQ_AcceptRewardMission:
	case PLAYER_C2S_REQ_AbandonRewardMission:
	case PLAYER_C2S_REQ_RefreshRewardMission:
	case PLAYER_C2S_REQ_RandCard:
	case PLAYER_C2S_REQ_BuyRandCard:
	case PLAYER_C2S_REQ_GetPrayCard:
	{
		try
		{
			need_reset_->ProcessMessage(msg);
		}
		catch (std::exception exc)
		{
			std::cout << exc.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "处理玩家NeedReset消息协议时发生异常" << std::endl;
		}
	}
		break;

		//练功台
	case PLAYER_C2S_REQ_ContendPlatform:
	case PLAYER_C2S_REQ_TakePlatformExp:
	case PLAYER_C2S_REQ_TakeExpPill:
	case PLAYER_C2S_REQ_BuyContendTimes:
	case PLAYER_C2S_NTF_EnterExercisePlatform:
	case PLAYER_C2S_REQ_ClearExercisePlatformCD:
	{
		try
		{
			exercise()->ProcessMessage(msg);
		}
		catch (std::exception exc)
		{
			std::cout << exc.what() << std::endl;
		}
		catch (...)
		{
			std::cout << "处理玩家练功台消息协议时发生异常" << std::endl;
		}
	}
		break;

	default:
	{
		RECORD_WARNING(FormatString("未知的协议号:protocol_id:", protocol_id));
		__FrozenPlayer(pid_,1);
	}
		break;
	}
}

void CPlayer::CheckOrder()
{
	GAME_WORLD()->CommitTask(pid_, [this]()
	{
		LOCK_BLOCK(order_lock_);

		std::ostringstream sql;
		sql << "select order_id,point,rmb from tb_order_form where pid = " << pid() << " and is_get = false";
		ResultSetPtr result_order{ MYSQL_QUERY(sql.str()) };
		if (!result_order)
			return;
		while (result_order->next())
		{
			if (orders_.insert(result_order->getString("order_id").c_str()).second)
			{
				int rmb = result_order->getInt("rmb");

				//int year = GetTimeValue(TIME_TYPE::YEAR, time(0));
				//int month = GetTimeValue(TIME_TYPE::MONTH, time(0));
				//int day = GetTimeValue(TIME_TYPE::DAY, time(0));
				//int hour = GetTimeValue(TIME_TYPE::HOUR, time(0));
				//bool get_reward_{ true };
				//if (year != 2014)
				//	get_reward_ = false;
				//if (month != 12)
				//	get_reward_ = false;
				//if (day < 17 || day > 24)
				//	get_reward_ = false;
				//if (day == 17 && hour < 14)
				//	get_reward_ = false;
				//if (day == 24 && hour > 13)
				//	get_reward_ = false;

				//充值奖励
				//if (get_reward_)
				{
					if (rmb >= 100 && rmb < 200)
					{
						SPMail mail = std::make_shared<CMail>();
						mail->SetModelID(MMI_Recharge);
						mail->AddRewardItem(LT_Gold, 100);
						mail->AddRewardItem(91002, 4);
						mail->SetParam0(100);
						GAME_WORLD()->GiveNewMail(mail, pid());
					}
					else if (rmb >= 200 && rmb < 500)
					{
						SPMail mail = std::make_shared<CMail>();
						mail->SetModelID(MMI_Recharge);
						mail->AddRewardItem(LT_Gold, 300);
						mail->AddRewardItem(91002, 10);
						mail->SetParam0(200);
						GAME_WORLD()->GiveNewMail(mail, pid());
					}
					else if (rmb >= 500 && rmb < 1000)
					{
						SPMail mail = std::make_shared<CMail>();
						mail->SetModelID(MMI_Recharge);
						mail->AddRewardItem(LT_Gold, 1000);
						mail->AddRewardItem(91002, 30);
						mail->SetParam0(500);
						GAME_WORLD()->GiveNewMail(mail, pid());
					}
					else if (rmb >= 1000)
					{
						SPMail mail = std::make_shared<CMail>();
						mail->SetModelID(MMI_Recharge);
						mail->AddRewardItem(LT_Gold, 3000);
						mail->AddRewardItem(91002, 80);
						mail->SetParam0(1000);
						GAME_WORLD()->GiveNewMail(mail, pid());
					}
				}

				ChangeVIPValue(rmb);
				ChangeGold(result_order->getInt("point"), 4);
			}
		}
	});
}

void CPlayer::ChangeVIPValue(int num)
{
	if (num <= 0)
		return;

	vip_value_ += num;
	vip_level_ = CVIPFun::GetVIPLevel(vip_value_);

	if (vip_level_ >= 9)
		GiveNewTitle(14);

	pto_PLAYER_S2C_NTF_VIPLevelUp pto;
	pto.set_vip_value(vip_value_);
	pto.set_vip_level(vip_level_);
	SendToAgent(pto, PLAYER_S2C_NTF_VIPLevelUp);
}

void CPlayer::ChangeGold(int num, int change_pos)
{
	if (0 == num)
		return;

	gold_ += num;

	if (num < 0)
	{
		ImplementRewardMission(RewardMissionType::RMT_UseGold, 0, 1);
	}
	else
	{
		if (GAME_WORLD()->watch_pid_ == pid_)
			RECORD_WARNING(FormatString("增加金币:", num, "位置:", change_pos));
	}

	pto_PLAYER_S2C_NTF_UpdatePlayerDiamond pto;
	pto.set_current_diamond(gold_);
	SendToAgent(pto, PLAYER_S2C_NTF_UpdatePlayerDiamond);
}

void CPlayer::ChangeSilver(__int64 num)
{
	if (0 == num)
		return;

	silver_ += num;

	pto_PLAYER_S2C_NTF_UpdatePlayerMoney pto;
	pto.set_current_money(silver_);
	SendToAgent(pto, PLAYER_S2C_NTF_UpdatePlayerMoney);
}

void CPlayer::ChangeHonor(__int64 num)
{
	if (0 == num)
		return;

	honor_ += num;

	pto_PLAYER_S2C_NTF_UpdatePlayerHonour pto;
	pto.set_current_honour(honor_);
	SendToAgent(pto, PLAYER_S2C_NTF_UpdatePlayerHonour);
}

void CPlayer::ChangeReputation(int num)
{
	if (0 == num)
		return;

	reputation_ += num;

	pto_PLAYER_S2C_NTF_UpdatePlayerReputation pto;
	pto.set_current_reputation(reputation_);
	SendToAgent(pto, PLAYER_S2C_NTF_UpdatePlayerReputation);
}

void CPlayer::ChangeExp(__int64 num)
{

	if (0 == num)
		return;

	__int64 temp_exp{ num };
	__int64 max_exp{ 0 };

	const int kOpenLevelUP{ 18 };				//自动升级主线进度
	const int kMaxAutoLevelUP{ 18 };			//自动升级等级

	const CExpLibrary* pExpLib{ CExpLibrary::GetExpLibrary(level()) };
	if (pExpLib)
		max_exp = pExpLib->GetNeedExp();

	int nTypeParameters{ 0 };
	int nParameters{ 0 };

	bool auto_level_up = IsOpenGuide(15);

	bool is_lv_up{ false };

	while (temp_exp + exp() >= max_exp && !auto_level_up/* && kMaxAutoLevelUP > level()*/ && level() < max_level())
	{
		temp_exp = temp_exp + exp() - max_exp;

		level_++;
		exp_ = 0;

		is_lv_up = true;

		if (level_ >= 50)
			GiveNewTitle(3);
		if (level_ >= 30)
			GiveNewTitle(2);
		if (level_ >= 10)
			GiveNewTitle(1);

		const CExpLibrary* temp_exp_lib = CExpLibrary::GetExpLibrary(level());

		if (nullptr != pExpLib)
			max_exp = temp_exp_lib->GetNeedExp();
		else
			max_exp = 0;
	}

	if (is_lv_up)
	{
		pto_PLAYER_S2C_RES_PlayerLvUp pto;
		pto.set_res(0);
		pto.set_player_lv(level());
		SendToAgent(pto, PLAYER_S2C_RES_PlayerLvUp);

		GAME_WORLD()->UpdateMaxLevel(level());
	}

	if (kOpenLevelUP > mission()->main_progress() && kMaxAutoLevelUP > level() && level() < max_level())
	{
		exp_ += temp_exp;
	}
	else if (pExpLib)
	{
		__int64 nMaxExp{ pExpLib->GetNeedExp() * 2 };

		exp_ += num;
		exp_ = exp_ < nMaxExp ? exp_ : nMaxExp;
		exp_ = exp_ > 0 ? exp_ : 0;
	}
	else
	{
		RECORD_WARNING(FormatString("找不到经验库所对应的等级,LV：", level()).c_str());
	}

	pto_PLAYER_S2C_NTF_UpdatePlayerExp pto;
	pto.set_current_exp(exp());
	SendToAgent(pto, PLAYER_S2C_NTF_UpdateExp);
}

void CPlayer::ChangeStamina(int num)
{
	if (0 == num)
		return;

	if (num < 0)
	{
		stamina_ += num;
	}
	else if (stamina_ < kMaxStamina)
	{
		stamina_ += num;
	}

	pto_PLAYER_S2C_NTF_UpdatePlayerStamina pto;
	pto.set_current_stamina(stamina_);
	SendToAgent(pto, PLAYER_S2C_NTF_UpdatePlayerStamina);
}

void CPlayer::ItemChangeStamina(int num)
{
	if (0 == num)
		return;
	stamina_ += num;

	pto_PLAYER_S2C_NTF_UpdatePlayerStamina pto;
	pto.set_current_stamina(stamina_);
	SendToAgent(pto, PLAYER_S2C_NTF_UpdatePlayerStamina);
}

bool CPlayer::IsOpenGuide(int guide_id)
{
	const COpenGuid* open_guide{ COpenGuid::GetOpenGuid(guide_id) };

	if (open_guide)
	{
		switch (open_guide->open_type())
		{
		case 1:
			if (open_guide->parameters() > level())
			{
				RECORD_TRACE("玩家未达到开启功能要求");
				return false;
			}
			break;
		case 2:
			if (open_guide->parameters() > main_progress())
			{
				//RECORD_TRACE("玩家未达到开启功能要求");
				return false;
			}
			break;
		}
	}
	return true;
}

bool CPlayer::GiveNewTitle(int title_id)
{
	if (title_id <= 0)
		return false;

	LOCK_BLOCK(title_lock_);

	if (have_titles_.insert(title_id).second)
	{
		pto_PLAYER_S2C_NTF_GetNewTitle pto;
		pto.set_title_id(title_id);
		SendToAgent(pto, PLAYER_S2C_NTF_GetTitle);
		return true;
	}
	else
	{
		return false;
	}
}

void CPlayer::AddRecents(int contact_pid)
{
	if (pid() == contact_pid)
		return;

	{
		LOCK_BLOCK(social_lock_);

		for (auto it = recents_list_.begin(); it != recents_list_.cend(); ++it)
		{
			if (*it == contact_pid)
			{
				recents_list_.erase(it);
				break;
			}
		}

		recents_list_.push_front(contact_pid);

		while (recents_list_.size() > kRecentsMaxNum)
			recents_list_.pop_back();
	}

	pto_SOCLIAL_S2C_NTF_AddContact pto;
	auto contact = pto.mutable_new_contact();
	SPIPlayerData data = PLAYER_DATA(contact_pid);
	if (data)
	{
		contact->set_p_id(data->pid());
		contact->set_lv(data->level());
		contact->set_name(data->name());
		contact->set_map_id(data->town_id());
	}
	SendToAgent(pto, SOCIAL_S2C_NTF_AddContact);
}

int	 CPlayer::main_progress()
{
	return	mission_->main_progress();
}

int CPlayer::exercise_time()
{
	return exercise_->exercise_time();
}

CHeroCard*	CPlayer::hero(int index)
{
	if (index >= kMaxHeroNum)
		return nullptr;
	return army_->FindHero(army_->GetFormationHero()[index]);
}

std::vector<const CEquip*> CPlayer::hero_equip(int hero_id)
{
	return std::move(knapsack_->GetHeroEquip(hero_id));
}

int	CPlayer::technology(int index)
{
	if (index >= kSoldierTechCount)
		return 0;
	return army_->GetTechnology()[index];
}

void CPlayer::ImplementMission(EMissionTargetType target_type, int target_id, int target_id_ex)
{
	mission_->ImplementMission(target_type, target_id, target_id_ex);
}

void CPlayer::ImplementRewardMission(RewardMissionType type, int target_id, int num)
{
	need_reset_->ImplementRewardMission(type, target_id, num);
}

int  CPlayer::clothes()
{
	return knapsack_->clothes_id();
}

void CPlayer::LevelUp()
{
	const CExpLibrary* exp_lib{ CExpLibrary::GetExpLibrary(level()) };
	if (!exp_lib)
		return;

	int			result{ 0 };
	__int64		need_exp{ exp_lib->GetNeedExp() };

	if (exp() < need_exp)
	{
		result = 1;
	}
	else if (level() >= max_level())
	{
		result = 2;
	}
	else
	{
		ChangeExp(-need_exp);

		level_++;

		mission()->ImplementMission(EMissionTargetType::MTT_DIYLvUp, 0, 0);

		army()->OpenRunePage(level_);

		if (level_ >= 50)
			GiveNewTitle(3);
		if (level_ >= 30)
			GiveNewTitle(2);
		if (level_ >= 10)
			GiveNewTitle(1);

		CTown* town{ GAME_WORLD()->FindTown(town_id()) };
		if (town)
		{
			pto_TOWN_S2C_NTF_LvUp pto_lv_up;
			pto_lv_up.set_pid(pid());
			pto_lv_up.set_lv(level());

			SendToAgent(pto_lv_up, TOWN_S2C_NTF_Lvup);
		}
	}

	pto_PLAYER_S2C_RES_PlayerLvUp pto;
	pto.set_res(result);
	pto.set_player_lv(level());
	SendToAgent(pto, PLAYER_S2C_RES_PlayerLvUp);
	
	//更新任务
	if (mission_)
		mission_->SendInitialMessage();
	
	GAME_WORLD()->UpdateMaxLevel(level_);
}

std::string	CPlayer::guild_name()
{
	auto guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
	{
		return guild->name();
	}
	else
	{
		return "";
	}
}

int	CPlayer::guild_postion()
{
	auto guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
		return (int)guild->GetPosition(pid_);
	else
		return 0;
}

void CPlayer::SetLevel(int level)
{
	pto_PLAYER_S2C_RES_PlayerLvUp pto;
	level_ = level;

	if (level_ > kMaxLevel)
		level_ = kMaxLevel;
	if (level_ < 1)
		level_ = 1;

	army()->OpenRunePage(level_);

	if (level_ >= 50)
		GiveNewTitle(3);
	if (level_ >= 30)
		GiveNewTitle(2);
	if (level_ >= 10)
		GiveNewTitle(1);

	pto.set_res(0);
	pto.set_player_lv(level_);
	SendToAgent(pto, PLAYER_S2C_RES_PlayerLvUp);

	GAME_WORLD()->UpdateMaxLevel(level_);
}

void CPlayer::SetMaxLevel(int max_level)
{
	if (max_level_ >= max_level)
		return;

	max_level_ = max_level;

	pto_PLAYER_S2C_NTF_ChangeMaxLevel pto;
	pto.set_max_level(max_level_);
	SendToAgent(pto, PLAYER_S2C_NTF_ChangeMaxLevel);
}

void CPlayer::SetBattleMasterProto(pto_BATTLE_STRUCT_Master_Ex *pto, bool is_fair)
{
	pto->set_pid(pid());
	pto->set_sid(sid());
	pto->set_name(name());
	pto->set_level(level());
	pto->set_clothes(knapsack()->clothes_id());
	pto->set_sex(sex());
	pto->set_title_id(using_title_);
	army()->SetHeroesAndSoldiersProtocol(pto, is_fair);
}

void CPlayer::StageBattleWin(const CStage* stage)
{
	SetMaxLevel(stage->m_nUnlockLevel);
	ChangeExp(stage->m_nRewardExp);
	ChangeSilver(stage->m_nRewardSilver);
	ChangeHonor(stage->m_nRewardHonor);
	knapsack()->GiveNewItem(stage->m_nRewardItem1ID, stage->m_nRewardItem1Num);
	knapsack()->GiveNewItem(stage->m_nRewardItem2ID, stage->m_nRewardItem2Num);
	knapsack()->GiveNewItem(stage->m_nRewardItem3ID, stage->m_nRewardItem3Num);
	knapsack()->GiveNewItem(stage->m_nRewardItem4ID, stage->m_nRewardItem4Num);
	knapsack()->GiveNewItem(stage->m_nRewardItem5ID, stage->m_nRewardItem5Num);
	knapsack()->GiveNewItem(stage->m_nRewardItem6ID, stage->m_nRewardItem6Num);
	ChangeStamina(-kStageStamina);

	ImplementMission(EMissionTargetType::MTT_CompleteMap, stage->m_nLevel, stage->m_nDifficulty);
	ImplementRewardMission(RewardMissionType::RMT_SweepStage, stage->m_nLevel, 1);

	mission()->SetStageProcess(stage->m_nLevel, (StageType)stage->m_nDifficulty);
}

void CPlayer::EliteStageBattleWin(int box_num, const CEliteStage* stage)
{
	need_reset_->AddEliteAlready(stage->GetLevel());

	ChangeStamina(-20);
	ChangeExp(stage->m_nRewardExp);
	ChangeSilver(stage->m_nRewardSilver);
	ChangeHonor(stage->m_nRewardHonour);
	knapsack()->GiveNewItem(stage->m_nRewardBox, box_num);

	mission_->SetStageProcess(stage->m_nLevel, StageType::ELITE);
	ImplementMission(EMissionTargetType::MTT_EliteStage, stage->m_nLevel, 0);
}

void CPlayer::MultiStageBattleWin(int box_num, const CEliteStage* stage)
{
	if (!need_reset_->IsMultiStageClear(stage->GetLevel()))
	{
		need_reset_->AddMultiAlready(stage->GetLevel());

		ChangeExp(stage->m_nRewardExp);
		ChangeSilver(stage->m_nRewardSilver);
		ChangeHonor(stage->m_nRewardHonour);
		knapsack()->GiveNewItem(stage->m_nRewardBox, box_num);

		mission()->SetStageProcess(stage->m_nLevel, StageType::MULTI);

		ImplementMission(EMissionTargetType::MTT_MultiStage, stage->m_nLevel, 0);
	}
	need_reset_->UpdateMultiStage();
}

void CPlayer::SpeedStageWin(int time)
{
	if (need_reset_->speed_today_best_time() <= 0 || time < need_reset_->speed_today_best_time())
	{
		need_reset_->speed_today_best_time(time);

		int rank = GAME_WORLD()->ChangeSpeedStageRank(pid());
		if (rank <= 10)
			GAME_WORLD()->Annunciate(name(), rank, 0, AnnunciateType::StageSpeedRank);
	}
}

void CPlayer::OnGameOver(pto_BATTLE_S2C_NTF_Game_Over& pto)
{
	if (GetTimeValue(TIME_TYPE::HOUR, time(0)) == 20 || GetTimeValue(TIME_TYPE::HOUR, time(0)) == 21)
	{
		BattleMode mode{ (BattleMode)pto.mode() };

		BattleRecord* data{ nullptr };

		switch (mode)
		{
		case BattleMode::WAR:
			data = &battle_record_[0];
			need_reset()->add_week_war_times();
			break;
		case BattleMode::ARENA1v1:
			data = &battle_record_[1];
			need_reset()->add_week_1v1_times();
			break;
		case BattleMode::ARENA3v3:
			data = &battle_record_[2];
			need_reset()->add_week_3v3_times();
			break;
		case BattleMode::ARENA1v1Easy:
			data = &battle_record_[1];
			need_reset()->add_week_1v1_times();
			break;
		case BattleMode::ARENA3v3Easy:
			data = &battle_record_[2];
			need_reset()->add_week_3v3_times();
			break;
		case BattleMode::ARENA1v1Normal:
			data = &battle_record_[1];
			need_reset()->add_week_1v1_times();
			break;
		case BattleMode::ARENA3v3Normal:
			data = &battle_record_[2];
			need_reset()->add_week_3v3_times();
			break;
		case BattleMode::EXERCISE:
			break;
		default:
			return;
		}

		for (int i = 0; i < pto.player_date_size(); i++)
		{
			if (pto.player_date(i).pid() == pid())
			{
				auto pPtoPlayer = pto.player_date(i);
				int nMark{ pPtoPlayer.player_mark() };
				break;
			}
		}

		switch (pto.player_grade())
		{
		case 0:data->S++; break;
		case 1:data->A++; break;
		case 2:data->B++; break;
		case 3:data->C++; break;
		}

		int nEnemyPoint{ pto.enemy_point() };

		if (pto.is_win())
		{
			int nX{ nEnemyPoint - data->point };

			int nDifference{ 25 };

			//对方队伍初始匹配积分 比个人的真实积分每高20点，得到的分数就提高1分
			//对方队伍初始匹配积分 比个人的真实积分每低15点，得到的分数就减少1分
			if (nX > 0)
				nDifference += (nX / 20);
			else
				nDifference += (nX / 15);

			if (nDifference > 40)
				nDifference = 40;
			if (nDifference < 1)
				nDifference = 1;

			data->point += nDifference;

			if (data->point >= 1800)
				GiveNewTitle(11);
			if (data->point >= 2200)
				GiveNewTitle(12);
			if (data->point >= 2500)
				GiveNewTitle(13);

			data->win++;
		}
		else
		{
			int nX{ nEnemyPoint - data->point };

			int nDifference{ -15 };

			//对方队伍初始匹配积分 比个人的真实积分每高20点，扣除的分数就减少1分
			//对方队伍初始匹配积分 比个人的真实积分每低25点，扣除的分数就增加1分
			if (nX > 0)
				data->point += (nX / 20);
			else
				data->point += (nX / 15);

			if (nDifference < -45)
				nDifference = -45;
			if (nDifference > -1)
				nDifference = -1;

			data->point += nDifference;

			data->lose++;
		}

		pto.set_point(data->point);
	}

	if (pto.is_win())
	{
		//给予跨服奖励
		__GiveArenaSingleBonus(true, pto.time());
		if (need_reset()->is_got_battle_reward() == false)
			__GetDayWinAward();

		need_reset()->is_got_battle_reward(true);
	}
	else
	{
		//给予1/4跨服奖励
		__GiveArenaSingleBonus(false, pto.time());
	}
	
	__UpdateBattleRecord();

	SendToAgent(pto, BATTLE_S2C_NTF_GameOver);
}

void CPlayer::__GiveArenaSingleBonus(bool bIsWin,int nTime)
{
	if (nTime < 60)	//少于60秒的战斗不给奖励
		return;

	const CRootData* root_data{ CRootData::GetRootData(level_) };

	if (!root_data)
		return;
	__int64 nSilver = root_data->GetRootData(level_, RootDataType::RDT_FairSingleSilver);
	__int64 nHonour = root_data->GetRootData(level_, RootDataType::RDT_FairSingleHonour);

	if (!bIsWin)
	{
		nSilver /= 4;
		nHonour /= 4;
	}

	ChangeSilver(nSilver);
	ChangeHonor(nHonour);
}

bool CPlayer::GetBoxState(int town_id, StageType type)
{
	if (town_id > 32)
	{
		RECORD_WARNING("城镇ID错误");
		return false;
	}

	int i{ 1 };
	for (; i < town_id; i *= 2);

	int num{ 0 };

	switch (type)
	{
	case StageType::NORMAL:
		num = box_normal_progress_;
		break;
	case StageType::HARD:
		num = box_hard_progress_;
		break;
	case StageType::NIGHTMARE:
		num = box_nightmare_progress_;
		break;
	default:
		return false;
	}

	if (num & i)
		return true;
	else
		return false;
}

void CPlayer::OnRoomKick()
{
	__RoomQuit();
}

bool CPlayer::IsFreind(int pid)
{
	LOCK_BLOCK(social_lock_);
	for (auto &it : friends_list_)
	{
		if (it == pid)
			return true;
	}
	return false;
}

int	CPlayer::bodygurand()
{
	return offline()->bodyguard();
}

time_t CPlayer::offline_last_time()
{
	return offline()->last_time();
}

int	CPlayer::battle_week_times(BattleRecordType type)
{
	switch (type)
	{
	case kBattleRecordWar:return need_reset_->week_war_times();
	case kBattleRecord1v1:return need_reset_->week_1v1_times();
	case kBattleRecord3v3:return need_reset_->week_3v3_times();
	}
	return 0;
}

void CPlayer::SendResetServer(int time)
{
	if (access_ != PlayerAccess::kAccessGM)
		return;
	GAME_WORLD()->Annunciate("", time, time, AnnunciateType::ResetServer);
}

void CPlayer::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select sid, name, sex, level, exp, silver, gold, honor, reputation, guild_id,stamina, vip_value, using_title, have_titles, recents_friend, access,unix_timestamp(offline_time) as ott, town_id,town_x,town_y from tb_player where pid = " << pid();
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (result && result->next())
	{
		sid_ = result->getInt("sid");
		last_offline_time_ = result->getInt64("ott");
		name_ = result->getString("name").c_str();
		sex_ = result->getBoolean("sex");
		level_ = result->getInt("level");
		exp_ = result->getInt64("exp");
		silver_ = result->getInt64("silver");
		gold_ = result->getInt("gold");
		honor_ = result->getInt64("honor");
		reputation_ = result->getInt("reputation");

		access_ = (PlayerAccess)result->getInt("access");

		guild_id_ = result->getInt("guild_id");

		vip_value_ = result->getInt("vip_value");
		vip_level_ = CVIPFun::GetVIPLevel(vip_value_);

		stamina_ = result->getInt("stamina");
		if (stamina_ < kMaxStamina)
		{
			stamina_ += static_cast<int>(GetDurationCount(time(0), last_offline_time_, DURATION_TYPE::MINUTE) / 5);
			stamina_ = stamina_ < kMaxStamina ? stamina_ : kMaxStamina;
		}

		using_title_ = result->getInt("using_title");
		std::vector<int> temp_titles = CSerializer<int>::ParseFromString(result->getString("have_titles").c_str());
		for (auto it : temp_titles)
			have_titles_.insert(it);

		temp_titles = CSerializer<int>::ParseFromString(result->getString("recents_friend").c_str());
		for (auto it : temp_titles)
			recents_list_.push_back(it);

		town_id_ = result->getInt("town_id");
		town_x_ = result->getInt("town_x");
		town_y_ = result->getInt("town_y");
	}
	else
	{
		throw 1;
	}

	__LoadAwards();
	__LoadFriends();
	__LoadRecord();
}

void CPlayer::__LoadAwards()
{
	std::ostringstream sql;
	sql << "select unix_timestamp(meditation_time) as mtt,online_day_award,vip_level_award,normal_box,hard_box,nightmare_box,speed_stage_record from tb_player_award where pid = " << pid();
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (result && result->next())
	{
		meditation_time_ = result->getInt64("mtt");
		vip_level_award_ = result->getInt("vip_level_award");
		online_day_award_ = result->getInt("online_day_award");
		box_normal_progress_ = result->getInt("normal_box");
		box_hard_progress_ = result->getInt("hard_box");
		box_nightmare_progress_ = result->getInt("nightmare_box");
		speed_stage_record_ = result->getInt("speed_stage_record");
	}
	else
	{
		throw 2;
	}
}

void CPlayer::__LoadRecord()
{
	std::ostringstream sql;
	sql << "select * from tb_player_record where pid = " << pid();
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (!result)
		throw 2;
	while (result->next())
	{
		int type = result->getInt("type");
		battle_record_[type].point = result->getInt("point");
		battle_record_[type].S = result->getInt("S");
		battle_record_[type].A = result->getInt("A");
		battle_record_[type].B = result->getInt("B");
		battle_record_[type].C = result->getInt("C");
		battle_record_[type].win = result->getInt("win");
		battle_record_[type].lose = result->getInt("lose");
	}
}

void CPlayer::__LoadFriends()
{
	std::ostringstream sql;
	sql << "select target_pid,is_black from tb_player_friend where pid = " << pid_;
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (!result)
		throw 4;
	while (result->next())
	{
		LOCK_BLOCK(social_lock_);

		if (result->getBoolean("is_black"))
			black_list_.insert(result->getInt("target_pid"));
		else
			friends_list_.insert(result->getInt("target_pid"));
	}
}

void CPlayer::__SaveAwards()
{
	std::ostringstream sql;
	sql << "update tb_player_award set meditation_time = from_unixtime(" << meditation_time_ <<
		"), online_day_award = " << online_day_award_ <<
		", vip_level_award = " << vip_level_award_.to_ulong() <<
		", normal_box = " << box_normal_progress_ <<
		", hard_box = " << box_hard_progress_ <<
		", nightmare_box = " << box_nightmare_progress_ <<
		", speed_stage_record = " << speed_stage_record_ <<
		" where pid = " << pid();
	MYSQL_UPDATE(sql.str());
}

void CPlayer::__SaveRecord()
{
	std::ostringstream sql;

	for (size_t i = 0; i < battle_record_.size(); i++)
	{
		sql.str("");

		sql << "update tb_player_record set point = " << battle_record_[i].point <<
			", S = " << battle_record_[i].S <<
			", A = " << battle_record_[i].A <<
			", B = " << battle_record_[i].B <<
			", C = " << battle_record_[i].C <<
			", win = " << battle_record_[i].win <<
			", lose = " << battle_record_[i].lose <<
			" where pid = " << pid() << " and type = " << i;
		MYSQL_UPDATE(sql.str());
	}
}

void CPlayer::__SaveOrder()
{
	LOCK_BLOCK(order_lock_);

	if (!orders_.empty())
	{
		std::string orders_str;
		for (auto &it : orders_)
		{
			orders_str += "'";
			orders_str += it.c_str();
			orders_str += "',";
		}

		orders_str.erase(orders_str.length() - 1, 1);

		std::ostringstream sql;
		sql << "update tb_order_form set is_get = true where order_id in (" << orders_str.c_str() << ")";
		MYSQL_UPDATE(sql.str());

		orders_.clear();
	}
}

void CPlayer::__EnterTown(const CMessage& msg)
{
	pto_TOWN_C2S_REQ_EnterTown pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	ENUM_EnterTownResult result{ ETR_SUCCESS };

	CTown* town = GAME_WORLD()->FindTown(pto.townid());

	if (!town)
	{
		result = ETR_NOT_FIND_TOWN;
		RECORD_WARNING("找不到城镇");
	}
	else if (is_in_battle())
	{
		result = ETR_IN_BATTLE;
		RECORD_WARNING("战斗中不能进入城镇");
	}
	else
	{
		__LeaveTown();
		result = town->EnterTown(FIND_PLAYER(pid_));
	}

	pto_TOWN_S2C_RES_EnterTown	pto_res;
	if (ETR_SUCCESS == result)
		town->SetAllPlayersToProtocol(pto_res);
	pto_res.set_townid(pto.townid());
	pto_res.set_result(result);
	SendToAgent(pto_res, TOWN_S2C_RES_EnterTown);
}

void CPlayer::__LeaveTown()
{
	CTown* old_town = GAME_WORLD()->FindTown(town_id_);
	if (old_town)
		old_town->LeaveTown(pid_);
	else
		RECORD_ERROR(FormatString("找不到城镇ID:", town_id()));
}

void CPlayer::__TownMove(const CMessage& msg)
{
	pto_TOWN_C2S_NTF_Move pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CTown* town = GAME_WORLD()->FindTown(town_id_);
	if (town)
		town->TownMove(pid_, pto.x(), pto.y());
	else
		RECORD_ERROR("找不到城镇");
}

void CPlayer::__SetProtobuff(dat_PLAYER_STRUCT_PlayerData* pto)
{
	pto->set_pid(pid());
	pto->set_sid(GAME_WORLD()->sid());
	pto->set_name(name_);
	pto->set_sex(sex_);
	pto->set_level(level_);
	pto->set_exp(exp_);
	pto->set_max_level(max_level_);

	pto->set_vip_level(vip_level_);
	pto->set_vip_exp(vip_value_);
	pto->set_silver(silver_);
	pto->set_gold(gold_);
	pto->set_reputation(reputation_);
	pto->set_honour(honor_);
	pto->set_stamina(stamina_);
	pto->set_town_id(town_id_);
	pto->set_town_x(town_x_);
	pto->set_town_y(town_y_);

	pto->set_clothes(clothes());
	pto->set_using_title(using_title_);
	pto->set_access(access_);
	pto->set_meditation(meditation_time_);
	pto->set_guild_id(guild_id_);
	pto->set_has_mail(knapsack()->HasUnreadMails());
	pto->set_buystamina(need_reset_->today_buy_stamina_times());

	pto->set_snake(need_reset_->snake_award());
	pto->set_puzzle(need_reset_->puzzle_award());

	pto->set_vip_level_award(vip_level_award_.to_ulong());

	pto->set_guild_contribute(need_reset_->today_guild_contribute());

	int is_get{ 1 };
	if (need_reset_->today_attendance())
		is_get = -1;
	pto->set_online_day_award(online_day_award_ * is_get);
	pto->set_vip_everyday(need_reset_->vip_every_day());
}

void CPlayer::__InitMaxLevel()
{
	for (size_t i = mission_->stage_normal_progress(); i >= 1; i--)
	{
		const CStage* stage{ CStage::GetStage(i, 1) };
		if (stage && stage->GetUnlockLevel())
		{
			int temp_level{ stage->GetUnlockLevel() };
			max_level_ = max_level_ > temp_level ? max_level_ : temp_level;
			return;
		}
	}
}

void CPlayer::__GetMeditation()
{
	int minutes{ static_cast<int>(GetDurationCount(time(0), meditation_time_, DURATION_TYPE::MINUTE)) };
	if (0 == minutes)
		return;

	__int64 exp_per_min = CRootData::GetRootData(level(), RootDataType::RDT_MeditationExp);

	const CVIPFun* vip_fun{ CVIPFun::GetVIPFun(vip_level_) };
	if (!vip_fun)
	{
		RECORD_WARNING(FormatString("VIP等级错误,pid:", pid_, ",vip_level:", vip_level_));
		return;
	}

	int max_minutes{ vip_fun->GetMeditationTime() * 60 };

	minutes = (minutes < max_minutes ? minutes : max_minutes);

	ChangeExp(minutes * exp_per_min);

	meditation_time_ = time(0);

	pto_PLAYER_S2C_RES_GetMeditation pto;
	pto.set_minutes(minutes);
	SendToAgent(pto, PLAYER_S2C_RES_GetMeditation);
}

void CPlayer::__OnPlayerInfo()
{
	need_reset_->SendInitialMessage();
	mission_->SendInitialMessage();
	knapsack_->SendInitialMessage();
	army_->SendInitialMessage();
	offline_->SendInitialMessage();
	exercise_->SendInitialMessage();

	__SendOnlineBoxInfo();
	__SendFriendInfo();
	__SendStageInfo();

	pto_PLAYER_S2C_RES_PlayerInfoZ pto;
	pto.set_pid(pid());
	__SetProtobuff(pto.mutable_player_data());
	SendToAgent(pto, PLAYER_S2C_RES_PlayerInfo);

	CheckOrder();
}

void CPlayer::__SendOnlineBoxInfo()
{
	need_reset_->SendOnlineBoxInfo();
}

void CPlayer::__SendStageInfo()
{
	//关卡
	pto_STAGE_S2C_NTF_PLAYER_STAGE_PROGRESS pto_stage;
	pto_stage.set_normal_progress(mission_->stage_normal_progress());
	pto_stage.set_hard_progress(mission_->stage_hard_progress());
	pto_stage.set_nightmare_progress(mission_->stage_nightmare_progress());
	SendToAgent(pto_stage, STAGE_S2C_NTF_StageProgress);

	//精英关卡
	need_reset_->UpdateEliteStage();
	need_reset_->UpdateMultiStage();

	//通关宝箱
	pto_PLAYER_S2C_NTF_UptateTownBox pto_box;
	for (size_t i = 1; i <= GAME_WORLD()->TownNum(); i++)
	{
		pto_box.add_town_box_state(GetBoxState(i, StageType::NORMAL));
		pto_box.add_town_box_state(GetBoxState(i, StageType::HARD));
		pto_box.add_town_box_state(GetBoxState(i, StageType::NIGHTMARE));
	}
	SendToAgent(pto_box, PLAYER_S2C_NTF_UptateTownBox);
}

void CPlayer::__SendFriendInfo()
{
	pto_SOCIAL_S2C_NTF_UpdateFriendsList pto;

	for (auto &it : friends_list_)
		__SetContactsProtocol(it, pto.add_friend_list());
	for (auto &it : black_list_)
		__SetContactsProtocol(it, pto.add_black_list());
	for (auto &it : recents_list_)
		__SetContactsProtocol(it, pto.add_contact_list());

	SendToAgent(pto, SOCIAL_S2C_NTF_UpdateFriendsList);
}

void CPlayer::__GetAllTitle()
{
	pto_PLAYER_S2C_RES_AllTitles pto;
	LOCK_BLOCK(title_lock_);
	for (auto &it : have_titles_)
		pto.add_title_id(it);
	SendToAgent(pto, PLAYER_S2C_RES_AllTitles);
}

void CPlayer::__UseTitle(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_UseTitle pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int title_id = pto.title_id();

	if (title_id == using_title())
		return;

	{
		LOCK_BLOCK(title_lock_);

		if (have_titles_.find(title_id) == have_titles_.cend() && title_id)
		{
			RECORD_WARNING(FormatString("找不到称号ID:", title_id));
			return;
		}
	}

	using_title_ = title_id;

	pto_PLAYER_S2C_RES_UseTitle pto_res;
	pto_res.set_res(0);
	pto_res.set_title_id(using_title());
	SendToAgent(pto_res, PLAYER_S2C_RES_UseTitle);

	CTown* town{ GAME_WORLD()->FindTown(town_id()) };
	if (town)
	{
		pto_TOWN_S2C_NTF_ChangeTitle pto_change;
		pto_change.set_pid(pid());
		pto_change.set_titleid(using_title());
		std::string str;
		pto_change.SerializeToString(&str);
		town->SendAllPlayersInTown(str, MSG_S2C, TOWN_S2C_NTF_ChangeTitle, pid());
	}
}

void CPlayer::__OnPlayerBase(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_PlayerBase pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (pto.pid() > 0)
	{
		SPIPlayerData data = PLAYER_DATA(pto.pid());

		if (data)
		{
			pto_PLAYER_S2C_RES_PlayerBase pto_res;
			pto_res.set_pid(data->pid());
			pto_res.set_sid(data->sid());
			pto_res.set_name(data->name());
			pto_res.set_sex(data->sex());
			SendToAgent(pto_res, PLAYER_S2C_RES_PlayerBase);
		}
	}
	else
	{
		COffLinePlayer* ai = COffLinePlayer::FindOffLinePlayerByPID(pto.pid());

		if (ai)
		{
			pto_PLAYER_S2C_RES_PlayerBase pto_res;
			pto_res.set_pid(ai->GetPID());
			pto_res.set_sid(sid());
			pto_res.set_name(ai->GetName());
			pto_res.set_sex(ai->GetSex());
			SendToAgent(pto_res, PLAYER_S2C_RES_PlayerBase);
		}
	}
}

void CPlayer::__OnPlayerBaseEx(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_PlayerBaseEx pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (pto.pid() > 0)
	{
		SPIPlayerData data = PLAYER_DATA(pto.pid());

		if (data)
		{
			pto_PLAYER_S2C_RES_PlayerBaseEx pto_res;
			pto_res.set_pid(data->pid());
			pto_res.set_level(data->level());
			pto_res.set_town_id(data->town_id());
			pto_res.set_guild_id(data->guild_id());
			pto_res.set_last_offline_time(data->offline_time());
			SendToAgent(pto_res, PLAYER_S2C_RES_PlayerBaseEx);
		}
	}

	else
	{
		COffLinePlayer* ai = COffLinePlayer::FindOffLinePlayerByPID(pto.pid());

		if (ai)
		{
			pto_PLAYER_S2C_RES_PlayerBaseEx pto_res;
			pto_res.set_pid(ai->GetPID());
			pto_res.set_level(ai->GetLevel());
			pto_res.set_town_id(0);
			pto_res.set_guild_id(0);
			pto_res.set_last_offline_time(time(0));
			SendToAgent(pto_res, PLAYER_S2C_RES_PlayerBase);
		}
	}
}

void CPlayer::__OnPLayerDetailInfo(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_PlayerDetailInfo pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_PLAYER_S2C_RES_PlayerDetailInfo pto_res;

	if (pto.pid() > 0)
	{
		auto data = PLAYER_DATA(pto.pid());

		if (!data)
			return;

		pto_res.set_pid(data->pid());
		pto_res.set_name(data->name());

		CGuild* guild = GAME_WORLD()->FindGuild(data->guild_id());
		if (guild)
			pto_res.set_guild(guild->name());
		else
			pto_res.set_guild("");

		pto_res.set_level(data->level());
		pto_res.set_sex(data->sex());
		pto_res.set_titleid(data->using_title());
		pto_res.set_reputation(data->reputation());

		pto_res.set_clothes(data->clothes());
		pto_res.set_offline_battle_rank(GAME_WORLD()->GetOfflinePlayerRank(data->pid()));//TODO
		pto_res.set_offline_battle_win_streak(data->offline_battle_win_streak());

		for (size_t i = 0; i < kMaxHeroNum; i++)
		{
			const CHeroCard* hero_card = data->hero(i);

			if (hero_card)
			{
				auto pto_hero = pto_res.add_heroes();
				pto_hero->set_hero_model_id(hero_card->hero_id());
				pto_hero->set_train_str(hero_card->train_str());
				pto_hero->set_train_cmd(hero_card->train_cmd());
				pto_hero->set_train_int(hero_card->train_int());
				pto_hero->set_used(hero_card->today_is_internal_affair());
				pto_hero->set_level(hero_card->hero_level());
				pto_hero->set_quality(hero_card->quality());

				for (auto &it_rune : hero_card->GetRunes())
				{
					auto pto_rune = pto_hero->add_rune();
					pto_rune->set_rune_id(CRuneLibrary::GetRuneID(it_rune));
					if (pto_rune->rune_id() != Broke_Rune_ID)
						pto_rune->set_colour(CRuneLibrary::GetRuneColor(it_rune));
					else
						pto_rune->set_colour(0);
					pto_rune->set_lock(CRuneLibrary::HasLock(it_rune));
				}

				std::vector<const CEquip*> equips = data->hero_equip(hero_card->hero_id());

				for (auto &it_equip : equips)
				{
					auto pto_equip = pto_res.add_equips();
					pto_equip->set_id(it_equip->GetID());
					pto_equip->set_lv(it_equip->level());
					pto_equip->set_hero_id(hero_card->hero_id());
					for (auto &it_gem : it_equip->GetGems())
						pto_equip->add_jewel(it_gem);
					pto_equip->set_quality(it_equip->quality());
					pto_equip->set_suit_id(it_equip->suit_id());
					pto_equip->set_uniqueid(it_equip->unique_id());
					pto_equip->set_enchant(it_equip->enchanting1());
					pto_equip->set_enchant_value(it_equip->enchanting1_value());
					pto_equip->set_enchant2(it_equip->enchanting2());
					pto_equip->set_enchant2_value(it_equip->enchanting2_value());
					pto_equip->set_enchant2_is_active(it_equip->enchanting2_is_active());
				}
			}
		}

		for (size_t i = 0; i < kSoldierTechCount; i++)
			pto_res.add_technology(data->technology(i));

		pto_res.set_speed_all_rank(data->speed_best_rank());

		const SpeedRankInfo* speed_info{ GAME_WORLD()->GetSpeedStageRankingDataByPID(data->pid()) };

		if (speed_info)
		{
			pto_res.set_speed_week_rank(GAME_WORLD()->GetSpeedStageRank(data->pid()));
			pto_res.set_speed_time(speed_info->time);
			for (auto &it : speed_info->heroes)
				pto_res.add_speed_heroes(it);
			for (auto &it : speed_info->soldiers)
				pto_res.add_speed_soldiers(it);
		}

		for (size_t i = 0; i < kBattleRecordCount; i++)
		{
			const BattleRecord battle_record = data->battle_record((BattleRecordType)i);

			auto data = pto_res.add_datas();
			data->set_point(battle_record.point);
			data->set_s(battle_record.S);
			data->set_a(battle_record.A);
			data->set_b(battle_record.B);
			data->set_c(battle_record.C);
			data->set_win(battle_record.win);
			data->set_lose(battle_record.lose);

			if (BattleRecordType::kBattleRecordWar)
				data->set_week_times(need_reset_->week_war_times());
			else if (BattleRecordType::kBattleRecordWar)
				data->set_week_times(need_reset_->week_1v1_times());
			else if (BattleRecordType::kBattleRecordWar)
				data->set_week_times(need_reset_->week_3v3_times());
		}
	}
	else
	{
		auto ai = COffLinePlayer::FindOffLinePlayerByPID(pto.pid());
		if (!ai)
			return;

		pto_res.set_pid(ai->GetPID());
		pto_res.set_name(ai->GetName());
		pto_res.set_guild("");

		pto_res.set_level(ai->GetSex());
		pto_res.set_sex(ai->GetSex());
		pto_res.set_titleid(0);
		pto_res.set_reputation(0);
		pto_res.set_clothes(0);
		pto_res.set_offline_battle_rank(GAME_WORLD()->GetOfflinePlayerRank(ai->GetPID()));//TODO
		pto_res.set_offline_battle_win_streak(0);

		for (size_t i = 0; i < kMaxHeroNum; i++)
		{
			const CHeroCard* hero_card = ai->GerHeroCard(i);

			if (hero_card)
			{
				auto pto_hero = pto_res.add_heroes();
				pto_hero->set_hero_model_id(hero_card->hero_id());
				pto_hero->set_train_str(hero_card->train_str());
				pto_hero->set_train_cmd(hero_card->train_cmd());
				pto_hero->set_train_int(hero_card->train_int());
				pto_hero->set_used(hero_card->today_is_internal_affair());
				pto_hero->set_level(hero_card->hero_level());
				pto_hero->set_quality(hero_card->quality());

				for (auto &it_rune : hero_card->GetRunes())
				{
					auto pto_rune = pto_hero->add_rune();
					pto_rune->set_rune_id(CRuneLibrary::GetRuneID(it_rune));
					if (pto_rune->rune_id() != Broke_Rune_ID)
						pto_rune->set_colour(CRuneLibrary::GetRuneColor(it_rune));
					else
						pto_rune->set_colour(0);
					pto_rune->set_lock(CRuneLibrary::HasLock(it_rune));
				}
			}
		}

		for (size_t i = 0; i < kSoldierTechCount; i++)
			pto_res.add_technology(ai->GetLevel());

		pto_res.set_speed_all_rank(0);

		pto_res.set_speed_week_rank(0);
		pto_res.set_speed_time(0);

		for (size_t i = 0; i < kBattleRecordCount; i++)
		{
			auto data = pto_res.add_datas();
			data->set_point(0);
			data->set_s(0);
			data->set_a(0);
			data->set_b(0);
			data->set_c(0);
			data->set_win(0);
			data->set_lose(0);
			data->set_week_times(0);
		}
	}

	SendToAgent(pto_res, PLAYER_S2C_RES_PlayerDetailInfo);
}

void CPlayer::__GetTownBox(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_GetTownBox pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	const CTownBox* pBox{ CTownBox::GetTownBox(pto.town_id(), pto.stage_difficulty()) };
	if (nullptr == pBox)
		return;

	StageType stage_type{ static_cast<StageType>(pto.stage_difficulty()) };

	int stage_progress{ 0 };

	switch (stage_type)
	{
	case StageType::NORMAL:stage_progress = mission_->stage_normal_progress(); break;
	case StageType::HARD:stage_progress = mission_->stage_hard_progress(); break;
	case StageType::NIGHTMARE:stage_progress = mission_->stage_nightmare_progress(); break;
	}

	pto_PLAYER_S2C_RES_GetTownBox pto_res;

	if (stage_progress < pBox->GetStageLevel())
	{
		pto_res.set_res(2);
	}
	else if (GetBoxState(pto.town_id(), stage_type))
	{
		pto_res.set_res(1);
	}
	else
	{
		pto_res.set_res(0);
		ImplementMission(MTT_GetTownBox, 0, 0);
		__SetBoxState(pto.town_id(), stage_type, true);
		ChangeSilver(pBox->GetSilver());
		ChangeGold(pBox->GetGold(), 5);
		ChangeHonor(pBox->GetHonour());
		ChangeStamina(pBox->GetStamina());
	}

	pto_res.set_towen_id(pto.town_id());
	pto_res.set_stage_difficult(pto.stage_difficulty());
	SendToAgent(pto_res, PLAYER_S2C_RES_GetTownBox);
}

void CPlayer::__SetBoxState(int town_id, StageType type, bool is_get)
{
	if (town_id > 32)
	{
		RECORD_WARNING("城镇ID错误");
		return;
	}

	int i{ 1 };
	for (; i < town_id; i *= 2);

	int* num{ nullptr };

	switch (type)
	{
	case StageType::NORMAL:
		num = &box_normal_progress_;
		break;
	case StageType::HARD:
		num = &box_hard_progress_;
		break;
	case StageType::NIGHTMARE:
		num = &box_nightmare_progress_;
		break;
	default:
		return;
	}

	if (is_get)
		*num |= i;
	else
		*num &= ~i;
}

void CPlayer::__UpdateTownTop10(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_UpdateTownStageRank pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	CTown* town{ GAME_WORLD()->FindTown(pto.town_id()) };
	if (!town)
		return;
	pto_PLAYER_S2C_RES_UpdateTownStageRank pto_res;
	town->SetTop10Protocol(&pto_res);
	SendToAgent(pto_res, PLAYER_S2C_RES_UpdateTownStageRank);
}

void CPlayer::__CreateGuild(const CMessage& msg)
{
	pto_GUILD_C2S_REQ_CreateGuild pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int result{ 0 };
	int need_gold{ 100 };

	if (guild_id_)
	{
		result = 3;
	}
	else if (!CGuild::CheckGuildName(pto.guild_name()))
	{
		result = 1;
	}
	else if (gold_ < need_gold)
	{
		result = 2;
	}
	else
	{
		try
		{
			guild_id_ = GAME_WORLD()->CreateGuild(pto.guild_name(), pid_);
			ChangeGold(-need_gold, 6);
			mission()->ImplementMission(EMissionTargetType::MTT_CreateOrJoinGuild, 0, 0);
		}
		catch (int ex)
		{
			guild_id_ = 0;
			result = ex;
		}
	}

	pto_GUILD_S2C_RES_CreateGuild pto_res;
	pto_res.set_result(result);
	pto_res.set_guild_id(guild_id_);
	SendToAgent(pto_res, GUILD_S2C_RES_CreateGuild);
}

void CPlayer::__GetGuildInfo()
{
	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);

	pto_GUILD_S2C_RES_GetGuildInfo pto_res;

	if (!guild)
	{
		pto_res.set_result(1);
	}
	else if (GuildPosition::NOT_MEMBER == guild->GetPosition(pid_))
	{
		pto_res.set_result(2);
	}
	else
	{
		pto_res.set_result(0);
		guild->SetProtocol(pto_res.mutable_guild(), pid_);
	}

	SendToAgent(pto_res, GUILD_S2C_RES_GetGuildInfo);
}

void CPlayer::__FindGuild(const CMessage& msg)
{
	pto_GUILD_C2S_REQ_FindGuild pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	auto guilds = GAME_WORLD()->FindGuild(pto.guild_name());

	pto_GUILD_S2C_RES_FindGuild pto_res;

	for (auto &it : guilds)
	{
		if (it->is_dissolve())
			continue;

		auto pto_guild = pto_res.add_guilds();
		pto_guild->set_guild_id(it->guild_id());
		pto_guild->set_guild_name(it->name());
		pto_guild->set_guild_level(it->level());
		pto_guild->set_member_num(it->member_num());
		pto_guild->set_is_apply(it->IsInApplyList(pid_));
		pto_guild->set_ranking(GAME_WORLD()->GetGuildRanking(it->guild_id()));
		SPIPlayerData data = PLAYER_DATA(it->chairman());
		if (data)
			pto_guild->set_chairman(data->name());
	}

	SendToAgent(pto_res, GUILD_S2C_RES_FindGuild);
}

void CPlayer::__ApplyGuild(const CMessage& msg)
{
	if (guild_id_)
		return;

	pto_GUILD_C2S_REQ_ApplyGuild pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CGuild* guild = GAME_WORLD()->FindGuild(pto.guild_id());
	if (guild)
		guild->ApplyForJoin(pid_);
	else
		RECORD_TRACE("没有找到公会");
}

void CPlayer::__AgreeApply(const CMessage& msg)
{
	if (!guild_id_)
		return;

	pto_GUILD_C2S_NTF_AgreeApply pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);
	if (guild)
		guild->AgreeApply(pid_, pto.pid(), pto.is_agree());
	else
		RECORD_WARNING("没有找到公会");
}

void CPlayer::__ChangeGuildAccess(const CMessage& msg)
{
	if (!guild_id_)
		return;

	int result = 0;

	pto_GUILD_C2S_REQ_ChangeAccess pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
	{
		result = guild->ChangeAccess(pid_, pto.pid(), pto.is_up());
	}
	else
	{
		result = 3;
		RECORD_WARNING("没有找到公会");
	}

	pto_GUILD_S2C_RES_ChangeAccess pto_res;
	pto_res.set_result(result);
	SendToAgent(pto_res, GUILD_S2C_RES_ChangeAccess);
}

void CPlayer::__QuitGuild()
{
	if (!guild_id_)
		return;

	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild && guild->QuitGuild(pid_))
		guild_id_ = 0;
	else
		RECORD_WARNING("没有找到公会");
}

void CPlayer::__GuildKick(const CMessage& msg)
{
	if (!guild_id_)
		return;

	pto_GUILD_C2S_REQ_Kick pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
		guild->Kick(pid_, pto.pid());
	else
		RECORD_WARNING("没有找到公会");
}

void CPlayer::__CessionGuide(const CMessage& msg)
{
	if (!guild_id_)
		return;

	pto_GUILD_C2S_REQ_Cession pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
		guild->Cession(pid_, pto.pid());
	else
		RECORD_WARNING("没有找到公会");
}

void CPlayer::__GetGuildName(const CMessage& msg)
{
	pto_GUILD_C2S_REQ_GuildName pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CGuild* guild = GAME_WORLD()->FindGuild(pto.guild_id());

	if (guild)
	{
		pto_GUILD_S2C_RES_GuildName pto_res;
		pto_res.set_guild_id(pto.guild_id());
		pto_res.set_guild_name(guild->name());
		SendToAgent(pto_res, GUILD_S2C_RES_GuildName);
	}
}

void CPlayer::__GuildContribute()
{
	if (!guild_id_)
		return;

	pto_GUILD_S2C_RES_Contribute pto_res;
	pto_res.set_result(0);
	int need_gold{ 0 };
	if (need_reset()->today_guild_contribute() >= CVIPFun::GetVIPFun(vip_level(), VIPFunType::VFT_DonationGold))
	{
		pto_res.set_result(1);
	}
	else
	{
		need_gold = CGoldSpend::GetGoldSpend(need_reset()->today_guild_contribute() + 1, GoldSpendType::GST_GuildContributeSpend);
		if (need_gold > gold())
			pto_res.set_result(2);
	}

	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
	{
		guild->Contribute(pid(), 5, 1);
	}
	else
	{
		pto_res.set_result(3);
		RECORD_WARNING("没有找到公会");
	}

	if (0 == pto_res.result())
	{
		need_reset()->change_today_guild_contribute(1);
		ChangeGold(-need_gold, 7);
		mission()->ImplementMission(MTT_GuildContribute, 0, 0);
		need_reset()->ImplementRewardMission(RewardMissionType::RMT_Guild, 0, 1);
	}

	SendToAgent(pto_res, GUILD_S2C_RES_Contribute);
}

void CPlayer::__GuildEditNotification(const CMessage& msg)
{
	if (!guild_id_)
		return;

	pto_GUILD_C2S_REQ_ChangeNotification pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);

	if (guild)
	{
		pto_GUILD_S2C_RES_ChangeNotification pto_res;
		pto_res.set_result(guild->EditNotice(pid_, pto.content()));
		pto_res.set_content(guild->notice());
		SendToAgent(pto_res, GUILD_S2C_RES_ChangeNotification);
	}
	else
	{
		RECORD_WARNING("没有找到公会");
	}
}

void CPlayer::__AddFriendsOrBlack(const CMessage& msg)
{
	pto_SOCLIAL_C2S_REQ_AddFriend pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	bool type{ false };
	if (pto.type())
		type = true;

	int result{ 0 };
	int target_pid{ 0 };

	if (pto.p_id())
	{
		SPIPlayerData data = PLAYER_DATA(pto.p_id());
		if (!data)
			return;
		target_pid = data->pid();
	}
	else
	{
		target_pid = GAME_WORLD()->FindPIDByName(pto.name().c_str());
	}

	pto_SOCLIAL_S2C_RES_AddFriend pto_res;
	pto_res.set_type(type);

	if (0 == target_pid)
	{
		result = 1;
	}
	else if (pid() == target_pid)
	{
		result = 4;
	}
	else
	{
		LOCK_BLOCK(social_lock_);

		std::set<int>*  list{ nullptr };

		if (type)
			list = &black_list_;
		else
			list = &friends_list_;

		if (false == type && (*list).size() >= 50)
		{
			result = 3;
		}
		else if (false == (*list).insert(target_pid).second)
		{
			result = 2;
		}
		else
		{
			SPIPlayerData data = PLAYER_DATA(target_pid);
			if (!data)
				return;
			auto frined_info = pto_res.mutable_friend_info();
			frined_info->set_p_id(target_pid);
			frined_info->set_lv(data->level());
			frined_info->set_map_id(data->town_id());
			frined_info->set_name(data->name());
		}
	}
	pto_res.set_res(result);
	SendToAgent(pto_res, SOCIAL_S2C_RES_AddFriend);

	if (0 == result)
	{
		std::ostringstream sql;
		sql << "replace into tb_player_friend values(" << pid_ << "," << target_pid << "," << type << ")";
		MYSQL_UPDATE(sql.str());

		LOCK_BLOCK(social_lock_);

		std::set<int>*  other_list{ nullptr };
		if (type)
			other_list = &friends_list_;
		else
			other_list = &black_list_;

		if ((*other_list).erase(target_pid))
		{
			pto_SOCLIAL_S2C_RES_DeleteFriend pto_delete;
			pto_delete.set_p_id(target_pid);

			if (type)
				pto_delete.set_type(0);
			else
				pto_delete.set_type(1);

			pto_delete.set_res(result);

			SendToAgent(pto_delete, SOCIAL_S2C_RES_DeleteFriend);
		}
	}
}

void CPlayer::__DeleteFriendsOrBlack(const CMessage& msg)
{
	pto_SOCLIAL_C2S_REQ_DeleteFriend pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int target_pid{ pto.p_id() };

	bool type{ false };
	if (pto.type())
		type = true;

	LOCK_BLOCK(social_lock_);

	std::set<int>*  list{ nullptr };
	if (type)
		list = &black_list_;
	else
		list = &friends_list_;

	pto_SOCLIAL_S2C_RES_DeleteFriend pto_res;
	pto_res.set_p_id(target_pid);
	pto_res.set_type(type);

	if ((*list).erase(target_pid))
	{
		pto_res.set_res(0);

		std::ostringstream sql;
		sql << "delete from tb_player_friend where pid = " << pid() << " and  target_pid = " << target_pid;
		MYSQL_UPDATE(sql.str());
	}
	else
	{
		pto_res.set_res(1);
	}

	SendToAgent(pto_res, SOCIAL_S2C_RES_DeleteFriend);
}

void CPlayer::__SetContactsProtocol(int pid, pto_SOCIALStruct_FriendInfo* pto)
{
	SPIPlayerData data = PLAYER_DATA(pid);
	if (!data)
		return;
	pto->set_p_id(pid);
	pto->set_name(data->name());
	pto->set_lv(data->level());
	pto->set_map_id(data->town_id());
}

void CPlayer::__ChatMessage(const CMessage& msg)
{
	if (!IsOpenGuide(40))
		return;

	pto_CHAT_C2S_NTF_World pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_CHAT_S2C_NTF_World pto_res;

	switch (pto.channel())
	{
	case Channel::All:
		break;
	case Channel::World:
	{
		if (time(0) - last_world_chat_time_ < 2)
			return;
		else
			last_world_chat_time_ = time(0);

		pto_res.set_msg(pto.msg());
		pto_res.set_senderpid(pid());
		pto_res.set_channel(pto.channel());
		std::string str;
		pto_res.SerializeToString(&str);
		GAME_WORLD()->SendToAllPlayers(str, MSG_S2C, CHAT_S2C_NTF_World);
	}
		break;
	case Channel::Private:
	{
		SPPlayer target_player{ FIND_PLAYER(pto.targetpid()) };

		if (target_player)
		{
			pto_res.set_msg(pto.msg());
			pto_res.set_senderpid(pid());
			pto_res.set_channel(pto.channel());
			target_player->SendToAgent(pto_res, CHAT_S2C_NTF_World);
			target_player->AddRecents(pid());
			AddRecents(target_player->pid());
		}
	}
		break;
	case Channel::Team:
	{
		if (room_id_)
		{
			auto room = GAME_WORLD()->FindRoom(room_id_);
			if (room)
			{
				pto_res.set_msg(pto.msg());
				pto_res.set_senderpid(pid());
				pto_res.set_channel(pto.channel());
				std::string str;
				pto_res.SerializeToString(&str);
				room->SendToAllPlayerInRoom(str, MSG_S2C, CHAT_S2C_NTF_World, 0);
			}
		}
	}
		break;
	case Channel::Union:
	{
		if (guild_id_)
		{
			CGuild* guild = GAME_WORLD()->FindGuild(guild_id_);
			if (guild)
			{
				pto_res.set_msg(pto.msg());
				pto_res.set_senderpid(pid());
				pto_res.set_channel(pto.channel());
				std::string str;
				pto_res.SerializeToString(&str);
				guild->SendToAllPlayers(str, MSG_S2C, CHAT_S2C_NTF_World, GuildPosition::RANK_AND_FILE);
			}
		}
	}
		break;
	}
}

void CPlayer::__RoomCreate(const CMessage& msg)
{
	if (room_id())
	{
		RECORD_TRACE("玩家已经在房间中,无法创建新房间");
		return;
	}

	int result{ 0 };

	pto_ROOM_C2S_REQ_Create pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	if ((BattleMode)pto.roomtype() == BattleMode::MULTISTAGE)
	{
		const CEliteStage* stage = CEliteStage::GetEliteStageByLevel(pto.mapid());
		if (stage && stage->GetOpenLevel() > level())
			return;
		if (stage && stage->GetLevel() > mission()->stage_multi_progress() + 1)
			return;
	}

	pto_ROOM_S2C_RES_Create pto_res;

	std::string room_name;

	if (pto.name().empty() || pto.name().length() > 32)
	{
		room_name.assign(name());
		room_name.append(ANSI_to_UTF8("的房间"));
	}

	SPRoomZ room{ GAME_WORLD()->CreateNewRoom(pid_, (BattleMode)pto.roomtype(), pto.mapid(), room_name.c_str(), pto.password().c_str()) };

	if (room)
	{
		room_id_ = room->room_id();
		room->SetProtocol(pto_res.mutable_roominfo());
	}
	else
	{
		result = 1;
	}

	pto_res.set_result(result);
	SendToAgent(pto_res, ROOM_S2C_RES_Create);
}

void CPlayer::__RoomList(const CMessage& msg)
{
	pto_ROOM_C2S_REQ_RoomList pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_ROOM_S2C_RES_RoomList pto_res;
	GAME_WORLD()->SetRoomListProtocol(&pto_res, pto.map_id(), pto.mode());
	SendToAgent(pto_res, ROOM_S2C_RES_RoomList);
}

void CPlayer::__RoomJoin(const CMessage& msg)
{
	if (room_id())
	{
		RECORD_TRACE("已经在房间中，无法加入房间");
		return;
	}

	pto_ROOM_C2S_REQ_Join pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	SPRoomZ room = GAME_WORLD()->FindRoom(pto.roomid());

	pto_ROOM_S2C_RES_Join pto_res;

	int result = 0;

	if (room.get())
	{
		result = room->JoinRoom(pid_, pto.password());

		if (0 == result)
		{
			room->SetProtocol(pto_res.mutable_roominfo());
			room_id_ = room->room_id();
		}
	}
	else
	{
		result = 3;
	}

	pto_res.set_result(result);
	SendToAgent(pto_res, ROOM_S2C_RES_Join);
}

void CPlayer::__RoomQuit()
{
	if (!room_id_)
		return;

	SPRoomZ room = GAME_WORLD()->FindRoom(room_id_);

	if (room.get())
	{
		room->QuitRoom(pid());
	}
	else
	{
		RECORD_WARNING("退出房间时找不到房间");
	}

	room_id_ = 0;
}

void CPlayer::__RoomLock(const CMessage& msg)
{
	SPRoomZ room = GAME_WORLD()->FindRoom(room_id_);

	if (!room)
	{
		RECORD_TRACE("找不到需要加锁的房间");
		return;
	}

	pto_ROOM_C2S_REQ_Lock pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	room->SetRoomIsLock(pto.is_lock(), pto.password(), pid());
}

void CPlayer::__RoomKick(const CMessage& msg)
{
	SPRoomZ room = GAME_WORLD()->FindRoom(room_id_);

	if (!room)
	{
		RECORD_TRACE("找不到需要踢人的房间");
		return;
	}

	pto_ROOM_C2S_REQ_Kick pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	room->KickPlayer(pid(), pto.slotid());
}

void CPlayer::__RoomReady(const CMessage& msg)
{
	SPRoomZ room = GAME_WORLD()->FindRoom(room_id_);

	if (!room)
	{
		RECORD_TRACE("找不到需要准备的房间");
		return;
	}

	pto_ROOM_C2S_REQ_Ready pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	room->Ready(pid(), pto.is_ready());
}

void CPlayer::__RoomChangePos(const CMessage& msg)
{
	SPRoomZ room = GAME_WORLD()->FindRoom(room_id_);

	if (!room)
	{
		RECORD_TRACE("找不到需要交换位置的房间");
		return;
	}

	pto_ROOM_C2S_REQ_ChangePos pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	room->ChangePos(pid(), pto.nslotid());
}

void CPlayer::__RoomStart()
{
	SPRoomZ room = GAME_WORLD()->FindRoom(room_id_);

	if (!room)
	{
		RECORD_TRACE("找不到需要准备的房间");
		return;
	}

	room->Start(pid_);
}

void CPlayer::__StartWar()
{
	__RoomQuit();

	if (!GAME_WORLD()->HasBattleServerConnect())
		return;

	if (PlayerState::NORMAL != state())
		return;

	state_ = PlayerState::MATCH;

	pto_BATTLE_S2B_REQ_MatchBattle_Ex pto;
	pto.set_is_match(true);
	pto.set_match_mode((int)BattleMode::WAR);
	pto.set_sid(GAME_WORLD()->sid());
	pto.set_point(point_war());
	SetBattleMasterProto(pto.add_master(), false);
	std::string str;
	pto.SerializeToString(&str);
	GAME_WORLD()->SendToBattleServer(str, MSG_S2B, BATTLE_S2B_REQ_MatchBattle, pid());

	pto_ROOM_S2C_NTF_MatchBattle pto_match;
	pto_match.set_is_match(true);
	SendToAgent(pto_match, ROOM_S2C_NTF_MatchBattle);
}

void CPlayer::__TestCommand(const CMessage& msg)
{
	pto_DEBUG_C2S_COMMAND pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	std::string message{ pto.msg() };

#ifndef _DEBUG
	__GMCommand(message);
#else
	__GMCommand(message);

	int index{ static_cast<int>(message.find_first_of(" ")) };

	if (index == -1)
	{
		if (message == "allequips")
		{
			for (size_t i = 101; i <= 106; i++)
				knapsack()->GiveNewEquip(i, 0);
		}
		else if (message == "allheroes")
		{
			army()->GiveAllHero();
		}
		else if (message == "allsoldiers")
		{
			army()->GiveAllSoldier();
		}
		else if (message == "mc")
		{
			mission()->ImplementMission(EMissionTargetType::MTT_AC, 0, 0);
		}
		else if (message == "resetday")
		{
			GAME_WORLD()->ResetDay();
		}
		else if (message == "wuss")
			GAME_WORLD()->__WindUpSpeedStage();
		else if (message == "iamgod")
		{
			ChangeSilver(999999999999);
			ChangeGold(999999999, 8);
			ChangeHonor(999999999);
			ChangeReputation(999999999);

			SetLevel(kMaxLevel);

			ChangeVIPValue(99999999);

			army()->SetAllTechnology(kMaxLevel);

			for (size_t i = 701; i <= 706; i++)
				knapsack()->GiveNewEquip(i, GetRandom(2, 5));

			army()->GiveAllHero();
			army()->SetAllHeroesLevel(kMaxLevel);
			army()->GiveAllSoldier();

			ChangeExp(99999999999);
		}
		else if (message == "ti")
		{
			for (size_t i = 60011; i <= 60016; i++)
				knapsack()->GiveNewItem(i, 3);
			for (size_t i = 60021; i <= 60026; i++)
				knapsack()->GiveNewItem(i, 3);
			for (size_t i = 60031; i <= 60036; i++)
				knapsack()->GiveNewItem(i, 3);

			for (size_t i = 20211; i <= 20214; i++)
				knapsack()->GiveNewItem(i, 99);
			for (size_t i = 20311; i <= 20314; i++)
				knapsack()->GiveNewItem(i, 99);
			for (size_t i = 20411; i <= 20414; i++)
				knapsack()->GiveNewItem(i, 99);
		}
	}
	else
	{
		std::string order{ message.substr(0, index) };
		message.erase(0, index + 1);

		if (order.empty() || message.empty())
			return;

		if (order == "goldtickets")
			knapsack()->GiveNewItem(10008, atoi(message.c_str()));
		else if (order == "level")
			SetLevel(atoi(message.c_str()));
		else if (order == "vip")
			ChangeVIPValue(atoi(message.c_str()));
		else if (order == "money")
			ChangeSilver(atoll(message.c_str()));
		else if (order == "gold")
			ChangeGold(atoi(message.c_str()), 9);
		else if (order == "technology")
			army()->SetAllTechnology(atoi(message.c_str()));
		else if (order == "exp")
			ChangeExp(atoll(message.c_str()));
		else if (order == "honor")
			ChangeHonor(atoi(message.c_str()));
		else if (order == "reputation")
			ChangeReputation(atoi(message.c_str()));
		else if (order == "mt")
			mission()->SetMission(atoi(message.c_str()));
		else if (order == "herolevel")
			army()->SetAllHeroesLevel(atoi(message.c_str()));
		else if (order == "runes")
			army()->GiveAllRunes(atoi(message.c_str()));
		else if (order == "makeequip")
			knapsack()->GiveNewEquip(atoi(message.c_str()), 2);
		else if (order == "fashion")
			knapsack()->AddFashion(atoi(message.c_str()));
		else if (order == "stamina")
			ChangeStamina(atoi(message.c_str()));
		else if (order == "suit")
			knapsack()->GiveSuit(atoi(message.c_str()));
		else if (order == "stage")
		{
			int iteminfo[2]{0};

			int nStrIndex{ 0 };
			int nInfoIndex{ 0 };

			while (message.size() > 0)
			{
				nStrIndex = message.find_first_of(",");

				if (-1 != nStrIndex)
				{
					iteminfo[nInfoIndex] = atoi(message.substr(0, nStrIndex).c_str());
					message.erase(0, nStrIndex + 1);
				}
				else
				{
					iteminfo[nInfoIndex] = atoi(message.c_str());
					break;
				}

				nInfoIndex++;
			}
			mission_->SetStageProcess(iteminfo[0], (StageType)iteminfo[1]);
		}
		else if (order == "item")
		{
			int iteminfo[2]{0};

			int nStrIndex{ 0 };
			int nInfoIndex{ 0 };

			while (message.size() > 0)
			{
				nStrIndex = message.find_first_of(",");

				if (-1 != nStrIndex)
				{
					iteminfo[nInfoIndex] = atoi(message.substr(0, nStrIndex).c_str());
					message.erase(0, nStrIndex + 1);
				}
				else
				{
					iteminfo[nInfoIndex] = atoi(message.c_str());
					break;
				}

				nInfoIndex++;
			}
			knapsack()->GiveNewItem(iteminfo[0], iteminfo[1], true);
		}

	}
#endif
}

void CPlayer::__GMCommand(std::string& message)
{
	if (PlayerAccess::kAccessGM > access_)
		return;

	int index{ static_cast<int>(message.find_first_of(" ")) };

	if (index != -1)
	{
		std::string order{ message.substr(0, index) };
		message.erase(0, index + 1);

		if (order.empty() || message.empty())
			return;

		if (order == "rs")
		{
			SendResetServer(atoi(message.c_str()));
		}
		else if (order == "smsg")
		{
			GAME_WORLD()->Annunciate(message.c_str(), 0, 0, AnnunciateType::GMMsg);
		}
		else if (order == "gmmc")
		{
			__GmImplementMission(atoi(message.c_str()));
		}
		else if (order == "kick")
		{
			__KickPlayer(atoi(message.c_str()));
		}
		else if (order == "watch")
		{
			GAME_WORLD()->watch_pid_ = atoi(message.c_str());
		}
		else if (order == "gmgivehero")
		{
			int iteminfo[2]{0};

			int nStrIndex{ 0 };
			int nInfoIndex{ 0 };

			while (message.size() > 0)
			{
				nStrIndex = message.find_first_of(",");

				if (-1 != nStrIndex)
				{
					iteminfo[nInfoIndex] = atoi(message.substr(0, nStrIndex).c_str());
					message.erase(0, nStrIndex + 1);
				}
				else
				{
					iteminfo[nInfoIndex] = atoi(message.c_str());
					break;
				}

				nInfoIndex++;
			}
			__GmGiveHero(iteminfo[0], iteminfo[1]);
		}
		else if (order == "gmrecharge")
		{
			int iteminfo[2]{0};

			int nStrIndex{ 0 };
			int nInfoIndex{ 0 };

			while (message.size() > 0)
			{
				nStrIndex = message.find_first_of(",");

				if (-1 != nStrIndex)
				{
					iteminfo[nInfoIndex] = atoi(message.substr(0, nStrIndex).c_str());
					message.erase(0, nStrIndex + 1);
				}
				else
				{
					iteminfo[nInfoIndex] = atoi(message.c_str());
					break;
				}

				nInfoIndex++;
			}
			__GMRecharge(iteminfo[0], iteminfo[1]);
		}
		else if (order == "frozen")
		{
			int iteminfo[2]{0};

			int nStrIndex{ 0 };
			int nInfoIndex{ 0 };

			while (message.size() > 0)
			{
				nStrIndex = message.find_first_of(",");

				if (-1 != nStrIndex)
				{
					iteminfo[nInfoIndex] = atoi(message.substr(0, nStrIndex).c_str());
					message.erase(0, nStrIndex + 1);
				}
				else
				{
					iteminfo[nInfoIndex] = atoi(message.c_str());
					break;
				}

				nInfoIndex++;
			}
			__FrozenPlayer(iteminfo[0], iteminfo[1]);
		}
	}
}

void CPlayer::__StartStage(const CMessage& msg)
{
	__RoomQuit();

	pto_STAGE_C2S_REQ_START pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int result{ 0 };

	if (pto.stage_difficulty() <= 0 || pto.stage_difficulty() >= 4)
		return;

	int stage_progress{ 0 };

	switch (pto.stage_difficulty())
	{
	case 1:stage_progress = mission_->stage_normal_progress();		break;
	case 2:stage_progress = mission_->stage_hard_progress();		break;
	case 3:stage_progress = mission_->stage_nightmare_progress();	break;
	default:return;
	}

	const CStage* stage{ CStage::GetStage(pto.stage_lv(), pto.stage_difficulty()) };

	if (!stage)
		result = 4;
	else if (stage_progress + 1 < pto.stage_lv())
		result = 2;
	else if (stamina() < kStageStamina)
		result = 3;

	pto_STAGE_S2C_RES_START pto_res;
	pto_res.set_stage_lv(pto.stage_lv());
	pto_res.set_stage_difficulty(pto.stage_difficulty());
	pto_res.set_nres(result);
	SendToAgent(pto_res, STAGE_S2C_RES_StartStage);

	if (0 == result)
		GAME_WORLD()->StartBattle(stage->m_nMapID, BattleMode::STAGE, { pid() }, {}, stage->GetLevel(), stage->GetDifficulty());
}

void CPlayer::__StartEliteStage(const CMessage &msg)
{
	__RoomQuit();

	pto_STAGE_C2S_REQ_StartEliteStage pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int elite_stage_level = pto.stage_lv();

	if (!IsOpenGuide(24))
		return;

	pto_STAGE_S2C_RES_StartEliteStage pto_res;
	pto_res.set_stage_lv(elite_stage_level);
	pto_res.set_res(0);

	const CEliteStage* stage{ CEliteStage::GetEliteStageByLevel(elite_stage_level) };

	if (nullptr == stage)
		pto_res.set_res(3);
	else if (mission()->stage_elite_progress() + 1 < elite_stage_level)
		pto_res.set_res(1);
	else if (stage->GetOpenLevel() > level())
		pto_res.set_res(4);
	else if (need_reset_->IsEliteAlreadyFighted(pto.stage_lv()))
		pto_res.set_res(2);
	else if (stamina() < 20)
		pto_res.set_res(5);

	SendToAgent(pto_res, STAGE_S2C_RES_StartEliteStage);

	if (0 == pto_res.res())
		GAME_WORLD()->StartBattle(stage->m_nMapID, BattleMode::ELITESTAGE, { pid() }, {}, stage->m_nLevel, 0, stage->ProduceBoxNum());
}

void CPlayer::__StartSweepEliteStage(const CMessage &msg)
{
	pto_STAGE_C2S_REQ_StartSweepEliteStage pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_STAGE_S2C_RES_StartSweepEliteStage pto_res;
	pto_res.set_res(0);
	pto_res.set_stage_lv(pto.stage_lv());

	const CEliteStage *stage{ CEliteStage::GetEliteStageByLevel(pto.stage_lv()) };

	int nEliteStageProgress = mission()->stage_elite_progress();

	if (nullptr == stage)
		pto_res.set_res(3);
	else if (nEliteStageProgress < pto.stage_lv())
		pto_res.set_res(1);
	else if (need_reset_->IsEliteAlreadyFighted(pto.stage_lv()))
		pto_res.set_res(5);
	else if (stamina() < 20)
		pto_res.set_res(2);
	else
		sweep_elite_stage_time_ = time(0);

	SendToAgent(pto_res, STAGE_S2C_RES_StartSweepEliteStage);
}

void CPlayer::__StartSpeedStage()
{
	__RoomQuit();

	if (!IsOpenGuide(38))
		return;

	pto_STAGE_S2C_RES_StartSpeedStage pto;
	pto.set_res(0);

	if (0 == GAME_WORLD()->speed_stage_id())
		pto.set_res(3);
	else if (0 >= need_reset_->speed_times())
		pto.set_res(1);

	SendToAgent(pto, STAGE_S2C_RES_StartSpeedStage);

	if (0 == pto.res())
	{
		need_reset_->ChangeSpeedTimes(-1);
		mission()->ImplementMission(EMissionTargetType::MTT_SpeedStage, 0, 0);
		GAME_WORLD()->StartBattle(CSpeedStage::GetMapID(GAME_WORLD()->speed_stage_id()), BattleMode::SPEED, { pid() }, {});
	}
}

void CPlayer::__SweepStage(const CMessage& msg)
{
	pto_STAGE_C2S_REQ_Sweep pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (3 == pto.type())
	{
		__SweepEliteStage(msg);
		return;
	}

	pto_STAGE_S2C_RES_Sweep pto_res;

	pto_res.set_res(0);

	if (pto.stage_difficulty() <= 0 || pto.stage_difficulty() >= 4)
		return;

	int nPlayerStageProgress{ 0 };

	switch (pto.stage_difficulty())
	{
	case 1:nPlayerStageProgress = mission_->stage_normal_progress();		break;
	case 2:nPlayerStageProgress = mission_->stage_hard_progress();			break;
	case 3:nPlayerStageProgress = mission_->stage_nightmare_progress();		break;
	}

	const CStage* stage{ CStage::GetStage(pto.stage_lv(), pto.stage_difficulty()) };

	if (!stage)
		pto_res.set_res(3);
	else if (nPlayerStageProgress < pto.stage_lv())
		pto_res.set_res(1);
	else if (stamina() < kStageStamina)
		pto_res.set_res(2);

	if (0 == pto_res.res())
	{
		ChangeStamina(-kStageStamina);

		if (!pto.is_sell())
		{
			SetMaxLevel(stage->m_nUnlockLevel);
			ChangeExp(stage->m_nRewardExp);
			ChangeSilver(stage->m_nRewardSilver);
			ChangeHonor(stage->m_nRewardHonor);
			knapsack()->GiveNewItem(stage->m_nRewardItem1ID, stage->m_nRewardItem1Num);
			knapsack()->GiveNewItem(stage->m_nRewardItem2ID, stage->m_nRewardItem2Num);
			knapsack()->GiveNewItem(stage->m_nRewardItem3ID, stage->m_nRewardItem3Num);
			knapsack()->GiveNewItem(stage->m_nRewardItem4ID, stage->m_nRewardItem4Num);
			knapsack()->GiveNewItem(stage->m_nRewardItem5ID, stage->m_nRewardItem5Num);
			knapsack()->GiveNewItem(stage->m_nRewardItem6ID, stage->m_nRewardItem6Num);
		}
		else
		{
			SetMaxLevel(stage->m_nUnlockLevel);
			ChangeExp(stage->m_nRewardExp);
			ChangeSilver(stage->m_nRewardSilver);
			ChangeHonor(stage->m_nRewardHonor);

			if (nullptr != CItemTP::GetItemTP(stage->m_nRewardItem1ID))
				ChangeSilver(CItemTP::GetItemTP(stage->m_nRewardItem1ID)->GetSellPrice());
			if (nullptr != CItemTP::GetItemTP(stage->m_nRewardItem2ID))
				ChangeSilver(CItemTP::GetItemTP(stage->m_nRewardItem2ID)->GetSellPrice());
			if (nullptr != CItemTP::GetItemTP(stage->m_nRewardItem3ID))
				ChangeSilver(CItemTP::GetItemTP(stage->m_nRewardItem3ID)->GetSellPrice());
			if (nullptr != CItemTP::GetItemTP(stage->m_nRewardItem4ID))
				ChangeSilver(CItemTP::GetItemTP(stage->m_nRewardItem4ID)->GetSellPrice());
			if (nullptr != CItemTP::GetItemTP(stage->m_nRewardItem5ID))
				ChangeSilver(CItemTP::GetItemTP(stage->m_nRewardItem5ID)->GetSellPrice());
			if (nullptr != CItemTP::GetItemTP(stage->m_nRewardItem6ID))
				ChangeSilver(CItemTP::GetItemTP(stage->m_nRewardItem6ID)->GetSellPrice());
		}

		ImplementMission(MTT_CompleteMap, stage->GetLevel(), stage->GetDifficulty());
		ImplementRewardMission(RMT_SweepStage, stage->GetLevel(), 1);
	}

	SendToAgent(pto_res, STAGE_S2C_RES_SweepStage);
}

void CPlayer::__SweepEliteStage(const CMessage &msg)
{
	pto_STAGE_C2S_REQ_Sweep pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_STAGE_S2C_RES_Sweep pto_res;
	pto_res.set_res(0);

	const CEliteStage *stage{ CEliteStage::GetEliteStageByLevel(pto.stage_lv()) };

	int nEliteStageProgress = mission()->stage_elite_progress();

	if (nullptr == stage)
		pto_res.set_res(3);
	else if (nEliteStageProgress < pto.stage_lv())
		pto_res.set_res(1);
	else if (need_reset_->IsEliteAlreadyFighted(pto.stage_lv()))
		pto_res.set_res(5);
	else if (stamina() < 20)
		pto_res.set_res(2);
	else if (vip_level() < 1 && (time(0) - sweep_elite_stage_time_) < 600)
		pto_res.set_res(4);

	if (0 == pto_res.res())
	{
		int box_num = stage->ProduceBoxNum();
		pto_res.set_box_num(box_num);

		need_reset_->AddEliteAlready(stage->GetLevel());
		ChangeStamina(-20);

		ChangeExp(stage->m_nRewardExp);
		ChangeSilver(stage->m_nRewardSilver);
		ChangeHonor(stage->m_nRewardHonour);
		knapsack()->GiveNewItem(stage->m_nRewardBox, box_num);
		need_reset_->UpdateEliteStage();

		ImplementMission(EMissionTargetType::MTT_EliteStage, stage->m_nLevel, 0);
	}

	SendToAgent(pto_res, STAGE_S2C_RES_SweepStage);
}

void CPlayer::__QuitBattle()
{
	CMessage msg{ "", MSG_C2S, BATTLE_C2S_NTF_MasterLeave };
	msg.SetID(pid());

	if (state() == PlayerState::BATTLE)
		GAME_WORLD()->SendToBattleServer(msg, pid());
	else if (state() == PlayerState::MATCH)
		GAME_WORLD()->SendToBattleServer("", MSG_C2S, ROOM_C2S_NTF_CancelMatch, pid());

	state(PlayerState::NORMAL);
}

void CPlayer::__GetDayWinAward()
{
	SPMail mail = std::make_shared<CMail>();
	mail->SetModelID(MMI_DayWinAward);
	mail->AddRewardItem(LT_Gold, 30);;
	GAME_WORLD()->GiveNewMail(mail, pid());
}

void CPlayer::__UpdateSpeedStage()
{
	pto_STAGE_S2C_RES_UpdateSpeedStage pto;
	pto.set_best_rank(speed_stage_record_);
	pto.set_best_time(need_reset_->speed_today_best_time());
	pto.set_surplus_times(need_reset_->speed_times());
	pto.set_max_level(GAME_WORLD()->speed_stage_max_level());
	pto.set_stage_id(GAME_WORLD()->speed_stage_id());

	int rank_num = GAME_WORLD()->GetSpeedStageRankingsNum();
	int pages = rank_num / 10;
	if (rank_num % 10)
		pages++;

	pto.set_max_page(pages);

	int rank = GAME_WORLD()->GetSpeedStageRank(pid());
	pto.set_rank(rank);

	int max_rank = GAME_WORLD()->GetSpeedStageRankingsNum();

	std::deque<int> show_rank;

	if (-1 == rank)
	{
		for (int i = max_rank; i > 0; i--)
			show_rank.push_front(i);
	}
	else
	{
		show_rank.push_back(rank);

		int front_num = rank - 1;
		int back_num = max_rank - rank;

		if (front_num < 2 && back_num < 2)
		{
			for (int i = 1; i <= front_num; i++)
				show_rank.push_front(rank - i);
			for (int i = 1; i <= back_num; i++)
				show_rank.push_back(rank + i);
		}
		else if (front_num < 2)
		{
			for (int i = 1; i <= front_num; i++)
				show_rank.push_front(rank - i);
			for (int i = 1; i <= (4 - front_num); i++)
				show_rank.push_back(rank + i);

		}
		else if (back_num < 2)
		{
			for (int i = 1; i <= back_num; i++)
				show_rank.push_back(rank + i);
			for (int i = 1; i <= (4 - back_num); i++)
				show_rank.push_front(rank - i);
		}
		else
		{
			for (int i = 1; i <= 2; i++)
				show_rank.push_front(rank - i);
			for (int i = 1; i <= 2; i++)
				show_rank.push_back(rank + i);
		}
	}

	for (auto &it : show_rank)
	{
		const SpeedRankInfo* rank_info = GAME_WORLD()->GetSpeedStageRankingDataByRank(it);

		if (rank_info)
		{
			auto data = PLAYER_DATA(rank_info->pid);

			if (data)
			{
				auto pto_player = pto.add_player_info();
				pto_player->set_name(data->name());
				pto_player->set_rank(it);
				pto_player->set_pid(rank_info->pid);
				pto_player->set_time(rank_info->time);
				pto_player->set_dress_id(data->clothes());
				pto_player->set_sex(data->sex());
			}
		}
	}

	SendToAgent(pto, STAGE_S2C_RES_UpdateSpeedStage);
}

void CPlayer::__CheckSpeedFormation(const CMessage &msg)
{
	pto_STAGE_C2S_REQ_CheckSpeedFormation pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_STAGE_S2C_RES_CheckSpeedFormation pto_res;
	pto_res.set_pid(pto.pid());

	const SpeedRankInfo* info = GAME_WORLD()->GetSpeedStageRankingDataByPID(pto.pid());

	if (nullptr == info)
		return;

	for (size_t i = 0; i < kMaxSoldierMun; i++)
		pto_res.add_soldiers(info->soldiers[i]);

	for (size_t i = 0; i < kMaxHeroNum; i++)
	{
		auto pto_hero = pto_res.add_heroes();
		pto_hero->set_hero_id(info->heroes[i]);
	}

	SendToAgent(pto, STAGE_S2C_RES_CheckSpeedFormation);
}

void CPlayer::__SpeedRankList(const CMessage &msg)
{
	pto_STAGE_C2S_REQ_SpeedRankList pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_STAGE_S2C_RES_SpeedRankList pto_res;
	pto_res.set_page(pto.page());

	int rank_num = GAME_WORLD()->GetSpeedStageRankingsNum();
	int pages = rank_num / 10;
	if (rank_num % 10)
		pages++;

	if (pto.page() > pages)
		return;

	int begin_rank = (10 * pto.page()) + 1;

	for (size_t i = 0; i < 10; i++)
	{
		const SpeedRankInfo* info = GAME_WORLD()->GetSpeedStageRankingDataByRank(begin_rank + i);

		if (info)
		{
			auto pto_info = pto_res.add_player_info();
			pto_info->set_rank(begin_rank + i);
			pto_info->set_pid(info->pid);
			pto_info->set_time(info->time);

			auto data = PLAYER_DATA(info->pid);
			if (data)
				pto_info->set_name(data->name());
			else
				pto_info->set_name("unknow");
		}
	}

	SendToAgent(pto_res, STAGE_S2C_RES_SpeedRankList);
}

void CPlayer::__UpdateBattleRecord()
{
	pto_PLAYER_S2C_RES_UpdateBattleData pto;

	for (size_t i = 0; i < battle_record_.size(); i++)
	{
		auto data = pto.add_datas();
		data->set_point(battle_record_[i].point);
		data->set_s(battle_record_[i].S);
		data->set_a(battle_record_[i].A);
		data->set_b(battle_record_[i].B);
		data->set_c(battle_record_[i].C);
		data->set_win(battle_record_[i].win);
		data->set_lose(battle_record_[i].lose);

		if (0 == i)
			data->set_week_times(need_reset_->week_war_times());
		else if (1 == i)
			data->set_week_times(need_reset_->week_1v1_times());
		else if (2 == i)
			data->set_week_times(need_reset_->week_3v3_times());
	}

	pto.set_reward_times(need_reset_->is_got_battle_reward());

	SendToAgent(pto, PLAYER_S2C_RES_UpdateBattleData);
}

std::vector<int> CPlayer::GetFriendList()
{
	LOCK_BLOCK(social_lock_);
	std::vector<int> list;
	for (auto &it : friends_list_)
		list.push_back(it);
	return std::move(list);
}

void CPlayer::__GetGift(const CMessage& msg)
{
	pto_PLAYER_C2S_REQ_GetGift pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	std::wstring	w_password(UTF8_to_UNICODE(pto.key()));
	std::wregex		regex(L"[0-9A-Za-z]");

	bool res = true;

	if (w_password.length() > 18)
	{
		res = false;
	}
	else
	{
		for (size_t i = 0; i < w_password.length(); i++)
			if (!regex_match(w_password.substr(i, 1), regex))
				res = false;
	}

	if (true == res)
	{
		std::ostringstream sql;
		sql << "select `type` from tb_gift where pid = 0 and `key` = '" << pto.key().c_str() << "'";
		ResultSetPtr result = MYSQL_QUERY(sql.str());
		if (result && result->next())
		{
			int type = result->getInt("type");

			sql.str("");
			sql << "select count(*) from tb_gift where pid = " << pid_ << " and type = " << type;
			ResultSetPtr already = MYSQL_QUERY(sql.str());

			if (already && already->next() && 0 == already->getInt(1))
			{
				SPGift gift = CGift::GetGift(type);

				if (gift)
				{
					for (auto &it : *gift->GetItem())
						knapsack()->GiveNewItem(it.first, it.second);

					sql.str("");
					sql << "update tb_gift set pid = " << pid() << " where `key` = '" << pto.key().c_str() << "'";
					MYSQL_UPDATE(sql.str());
				}
				else
				{
					res = false;
				}
			}
			else
			{
				res = false;
			}
		}
		else
		{
			res = false;
		}
	}
	else
	{
		res = false;
	}

	pto_PLAYER_S2C_RES_GetGift pto_res;
	pto_res.set_result(res);
	SendToAgent(pto_res, PLAYER_S2C_RES_GetGift);
}

void CPlayer::__GMRecharge(int pid, int rmb)
{
	std::ostringstream sql;
	sql << "select count(*) from tb_player where pid = " << pid;
	ResultSetPtr ptr = MYSQL_QUERY(sql.str());

	if (ptr && ptr->rowsCount() > 0)
	{
		sql.str("");
		sql << "insert into tb_order_form values('HLZJ_GM" << time(0) << "', " << pid << ", " << rmb * 10 << ", " << rmb << ", default, false," << pid_ << ")";
		MYSQL_UPDATE(sql.str());
	}
}

void CPlayer::__GmGiveHero(int pid, int hero_id)
{
	SPPlayer player = FIND_PLAYER(pid);

	if (player)
		player->army()->AddHero(hero_id, true);
}

void CPlayer::__GmImplementMission(int pid)
{
	SPPlayer player = FIND_PLAYER(pid);

	if (player)
		player->mission()->ImplementMission(EMissionTargetType::MTT_AC, 0, 0);
}

void CPlayer::__KickPlayer(int pid)
{
	SPPlayer player = FIND_PLAYER(pid);
	if (player)
		GAME_WORLD()->SendToAgentServer("", MSG_S2A, SYSTEM_S2A_NTF_GMKick, pid, player->session());
}

void CPlayer::__FrozenPlayer(int pid, int hour)
{
	__KickPlayer(pid);

	std::ostringstream sql;
	sql << "update tb_player set frozen_time = from_unixtime(" << time(0) + hour * 60 * 60 << ") where pid = " << pid;
	MYSQL_UPDATE(sql.str());
}

void CPlayer::__GetVIPAward(const CMessage &msg)
{
	pto_PLAYER_C2S_REQ_GetVIPAward pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	bool result = false;

	if (pto.type() > (int)vip_level_award_.size() || pto.type() <= 0 || pto.type() > vip_level() || vip_level_award_[pto.type() - 1])
	{
		result = false;
	}
	else
	{
		SPRechargeReward reward = CVIPGift::GetRechargeReward(pto.type());
		if (reward)
		{
			army()->AddHero(reward->hero, true);
			army()->AddSoldier(reward->soldier);
			knapsack()->AddFashion(reward->fashion);
			for (auto &it : reward->item)
				knapsack()->GiveNewItem(it.first, it.second, true);
			for (auto &it : reward->equip)
				knapsack()->GiveNewEquip(it.first, it.second);

			vip_level_award_[pto.type() - 1] = true;

			result = true;
		}
	}

	pto_PLAYER_S2C_RES_GetVIPAward pto_res;
	pto_res.set_type(vip_level_award_.to_ulong());
	pto_res.set_result(result);
	SendToAgent(pto_res, PLAYER_S2C_RES_GetVIPAward);
}

void CPlayer::__GetOnlineAward(const CMessage &msg)
{
	pto_PLAYER_C2S_REQ_GetOnlineAward pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	bool result = false;

	if (pto.type() != (online_day_award_ + 1) || need_reset_->today_attendance())
	{
		result = false;
	}
	else
	{
		SPCheckInGift gift = CVIPGift::GetCheckInGift(pto.type());
		if (gift)
		{
			army()->AddHero(gift->hero, true);
			for (auto &it : gift->item)
				knapsack()->GiveNewItem(it.first, it.second, true);

			result = true;

			online_day_award_++;

			need_reset_->today_attendance(true);
		}
	}

	pto_PLAYER_S2C_RES_GetOnlineAward pto_res;
	int i = 1;
	if (need_reset_->today_attendance())
		i = -1;
	pto_res.set_type(pto.type() * i);
	pto_res.set_result(result);
	SendToAgent(pto_res, PLAYER_S2C_RES_GetOnlineAward);
}
