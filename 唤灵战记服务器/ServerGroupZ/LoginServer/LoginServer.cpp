#include "stdafx.h"
#include "LoginServer.h"
#include <regex>

CLoginServer::CLoginServer(int id) :
login_server_info_{ SERVER_LIST()->GetLoginServerInformation(id) }
{
	if (!login_server_info_)
		throw std::exception(FormatString("找不到登陆服务器信息,ServerID:", id).c_str());

	for (auto &game : SERVER_LIST()->GetGameServersByPlatform(login_server_info_->platform_id))
	{
		for (auto &area : game->area_id)
			agent_servers_[area] = SERVER_LIST()->GetAgentServersByGameServerID(game->id);
	}

	__ShowServerInfo();

	CIllegalWords::Load();

#ifdef _TEST
	InitTCPService(login_server_info_->tcp_port, 100, true);
	InitUDPService(login_server_info_->udp_port, 128, 100, true);
#else
	InitTCPService(login_server_info_->tcp_port, 100, false);
	InitUDPService(login_server_info_->udp_port, 128, 100, false);
#endif

	StartClock(1, kLoginServerLoopTime);
}

CLoginServer::~CLoginServer()
{

}

void CLoginServer::ConnectDatabase()
{
	auto database_info = SERVER_LIST()->GetDatabaseInformation(login_server_info_->database_id);
	CBasicServer::ConnectDatabase(SERVER_LIST()->GetHostIP(database_info->host_id), database_info->port, database_info->username.c_str(), database_info->password.c_str(), database_info->database_name.c_str());
}

void CLoginServer::SendToClient(const std::string& content, unsigned char protocol_type, int protocol_id, networkz::ISession* session)
{
	SendToClient({ content, protocol_type, protocol_id }, session);
}

void CLoginServer::SendToClient(CMessage msg, networkz::ISession* session)
{
	msg.ClearID();

	if (session)
		session->send(msg, msg.length());
	else
		RECORD_ERROR("发送失败,空的用户会话");
}

void CLoginServer::SendToAgent(const std::string& content, unsigned char protocol_type, int protocol_id, const char* ip, unsigned short port)
{
	CMessage msg{ content, protocol_type, protocol_id };

	CBasicServer::UDPSend(msg, msg.length(), ip, port);
}

void CLoginServer::_OnAccept(networkz::ISession* session, int error_code)
{
	if (!error_code)
	{
		RECORD_INFO(FormatString("新用户连接:", session->remote_ip()));
		lock_.lock();
		all_sessions_.insert(std::make_pair(session, time(0)));
		lock_.unlock();
	}
}

void CLoginServer::_OnClock(int clock_id, const tm& now_datetime)
{
	if (1 == clock_id)
	{
		LOCK_BLOCK(lock_);
		for (auto &it : all_sessions_)
		{
			if ((time(0) - it.second) > kInvalidConnectTimeout)
			{
				RECORD_DEBUG("登陆超时,断开连接");
				it.first->close();
			}
		}
	}
}

void CLoginServer::_OnRecycle(networkz::ISession* session, int error_code)
{
	UserLoginInfo* user_info = static_cast<UserLoginInfo*>(session->argument());
	if (user_info)
		delete user_info;
	LOCK_BLOCK(lock_);
	all_sessions_.erase(session);
}

void CLoginServer::_C2A(const CMessage& msg)
{
	int protocol_id = msg.GetProtocolID();

	switch (protocol_id)
	{
	case LOGIN_C2A_REQ_Login:
		__DoLogin(msg);
		break;
	case LOGIN_C2A_REQ_Create:
		__DoCreate(msg);
		break;
	case LOGIN_C2A_REQ_GetName:
		__OnGetName(msg);
		break;
	default:
		RECORD_WARNING(FormatString("未知的登陆协议号:", protocol_id));
		break;
	}
}

void CLoginServer::__ShowServerInfo()
{
	_PrintLine("*****************************************");
	_Print("服务器名:\tLoginServer");

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
	_PrintLine(FormatString("平台ID:\t\t", login_server_info_->platform_id).c_str());
	_PrintLine("*****************************************");
}

