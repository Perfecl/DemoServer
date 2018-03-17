#include "stdafx.h"
#include "BasicServer.h"
#include "MySQL.h"
#include "..\SharedModule\LoggerZ\LoggerZ.h"

std::vector<std::string> CBasicServer::host_ipv4_;

CCriticalSectionZ CBasicServer::print_lock_;

CBasicServer::CBasicServer()
{

}

CBasicServer::~CBasicServer()
{
	is_server_close_ = true;

	networkz::Destroy();

	loggerz::DestroyLogger();

	delete mysql_;
}

void CBasicServer::WriteLog(std::string msg, LogLevel level, const char* file_name, int line)
{
	switch (level)
	{
	case LogLevel::Trace:
#ifdef _TRACE
		_PrintLine(msg.c_str());
#endif
		break;
	case LogLevel::Debug:
#ifdef _TEST
		_PrintLine(msg.c_str());
#endif
		break;
	case LogLevel::Info:
		loggerz::RecordInfo(msg.c_str());
		_PrintLine(msg.c_str());
		break;
	case LogLevel::Warning:
		loggerz::RecordWarning(msg.c_str());
		_PrintLine(msg.c_str());
		break;
	case LogLevel::Error:
		msg = FormatString(msg.c_str(), "\n", file_name, ",第", line, "行");
		loggerz::RecordError(msg.c_str());
		_PrintLine(msg.c_str());
		break;
	case LogLevel::Fatal:
		msg = FormatString(msg.c_str(), "\n", file_name, ",第", line, "行");
		loggerz::RecordFatal(msg.c_str());
		_PrintLine(msg.c_str());
		break;
	default:
		_PrintLine(FormatString("错误的日志类型:", static_cast<int>(level)).c_str());
		return;
	}
}

const std::vector<std::string>& CBasicServer::GetHostIPv4()
{
	if (host_ipv4_.empty())
	{
		const char** ip_address_{ nullptr };
		size_t		 ip_num{ 0 };

		networkz::GetHostIPv4(ip_address_, ip_num);

		for (size_t i = 0; i < ip_num; i++)
			host_ipv4_.push_back(ip_address_[i]);
	}

	return host_ipv4_;
}

int CBasicServer::GenerateServerID()
{
#ifdef _TEST
	std::string ip = GetHostIPv4()[0];
	ip.erase(0, ip.find_last_of(".") + 1);
	return atoi(ip.c_str());
#else
	_Print("请输入服务器ID:");
	int server_id{ 0 };
	std::cin >> server_id;
	return server_id;
#endif
}

void CBasicServer::InitTCPService(unsigned short port, size_t concurrence, bool is_print_error_info)
{
	networkz::InitTCPService(port, [](networkz::IOCallbackType type, networkz::ISession* session, int error_code, void* arg)
	{
		CBasicServer* svr = static_cast<CBasicServer*>(arg);

		if (svr)
		{
			switch (type)
			{
			case networkz::kConnect:
				svr->_OnConnect(session, error_code);
				break;
			case networkz::kAccept:
				svr->_OnAccept(session, error_code);
				break;
			case networkz::kReceive:
				svr->_OnReceive(session, error_code);
				break;
			case networkz::kRecycle:
				svr->_OnRecycle(session, error_code);
				break;
			default:
				RECORD_ERROR(FormatString("未知的IO类型", type));
				break;
			}
		}
		else
		{
			RECORD_ERROR("TCP回调函数参数空指针");
		}

	}, this, concurrence, is_print_error_info);

	RECORD_INFO(FormatString("开始监听", port, "端口..."));
}

void CBasicServer::InitUDPService(unsigned short port, size_t buffer_size, size_t concurrence, bool is_print_error_info)
{
	networkz::InitUDPService(port, buffer_size, [](const char* data, size_t length, const char* remote_ip, int error_code, void* arg)
	{
		CBasicServer* svr = static_cast<CBasicServer*>(arg);

		if (svr)
			svr->_OnUDPReceive(data, length, remote_ip, error_code);
		else
			RECORD_ERROR("UDP回调函数参数空指针");

	}, this, concurrence, is_print_error_info);
}

