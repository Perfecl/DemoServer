#include "logger.h"
#include <system.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <boost/shared_ptr.hpp>
#include "log_file.h"

static char kMilliSecondsString[1024][4] = {};

static inline void LazyInitMilliSecondsString() {
  if (kMilliSecondsString[0][0]) return;
  for (int32_t i = 0; i < 1024; ++i) {
    uint32_t base = i % 1000;
    kMilliSecondsString[i][3] = "0123456789"[base % 10];
    kMilliSecondsString[i][2] = "0123456789"[base / 10 % 10];
    kMilliSecondsString[i][1] = "0123456789"[base / 100 % 10];
    kMilliSecondsString[i][0] = '.';
  }
}

Logger::Logger(const char *filename, const char *link_name, int32_t buffer_size)
    : size_(0),
      log_level_(kLoggerLevel_Info),
      file_name_(filename),
      buffer_size_(buffer_size) {
  LazyInitMilliSecondsString();
  if (link_name) link_name_ = link_name;
  file_.reset(new LogFile(file_name_, buffer_size_));
  assert(file_ && "open file error");
  size_ = file_->offset();
  CreateLink();
  this->enable_std_cout_ = false;
}

Logger::~Logger() {}

void Logger::Flush() {
  boost::shared_ptr<LogFile> f1 = this->file_;
  boost::shared_ptr<LogFile> f2 = this->backup_file_;
  if (f1) f1->Flush();
  if (f2) f2->Flush();
}

void Logger::CreateLink() {
  if (file_name_ != link_name_ && link_name_.size()) {
    remove(link_name_.c_str());
    std::string link = file_name_;
    std::string::size_type pos = link.find_last_of('/');
    if (pos != std::string::npos) {
      link = link.substr(pos + 1, link.size() - pos - 1);
    }
    symlink(link.c_str(), link_name_.c_str());
  }
}

void Logger::ChangeLoggerFile(const char *new_file_name) {
  if (file_name_ == new_file_name) return;
  std::lock_guard<std::mutex> guard(mutex_);
  LogFile* new_file = new LogFile(new_file_name, this->buffer_size_);
  if (new_file) {
    boost::shared_ptr<LogFile> close_file = this->backup_file_;
    boost::shared_ptr<LogFile> move_file = this->file_;
    this->file_.reset(new_file);
    size_ = this->file_->offset();
    this->backup_file_ = move_file;
    this->backup_file_->Flush();
    this->file_name_ = new_file_name;
    CreateLink();
  }
}

static const char *LOGGER_LEVEL[] = {"[INFO ]", "[DEBUG]", "[TRACE]",
                                     "[WARN ]", "[ERROR]", "[FATAL]"};
#define __FORMAT_MESSAGE__(level)    \
  va_list ap;                        \
  va_start(ap, pattern);             \
  FormatMessage(level, pattern, ap); \
  va_end(ap);

__thread time_t t_time = 0;
__thread char t_time_str[9] = {0};

inline void Logger::FormatMessage(int level, const char *pattern, va_list ap,
                                  const char *prefix /*= NULL*/) {
  int size = 0;
  char msg[LOG_MAX_LEN + 1];
  int msglen = LOG_MAX_LEN;

  std::pair<time_t, time_t> current_time = GetCurrentTime();
  if (current_time.first > t_time) {
    struct tm tm_now;
    localtime_r(&current_time.first, &tm_now);
    snprintf(t_time_str, sizeof(t_time_str), "%02d:%02d:%02d", tm_now.tm_hour,
             tm_now.tm_min, tm_now.tm_sec);
    t_time = current_time.first;
  }

  memcpy(msg + size, t_time_str, sizeof(t_time_str) - 1);
  size += sizeof(t_time_str) - 1;

  memcpy(msg + size, kMilliSecondsString[current_time.second % 1024],
         sizeof(uint32_t));
  size += sizeof(uint32_t);

  msg[size] = ' ';
  size += 1;

  if (level >= kLoggerLevel_Info && level <= kLoggerLevel_Fatal) {
    memcpy(msg + size, LOGGER_LEVEL[level], sizeof(/*LOGGER_LEVEL[0]*/uint64_t));
    size += sizeof(/*LOGGER_LEVEL[0]*/uint64_t) - 1;

    msg[size] = ' ';
    size += 1;
  }

  if (prefix) {
    //size += snprintf(msg + size, msglen - size, "%s ", prefix);
    int32_t l = strlen(prefix);
    memcpy(msg + size, prefix, l);
    size += l;
    msg[size] = ' ';
    size += 1;
  }
  size += vsnprintf(msg + size, msglen - size - 1, pattern, ap);

  if (level >= kLoggerLevel_Error && this->enable_std_cout_) {
    msg[size + 1] = 0;
    printf("%s\n", msg);
  }
  LogMessage(msg, size);
}

void Logger::Debug(const char *pattern, ...) {
  __FORMAT_MESSAGE__(kLoggerLevel_Debug);
}

void Logger::Fatal(const char *pattern, ...) {
  __FORMAT_MESSAGE__(kLoggerLevel_Fatal);
  abort();
}

void Logger::Error(const char *pattern, ...) {
  __FORMAT_MESSAGE__(kLoggerLevel_Error);
}

void Logger::Warn(const char *pattern, ...) {
  __FORMAT_MESSAGE__(kLoggerLevel_Warn);
}

void Logger::Info(const char *pattern, ...) {
  __FORMAT_MESSAGE__(kLoggerLevel_Info);
}

void Logger::Trace(const char *pattern, ...) {
  __FORMAT_MESSAGE__(kLoggerLevel_Trace);
}

void Logger::VDebug(const char *prefix, const char *pattern, va_list ap) {
  FormatMessage(kLoggerLevel_Debug, pattern, ap, prefix);
}

void Logger::VFatal(const char *prefix, const char *pattern, va_list ap) {
  FormatMessage(kLoggerLevel_Fatal, pattern, ap, prefix);
  abort();
}

void Logger::VError(const char *prefix, const char *pattern, va_list ap) {
  FormatMessage(kLoggerLevel_Error, pattern, ap, prefix);
}

void Logger::VWarn(const char *prefix, const char *pattern, va_list ap) {
  FormatMessage(kLoggerLevel_Warn, pattern, ap, prefix);
}

void Logger::VInfo(const char *prefix, const char *pattern, va_list ap) {
  FormatMessage(kLoggerLevel_Info, pattern, ap, prefix);
}

void Logger::VTrace(const char *prefix, const char *pattern, va_list ap) {
  FormatMessage(kLoggerLevel_Trace, pattern, ap, prefix);
}

void Logger::LogMessage(char *str, size_t len) {
  str[len] = '\n';
  this->file_->Write(str, len + 1);
  size_ += len;
}

std::string FormatLogFileName(const char *path, const char *prefix) {
  char log_date_str[14] = {0};
  FormatDateHour(log_date_str, GetTime());
  char log_file_name[256];
  snprintf(log_file_name, sizeof log_file_name, "%s/%s_%s.log", path, prefix,
           log_date_str);

  return log_file_name;
}

std::string FormatLogFileNameWithoutHour(const char *path, const char *prefix) {
  char log_date_str[14] = {0};
  FormatDate(log_date_str, GetTime());
  char log_file_name[256];
  snprintf(log_file_name, sizeof log_file_name, "%s/%s_%s.log", path, prefix,
           log_date_str);

  return log_file_name;
}

Logger* logger = NULL;
