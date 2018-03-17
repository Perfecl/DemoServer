#pragma once

class CLoginServer : public CBasicServer
{
public:
	static const int		kLoginTimeout{ 600 };						//��¼��ʱʱ��(��)
	static const int		kInvalidConnectTimeout{ 15 };				//��Ч���ӱ���ʱ��(��)
	static const int		kLoginServerLoopTime{ 10000 };				//Loopѭ��ʱ��(����)

	typedef std::pair<int, std::string> UserLoginInfo;					//(��������,����ʺ�)

public:
	CLoginServer(int id);
	~CLoginServer();

	void ConnectDatabase();																													//�������ݿ�

	void SendToClient(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session);				//������Ϣ���ͻ���
	void SendToClient(CMessage msg, networkz::ISession* session);																			//������Ϣ���ͻ���

	void SendToAgent(const std::string& content, unsigned char protocol_type, int protocol_id, const char* ip, unsigned short port);		//������Ϣ�����������

protected:
	virtual void _OnAccept(networkz::ISession* session, int error_code) override;
	virtual void _OnRecycle(networkz::ISession* session, int error_code) override;
	virtual void _OnClock(int clock_id, const tm& now_datetime) override;

	virtual void _C2A(const CMessage& msg) override;

private:
	const LoginServer* const												login_server_info_;												//��¼��������Ϣ
	std::map<int, std::vector<const AgentServer*>>							agent_servers_;													//sid,rea����Ӧ��AgentServerID

	std::map<networkz::ISession*, time_t>									all_sessions_;													//����sessions
	CCriticalSectionZ														lock_;															//����sessions�߳���

	void __ShowServerInfo();																												//��ʾ��������Ϣ

	void __DoLogin(const CMessage& msg);																									//��½
	void __DoCreate(const CMessage& msg);																									//�������
	void __OnGetName(const CMessage& msg);																									//��ȡ����

	bool __VerifyUsername(const std::string& name);																							//��֤�ʻ���
	bool __VerifyLoginCode(const std::string& username, const std::string& time, int server_id, bool is_adult, const std::string& code);	//��֤��½��

	void __LoginSuccess(int pid, networkz::ISession* session);																				//��½�ɹ�
	int  __CheckPlayerName(const std::string& name, int sid);																				//����������
};

