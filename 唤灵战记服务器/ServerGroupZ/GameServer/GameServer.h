#pragma once

class CGameServer : public CBasicServer
{
public:
	CGameServer(int id);
	~CGameServer();

	bool ConnectDatabase();																														//连接数据库

	void ConnectBattleServer();																													//连接BattleServe

	void CreateGameWorld();																														//创建游戏世界

	void VerifySecurityKeyFile();																												//验证安全密钥文件

	void SendToAgentServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, networkz::ISession* session);		//发送消息给代理服务器
	void SendToAgentServer(CMessage msg, int pid, networkz::ISession* session);																	//发送消息给代理服务器

	void SendToBattleServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid);									//发送消息给战斗服务器
	void SendToBattleServer(CMessage msg, int pid);																								//发送消息给战斗服务器

	inline int GetSID(){ return game_server_info_->area_id[0]; }																				//获取服务器号

	inline networkz::ISession*	battle_session(){ return battle_server_session_; }

protected:
	virtual void _OnProcessMessage(const CMessage& msg) override;
	virtual void _OnClock(int clock_id, const tm& now_datetime) override;
	virtual void _OnConnect(networkz::ISession* session, int error_code) override;
	virtual void _OnRecycle(networkz::ISession* session, int error_code) override;

private:
	const GameServer* const			game_server_info_;								//游戏服务器信息
	const BattleServer*	const		battle_server_info_;							//战斗服务器信息

	networkz::ISession*				battle_server_session_{ nullptr };				//战斗服务器指针

	void __ShowServerInfo();														//显示服务器信息
};
