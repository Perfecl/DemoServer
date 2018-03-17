#include "stdafx.h"
#include "Guild.h"
#include "Player.h"
#include <regex>
#include "Mission.h"

std::vector<size_t> CGuild::member_num_max{ 16, 16, 18, 20, 22, 24, 26, 28, 30, 32, 35 };

CGuild::CGuild(int guild_id) :
guild_id_{ guild_id }
{
	__LoadFromDatabase();
}

CGuild::~CGuild()
{

}

void CGuild::SaveToDatabase()
{
	LOCK_BLOCK(lock_);

	CSerializer<int> ser;
	for (auto &it : apply_list_)
		ser.AddValue(it);

	std::ostringstream sql;
	sql << "update tb_guild set level = " << level_ << ",exp = " << exp_ << ",fund = " << fund_ << ",notice = '" << notice_ << "',apply_list = '" << ser.SerializerToString().c_str() << "',is_dissolve = " << is_dissolve_ << " where guild_id = " << guild_id_ << " and sid = " << GAME_WORLD()->sid();
	MYSQL_UPDATE(sql.str());

	sql.str("");
	sql << "delete from tb_guild_member where guild_id = " << guild_id_;
	MYSQL_UPDATE(sql.str());

	if (!member_.empty())
	{
		sql.str("");
		sql << "insert into tb_guild_member values";
		for (auto &it : member_)
			sql << "(" << guild_id_ << ", " << it.first << ", " << (int)it.second.first << ", " << it.second.second << "),";
		std::string str_sql = std::move(sql.str());
		str_sql.erase(str_sql.length() - 1, 1);
		MYSQL_UPDATE(str_sql);
	}
}

void CGuild::SendToAllPlayers(const std::string& content, unsigned char protocol_type, int protocol_id, GuildPosition position_least, int except_pid)
{
	std::map<networkz::ISession*, std::vector<int>>  sessions;

	LOCK_BLOCK(lock_);

	for (auto &it : member_)
	{
		if (except_pid == it.first || it.second.first < position_least)
			continue;

		SPPlayer player = FIND_PLAYER(it.first);

		if (player)
		{
			auto it = sessions.find(player->session());
			if (sessions.cend() == it)
				sessions.insert(std::make_pair(player->session(), std::move(std::vector < int > { player->pid() })));
			else
				it->second.push_back(player->pid());
		}
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
		GAME_WORLD()->SendToAgentServer(str, MSG_S2A, SYSTEM_S2A_NTF_Transpond, 0, it.first);
	}
}

bool CGuild::CheckGuildName(const std::string& str)
{
	int length = GetNameStandardLength(str);

	if (length > 16 || length < 2)
		return false;

	if (CIllegalWords::HasIllegalWords(str))
		return false;

	return true;
}

GuildPosition CGuild::GetPosition(int pid)
{
	LOCK_BLOCK(lock_);

	auto it = member_.find(pid);
	if (member_.cend() == it)
		return GuildPosition::NOT_MEMBER;
	else
		return it->second.first;
}

int	CGuild::GetPositionNum(GuildPosition position)
{
	int num{ 0 };

	LOCK_BLOCK(lock_);

	for (auto &it : member_)
	{
		if (position == it.second.first)
			num++;
	}

	return num;
}

int CGuild::EditNotice(int pid, const std::string& str)
{
	if (GetPosition(pid) < GuildPosition::VICE_CHAIRMAN)
		return 3;

	int length = GetNameStandardLength(str);

	if (length > 128 || length < 0)
		return 1;

	if (CIllegalWords::HasIllegalWords(str))
	{
		return 2;
	}
	else
	{
		notice_ = str;
		return 0;
	}
}

bool CGuild::IsInApplyList(int pid)
{
	LOCK_BLOCK(lock_);
	return apply_list_.find(pid) != apply_list_.cend();
}

void CGuild::ApplyForJoin(int pid)
{
	LOCK_BLOCK(lock_);

	if (member_.size() >= member_num_max[level_])
		return;

	if (!IsInApplyList(pid))
	{
		apply_list_.insert(pid);

		pto_GUILD_S2C_NTF_ApplyGuild pto;
		pto.set_pid(pid);
		std::string str;
		pto.SerializeToString(&str);
		SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_ApplyGuild, GuildPosition::OFFICER);
	}
}

