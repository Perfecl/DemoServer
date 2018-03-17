#pragma once

#define SERVER_LIST()  CServerList::GetInstance()

struct LoginServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//����ID
	unsigned short		tcp_port{ 0 };			//tcp�˿�
	unsigned short		udp_port{ 0 };			//udp�˿�
	int					database_id{ 0 };		//���ݿ�ID
	int					platform_id{ 0 };		//ƽ̨ID
};

struct AgentServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//����ID
	unsigned short		tcp_port{ 0 };			//tcp�˿�
	unsigned short		udp_port{ 0 };			//upd�˿�
	int					game_server_id{ 0 };	//��Ϸ������ID
	int					platform_id{ 0 };		//ƽ̨ID
};

struct GameServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//����ID
	unsigned short		tcp_port{ 0 };			//tcp�˿�
	std::vector<int>	area_id;				//����
	int					database_id{ 0 };		//���ݿ�ID
	int					battle_server_id{ 0 };	//���ս��ID
	int					platform_id{ 0 };		//ƽ̨ID
};

struct BattleServer
{
	int					id{ 0 };				//ID
	int					host_id{ 0 };			//����ID
	unsigned short		tcp_port{ 0 };			//tcp�˿�
	int					platform_id{ 0 };		//ƽ̨ID
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

	//��ȡ��������Ϣ
	const LoginServer*	GetLoginServerInformation(int login_id) const;
	const AgentServer*	GetAgentServerInformation(int agent_id) const;
	const GameServer*	GetGameServerInformation(int game_id) const;
	const BattleServer* GetBattleServerInformation(int battle_id) const;
	const Database*		GetDatabaseInformation(int database_id) const;

	//��ȡ������ID
	std::vector<int>	GetLoginServersByHostID(int host_id) const;
	std::vector<int>	GetAgentServersByHostID(int host_id) const;
	std::vector<int>	GetGameServersByHostID(int host_id) const;
	std::vector<int>	GetBattleServersByHostID(int host_id) const;

	//��ȡ������ID
	std::vector<int>	GetLoginServersByHostIP(const char* host_ip) const;
	std::vector<int>	GetAgentServersByHostIP(const char* host_ip) const;
	std::vector<int>	GetGameServersByHostIP(const char* host_ip) const;
	std::vector<int>	GetBattleServersByHostIP(const char* host_ip) const;

	std::vector<const GameServer*>		GetGameServersByPlatform(int platform_id) const;			//��ƽ̨ID������е�GameServer
	std::vector<const LoginServer*>		GetLoginServersByPlatform(int platform_id) const;			//��ƽ̨ID������е�LoginServer

	std::vector<const AgentServer*>		GetAgentServersByGameServerID(int gameserver_id) const;		//��GameServerID������е�AgentServer

	const char*			GetHostIP(int host_id) const;								//��ȡ����IP
	int					GetHostID(const char* host_ip) const;						//��ȡ����ID

private:
	std::map<int, LoginServer>		login_servers_;									//��½������
	std::map<int, AgentServer>		agent_servers_;									//���������
	std::map<int, GameServer>		game_servers_;									//��Ϸ������
	std::map<int, BattleServer>		battle_servers_;								//ս��������
	std::map<int, Database>			databases_;										//���ݿ�

	std::map <int, std::string>		host_list_;										//�����б�

	std::map<int, std::string>		platform_key_;

	CServerList();

	void __Load();
};