void CLoginServer::__DoLogin(const CMessage& msg)
{
	pto_LOGIN_C2A_REQ_Login pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	//验证用户名
	if (false == __VerifyUsername(pto.username()))
	{
		msg.session()->close();
		return;
	}

#ifndef _TEST
	//平台登陆验证
	if (false == __VerifyLoginCode(pto.username(), pto.time(), pto.sid(), pto.is_adult(), pto.code()))
	{
		msg.session()->close();
		return;
	}
#endif

	//增加登陆时长
	lock_.lock();
	all_sessions_[msg.session()] += kLoginTimeout;
	lock_.unlock();

	//把SID当作参数加入Session
	msg.session()->argument(new UserLoginInfo{ pto.sid(), pto.username() });

	std::ostringstream sql;
	sql << "select sid from tb_game_world where sid = " << pto.sid();
	ResultSetPtr sql_exist{ MySQLQuery(sql.str()) };

	LOGIN_RESULT login_result{ LR_SUCCESS };

	if (sql_exist && sql_exist->next())
	{
		sql.str("");
		sql << "select pid,unix_timestamp(frozen_time) from tb_player where username = '" << pto.username().c_str() << "' and sid = " << pto.sid();
		ResultSetPtr sql_result{ MySQLQuery(sql.str()) };
		if (!sql_result)
			return;

		if (sql_result->next())
		{
			int pid = sql_result->getInt(1);
			time_t frozen_time = sql_result->getInt64(2);

			if (time(0) >= frozen_time)
			{
				__LoginSuccess(pid, msg.session());
				return;
			}
			else
			{
				login_result = LR_FROZEN;
			}
		}
		else
		{
			login_result = LR_NEED_CREATE;
		}
	}
	else
	{
		login_result = LR_NOT_FIND_SERVER;
	}

	pto_LOGIN_A2C_RES_Login pto_res;
	pto_res.set_result(login_result);
	std::string str;
	pto_res.SerializeToString(&str);
	SendToClient(str, MSG_A2C, LOGIN_A2C_RES_Login, msg.session());
}

void CLoginServer::__DoCreate(const CMessage& msg)
{
	networkz::ISession* session = msg.session();

	UserLoginInfo* user_info = static_cast<UserLoginInfo*>(session->argument());

	if (!user_info)
	{
		RECORD_WARNING("未经验证的玩家");
		return;
	}

	pto_LOGIN_C2A_REQ_Create pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	LOGIN_RESULT result = static_cast<LOGIN_RESULT>(__CheckPlayerName(pto.name(), user_info->first));

	if (LR_SUCCESS == result)
	{
		Concurrency::create_task([this, user_info, session, pto]()
		{
			LOGIN_RESULT result = LR_SUCCESS;

			std::ostringstream sql;
			sql << "call sp_create_player(" << user_info->first << ",'" << user_info->second.c_str() << "','" << pto.name().c_str() << "'," << pto.sex() << ", @param_pid)";
			MySQLExecute(sql.str());

			ResultSetPtr create_result{ MySQLQuery("select @param_pid") };
			if (create_result && create_result->next())
			{
				__LoginSuccess(create_result->getInt(1), session);
				return;
			}
			else
			{
				RECORD_ERROR("创建人物失败");
				result = LR_CREATE_FAIL;
			}

			pto_LOGIN_A2C_RES_Login pto_res;
			pto_res.set_result(result);
			std::string str;
			pto_res.SerializeToString(&str);
			SendToClient(str, MSG_A2C, LOGIN_A2C_RES_Login, session);
		});
	}
	else
	{
		pto_LOGIN_A2C_RES_Login pto_res;
		pto_res.set_result(result);
		std::string str;
		pto_res.SerializeToString(&str);
		SendToClient(str, MSG_A2C, LOGIN_A2C_RES_Login, msg.session());
	}
}

void CLoginServer::__OnGetName(const CMessage& msg)
{
	if (!msg.session()->argument())
	{
		RECORD_WARNING("未经验证的玩家");
		return;
	}

	pto_LOGIN_C2A_REQ_GetName pto;
	pto.ParseFromArray(msg.body_c_str(), msg.body_size());

	pto_LOGIN_A2C_RES_GetName pto_res;
	pto_res.set_name(CNameLibrary::GetRandomName(pto.sex()));
	std::string str;
	pto_res.SerializeToString(&str);
	SendToClient(str, MSG_A2C, LOGIN_A2C_RES_GetName, msg.session());
}

bool CLoginServer::__VerifyUsername(const std::string& name)
{
	bool result = true;

	if (name.length() > kUsernameMaxLength)
		result = false;
	else if (false == std::regex_match(name, std::regex("[0-9A-Za-z_]+")))
		result = false;

	if (false == result)
		RECORD_WARNING(FormatString("用户帐号格式异常:", name.c_str()));

	return result;
}