void CBasicServer::UDPSend(const char* msg, size_t length, const char* ip, unsigned short udp_port)
{
	networkz::UDPSend(msg, length, ip, udp_port);
}

void CBasicServer::PostConnect(const char* ip, unsigned short udp_port)
{
	networkz::PostConnect(ip, udp_port);
}

void CBasicServer::InitLoggerService(const char* path)
{
	loggerz::InitializeLogger(path);
}

bool CBasicServer::ConnectDatabase(const char* host, unsigned short port, const char* username, const char* passward, const char* db_name)
{
	if (!mysql_)
		mysql_ = new CMySQL;

	return mysql_->Connect(host, port, username, passward, db_name);
}

void CBasicServer::RunNetwork(size_t thread_num)
{
	networkz::Run(thread_num);
}

void CBasicServer::StartClock(int id, int interval)
{
	std::thread trd([this, id, interval]()
	{
		while (false == is_server_close_)
		{
			Sleep(interval);

			time_t now_time = time(0);
			tm local_time{ 0 };
			localtime_s(&local_time, &now_time);
			_OnClock(id, local_time);
		}
	});

	trd.detach();

	timer_threads_.push_back(std::move(trd));
}

ResultSetPtr CBasicServer::MySQLQuery(const std::string& sql)
{
	if (!mysql_)
	{
		RECORD_FATAL("请先连接数据库");
		return nullptr;
	}

	if (sql.empty())
	{
		RECORD_FATAL("SQL查询语句空指针");
		return nullptr;
	}

	return mysql_->Query(sql.c_str(), sql.length());
}

__int64	CBasicServer::MySQLUpdate(const std::string& sql)
{
	if (!mysql_)
	{
		RECORD_FATAL("请先连接数据库");
		return 0;
	}

	if (sql.empty())
	{
		RECORD_FATAL("SQL更新语句空指针");
		return 0;
	}

	return	mysql_->Update(sql.c_str(), sql.length());
}

bool CBasicServer::MySQLExecute(const std::string& sql)
{
	if (!mysql_)
	{
		RECORD_FATAL("请先连接数据库");
		return 0;
	}

	if (sql.empty())
	{
		RECORD_FATAL("SQL执行语句空指针");
		return false;
	}

	return mysql_->Execute(sql.c_str(), sql.length());
}

void CBasicServer::_OnReceive(networkz::ISession* session, int error_code)
{
	if (error_code)
		return;

	size_t data_length = session->data_length();
	const char* data = session->data(false);

	while (true)
	{
		if (data_length < CMessage::kHeaderSize)
			break;

		if (CMessage::HasHeaderFormString(data, data_length))
		{
			CMessage msg{ std::move(CMessage::FromString(data, data_length)) };

			if (!msg.IsEmpty())
			{
				msg.session(session);
				_OnProcessMessage(msg);
				session->erase_data(0, msg.length());
				data_length -= msg.length();
			}
			else
			{
				break;
			}
		}
		else
		{
			RECORD_TRACE("发现丢包");
			session->erase_data(0, 1);
			data_length--;
		}
	}
}

void CBasicServer::_OnProcessMessage(const CMessage& msg)
{
	switch (msg.GetType())
	{
	case MSG_C2S:_C2S(msg); break;
	case MSG_S2C:_S2C(msg); break;
	case MSG_A2S:_A2S(msg); break;
	case MSG_S2A:_S2A(msg); break;
	case MSG_C2A:_C2A(msg); break;
	case MSG_A2C:_A2C(msg); break;
	case MSG_S2B:_S2B(msg); break;
	case MSG_B2S:_B2S(msg); break;
	case MSG_L2A:_L2A(msg); break;
	case MSG_C2L:_C2L(msg); break;
	case MSG_L2C:_L2C(msg); break;
	default:RECORD_WARNING(FormatString("错误的消息类型:", msg.GetType())); break;
	}
}

void CBasicServer::_Print(const char* msg)
{
	print_lock_.lock();
	std::cout << msg;
	print_lock_.unlock();
}

void CBasicServer::_PrintLine(const char* msg)
{
	print_lock_.lock();
	std::cout << msg << std::endl;
	print_lock_.unlock();
}
