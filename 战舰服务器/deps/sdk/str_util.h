#pragma once
#include <assert.h>
#include <slice.h>
#include <vector>
#include <string>
#include <array_stream.h>

inline void SplitString(const std::string& s, std::vector<std::string>& v,
                        const std::string& c) {
  std::string::size_type pos1, pos2;
  pos2 = s.find(c);
  pos1 = 0;
  while (std::string::npos != pos2) {
    v.push_back(s.substr(pos1, pos2 - pos1));

    pos1 = pos2 + c.size();
    pos2 = s.find(c, pos1);
  }
  if (pos1 != s.length()) v.push_back(s.substr(pos1));
}

//void Callback(Slice s);
template <typename Fn>
inline void ForEachString(StringSlice s, StringSlice split, Fn fn) {
  size_t pos1, pos2;
  pos2 = s.find(split);
  pos1 = 0;
  while (std::string::npos != pos2) {
    fn(s.substr(pos1, pos2 - pos1));

    pos1 = pos2 + split.size();
    pos2 = s.find(split, pos1);
  }
  if (pos1 != s.length()) {
    fn(s.substr(pos1));
  }
}

//base64编解码
std::string encode64(const std::string& str);
std::string decode64(const std::string& str);

//获取MD5值
std::string GetMD5(const void* begin, size_t len);
std::string GetMD5(const std::string& str);

//{prefix}:{type}
template <typename P1, typename T1>
static inline std::string MakeKVStorageKey(const P1& prefix, const T1& type) {
  DefaultArrayStream stream;
  stream.Append(prefix);
  stream.Append(":");
  stream.Append(type);
  return stream.str();
}

//{prefix}:{type}:{key1}
template <typename P1, typename T1, typename K1>
static inline std::string MakeKVStorageKey(const P1& prefix, const T1& type,
                                    const K1& key1) {
  DefaultArrayStream stream;
  stream.Append(prefix);
  stream.Append(":");
  stream.Append(type);
  stream.Append(":");
  stream.Append(key1);
  return stream.str();
}

//{prefix}:{type}:{key1}:{key2}
template <typename P1, typename T1, typename K1, typename K2>
static inline std::string MakeKVStorageKey(const P1& prefix, const T1& type,
                                           const K1& key1, const K2& key2) {
  DefaultArrayStream stream;
  stream.Append(prefix);
  stream.Append(":");
  stream.Append(type);
  stream.Append(":");
  stream.Append(key1);
  stream.Append(":");
  stream.Append(key2);
  return stream.str();
}

//{prefix}:{type}:{key1}:{key2}:{key3}
template <typename P1, typename T1, typename K1, typename K2, typename K3>
static inline std::string MakeKVStorageKey(const P1& prefix, const T1& type,
                                           const K1& key1, const K2& key2,
                                           const K3& key3) {
  DefaultArrayStream stream;
  stream.Append(prefix);
  stream.Append(":");
  stream.Append(type);
  stream.Append(":");
  stream.Append(key1);
  stream.Append(":");
  stream.Append(key2);
  stream.Append(":");
  stream.Append(key3);
  return stream.str();
}

