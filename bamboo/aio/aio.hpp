#pragma once

#include <bamboo/aio/aioif.hpp>

namespace bamboo {
namespace aio {

/**
 * 单线程模式Aio，只有一个io
 *
 * @see bamboo::aio::AioIf
 */
class Aio : public AioIf {
 public:
  Aio();
  virtual ~Aio();

  std::size_t GetIoSize() override;
  boost::asio::io_context& GetIo(std::size_t index) override;
  boost::asio::io_context& GetMasterIo() override;

 protected:
  std::pair<boost::asio::io_context&, std::size_t> AllocateIo() override;
  void IoRun() override;
  void StopHandle() override;

 private:
  boost::asio::io_context io_;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard_;
};

}
}

