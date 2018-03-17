#ifndef __STORAGE_EXT_H__
#define __STORAGE_EXT_H__
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>
#include <storage.h>
#include <vector_map.h>
#include <vector_set.h>
#include <string>

namespace storage_ext {

template <typename T1>
inline void Save(const std::string& key, const T1& value) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);

  const char* begin = reinterpret_cast<const char*>(&value);
  const char* end = begin + sizeof(value);

  storage::Set(key, std::string(begin, end));
}

template <typename T1>
inline void Load(const std::string& key, T1& value) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);

  const std::string& v = storage::Get(key);

  if (v.size()) {
#ifdef __DEBUG
    if (v.size() != sizeof(value)) assert(false && "length mismatch");
    return;
#endif
    if (v.size() == sizeof(value)) {
      const T1* ptr = reinterpret_cast<const T1*>(v.c_str());
      value = *ptr;
    }
  }
}

template <typename T1, typename T2>
inline void Save(const std::string& key,
                          const VectorMap<T1, T2>& map) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);
  BOOST_STATIC_ASSERT(boost::is_pod<T2>::value);

  const char* begin = reinterpret_cast<const char*>(&*map.begin());
  const char* end = begin + map.size() * sizeof(*map.begin());

  storage::Set(key, std::string(begin, end));
}

template <typename T1, typename T2>
inline void Load(const std::string& key, VectorMap<T1, T2>& map) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);
  BOOST_STATIC_ASSERT(boost::is_pod<T2>::value);

  const std::string& value = storage::Get(key);
  typedef typename VectorMap<T1, T2>::value_type value_type;
  const value_type* pair = reinterpret_cast<const value_type*>(value.c_str());
  map.reserve(value.size() / sizeof(value_type));
  for (size_t index = 0; index < value.size() / sizeof(value_type); ++index) {
    map.insert(std::make_pair(pair[index].first, pair[index].second));
  }
}

template <typename T1>
inline void Save(const std::string& key, const VectorSet<T1>& set) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);

  const char* begin = reinterpret_cast<const char*>(&*set.begin());
  const char* end = begin + set.size() * sizeof(*set.begin());

  storage::Set(key, std::string(begin, end));
}

template <typename T1>
inline void Load(const std::string& key, VectorSet<T1>& set) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);

  const std::string& value = storage::Get(key);
  typedef typename VectorSet<T1>::value_type value_type;
  const value_type* pair = reinterpret_cast<const value_type*>(value.c_str());
  for (size_t index = 0; index < value.size() / sizeof(value_type); ++index) {
    set.insert(pair[index]);
  }
}

template <typename T1>
inline void Save(const std::string& key, const std::vector<T1>& vec) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);

  const char* begin = reinterpret_cast<const char*>(&*vec.begin());
  const char* end = begin + vec.size() * sizeof(*vec.begin());

  storage::Set(key, std::string(begin, end));
}

template <typename T1>
inline void Load(const std::string& key, std::vector<T1>& vec) {
  BOOST_STATIC_ASSERT(boost::is_pod<T1>::value);

  const std::string& value = storage::Get(key);
  const T1* pair = reinterpret_cast<const T1*>(value.c_str());
  for (size_t index = 0; index < value.size() / sizeof(*vec.begin()); ++index) {
    vec.push_back(pair[index]);
  }
}

struct DeleteItemByPrefixFn : storage::ForEachCallback {
 public:
  DeleteItemByPrefixFn(const std::string& p) : prefix_(p) {}
  const std::string& prefix() const { return this->prefix_; }

  bool every(const storage::Slice& key, const storage::Slice& value) const {
    if (prefix_.length() >= sizeof(int64_t) &&
        *(int64_t*)key.data() != *(int64_t*)prefix_.data())
      return false;
    this->delete_items_.push_back(std::string(key.data(), key.size()));
    return true;
  }

  ~DeleteItemByPrefixFn() {
    for (std::vector<std::string>::const_iterator iter = this->delete_items_.begin();
         iter != this->delete_items_.end(); ++iter) {
      storage::Delete(*iter);
    }
    this->delete_items_.clear();
  }
 private:
  std::string prefix_;
  mutable std::vector<std::string> delete_items_;
};
}
#endif
