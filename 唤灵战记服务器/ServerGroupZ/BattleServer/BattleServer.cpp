#include "stdafx.h"
#include "BattleServer.h"

CBattleServer::CBattleServer(int id) :
battle_server_info_{ SERVER_LIST()->GetBattleServerInformation(id) }
{
	if (!battle_server_info_)
		throw std::exception(FormatString("找不到战斗服务器信息,ServerID:", id).c_str());

	__ShowServerInfo();

#ifdef _TEST
	InitTCPService(battle_server_info_->tcp_port, 1, true);
#else
	InitTCPService(battle_server_info_->tcp_port, 1, false);
#endif

	CUnitTP::Load();
	CSoldierTP::Load();
	CHeroTP::Load();
	CSkill::Load();
	CPassSkill::Load();
	CEquipTP::Load();
	CAddition::Load();
	CInteriorLib::Load();
	CEliteStage::Load();
	CItemTP::Load();
	CLottery::Load();
	CBattleManager::LoadData();
	CSuitTP::Load();

	StartClock(1, kBattleServerLoopTime);

	CBattleManager::StartBattleService([](const CMessage& msg, void* param, int pid, int sid)
	{
		((CBattleServer*)param)->ProcessBattleCallbackMessage(msg, pid, sid);
	}, this);
}

CBattleServer::~CBattleServer()
{

}

networkz::ISession* CBattleServer::FindSession(int sid)
{
	LOCK_BLOCK(lock_);
	for (auto &it : game_servers_)
	{
		if (it.second == sid)
			return it.first;
	}

	return nullptr;
}

int	CBattleServer::GetSID(networkz::ISession* session)
{
	LOCK_BLOCK(lock_);
	auto it = game_servers_.find(session);

	if (it == game_servers_.cend())
		return 0;
	else
		return it->second;
}

void CBattleServer::SendToGameServer(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session)
{
	SendToGameServer({ content, protocol_type, protocol_id }, session);
}

void CBattleServer::SendToGameServer(CMessage msg, networkz::ISession* session)
{
	if (session)
		session->send(msg, msg.length());
}

void CBattleServer::ProcessBattleCallbackMessage(const CMessage& msg, int pid, int sid)
{
	if (sid <= 0)
		return;
	SendToGameServer(msg, FindSession(sid));
}

void CBattleServer::__ShowServerInfo()
{
	_PrintLine("*****************************************");
	_Print("服务器名:\tBattleServer");

#ifdef _TEST
	_Print("(测试版)");
#endif 

	_Print("\n运行版本:\t");

#ifdef _DEBUG 
	_PrintLine("Debug");
#else
	_PrintLine("Release");
#endif

	char str_data[32]{0};
	time_t now = time(0);
	ctime_s(str_data, sizeof(str_data), &now);

	_Print("启动时间:\t");
	_Print(str_data);
	_PrintLine(FormatString("平台ID:\t\t", battle_server_info_->platform_id).c_str());
	_PrintLine("*****************************************");
}

void CBattleServer::_OnProcessMessage(const CMessage& msg)
{
	switch (msg.GetProtocolID())
	{
	case BATTLE_S2B_REQ_MatchBattle:
		__OnMatchBattle(msg);
		break;
	case ROOM_C2S_NTF_CancelMatch:
		__CancelMatch(msg);
		break;
	case SYSTEM_S2B_NTF_Connect:
		__OnGameServerConnect(msg);
		break;
	default:
		CBattleManager::ProcessMessage(msg);
		break;
	}
}

void CBattleServer::_OnAccept(networkz::ISession* session, int error_code)
{
	RECORD_INFO(FormatString("有一个新GameServer连接:", session->remote_ip()));
}

void CBattleServer::_OnRecycle(networkz::ISession* session, int error_code)
{
	LOCK_BLOCK(lock_);
	game_servers_.erase(session);
}

void CBattleServer::_OnClock(int clock_id, const tm& now_datetime)
{
	__TimeLoop();
}

