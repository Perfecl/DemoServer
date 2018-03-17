#pragma once
#include <vector>
#include <system.h>
#include <string>
#include <boost/unordered_map.hpp>
#include <noncopyable.h>

class Player : NonCopyable {
 public:
  Player(int64_t uid) : uid_(uid) {}
  virtual ~Player() {}

 public:
  int64_t uid() const { return uid_; }
  const std::string& account() const { return account_; }
  void account(const std::string& account) { this->account_ = account; }
  //上次活跃时间,方便被T掉
  time_t last_active_time() const { return last_active_time_; }
  virtual void active() { this->last_active_time_ = GetSeconds(); }
  //是否可以删除
  virtual bool can_be_delete() = 0;

 private:
  const int64_t uid_;
  std::string account_;
  time_t last_active_time_;
};

template <typename T>
class PlayerContainer : NonCopyable {
 public:
  typedef T value_type;
  typedef value_type* value_pointer;
  friend class Server;

  PlayerContainer() {}
  ~PlayerContainer() {
    for (typename container_type::iterator iter = this->key_container_.begin();
         iter != this->key_container_.end(); ++iter) {
      DeleteValue(iter->second);
    }
    this->key_container_.clear();
  }

  //只会从当前缓存内找玩家,找不到返回NULL
  value_pointer GetPlayerByID(int64_t uid) {
   typename container_type::iterator iter = this->key_container_.find(uid);
   return iter != this->key_container_.end() ? iter->second : NULL;
  }

  void Erase(int64_t uid) {
    typename container_type::iterator iter = this->key_container_.find(uid);
    if (iter != this->key_container_.end()) {
      value_pointer player = iter->second;
      this->key_container_.erase(iter);
      DeleteValue(player);
    }
  }

  //从缓存里面找到玩家,如果找不到就构造一个
  value_pointer GetOrNewPlayer(int64_t uid) {
   typename container_type::iterator iter = this->key_container_.find(uid);
   if (iter != this->key_container_.end()) return iter->second;

   value_pointer pointer = NewValue(uid);
   this->key_container_[uid] = pointer;
   return pointer;
  }

  //!!!不能做增加/删除动作!!!
  template <typename Fn>
  void ForEach(const Fn& fn) const {
    for (typename container_type::const_iterator iter = this->key_container_.begin();
         iter != this->key_container_.end(); ++iter) {
      fn(iter->second);
    }
  }

  void EraseTimeOutPlayer() {
    std::vector<int64_t> vec;
    for (typename container_type::iterator iter = this->key_container_.begin();
         iter != this->key_container_.end(); ++iter) {
      if (iter->second->can_be_delete())
        vec.push_back(iter->first);
    }
    for (size_t i = 0; i < vec.size(); ++i) {
      this->Erase(vec[i]);
    }
  }

  int32_t size() const { return this->key_container_.size(); }

 private:
  static value_pointer NewValue(const int64_t& uid) { return new value_type(uid); }
  static void DeleteValue(value_pointer pointer) { delete pointer; }

 private:
  typedef boost::unordered_map<int64_t, value_pointer> container_type;

  container_type key_container_;
};
