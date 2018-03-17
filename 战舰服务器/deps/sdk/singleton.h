#pragma once
#include <pthread.h>

template <typename T>
class Singleton {
 public:
  ~Singleton() {
    if (ptr) {
      delete ptr;
      ptr = NULL;
    }
  }
  static T& Instance() {
    pthread_once(&once, create_instance);
    return *ptr;
  }

 private:
  static void create_instance(void) { ptr = new T(); }

 private:
  static pthread_once_t once;
  static T* ptr;
};

template <typename T>
pthread_once_t Singleton<T>::once = PTHREAD_ONCE_INIT;

template <typename T>
T* Singleton<T>::ptr = NULL;
