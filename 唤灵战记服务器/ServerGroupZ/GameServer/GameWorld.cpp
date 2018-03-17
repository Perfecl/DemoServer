#include "stdafx.h"
#include "GameWorld.h"
#include "GameServer.h"
#include "Player.h"
#include "PlayerCache.h"
#include "Town.h"
#include "Guild.h"
#include "RoomZ.h"
#include "NeedReset.h"
#include "Knapsack.h"
#include "Mission.h"
#include "Mail.h"
#include "PlayerExercise.h"
#include "Army.h"
#include "Offline.h"

CGameWorld* CGameWorld::instance_;

int	CGameWorld::watch_pid_{ 0 };

CGameWorld::CGameWorld(CGameServer& game_svr) :
game_server_{ game_svr }
{
	is_open_ = true;

	instance_ = this;

	__LoadFromDatabase();
}

CGameWorld::~CGameWorld()
{
	is_open_ = false;

	__CloseFinishEscort();

	__SaveToDatabase();

	RECORD_INFO("正在储存服务器数据....");
	{
		LOCK_BLOCK(saving_lock_);
		try
		{
			for (auto &it : player_saving_tasks_)
				it.second.wait();
		}
		catch (std::exception ex)
		{
			MessageBoxA(NULL, ex.what(), "", MB_OK);
		}

	}
	RECORD_INFO("服务器数据储存成功....");

	for (auto &it : towns_)
		delete it.second;
	for (auto &it : guilds_)
		delete it.second;
}

void CGameWorld::OnClock(int clock_id, const tm& now_datetime)
{
	if (!is_open_)
		return;

	int week_day = now_datetime.tm_wday;
	int hour = now_datetime.tm_hour;
	int minute = now_datetime.tm_min;

	__ExercisePlatformLoop();
	__FinishEscort();

	{
		LOCK_BLOCK(players_lock_);
		for (auto &it : online_players_)
		{
			it.second->CheckOrder();
			if (minute % 5 == 0)
				it.second->ChangeStamina(1);
		}
	}

	if (minute % 5 == 0)
		__SaveToDatabase();

	switch (hour)
	{
	case 0:
		event_hour_.reset();
		break;
	case 4:
		if (false == event_hour_[4])
		{
			if (0 == week_day)
				__WindUpTrade();
			else if (1 == week_day)
				__WindUpArenaReward();

			__RoutineReset();

			player_cache_lock_.lock();
			playerdata_cache_.clear();
			player_cache_lock_.unlock();

			SendToAllPlayers("", MSG_S2C, PLAYER_S2C_NTF_DayReset);

			event_hour_[4] = true;
		}
		break;
	case 12:
		if (false == event_hour_[12])
		{
			LOCK_BLOCK(players_lock_);
			for (auto &it : online_players_)
				it.second->ChangeStamina(20);
			event_hour_[12] = true;
		}
		break;
	case 18:
		if (false == event_hour_[18])
		{
			LOCK_BLOCK(players_lock_);
			for (auto &it : online_players_)
				it.second->ChangeStamina(20);
			event_hour_[18] = true;
		}
		break;
	case 19:
		if (false == event_hour_[19])
		{
			SendOfflineBattleReward();
			event_hour_[19] = true;
		}
		break;
	case 21:
		if (false == event_hour_[21])
		{
			__WindUpSpeedStage();
			event_hour_[21] = true;
		}
		break;
	default:
		break;
	}
}

void CGameWorld::InitializeGameWorld(CGameServer& game_svr)
{
	if (instance_)
	{
		RECORD_ERROR("游戏世界已经初始化");
		return;
	}

	CFashionTP::Load();
	CEquipTP::Load();
	CItemTP::Load();
	CMissionTP::Load();
	CRewardMission::Load();
	CExpLibrary::Load();
	COpenGuid::Load();
	CLottery::Load();
	CLotteryChance::Load();
	CTimeBox::Load();
	CTownBox::Load();
	CCombinationReward::Load();
	CVIPFun::Load();
	CGoldSpend::Load();
	CRootData::Load();
	CUnitTP::Load();
	CSoldierTP::Load();
	CHeroTP::Load();
	CRecastLib::Load();
	CRecruitLib::Load();
	COpenTP::Load();
	CInteriorEvent::Load();
	CInteriorLib::Load();
	CSkill::Load();
	CPassSkill::Load();
	CTradeCard::Load();
	CStage::Load();
	CEliteStage::Load();
	CSpeedStage::Load();
	CTownRankBox::Load();
	CAddition::Load();
	COfflineRewardStage::Load();
	CRuneLibrary::Load();
	CFairArenaFirstWin::Load();
	CBlueprint::Load();
	CSuitTP::Load();
	COpenGuid::Load();
	CHeroQuality::Load();
	CEnchanting::Load();
	CEnchantingCost::Load();
	CResolveTP::Load();
	CRecastCost::Load();
	CMall::Load();
	CIllegalWords::Load();
	CVIPGift::Load();
	CGift::Load();
	CGuildLibrary::Load();

	COffLinePlayer::ProduceOfflineBattleAI();

	new CGameWorld(game_svr);
	instance_->__InitializeTowns();
	game_svr.StartClock(1, CGameWorld::kTimeLoop);

	for (auto &it : instance_->exercise_platforms_)
	{
		SPExpPill pill = std::make_shared<ExpPill>();
		pill->id = it.second->id;
		instance_->exp_pills_.insert(std::make_pair(pill->id, pill));
	}
}

void CGameWorld::CloseGameWorld()
{
	delete instance_;
	instance_ = nullptr;
}

CGameWorld* CGameWorld::GetInstance()
{
	if (nullptr == instance_)
		RECORD_FATAL("游戏世界实例未初始化");

	return instance_;
}

void CGameWorld::SendToAgentServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, networkz::ISession* session)
{
	game_server_.SendToAgentServer(content, protocol_type, protocol_id, pid, session);
}

void CGameWorld::SendToAgentServer(CMessage msg, int pid, networkz::ISession* session)
{
	game_server_.SendToAgentServer(std::move(msg), pid, session);
}

void CGameWorld::SendToBattleServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid)
{
	game_server_.SendToBattleServer(content, protocol_type, protocol_id, pid);
}

void CGameWorld::SendToBattleServer(CMessage msg, int pid)
{
	game_server_.SendToBattleServer(std::move(msg), pid);
}

void CGameWorld::SendToAllPlayers(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid)
{
	std::map<networkz::ISession*, std::vector<int>>  sessions;
	LOCK_BLOCK(players_lock_);

	for (auto &player : online_players_)
	{
		if (except_pid == player.first)
			continue;

		auto it = sessions.find(player.second->session());
		if (sessions.cend() == it)
			sessions.insert(std::make_pair(player.second->session(), std::move(std::vector < int > { player.first })));
		else
			it->second.push_back(player.first);
	}

	CMessage msg{ content, protocol_type, protocol_id };
	std::string str;
	for (auto &it : sessions)
	{
		pto_SYSTEM_S2A_NTF_Transpond pto;
		for (auto &itPids : it.second)
			pto.add_pid(itPids);
		pto.set_message(msg, msg.length());
		pto.SerializeToString(&str);
		SendToAgentServer(str, MSG_S2A, SYSTEM_S2A_NTF_Transpond, 0, it.first);
	}
}

bool CGameWorld::HasBattleServerConnect()
{
	if (game_server_.battle_session())
		return true;
	else
		return false;
}

int	CGameWorld::sid()
{
	return game_server_.GetSID();
}

void CGameWorld::ProcessMessage(const CMessage& msg)
{
	if (!is_open_)
		return;

	switch (msg.GetProtocolID())
	{
	case BATTLE_B2S_RES_CreateBattle:
		__OnCreateBattle(msg);
		break;
	case BATTLE_B2S_NTF_DeleteBattle:
		__OnDeleteBattle(msg);
		break;
	case BATTLE_B2S_NTF_CancelMatch:
		__CancelMatch(msg);
		break;
	case BATTLE_S2C_NTF_GameOver:
		__GameOver(msg);
		break;
	case LOGIN_C2S_REQ_EnterGame:
		__EnterGame(msg);
		break;
	case SYSTEM_A2S_NTF_OffLine:
		__LeaveGame(msg);
		break;
	default:
	{
		SPPlayer player = FIND_PLAYER(msg.GetID());

		if (player)
		{
			if (msg.GetType() == MSG_S2C)
				SendToAgentServer(msg, player->pid(), player->session());
			else
			{
				try
				{
					player->ProcessMessage(msg);
				}
				catch (std::exception exc)
				{
					std::cout << exc.what() << std::endl;
				}
				catch (...)
				{
					std::cout << "处理玩家协议时发生异常" << std::endl;
				}
			}
		}
		else
		{
			RECORD_WARNING(FormatString("找不到玩家,pid:", msg.GetID()));
		}
	}
		break;
	}
}

