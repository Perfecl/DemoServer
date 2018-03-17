#include "stdafx.h"
#include "GameServer.h"
#include <filesystem>

CGameServer::CGameServer(int id) :
game_server_info_{ SERVER_LIST()->GetGameServerInformation(id) },
battle_server_info_{ SERVER_LIST()->GetBattleServerInformation(game_server_info_->battle_server_id) }
{
	if (!battle_server_info_)
		throw std::exception(FormatString("找不到游戏服务器信息,ServerID:", id).c_str());

	__ShowServerInfo();

	InitLoggerService(FormatString("Logs\\S", id, "TIME", time(0), ".log").c_str());

#ifdef _TEST
	InitTCPService(game_server_info_->tcp_port, 1, true);
#else
	//VerifySecurityKeyFile();
	InitTCPService(game_server_info_->tcp_port, 1, false);
#endif
}

CGameServer::~CGameServer()
{
	CGameWorld::CloseGameWorld();
}

bool CGameServer::ConnectDatabase()
{
	auto database_info = SERVER_LIST()->GetDatabaseInformation(game_server_info_->database_id);
	return CBasicServer::ConnectDatabase(SERVER_LIST()->GetHostIP(database_info->host_id), database_info->port, database_info->username.c_str(), database_info->password.c_str(), database_info->database_name.c_str());
}

void CGameServer::ConnectBattleServer()
{
	CBasicServer::PostConnect(SERVER_LIST()->GetHostIP(battle_server_info_->host_id), battle_server_info_->tcp_port);
}

void CGameServer::CreateGameWorld()
{
	CGameWorld::InitializeGameWorld(*this);
}

void CGameServer::SendToAgentServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid, networkz::ISession* session)
{
	SendToAgentServer({ content, protocol_type, protocol_id }, pid, session);
}

void CGameServer::SendToAgentServer(CMessage msg, int pid, networkz::ISession* session)
{
	msg.SetID(pid);

	if (session)
		session->send(msg, msg.length());
	else
		RECORD_WARNING("空的AgentServer会话");
}

void CGameServer::SendToBattleServer(const std::string& content, unsigned char protocol_type, int protocol_id, int pid)
{
	SendToBattleServer({ content, protocol_type, protocol_id }, pid);
}

void CGameServer::SendToBattleServer(CMessage msg, int pid)
{
	msg.SetID(pid);

	if (battle_server_session_)
		battle_server_session_->send(msg, msg.length());
	else
		RECORD_WARNING("空的BattleServer会话");
}

void CGameServer::VerifySecurityKeyFile()
{
	if (!std::tr2::sys::exists(std::tr2::sys::path(ToMD5(MacAddress() + SECURITY_KEY) + ".key")))
		throw std::exception(FormatString("未找到安全密钥文件,请把代码发至管理员:", MacAddress()).c_str());
}

void CGameServer::_OnProcessMessage(const CMessage& msg)
{
	GAME_WORLD()->ProcessMessage(msg);
}

void CGameServer::_OnClock(int clock_id, const tm& now_datetime)
{
	GAME_WORLD()->OnClock(clock_id, now_datetime);
}

void CGameServer::_OnRecycle(networkz::ISession* session, int error_code)
{
	if (session == battle_server_session_)
	{
		battle_server_session_ = nullptr;

		RECORD_INFO(FormatString("和BattleServer断开连接IP:", session->remote_ip()));
	}
	else
	{
		GAME_WORLD()->OfflineBySession(session);

		RECORD_INFO(FormatString("和AgentServer断开连接,IP:", session->remote_ip()));
	}
}

void CGameServer::_OnConnect(networkz::ISession* session, int error_code)
{
	if (error_code)
	{
		RECORD_TRACE("正在重连游戏服务器.");
		ConnectBattleServer();
	}
	else
	{
		battle_server_session_ = session;
		pto_SYSTEM_S2B_NTF_Connect pto;
		pto.set_sid(GAME_WORLD()->sid());
		pto.set_code("BattleServer");
		std::string str;
		pto.SerializeToString(&str);
		SendToBattleServer(str, MSG_S2B, SYSTEM_S2B_NTF_Connect, 0);
		RECORD_INFO("连接战斗服务器成功");
	}
}

void CGameServer::__ShowServerInfo()
{
	_PrintLine("*****************************************");
	_Print("服务器名:\tGameServer");

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
	_PrintLine(FormatString("平台ID:\t\t", game_server_info_->platform_id).c_str());
	_PrintLine(FormatString("服务器号:\t", game_server_info_->area_id[0]).c_str());
	_PrintLine("*****************************************");
}
