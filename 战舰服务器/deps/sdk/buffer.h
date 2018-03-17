#pragma once
#include <cassert>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <noncopyable.h>

class Buffer : NonCopyable {
 public:
  Buffer(int init_size)
      : array_(NULL), write_(0), read_(0), size_(init_size) {
    array_ = static_cast<char *>(std::malloc(init_size));
  }

  ~Buffer() { std::free(array_); }

 public:
  size_t Append(const void *data, size_t size) {
    return Append(static_cast<const char *>(data), size);
  }

  size_t Append(const char *data, size_t size) {
    int need = size - WritableLength();
    if (need > 0) {
      array_ = static_cast<char *>(std::realloc(array_, this->size_ + need));
      assert(array_);
      this->size_ += need;
    }
    std::memcpy(array_ + write_, data, size);
    write_ += size;
    return size;
  }

  size_t Capacity() const { return size_; }

  size_t WritableLength() const { return size_ - write_; }

  size_t ReadableLength() const { return write_ - read_; }

  char *BeginWrite() const { return array_ + write_; }

  char *BeginRead() const { return array_ + read_; }

  void HasWritten(size_t size) { write_ += size; }

  void HasRead(size_t size) { read_ += size; }

  void Retrieve() {
    size_t len = ReadableLength();
    if (read_) {
      std::memmove(array_, BeginRead(), len);
    }
    read_ = 0;
    write_ = len;
  }

 private:
  char *array_;
  size_t write_;
  size_t read_;
  size_t size_;
};

