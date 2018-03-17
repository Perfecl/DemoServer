#pragma once

struct PerMatch
{
	~PerMatch(){ for (auto &it : players)delete it.second; }

	BattleMode				match_mode{ BattleMode::NORMAL };
	int						point{ 0 };
	int						time{ 0 };
	int						point_tolerant{ 0 };
	networkz::ISession*		session{ nullptr };
	bool					is_first_match{ false };
	std::vector<std::pair<int, pto_BATTLE_STRUCT_Master_Ex*>> players;
};

class CBattleServer : public CBasicServer
{
public:
	static const int		kBattleServerLoopTime{ 100 };				//Loopѭ��ʱ��

public:
	CBattleServer(int id);
	~CBattleServer();

	networkz::ISession*	FindSession(int sid);
	int	GetSID(networkz::ISession* session);

	void	SendToGameServer(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session);
	void	SendToGameServer(CMessage msg, networkz::ISession* session);

	void	ProcessBattleCallbackMessage(const CMessage& msg, int pid, int sid);

protected:
	virtual void _OnProcessMessage(const CMessage& msg) override;
	virtual void _OnAccept(networkz::ISession* session, int error_code) override;
	virtual void _OnRecycle(networkz::ISession* session, int error_code) override;
	virtual void _OnClock(int clock_id, const tm& now_datetime) override;

private:
	const BattleServer* const battle_server_info_;						//ս����������Ϣ

	std::list<std::shared_ptr<PerMatch>>	list_war_;					//Warս��ƥ���б�
	std::list<std::shared_ptr<PerMatch>>	list_1v1_;					//1v1ս��ƥ���б�
	std::list<std::shared_ptr<PerMatch>>	list_2v2_;					//2v2ս��ƥ���б�
	std::list<std::shared_ptr<PerMatch>>	list_3v3_;					//3v3ս��ƥ���б�
	std::list<std::shared_ptr<PerMatch>>	list_practice_;				//��ϰս��ƥ���б�

	std::map<networkz::ISession*, int>		game_servers_;

	CCriticalSectionZ						lock_;						//�߳���

	bool									need_match_{ false };

	void __ShowServerInfo();											//��ʾ��������Ϣ

	void __OnGameServerConnect(const CMessage& msg);
	void __OnMatchBattle(const CMessage& msg);
	void __CancelMatch(const CMessage& msg);

	void __TimeLoop();
	void __DoMatch(std::list<std::shared_ptr<PerMatch>>& list);
	void __BeginWihtAI(std::shared_ptr<PerMatch> p1, BattleMode enMode);					//��AIƥ��
	void __MatchSuccess(std::shared_ptr<PerMatch> p1, std::shared_ptr<PerMatch> p2);		//ƥ��ɹ�
};

