#include "log_file.h"

LogFile::LogFile(const std::string& file_name, int buffer_size)
    : file_(fopen(file_name.c_str(), "a+")),
      buffer_(new char[buffer_size]),
      file_name_(file_name),
      offset_(0) {
  if (file_) {
    offset_ = fseek(file_, 0, SEEK_END);
    setvbuf(file_, buffer_, _IOFBF, buffer_size);
  }
}

LogFile::~LogFile() {
  fclose(this->file_);
  this->file_ = NULL;
  delete[] buffer_;
  buffer_ = NULL;
}

int LogFile::Write(const char* str, int len) {
  if (file_) {
    int size = fwrite(str, 1, len, file_);
    offset_ += size;
    return size;
  }
  return 0;
}

void LogFile::Flush() {
  if (file_) fflush(file_);
}