void CGuild::AgreeApply(int pid, int target_pid, bool is_agree)
{
	if (GetPosition(pid) < GuildPosition::OFFICER)
	{
		RECORD_WARNING("没有权限同意申请");
		return;
	}

	if (IsInApplyList(target_pid))
	{
		LOCK_BLOCK(lock_);

		SPIPlayerData data = PLAYER_DATA(target_pid);
		if (!data)
			return;

		if (data->guild_id())
			is_agree = false;

		apply_list_.erase(target_pid);

		if (is_agree)
		{
			if (member_.size() >= member_num_max[level_])
				return;


			member_.insert(std::make_pair(target_pid, std::make_pair(GuildPosition::RANK_AND_FILE, 0)));
			data->guild_id(guild_id_);
		}

		pto_GUILD_S2C_NTF_JoinGuild pto;
		pto.set_pid(target_pid);
		pto.set_is_agree(is_agree);
		pto.set_name(data->name());
		std::string str;
		pto.SerializeToString(&str);
		SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_JoinGuild, GuildPosition::RANK_AND_FILE, target_pid);

		SPPlayer player = FIND_PLAYER(target_pid);
		if (player)
		{
			pto_GUILD_S2C_RES_ApplyGuild pto;
			pto.set_guild_id(player->guild_id());
			std::string str;
			pto.SerializeToString(&str);
			player->SendToAgent(pto, GUILD_S2C_RES_ApplyGuild);
			if (player->guild_id())
			{
				player->mission()->ImplementMission(EMissionTargetType::MTT_CreateOrJoinGuild, 0, 0);
			}
		}
	}
	else
	{
		RECORD_TRACE("玩家不再申请名单中");
	}
}

int	CGuild::ChangeAccess(int pid, int target_pid, bool is_up)
{
	GuildPosition src_position = GetPosition(pid);

	LOCK_BLOCK(lock_);

	auto it = member_.find(target_pid);
	if (member_.cend() == it)
		return 2;

	if (src_position <= it->second.first)
		return 4;

	if (is_up)
	{
		if (src_position <= static_cast<GuildPosition>((int)it->second.first + 1))
			return 4;

		if (GuildPosition::OFFICER == it->second.first && GetPositionNum(GuildPosition::VICE_CHAIRMAN) >= 2)
			return 1;

		it->second.first = static_cast<GuildPosition>((int)it->second.first + 1);
	}
	else
	{
		if (it->second.first <= GuildPosition::RANK_AND_FILE)
			return 4;

		it->second.first = static_cast<GuildPosition>((int)it->second.first - 1);
	}

	pto_GUILD_S2C_NTF_ChangeAccess pto;

	SPIPlayerData player = PLAYER_DATA(target_pid);
	if (!player)
		return 2;
	SPIPlayerData src_player = PLAYER_DATA(pid);
	if (!src_player)
		return 2;

	pto.set_pid(player->pid());
	pto.set_name(player->name());
	pto.set_src_name(src_player->name());
	pto.set_position((int)it->second.first);
	pto.set_is_up(is_up);
	std::string str;
	pto.SerializeToString(&str);
	SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_ChangeAccess, GuildPosition::RANK_AND_FILE);

	return 0;
}

bool CGuild::QuitGuild(int pid)
{
	LOCK_BLOCK(lock_);

	if (pid == chairman_id_)
	{
		is_dissolve_ = true;

		for (auto &it : member_)
		{
			SPIPlayerData player = PLAYER_DATA(it.first);
			if (player)
				player->guild_id(0);
		}

		SPIPlayerData chair_player = PLAYER_DATA(pid);
		if (!chair_player)
			return false;

		pto_GUILD_S2C_NTF_QuitGuild pto;
		pto.set_pid(-1);
		pto.set_name(chair_player->name());
		std::string str;
		pto.SerializeToString(&str);
		SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_QuitGuild, GuildPosition::RANK_AND_FILE);

		return true;
	}

	if (member_.find(pid) != member_.cend())
	{
		pto_GUILD_S2C_NTF_QuitGuild pto;
		pto.set_pid(pid);
		SPIPlayerData player = PLAYER_DATA(pid);
		if (!player)
			return false;

		pto.set_name(player->name());
		std::string str;
		pto.SerializeToString(&str);
		SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_QuitGuild, GuildPosition::RANK_AND_FILE);

		member_.erase(pid);

		return true;
	}

	return false;
}

void CGuild::Kick(int src_pid, int target_pid)
{
	if (GetPosition(src_pid) <= GetPosition(target_pid))
		return;

	if (member_.find(target_pid) != member_.cend())
	{
		pto_GUILD_S2C_NTF_QuitGuild pto;

		SPIPlayerData player = PLAYER_DATA(target_pid);
		if (!player)
			return;

		player->guild_id(0);

		SPIPlayerData src_player = PLAYER_DATA(src_pid);
		if (!src_player)
			return;

		pto.set_pid(target_pid);
		pto.set_name(player->name());
		pto.set_src_pid(src_pid);
		pto.set_src_name(src_player->name());
		std::string str;
		pto.SerializeToString(&str);
		SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_QuitGuild, GuildPosition::RANK_AND_FILE);

		member_.erase(target_pid);
	}
}

