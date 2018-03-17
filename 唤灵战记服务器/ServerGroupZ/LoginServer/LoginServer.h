#pragma once

class CLoginServer : public CBasicServer
{
public:
	static const int		kLoginTimeout{ 600 };						//登录超时时间(秒)
	static const int		kInvalidConnectTimeout{ 15 };				//无效连接保留时间(秒)
	static const int		kLoginServerLoopTime{ 10000 };				//Loop循环时间(毫秒)

	typedef std::pair<int, std::string> UserLoginInfo;					//(服务器号,玩家帐号)

public:
	CLoginServer(int id);
	~CLoginServer();

	void ConnectDatabase();																													//连接数据库

	void SendToClient(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session);				//发送消息给客户端
	void SendToClient(CMessage msg, networkz::ISession* session);																			//发送消息给客户端

	void SendToAgent(const std::string& content, unsigned char protocol_type, int protocol_id, const char* ip, unsigned short port);		//发送消息给代理服务器

protected:
	virtual void _OnAccept(networkz::ISession* session, int error_code) override;
	virtual void _OnRecycle(networkz::ISession* session, int error_code) override;
	virtual void _OnClock(int clock_id, const tm& now_datetime) override;

	virtual void _C2A(const CMessage& msg) override;

private:
	const LoginServer* const												login_server_info_;												//登录服务器信息
	std::map<int, std::vector<const AgentServer*>>							agent_servers_;													//sid,rea所对应的AgentServerID

	std::map<networkz::ISession*, time_t>									all_sessions_;													//连接sessions
	CCriticalSectionZ														lock_;															//连接sessions线程锁

	void __ShowServerInfo();																												//显示服务器信息

	void __DoLogin(const CMessage& msg);																									//登陆
	void __DoCreate(const CMessage& msg);																									//创建玩家
	void __OnGetName(const CMessage& msg);																									//获取名字

	bool __VerifyUsername(const std::string& name);																							//验证帐户名
	bool __VerifyLoginCode(const std::string& username, const std::string& time, int server_id, bool is_adult, const std::string& code);	//验证登陆码

	void __LoginSuccess(int pid, networkz::ISession* session);																				//登陆成功
	int  __CheckPlayerName(const std::string& name, int sid);																				//检查玩家姓名
};

