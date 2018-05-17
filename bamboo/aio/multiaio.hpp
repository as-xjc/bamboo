#pragma once

#include <memory>
#include <vector>

#include <bamboo/aio/aioif.hpp>

namespace bamboo {
namespace aio {

/**
 * 多线程模式Aio
 *
 * @brief 基于CPU核心数量创建线程，并且分配同等数量的io。
 *        极端情况下，CPU核心数量为1时，退化成单线程模式
 *
 * @see bamboo::aio::AioIf
 */
class MultiAio : public AioIf {
 public:
  MultiAio();
  virtual ~MultiAio();

  std::size_t GetIoSize() override;
  boost::asio::io_context& GetIo(std::size_t index) override;
  boost::asio::io_context& GetMasterIo() override;

 protected:
  std::pair<boost::asio::io_context&, std::size_t> AllocateIo() override;
  void IoRun() override;
  void StopHandle() override;

 private:
  std::vector<std::unique_ptr<boost::asio::io_context>> ios_;
  using guard_t = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
  std::vector<std::unique_ptr<guard_t>> guards_;
  std::vector<std::thread> threads_;
  std::size_t index_{0};
};

}
}

