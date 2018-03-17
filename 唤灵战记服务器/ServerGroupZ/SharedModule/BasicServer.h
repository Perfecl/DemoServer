#pragma once
#include "Message.h"
#include "..\SharedModule\mysql\include\cppconn\resultset.h"
#include "..\SharedModule\mysql\include\cppconn\prepared_statement.h"

#define RECORD_TRACE(x)		CBasicServer::WriteLog((x),CBasicServer::LogLevel::Trace)
#define RECORD_DEBUG(x)		CBasicServer::WriteLog((x),CBasicServer::LogLevel::Debug)
#define RECORD_INFO(x)		CBasicServer::WriteLog((x),CBasicServer::LogLevel::Info)
#define RECORD_WARNING(x)	CBasicServer::WriteLog((x),CBasicServer::LogLevel::Warning)
#define RECORD_ERROR(x)		CBasicServer::WriteLog((x),CBasicServer::LogLevel::Error,__FILE__,__LINE__)
#define RECORD_FATAL(x)		CBasicServer::WriteLog((x),CBasicServer::LogLevel::Fatal,__FILE__,__LINE__)

typedef std::shared_ptr<sql::ResultSet>				ResultSetPtr;
typedef std::shared_ptr<sql::PreparedStatement>		PreparedStatementPtr;

class CMySQL;
class CBasicServer
{
public:
	enum class LogLevel{ Trace, Debug, Info, Warning, Error, Fatal };															//日志类型

	static void WriteLog(std::string msg, LogLevel level, const char* file_name = nullptr, int line = 0);						//写日志

	static const std::vector<std::string>& GetHostIPv4();																		//获取本机IPv4

	static int  GenerateServerID();																								//生成服务器ID

protected:
	static std::vector<std::string> host_ipv4_;																					//本机IPv4地址

	static CCriticalSectionZ print_lock_;																						//打印线程锁

	static void _Print(const char* msg);																						//打印
	static void _PrintLine(const char* msg);																					//打印行

public:
	CBasicServer();
	~CBasicServer();

	void	InitTCPService(unsigned short port, size_t concurrence, bool is_print_error_info);									//初始化TCP服务
	void	InitUDPService(unsigned short port, size_t buffer_size, size_t concurrence, bool is_print_error_info);				//初始化UDP服务
	void	UDPSend(const char* msg, size_t length, const char* ip, unsigned short udp_port);									//发送UDP消息
	void	PostConnect(const char* ip, unsigned short udp_port);																//发送连接请求
	void	RunNetwork(size_t thread_num);																						//运行网络
	void	StartClock(int id, int interval);																					//开始时钟
	void	InitLoggerService(const char* path);																				//初始化日志服务

	bool						ConnectDatabase(const char* host, unsigned short port, const char* username, const char* passward, const char* db_name);	//连接数据库
	ResultSetPtr				MySQLQuery(const std::string& sql);																//数据库查询方法
	__int64						MySQLUpdate(const std::string& sql);															//数据库更新方法
	bool						MySQLExecute(const std::string& sql);															//数据库执行方法

protected:
	std::vector<std::thread>					timer_threads_;																	//计时器线程

	bool										is_server_close_{ false };														//服务器是否关闭

	CMySQL*										mysql_{ nullptr };																//MYSQL指针

	virtual void _OnReceive(networkz::ISession* session, int error_code);														//当接收消息时
	virtual void _OnConnect(networkz::ISession* session, int error_code){}														//当连接请求时
	virtual void _OnAccept(networkz::ISession* session, int error_code){}														//当接受请求时
	virtual void _OnRecycle(networkz::ISession* session, int error_code){}														//当IO回收时
	virtual void _OnUDPReceive(const char* data, size_t length, const char* remote_ip, int error_code){}						//当UDP接收消息时
	virtual void _OnClock(int clock_id, const tm& now_datetime){};																//时钟回调

	virtual void _OnProcessMessage(const CMessage& msg);																		//处理消息
	virtual void _C2S(const CMessage& msg){}
	virtual void _S2C(const CMessage& msg){}
	virtual void _A2S(const CMessage& msg){}
	virtual void _S2A(const CMessage& msg){}
	virtual void _C2A(const CMessage& msg){}
	virtual void _A2C(const CMessage& msg){}
	virtual void _S2B(const CMessage& msg){}
	virtual void _B2S(const CMessage& msg){}
	virtual void _L2A(const CMessage& msg){}
	virtual void _C2L(const CMessage& msg){}
	virtual void _L2C(const CMessage& msg){}
};
