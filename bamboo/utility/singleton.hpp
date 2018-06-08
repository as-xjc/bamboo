#pragma once

#include <mutex>

namespace bamboo {
namespace utility {

template <typename T>
struct Singleton {
  Singleton() = delete;
  ~Singleton() = delete;

  Singleton(const Singleton&) = delete;
  Singleton& operator=(const Singleton&) = delete;

  Singleton(Singleton&&) = delete;
  Singleton& operator=(Singleton&&) = delete;

  static T* Instance() {
    static std::once_flag G_O;
    static T* G_T = nullptr;
    std::call_once(G_O, [&]() { G_T = new T(); });
    return G_T;
  }
};

}
}