#include "storage.h"
#include <system.h>
#include <leveldb/db.h>
#include <leveldb/cache.h>
#include <logger.h>

static leveldb::DB* db = NULL;

namespace storage {

void Init(const std::string& db_dir, int32_t lru_size) {
  std::string db_name;
  leveldb::Options options;
  db_name = db_dir;
  options.create_if_missing = true;
  //options.compression = leveldb::kNoCompression;
  if (lru_size&& !options.block_cache)
    options.block_cache = leveldb::NewLRUCache(lru_size);
  leveldb::Status status = leveldb::DB::Open(options, db_name.c_str(), &db);
  if (!status.ok()) {
    ERROR_LOG(logger)("Init LevelDB Fail");
  } else {
    db->CompactRange(NULL, NULL);
    TRACE_LOG(logger)("Init LevelDB Success");
  }
}

void UnInit() {
  if (db) delete db;
  db = NULL;
}

void ForEach(const ForEachCallback& callback) {
  const std::string& prefix = callback.prefix();
  leveldb::ReadOptions options;
  options.fill_cache = false;
  leveldb::Iterator* it = db->NewIterator(options);
  for (it->Seek(callback.prefix()); it->Valid(); it->Next()) {
    if (it->key().size() < prefix.size() ||
        memcmp(it->key().data(), prefix.c_str(), prefix.size()) != 0)
      break;
    if (!callback.every(it->key(), it->value())) break;
  }
  delete it;
}

void Set(const std::string& key, const std::string& value) {
  if (db) {
    //INFO_LOG(logger)("LevelDB Put:%s", key.c_str());
    leveldb::Status s = db->Put(leveldb::WriteOptions(), key, value);
    DEBUG_LOG(logger)("LevelDB Put:%s, Length:%lu, Success:%d", key.c_str(), value.size(), s.ok());
    if (!s.ok()) {
      ERROR_LOG(logger)("LevelDB Put Fail, %s", s.ToString().c_str());
    }
  }
}

std::string Get(const std::string& key) {
  std::string value;
  //INFO_LOG(logger)("LevelDB Get:%s", key.c_str());
  if (db) {
    leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
    if (!s.ok()) value.clear();
    DEBUG_LOG(logger)("LevelDB Get:%s, Success:%d", key.c_str(), s.ok());
  }
  return value;
}

void Delete(const std::string& key) {
  if (db) {
    //INFO_LOG(logger)("LevelDB Delete:%s", key.c_str());
    leveldb::Status s = db->Delete(leveldb::WriteOptions(), key);
    DEBUG_LOG(logger)("LevelDB Delete:%s, Success:%d", key.c_str(), s.ok());
  }
}

}
