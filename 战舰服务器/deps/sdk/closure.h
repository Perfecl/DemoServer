#pragma once
#include <stdint.h>
#include <map>
#include <singleton.h>
#include <system.h>
#include <noncopyable.h>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/unordered_map.hpp>

class Closure : NonCopyable, public boost::enable_shared_from_this<Closure> {
 public:
  Closure();
  virtual ~Closure() {}

  virtual void Run() = 0;
  virtual void TimeOut() = 0;

  int64_t id() const { return this->id_; }
  int64_t time() const { return this->time_; }

 private:
  const int64_t id_;
  const int64_t time_;
};

typedef boost::shared_ptr<Closure> ClosurePtr;

class ClosureManager : NonCopyable, public Singleton<ClosureManager> {
 public:
  ClosureManager();
  //move语义,只能获取一次
  ClosurePtr GetClosure(int64_t id);

  template <typename T>
  void AddClosure(const boost::shared_ptr<T>& ptr, int64_t time_out = 5000) {
    ClosurePtr p = boost::const_pointer_cast<Closure>(ptr);
    this->closures_[p->id()] = p;
    this->time_out_queue_.insert(
        std::make_pair(GetMilliSeconds() + time_out, p->id()));
  }

  void Update();

  int64_t NewID();
 private:
  int64_t id_seeds_;

  std::multimap<int64_t, int64_t> time_out_queue_;
  boost::unordered_map<int64_t, ClosurePtr> closures_;
};
