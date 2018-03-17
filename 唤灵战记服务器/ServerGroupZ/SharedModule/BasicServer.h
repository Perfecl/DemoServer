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
	enum class LogLevel{ Trace, Debug, Info, Warning, Error, Fatal };															//��־����

	static void WriteLog(std::string msg, LogLevel level, const char* file_name = nullptr, int line = 0);						//д��־

	static const std::vector<std::string>& GetHostIPv4();																		//��ȡ����IPv4

	static int  GenerateServerID();																								//���ɷ�����ID

protected:
	static std::vector<std::string> host_ipv4_;																					//����IPv4��ַ

	static CCriticalSectionZ print_lock_;																						//��ӡ�߳���

	static void _Print(const char* msg);																						//��ӡ
	static void _PrintLine(const char* msg);																					//��ӡ��

public:
	CBasicServer();
	~CBasicServer();

	void	InitTCPService(unsigned short port, size_t concurrence, bool is_print_error_info);									//��ʼ��TCP����
	void	InitUDPService(unsigned short port, size_t buffer_size, size_t concurrence, bool is_print_error_info);				//��ʼ��UDP����
	void	UDPSend(const char* msg, size_t length, const char* ip, unsigned short udp_port);									//����UDP��Ϣ
	void	PostConnect(const char* ip, unsigned short udp_port);																//������������
	void	RunNetwork(size_t thread_num);																						//��������
	void	StartClock(int id, int interval);																					//��ʼʱ��
	void	InitLoggerService(const char* path);																				//��ʼ����־����

	bool						ConnectDatabase(const char* host, unsigned short port, const char* username, const char* passward, const char* db_name);	//�������ݿ�
	ResultSetPtr				MySQLQuery(const std::string& sql);																//���ݿ��ѯ����
	__int64						MySQLUpdate(const std::string& sql);															//���ݿ���·���
	bool						MySQLExecute(const std::string& sql);															//���ݿ�ִ�з���

protected:
	std::vector<std::thread>					timer_threads_;																	//��ʱ���߳�

	bool										is_server_close_{ false };														//�������Ƿ�ر�

	CMySQL*										mysql_{ nullptr };																//MYSQLָ��

	virtual void _OnReceive(networkz::ISession* session, int error_code);														//��������Ϣʱ
	virtual void _OnConnect(networkz::ISession* session, int error_code){}														//����������ʱ
	virtual void _OnAccept(networkz::ISession* session, int error_code){}														//����������ʱ
	virtual void _OnRecycle(networkz::ISession* session, int error_code){}														//��IO����ʱ
	virtual void _OnUDPReceive(const char* data, size_t length, const char* remote_ip, int error_code){}						//��UDP������Ϣʱ
	virtual void _OnClock(int clock_id, const tm& now_datetime){};																//ʱ�ӻص�

	virtual void _OnProcessMessage(const CMessage& msg);																		//������Ϣ
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