void CBattleServer::__OnMatchBattle(const CMessage& msg)
{
	pto_BATTLE_S2B_REQ_MatchBattle_Ex pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	std::shared_ptr<PerMatch> pMatch = std::make_shared<PerMatch>();

	pMatch->match_mode = (BattleMode)pto.match_mode();
	pMatch->point = pto.point();
	pMatch->session = msg.session();
	pMatch->is_first_match = pto.is_first();

	for (int i = 0; i < pto.master_size(); i++)
		pMatch->players.push_back(std::make_pair(pto.master(i).pid(), new pto_BATTLE_STRUCT_Master_Ex(pto.master(i))));

	{
		LOCK_BLOCK(lock_);

		switch (pMatch->match_mode)
		{
		case BattleMode::WAR:
			list_war_.push_back(pMatch);
			break;
		case BattleMode::ARENA1v1:
			list_1v1_.push_back(pMatch);
			break;
		case BattleMode::ARENA3v3:
			list_3v3_.push_back(pMatch);
			break;
		case BattleMode::EXERCISE:
			list_practice_.push_back(pMatch);
			break;
		}
	}

	need_match_ = true;

	RECORD_TRACE("开始匹配");
}

void CBattleServer::__CancelMatch(const CMessage& msg)
{
	std::list<std::shared_ptr<PerMatch>>* arr[]{&list_war_, &list_1v1_, &list_2v2_, &list_3v3_, &list_practice_};

	LOCK_BLOCK(lock_);

	for (auto &itList : arr)
	{
		for (auto itPerMatch = itList->begin(); itPerMatch != itList->cend(); ++itPerMatch)
		{
			int nPID{ msg.GetID() };

			for (auto &it : (*itPerMatch)->players)
			{
				if (it.first == nPID)
				{
					pto_BATTLE_B2S_NTF_CancelMatch pto;
					for (auto &it_player : (*itPerMatch)->players)
						pto.add_pid(it_player.first);

					std::string str;
					pto.SerializeToString(&str);

					SendToGameServer(str, MSG_B2S, BATTLE_B2S_NTF_CancelMatch, (*itPerMatch)->session);

					itList->erase(itPerMatch);
					return;
				}
			}
		}
	}
}

void CBattleServer::__TimeLoop()
{
	if (!need_match_)
		return;

	std::list<std::shared_ptr<PerMatch>>* arr[]{&list_war_, &list_1v1_, &list_2v2_, &list_3v3_, &list_practice_};

	LOCK_BLOCK(lock_);

	bool is_empty{ true };

	for (auto &itList : arr)
	{
		if (itList)
		{
			__DoMatch(*itList);

			if (!itList->empty())
			{
				is_empty = false;

				for (auto &it : *itList)
				{
					it->time += kBattleServerLoopTime;

					if (it->point_tolerant < 150 && (it->time % 5000 == 0))
					{
						if (itList->size() < 100)
							it->point_tolerant += 15;
						else
							it->point_tolerant += 1;
					}
				}
			}
		}
	}

	if (is_empty)
		need_match_ = false;
}

