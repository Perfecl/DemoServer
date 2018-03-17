#pragma once
#include <math.h>

#define M_PI 3.14159265358979323846
#define M_EXP 0.00001
#define M_EXP_1 (1.0 / M_EXP)

template <typename ValType>
class Vector2D_T {
 public:
  Vector2D_T() : x_(0), y_(0) {}
  Vector2D_T(ValType x, ValType y) : x_(x), y_(y) {}
  ValType X() const { return x_; }
  ValType Y() const { return y_; }
  void X(ValType x) { x_ = x; }
  void Y(ValType y) { y_ = y; }
  void Reset(ValType x = 0, ValType y = 0) {
    if (abs(x) < M_EXP) {
      x = 0;
    }
    if (abs(y) < M_EXP) {
      y = 0;
    }
    x_ = x;
    y_ = y;
  }
  void Plus(ValType xp, ValType yp) {
    x_ += xp;
    y_ += yp;
  }
  ValType Length() const {
    ValType len = sqrt(x_ * x_ + y_ * y_);
    return len;
  }
  Vector2D_T& operator+=(Vector2D_T& other) {
    x_ += other.x_;
    y_ += other.y_;
    return *this;
  }

  Vector2D_T& operator-=(Vector2D_T& other) {
    x_ -= other.x_;
    y_ -= other.y_;
    return *this;
  }
  Vector2D_T& operator*=(ValType times) {
    x_ *= times;
    y_ *= times;
    return *this;
  }

  Vector2D_T& operator/=(ValType times) {
    x_ /= times;
    y_ /= times;
    return *this;
  }
  inline Vector2D_T& operator+(Vector2D_T& other) {
    Vector2D_T v(x_, y_);
    return v += other;
  }
  inline Vector2D_T& operator-(Vector2D_T& other) {
    Vector2D_T v(x_, y_);
    return v -= other;
  }
  inline Vector2D_T& operator*(ValType times) {
    Vector2D_T v(x_, y_);
    return v *= times;
  }
  inline Vector2D_T& operator/(ValType times) {
    Vector2D_T v(x_, y_);
    return v /= times;
  }
  inline bool operator==(ValType times) {
    Vector2D_T v(x_, y_);
    return v == times;
  }

  ValType Distance(Vector2D_T& other) {
    ValType diff_x = other.x_ - x_;
    ValType diff_y = other.y_ - y_;
    ValType distance = sqrt(diff_x * diff_x + diff_y * diff_y) /**0.5*/;
    return distance;
  }
  template <typename DistanceType>
  DistanceType Distance(Vector2D_T& other) {
    DistanceType diff_x = other.x_ - x_;
    DistanceType diff_y = other.y_ - y_;
    DistanceType distance = sqrt(diff_x * diff_x + diff_y * diff_y) /**0.5*/;
    return distance;
  }

 private:
  ValType x_;
  ValType y_;
};

typedef Vector2D_T<float> Vector2D;
