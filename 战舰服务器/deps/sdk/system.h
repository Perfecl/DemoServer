#pragma once
#include <common_define.h>
#include <algorithm>
#include <time.h>
#include <sys/time.h>
#include <string>

extern int time_offset;

//获取系统的当前秒数
inline time_t GetSeconds() { return time(NULL); }

//获取虚拟时间
inline time_t GetVirtualSeconds() { return time(NULL) + time_offset; }

//获取流逝时间
inline time_t GetClock() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec;
}

//获取系统当前的毫秒数
inline time_t GetMilliSeconds() {
  uint64_t time_millisecond;
  timeval time_val;
  gettimeofday(&time_val, NULL);
  time_millisecond = time_val.tv_sec * 1000 + time_val.tv_usec / 1000;
  return time_millisecond;
}

//first是秒数
//second是毫秒数
inline std::pair<time_t, time_t> GetCurrentTime() {
  timeval time_val;
  gettimeofday(&time_val, NULL);
  return std::make_pair(time_val.tv_sec, time_val.tv_usec / 1000);
}

//获取localtime
inline tm GetTime() {
  tm time_tm;
  time_t time_sec = GetCurrentTime().first;
  localtime_r(&time_sec, &time_tm);
  return time_tm;
}

//20091012
int32_t GetDateTimeShortFormat();

//2009-10-12_01
void FormatDateHour(char *array, const tm& t);
//2009-10-12
void FormatDate(char *array, const tm& t);

inline std::string GetDateStr() {
  std::string str;
  str.resize(11, 0);
  FormatDate(&str[0], GetTime());
  return str;
}

//获取两个时间戳相差的天数
inline int GetSecondsDiffDays(time_t s1, time_t s2) {
  tm tm_1;
  tm tm_2;
  localtime_r(&s1, &tm_1);
  localtime_r(&s2, &tm_2);
  tm_1.tm_sec = tm_1.tm_min = tm_1.tm_hour = 0;
  tm_2.tm_sec = tm_2.tm_min = tm_2.tm_hour = 0;
  s1 = mktime(&tm_1);
  s2 = mktime(&tm_2);
  return (s2 - s1) / (3600 * 24);
}

//判断两个时间戳是不是同一天
inline bool IsSameDay(time_t s1, time_t s2) {
  return GetSecondsDiffDays(s1, s2) == 0;
}

//获取文件最后修改的时间
time_t GetFileChangedTime(const char* file_name);

//睡眠,让出时间片
void Yield(int32_t millisec);

//创建守护进程
void Daemon();

//获取线程ID
int32_t GetThreadID();

//创建路径
void Mkdir(const char *path);

//创建唯一ID
//YY-ddd-hh-mm-ss-FFFF-SSSS
int64_t GenUniqueID(uint32_t server_id, int64_t id);

inline time_t GetZeroClock(time_t s1) {
  tm tm_1;
  localtime_r(&s1, &tm_1);
  tm_1.tm_sec = tm_1.tm_min = tm_1.tm_hour = 0;
  return mktime(&tm_1);
}

inline time_t MakeSundayTime(time_t s1) {
  tm tm_1;
  localtime_r(&s1, &tm_1);
  tm_1.tm_sec = tm_1.tm_min = tm_1.tm_hour = 0;
  return mktime(&tm_1) - tm_1.tm_wday * 24 * 3600;
}

//支持
//%Y-%m-%d %H:%M:%S
//%Y/%m/%d %H:%M:%S
int64_t GetSecondsFromString(const std::string& str);

inline time_t MakeDateTime(int year, int month, int day, int hour, int mintue,
                           int second) {
  tm time_map;
  time_map.tm_year = year - 1900;
  time_map.tm_mon = month - 1;
  time_map.tm_mday = day;
  time_map.tm_hour = hour;
  time_map.tm_min = mintue;
  time_map.tm_sec = second;
  return mktime(&time_map);
}
