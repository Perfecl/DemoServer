#pragma once
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <common_define.h>
#include <string>
#include <noncopyable.h>
#include <singleton.h>

//策划配置文件的基类
class ConfigEntry : NonCopyable,
                    public boost::enable_shared_from_this<ConfigEntry> {
 public:
  typedef int64_t key_type;

  ConfigEntry() : uid_(0) {}
  virtual ~ConfigEntry() {}

  int64_t id() const { return uid_; }
 private:
  friend class XmlConfigFile;
  void set_id(int64_t uid) { const_cast<key_type&>(this->uid_) = uid; }

 private:
  const key_type uid_;
};

template <typename T>
class ConfigEntryManager : NonCopyable,
                           public Singleton<ConfigEntryManager<T> > {
 public:
  typedef ConfigEntry::key_type key_type;
  typedef boost::shared_ptr<T> pointer;

  const pointer& GetEntryByID(key_type id) {
    static pointer empty;

    typename ContainerType::iterator iter = this->map_.find(id);
    if (iter != this->map_.end()) return iter->second;

    return empty;
  }

  const pointer& GetEntryInCache(key_type id) {
    static pointer empty;

    typename ContainerType::iterator iter = this->reload_map_.find(id);
    if (iter != this->reload_map_.end()) {
      const pointer& ret = (this->map_[iter->first] = iter->second);
      this->reload_map_.erase(iter);
      return ret;
    }
    return empty;
  }

  bool AddEntry(key_type id, const pointer& entry) {
    typename ContainerType::iterator iter = this->map_.find(id);
    if (iter != this->map_.end()) {
      if (iter->second == entry) return true;
      return false;
    }
    this->map_[id] = entry;
    return true;
  }

  void lock() {
    reload_map_ = map_;
    map_.clear();
  }
  void unlock() { reload_map_.clear(); }

 private:
  typedef boost::unordered_map<key_type, pointer> ContainerType;
  ContainerType map_;
  ContainerType reload_map_;
};
