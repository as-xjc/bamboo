#include "bamboo/concurrency/asyncrun.hpp"

#include <boost/asio/local/connect_pair.hpp>

namespace bamboo {
namespace concurrency {

AsyncRun::AsyncRun(boost::asio::io_context& io) : sender_(io),
                                                  receiver_(io) {

  boost::asio::local::connect_pair(sender_, receiver_);

  pendingDoneList_.reset(new std::list<AsyncInfoPtr>);
  doneList_.reset(new std::list<AsyncInfoPtr>);

  masterThread_ = std::thread(&AsyncRun::MasterThread, this);
  DoRead();
}

AsyncRun::~AsyncRun() {
  isRunning_ = false;
  masterThread_.join();
}

void AsyncRun::MasterThread() {
  while (isRunning_.load()) {
    {
      std::lock_guard<std::mutex> lock(sendLock_);
      while (!sendList_.empty()) {
        checkList_.push_back(std::move(sendList_.front()));
        sendList_.pop_front();
      }
    }

    std::size_t done = 0;
    for (auto it = checkList_.begin(); it != checkList_.end();) {
      if ((*it)->checker()) {
        doneLock_.lock();
        pendingDoneList_->push_back(std::move(*it));
        doneLock_.unlock();
        it = checkList_.erase(it);
        done += 1;
      } else {
        ++it;
      }
    }
    if (done > 0) {
      static const char c = 'i';
      sender_.write_some(boost::asio::buffer(&c, 1));
    }
    std::this_thread::yield();
  }
}

void AsyncRun::HandleDone() {
  if (!doneList_->empty()) {
    for (auto& ptr : *doneList_) {
      ptr->callback();
    }
    doneList_->clear();
  }

  doneLock_.lock();
  pendingDoneList_.swap(doneList_);
  doneLock_.unlock();

  for (auto& ptr : *doneList_) {
    ptr->callback();
  }

  doneList_->clear();
}

void AsyncRun::DoRead() {
  receiver_.async_read_some(boost::asio::buffer(buffer_.data(), buffer_.size()),
                             [this](const boost::system::error_code& ec, std::size_t rd) {
                               HandleDone();
                               DoRead();
                             });
}

}
}