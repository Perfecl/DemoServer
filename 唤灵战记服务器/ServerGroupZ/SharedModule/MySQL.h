#pragma once
#include <mysql_driver.h>
#include <cppconn\exception.h>
#include <cppconn\statement.h>

class CMySQL
{
public:
	CMySQL();
	~CMySQL();

	bool Connect(const char* host, unsigned short port, const char* username, const char* passward, const char* db_name);

	std::shared_ptr<sql::PreparedStatement> CreatePrepareStatement(const char* sql, size_t length);
	std::shared_ptr<sql::ResultSet>			Query(const char* sql, size_t length);
	__int64									Update(const char* sql, size_t length);
	bool									Execute(const char* sql, size_t length);

private:
	sql::mysql::MySQL_Driver* const 	mysql_driver_;						//mysql数据库驱动
	sql::Connection*					mysql_connection_{ nullptr };		//mysql数据库连接
	sql::Statement*						mysql_statement_;					//mysql数据库报表
	CCriticalSectionZ					mysql_lock_;						//mysql线程锁
};

