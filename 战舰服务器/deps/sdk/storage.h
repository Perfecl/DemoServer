#ifndef __STORAGE_H__
#define __STORAGE_H__
#include <string>
#include <leveldb/slice.h>

namespace storage {
typedef leveldb::Slice Slice;

//必须是一个栈变量
struct ForEachCallback {
  virtual ~ForEachCallback() {}
  virtual const std::string& prefix() const = 0;
  //返回false, 中断遍历
  virtual bool every(const Slice& key, const Slice& value) const = 0;
};

void Init(const std::string& db_dir, int lru_size);
void UnInit();
void Set(const std::string& key, const std::string& value);
std::string Get(const std::string& key);
void Delete(const std::string& key);
void ForEach(const ForEachCallback& callback);
};
#endif
