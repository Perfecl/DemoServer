#ifndef __RADIX64_H__
#define __RADIX64_H__
#include <string>

class Radix64 {
 public:
  Radix64(std::string& v) : base64_(v) { Init(); }

  bool operator[](int index) const;
  bool Get(int index) const;
  void Set(int index, bool value);

  static int GetInt(char c);
  static char GetChar(int i);
 private:
  static void Init();
  void Resize(int index);
 private:
  enum { kRadix = 6 };
  static const char* kBase64;
  static char kMap2Int[128];

  std::string& base64_;
};

const char* Radix64::kBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
char Radix64::kMap2Int[128] = {0};

inline void Radix64::Init() {
  if (kMap2Int[0]) return;
  for (int i8 = 0; i8 < 64; ++i8) {
    kMap2Int[(int)kBase64[i8]] = i8;
  }
  kMap2Int[0] = 1;
}

inline int Radix64::GetInt(char c) {
  Radix64::Init();
  return Radix64::kMap2Int[(int)c];
}

inline char Radix64::GetChar(int i) {
  return Radix64::kBase64[i];
}

inline void Radix64::Resize(int index) {
  if (base64_.length() * kRadix > (unsigned)index) return;
  base64_.resize(index / kRadix + 1, GetChar(0));
}

inline void Radix64::Set(int index, bool v) {
  this->Resize(index);
  int div = index / kRadix;
  int mod = index % kRadix;
  int number = GetInt(base64_[div]);
  v ? number |= 1 << mod : number &= ~(1 << mod);
  base64_[div] = GetChar(number);
}

inline bool Radix64::Get(int index) const {
  if (base64_.length() * kRadix < (unsigned)index) return false;
  int div = index / kRadix;
  int mod = index % kRadix;
  int value = GetInt(base64_[div]);
  return value & (1 << mod);
}

inline bool Radix64::operator[](int index) const { return Get(index); }

#endif
