#include "tokens.h"
#include "murmur3.h"
#include <cpp/message.pb.h>
#include "server.h"
#include <logger.h>

TokensSession::TokensSession()
{
  token_uid_ = GetSeconds();
  token_uid_ <<= 31;
}

const char TokensSession::token_key[17] = "qmdS4lizsIOKlsw2";

uint64_t TokensSession::MurmurHash3_8(const void* key, const int len) {
  char temp[17] = {0};
  MurmurHash3_x64_128(key, len, 0, temp);
  return *((uint64_t*)temp);
}

uint64_t TokensSession::MakeTokenCode(const std::string& openid, int64_t uid,
                                      int64_t time, int64_t tuid) {
  char temp[512] = {0};
  snprintf(temp, sizeof(temp), "%s,%s,%ld,%ld,%ld", token_key, openid.c_str(),
           uid, time, tuid);
  return MurmurHash3_8(temp, strlen(temp));
}

std::string TokensSession::MakeToken(const std::string& openid, int64_t uid,
                                     int64_t time) {
  ++token_uid_;
  char str[1024] = {0};
  snprintf(str, sizeof(str), "%016lx,%s,%ld,%ld,%ld",
           MakeTokenCode(openid, uid, time, token_uid_), openid.c_str(), uid,
           time, token_uid_);
  map_[uid] = token_uid_;
  return str;
}

int32_t TokensSession::CheckToken(const std::string& token,
                                  int64_t& player_id) {
  if (token.length() <= 18) return sy::ERR_TOKEN_INVALID;

  char temp_token[512] = {0};
  strcpy(temp_token, token.c_str());

  uint64_t code1 = 0;
  sscanf(temp_token, "%lx", &code1);

  memcpy(temp_token, token_key, 16);

  uint64_t code2 = MurmurHash3_8(temp_token, strlen(temp_token));

  if (code1 == code2) {
    int64_t uid = 0;
    int64_t time = 0;
    int64_t tuid = 0;
    char openid[64] = {0};
    char temp_code[17] = {0};
    sscanf(temp_token, "%[^,],%[^,],%ld,%ld,%ld", temp_code, openid, &uid,
           &time, &tuid);

    boost::unordered_map<int64_t, int64_t>::iterator it = map_.find(uid);
    if (it != map_.end() && it->second > tuid) {
      TRACE_LOG(logger)("tuid expect:%ld, real;%ld", it->second, tuid);
      return sy::ERR_TOKEN_TIMEOUT;
    }

    if (GetSeconds() >= time) {
      TRACE_LOG(logger)("CurrentSeconds:%ld, time:%ld", GetSeconds(), time);
      return sy::ERR_TOKEN_TIMEOUT;
    }

    player_id = uid;
    return sy::ERR_OK;
  } else {
    TRACE_LOG(logger)("token:{%s}, temp token:{%s}", token.c_str(), temp_token);
    TRACE_LOG(logger)("Code1:%lu, Code2:%lu", code1, code2);
  }

  return sy::ERR_TOKEN_INVALID;
}
