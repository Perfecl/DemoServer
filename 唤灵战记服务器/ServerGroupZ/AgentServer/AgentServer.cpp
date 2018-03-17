#include "stdafx.h"
#include "AgentServer.h"

CAgentServer::CAgentServer(int id) :
agent_server_info_{ SERVER_LIST()->GetAgentServerInformation(id) },
game_server_info_{ CServerList::GetInstance()->GetGameServerInformation(agent_server_info_->game_server_id) }
{
	if (!game_server_info_)
		throw std::exception(FormatString("找不到代理服务器信息,ServerID:", id).c_str());

	__ShowServerInfo();

#ifdef _TEST
	InitTCPService(agent_server_info_->tcp_port, 100, true);
	InitUDPService(agent_server_info_->udp_port, 128, 100, true);
#else
	InitTCPService(agent_server_info_->tcp_port, 100, false);
	InitUDPService(agent_server_info_->udp_port, 128, 100, false);
#endif

	StartClock(1, kAgentServerLoopTime);
}

CAgentServer::~CAgentServer()
{

}

void CAgentServer::ConnectGameServer()
{
	CBasicServer::PostConnect(SERVER_LIST()->GetHostIP(game_server_info_->host_id), game_server_info_->tcp_port);
}

void CAgentServer::SendToGameServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid)
{
	SendToGameServer({ content, protocol_type, protocol_id }, pid);
}

void CAgentServer::SendToGameServer(CMessage msg, int pid)
{
	msg.SetID(pid);

	if (gameserver_session_)
		gameserver_session_->send(msg, msg.length());
	else
		RECORD_TRACE("发送失败,GameServer断开连接");
}

void CAgentServer::SendToClient(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session)
{
	SendToClient({ content, protocol_type, protocol_id }, session);
}

void CAgentServer::SendToClient(CMessage msg, networkz::ISession* session)
{
	msg.ClearID();

	if (session)
		session->send(msg, msg.length());
	else
		RECORD_TRACE("发送失败,空的用户会话");
}

void CAgentServer::_OnConnect(networkz::ISession* session, int error_code)
{
	if (error_code)
	{
		RECORD_TRACE("正在重连游戏服务器.");
		ConnectGameServer();
	}
	else
	{
		gameserver_session_ = session;
		RECORD_INFO("连接游戏服务器成功");
	}
}

void CAgentServer::_OnAccept(networkz::ISession* session, int error_code)
{
	RECORD_INFO(FormatString("有一个新用户连接:", session->remote_ip()));

	LOCK_BLOCK(lock_);
	unverified_sessions_.insert(std::make_pair(session, time(0)));
}

void CAgentServer::_OnRecycle(networkz::ISession* session, int error_code)
{
	if (session == gameserver_session_)
	{
		gameserver_session_ = nullptr;

		__Clear();

		RECORD_INFO(FormatString("和GameServer断开连接,准备重新连接... ip:", session->remote_ip()));

		ConnectGameServer();
	}
	else
	{
		lock_.lock();
		unverified_sessions_.erase(session);
		lock_.unlock();

		SessionInfo* session_info = reinterpret_cast<SessionInfo*>(session->argument());
		if (session_info)
		{
			player_lock_.lock();
			online_players_.erase(session_info->first);
			player_lock_.unlock();
			SendToGameServer("", MSG_A2S, SYSTEM_A2S_NTF_OffLine, session_info->first);
			delete session_info;
		}
	}
}

void CAgentServer::_OnUDPReceive(const char* data, size_t length, const char* remote_ip, int error_code)
{
	if (!CMessage::HasHeaderFormString(data, length))
	{
		RECORD_TRACE("UDP丢包");
		return;
	}

	CMessage msg{ std::move(CMessage::FromString(data, length)) };

	if (msg.GetProtocolID() == SYSTEM_L2A_NTF_ValidLogin)
	{
		pto_SYSTEM_L2A_NTF_ValidLogin pto;
		pto.ParseFromArray(msg.body_c_str(), msg.body_size());

		CanLoginInfo info{ 0 };
		info.time = time(0);
		info.sid = pto.sid();
		strcpy_s(info.ip, pto.ip().c_str());

		LOCK_BLOCK(lock_);
		can_login_info_.insert(std::make_pair(pto.pid(), std::move(info)));
	}
	else
	{
		RECORD_WARNING(FormatString("未知的UDP协议号", msg.GetProtocolID()));
	}
}

void CAgentServer::_OnClock(int clock_id, const tm& now_datetime)
{
	if (1 == clock_id)
	{
		lock_.lock();
		for (auto &it : unverified_sessions_)
		{
			if ((time(0) - it.second) > kInvalidConnectTimeout)
				it.first->close();
		}

		for (auto it = can_login_info_.begin(); it != can_login_info_.cend();)
		{
			if ((time(0) - it->second.time) > kValidLoginTimeOut)
				it = can_login_info_.erase(it);
			else
				++it;
		}
		lock_.unlock();

#ifndef _TEST
		player_lock_.lock();
		for (auto &it : online_players_)
		{
			SessionInfo* info = reinterpret_cast<SessionInfo*>(it.second->argument());

			if (info)
			{
				if (info && (time(0) - info->second) > kHeartBeatTime)
				{
					RECORD_TRACE("心跳包超时");
					it.second->close();
				}
			}
			else
			{
				RECORD_WARNING("计算心跳包时找不到会话信息");
				it.second->close();
			}
		}
		player_lock_.unlock();
#endif
	}
}

