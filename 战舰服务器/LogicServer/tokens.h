#pragma once
#include <common_define.h>
#include <system.h>
#include <noncopyable.h>
#include <boost/unordered_map.hpp>

class TokensSession : NonCopyable {
 public:
  enum {
    TOKEN_OK,
    TOKEN_ERROR,
    TOKEN_INVALID,
    TOKEN_TIMEOUT,
  };

 public:
  TokensSession();
  std::string MakeToken(const std::string& openid, int64_t uid, int64_t time);
  int32_t CheckToken(const std::string& token, int64_t& player_id);

 private:
  static const char token_key[17];

  int64_t token_uid_;
  boost::unordered_map<int64_t, int64_t> map_;

  uint64_t MurmurHash3_8(const void* key, const int len);
  uint64_t MakeTokenCode(const std::string& openid, int64_t uid, int64_t time,
                         int64_t tuid);
};

