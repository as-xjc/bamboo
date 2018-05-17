#include "bamboo/aio/aio.hpp"

namespace bamboo {
namespace aio {

Aio::Aio(): guard_(boost::asio::make_work_guard(io_)) {}

Aio::~Aio() { Stop(); }

std::pair<boost::asio::io_context&, std::size_t> Aio::AllocateIo() {
  return std::make_pair(std::ref(io_), 0);
}

boost::asio::io_context& Aio::GetIo(std::size_t index) {
  return io_;
}

boost::asio::io_context& Aio::GetMasterIo() {
  return io_;
}

void Aio::IoRun() {
  for (;;) {
    try {
      io_.run();
      break;
    } catch (std::exception& e) {
      BB_ERROR_LOG("catch exception:%s", e.what());
    }
  }
}

std::size_t Aio::GetIoSize() {
  return 1;
}

void Aio::StopHandle() {
  io_.stop();
}

}
}