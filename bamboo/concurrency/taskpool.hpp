#pragma once

#include <thread>
#include <list>
#include <vector>

namespace bamboo {
namespace concurrency {

class TaskPool final {
 public:
  TaskPool();
  TaskPool(std::size_t size);
  ~TaskPool();

  using Handler = std::function<void()>;
  void Add(Handler&& handle);

 private:
  void TaskMain();

  std::list<Handler> list_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::vector<std::thread> threads_;
  std::atomic_bool isRunning_{true};
};

}
}

