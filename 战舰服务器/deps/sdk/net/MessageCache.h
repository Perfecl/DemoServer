#ifndef __MESSAGE_CACHE_H__
#define __MESSAGE_CACHE_H__
#include <singleton.h>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/shared_array.hpp>

class MessageCache : public Singleton<MessageCache> {
 public:
  typedef std::vector<char> RawMessage;
  void SendRawMessage(int64_t player_id, int32_t msgid, char* msg, int32_t length);
  void ClearRawMessage(int64_t player_id);
  std::vector<RawMessage>& GetCachedMessage(int64_t player_id);
  uint16_t GetSequence(int64_t player_id);
 private:
    boost::unordered_map<int64_t, std::vector<RawMessage> > cache_;
    //序列号
    boost::unordered_map<int64_t, uint16_t> sequence_;
};

#endif
