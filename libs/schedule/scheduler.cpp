#include "bamboo/schedule/scheduler.hpp"

#include "bamboo/log/log.hpp"

namespace bamboo {
namespace schedule {

Scheduler::Scheduler(boost::asio::io_context& io) :io_(io) {}

Scheduler::~Scheduler() {}

Scheduler::ID Scheduler::GenId() {
  auto it = maps_.end();
  do {
    Scheduler::ID id = ++id_;
    if (id == 0) id = ++id_;
    it = maps_.find(id);
    if (it == maps_.end()) return id;
  } while(it != maps_.end());

  return 0;
}

Scheduler::ID Scheduler::RegisterTimeout(Handler&& handler, std::time_t millisecond, bool isCycle) {
  auto id = GenId();
  if (id == 0) {
    BB_ERROR_LOG("schedule id == 0 ???");
    return 0;
  }
  auto info = std::make_shared<ScheduleInfo>(io_);
  info->callback = std::move(handler);
  info->isCycle = isCycle;
  info->timeout = std::chrono::milliseconds(millisecond);
  info->wait.expires_after(info->timeout);

  info->waitHandler = [this, id](const boost::system::error_code& ec) {
    if (ec) {
      maps_.erase(id);
      BB_ERROR_LOG("schedule[%llu] error:%s", id, ec.message().c_str());
      return;
    }

    HandleTimeout(id, false);
  };

  maps_[id] = info;
  HandleTimeout(id, true);

  return id;
}

Scheduler::ID Scheduler::Timeout(Handler&& handler, std::time_t millisecond) {
  return RegisterTimeout(std::move(handler), millisecond, false);
}

Scheduler::ID Scheduler::Heartbeat(Handler&& handler, std::time_t millisecond) {
  return RegisterTimeout(std::move(handler), millisecond, true);
}

void Scheduler::Cancel(Scheduler::ID id) {
  auto it = maps_.find(id);
  if (it == maps_.end()) return;
  it->second->wait.cancel();

  maps_.erase(id);
}

void Scheduler::HandleTimeout(Scheduler::ID id, bool first) {
  auto it = maps_.find(id);
  if (it == maps_.end()) return;

  if (!first) {
    if (it->second->callback) {
      it->second->callback();
    }

    if (it->second->isCycle) {
      it->second->wait.expires_after(it->second->timeout);
    } else {
      maps_.erase(id);
      return;
    }
  }

  it->second->wait.async_wait(it->second->waitHandler);
}

}
}