void CAgentServer::_C2S(const CMessage& msg)
{
	if (!gameserver_session_)
		return;

	if (LOGIN_C2S_REQ_EnterGame == msg.GetProtocolID())
	{
		__DoEnterGame(msg);
	}
	else
	{
		SessionInfo* info = reinterpret_cast<SessionInfo*>(msg.session()->argument());

		if (info)
		{
			SendToGameServer(msg, info->first);
		}
		else
		{
			RECORD_WARNING(FormatString("找不到用户信息,protoco;", msg.GetProtocolID()));
			msg.session()->close();
		}
	}
}

void CAgentServer::_S2C(const CMessage& msg)
{
	if (LOGIN_S2C_RES_EnterGame == msg.GetProtocolID())
		__OnEnterGame(msg);
	else
		SendToClient(msg, __FindOnlinePlayerSession(msg.GetID()));
}

void CAgentServer::_C2A(const CMessage& msg)
{
	if (msg.GetProtocolID() == SYSTEM_C2A_REQ_HeartBeat)
	{
		SessionInfo* info = reinterpret_cast<SessionInfo*>(msg.session()->argument());
		if (info)
		{
			info->second = time(0);
			SendToClient("", MSG_A2C, SYSTEM_A2C_RES_HeartBeat, msg.session());
		}
	}
	else
	{
		RECORD_WARNING(FormatString("未知的C2A协议", msg.GetProtocolID()));
	}
}

void CAgentServer::_S2A(const CMessage& msg)
{
	if (msg.GetProtocolID() == SYSTEM_S2A_NTF_Transpond)
	{
		pto_SYSTEM_S2A_NTF_Transpond pto;
		pto.ParseFromArray(msg.body_c_str(), msg.body_size());
		for (int i = 0; i < pto.pid_size(); i++)
			SendToClient(std::move(CMessage(pto.message())), __FindOnlinePlayerSession(pto.pid(i)));
	}
	else if (msg.GetProtocolID() == SYSTEM_S2A_NTF_GMKick)
	{
		int pid = msg.GetID();

		auto session = __FindOnlinePlayerSession(pid);
		if (session)
			session->close();
	}
	else
	{
		RECORD_WARNING("未知的S2A协议");
	}
}

void CAgentServer::__ShowServerInfo()
{
	_PrintLine("*****************************************");
	_Print("服务器名:\tAgentServer");

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
	_PrintLine(FormatString("平台ID:\t\t", agent_server_info_->platform_id).c_str());
	_PrintLine(FormatString("服务器号:\t", game_server_info_->area_id[0]).c_str());
	_PrintLine("*****************************************");
}

void CAgentServer::__DoEnterGame(const CMessage& msg)
{
	pto_LOGIN_C2S_REQ_EnterGame pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	int sid = __GetCanLoginSID(pto.pid(), msg.session()->remote_ip());
	if (!sid)
	{
		RECORD_WARNING(FormatString("找不到可登录sid,pid:", pto.pid(), ",ip:", msg.session()->remote_ip()));
		return;
	}

	lock_.lock();
	unverified_sessions_.erase(msg.session());
	lock_.unlock();

	networkz::ISession* old_session = __FindOnlinePlayerSession(pto.pid());
	if (old_session)
	{
		pto_SYSTEM_A2C_NTF_Kick pto_kick;
		pto_kick.set_reason(KR_LOGIN_AGAIN);
		std::string str;
		pto_kick.SerializeToString(&str);
		SendToClient(str, MSG_A2C, SYSTEM_A2C_NTF_Kick, old_session);

		delete reinterpret_cast<SessionInfo*>(old_session->argument());
		old_session->argument(nullptr);

		lock_.lock();
		unverified_sessions_[old_session] = time(0);
		lock_.unlock();
	}

	msg.session()->argument(new SessionInfo(pto.pid(), time(0)));

	player_lock_.lock();
	online_players_[pto.pid()] = msg.session();
	player_lock_.unlock();

	SendToGameServer(msg, pto.pid());
}

void CAgentServer::__OnEnterGame(const CMessage& msg)
{
	pto_LOGIN_S2C_RES_EnterGame pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	networkz::ISession* session = __FindOnlinePlayerSession(msg.GetID());

	if (session)
	{
		SendToClient(msg, session);

		if (false == pto.result())
		{
			delete reinterpret_cast<SessionInfo*>(session->argument());
			session->argument(nullptr);

			player_lock_.lock();
			online_players_.erase(msg.GetID());
			player_lock_.unlock();

			lock_.lock();
			unverified_sessions_[session] = time(0);
			lock_.unlock();
		}
	}
	else
	{
		RECORD_WARNING("返回进入游戏时找不到玩家");
	}
}

int CAgentServer::__GetCanLoginSID(int pid, const char* ip)
{
	LOCK_BLOCK(lock_);

	auto it = can_login_info_.find(pid);

	if (can_login_info_.cend() != it)
	{
		if (0 == strcmp(it->second.ip, ip))
		{
			int sid = it->second.sid;
			can_login_info_.erase(pid);
			return sid;
		}
	}

	return 0;
}

networkz::ISession* CAgentServer::__FindOnlinePlayerSession(int pid)
{
	LOCK_BLOCK(player_lock_);

	auto it = online_players_.find(pid);

	if (online_players_.cend() == it)
		return nullptr;
	else
		return it->second;
}

void CAgentServer::__Clear()
{
	lock_.lock();
	for (auto &it : unverified_sessions_)
		it.first->close();
	can_login_info_.clear();
	lock_.unlock();

	player_lock_.lock();
	for (auto &it : online_players_)
		it.second->close();
	player_lock_.unlock();
}
