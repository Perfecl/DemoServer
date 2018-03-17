#pragma once

#define SERVER_LIST()  CServerList::GetInstance()

struct LoginServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//主机ID
	unsigned short		tcp_port{ 0 };			//tcp端口
	unsigned short		udp_port{ 0 };			//udp端口
	int					database_id{ 0 };		//数据库ID
	int					platform_id{ 0 };		//平台ID
};

struct AgentServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//主机ID
	unsigned short		tcp_port{ 0 };			//tcp端口
	unsigned short		udp_port{ 0 };			//upd端口
	int					game_server_id{ 0 };	//游戏服务器ID
	int					platform_id{ 0 };		//平台ID
};

struct GameServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//主机ID
	unsigned short		tcp_port{ 0 };			//tcp端口
	std::vector<int>	area_id;				//区号
	int					database_id{ 0 };		//数据库ID
	int					battle_server_id{ 0 };	//跨服战场ID
	int					platform_id{ 0 };		//平台ID
};

struct BattleServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//主机ID
	unsigned short		tcp_port{ 0 };			//tcp端口
	int					platform_id{ 0 };		//平台ID
};

struct Database
{
	int					id{ 0 };
	int					host_id{ 0 };
	unsigned short		port{ 0 };
	std::string			username;
	std::string			password;
	std::string			database_name;
};

class CServerList
{
public:
	static const CServerList* GetInstance();

private:
	static const CServerList* instance_;

public:
	~CServerList();

	//获取服务器信息
	const LoginServer*	GetLoginServerInformation(int login_id) const;
	const AgentServer*	GetAgentServerInformation(int agent_id) const;
	const GameServer*	GetGameServerInformation(int game_id) const;
	const BattleServer* GetBattleServerInformation(int battle_id) const;
	const Database*		GetDatabaseInformation(int database_id) const;

	//获取服务器ID
	std::vector<int>	GetLoginServersByHostID(int host_id) const;
	std::vector<int>	GetAgentServersByHostID(int host_id) const;
	std::vector<int>	GetGameServersByHostID(int host_id) const;
	std::vector<int>	GetBattleServersByHostID(int host_id) const;

	//获取服务器ID
	std::vector<int>	GetLoginServersByHostIP(const char* host_ip) const;
	std::vector<int>	GetAgentServersByHostIP(const char* host_ip) const;
	std::vector<int>	GetGameServersByHostIP(const char* host_ip) const;
	std::vector<int>	GetBattleServersByHostIP(const char* host_ip) const;

	std::vector<const GameServer*>		GetGameServersByPlatform(int platform_id) const;			//用平台ID获得所有的GameServer
	std::vector<const LoginServer*>		GetLoginServersByPlatform(int platform_id) const;			//用平台ID获得所有的LoginServer

	std::vector<const AgentServer*>		GetAgentServersByGameServerID(int gameserver_id) const;		//用GameServerID获得所有的AgentServer

	const char*			GetHostIP(int host_id) const;								//获取主机IP
	int					GetHostID(const char* host_ip) const;						//获取主机ID

private:
	std::map<int, LoginServer>		login_servers_;									//登陆服务器
	std::map<int, AgentServer>		agent_servers_;									//代理服务器
	std::map<int, GameServer>		game_servers_;									//游戏服务器
	std::map<int, BattleServer>		battle_servers_;								//战斗服务器
	std::map<int, Database>			databases_;										//数据库

	std::map <int, std::string>		host_list_;										//主机列表

	std::map<int, std::string>		platform_key_;

	CServerList();

	void __Load();
};

