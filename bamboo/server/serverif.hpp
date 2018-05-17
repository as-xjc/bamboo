#pragma once

#include <memory>
#include <vector>

#include <boost/program_options.hpp>

#include <bamboo/define.hpp>
#include <bamboo/net/acceptorif.hpp>
#include <bamboo/net/connectorif.hpp>
#include <bamboo/schedule/scheduler.hpp>

namespace bamboo {

namespace aio {
class AioIf;
}

namespace server {

/**
 * 服务基类
 */
class ServerIf {
 public:
  /// 默认析构函数
  virtual ~ServerIf();

  /// 获取服务名称
  virtual const std::string& GetName() const final;

  /// 服务启动函数
  virtual bool Start() final;

  /// 服务关闭函数
  virtual void Stop() final;

  /// 服务所处的io索引
  virtual std::size_t GetIoIndex() const final;

  /// 配置接口
  virtual void Configure(boost::program_options::variables_map&) = 0;

  /**
   * 创建监听助手类
   * @tparam ACCEPTOR 监听助手类，必须派生于 bamboo::net::AcceptorIf
   * @tparam ARGS 参数类型
   * @param address 监听地址
   * @param port 监听端口
   * @param args 构造函数参数
   * @return 对象的共享指针
   */
  template<typename ACCEPTOR, typename... ARGS>
  std::shared_ptr<ACCEPTOR> CreateAcceptor(std::string address, uint16_t port, ARGS&& ... args) {
    static_assert(std::is_base_of<bamboo::net::AcceptorIf, ACCEPTOR>::value,
                  "acceptor must be base of bamboo::net::AcceptorIf");

    std::shared_ptr<ACCEPTOR> ptr;
    try {
      ptr.reset(new ACCEPTOR(io_, std::forward<ARGS>(args)...));
      ptr->SetAddress(std::move(address), port);
    } catch (std::exception& e) {
      BB_ERROR_LOG("CreateAcceptor fail:%s", e.what());
      return nullptr;
    }

    acceptors_.push_back(std::dynamic_pointer_cast<bamboo::net::AcceptorIf>(ptr));

    return ptr;
  }

  /**
   * 创建链接助手类
   * @tparam CONNECTOR 连接助手类，必须派生于 bamboo::net::ConnectorIf
   * @tparam ARGS 参数类型
   * @param args 构造函数参数
   * @return 对象的共享指针
   */
  template<typename CONNECTOR, typename... ARGS>
  std::shared_ptr<CONNECTOR> CreateConnector(ARGS&& ... args) {
    static_assert(std::is_base_of<bamboo::net::ConnectorIf, CONNECTOR>::value,
                  "connector must be base of bamboo::net::ConnectorIf");

    std::shared_ptr<CONNECTOR> ptr;
    try {
      ptr.reset(new CONNECTOR(io_, std::forward<ARGS>(args)...));
    } catch (std::exception& e) {
      BB_ERROR_LOG("CreateConnector fail:%s", e.what());
      return nullptr;
    }

    connectors_.push_back(std::dynamic_pointer_cast<bamboo::net::ConnectorIf>(ptr));

    return ptr;
  }

  /// 获取调度管理类
  virtual bamboo::schedule::Scheduler& GetScheduler() final;

  /// 根据索引返回监听助手类
  virtual bamboo::net::AcceptorPtr GetAcceptor(uint32_t index = 0) final;

  /// 根据索引返回链接助手类
  virtual bamboo::net::ConnectorPtr GetConnector(uint32_t index = 0) final;

 protected:
  /// 默认构造函数，只能被 AioIf 类所构建
  ServerIf(boost::asio::io_context& io, std::string name, std::size_t index);

  friend class bamboo::aio::AioIf;

  /// 启动前的准备函数
  virtual bool PrepareStart() = 0;

  /// 启动后的完成函数
  virtual bool FinishStart() = 0;

  /// 关闭的处理函数
  virtual void StopHandle() = 0;

 private:
  std::string name_;
  boost::asio::io_context& io_;
  std::vector<bamboo::net::AcceptorPtr> acceptors_;
  std::vector<bamboo::net::ConnectorPtr> connectors_;
  std::unique_ptr<bamboo::schedule::Scheduler> scheduler_;
  const std::size_t ioIndex_{0};
};

using ServerPtr = std::shared_ptr<ServerIf>;

}
}