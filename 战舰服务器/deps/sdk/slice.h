#pragma once
#include <string>
#include <string.h>
#include <cstddef>

template <typename T = char>
class Slice {
 public:
  typedef T value_type;
  typedef const value_type* const_iterator;

  static size_t npos;

 public:
  Slice(const value_type* data, size_t length) : data_(data), length_(length) {}

  Slice(const std::basic_string<value_type>& str)
      : data_(str.data()), length_(str.size()) {}

  template <int N>
  Slice(const value_type (&array)[N])
      : data_(array), length_(N - 1) {}

  template <typename Iter>
  Slice(Iter begin, Iter end)
      : data_(&*begin), length_(end - begin) {}

  Slice(const Slice& slice) : data_(slice.data_), length_(slice.length_) {}

  ~Slice() {}

  const value_type& operator[](size_t pos) const {
    assert(pos < length_);
    return data_[pos];
  }

  size_t find(const Slice& slice, size_t pos = 0) {
    if (this->size() - pos <= 0) return npos;
    for (const_iterator iter = this->begin() + pos; iter != this->end();
         ++iter) {
      if (end() - iter < (long)slice.size()) return npos;
      if (!memcmp(iter, slice.begin(), sizeof(value_type) * slice.size())) {
        return iter - begin();
      }
    }
    return npos;
  }

  Slice substr(size_t begin, size_t count = Slice::npos) {
    count = count == Slice::npos ? this->size() - begin : count;
    return Slice(this->begin() + begin, count);
  }

  size_t count_if(value_type v) const {
    size_t count = 0;
    for (const_iterator iter = this->begin(); iter != this->end(); ++iter) {
      if (*iter == v) count += 1;
    }
    return count;
  }

  const_iterator begin() const { return data_; }
  const_iterator end() const { return data_ + length_; }
  const_iterator data() const { return data_; }
  size_t length() const { return length_; }
  size_t size() const { return length(); }

 private:
  const value_type* data_;
  size_t length_;
};

template <typename T>
size_t Slice<T>::npos = -1;

typedef Slice<char> StringSlice;
