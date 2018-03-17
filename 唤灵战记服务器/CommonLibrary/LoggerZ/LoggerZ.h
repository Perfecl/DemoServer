#pragma once

#ifdef LOGGERZ_EXPORTS
#define LOGGERZ_API __declspec(dllexport)
#else
#define LOGGERZ_API __declspec(dllimport)
#endif

namespace loggerz
{
	extern "C" LOGGERZ_API void InitializeLogger(const char* file_path);

	extern "C" LOGGERZ_API void DestroyLogger();

	extern "C" LOGGERZ_API void RecordTrace(const char* msg);

	extern "C" LOGGERZ_API void RecordDebug(const char* msg);

	extern "C" LOGGERZ_API void RecordInfo(const char* msg);

	extern "C" LOGGERZ_API void RecordWarning(const char* msg);

	extern "C" LOGGERZ_API void RecordError(const char* msg);

	extern "C" LOGGERZ_API void RecordFatal(const char* msg);
}
