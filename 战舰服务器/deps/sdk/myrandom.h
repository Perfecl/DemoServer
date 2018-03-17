#pragma once
#include <vector>
#include <algorithm>

//获取下一个伪随机数
//[0, 9999]
int32_t GetNextRandom(int32_t seeds);

struct RandomNubmer {
  RandomNubmer(int32_t& seeds) : seeds(seeds) {}

  static int32_t min() { return 0; }
  static int32_t max() { return 9999; }

  //获取下一个随机数
  int32_t operator()() const {
    int32_t num = GetNextRandom(seeds);
    ++seeds;
    return num;
  }

  double next() const {
    return double((*this)()) / max();
  }

  //给定区间[0.0, 1.0]
  //判断是否成功
  bool success(double num) const {
    return (*this)() <= (num * max());
  }

  //给定区间[MIN, MAX]随机一个数字
  int32_t rand(int32_t min, int32_t max) const {
    int32_t r = min + next() * (max - min);
    return r;
  }

  int32_t& seeds;
};

//全开全闭的随机数
//[begin, end]
int32_t RandomBetween(int32_t begin, int32_t end);

inline int32_t RandomIn10000() { return RandomBetween(0, 10000 - 1); }