SPPlayer CGameWorld::FindPlayer(int pid)
{
	LOCK_BLOCK(players_lock_);
	auto it = online_players_.find(pid);
	if (online_players_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CGameWorld::ErasePlayer(int pid)
{
	LOCK_BLOCK(players_lock_);
	GAME_WORLD()->online_players_.erase(pid);
}

CTown* CGameWorld::FindTown(int town_id)
{
	auto it = towns_.find(town_id);
	if (towns_.cend() == it)
		return nullptr;
	else
		return it->second;
}

int CGameWorld::CreateGuild(const std::string &name, int pid)
{
	std::ostringstream sql;
	sql << "call sp_create_guild(" << sid() << ",'" << name.c_str() << "'," << pid << ", @param_guild_id)";
	MySQLExecute(sql.str());

	ResultSetPtr create_result{ MySQLQuery("select @param_guild_id") };
	if (create_result && create_result->next())
	{
		int guild_id = create_result->getInt(1);

		if (guild_id <= 0)
		{
			throw abs(guild_id);
		}
		else
		{
			try
			{
				CGuild* guild = new CGuild(guild_id);

				LOCK_BLOCK(guilds_lock_);

				if (false == guilds_.insert(std::make_pair(guild_id, guild)).second)
				{
					delete guild;
					throw 5;
				}

				sql.str("");
				sql << "update tb_player set guild_id = " << guild_id << " where pid = " << pid;
				MySQLUpdate(sql.str());
			}
			catch (std::exception ex)
			{
				throw 5;
			}

			return guild_id;
		}
	}
	else
	{
		throw 5;
	}
}

int	CGameWorld::GetGuildRanking(int guild_id)
{
	LOCK_BLOCK(guilds_lock_);

	for (size_t i = 0; i < guild_ranking_.size(); i++)
	{
		if (guild_id == guild_ranking_[i])
			return i + 1;
	}
	return 0;
}

std::vector<CGuild*> CGameWorld::FindGuild(const std::string& guild_name)
{
	std::vector<CGuild*> vct_guilds;

	LOCK_BLOCK(guilds_lock_);

	for (auto &it : guilds_)
	{
		if (guild_name == it.second->name() || "%" == guild_name)
			vct_guilds.push_back(it.second);
	}

	return std::move(vct_guilds);
}

CGuild*	CGameWorld::FindGuild(int guild_id)
{
	LOCK_BLOCK(guilds_lock_);

	auto it = guilds_.find(guild_id);

	if (guilds_.cend() == it)
		return nullptr;
	else
		return it->second;
}

SPRoomZ CGameWorld::CreateNewRoom(int pid, BattleMode mode, int map_id, const char* name, const char* password)
{
	SPRoomZ room = std::make_shared<CRoomZ>(++room_id_allocator_, mode, pid);

	std::string room_name{ name };

	room->name(name);
	room->map_id(map_id);
	room->password(password);

	LOCK_BLOCK(rooms_lock_);
	if (false == rooms_.insert(std::make_pair(room->room_id(), room)).second)
		return nullptr;
	else
		return room;
}

void CGameWorld::DeleteRoom(int room_id)
{
	LOCK_BLOCK(rooms_lock_);
	rooms_.erase(room_id);
}

SPRoomZ	CGameWorld::FindRoom(int room_id)
{
	LOCK_BLOCK(rooms_lock_);

	auto it = rooms_.find(room_id);
	if (rooms_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CGameWorld::SetRoomListProtocol(pto_ROOM_S2C_RES_RoomList* pto, int map_id, int mode)
{
	if (mode == -1)
		return;

	LOCK_BLOCK(rooms_lock_);

	for (auto &it : rooms_)
	{
		if (map_id > 0 && it.second->map_id() != map_id)
			continue;

		if ((int)it.second->room_mode() != mode)
			continue;

		it.second->SetProtocol(pto->add_arrroomlist());
	}
}

void CGameWorld::OfflineBySession(networkz::ISession* session)
{
	if (!is_open_)
		return;

	LOCK_BLOCK(players_lock_);

	for (auto &it : online_players_)
	{
		if (it.second->session() == session)
			__DeletePlayer(it.first);
	}
}

Concurrency::task<void>& CGameWorld::GetTask(int pid)
{
	LOCK_BLOCK(saving_lock_);

	player_saving_tasks_.find(pid);
	auto it = player_saving_tasks_.find(pid);
	if (player_saving_tasks_.cend() == it)
		player_saving_tasks_.insert(std::make_pair(pid, Concurrency::task<void>([](){})));

	return player_saving_tasks_[pid];
}

ResultSetPtr CGameWorld::MySQLQuery(const std::string& sql)
{
	return std::move(game_server_.MySQLQuery(sql));
}

__int64	CGameWorld::MySQLUpdate(const std::string& sql)
{
	return game_server_.MySQLUpdate(sql);
}

bool CGameWorld::MySQLExecute(const std::string& sql)
{
	return game_server_.MySQLExecute(sql);
}

int	CGameWorld::GetSpeedStageRank(int pid)
{
	LOCK_BLOCK(speed_stage_lock_);

	int rank{ 1 };

	for (auto &it : week_speed_ranking_)
	{
		if (it.pid == pid)
			return rank;
		else
			rank++;
	}
	return -1;
}

const SpeedRankInfo* CGameWorld::GetSpeedStageRankingDataByPID(int nPID)
{
	LOCK_BLOCK(speed_stage_lock_);

	for (auto &it : week_speed_ranking_)
	{
		if (it.pid == nPID)
			return &it;
	}

	return nullptr;
}

const SpeedRankInfo* CGameWorld::GetSpeedStageRankingDataByRank(int nRank)
{
	if (nRank <= 0)
		return nullptr;

	LOCK_BLOCK(speed_stage_lock_);

	if (week_speed_ranking_.empty())
		return nullptr;

	if (week_speed_ranking_.size() < (size_t)nRank)
		return nullptr;

	auto it = week_speed_ranking_.begin();

	for (int i = 1; i < nRank; i++)
	{
		if (week_speed_ranking_.cend() == it)
			return nullptr;
		++it;
	}

	return &*it;
}

int	CGameWorld::ChangeSpeedStageRank(int pid)
{
	LOCK_BLOCK(speed_stage_lock_);

	for (auto it = week_speed_ranking_.begin(); it != week_speed_ranking_.cend(); ++it)
	{
		if (it->pid == pid)
		{
			week_speed_ranking_.erase(it);
			break;
		}
	}

	SpeedRankInfo info;

	auto player = FIND_PLAYER(pid);

	if (!player)
		return 0;

	info.pid = pid;
	info.time = player->need_reset()->speed_today_best_time();

	info.heroes = player->army()->GetFormationHero();
	info.soldiers = player->army()->GetFormationSoldier();

	int rank{ 1 };
	for (auto it = week_speed_ranking_.begin(); it != week_speed_ranking_.cend(); ++it)
	{
		if (it->time > info.time)
		{
			week_speed_ranking_.insert(it, std::move(info));
			return rank;
		}
		rank++;
	}

	week_speed_ranking_.push_back(std::move(info));

	return week_speed_ranking_.size();
}

bool CGameWorld::UpdateMaxLevel(int level)
{
	if (level > max_level_in_server_)
	{
		max_level_in_server_ = level;
		return true;
	}
	return false;
}

int	CGameWorld::FindPIDByName(const char* name)
{
	std::ostringstream sql;
	sql << "select pid from tb_player where name = '" << name << "' and sid = " << sid();
	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (result && result->next())
		return result->getInt(1);
	else
		return 0;
}

SPIPlayerData CGameWorld::GetPlayerData(int pid)
{
	SPPlayer player = FIND_PLAYER(pid);
	if (player)
		return player;

	LOCK_BLOCK(player_cache_lock_);

	auto it = playerdata_cache_.find(pid);

	if (playerdata_cache_.cend() == it)
	{
		try
		{
			auto cache = std::make_shared<CPlayerCache>(pid);
			playerdata_cache_.insert(std::make_pair(pid, cache));
			return cache;
		}
		catch (std::exception ex)
		{
			RECORD_WARNING(ex.what());
			return nullptr;
		}
	}
	else
	{
		return it->second;
	}
}

void CGameWorld::Annunciate(const std::string& name, int arg0, int arg1, AnnunciateType type)
{
	pto_CHAT_S2C_NTF_BroadCast pto;
	pto.set_id(static_cast<int>(type));
	pto.set_name0(name);
	pto.set_param0(arg0);
	pto.set_param1(arg1);
	std::string str;
	pto.SerializeToString(&str);
	SendToAllPlayers(str, MSG_S2C, CHAT_S2C_NTF_BroadCast);
}

int	CGameWorld::StartBattle(int map_id, BattleMode mode, std::vector<int> atkmasters, std::vector<int> defmasters, int stage_id, int stage_diff, int box_num)
{
	if (!HasBattleServerConnect())
		return -1;

	pto_BATTLE_S2B_REQ_CreateBattle_Ex pto;
	pto.set_map(map_id);
	pto.set_mode(static_cast<int>(mode));

	pto.set_atk_sid(sid());
	pto.set_def_sid(sid());

	bool is_fair{ IsFairMode(mode) };

	for (auto &it : atkmasters)
	{
		SPPlayer player{ FindPlayer(it) };

		if (player)
		{
			if (player->is_in_battle())
			{
				RECORD_TRACE("玩家正在战斗中,无法开始新的战斗");
				return player->pid();
			}
			pto_BATTLE_STRUCT_Master_Ex* pto_master = pto.add_atk_masters();

			player->SetBattleMasterProto(pto_master, is_fair);

			if (mode == BattleMode::MULTISTAGE)
				pto_master->set_multi_stage_clear(player->need_reset()->IsMultiStageClear(stage_id));
			else
				pto_master->set_multi_stage_clear(false);
		}
	}
	for (auto &it : defmasters)
	{
		SPPlayer player{ FindPlayer(it) };

		if (player)
		{
			if (player->is_in_battle())
			{
				RECORD_TRACE("玩家正在战斗中,无法开始新的战斗");
				return player->pid();
			}
			pto_BATTLE_STRUCT_Master_Ex* pto_master = pto.add_def_masters();

			player->SetBattleMasterProto(pto_master, is_fair);

			if (mode == BattleMode::MULTISTAGE)
				pto_master->set_multi_stage_clear(player->need_reset()->IsMultiStageClear(stage_id));
			else
				pto_master->set_multi_stage_clear(false);
		}
	}

	pto.set_stage_id(stage_id);
	pto.set_stage_difficulty(stage_diff);
	pto.set_box_num(box_num);

	std::string str;
	pto.SerializeToString(&str);
	SendToBattleServer(str, MSG_S2B, BATTLE_S2B_REQ_CreateBattle, 0);

	return 0;
}

void CGameWorld::ResetDay()
{
	ResetDayOfflineExp();

	__RoutineReset();

	player_cache_lock_.lock();
	playerdata_cache_.clear();
	player_cache_lock_.unlock();
}

void CGameWorld::GiveNewMail(SPMail mail, int pid)
{
	SPPlayer player{ FindPlayer(pid) };

	if (player)
		player->knapsack()->AddMail(mail);
	else
		CMail::InsertToDatabase(mail, pid);
}

void CGameWorld::SendExercisePlatform(const std::string& content, unsigned char protocol_type, int protocol_id)
{
	for (auto &it : online_players_)
		if (it.second->exercise()->in_exercise_platform())
			SendToAgentServer(content, protocol_type, protocol_id, it.second->pid(), it.second->session());
}

SPExercisePlatform CGameWorld::GetExercisePlatform(int id)
{
	LOCK_BLOCK(exercise_lock_);

	auto it = exercise_platforms_.find(id);
	if (exercise_platforms_.cend() == it)
		return nullptr;
	else
		return it->second;
}

SPExpPill CGameWorld::GetExpPill(int id)
{
	LOCK_BLOCK(exercise_lock_);

	auto it = exp_pills_.find(id);
	if (exp_pills_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CGameWorld::AddOfflinExpBank(int pid, __int64 exp)
{

	LOCK_BLOCK(exercise_lock_);
	auto it = offline_exp_bank_.find(pid);
	if (offline_exp_bank_.cend() == it)
	{
		std::shared_ptr<OfflineExerciseExp> offline_exp = std::make_shared<OfflineExerciseExp>();
		offline_exp->exp_ = exp;
		offline_exp->exercise_time_ = 1;
		offline_exp_bank_.insert(std::make_pair(pid, offline_exp));
	}
	else
	{
		it->second->exp_ += exp;
		it->second->exercise_time_ += 1;
	}
}

SPOfflineExerciseExp CGameWorld::GetOfflineExp(int pid)
{
	LOCK_BLOCK(exercise_lock_);
	auto it = offline_exp_bank_.find(pid);
	if (offline_exp_bank_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CGameWorld::ResetDayOfflineExp()
{
	for (auto &it : offline_exp_bank_)
		it.second->exercise_time_ = 0;
}

void CGameWorld::DeleteOfflineExp(int pid)
{
	LOCK_BLOCK(exercise_lock_);
	offline_exp_bank_.erase(pid);
}

void CGameWorld::AddDownFromPlatform(int pid, int type, int enemy_id, time_t time)
{
	SPDownFromPlatform msg = std::make_shared<DownFromPlatform>();
	msg->type = type;
	msg->pid = enemy_id;
	msg->time = time;
	LOCK_BLOCK(exercise_lock_);
	down_msg_.insert(std::make_pair(pid, msg));
}

SPDownFromPlatform CGameWorld::GetDownMsg(int pid)
{
	LOCK_BLOCK(exercise_lock_);
	auto it = down_msg_.find(pid);
	if (down_msg_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CGameWorld::DeleteDownMsg(int pid)
{
	LOCK_BLOCK(exercise_lock_);
	down_msg_.erase(pid);
}

void CGameWorld::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select speed_stage_id,speed_stage_max_level,unix_timestamp(last_close_time) as ctt,event_flag from tb_game_world where sid = " << sid();
	ResultSetPtr result{ MySQLQuery(sql.str()) };
	if (result && result->next())
	{
		speed_stage_id_ = result->getInt("speed_stage_id");
		speed_stage_max_level_ = result->getInt("speed_stage_max_level");
		last_close_time_ = result->getInt64("ctt");
		event_hour_ = result->getInt("event_flag");

		sql.str("");
		sql << "select max(level) from tb_player where sid = " << sid();
		ResultSetPtr result_max_level{ MySQLQuery(sql.str()) };
		if (result_max_level && result_max_level->next())
			UpdateMaxLevel(result_max_level->getInt(1));

		sql.str("");
		sql << "select ";
		for (size_t i = 0; i < trade_cards_.size(); ++i)
			sql << "trade_card" << i + 1 << ",";
		sql << "trade_reward1,trade_reward2,trade_reward3 from tb_trade where sid = " << sid();
		ResultSetPtr result_trade{ MySQLQuery(sql.str()) };
		if (result_trade && result_trade->next())
		{
			for (size_t i = 0; i < trade_cards_.size(); ++i)
				trade_cards_[i] = result_trade->getInt(i + 1);
			trade_rewards_[0] = result_trade->getInt("trade_reward1");
			trade_rewards_[1] = result_trade->getInt("trade_reward2");
			trade_rewards_[2] = result_trade->getInt("trade_reward3");
		}

		sql.str("");
		sql << "select guild_id from tb_guild where sid = " << sid() << " and is_dissolve = false order by level desc";
		ResultSetPtr result_guild{ MySQLQuery(sql.str()) };
		if (!result_guild)
			return;
		while (result_guild->next())
		{
			int guild_id = result_guild->getInt(1);
			guild_ranking_.push_back(guild_id);
			guilds_[guild_id] = new CGuild(guild_id);
		}

		sql.str("");
		sql << "select * from tb_speed_stage_ranking where sid = " << sid() << " order by time asc";
		ResultSetPtr result_speed{ MySQLQuery(sql.str()) };
		if (!result_speed)
			return;
		while (result_speed->next())
		{
			SpeedRankInfo info;
			info.pid = result_speed->getInt("pid");
			info.time = result_speed->getInt("time");
			info.heroes[0] = result_speed->getInt("hero1");
			info.heroes[1] = result_speed->getInt("hero2");
			info.heroes[2] = result_speed->getInt("hero3");
			info.soldiers[0] = result_speed->getInt("soldier1");
			info.soldiers[1] = result_speed->getInt("soldier2");
			info.soldiers[2] = result_speed->getInt("soldier3");
			info.soldiers[3] = result_speed->getInt("soldier4");
			info.soldiers[4] = result_speed->getInt("soldier5");
			info.soldiers[5] = result_speed->getInt("soldier6");
			week_speed_ranking_.push_back(std::move(info));
		}

		sql.str("");
		sql << "select * from tb_exercise_platform where sid = " << sid();
		ResultSetPtr result_exercise{ MySQLQuery(sql.str()) };
		if (!result_exercise)
			return;
		while (result_exercise->next())
		{
			auto it = std::make_shared<CExercisePlatform>();
			it->id = result_exercise->getInt("id");
			it->level = result_exercise->getInt("level");
			it->start_time = result_exercise->getInt64("start_time");
			it->pid = result_exercise->getInt("pid");
			it->exp_mul = static_cast<float>(result_exercise->getDouble("exp_mul"));
			exercise_platforms_[it->id] = it;
		}

		sql.str("");
		sql << "select * from tb_player_exercise_exp where pid in (select pid from tb_player where sid =" << sid() << ")";
		ResultSetPtr result_exp{ MySQLQuery(sql.str()) };
		if (!result_exp)
			return;
		while (result_exp->next())
		{
			auto it = std::make_shared<OfflineExerciseExp>();
			it->exercise_time_ = result_exp->getInt("exercise_time");
			it->exp_ = result_exp->getInt64("exp");
			offline_exp_bank_[result_exp->getInt("pid")] = it;
		}

		//服务器离线排行数据
		sql.str("");
		sql << "select * from tb_offline_ranking where sid = " << sid() << " order by rank";
		ResultSetPtr result_offline_ranking{ MYSQL_QUERY(sql.str()) };
		if (!result_offline_ranking)
			return;
		if (result_offline_ranking->rowsCount() <= 0)
			COffLinePlayer::InsertAIIntoRanking();
		else
			while (result_offline_ranking->next())
				AddOfflineBattleRanking(result_offline_ranking->getInt("pid"));
	}
	else
	{
		COffLinePlayer::InsertAIIntoRanking();

		__GenerateTradeData();
		__GenerateSpeedStageData();

		std::ostringstream sql;
		sql << "insert into tb_game_world values(" << sid() << "," << speed_stage_id_ << "," << speed_stage_max_level_ << ",default,default,default)";
		MySQLUpdate(sql.str());

		sql.str("");
		sql << "insert into tb_trade values(" << sid() << ",";
		for (auto &it : trade_cards_)
			sql << it << ",";
		sql << trade_rewards_[0] << "," << trade_rewards_[1] << "," << trade_rewards_[2] << ")";
		MySQLUpdate(sql.str());

		__CreateExercise();
	}
}

void CGameWorld::__SaveToDatabase()
{
	CommitTask(0, [this]()
	{
		std::ostringstream sql;
		std::string str_sql;

		sql << "update tb_game_world set speed_stage_id = " << speed_stage_id_ << ", speed_stage_max_level = " << speed_stage_max_level_ << ", event_flag = " << event_hour_.to_ulong();
		if (false == is_open_)
			sql << ", last_close_time = now()";
		sql << " where sid = " << sid();
		MySQLUpdate(sql.str());

		sql.str("");
		sql << "update tb_trade set ";
		for (size_t i = 0; i < trade_cards_.size(); ++i)
			sql << "trade_card" << i + 1 << " = " << trade_cards_[i] << ", ";
		sql << "trade_reward1 = " << trade_rewards_[0] << ", trade_reward2 = " << trade_rewards_[1] << ", trade_reward3 = " << trade_rewards_[2] << " where sid = " << sid();
		MySQLUpdate(sql.str());

		{
			LOCK_BLOCK(guilds_lock_);
			for (auto &it : guilds_)
				it.second->SaveToDatabase();
		}

		//竞速赛排行
		sql.str("");
		sql << "delete from tb_speed_stage_ranking where sid = " << sid();
		MySQLUpdate(sql.str());
		{
			LOCK_BLOCK(speed_stage_lock_);
			if (!week_speed_ranking_.empty())
			{
				sql.str("");
				sql << "insert into tb_speed_stage_ranking values";
				for (auto &it : week_speed_ranking_)
					sql << "(" << sid() << "," << it.pid << "," << it.time << "," << it.heroes[0] << "," << it.heroes[1] << "," << it.heroes[2] << "," << it.soldiers[0] << "," << it.soldiers[1] << "," << it.soldiers[2] << "," << it.soldiers[3] << "," << it.soldiers[4] << "," << it.soldiers[5] << "),";
				str_sql = std::move(sql.str());
				str_sql.erase(str_sql.length() - 1, 1);
				MySQLUpdate(str_sql);
			}

		}
		

		//练功台
		LOCK_BLOCK(exercise_lock_);
		if (!exercise_platforms_.empty())
		{
			sql.str("");
			sql << "replace into tb_exercise_platform values";
			LOCK_BLOCK(exercise_lock_);
			for (auto &it : exercise_platforms_)
				sql << "(" << it.second->id << "," << sid() << "," << it.second->pid << "," << it.second->start_time << "," << it.second->level << "," << it.second->exp_mul << "),";
			str_sql = std::move(sql.str());
			str_sql.erase(str_sql.length() - 1, 1);
			MySQLUpdate(str_sql);
		}

		//玩家练功台经验
		sql.str("");
		sql << "delete from tb_player_exercise_exp where pid in (select pid from tb_player where sid =" << sid() << " )";
		MYSQL_UPDATE(sql.str());
		if (!offline_exp_bank_.empty())
		{
			sql.str("");
			sql << "insert into tb_player_exercise_exp values";
			for (auto &it : offline_exp_bank_)
				sql << "(" << it.first << "," << it.second->exp_ << "," << it.second->exercise_time_ << "),";
			str_sql = std::move(sql.str());
			str_sql.erase(str_sql.length() - 1, 1);
			MySQLUpdate(str_sql);
		}

		//服务器离线排行数据
		int rank = 1;
		sql.str("");
		sql << "delete from tb_offline_ranking where sid = " << sid();
		MYSQL_UPDATE(sql.str());
		{
			LOCK_BLOCK(offline_battle_lock_);
			if (!offline_battle_ranking_.empty())
			{
				sql.str("");
				sql << "insert into tb_offline_ranking values";

				for (auto &it : offline_battle_ranking_)
					sql << "(" << sid() << "," << rank++ << "," << it << "),";
				str_sql = std::move(sql.str());
				str_sql.erase(str_sql.length() - 1, 1);
				MySQLUpdate(str_sql);
			}
		}
		
	});

	LOCK_BLOCK(players_lock_);
	for (auto &it : online_players_)
		it.second->SaveToDatabase(!is_open_);
}

void CGameWorld::__InitializeTowns()
{
	World_Map_Library lib_towns;
	lib_towns.ParseFromString(GetDataFromFile(GAME_DATA_PATH"WorldMap.txt"));
	for (int i = 0; i < lib_towns.towns_library_size(); i++)
	{
		auto lib_town = lib_towns.towns_library(i);

		CTown* town = new CTown(lib_town.town_id());
		town->main_progress(lib_town.main_quest_progress());

		if (false == towns_.insert(std::make_pair(town->town_id(), town)).second)
		{
			RECORD_ERROR(FormatString("添加城镇失败,城镇ID:", town->town_id()));
			delete town;
		}
	}
}

void CGameWorld::__EnterGame(const CMessage& msg)
{
	pto_LOGIN_C2S_REQ_EnterGame pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (pto.pid() <= 0)
	{
		RECORD_WARNING(FormatString("登陆游戏PID错误,pid:", pto.pid()));
		return;
	}

	player_cache_lock_.lock();
	playerdata_cache_.erase(pto.pid());
	player_cache_lock_.unlock();

	SPPlayer player = FIND_PLAYER(pto.pid());

	bool result = true;

	if (player)
	{
		player->ChangeUser(msg.session());

		pto_LOGIN_S2C_RES_EnterGame pto_res;
		pto_res.set_result(result);
		pto_res.set_pid(player->pid());
		player->SendToAgent(pto_res, LOGIN_S2C_RES_EnterGame);
	}
	else
	{
		__AddPlayer(pto.pid(), msg.session());
	}
}

void CGameWorld::__LeaveGame(const CMessage& msg)
{
	__DeletePlayer(msg.GetID());
}

void CGameWorld::__AddPlayer(int pid, networkz::ISession* session)
{
	CommitTask(pid, [pid, session, this]()
	{
		bool result = false;

		std::string str;

		try
		{
			SPPlayer player = std::make_shared<CPlayer>(pid, session);

			LOCK_BLOCK(players_lock_);

			if (online_players_.insert(std::make_pair(pid, player)).second)
			{
				result = true;
				RECORD_INFO(FormatString("pid:", pid, " 登陆游戏,在线人数:", online_players_.size()));

				pto_PLAYER_S2C_NTF_FriendOnline ptoOnline;
				ptoOnline.set_pid(pid);
				ptoOnline.set_map_id(player->town_id());
				ptoOnline.SerializeToString(&str);
				SendToAllPlayers(str, MSG_S2C, PLAYER_S2C_NTF_FriendOnline, pid);
			}
		}
		catch (std::exception ex)
		{
			result = false;
			RECORD_ERROR(ex.what());
		}

		pto_LOGIN_S2C_RES_EnterGame ptoEnter;
		ptoEnter.set_result(result);
		ptoEnter.set_pid(pid);
		ptoEnter.SerializeToString(&str);
		SendToAgentServer(str, MSG_S2C, LOGIN_S2C_RES_EnterGame, pid, session);
	});
}

void CGameWorld::__DeletePlayer(int pid)
{
	SPPlayer player = FIND_PLAYER(pid);

	if (!player)
	{
		RECORD_WARNING(FormatString("找不到需要离线的玩家,pid:", pid));
		return;
	}

	player->ReadyToOffline();
	player->SaveToDatabase(true);

	LOCK_BLOCK(players_lock_);

	pto_PLAYER_S2C_NTF_FriendOnline ptoOnline;
	ptoOnline.set_pid(pid);
	ptoOnline.set_map_id(0);
	std::string str;
	ptoOnline.SerializeToString(&str);
	SendToAllPlayers(str, MSG_S2C, PLAYER_S2C_NTF_FriendOnline, pid);
}

void CGameWorld::__GenerateTradeData()
{
	trade_rewards_.fill(0);
	trade_cards_.fill(0);

	for (size_t i = 0; i < 2; i++)
	{
		int value{ 0 };
		do
		{
			value = GetRandom(1, 12);

		} while (IsInArray(trade_cards_, value, 0));

		trade_cards_[i] = value;
	}

	for (size_t i = 2; i < 5; i++)
	{
		int value{ 0 };
		do
		{
			value = GetRandom(1, 12);

		} while (IsInArray(trade_cards_, value, 2));

		trade_cards_[i] = value;
	}

	for (size_t i = 5; i < 9; i++)
	{
		int value{ 0 };
		do
		{
			value = GetRandom(1, 12);

		} while (IsInArray(trade_cards_, value, 5));

		trade_cards_[i] = value;
	}

	trade_rewards_[0] = CCombinationReward::GetCombinationReward(max_level_in_server_, 2);
	trade_rewards_[1] = CCombinationReward::GetCombinationReward(max_level_in_server_, 3);
	trade_rewards_[2] = CCombinationReward::GetCombinationReward(max_level_in_server_, 4);
}

void CGameWorld::__GenerateSpeedStageData()
{
	LOCK_BLOCK(speed_stage_lock_);

	speed_stage_id_ = CSpeedStage::ProduceSpeedStageID(speed_stage_id_);
	speed_stage_max_level_ = max_level_in_server_;
}

void CGameWorld::__CancelMatch(const CMessage &msg)
{
	pto_BATTLE_B2S_NTF_CancelMatch pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	for (int i = 0; i < pto.pid_size(); i++)
	{
		SPPlayer player = FindPlayer(pto.pid(i));
		if (player)
		{
			player->state(PlayerState::NORMAL);

			auto room = GAME_WORLD()->FindRoom(player->room_id());

			if (room)
				room->SetRoomState(RoomState::WAITING);

			pto_ROOM_S2C_NTF_MatchBattle pto;
			pto.set_is_match(false);
			player->SendToAgent(pto, ROOM_S2C_NTF_MatchBattle);
		}
	}
}

void CGameWorld::__OnCreateBattle(const CMessage& msg)
{
	pto_BATTLE_B2S_RES_CreateBattle pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	if (false == pto.result())
		return;

	for (int i = 0; i < pto.pids_size(); i++)
	{
		SPPlayer player = GAME_WORLD()->FindPlayer(pto.pids(i));

		if (player)
		{
			player->state(PlayerState::BATTLE);

			auto room = GAME_WORLD()->FindRoom(player->room_id());

			if (room)
				room->SetRoomState(RoomState::BATTLING);

			CTown* town{ GAME_WORLD()->FindTown(player->town_id()) };

			town->LeaveTown(player->pid());
		}
	}
}

void CGameWorld::__OnDeleteBattle(const CMessage& msg)
{
	pto_BATTLE_B2S_NTF_DeleteBattle pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	BattleMode mode = static_cast<BattleMode>(pto.mode());
	int map_id{ pto.mapid() };

	std::set<SPRoomZ> rooms;

	for (int i = 0; i < pto.win_pids_size(); ++i)
	{
		SPPlayer player = GAME_WORLD()->FindPlayer(pto.win_pids(i));

		if (player)
		{
			player->state(PlayerState::NORMAL);

			if (player->room_id())
			{
				auto room = GAME_WORLD()->FindRoom(player->room_id());
				rooms.insert(room);
			}

			if (mode == BattleMode::ARENA1v1 || mode == BattleMode::ARENA1v1Easy || mode == BattleMode::ARENA1v1Normal ||
				mode == BattleMode::ARENA3v3Easy || mode == BattleMode::ARENA3v3Normal || mode == BattleMode::ARENA3v3 || mode == BattleMode::EXERCISE)
			{
				player->ImplementMission(EMissionTargetType::MTT_FairArena, (int)mode, 0);
				player->ImplementRewardMission(RewardMissionType::RMT_Arena, 0, 1);
			}
			if (mode == BattleMode::WAR)
			{
				player->ImplementMission(EMissionTargetType::MTT_War, 0, 0);
			}
		}
		else
		{
			RECORD_WARNING("战斗结束找不到玩家");
		}
	}

	for (int i = 0; i < pto.lose_pids_size(); ++i)
	{
		SPPlayer player = GAME_WORLD()->FindPlayer(pto.lose_pids(i));
		if (player)
		{
			player->state(PlayerState::NORMAL);

			if (player->room_id())
			{
				auto room = GAME_WORLD()->FindRoom(player->room_id());
				rooms.insert(room);
			}

			if (mode == BattleMode::ARENA1v1 || mode == BattleMode::ARENA1v1Easy || mode == BattleMode::ARENA1v1Normal
				|| mode == BattleMode::ARENA3v3Easy || mode == BattleMode::ARENA3v3Normal || mode == BattleMode::ARENA3v3)
			{
				player->ImplementMission(EMissionTargetType::MTT_FairArena, (int)mode, 0);
				player->ImplementRewardMission(RewardMissionType::RMT_Arena, 0, 1);
			}
			if (mode == BattleMode::WAR)
			{
				player->ImplementMission(EMissionTargetType::MTT_War, 0, 0);
			}
			if (mode == BattleMode::ELITESTAGE)
				player->need_reset()->UpdateEliteStage();
			if (mode == BattleMode::MULTISTAGE)
				player->need_reset()->UpdateMultiStage();
		}
		else
		{
			RECORD_WARNING("战斗结束找不到玩家");
		}
	}

	for (auto &it : rooms)
	{
		it->CancelAllReady();
		it->SetRoomState(RoomState::WAITING);
	}

	for (int i = 0; i < pto.win_pids_size(); ++i)
	{
		SPPlayer player = GAME_WORLD()->FindPlayer(pto.win_pids(i));

		if (!player)
			continue;

		switch (mode)
		{
		case BattleMode::STAGE:
		{
			const CStage* stage = CStage::GetStage(map_id);
			if (stage)
			{
				player->StageBattleWin(stage);
				for (int i = 0; i < pto.stage_loot_ex_size(); i++)
				{
					if (pto.stage_loot_ex(i).pid() == player->pid())
					{
						for (int k = 0; k < pto.stage_loot_ex(i).stage_loot_size(); k++)
						{
							player->knapsack()->GiveNewItem(pto.stage_loot_ex(i).stage_loot(k).nitemid(), pto.stage_loot_ex(i).stage_loot(k).nitemnum());
						}
					}
				}
			}
		}
			break;
		case BattleMode::ELITESTAGE:
		{
			const CEliteStage* elite_stage = CEliteStage::GetEliteStageByMapID(map_id);
			if (elite_stage)
			{
				player->EliteStageBattleWin(pto.box_num(), elite_stage);
				player->need_reset()->UpdateEliteStage();
				for (int i = 0; i < pto.stage_loot_ex_size(); i++)
				{
					if (pto.stage_loot_ex(i).pid() == player->pid())
					{
						for (int k = 0; k < pto.stage_loot_ex(i).stage_loot_size(); k++)
						{
							player->knapsack()->GiveNewItem(pto.stage_loot_ex(i).stage_loot(k).nitemid(), pto.stage_loot_ex(i).stage_loot(k).nitemnum());
						}
					}
				}
			}

		}
			break;
		case BattleMode::MULTISTAGE:
		{
			const CEliteStage* multi_stage = CEliteStage::GetMultiStageByMapID(map_id);

			if (multi_stage)
			{
				player->MultiStageBattleWin(pto.box_num(), multi_stage);
				player->need_reset()->UpdateMultiStage();
				for (int i = 0; i < pto.stage_loot_ex_size(); i++)
				{
					if (pto.stage_loot_ex(i).pid() == player->pid())
						for (int k = 0; k < pto.stage_loot_ex(i).stage_loot_size(); k++)
							player->knapsack()->GiveNewItem(pto.stage_loot_ex(i).stage_loot(k).nitemid(), pto.stage_loot_ex(i).stage_loot(k).nitemnum());

				}
			}
		}
			break;
		case BattleMode::SPEED:
			if (map_id == CSpeedStage::GetMapID(GAME_WORLD()->speed_stage_id()))
			{
				player->SpeedStageWin(pto.use_time());
				break;
			}
		}
	}
}

void CGameWorld::__GameOver(const CMessage& msg)
{
	pto_BATTLE_S2C_NTF_Game_Over pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	SPPlayer player{ GAME_WORLD()->FindPlayer(msg.GetID()) };

	if (player)
	{
		if (IsCrossRealm(static_cast<BattleMode>(pto.mode())))
			player->OnGameOver(pto);
		else
			GAME_WORLD()->SendToAgentServer(msg, player->pid(), player->session());
	}
	else
	{
		if (IsCrossRealm(static_cast<BattleMode>(pto.mode())))
		{
			RECORD_WARNING("跨服战斗未结算");
		}
	}
}

void CGameWorld::__ExercisePlatformLoop()
{
	for (int i = 0; i < ExercisePlatformsSize(); i++)
	{
		auto platform = GetExercisePlatform(i + 1);
		if (platform)
		{
			if (!platform->pid)
				continue;
			SPPlayer player = FindPlayer(platform->pid);
			if (player)
			{
				const CRootData* data = CRootData::GetRootData(player->level());
				if (player->exercise_time() < EverydayMaxExercisePlatformTimes)
				{
					player->exercise()->AddExerciseTime();
					if (data)
						player->exercise()->ChangeExpBox(__int64(platform->exp_mul * data->exercise_platform_exp()));
					pto_PLAYER_S2C_NTF_UpdatePlatformExp pto_ntf;
					pto_ntf.set_exp_box(player->exercise()->exp_box());
					pto_ntf.set_exercise_time(player->exercise()->exercise_time());
					player->SendToAgent(pto_ntf, PLAYER_S2C_NTF_UpdatePlatformExp);
				}

				time_t capture_time = time(0) - platform->start_time;
				if ((capture_time >= 3540 && capture_time < 3600) ||
					(capture_time >= 7140 && capture_time < 7200) ||
					(capture_time >= 10740 && capture_time < 10800) ||
					(capture_time >= 14340 && capture_time < 14400) ||
					(capture_time >= 17940 && capture_time < 18000) ||
					(capture_time >= 21540 && capture_time < 21600))
					//if ((capture_time >= 0 && capture_time < 60) ||
					//	(capture_time >= 60 && capture_time < 120) ||
					//	(capture_time >= 120 && capture_time < 180))
				{
					auto pill = GAME_WORLD()->GetExpPill(i + 1);
					if (pill)
					{
						pill->times = GetRandom(3, 10);
						pill->exp = __int64((platform->exp_mul * data->exercise_platform_exp()) * 60 * GetRandom(8, 20) / 100);
						pill->taken_player.clear();

						pto_PLAYER_S2C_NTF_UpdateExpPill pto_ntf_pill;
						pto_PLAYER_STRUCT_ExpPill* pto_pill = pto_ntf_pill.mutable_pill();
						pto_pill->set_id(pill->id);
						pto_pill->set_collect(true);

						std::string str_pto_ntf;
						pto_ntf_pill.SerializeToString(&str_pto_ntf);

						SendExercisePlatform(str_pto_ntf, MSG_S2C, PLAYER_S2C_NTF_UpdateExpPill);
					}
				}

				if (capture_time >= ExercisePlatformMaxOccupyTime)
				{
					player->exercise()->SendDownFromPlatform(1, 0, time(0));
					platform->pid = 0;
					platform->start_time = 0;

					pto_PLAYER_S2C_NTF_ChangePlatformOwner pto_ntf;
					pto_PLAYER_STRUCT_Platform* pto_platform = pto_ntf.mutable_platform();
					pto_platform->set_id(platform->id);
					pto_platform->set_pid(platform->pid);
					pto_platform->set_start_time(platform->start_time);
					pto_platform->set_level(0);
					pto_platform->set_guild_id(0);

					std::string str_pto;
					pto_ntf.SerializeToString(&str_pto);
					SendExercisePlatform(str_pto, MSG_S2C, PLAYER_S2C_NTF_ChangePlatformOwner);
				}
			}

			else
			{
				SPIPlayerData player_data = PLAYER_DATA(platform->pid);
				if (player_data)
				{
					const CRootData* data = CRootData::GetRootData(player_data->level());
					int offline_exercise_time = 0;
					auto offline_exp_ = GAME_WORLD()->GetOfflineExp(platform->pid);
					if (offline_exp_)
						offline_exercise_time = offline_exp_->exercise_time_;

					if (offline_exercise_time + player_data->exercise_time() < EverydayMaxExercisePlatformTimes)
					{
						__int64 get_exp = 0;
						if (data)
							get_exp = __int64(platform->exp_mul * data->exercise_platform_exp());
						GAME_WORLD()->AddOfflinExpBank(platform->pid, get_exp);
					}

					time_t capture_time = time(0) - platform->start_time;
					if ((capture_time >= 3540 && capture_time < 3600) ||
						(capture_time >= 7140 && capture_time < 7200) ||
						(capture_time >= 10740 && capture_time < 10800) ||
						(capture_time >= 14340 && capture_time < 14400) ||
						(capture_time >= 17940 && capture_time < 18000) ||
						(capture_time >= 21540 && capture_time < 21600))
					{
						auto pill = GAME_WORLD()->GetExpPill(i + 1);
						if (pill)
						{
							pill->times = GetRandom(3, 10);
							pill->exp = __int64((platform->exp_mul * data->exercise_platform_exp()) * 60 * GetRandom(8, 20) / 100);
							pill->taken_player.clear();

							pto_PLAYER_S2C_NTF_UpdateExpPill pto_ntf_pill;
							pto_PLAYER_STRUCT_ExpPill* pto_pill = pto_ntf_pill.mutable_pill();
							pto_pill->set_id(pill->id);
							pto_pill->set_collect(true);

							std::string str_pto_ntf;
							pto_ntf_pill.SerializeToString(&str_pto_ntf);

							SendExercisePlatform(str_pto_ntf, MSG_S2C, PLAYER_S2C_NTF_UpdateExpPill);
						}
					}

					if (capture_time >= ExercisePlatformMaxOccupyTime)
					{
						AddDownFromPlatform(platform->pid, 1, 0, time(0));
						platform->pid = 0;
						platform->start_time = 0;

						pto_PLAYER_S2C_NTF_ChangePlatformOwner pto_ntf;
						pto_PLAYER_STRUCT_Platform* pto_platform = pto_ntf.mutable_platform();
						pto_platform->set_id(platform->id);
						pto_platform->set_pid(platform->pid);
						pto_platform->set_start_time(platform->start_time);
						pto_platform->set_level(0);
						pto_platform->set_guild_id(0);

						std::string str_pto;
						pto_ntf.SerializeToString(&str_pto);
						SendExercisePlatform(str_pto, MSG_S2C, PLAYER_S2C_NTF_ChangePlatformOwner);
					}
				}
			}
		}
	}
}

void CGameWorld::__CreateExercise()
{
	dat_EXERCISEPLATFORM_STRUCT_ExercisePlatformLibrary exercise_platform_library;
	exercise_platform_library.ParseFromString(GetDataFromFile(GAME_DATA_PATH"ExercisePlatform.txt"));

	std::ostringstream sql;

	for (int i = 0; i < exercise_platform_library.exercise_platform_library_size(); i++)
	{
		auto pto_platform = exercise_platform_library.exercise_platform_library(i);
		SPExercisePlatform platform = std::make_shared<CExercisePlatform>();

		platform->id = pto_platform.id();
		platform->level = pto_platform.level();
		platform->exp_mul = pto_platform.exp_mul();
		exercise_platforms_[platform->id] = platform;

		sql.str("");
		sql << "replace into tb_exercise_platform values(" << platform->id << "," << sid() << ",default,default," << platform->level << "," << platform->exp_mul << ")";
		MYSQL_UPDATE(sql.str());

		SPExpPill pill = std::make_shared<ExpPill>();
		pill->id = pto_platform.id();
		exp_pills_.insert(std::make_pair(pill->id, pill));
	}
	printf(FormatString("加载", exercise_platforms_.size(), "个练功台子\n").c_str());
}

#pragma region 离线模块
void CGameWorld::SendEscortPlayer(const std::string& content, unsigned char protocol_type, int protocol_id)
{
	LOCK_BLOCK(offline_battle_lock_);

	for (auto &it : online_players_)
		if (it.second->offline()->IsInEscrot())
			SendToAgentServer(content, protocol_type, protocol_id, it.second->pid(), it.second->session());
}

int	 CGameWorld::AddOfflineBattleRanking(int pid)
{
	LOCK_BLOCK(offline_battle_lock_);

	offline_battle_ranking_.push_back(pid);

	return offline_battle_ranking_.size();
}

int	 CGameWorld::GetPIDByRank(int rank)
{
	LOCK_BLOCK(offline_battle_lock_);

	if (offline_battle_ranking_.empty())
		return 0;

	if (offline_battle_ranking_.size() < (size_t)rank)
		return 0;

	auto it = offline_battle_ranking_.begin();

	for (int i = 1; i < rank; i++)
	{
		if (offline_battle_ranking_.cend() == it)
			return 0;
		++it;
	}

	return *it;
}

int	 CGameWorld::GetOfflinePlayerRank(int pid)
{
	LOCK_BLOCK(offline_battle_lock_);

	SPPlayer player{ FIND_PLAYER(pid) };

	int rank{ 1 };

	for (auto &it : offline_battle_ranking_)
	{
		if (it == pid)
		{
			if (player)
				player->offline()->offline_rank(rank);
			return rank;
		}

		rank++;
	}

	if (player)
		player->offline()->offline_rank(0);

	return 0;
}

void CGameWorld::AddOffLineBattleSaveData(pto_OFFLINEBATTLE_Struct_SaveOfflineBattle* save_data)
{
	LOCK_BLOCK(offline_battle_lock_);

	pto_OFFLINEBATTLE_Struct_SaveOfflineBattle* record = new pto_OFFLINEBATTLE_Struct_SaveOfflineBattle;

	record->CopyFrom(*save_data);

	if (false == offLine_battle_save_data.insert(std::make_pair(record->id(), record)).second)
		delete record;
}

bool CGameWorld::GetPlayersRank(int& nAtkRank, int& nDefRank, CPlayer* pAtkPlayer, int nDefPID)
{
	LOCK_BLOCK(offline_battle_lock_);

	bool m_bResourcesFound = false;

	for (auto &it : offline_battle_ranking_)
	{
		if (nAtkRank >= (int)offline_battle_ranking_.size())
			break;

		if (it == pAtkPlayer->pid())
		{
			m_bResourcesFound = true;
			break;
		}

		nAtkRank++;
	}

	for (auto &it : offline_battle_ranking_)
	{
		if (nDefRank >= (int)offline_battle_ranking_.size())
			break;

		if (it == nDefPID)
			break;

		nDefRank++;
	}

	return m_bResourcesFound;
}

void CGameWorld::ResetOfflineRank(int& nAtkRank, int& nDefRank, CPlayer* pAtkPlayer, int nDefUId, bool bResourcesFound)
{
	if (nullptr == pAtkPlayer)
	{
		printf("未能找到玩家");
		return;
	}

	if (nAtkRank <= nDefRank)
		return;

	//if (true == bResourcesFound)
	__EraseOfflineRanking(nDefRank);
	__InsertOfflineRanking(nDefRank, pAtkPlayer->pid());

	__EraseOfflineRanking(nAtkRank);
	__InsertOfflineRanking(nAtkRank, nDefUId);

}

void CGameWorld::SavePlayerOfflineBattleData(bool bChallenge, int id, time_t nTime, bool bWin, int nPlayerPID, int pid)
{
	OffLineBattleData* pData = new OffLineBattleData;
	pData->m_bChallenge = bChallenge;
	pData->m_nId = id;
	pData->m_nTime = nTime;
	pData->m_bWin = bWin;
	pData->m_nEnemyPId = pid;
	LOCK_BLOCK(offline_battle_lock_);
	offline_datas_.insert(std::make_pair(nPlayerPID, pData));
}

void CGameWorld::UpdatePlayerOfflineBattleData(CPlayer* player)
{
	LOCK_BLOCK(offline_battle_lock_);

	if (!GetOfflinePlayerRank(player->pid()))
	{
		AddOfflineBattleRanking(player->pid());
		player->offline()->offline_rank(offline_battle_ranking_.size());
	}

	pto_OFFLINEBATTLE_S2C_RES_UpdatePlayerOfflineBattleData pto;

	for (auto &it : player->army()->GetFormationHero())
		pto.add_hero_id(it);

	pto.set_ranking(player->offline()->offline_rank());
	pto.set_times(player->need_reset()->surplus_times());
	pto.set_continuous(player->offline()->win_streak());
	if (player->offline()->last_time() < 0)
		pto.set_last_times(0);
	else
		pto.set_last_times(player->offline()->last_time());
	pto.set_buy_times(player->need_reset()->offline_buy_times());

	player->offline()->GetEnemyData(&pto);

	auto itValue = offline_datas_.equal_range(player->pid());

	if (itValue.first != itValue.second)
	{
		std::vector<pto_OFFLINEBATTLE_Struct_PlayerOfflineBattleSaveData> vctPOBSD;
		for (auto it = itValue.first; it != itValue.second; ++it)
		{
			pto_OFFLINEBATTLE_Struct_PlayerOfflineBattleSaveData ptoPOBSD;
			ptoPOBSD.set_id(it->second->m_nId);
			ptoPOBSD.set_challenge(it->second->m_bChallenge);
			ptoPOBSD.set_time(it->second->m_nTime);
			ptoPOBSD.set_win(it->second->m_bWin);

			if (it->second->m_nEnemyPId > 0)
			{
				SPIPlayerData base_data = PLAYER_DATA(it->second->m_nEnemyPId);
				if (base_data)
					ptoPOBSD.set_enmey_name(base_data->name());
			}

			else
			{
				COffLinePlayer* pAIPlayer = COffLinePlayer::FindOffLinePlayerByPID(it->second->m_nEnemyPId);
				if (pAIPlayer)
				{
					ptoPOBSD.set_enmey_name(pAIPlayer->GetName());
				}
			}

			ptoPOBSD.set_pid(it->second->m_nEnemyPId);
			vctPOBSD.push_back(ptoPOBSD);
		}
		int times = 0;
		for (int i = vctPOBSD.size() - 1; i >= 0 && times <= 5; i--)
		{
			auto pPOBSD = pto.add_save_data();
			*pPOBSD = vctPOBSD.at(i);
			times++;
		}
	}

	player->SendToAgent(pto, PLAYER_S2C_RES_UpdatePlayerOfflineBattleData);
}

void CGameWorld::GetOfflineList(CPlayer* player)
{
	LOCK_BLOCK(offline_battle_lock_);

	pto_OFFLINE_S2C_RES_OfflineList pto;

	int nListNum = offline_battle_ranking_.size();

	nListNum = nListNum < 20 ? nListNum : 20;

	for (int i = 0; i < nListNum; i++)
	{
		pto_OFFLINEBATTLE_STRUCT_OfflineList* pList = pto.add_offline_list();
		pList->set_rank(i + 1);
		pList->set_pid(GetPIDByRank(i + 1));
	}

	player->SendToAgent(pto, OFFLINEBATTLE_S2C_RES_OfflineList);
}

const pto_OFFLINEBATTLE_Struct_SaveOfflineBattle* CGameWorld::GetOfflineBattleSavedata(int nID)
{
	LOCK_BLOCK(offline_battle_lock_);

	auto it = offLine_battle_save_data.find(nID);
	if (it == offLine_battle_save_data.cend())
		return nullptr;
	else
		return it->second;
}

void CGameWorld::StartWorldBoss()
{
	world_boss_rankings_.clear();
	__ProduceWorldBoss();
}

void CGameWorld::EndWorldBoss()
{
	world_boss_rankings_.clear();
	world_boss_.m_nHP = 0;
}

SWorldBossRank*	CGameWorld::FindWorldBossRanking(int pid)
{
	LOCK_BLOCK(offline_battle_lock_);

	for (auto &it : world_boss_rankings_)
	{
		if (it->m_nPID == pid)
			return it;
	}

	return nullptr;
}

void CGameWorld::ChangeWorldBossRank(int pid, int damage)
{
	LOCK_BLOCK(offline_battle_lock_);

	for (auto it = world_boss_rankings_.begin(); it != world_boss_rankings_.cend(); ++it)
	{
		if ((*it)->m_nPID == pid)
		{
			delete *it;
			world_boss_rankings_.erase(it);
			break;
		}
	}

	SWorldBossRank* rank{ new SWorldBossRank };
	rank->m_nPID = pid;
	rank->m_nDmg = damage;

	for (auto it = world_boss_rankings_.begin(); it != world_boss_rankings_.cend(); ++it)
	{
		if ((*it)->m_nDmg < damage)
		{
			world_boss_rankings_.insert(it, rank);
			return;
		}
	}

	world_boss_rankings_.push_back(rank);
}

void CGameWorld::UpdateWorldBoss(CPlayer* player)
{
	pto_OFFLINEBATTLE_S2C_RES_UpdateWorldBoss pto;
	pto.set_boss_hp(world_boss_.m_nHP);
	pto.set_boss_max_hp(world_boss_.m_nMaxHP);
	pto.set_last_times(0);

	LOCK_BLOCK(offline_battle_lock_);

	for (auto it = world_boss_rankings_.begin(); it != world_boss_rankings_.cend(); ++it)
	{
		if ((*it)->m_nPID == player->pid())
		{
			pto.set_last_times((*it)->m_nLastTime);
		}
	}

	for (size_t nRank = 1; nRank <= 10; nRank++)
	{
		const SWorldBossRank* pData = GetWorldRankingDataByRank(nRank);

		if (pData != nullptr)
		{
			pto_OFFLINEBATTLE_STRUCT_WorldBossRank* pRank = pto.add_ranks();
			pRank->set_pid(pData->m_nPID);
			pRank->set_dmg(pData->m_nDmg);
			pRank->set_rank(nRank);
		}
	}

	player->SendToAgent(pto, OFFLINEBATTLE_S2C_RES_UpdateWorldBoss);
}

const SWorldBossRank* CGameWorld::GetWorldRankingDataByRank(int rank)
{
	LOCK_BLOCK(offline_battle_lock_);

	if (world_boss_rankings_.empty())
		return nullptr;

	if (world_boss_rankings_.size() < (size_t)rank)
		return nullptr;

	if (rank <= 0)
		return nullptr;

	auto it = world_boss_rankings_.begin();

	for (int i = 1; i < rank; i++)
	{
		if (world_boss_rankings_.cend() == it)
			return nullptr;
		++it;
	}

	return *it;
}

PlayerEscort* CGameWorld::FindEscort(int nPID)
{
	LOCK_BLOCK(offline_battle_lock_);

	auto it = offline_escorts_.find(nPID);

	if (it == offline_escorts_.cend())
		return nullptr;
	else
		return it->second;
}

void CGameWorld::InserEscort(PlayerEscort* pescort)
{
	LOCK_BLOCK(offline_battle_lock_);

	if (false == offline_escorts_.insert(std::make_pair(pescort->m_nPId, pescort)).second)
		delete pescort;
}

void CGameWorld::DeleteEscort(int nPID)
{
	LOCK_BLOCK(offline_battle_lock_);

	auto it = offline_escorts_.find(nPID);

	if (it != offline_escorts_.cend())
	{
		delete it->second;
		offline_escorts_.erase(it);
	}
}

void CGameWorld::UpdateEscort(pto_OFFLINEBATTLE_S2C_RES_UpdateEscort* pto)
{
	LOCK_BLOCK(offline_battle_lock_);

	for (auto &it : offline_escorts_)
	{
		SPIPlayerData data = PLAYER_DATA(it.second->m_nPId);

		if (data)
		{
			pto_OFFLINEBATTLE_S2C_NTF_PlayereEscort* pESPD = pto->add_player_escort();
			pESPD->set_hp(it.second->m_nHp);
			pESPD->set_y(it.second->m_nY);
			pESPD->set_p_id(it.second->m_nPId);
			pESPD->set_car_lv(it.second->level);
			pESPD->set_lv(data->level());
			pESPD->set_name(data->name());
			pESPD->set_time(it.second->m_nStartTime);

			SPIPlayerData protector_data = PLAYER_DATA(data->bodygurand());

			if (protector_data)
				pESPD->set_protecter_name(protector_data->name());
			else
				pESPD->set_protecter_name("");
		}
	}
}

void CGameWorld::SendOfflineBattleReward()
{
	LOCK_BLOCK(offline_battle_lock_);

	int rank{ 1 };
	__int64 silver = CRootData::GetRootData(max_level_in_server_, RDT_OfflineBattleWeekSilver);
	__int64 reputation = CRootData::GetRootData(max_level_in_server_, RDT_OfflineBattleWeekReputation);

	for (auto &it : offline_battle_ranking_)
	{
		__SendOfflineBattleRankRewardMail(it, rank, reputation, silver);
		rank++;
	}
}

void CGameWorld::__EraseOfflineRanking(int nRank)
{
	LOCK_BLOCK(offline_battle_lock_);

	if (nRank <= 0 || nRank > (int)offline_battle_ranking_.size())
		return;

	int nNum{ 1 };

	for (auto it = offline_battle_ranking_.begin(); it != offline_battle_ranking_.cend(); ++it)
	{
		if (nNum == nRank)
		{
			offline_battle_ranking_.erase(it);
			break;
		}
		nNum++;
	}
}

void CGameWorld::__InsertOfflineRanking(int nRank, int nPID)
{
	LOCK_BLOCK(offline_battle_lock_);

	int nNum{ 1 };

	for (auto it = offline_battle_ranking_.begin(); it != offline_battle_ranking_.cend(); ++it)
	{
		if (nNum == nRank)
		{
			offline_battle_ranking_.insert(it, nPID);
			return;
		}
		nNum++;
	}
	offline_battle_ranking_.insert(offline_battle_ranking_.cend(), nPID);
}

void CGameWorld::__ProduceWorldBoss()
{
	world_boss_.m_nStr = world_boss_.m_nCmd = world_boss_.m_nInt = ((max_level_in_server_ * 2) + 30);
	__int64 hp = CAddition::GetCustomMasterEquipHP(max_level_in_server_);
	world_boss_.m_nHP = world_boss_.m_nMaxHP = hp * 30000;
}

void CGameWorld::__SendOfflineBattleRankRewardMail(int pid, int rank, __int64 reputation, __int64 silver)
{
	if (pid < 0)
		return;

	const COfflineRewardStage* reward = COfflineRewardStage::GetOfflineRewardStage(rank);
	if (!reward)
		return;

	__int64 final_reputation = __int64(reputation + (reward->GetStage() - ((rank - reward->GetRank()) * reward->GetStageReduce())));
	__int64 final_silver = __int64((silver + (reward->GetStage() - ((rank - reward->GetRank()) * reward->GetStageReduce()))) * reward->GetCoefficient());

	SPMail mail = std::make_shared<CMail>();
	mail->SetModelID(MMI_OfflinBattle);
	mail->AddRewardItem(LT_Silver, final_silver);
	mail->AddRewardItem(LT_Reputation, final_reputation);
	mail->SetParam0(rank);

	if (rank <= 10)
		mail->SetRewardTitle(10);
	else if (rank <= 30)
		mail->SetRewardTitle(9);

	SPIPlayerData data = PLAYER_DATA(pid);
	if (!data)
		return;

	time_t offline_time = data->offline_time();

	if (-1 == data->offline_last_time())
		return;

	if (0 == offline_time || (time(0) - offline_time) < 259200)
		GiveNewMail(mail, pid);
}

void CGameWorld::__FinishEscort()
{
	time_t now_time = time(0);

	LOCK_BLOCK(offline_battle_lock_);
	for (auto it = offline_escorts_.begin(); it != offline_escorts_.cend();)
	{
		SPIPlayerData data = PLAYER_DATA(it->second->m_nPId);

		if (data)
		{
			int m_nFinishTime = 0;

			switch (it->second->level)
			{
			case 1:
				m_nFinishTime = 1200;
				break;
			case 2:
				m_nFinishTime = 1500;
				break;
			case 3:
				m_nFinishTime = 1800;
				break;
			case 4:
				m_nFinishTime = 2100;
				break;
			case 5:
				m_nFinishTime = 2400;
				break;
			default:
				break;
			}

			if (now_time - it->second->m_nStartTime >= m_nFinishTime)
			{
				int nReward = COffline::GetEscortReward(data->level(), it->second->level, it->second);
				int reputation = COffline::GetEscortReputation(data->level(), it->second->level, it->second);

				SPMail mail = std::make_shared<CMail>();
				mail->SetModelID(MMI_Escort);
				mail->AddRewardItem(LT_Silver, nReward);
				mail->AddRewardItem(LT_Reputation, reputation);

				GiveNewMail(mail, it->first);

				delete it->second;
				it = offline_escorts_.erase(it);
			}
			else
			{
				it++;
			}
		}
	}
}

void CGameWorld::__CloseFinishEscort()
{
	LOCK_BLOCK(offline_battle_lock_);

	time_t now_time = time(0);

	for (auto it = offline_escorts_.begin(); it != offline_escorts_.cend();)
	{
		SPIPlayerData data = PLAYER_DATA(it->second->m_nPId);

		if (data)
		{
			int nReward = COffline::GetEscortReward(data->level(), it->second->level, it->second);
			int reputation = COffline::GetEscortReputation(data->level(), it->second->level, it->second);

			SPMail mail = std::make_shared<CMail>();
			mail->SetModelID(MMI_Escort);
			mail->AddRewardItem(LT_Silver, nReward);
			mail->AddRewardItem(LT_Reputation, reputation);

			GiveNewMail(mail, it->first);

			delete it->second;
			it = offline_escorts_.erase(it);
		}
		else
		{
			it++;
		}
	}
}
#pragma endregion

#pragma region 结算奖励
void CGameWorld::__SendSpeedStageReward()
{
	LOCK_BLOCK(speed_stage_lock_);

	int rank{ 1 };

	//TODO重置玩家最好时间，玩家更新最高排名
	for (auto &it : week_speed_ranking_)
	{
		SPPlayer player = FIND_PLAYER(it.pid);

		if (player)
		{
			player->set_speed_best_rank(rank);
		}
		else
		{
			std::ostringstream sql;
			sql << "update tb_player_award set speed_stage_record = " << rank << " where pid = " << it.pid << " and speed_stage_record > " << rank;
			MySQLUpdate(sql.str());
		}

		SPIPlayerData data = PLAYER_DATA(it.pid);
		if (data)
		{
			__int64 silver = CRootData::GetRootData(data->level(), RDT_SpeedSilver);
			__int64 honor = CRootData::GetRootData(data->level(), RDT_SpeedHonour);
			__SendSpeedStageRankRewardMail(it.pid, rank, silver, honor);
		}
		rank++;
	}
}

__int64 CGameWorld::__GetBaseReward(LotteryType type)
{
	__int64 reward{ 0 };

	switch (type)
	{
	case LT_Silver:
		reward = CRootData::GetRootData(max_level_in_server_, RDT_SpeedSilver);
		break;
	case LT_Honour:
		reward = CRootData::GetRootData(max_level_in_server_, RDT_SpeedHonour);
		break;
	}

	return reward;
}

void CGameWorld::__SendSpeedStageRankRewardMail(int pid, int rank, __int64 silver, __int64 honour)
{
	__int64 final_silver = (__int64)(silver * CSpeedStage::GetRewardCoefficient(rank));
	__int64 final_honor = (__int64)(honour * CSpeedStage::GetRewardCoefficient(rank));

	SPMail mail = std::make_shared<CMail>();
	mail->SetModelID(MMI_SpeedStage);
	mail->AddRewardItem(LT_Silver, final_silver);
	mail->AddRewardItem(LT_Honour, final_honor);
	mail->AddRewardItem(LT_Reputation, CSpeedStage::GetGoldReward(rank) / 2);
	mail->SetParam0(rank);

	if (1 == rank)
		mail->SetRewardTitle(15);

	GiveNewMail(mail, pid);
}

void CGameWorld::__ResetSpeedStageData()
{
	__GenerateSpeedStageData();

	LOCK_BLOCK(speed_stage_lock_);
	week_speed_ranking_.clear();
}

void CGameWorld::__SendFairReward(int level, int point, int type, int pid)
{
	int			gold{ 0 };
	__int64		silver{ 0 };

	float mul{ 0 };

	switch (type)
	{
	case 1:
		mul = 0.2f;
		break;
	case 3:
		mul = 0.5f;
		break;
	}

	if (point >= 3000)
		gold = (int)(6000 * mul);
	else if (point >= 2500)
		gold = (int)(3000 * mul);
	else if (point >= 2000)
		gold = (int)(2200 * mul);
	else if (point >= 1800)
		gold = (int)(1600 * mul);
	else if (point >= 1700)
		gold = (int)(1000 * mul);
	else if (point >= 1400)
		gold = (int)(600 * mul);
	else
		gold = (int)(200 * mul);

	silver = __int64(CRootData::GetRootData(level, RootDataType::RDT_ArenaSilver) / 1500.0f * point * mul);

	SPMail mail = std::make_shared<CMail>();
	mail->SetModelID(MMI_FairArena);
	mail->AddRewardItem(LT_Gold, gold);
	mail->AddRewardItem(LT_Silver, silver);
	mail->SetParam0(point);

	GiveNewMail(mail, pid);
}

void CGameWorld::__SendWarReward(int level, int point, int pid)
{
	int	gold{ 0 };
	int honor = int(point * 1.5f);

	if (honor < 1000)
		honor = 1000;
	else if (honor > 5000)
		honor = 5000;

	if (point >= 3000)
		gold = 5000;
	else if (point >= 2500)
		gold = 3000;
	else if (point >= 2000)
		gold = 1500;
	else if (point >= 1800)
		gold = 500;
	else if (point >= 1700)
		gold = 200;
	else if (point >= 1400)
		gold = 50;
	else
		gold = 10;

	SPMail mail = std::make_shared<CMail>();
	mail->SetModelID(MMI_TerritoryBattle);
	mail->AddRewardItem(LT_Gold, gold);
	mail->AddRewardItem(LT_Honour, honor);
	mail->SetParam0(point);
	GiveNewMail(mail, pid);
}

void CGameWorld::__WindUpArenaReward()
{
	std::ostringstream sql;
	sql << "select distinct tb_player.`pid` from tb_player_record, tb_player where tb_player.`pid` = tb_player_record.`pid` and tb_player.`pid` in(select pid from tb_player where sid = " << sid()
		<< ") and tb_player.`pid` in (select pid from tb_player_need_reset where week_war_times >= 3 or week_1v1_times >= 3 or week_3v3_times >= 3)";

	ResultSetPtr result_pids{ MYSQL_QUERY(sql.str()) };

	if (!result_pids)
		return;
	while (result_pids->next())
	{
		SPIPlayerData data = PLAYER_DATA(result_pids->getInt(1));
		if (data)
		{
			if (data->battle_week_times(BattleRecordType::kBattleRecordWar) >= 3)
				__SendWarReward(data->level(), data->battle_record(BattleRecordType::kBattleRecordWar).point, data->pid());
			if (data->battle_week_times(BattleRecordType::kBattleRecord1v1) >= 3)
				__SendFairReward(data->level(), data->battle_record(BattleRecordType::kBattleRecord1v1).point, 1, data->pid());
			if (data->battle_week_times(BattleRecordType::kBattleRecord3v3) >= 3)
				__SendFairReward(data->level(), data->battle_record(BattleRecordType::kBattleRecord3v3).point, 3, data->pid());
		}
	}

	players_lock_.lock();
	for (auto &it : online_players_)
		it.second->need_reset()->ResetBattleWeekTimes();
	players_lock_.unlock();

	sql.str("");
	sql << "update tb_player_need_reset set week_war_times = default, week_1v1_times = default, week_3v3_times = default where pid in(select pid from tb_player where sid = " << sid() << ")";
	MySQLUpdate(sql.str());
}

void CGameWorld::__WindUpTrade()
{
	__GenerateTradeData();

	players_lock_.lock();
	for (auto &it : online_players_)
		it.second->need_reset()->WeekResetTrade();
	players_lock_.unlock();

	std::ostringstream sql;
	sql.str("");
	sql << "update tb_player_need_reset set trade_hands = '', trade_got_award_w = default where pid in (select pid from tb_player where sid = " << sid() << ")";
	MySQLUpdate(sql.str());
}

void CGameWorld::__WindUpSpeedStage()
{
	__SendSpeedStageReward();
	__ResetSpeedStageData();

	LOCK_BLOCK(players_lock_);
	for (auto &it : online_players_)
		it.second->need_reset()->speed_today_best_time(0);

	std::ostringstream sql;
	sql << "update tb_player_need_reset set speed_today_best_time  = default where pid in (select pid from tb_player where sid =" << sid() << ")";
	MYSQL_UPDATE(sql.str());
}

void CGameWorld::__RoutineReset()
{
	{
		LOCK_BLOCK(players_lock_);

		for (auto &it : online_players_)
		{
			it.second->need_reset()->RoutineReset();
			it.second->exercise()->RoutineReset();
		}
	}

	std::ostringstream sql;
	sql << "update tb_player_need_reset set snake_award_d = default"
		", puzzle_award_d  = default"
		", buy_stamina_times_d  = default"
		", trade_draw_times_d  = default"
		", trade_cards = ''"
		", rm_times = default"
		", rm_buy_times  = default"
		", elite_already_done =''"
		",elite_buy_times  = default"
		" ,multi_already_times = ''"
		" ,multi_buy_times = ''"
		" ,speed_times  = default"
		" ,is_got_battle_reward = default"
		" ,escort_times  = default"
		" ,rob_times = default"
		" ,surplus_times = default"
		" ,protect_times  = default"
		" ,offline_buy_times  = default"
		" ,today_attendance = default"
		" ,vip_every_day  = default"
		" ,guild_contribute = default"
		" ,online_box_award = default"
		" where pid in (select pid from tb_player where sid =" << sid() << ")";
	MYSQL_UPDATE(sql.str());

	sql.str("");
	sql << "update tb_player_internal set levy_times  = default"
		<< ", gold_point = default"
		<< ", diggings_times  = default"
		<< ", train_times  = default"
		<< ", guild_times  = default"
		<< " where pid in (select pid from tb_player where sid =" << sid() << ")";
	MYSQL_UPDATE(sql.str());

	sql.str("");
	sql << "update tb_player_hero set today_affair = false where pid in (select pid from tb_player where sid =" << sid() << ")";
	MYSQL_UPDATE(sql.str());

	sql.str("");
	sql << "update tb_player_exercise set contend_times = default"
		", buy_contend  = default"
		", exp_pill_times = default"
		", last_exercise_time = today_exercise_time "
		", last_exercise_exp = today_exercise_exp "
		", today_exercise_time = default"
		", today_exercise_exp = default"
		" where pid in (select pid from tb_player where sid =" << sid() << ")";
	MYSQL_UPDATE(sql.str());
}
#pragma endregion