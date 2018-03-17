#pragma once
#include <noncopyable.h>
#include <pthread.h>

//命名跟C++11的标准库一样
//以后升级的时候可以直接干掉这个头文件
//递归锁
namespace std {

class mutex : NonCopyable {
 public:
  friend class condition_variable;
  mutex() {
    pthread_mutexattr_init(&attr_);
    pthread_mutexattr_settype(&attr_, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex_, &attr_);
  }
  ~mutex() {
    pthread_mutexattr_destroy(&attr_);
    pthread_mutex_destroy(&mutex_);
  }

  void lock() { pthread_mutex_lock(&mutex_); }
  void unlock() { pthread_mutex_unlock(&mutex_); }

 private:
  pthread_mutexattr_t attr_;
  pthread_mutex_t mutex_;
};

template <typename T>
class lock_guard {
  typedef T Mutex;

 public:
  lock_guard(Mutex& mutex) : mutex_(mutex) { mutex_.lock(); }
  ~lock_guard() { mutex_.unlock(); }

 private:
  Mutex& mutex_;
};

class condition_variable : NonCopyable {
 public:
  condition_variable() { ::pthread_cond_init(&this->cond_, NULL); }
  ~condition_variable() { ::pthread_cond_destroy(&this->cond_); }

  void notify_one() { ::pthread_cond_signal(&this->cond_); }

  void notify_all() { ::pthread_cond_broadcast(&this->cond_); }

  void wait(std::mutex& lock) {
    ::pthread_cond_wait(&this->cond_, &lock.mutex_);
  }

  void wait_for(std::mutex& lock, int milliseconds) {
    struct timespec timeout;
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_nsec = (milliseconds % 1000) * 1000;
    ::pthread_cond_timedwait(&this->cond_, &lock.mutex_, &timeout);
  }

 private:
  pthread_cond_t cond_;
};
}
