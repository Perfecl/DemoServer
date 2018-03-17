#ifndef __BLOCKING_QUEUE_H__
#define __BLOCKING_QUEUE_H__
#include <mutex.h>
#include <stdint.h>
#include <vector>
#include <noncopyable.h>
#include <deque>

template <class T>
class MessageQueue : NonCopyable {
 public:
  typedef T value_type;
  typedef std::vector<value_type> container_type;
  typedef std::mutex Mutex;
  typedef std::lock_guard<Mutex> LockGuard;

 public:
  bool Push(const value_type& v) {
    LockGuard guard(mutex_);
    queue_.push_back(v);
    return true;
  }

  bool Pop(value_type& v) {
    LockGuard guard(mutex_);
    if (!queue_.empty()) {
      v = queue_.front();
      queue_.pop_front();
      return true;
    }
    queue_.clear();
    return false;
  }

  bool Pop(container_type& v) {
    LockGuard guard(mutex_);
    if (!queue_.empty()) {
      std::swap(queue_, v);
      queue_.clear();
      return true;
    }
    return false;
  }

  uint64_t Size() const {
    LockGuard guard(mutex_);
    return queue_.size();
  }

 private:
  container_type queue_;
  Mutex mutex_;
};

template <class T>
class BlockingMessageQueue : NonCopyable {
 public:
  typedef T value_type;
  typedef std::deque<value_type> container_type;
  typedef std::mutex Mutex;
  typedef std::lock_guard<Mutex> LockGuard;
  typedef std::condition_variable Condition;

 public:
  bool Push(const value_type& v) {
    LockGuard guard(mutex_);
    queue_.push_back(v);
    cond_.notify_one();
    return true;
  }

  bool Pop(value_type& v) {
    LockGuard guard(mutex_);
    while (queue_.empty()) {
      cond_.wait(this->mutex_);
    }
    v = queue_.front();
    queue_.pop_front();
    return true;
  }

  bool Pop(container_type& v) {
    LockGuard guard(mutex_);
    while (queue_.empty()) {
      cond_.wait(this->mutex_);
    }
    std::swap(queue_, v);
    queue_.clear();
    return true;
  }

  void PopWithoutWait(container_type& v) {
    std::swap(queue_, v);
  }

  uint64_t Size() const {
    return queue_.size();
  }

 private:
  container_type queue_;
  Mutex mutex_;
  Condition cond_;
};

#endif