void CBattleServer::__DoMatch(std::list<std::shared_ptr<PerMatch>>& list)
{
	for (auto itBegin = list.begin(); itBegin != list.cend(); ++itBegin)
	{
		auto it = itBegin;
		++it;

		if ((*itBegin)->match_mode == BattleMode::EXERCISE)
		{
			if ((*itBegin)->time >= 30000)
			{
				__BeginWihtAI(*itBegin, BattleMode::ARENA1v1Normal);
				list.erase(itBegin);
				return;
			}
		}

		if ((*itBegin)->match_mode == BattleMode::ARENA1v1)
		{
			if (((*itBegin)->point == 1500 && (*itBegin)->is_first_match) || (*itBegin)->point < 1400)
			{
				__BeginWihtAI(*itBegin, BattleMode::ARENA1v1Easy);
				list.erase(itBegin);
				return;
			}

			if ((*itBegin)->point <= 1700 && (*itBegin)->time >= 20000)
			{
				__BeginWihtAI(*itBegin, BattleMode::ARENA1v1Easy);
				list.erase(itBegin);
				return;
			}
			else if ((*itBegin)->point <= 1900 && (*itBegin)->time >= 30000)
			{
				__BeginWihtAI(*itBegin, BattleMode::ARENA1v1Normal);
				list.erase(itBegin);
				return;
			}
		}

		if ((*itBegin)->match_mode == BattleMode::ARENA3v3)
		{
			if (((*itBegin)->point == 1500 && (*itBegin)->is_first_match) || (*itBegin)->point < 1400)
			{
				__BeginWihtAI(*itBegin, BattleMode::ARENA3v3Easy);
				list.erase(itBegin);
				return;
			}
			else if ((*itBegin)->point <= 1900 && (*itBegin)->time >= 30000)
			{
				__BeginWihtAI(*itBegin, BattleMode::ARENA3v3Normal);
				list.erase(itBegin);
				return;
			}
		}

		while (true)
		{
			if (it == list.cend())
				break;

			int nMin1{ (*itBegin)->point - (*itBegin)->point_tolerant };
			int nMax1{ (*itBegin)->point + (*itBegin)->point_tolerant };

			int nMin2{ (*it)->point - (*it)->point_tolerant };
			int nMax2{ (*it)->point + (*it)->point_tolerant };

			int  z1 = nMin1 > nMin2 ? nMin1 : nMin2;
			int  z2 = nMax1 < nMax2 ? nMax1 : nMax2;

			if (z1 > z2)
			{
				++it;
			}
			else
			{
				__MatchSuccess(*itBegin, *it);
				list.erase(it);
				list.erase(itBegin);
				return;
			}
		}
	}
}

void CBattleServer::__BeginWihtAI(std::shared_ptr<PerMatch> p1, BattleMode enMode)
{
	pto_BATTLE_S2B_REQ_CreateBattle_Ex pto;
	pto.set_map(3);

	pto.set_mode((int)enMode);

	for (auto &it : p1->players)
		pto.add_atk_masters()->CopyFrom(*it.second);
	pto.set_atk_sid(GetSID(p1->session));
	pto.set_atk_point(p1->point);
	pto.set_def_sid(-1);

	switch (enMode)
	{
	case BattleMode::ARENA1v1Easy:
		pto.set_def_point(1500);
		break;
	case BattleMode::ARENA1v1Normal:
		pto.set_def_point(1700);
		break;
	case BattleMode::ARENA3v3Easy:
		pto.set_def_point(1500);
		break;
	case BattleMode::ARENA3v3Normal:
		pto.set_def_point(1700);
		break;
	default:
		pto.set_def_point(1500);
		break;
	}

	std::string str;
	pto.SerializeToString(&str);

	CMessage msg{ str, MSG_S2B, BATTLE_S2B_REQ_CreateBattle };
	CBattleManager::ProcessMessage(msg);
}

void CBattleServer::__MatchSuccess(std::shared_ptr<PerMatch> p1, std::shared_ptr<PerMatch> p2)
{
	RECORD_TRACE("匹配成功");

	pto_BATTLE_S2B_REQ_CreateBattle_Ex ptoCreateBattle;
	ptoCreateBattle.set_map(3);

	ptoCreateBattle.set_mode((int)p1->match_mode);

	for (auto &it : p1->players)
		ptoCreateBattle.add_atk_masters()->CopyFrom(*it.second);
	ptoCreateBattle.set_atk_sid(GetSID(p1->session));
	ptoCreateBattle.set_atk_point(p1->point);

	for (auto &it : p2->players)
		ptoCreateBattle.add_def_masters()->CopyFrom(*it.second);
	ptoCreateBattle.set_def_sid(GetSID(p2->session));
	ptoCreateBattle.set_def_point(p2->point);

	std::string str;
	ptoCreateBattle.SerializeToString(&str);

	CMessage msg{ str, MSG_S2B, BATTLE_S2B_REQ_CreateBattle };
	CBattleManager::ProcessMessage(msg);
}

void CBattleServer::__OnGameServerConnect(const CMessage& msg)
{
	pto_SYSTEM_S2B_NTF_Connect pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());
	LOCK_BLOCK(lock_);
	game_servers_[msg.session()] = pto.sid();
}
