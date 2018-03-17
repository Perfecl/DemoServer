#include "stdafx.h"
#include "LoggerZ.h"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
using namespace logging::trivial;

namespace loggerz
{
	static char file_path_[256]{0};

	static bool is_init_{ false };

	static src::severity_logger<severity_level> lg;

	static CRITICAL_SECTION lock;

	LOGGERZ_API void InitializeLogger(const char* file_path)
	{
		if (is_init_)
		{
			printf("初始化失败:Logger已经初始化\n");
			return;
		}

		if (nullptr == file_path)
		{
			printf("初始化失败:文件路径空指针\n");
			return;
		}

		InitializeCriticalSectionAndSpinCount(&lock, 4000);

		memset(file_path_, 0, sizeof(file_path_));

		strcpy_s(file_path_, file_path);

		logging::add_file_log(
			keywords::file_name = file_path_,
			keywords::rotation_size = 10 * 1024 * 1024,
			keywords::format = (expr::stream << "[" << logging::trivial::severity << "] " << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S ") << expr::smessage));

		logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);

		logging::add_common_attributes();

		is_init_ = true;
	}

	LOGGERZ_API void DestroyLogger()
	{
		DeleteCriticalSection(&lock);

		is_init_ = false;
	}

	LOGGERZ_API void RecordTrace(const char* msg)
	{
		if (!is_init_)
			return;

		EnterCriticalSection(&lock);

		if (msg)
			BOOST_LOG_SEV(lg, severity_level::trace) << msg;
		else
			printf("日志消息空指针\n");

		LeaveCriticalSection(&lock);
	}

	LOGGERZ_API void RecordDebug(const char* msg)
	{
		if (!is_init_)
			return;

		EnterCriticalSection(&lock);

		if (msg)
			BOOST_LOG_SEV(lg, severity_level::debug) << msg;
		else
			printf("日志消息空指针\n");

		LeaveCriticalSection(&lock);
	}

	LOGGERZ_API void RecordInfo(const char* msg)
	{
		if (!is_init_)
			return;

		EnterCriticalSection(&lock);

		if (msg)
			BOOST_LOG_SEV(lg, severity_level::info) << msg;
		else
			printf("日志消息空指针\n");

		LeaveCriticalSection(&lock);
	}

	LOGGERZ_API void RecordWarning(const char* msg)
	{
		if (!is_init_)
			return;

		EnterCriticalSection(&lock);

		if (msg)
			BOOST_LOG_SEV(lg, severity_level::warning) << msg;
		else
			printf("日志消息空指针\n");

		LeaveCriticalSection(&lock);
	}

	LOGGERZ_API void RecordError(const char* msg)
	{
		if (!is_init_)
			return;

		EnterCriticalSection(&lock);

		if (msg)
			BOOST_LOG_SEV(lg, severity_level::error) << msg;
		else
			printf("日志消息空指针\n");

		LeaveCriticalSection(&lock);
	}

	LOGGERZ_API void RecordFatal(const char* msg)
	{
		if (!is_init_)
			return;

		EnterCriticalSection(&lock);

		if (msg)
			BOOST_LOG_SEV(lg, severity_level::fatal) << msg;
		else
			printf("日志消息空指针\n");

		LeaveCriticalSection(&lock);
	}
}
