#ifndef __MULTI_THREAD_H__
#define __MULTI_THREAD_H__
#include <boost/atomic.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <mutex.h>
#include <noncopyable.h>

template <typename K, typename V, typename Hash = boost::hash<K>,
          typename Equal = std::equal_to<K> >
class ConcurrentHashMap {
 public:
  typedef boost::unordered_map<K, V, Hash, Equal> container_type;
  typedef typename container_type::value_type value_type;
  typedef typename container_type::iterator iterator;

  ConcurrentHashMap() {}
  ~ConcurrentHashMap() {}

  bool Get(const K& key, V& value) {
    std::lock_guard<Mutex> m(this->mutex_);
    iterator iter = container_.find(key);
    if (iter != container_.end()) {
      value = iter->second;
      return true;
    }
    return false;
  }

  bool Set(const K& key, const V& value) {
    std::lock_guard<Mutex> m(this->mutex_);
    iterator iter = container_.find(key);
    if (iter != container_.end())
      return false;
    container_[key] = value;
    return true;
  }

 private:
  typedef std::mutex Mutex;
  Mutex mutex_;
  container_type container_;
};

class GlobalCounter : NonCopyable {
 public:
  typedef boost::shared_ptr<boost::atomic_int64_t> Value;
  typedef ConcurrentHashMap<int64_t, Value> Container;

  bool ResetNewValue(int64_t key, int64_t old_value, int64_t new_value) {
    boost::atomic_int64_t& v = *GetValue(key);
    return v.compare_exchange_weak(old_value, new_value);
  }

  int64_t Get(int64_t key) {
    boost::atomic_int64_t& v = *GetValue(key);
    return v;
  }
  int64_t GetAndInc(int64_t key) {
    boost::atomic_int64_t& v = *GetValue(key);
    return v++;
  }

 private:
  Value& GetValue(int64_t key) {
    //会有内存泄漏
    static __thread Container::container_type* cache;
    if (!cache) cache = new Container::container_type();

    if (cache->find(key) == cache->end()) {
      Value v = Value(new boost::atomic_int64_t(0));
      container_.Set(key, v);
      container_.Get(key, v);
      cache->operator[](key) = v;
    }
    return cache->operator[](key);
  }

 private:
  Container container_;
};

#endif
