#include "stdafx.h"
#include "ServerList.h"

const CServerList* CServerList::instance_{ nullptr };

CServerList::CServerList()
{
	__Load();
}

CServerList::~CServerList()
{

}

const CServerList* CServerList::GetInstance()
{
	if (nullptr == instance_)
		instance_ = new CServerList;

	return instance_;
}

const LoginServer* CServerList::GetLoginServerInformation(int id) const
{
	auto it = login_servers_.find(id);
	if (login_servers_.cend() == it)
		return nullptr;
	else
		return &it->second;
}

const AgentServer* CServerList::GetAgentServerInformation(int id) const
{
	auto it = agent_servers_.find(id);
	if (agent_servers_.cend() == it)
		return nullptr;
	else
		return &it->second;
}

const GameServer* CServerList::GetGameServerInformation(int id) const
{
	auto it = game_servers_.find(id);
	if (game_servers_.cend() == it)
		return nullptr;
	else
		return &it->second;
}

const BattleServer* CServerList::GetBattleServerInformation(int id) const
{
	auto it = battle_servers_.find(id);
	if (battle_servers_.cend() == it)
		return nullptr;
	else
		return &it->second;
}

const Database*	CServerList::GetDatabaseInformation(int database_id) const
{
	auto it = databases_.find(database_id);
	if (databases_.cend() == it)
		return nullptr;
	else
		return &it->second;
}

std::vector<int> CServerList::GetLoginServersByHostIP(const char* host_ip) const
{
	return std::move(GetLoginServersByHostID(GetHostID(host_ip)));
}

std::vector<int> CServerList::GetAgentServersByHostIP(const char* host_ip) const
{
	return std::move(GetAgentServersByHostID(GetHostID(host_ip)));
}

std::vector<int> CServerList::GetGameServersByHostIP(const char* host_ip) const
{
	return std::move(GetGameServersByHostID(GetHostID(host_ip)));
}

std::vector<int> CServerList::GetBattleServersByHostIP(const char* host_ip) const
{
	return std::move(GetBattleServersByHostID(GetHostID(host_ip)));
}

std::vector<int> CServerList::GetLoginServersByHostID(int host_id) const
{
	std::vector<int> temp;

	for (auto &it : login_servers_)
	{
		if (host_id == it.second.host_id)
			temp.push_back(it.first);
	}

	return std::move(temp);
}

std::vector<int> CServerList::GetAgentServersByHostID(int host_id) const
{
	std::vector<int> temp;

	for (auto &it : agent_servers_)
	{
		if (host_id == it.second.host_id)
			temp.push_back(it.first);
	}

	return std::move(temp);
}

std::vector<int> CServerList::GetGameServersByHostID(int host_id) const
{
	std::vector<int> temp;

	for (auto &it : game_servers_)
	{
		if (host_id == it.second.host_id)
			temp.push_back(it.first);
	}

	return std::move(temp);
}

std::vector<int> CServerList::GetBattleServersByHostID(int host_id) const
{
	std::vector<int> temp;

	for (auto &it : battle_servers_)
	{
		if (host_id == it.second.host_id)
			temp.push_back(it.first);
	}

	return std::move(temp);
}

std::vector<const GameServer*> CServerList::GetGameServersByPlatform(int platform_id) const
{
	std::vector<const GameServer*> temp;

	for (auto &it : game_servers_)
	{
		if (platform_id == it.second.platform_id)
			temp.push_back(&it.second);
	}

	return std::move(temp);
}

std::vector<const LoginServer*>	CServerList::GetLoginServersByPlatform(int platform_id) const
{
	std::vector<const LoginServer*> temp;

	for (auto &it : login_servers_)
	{
		if (platform_id == it.second.platform_id)
			temp.push_back(&it.second);
	}

	return std::move(temp);
}

std::vector<const AgentServer*>	CServerList::GetAgentServersByGameServerID(int gameserver_id) const
{
	std::vector<const AgentServer*> temp;

	for (auto &it : agent_servers_)
	{
		if (gameserver_id == it.second.game_server_id)
			temp.push_back(&it.second);
	}

	return std::move(temp);
}

const char*	CServerList::GetHostIP(int host_id) const
{
	auto it = host_list_.find(host_id);

	if (host_list_.cend() == it)
		return nullptr;
	else
		return it->second.c_str();
}

int	CServerList::GetHostID(const char* host_ip) const
{
	for (auto &it : host_list_)
	{
		if (0 == strcmp(it.second.c_str(), host_ip))
			return it.first;
	}

	return 0;
}

void CServerList::__Load()
{
	dat_SERVER_Config server_list;

	server_list.ParseFromString(GetDataFromFile(GAME_DATA_PATH"ServerConfig.txt"));

	for (int i = 0; i < server_list.login_servers_size(); i++)
	{
		LoginServer info;
		auto it = server_list.login_servers(i);
		info.id = it.id();
		info.host_id = it.host_id();
		info.tcp_port = it.tcp_port();
		info.udp_port = it.udp_port();
		info.database_id = it.database();
		info.platform_id = it.platform();
		login_servers_[info.id] = std::move(info);
	}

	for (int i = 0; i < server_list.agent_servers_size(); i++)
	{
		AgentServer info;
		auto it = server_list.agent_servers(i);
		info.id = it.id();
		info.host_id = it.host_id();
		info.tcp_port = it.tcp_port();
		info.udp_port = it.udp_port();
		info.game_server_id = it.game_server();
		info.platform_id = it.platform();
		agent_servers_[info.id] = std::move(info);
	}

	for (int i = 0; i < server_list.game_servers_size(); i++)
	{
		GameServer info;
		auto it = server_list.game_servers(i);
		info.id = it.id();
		info.host_id = it.host_id();
		info.tcp_port = it.tcp_port();
		info.database_id = it.database();
		info.battle_server_id = it.battle_server();
		for (int i = 0; i < it.area_size(); i++)
			info.area_id.push_back(it.area(i));
		info.platform_id = it.platform();
		game_servers_[info.id] = std::move(info);
	}

	for (int i = 0; i < server_list.battle_servers_size(); i++)
	{
		BattleServer info;
		auto it = server_list.battle_servers(i);
		info.id = it.id();
		info.host_id = it.host_id();
		info.tcp_port = it.tcp_port();
		info.platform_id = it.platform();
		battle_servers_[info.id] = std::move(info);
	}

	for (int i = 0; i < server_list.databases_size(); i++)
	{
		Database info;
		auto it = server_list.databases(i);
		info.id = it.id();
		info.host_id = it.host_id();
		info.port = it.port();
		info.username = it.username();
		info.password = it.password();
		info.database_name = it.database_name();
		databases_[info.id] = std::move(info);
	}

	for (int i = 0; i < server_list.hosts_size(); i++)
	{
		auto it = server_list.hosts(i);
		host_list_[it.id()] = it.ip();
	}

	for (int i = 0; i < server_list.platforms_size(); i++)
	{
		auto it = server_list.platforms(i);
		platform_key_.insert(std::make_pair(it.id(), it.key()));
	}
}
