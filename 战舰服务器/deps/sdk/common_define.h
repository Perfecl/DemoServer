#pragma once
#include <stdint.h>
#include <limits.h>
#include <cstddef>

#define GAME_NAME                 "zhanjian"        //游戏名称

#define TOKEN_TTL                 (8*3600)          //口令超时时间

#define ONE_FRAME_TIME            (1000/50)         //主线程50帧
#define WORKER_FRAME_TIME         (1000/20)         //工作线程20帧
#define FLUSH_LOG_FRAME_TIME      (1000/2)          //刷新LOG的线程2帧
#define CS_MSG_MAX_LEN            (1*1024*1024)     //消息最大长度
#define SS_MSG_MAX_LEN            (2*1024*1024)     //服务器之间通讯消息最大长度
#define COMPRESS_MESSAGE_MIN_SIZE (128)             //开启压缩消息的最小长度
#define CSHEAD_LENGTH             (sizeof(CSHead))  //客户端服务器通讯之间的包头长度
#define SSHEAD_LENGTH             (sizeof(SSHead))  //服务器与服务器之间通讯的包头长度

#define CSHEAD_COMPRESS_FLAG      (1<<24)           //是否是压缩包
#define CSHEAD_ENCRYPT_FLAG       (1<<25)           //是否是加密包
#define MSG_LEN_MASK              ((~0u) >> 8)      //消息长度掩码

#define LOG_MAX_LEN               (64*1024)         //单行log的最大长度
#define LOG_PATH                  "./log"           //log的目录
#define BILL_PATH                 "./bill"          //bill的目录
#define IDLE_SOCK_TIME            (60*3)            //180s还不发消息,服务器就会T掉客户端
#define LOGIC_LRU_CACHE_SIZE      (1024)            //逻辑服务器LRU Cache的大小
#define RECORD_LRU_CACHE_SIZE     (4*1024)          //存档服务器LRU Cache的大小

#define __OUT__                                     //输出参数

#define CHECK_RET(EXPR) ({ int r = EXPR; if (r) return r; })

#ifdef _DEBUG
#define ENABLE_PRINT_SCREEN
#endif

enum EntryType {
  ENTRY_TYPE_NONE = 0,
  ENTRY_TYPE_PLAYER = 1,          //玩家
  ENTRY_TYPE_GM = 2,              //GM客户端
  ENTRY_TYPE_HTTP = 3,            //HTTP客户端

  ENTRY_TYPE_SERVER       = 10,   //发送给服务器的消息
  ENTRY_TYPE_LOGIC_SERVER = 11,   //逻辑服务器
  ENTRY_TYPE_RECORD_SERVER = 12,  //存档服务器
  ENTRY_TYPE_AUTH_SERVER = 13,    //认证服务器
  ENTRY_TYPE_CENTER_SERVER = 14,  //中心服务器
};

#pragma pack(1)

struct CSHead {
  uint32_t length;
  uint16_t msgid;
  uint16_t seq;

  //低24bit是有效长度
  uint32_t real_length() const {
    return length & MSG_LEN_MASK;
  }

  //是否是压缩包
  bool compress() const {
    return length & CSHEAD_COMPRESS_FLAG;
  }

  //是否是加密包
  bool encrypt() const {
    return length & CSHEAD_ENCRYPT_FLAG;
  }
};

//服务器与服务器通讯的包头
struct SSHead {
  uint32_t length;
  uint16_t msgid;
  uint8_t  dest_type; //消息去处(EntryType)
  uint64_t dest_id;

  uint32_t real_length() const {
    return length & MSG_LEN_MASK;
  }
};

#pragma pack()

template <typename T, int32_t N>
int32_t ArraySize(const T(&a)[N]) {
  return N;
}

//Murmur3Hash
uint32_t GetHashValue(const void* data, size_t len);
