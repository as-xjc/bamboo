#include "bamboo/concurrency/taskpool.hpp"

namespace {
const std::size_t DEFAULT_CPU_MIN_SIZE = 1;
}

namespace bamboo {
namespace concurrency {

TaskPool::TaskPool() : TaskPool(std::thread::hardware_concurrency()) {
}

TaskPool::TaskPool(std::size_t size) {
  size = std::max(size, DEFAULT_CPU_MIN_SIZE);

  for (int i = 0; i < size; ++i) {
    threads_.emplace_back(&TaskPool::TaskMain, this);
  }
}

TaskPool::~TaskPool() {
  isRunning_ = false;
  cond_.notify_all();

  for (auto& thread : threads_) {
    thread.detach();
  }
}

void TaskPool::Add(bamboo::concurrency::TaskPool::Handler&& handle) {
  std::lock_guard<std::mutex> lock(mutex_);
  list_.push_back(std::move(handle));
  cond_.notify_one();
}

void TaskPool::TaskMain() {
  while (isRunning_.load()) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (list_.empty()) {
      cond_.wait(lock);
    } else {
      Handler h = std::move(list_.front());
      list_.pop_front();
      h();
      std::this_thread::yield();
    }
  }
}

}
}
