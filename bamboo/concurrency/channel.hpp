#pragma once

#include <list>
#include <thread>
#include <mutex>

namespace bamboo {
namespace concurrency {

template <typename TYPE>
class Channel final {
 public:
  Channel() {}
  ~Channel() {}

  bool Empty() {
    std::lock_guard<std::mutex> l(lock_);
    return list_.empty();
  }

  void Push(const TYPE& type) {
    std::lock_guard<std::mutex> l(lock_);
    list_.push_back(type);
  }

  void Push(TYPE&& type) {
    std::lock_guard<std::mutex> l(lock_);
    list_.push_back(std::move(type));
  }

  std::size_t Size() {
    std::lock_guard<std::mutex> l(lock_);
    return list_.size();
  }

  bool TryPop(TYPE& type) {
    std::lock_guard<std::mutex> l(lock_);

    if (list_.empty()) return false;

    type = std::move(list_.front());
    list_.pop_front();
    return true;
  }

  void Pop(TYPE& type) {
    while (!TryPop(type)) {
      std::this_thread::yield();
    }
  }

 private:
  std::list<TYPE> list_;
  std::mutex lock_;
};

}
}
