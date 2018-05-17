#include "bamboo/aio/multiaio.hpp"

#include <thread>

#include "bamboo/log/log.hpp"

namespace bamboo {
namespace aio {

MultiAio::MultiAio() {
  int cpu = std::thread::hardware_concurrency();
  cpu = std::max(cpu , 2);

  ios_.reserve(cpu);
  for (int i = 0; i < cpu; ++i) {
    ios_.emplace_back(new boost::asio::io_context);
  }
  guards_.reserve(cpu);
  for (int i = 0; i < cpu; ++i) {
    boost::asio::io_context& io = *ios_[i];
    guards_.emplace_back(new guard_t(boost::asio::make_work_guard(io)));
  }
  threads_.reserve(cpu - 1);

  BB_INFO_LOG("multi-aio init %d io_contexts", cpu);
}

MultiAio::~MultiAio() { Stop(); }

void MultiAio::IoRun() {
  for (int i = 0; i < ios_.size()-1; ++i) {
    boost::asio::io_context* io = ios_[i].get();
    threads_.emplace_back([io]() {
      for (;;) {
        try {
          io->run();
          break;
        } catch (std::exception& e) {
          BB_ERROR_LOG("catch exception:%s", e.what());
        }
      }
    });
  }

  for (;;) {
    try {
      GetMasterIo().run();
      break;
    } catch (std::exception& e) {
      BB_ERROR_LOG("catch exception:%s", e.what());
    }
  }
}

void MultiAio::StopHandle() {
  for (auto& ptr : ios_) {
    ptr->stop();
  }

  for (auto& thread : threads_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

std::size_t MultiAio::GetIoSize() {
  return ios_.size();
}

std::pair<boost::asio::io_context&, std::size_t> MultiAio::AllocateIo() {
  std::size_t index = index_++ % ios_.size();
  return std::make_pair(std::ref(*ios_[index]), index);
}

boost::asio::io_context& MultiAio::GetIo(std::size_t index) {
  return *ios_[index];
}

boost::asio::io_context& MultiAio::GetMasterIo() {
  return *(*ios_.rbegin());
}

}
}