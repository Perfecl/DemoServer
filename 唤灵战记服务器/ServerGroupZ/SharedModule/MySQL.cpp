#include "stdafx.h"
#include "MySQL.h"
#include "BasicServer.h"

using namespace sql;
using namespace mysql;

CMySQL::CMySQL() :
mysql_driver_{ get_mysql_driver_instance() }
{

}

CMySQL::~CMySQL()
{

}

bool CMySQL::Connect(const char* host, unsigned short port, const char* username, const char* passward, const char* db_name)
{
	try
	{
		mysql_connection_ = mysql_driver_->connect(FormatString(host, ":", port).c_str(), username, passward);
		mysql_connection_->setSchema(db_name);
		mysql_statement_ = mysql_connection_->createStatement();
		RECORD_INFO("连接数据库成功!");

		//防止时间过长导致connect断线
		Concurrency::create_task([this]()
		{
			while (true)
			{
				Sleep(1000 * 60 * 60);
				Query("select now()", 0);
			}
		});

		return true;
	}
	catch (SQLException ex)
	{
		RECORD_ERROR(ex.what());
		return false;
	}
}

std::shared_ptr<sql::PreparedStatement> CMySQL::CreatePrepareStatement(const char* sql, size_t length)
{
	if (0 == length)
		length = strlen(sql);

	try
	{
		sql::SQLString sql_str{ sql, length };

		LOCK_BLOCK(mysql_lock_);

		return std::move(std::shared_ptr<sql::PreparedStatement>(mysql_connection_->prepareStatement(sql_str)));
	}
	catch (SQLException ex)
	{
		RECORD_ERROR(FormatString(ex.what(), ",sql语句:", sql));
	}

	return nullptr;
}

std::shared_ptr<sql::ResultSet> CMySQL::Query(const char* sql, size_t length)
{
	if (0 == length)
		length = strlen(sql);

	try
	{
		sql::SQLString sql_str{ sql, length };
		LOCK_BLOCK(mysql_lock_);
		return std::move(std::shared_ptr<sql::ResultSet>(mysql_statement_->executeQuery(sql_str)));
	}
	catch (SQLException ex)
	{
		RECORD_ERROR(FormatString(ex.what(), ",sql语句:", sql));
	}

	return nullptr;
}

__int64 CMySQL::Update(const char* sql, size_t length)
{
	if (0 == length)
		length = strlen(sql);

	try
	{
		sql::SQLString sql_str{ sql, length };
		LOCK_BLOCK(mysql_lock_);
		return mysql_statement_->executeUpdate(sql_str);
	}
	catch (SQLException ex)
	{
		RECORD_ERROR(FormatString(ex.what(), ",sql语句:", sql));
	}

	return 0;
}

bool CMySQL::Execute(const char* sql, size_t length)
{
	if (0 == length)
		length = strlen(sql);

	try
	{
		sql::SQLString sql_str{ sql, length };
		LOCK_BLOCK(mysql_lock_);
		return mysql_statement_->execute(sql_str);
	}
	catch (SQLException ex)
	{
		RECORD_ERROR(FormatString(ex.what(), ",sql语句:", sql));
	}

	return false;
}