#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <noncopyable.h>
#include <string>

template <int32_t N>
class ArrayStream : NonCopyable {
 public:
  ArrayStream() : offset_(0) { array_[0] = 0; }

  template <int32_t COUNT>
  void Append(const char (&ptr)[COUNT]) {
    ::memcpy(array_ + offset_, ptr, COUNT - 1);
    offset_ += COUNT - 1;
  }

  void Append(const char* pattern, ...)
      __attribute__((__format__(__printf__, 2, 3))) {
    va_list ap;
    va_start(ap, pattern);
    int32_t len = vsnprintf(array_ + offset_, N - offset_, pattern, ap);
    va_end(ap);
    if (len < 0 || offset_ + len > N) {
      offset_ = -1;
      return;
    }
    offset_ += len;
  }


  void Append(const std::string& str) {
    ::memcpy(array_ + offset_, str.c_str(), str.length());
    offset_ += str.length();
  }
  void Append(int64_t i) { this->Append("%ld", i); }
  void Append(int32_t i) { this->Append("%d", i); }
  void Append(int16_t i) { this->Append("%d", i); }
  void Append(int8_t i) { this->Append("%d", i); }

  void Append(uint64_t i) { this->Append("%lu", i); }
  void Append(uint32_t i) { this->Append("%u", i); }
  void Append(uint16_t i) { this->Append("%u", i); }
  void Append(uint8_t i) { this->Append("%u", i); }

  const char* c_str() const {
    return offset_ < N ? array_ : NULL;
  }

  std::string str() const {
    return offset_ < N ? std::string(&this->array_[0], &this->array_[offset_])
                       : std::string();
  }

  std::string str(int32_t pos, int32_t len) const {
    if (pos >= 0 && pos < offset_ && len + pos <= offset_) {
      return std::string(&this->array_[pos], &this->array_[pos + len]);
    }
    return std::string();
  }

  void clear() {
    array_[0] = 0;
    offset_ = 0;
  }

  size_t size() const { return offset_; }

 private:
  int32_t offset_;
  char array_[N + 1];
};

typedef ArrayStream<1024*2> DefaultArrayStream;
