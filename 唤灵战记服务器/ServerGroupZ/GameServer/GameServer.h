#pragma once

class CGameServer : public CBasicServer
{
public:
	CGameServer(int id);
	~CGameServer();

	bool ConnectDatabase();																														//�������ݿ�

	void ConnectBattleServer();																													//����BattleServe

	void CreateGameWorld();																														//������Ϸ����

	void VerifySecurityKeyFile();																												//��֤��ȫ��Կ�ļ�

	void SendToAgentServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, networkz::ISession* session);		//������Ϣ�����������
	void SendToAgentServer(CMessage msg, int pid, networkz::ISession* session);																	//������Ϣ�����������

	void SendToBattleServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid);									//������Ϣ��ս��������
	void SendToBattleServer(CMessage msg, int pid);																								//������Ϣ��ս��������

	inline int GetSID(){ return game_server_info_->area_id[0]; }																				//��ȡ��������

	inline networkz::ISession*	battle_session(){ return battle_server_session_; }

protected:
	virtual void _OnProcessMessage(const CMessage& msg) override;
	virtual void _OnClock(int clock_id, const tm& now_datetime) override;
	virtual void _OnConnect(networkz::ISession* session, int error_code) override;
	virtual void _OnRecycle(networkz::ISession* session, int error_code) override;

private:
	const GameServer* const			game_server_info_;								//��Ϸ��������Ϣ
	const BattleServer*	const		battle_server_info_;							//ս����������Ϣ

	networkz::ISession*				battle_server_session_{ nullptr };				//ս��������ָ��

	void __ShowServerInfo();														//��ʾ��������Ϣ
};
