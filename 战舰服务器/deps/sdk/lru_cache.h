#pragma once
#include <boost/unordered_map.hpp>
#include <list>
#include <cstddef>

template <typename K, typename T>
class LRUCache {
 public:
  typedef T (*NewValue)(const K&);
  typedef void (*DeleteValue)(T);

  typedef std::pair<const K, T> ListValue;

  typedef std::list<ListValue> CacheList;
  typedef boost::unordered_map<K, typename CacheList::iterator> CacheMap;
  typedef typename CacheMap::iterator MapIterType;

  LRUCache(NewValue fn_new, DeleteValue fn_delete, size_t capacity_)
      : fn_new(fn_new), fn_delete(fn_delete), capacity(capacity_){};

  ~LRUCache() {
    for (MapIterType iter = map_.begin(); iter != map_.end(); ++iter) {
     fn_delete(iter->second->second);
    }
    this->map_.clear();
    this->list_.clear();
  }

  T& operator[](const K& key) { return get(key); }

  T& get(const K key) {
    MapIterType v = map_.find(key);
    if (v != map_.end()) {
      list_.splice(list_.begin(), list_, v->second);
      return v->second->second;
    }

    T data = fn_new(key);
    put(key, data);
    return list_.front().second;
  }

  void put(K key, T data) {
    if (capacity == list_.size()) eraseLru();

    MapIterType v = map_.find(key);
    if (v == map_.end()) {
      list_.push_front(ListValue(key, data));
      map_.insert(make_pair(key, list_.begin()));
    } else {
      v->second->second = data;
    }
  }

  void erase(K key) {
    MapIterType v = map_.find(key);
    if (v != map_.end()) {
      list_.erase(v->second);
      map_.erase(v);
    }
  }

 private:
  void eraseLru() {
    if (!list_.empty()) {
      ListValue& v = list_.back();
      map_.erase(v.first);
      list_.pop_back();
      fn_delete(v.second);
    }
  }

 private:
  const NewValue fn_new;
  const DeleteValue fn_delete;
  const size_t capacity;

  CacheMap map_;
  CacheList list_;
};
