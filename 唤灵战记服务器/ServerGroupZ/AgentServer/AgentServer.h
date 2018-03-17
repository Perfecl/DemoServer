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
	static const int kAgentServerLoopTime{ 10000 };			//Loopѭ��ʱ��(����)
	static const int kInvalidConnectTimeout{ 15 };			//��Ч���ӱ���ʱ��(��)
	static const int kValidLoginTimeOut{ 15 };				//��¼����ʱ��(��)
	static const int kHeartBeatTime{ 30 };					//������ʱ��(��)

public:
	CAgentServer(int id);
	~CAgentServer();

	void ConnectGameServer();																									//������Ϸ������

	void SendToGameServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid);					//���͸���Ϸ������
	void SendToGameServer(CMessage msg, int pid);																				//���͸���Ϸ������

	void SendToClient(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session);	//���͸��ͻ���
	void SendToClient(CMessage msg, networkz::ISession* session);																//���͸��ͻ���

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
	const AgentServer* const agent_server_info_;										//�����������Ϣ
	const GameServer*  const game_server_info_;											//��Ӧ����Ϸ��������Ϣ

	networkz::ISession*								gameserver_session_{ nullptr };		//��Ϸ�������Ự

	std::map<networkz::ISession*, time_t>			unverified_sessions_;				//δ����֤�ĻỰ
	std::map<int, CanLoginInfo>						can_login_info_;					//�ɵ�½�ĵ�½��Ϣ
	CCriticalSectionZ								lock_;								//�߳���

	std::unordered_map<int, networkz::ISession*>	online_players_;					//�������(pid,�Ự)
	CCriticalSectionZ								player_lock_;						//����߳���

	void	__DoEnterGame(const CMessage& msg);											//��½��Ϸ(C2S)
	void	__OnEnterGame(const CMessage& msg);											//������Ϸ����(S2C)

	void	__ShowServerInfo();															//��ʾ��������Ϣ
	int		__GetCanLoginSID(int pid, const char* ip);									//��ȡ�ɵ�¼SID

	networkz::ISession* __FindOnlinePlayerSession(int pid);								//Ѱ��������һỰ

	void	__Clear();																	//����
};
