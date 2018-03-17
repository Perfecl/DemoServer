#include "MessageCache.h"

void MessageCache::SendRawMessage(int64_t player_id, int32_t msgid, char* msg,
                                  int32_t length) {
  std::vector<RawMessage>& vec = this->cache_[player_id];
  vec.push_back(RawMessage());
  vec.back().assign(msg, msg + length);
}

void MessageCache::ClearRawMessage(int64_t player_id) {
  this->cache_[player_id].clear();
}

std::vector<MessageCache::RawMessage>& MessageCache::GetCachedMessage(int64_t player_id) {
  return this->cache_[player_id];
}

uint16_t MessageCache::GetSequence(int64_t player_id) {
  return this->sequence_[player_id]++;
}
