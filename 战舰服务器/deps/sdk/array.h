#pragma once
#include <cstddef>
#include <assert.h>

template <typename T, size_t N>
class Array {
 public:
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef pointer iterator;
  typedef const_pointer const_iterator;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  Array() : size_(0) {}

  iterator begin() { return &array_[0]; }
  const_iterator begin() const { return &array_[0]; }
  iterator end() { return &array_[size_]; }
  const_iterator end() const { return &array_[size_]; }

  void push_back(const_reference value) {
    assert(size_ < N);
    this->array_[size_] = value;
    ++size_;
  }

  reference at(size_t index) { return array_[index]; }
  const_reference at(size_t index) const { return array_[index]; }
  reference operator[](size_t index) { return array_[index]; }
  const_reference operator[](size_t index) const { return array_[index]; }

  void resize(size_t new_size, const_reference v = value_type()) {
    if (new_size > N) new_size = N;
    if (new_size < size_) size_ = new_size;
    else {
      while(size_ < new_size) this->push_back(v);
    }
  }

  void clear() { this->size_ = 0; }
  size_t size() const { return size_; }
  bool empty() const { return size() == 0; }
  bool full() const { return size() >= N; }

 private:
  size_t size_;
  value_type array_[N];
};

