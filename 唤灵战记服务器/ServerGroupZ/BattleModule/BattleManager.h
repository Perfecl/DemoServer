#pragma once
#include <mmsystem.h>

typedef void(*BattleCallback)(const CMessage& msg, void* param, int pid, int sid);

class CBattleManager
{
public:
	static const int kBattleLoopInterval{ 10 };						//ս��loop���

	static CBattleManager* GetInstance(){ return instance_; }

	static void DoCallback(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, int sid);
	static void DoCallback(CMessage msg, int pid, int sid);

	static void ProcessMessage(const CMessage& msg);

	static void StartBattleService(BattleCallback fun, void* param);

	static void LoadData();

private:
	static CBattleManager* instance_;

public:
	~CBattleManager();

	void SetCallback(BattleCallback fun, void* param){ callback_function_ = fun; callback_param_ = param; }

	void Loop();

private:
	std::unordered_map<int, CBattle*>	player_battles_;						//��Һ�ս��ӳ��
	CCriticalSectionZ					battle_lock_;							//ս���߳���

	std::queue<CMessage>				message_queue_;							//��Ϣ����
	CCriticalSectionZ					lock_;									//�߳���

	std::set<CBattle*>					battles_;								//����ս��

	MMRESULT							timer_id_{ 0 };							//��ʱ��ID

	BattleCallback						callback_function_{ nullptr };			//ս���ص�����
	void*								callback_param_{ nullptr };

	CBattleManager();

	void __CreateBattle(const CMessage& msg);
};

inline void	SEND_TO_GAME(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, int sid)
{
	CBattleManager::GetInstance()->DoCallback(content, protocol_type, protocol_id, pid, sid);
}

inline void	SEND_TO_GAME(const CMessage &msg, int pid, int sid)
{
	CBattleManager::GetInstance()->DoCallback(msg, pid, sid);
}
