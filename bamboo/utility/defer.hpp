#pragma once

#include <list>
#include <functional>

namespace bamboo {
namespace utility {

/**
 * 延迟类
 *
 * @brief 模拟 golang 的 defer，方便处理一些延迟操作
 */
class Defer final {
 public:
  Defer() {}
  ~Defer() { Done(); }

  Defer(const Defer& other) = delete;
  Defer& operator=(const Defer&) = delete;

  Defer(Defer&& other) = delete;
  Defer& operator=(Defer&& other) = delete;

  explicit Defer(std::function<void()>&& cb) {
    cbs_.push_back(std::move(cb));
  }

  void Add(std::function<void()>&& cb) {
    cbs_.push_back(std::move(cb));
  }

  void Done() {
    if (cbs_.empty()) return;

    for (auto it = cbs_.rbegin(); it != cbs_.rend() ; ++it) {
      if (*it) (*it)();
    }
    cbs_.clear();
  }

 private:
  std::list<std::function<void()>> cbs_;
};

}
}

#define DEFER(cmd) ::bamboo::utility::Defer ___simulate_go_defer___([&]() { cmd; })
#define DEFER_ADD(cmd) ___simulate_go_defer___.Add([&]() { cmd; })
#define DEFER_CLASS(cmd) ::bamboo::utility::::Defer ___simulate_go_defer_in_class___([&, this]() { cmd; })
#define DEFER_CLASS_ADD(cmd) ___simulate_go_defer_in_class___.Add([&, this]() { cmd; })
