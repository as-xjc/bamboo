#pragma once

#include <map>
#include <functional>
#include <bamboo/define.hpp>

namespace bamboo {
namespace schedule {

/**
 * 调度器
 */
class Scheduler final : public std::enable_shared_from_this<Scheduler> {
 public:
  /// 默认的构造函数
  Scheduler(boost::asio::io_context&);

  /// 默认的析构函数
  virtual ~Scheduler();

  /// 调度Id类型
  using ID = uint64_t;

  /// 回调类型
  using Handler = std::function<void()>;

  /**
   * 注册延迟调用函数
   * @param handler 回调函数
   * @param millisecond 延迟毫秒数
   * @return 调度ID
   */
  ID Timeout(Handler&& handler, std::time_t millisecond);

  /**
   * 注册循环调用函数
   * @param handler 回到函数
   * @param millisecond 循环毫秒数
   * @return 调度ID
   */
  ID Heartbeat(Handler&& handler, std::time_t millisecond);

  /// 取消调度
  void Cancel(ID id);

 private:
  /// 生成新ID
  ID GenId();
  boost::asio::io_context& io_;

  struct ScheduleInfo {
    bool isCycle{false};
    Handler callback;
    boost::asio::steady_timer wait;
    std::chrono::milliseconds timeout;
    std::function<void(const boost::system::error_code& ec)> waitHandler;
    ScheduleInfo(boost::asio::io_context& io) : wait(io) {}
  };

  void HandleTimeout(ID id, bool first);
  ID RegisterTimeout(Handler&&, std::time_t millisecond, bool isCycle);

  std::map<uint64_t, std::shared_ptr<ScheduleInfo>> maps_;
  ID id_{0};
};

}
}



