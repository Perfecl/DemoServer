#include <sys/types.h>
#include <sys/stat.h>
#include <system.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <assert.h>
#include <string.h>

int time_offset = 0;

time_t GetFileChangedTime(const char* file_name) {
  struct stat file_stat;
  int32_t result = ::stat(file_name, &file_stat);

  if (-1 == result) {
    file_stat.st_mtim.tv_sec = 0;
  }

  return file_stat.st_mtim.tv_sec;
}

void Yield(int32_t millisec) {
  assert(millisec >= 0);
  ::usleep(millisec * 1000);
}

int32_t GetDateTimeShortFormat() {
  tm t = GetTime();
  return (t.tm_yday + 1900) * 10000 + (t.tm_mon + 1) * 100 + t.tm_mday;
}

void FormatDateHour(char *array, const tm& t) {
  sprintf(array, "%04d-%02d-%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
          t.tm_hour);
}

void FormatDate(char* array, const tm& t) {
  sprintf(array, "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
}

void Daemon() { daemon(1, 1); }

int32_t GetThreadID() {
  static __thread int32_t tid = 0;
  if (!tid) {
    tid = syscall(SYS_gettid);
  }
  return tid;
}

void Mkdir(const char* path) { mkdir(path, 0755); }

int64_t GenUniqueID(uint32_t server_id, int64_t old) {
  const struct tm& tm = GetTime();
  int64_t id = tm.tm_year % 100;
  id = id * 1000 + tm.tm_yday + 1;
  id = id * 100 + tm.tm_hour;
  id = id * 100 + tm.tm_min;
  id = id * 100 + tm.tm_sec;
  id = id * 10000 + server_id % 10000;
  id = id * 10000;
  int32_t sequence = old % 10000;
  id = id + (sequence > 9000 ? 0 : sequence);
  return id;
}

//%Y-%m-%d %H:%M:%S
//%Y/%m/%d %H:%M:%S
int64_t GetSecondsFromString(const std::string& str) {
  struct tm t;
  memset(&t, 0, sizeof(t));
  if (sscanf(str.c_str(), "%d-%d-%d %d:%d:%d", &t.tm_year, &t.tm_mon,
             &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) >= 3 ||
      sscanf(str.c_str(), "%d/%d/%d %d:%d:%d", &t.tm_year, &t.tm_mon,
             &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) >= 3) {
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    return mktime(&t);
  }
  return 0;
}
