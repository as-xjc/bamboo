#pragma once

#include <list>
#include <future>
#include <memory>
#include <functional>

#include <boost/asio/io_context.hpp>
#include <boost/asio/local/stream_protocol.hpp>

namespace bamboo {
namespace concurrency {

class AsyncRun final {
 public:
  AsyncRun(boost::asio::io_context& io);
  ~AsyncRun();

  template <typename RETURN>
  void Run(std::function<RETURN()>&& call, std::function<void(RETURN)>&& cb) {
    auto f = std::async(std::launch::async, std::move(call));
    auto future = std::make_shared<std::future<RETURN>>(std::move(f));
    auto callcb = std::make_shared<std::function<void(RETURN)>>(std::move(cb));

    std::unique_ptr<AsyncInfo> info(new AsyncInfo);
    info->checker = [future]() -> bool {
      return future->wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    };

    info->callback = [callcb, future]() {
      (*callcb)(future->get());
    };

    sendLock_.lock();
    sendList_.push_back(std::move(info));
    sendLock_.unlock();
  };

  template <typename RETURN>
  void Run(RETURN(*call)(), void(*cb)(RETURN)) {
    Run(std::function<RETURN()>(call), std::function<void(RETURN)>(cb));
  }

  template <typename RETURN>
  void Run(std::function<RETURN()>&& call, void(*cb)(RETURN)) {
    Run(std::move(call), std::function<void(RETURN)>(cb));
  }

  template <typename RETURN>
  void Run(RETURN(*call)(), std::function<void(RETURN)>&& cb) {
    Run(std::function<RETURN()>(call), std::move(cb));
  }

 private:
  struct AsyncInfo {
    std::function<bool()> checker;
    std::function<void()> callback;
  };

  void MasterThread();
  void HandleDone();
  void DoRead();

  using AsyncInfoPtr = std::unique_ptr<AsyncInfo>;

  std::atomic_bool isRunning_{true};
  std::thread masterThread_;
  boost::asio::local::stream_protocol::socket sender_;
  boost::asio::local::stream_protocol::socket receiver_;
  std::array<char, 4> buffer_;

  std::list<AsyncInfoPtr> sendList_;
  std::mutex sendLock_;
  std::list<AsyncInfoPtr> checkList_;

  std::unique_ptr<std::list<AsyncInfoPtr>> pendingDoneList_;
  std::unique_ptr<std::list<AsyncInfoPtr>> doneList_;
  std::mutex doneLock_;
};

}
}