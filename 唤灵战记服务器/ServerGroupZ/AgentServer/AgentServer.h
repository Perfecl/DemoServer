#pragma once

class CAgentServer : public CBasicServer
{
	struct CanLoginInfo
	{
		int		sid;
		char	ip[16];
		time_t	time;
	};

	typedef std::pair<int, time_t> SessionInfo;

public:
	static const int kAgentServerLoopTime{ 10000 };			//Loop循环时间(毫秒)
	static const int kInvalidConnectTimeout{ 15 };			//无效连接保留时间(秒)
	static const int kValidLoginTimeOut{ 15 };				//登录保留时间(秒)
	static const int kHeartBeatTime{ 30 };					//心跳包时间(秒)

public:
	CAgentServer(int id);
	~CAgentServer();

	void ConnectGameServer();																									//连接游戏服务器

	void SendToGameServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid);					//发送给游戏服务器
	void SendToGameServer(CMessage msg, int pid);																				//发送给游戏服务器

	void SendToClient(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session);	//发送给客户端
	void SendToClient(CMessage msg, networkz::ISession* session);																//发送给客户端

protected:
	virtual void _OnConnect(networkz::ISession* session, int error_code) override;
	virtual void _OnAccept(networkz::ISession* session, int error_code) override;
	virtual void _OnRecycle(networkz::ISession* session, int error_code) override;
	virtual void _OnUDPReceive(const char* data, size_t length, const char* remote_ip, int error_code) override;
	virtual void _OnClock(int clock_id, const tm& now_datetime) override;

	virtual void _C2S(const CMessage& msg) override;
	virtual void _S2C(const CMessage& msg) override;
	virtual void _C2A(const CMessage& msg) override;
	virtual void _S2A(const CMessage& msg) override;

private:
	const AgentServer* const agent_server_info_;										//代理服务器信息
	const GameServer*  const game_server_info_;											//对应的游戏服务器信息

	networkz::ISession*								gameserver_session_{ nullptr };		//游戏服务器会话

	std::map<networkz::ISession*, time_t>			unverified_sessions_;				//未经验证的会话
	std::map<int, CanLoginInfo>						can_login_info_;					//可登陆的登陆信息
	CCriticalSectionZ								lock_;								//线程锁

	std::unordered_map<int, networkz::ISession*>	online_players_;					//在线玩家(pid,会话)
	CCriticalSectionZ								player_lock_;						//玩家线程锁

	void	__DoEnterGame(const CMessage& msg);											//登陆游戏(C2S)
	void	__OnEnterGame(const CMessage& msg);											//登入游戏返回(S2C)

	void	__ShowServerInfo();															//显示服务器信息
	int		__GetCanLoginSID(int pid, const char* ip);									//获取可登录SID

	networkz::ISession* __FindOnlinePlayerSession(int pid);								//寻找在线玩家会话

	void	__Clear();																	//清理
};
