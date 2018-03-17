#ifndef __LOG_FILE_H__
#define __LOG_FILE_H__
#include <boost/shared_ptr.hpp>
#include <noncopyable.h>
#include <memory>
#include <string>
#include <cstddef>
#include <stdio.h>

class LogFile : NonCopyable {
 public:
  enum {
    kLogFileCache = 32 * 1024,
  };

  LogFile(const std::string &file_name, int buffer_size = kLogFileCache);
  ~LogFile();

  int Write(const char *str, int len);
  void Flush();

  operator bool() const { return file_; }
  size_t offset() const { return offset_; }

 private:
  FILE* file_;
  char* buffer_;
  std::string file_name_;
  size_t offset_;
};

#endif
