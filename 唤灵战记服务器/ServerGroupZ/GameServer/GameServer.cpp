#include "stdafx.h"
#include "GameServer.h"
#include <filesystem>

CGameServer::CGameServer(int id) :
game_server_info_{ SERVER_LIST()->GetGameServerInformation(id) },
battle_server_info_{ SERVER_LIST()->GetBattleServerInformation(game_server_info_->battle_server_id) }
{
	if (!battle_server_info_)
		throw std::exception(FormatString("�Ҳ�����Ϸ��������Ϣ,ServerID:", id).c_str());

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
		RECORD_WARNING("�յ�AgentServer�Ự");
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
		RECORD_WARNING("�յ�BattleServer�Ự");
}

void CGameServer::VerifySecurityKeyFile()
{
	if (!std::tr2::sys::exists(std::tr2::sys::path(ToMD5(MacAddress() + SECURITY_KEY) + ".key")))
		throw std::exception(FormatString("δ�ҵ���ȫ��Կ�ļ�,��Ѵ��뷢������Ա:", MacAddress()).c_str());
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

		RECORD_INFO(FormatString("��BattleServer�Ͽ�����IP:", session->remote_ip()));
	}
	else
	{
		GAME_WORLD()->OfflineBySession(session);

		RECORD_INFO(FormatString("��AgentServer�Ͽ�����,IP:", session->remote_ip()));
	}
}

void CGameServer::_OnConnect(networkz::ISession* session, int error_code)
{
	if (error_code)
	{
		RECORD_TRACE("����������Ϸ������.");
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
		RECORD_INFO("����ս���������ɹ�");
	}
}

void CGameServer::__ShowServerInfo()
{
	_PrintLine("*****************************************");
	_Print("��������:\tGameServer");

#ifdef _TEST
	_Print("(���԰�)");
#endif 

	_Print("\n���а汾:\t");

#ifdef _DEBUG 
	_PrintLine("Debug");
#else
	_PrintLine("Release");
#endif

	char str_data[32]{0};
	time_t now = time(0);
	ctime_s(str_data, sizeof(str_data), &now);

	_Print("����ʱ��:\t");
	_Print(str_data);
	_PrintLine(FormatString("ƽ̨ID:\t\t", game_server_info_->platform_id).c_str());
	_PrintLine(FormatString("��������:\t", game_server_info_->area_id[0]).c_str());
	_PrintLine("*****************************************");
}
