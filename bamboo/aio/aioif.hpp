#pragma once

#include <memory>
#include <vector>

#include <boost/asio/signal_set.hpp>

#include <bamboo/define.hpp>
#include <bamboo/server/serverif.hpp>
#include <bamboo/distributed/registry.hpp>

namespace bamboo {
namespace aio {

/**
 * Aio基类
 */
class AioIf {
 public:

  /// 默认构造函数
  AioIf();

  /// 默认析构函数
  virtual ~AioIf();

  /**
   * 创建服务
   *
   * @brief aio会为每个服务分配一个io。
   *        单线程模式下，每个服务处于同一个io
   *        多线程模式下，会有多个io，每个服务仅存在一个io上
   *
   * @tparam SERVER 服务类，必须派生于 bamboo::server::ServerIf
   * @tparam ARGS 参数类型
   * @param name 服务名称
   * @param args 服务的构造函数参数
   * @return <对象的共享指针, Io索引>
   */
  template<typename SERVER, typename... ARGS>
  std::pair<std::shared_ptr<SERVER>, std::size_t> CreateServer(std::string name, ARGS&& ... args) {
    static_assert(std::is_base_of<bamboo::server::ServerIf, SERVER>::value,
                  "server must be base of bamboo::server::ServerIf");

    std::shared_ptr<SERVER> ptr;
    auto allocateio = AllocateIo();
    try {
      /**
       * @note  AioIf is friend class of ServerIf, so can not use std::make_shared
       */
      ptr.reset(new SERVER(allocateio.first, name, allocateio.second, std::forward<ARGS>(args)...));
    } catch (std::exception& e) {
      BB_ERROR_LOG("create server[%s] fail:%s", name.c_str(), e.what());
      return {};
    }
    servers_.push_back(std::dynamic_pointer_cast<bamboo::server::ServerIf>(ptr));
    return std::make_pair(ptr, allocateio.second);
  }

  /**
   * 指定io索引创建服务
   *
   * @see CreateServer
   */
  template<typename SERVER, typename... ARGS>
  std::pair<std::shared_ptr<SERVER>, std::size_t> CreateServerWithIndex(std::size_t ioIndex, std::string name, ARGS&& ... args) {
    static_assert(std::is_base_of<bamboo::server::ServerIf, SERVER>::value,
                  "server must be base of bamboo::server::ServerIf");

    std::shared_ptr<SERVER> ptr;
    if (ioIndex >= GetIoSize()) ioIndex = 0;
    auto& allocateio = GetIo(ioIndex);
    try {
      /**
       * @note  AioIf is friend class of ServerIf, so can not use std::make_shared
       */
      ptr.reset(new SERVER(allocateio, name, ioIndex, std::forward<ARGS>(args)...));
    } catch (std::exception& e) {
      BB_ERROR_LOG("create server[%s] fail:%s", name.c_str(), e.what());
      return {};
    }
    servers_.push_back(std::dynamic_pointer_cast<bamboo::server::ServerIf>(ptr));
    return std::make_pair(ptr, ioIndex);
  }

  /**
   * 批量创建服务
   *
   * @see CreateServer
   */
  template<typename SERVER, typename... ARGS>
  void BatchCreateServer(std::size_t size, std::string name, ARGS&& ... args) {
    for (std::size_t i = 0; i < size; ++i) {
      CreateServer<SERVER>(name, std::forward<ARGS>(args)...);
    }
  }

  /**
   * 遍历所有服务，执行处理函数
   * @param handler 处理函数
   */
  virtual void ForeachServer(std::function<void(bamboo::server::ServerPtr&)>&& handler) final {
    for (auto& server : servers_) {
      handler(server);
    }
  }

  /**
   * 执行所有服务的配置接口
   * @param map 配置数据
   */
  virtual void Configure(boost::program_options::variables_map& map) final;

  /**
   * 注册信号处理
   * @param signal 信号
   * @param handler 信号处理函数
   */
  virtual void Signal(int signal, std::function<void(int)>&& handler) final;

  /**
   * 创建服务发现功能
   * @return 对象的共享指针
   */
  virtual std::shared_ptr<bamboo::distributed::Registry> InitRegistry() final;

  /// 返回服务发现对象
  virtual std::shared_ptr<bamboo::distributed::Registry> GetRegistry() final;

  /// 启动所有服务
  virtual void Start() final;

  /// 关闭所有服务
  virtual void Stop() final;

  /// 获取io数量
  virtual std::size_t GetIoSize() = 0;

  /// 根据索引获取io
  virtual boost::asio::io_context& GetIo(std::size_t index) = 0;

  /// 获取主io
  virtual boost::asio::io_context& GetMasterIo() = 0;

 protected:

  /// 为服务分配io
  virtual std::pair<boost::asio::io_context&, std::size_t> AllocateIo() = 0;

  /// io 启动处理
  virtual void IoRun() = 0;

  /// io 暂停处理
  virtual void StopHandle() = 0;

  std::vector<bamboo::server::ServerPtr> servers_;
  std::shared_ptr<bamboo::distributed::Registry> registry_;

  struct SignalInfo {
    boost::asio::signal_set signal;
    std::function<void(const boost::system::error_code&, int)> handler;
    SignalInfo(boost::asio::io_context& io, int signal) : signal(io, signal) {}
  };
  std::map<int, std::shared_ptr<SignalInfo>> signals_;
};

using AioPtr = std::shared_ptr<AioIf>;

}
}