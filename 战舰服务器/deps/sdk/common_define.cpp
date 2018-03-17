#include "common_define.h"
#include "murmur3.h"

const uint32_t hash_seed = 0;

uint32_t GetHashValue(const void* data, size_t len) {
  uint32_t hash_value = 0;
  MurmurHash3_x86_32(data, len, hash_seed, &hash_value);
  return hash_value;
}