bool CLoginServer::__VerifyLoginCode(const std::string& username, const std::string& time_, int server_id, bool is_adult, const std::string& code)
{
	std::ostringstream key;
	key << username << server_id << is_adult << time_;

	switch (login_server_info_->platform_id)
	{
	case 32:
		key << "kQOoX3s7zK";
		break;
	default:
		RECORD_ERROR(FormatString("未知的平台ID:", login_server_info_->platform_id));
		return false;
	}

	if (code != ToMD5(key.str()))
	{
		RECORD_WARNING("MD5不匹配");
		RECORD_TRACE(FormatString("客户端:", code, "\n服务器:", ToMD5(key.str())));
		return false;
	}
	else if (abs(time(0) - atoi(time_.c_str())) > 120)
	{
		RECORD_WARNING("时间不匹配");
		RECORD_TRACE(FormatString("客户端:", time_, "\n服务器:", time(0)));
		return false;
	}
	else
	{
		return true;
	}
}

void CLoginServer::__LoginSuccess(int pid, networkz::ISession* session)
{
	if (pid <= 0)
	{
		RECORD_ERROR(FormatString("pid错误:", pid));
		return;
	}

	pto_LOGIN_A2C_RES_Login pto;

	UserLoginInfo* user_info = static_cast<UserLoginInfo*>(session->argument());

	auto it = agent_servers_.find(user_info->first);

	std::string str;

	if (agent_servers_.cend() != it)
	{
		if (it->second.empty())
		{
			RECORD_FATAL("找不到Agent服务器");
			return;
		}
		const AgentServer* agent_info = it->second[GetRandom(SIZE_0, it->second.size() - 1)];
		if (!agent_info)
		{
			RECORD_FATAL("找不到Agent信息");
			return;
		}
		const char* agent_ip = SERVER_LIST()->GetHostIP(agent_info->host_id);

		pto_SYSTEM_L2A_NTF_ValidLogin pto_valid;
		pto_valid.set_ip(session->remote_ip());
		pto_valid.set_pid(pid);
		pto_valid.set_sid(user_info->first);
		pto_valid.SerializeToString(&str);
		SendToAgent(str, MSG_A2S, SYSTEM_L2A_NTF_ValidLogin, agent_ip, agent_info->udp_port);

		pto.set_result(LR_SUCCESS);
		pto.set_agent_ip(agent_ip);
		pto.set_agent_port(agent_info->tcp_port);
	}
	else
	{
		pto.set_result(LR_NOT_FIND_SERVER);
	}

	pto.set_pid(pid);
	pto.set_server_time(time(0));
	pto.SerializeToString(&str);
	SendToClient(str, MSG_A2C, LOGIN_A2C_RES_Login, session);

	lock_.lock();
	all_sessions_[session] = kInvalidConnectTimeout;
	lock_.unlock();
}

int  CLoginServer::__CheckPlayerName(const std::string& name, int sid)
{
	std::wstring wname = UTF8_to_UNICODE(name);

	std::wregex reg_base(L"[0-9A-Za-z_]");
	std::wregex reg_chinese(L"[\\u4e00-\\u9fa5]");
	std::wregex reg_japanese(L"[\\u3040-\\u309F\\u30A0-\\u30FF]");

	size_t length{ 0 };

	for (size_t i = 0; i < wname.length(); i++)
	{
		if (regex_match(wname.substr(i, 1), reg_chinese))
			length += 2;
		else if (regex_match(wname.substr(i, 1), reg_japanese))
			length += 2;
		else if (regex_match(wname.substr(i, 1), reg_base))
			length += 1;
		else
			return LR_ILLEGAL_CHARACTER;
	}

	if (length > kMaxPlayerNameLength || length < 2)
		return LR_LENGTH_ERROR;

	if (CIllegalWords::HasIllegalWords(name))
		return LR_SENSITIVE_WORD;

	std::ostringstream sql;
	sql << "select pid from tb_player where name = '" << name << "' and sid = " << sid;
	ResultSetPtr sql_result{ MySQLQuery(sql.str()) };
	if (!sql_result)
		return LR_CREATE_ERROR;
	if (sql_result->rowsCount() > 0)
		return LR_NAME_HAS_EXSIT;

	return LR_SUCCESS;
}
