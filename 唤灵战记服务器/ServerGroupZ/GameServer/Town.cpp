#include "stdafx.h"
#include "Town.h"
#include "Player.h"
#include "Mail.h"

CTown::CTown(int id) :
town_id_{ id }
{
	__LoadFromDatabase();
}

CTown::~CTown()
{

}

ENUM_EnterTownResult CTown::EnterTown(SPPlayer player)
{
	if (!player)
	{
		RECORD_ERROR("进入城镇玩家空指针");
		return ETR_FAIL;
	}

	if (player->main_progress() < main_progress_)
		return ETR_MISSION_UNFINISHED;

	LOCK_BLOCK(lock_);

	if (players_in_town_.insert(std::make_pair(player->pid(), player)).second)
	{
		if (player->town_id() != town_id_)
		{
			player->SetTownID(town_id_);
			player->SetTownPosition(100, 100);
		}

		pto_TOWN_S2C_NTF_SingleInfo pto;
		auto pto_player = pto.mutable_player();
		pto_player->set_pid(player->pid());
		pto_player->set_x(player->town_x());
		pto_player->set_y(player->town_y());
		pto_player->set_lv(player->level());
		pto_player->set_moduleid(player->clothes());
		pto_player->set_titleid(player->using_title());
		pto_player->set_petid(player->pet_id());
		pto_player->set_petlv(player->pet_level());
		std::string str;
		pto.SerializeToString(&str);
		SendAllPlayersInTown(str, MSG_S2C, TOWN_S2C_NTF_SingleInfo, player->pid());

		return ETR_SUCCESS;
	}
	else
	{
		return ETR_ALREADY_IN_TOWN;
	}
}

void CTown::LeaveTown(int pid)
{
	LOCK_BLOCK(lock_);

	if (players_in_town_.erase(pid))
	{
		pto_TOWN_S2C_NTF_Leave pto;
		pto.set_pid(pid);
		std::string str;
		pto.SerializeToString(&str);
		SendAllPlayersInTown(str, MSG_S2C, TOWN_S2C_NTF_Leave, pid);
	}
}

void CTown::TownMove(int pid, int x, int y)
{
	SPPlayer player = FindPlayerInTown(pid);

	if (player)
	{
		player->SetTownPosition(x, y);
		pto_TOWN_S2C_NTF_Move pto;
		pto.set_pid(pid);
		pto.set_x(player->town_x());
		pto.set_y(player->town_y());
		std::string str;
		pto.SerializeToString(&str);
		SendAllPlayersInTown(str, MSG_S2C, TOWN_S2C_NTF_Move, pid);
	}
	else
	{
		RECORD_TRACE(FormatString("玩家城镇移动错误,pid:", pid));
	}
}

SPPlayer CTown::FindPlayerInTown(int pid)
{
	LOCK_BLOCK(lock_);

	auto it = players_in_town_.find(pid);
	if (players_in_town_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CTown::SendAllPlayersInTown(const std::string& content, unsigned char protocol_type, int protocol_id, int except_pid)
{
	std::map<networkz::ISession*, std::vector<int>>  sessions;

	lock_.lock();
	for (auto &it_player : players_in_town_)
	{
		if (it_player.second->pid() != except_pid)
		{
			auto it = sessions.find(it_player.second->session());
			if (sessions.cend() == it)
				sessions.insert(std::make_pair(it_player.second->session(), std::move(std::vector < int > { it_player.first })));
			else
				it->second.push_back(it_player.first);
		}
	}
	lock_.unlock();

	CMessage msg{ content, protocol_type, protocol_id };
	std::string str;
	for (auto &it : sessions)
	{
		pto_SYSTEM_S2A_NTF_Transpond pto;
		for (auto &it_pid : it.second)
			pto.add_pid(it_pid);
		pto.set_message(msg, msg.length());
		pto.SerializeToString(&str);
		GAME_WORLD()->SendToAgentServer(str, MSG_S2A, SYSTEM_S2A_NTF_Transpond, 0, it.first);
	}
}

void CTown::SetAllPlayersToProtocol(pto_TOWN_S2C_RES_EnterTown& pto)
{
	LOCK_BLOCK(lock_);

	for (auto &it : players_in_town_)
	{
		auto player = pto.add_players();
		player->set_pid(it.second->pid());
		player->set_x(it.second->town_x());
		player->set_y(it.second->town_y());
		player->set_lv(it.second->level());
		player->set_moduleid(it.second->clothes());
		player->set_titleid(it.second->using_title());
		player->set_petid(it.second->pet_id());
		player->set_petlv(it.second->pet_level());
	}
}

void CTown::SetTop10Protocol(pto_PLAYER_S2C_RES_UpdateTownStageRank* pto)
{
	if (!pto)
		return;

	pto->set_town_id(town_id());

	LOCK_BLOCK(lock_);
	for (auto &it : pass_ranking_list_)
	{
		auto pto_player = pto->add_town_stage_rank();
		pto_player->set_pid(it.first);
		pto_player->set_time(it.second);

		SPIPlayerData data = PLAYER_DATA(it.first);
		if (!data)
			return;
		pto_player->set_name(data->name());
	}
}

bool CTown::AddTop10Player(int pid)
{
	if (pass_ranking_list_.size() >= 10)
		return false;

	{
		LOCK_BLOCK(lock_);

		for (auto &it : pass_ranking_list_)
			if (it.first == pid)
				return false;

		pass_ranking_list_.push_back(std::make_pair(pid, time(0)));

		std::ostringstream sql;
		sql << "insert into tb_town_ranking values(" << town_id_ << "," << GAME_WORLD()->sid() << "," << pid << ",default)";
		MYSQL_UPDATE(sql.str());
	}

	SPPlayer player{ GAME_WORLD()->FindPlayer(pid) };

	if (player)
	{
		GAME_WORLD()->Annunciate(player->name(), town_id_, pass_ranking_list_.size(), AnnunciateType::TownStageRank);

		switch (town_id_)
		{
		case 1:
			player->GiveNewTitle(4);
			break;
		case 2:
			player->GiveNewTitle(5);
			break;
		case 3:
			player->GiveNewTitle(6);
			break;
		case 4:
			player->GiveNewTitle(7);
			break;
		case 5:
			player->GiveNewTitle(16);
			break;
		}
	}

	const CTownRankBox* town_reward{ CTownRankBox::GetTownRankBox(town_id()) };

	if (town_reward)
	{
		SPMail mail = std::make_shared<CMail>();
		mail->SetModelID(MMI_TownStageRank);
		mail->AddRewardItem(LT_Silver, town_reward->m_nSilver);
		mail->AddRewardItem(LT_Gold, town_reward->m_nGold);
		mail->AddRewardItem(LT_Honour, town_reward->m_nHonour);
		mail->AddRewardItem(LT_Stamina, town_reward->m_nStamina);
		mail->SetParam0(pass_ranking_list_.size());
		GAME_WORLD()->GiveNewMail(mail, pid);
	}

	return true;
}

void CTown::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select pid, unix_timestamp(pass_time) from tb_town_ranking where sid = " << GAME_WORLD()->sid() << " and town_id = " << town_id_ << " order by pass_time asc limit 0, 10";

	ResultSetPtr result{ MYSQL_QUERY(sql.str()) };
	if (!result)
		return;

	while (result->next())
		pass_ranking_list_.push_back(std::make_pair(result->getInt(1), result->getInt64(2)));
}
