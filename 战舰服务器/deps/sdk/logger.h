#pragma once
#include <common_define.h>
#include <stdarg.h>
#include <string>
#include <mutex.h>
#include <assert.h>
#include <boost/shared_ptr.hpp>

#define __SHORT_FILE__ \
  strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__

enum LoggerLevel {
  kLoggerLevel_Info = 0,
  kLoggerLevel_Debug = 1,
  kLoggerLevel_Trace = 2,
  kLoggerLevel_Warn = 3,
  kLoggerLevel_Error = 4,
  kLoggerLevel_Fatal = 5,
};

class LogFile;

class Logger {
 public:
  Logger(const char *file_name, const char *link_name, int32_t buffer_size);
  ~Logger();

  void ChangeLoggerFile(const char *new_file);
  void SetPrintScreen(bool b) { this->enable_std_cout_ = b; }

  void Debug(const char *pattern, ...)
      __attribute__((__format__(__printf__, 2, 3)));
  void Fatal(const char *pattern, ...)
      __attribute__((__format__(__printf__, 2, 3)));
  void Error(const char *pattern, ...)
      __attribute__((__format__(__printf__, 2, 3)));
  void Warn(const char *pattern, ...)
      __attribute__((__format__(__printf__, 2, 3)));
  void Info(const char *pattern, ...)
      __attribute__((__format__(__printf__, 2, 3)));
  void Trace(const char *pattern, ...)
      __attribute__((__format__(__printf__, 2, 3)));

  void VDebug(const char *prefix, const char *pattern, va_list ap)
      __attribute__((__format__(__printf__, 3, 0)));
  void VFatal(const char *prefix, const char *pattern, va_list ap)
      __attribute__((__format__(__printf__, 3, 0)));
  void VError(const char *prefix, const char *pattern, va_list ap)
      __attribute__((__format__(__printf__, 3, 0)));
  void VWarn(const char *prefix, const char *pattern, va_list ap)
      __attribute__((__format__(__printf__, 3, 0)));
  void VInfo(const char *prefix, const char *pattern, va_list ap)
      __attribute__((__format__(__printf__, 3, 0)));
  void VTrace(const char *prefix, const char *pattern, va_list ap)
      __attribute__((__format__(__printf__, 3, 0)));

  size_t GetWrittenSize() const { return size_; }

  void LogMessage(char *str, size_t len);
  void Flush();
 public:
  template <typename T>
  static Logger &InitDefaultLogger(const char *file_name, const char *link_name,
                                   int8_t log_level,
                                   int32_t buffer_size = 2 * 1024 * 1024) {
    static std::mutex kMutex;
    static Logger *__logger__ = NULL;
    std::lock_guard<std::mutex> guard(kMutex);
    if (!__logger__) {
      __logger__ = new Logger(file_name, link_name, buffer_size);
    } else {
      __logger__->ChangeLoggerFile(file_name);
    }
    __logger__->log_level(log_level);
    return *__logger__;
  }

  int8_t log_level() const { return log_level_; }
  void log_level(int8_t log_level) {
    assert(log_level >= kLoggerLevel_Info && log_level <= kLoggerLevel_Fatal);
    log_level_ = log_level;
  }

 private:
  void FormatMessage(int level, const char *pattern, va_list ap,
                     const char *prefix = NULL);
  void CreateLink();

 private:
  long long size_;
  int8_t log_level_;
  std::string file_name_;
  std::string link_name_;
  std::mutex mutex_;
  boost::shared_ptr<LogFile> file_;
  boost::shared_ptr<LogFile> backup_file_;
  bool enable_std_cout_;
  int32_t buffer_size_;
};

std::string FormatLogFileName(const char* path, const char *prefix);
std::string FormatLogFileNameWithoutHour(const char* path, const char *prefix);

//需要在主程序那边初始化一次
extern Logger* logger;

#define INFO_LOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Info) LOGGER->Info
#define DEBUG_LOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Debug) LOGGER->Debug
#define TRACE_LOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Trace) LOGGER->Trace
#define WARN_LOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Warn) LOGGER->Warn
#define ERROR_LOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Error) LOGGER->Error
#define FATAL_LOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Fatal) LOGGER->Fatal

#define INFO_VLOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Info) LOGGER->VInfo
#define DEBUG_VLOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Debug) LOGGER->VDebug
#define TRACE_VLOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Trace) LOGGER->VTrace
#define WARN_VLOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Warn) LOGGER->VWarn
#define ERROR_VLOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Error) LOGGER->VError
#define FATAL_VLOG(LOGGER) if (LOGGER && LOGGER->log_level() <= kLoggerLevel_Fatal) LOGGER->VFatal