void CGuild::Cession(int pid, int target_pid)
{
	LOCK_BLOCK(lock_);

	auto it_chairman = member_.find(pid);
	if (member_.cend() == it_chairman)
		return;

	if (it_chairman->second.first != GuildPosition::CHAIRMAN)
		return;

	auto it_target = member_.find(target_pid);
	if (member_.cend() == it_target)
		return;

	it_chairman->second.first = GuildPosition::OFFICER;
	it_target->second.first = GuildPosition::CHAIRMAN;
	chairman_id_ = target_pid;

	SPIPlayerData src_player = PLAYER_DATA(pid);
	if (!src_player)
		return;
	SPIPlayerData player = PLAYER_DATA(target_pid);
	if (!player)
		return;

	pto_GUILD_S2C_NTF_Cession pto;
	pto.set_old_pid(pid);
	pto.set_old_name(src_player->name());
	pto.set_new_pid(target_pid);
	pto.set_new_name(player->name());
	std::string str;
	pto.SerializeToString(&str);
	SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_Cession, GuildPosition::RANK_AND_FILE);
}

void CGuild::Contribute(int pid, int contribute, int fund)
{
	LOCK_BLOCK(lock_);

	auto it = member_.find(pid);

	if (member_.cend() != it)
	{
		it->second.second += contribute;
		fund_ += fund;

		ChangeExp(5);

		SPIPlayerData player = PLAYER_DATA(pid);
		if (!player)
			return;
		SPPlayer ptPlayer = FIND_PLAYER(pid);
		if (nullptr != ptPlayer)
			ptPlayer->ChangeReputation(10);

		pto_GUILD_S2C_NTF_Contribute pto;
		pto.set_name(player->name());
		pto.set_guild_fund(fund_);
		pto.set_contribute(it->second.second);
		pto.set_pid(pid);
		pto.set_exp(exp_);
		std::string str;
		pto.SerializeToString(&str);
		SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_Contribute, GuildPosition::RANK_AND_FILE);
	}
}

void CGuild::SetProtocol(dat_STRUCT_GUILD* pto, int pid)
{
	if (!pto)
		return;

	pto->set_guild_id(guild_id_);
	pto->set_name(name_);
	pto->set_level(level_);
	pto->set_exp(exp_);
	pto->set_fund(fund_);
	pto->set_notice(notice_);
	pto->set_ranking(GAME_WORLD()->GetGuildRanking(guild_id_));

	for (auto &it : member_)
	{
		auto it_member = pto->add_members();
		it_member->set_pid(it.first);
		it_member->set_position(static_cast<int>(it.second.first));
		it_member->set_contribution(it.second.second);
	}

	if (GetPosition(pid) >= GuildPosition::OFFICER)
	{
		for (auto &it : apply_list_)
			pto->add_apply_member(it);
	}
}

void CGuild::ChangeExp(int num)
{
	exp_ += num;
	bool level_up{ true };
	while (level_up)
	{
		SPGuildLibrary guild_library = CGuildLibrary::GetGuildLibrary(level_ + 1);
		if (!guild_library)
		{
			level_up = false;
		}
		else if (exp_ >= guild_library->exp())
		{
			level_++;
			exp_ = exp_ - guild_library->exp();
			pto_GUILD_S2C_NTF_LevelUp pto_ntf;
			pto_ntf.set_level(level_);
			pto_ntf.set_exp(exp_);
			std::string str;
			pto_ntf.SerializeToString(&str);
			SendToAllPlayers(str, MSG_S2C, GUILD_S2C_NTF_LevelUp, GuildPosition::RANK_AND_FILE);
		}
		else
		{
			level_up = false;
		}
	}
}

void CGuild::__LoadFromDatabase()
{
	std::ostringstream sql;
	sql << "select name,level,exp,fund,notice,apply_list from tb_guild where guild_id = " << guild_id_;
	ResultSetPtr result_guild = MYSQL_QUERY(sql.str());
	if (result_guild && result_guild->next())
	{
		name_ = result_guild->getString("name").c_str();
		level_ = result_guild->getInt("level");
		exp_ = result_guild->getInt("exp");
		fund_ = result_guild->getInt("fund");
		notice_ = result_guild->getString("notice").c_str();

		auto applys = CSerializer<int>::ParseFromString(result_guild->getString("apply_list").c_str());
		for (auto &it : applys)
			apply_list_.insert(it);

		sql.str("");
		sql << "select pid,position,contribution from tb_guild_member where guild_id = " << guild_id_;
		ResultSetPtr result_members = MYSQL_QUERY(sql.str());
		if (!result_members)
			return;

		while (result_members->next())
		{
			int pid = result_members->getInt("pid");

			GuildPosition position = static_cast<GuildPosition>(result_members->getInt("position"));

			if (GuildPosition::CHAIRMAN == position)
				chairman_id_ = pid;

			member_[pid] = std::make_pair(position, result_members->getInt("contribution"));
		}
	}
	else
	{
		throw std::exception(FormatString("查找公会失败,guild_id", guild_id_).c_str());
	}
}
