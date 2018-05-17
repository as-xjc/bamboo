#pragma once

#include <memory>

#include <bamboo/define.hpp>

#include <bamboo/net/connmanagerif.hpp>

namespace bamboo {

namespace server {
class ServerIf;
}

namespace net {

/**
 * 监听助手类基类
 */
class AcceptorIf {
 public:

  /// 默认的析构函数
  virtual ~AcceptorIf();

  friend class bamboo::server::ServerIf;

  /**
   * 设置监听的地址
   * @param address 地址
   * @param port 端口
   */
  virtual void SetAddress(std::string address, uint16_t port) final;

  /// 开始处理
  virtual void Start() = 0;

  /// 停止处理
  virtual void Stop() = 0;

  /**
   * 创建链接管理类
   * @tparam CONNMANGER 链接管理类，必须派生于 bamboo::net::ConnManagerIf
   * @tparam ARGS 参数类型
   * @param args 构造函数参数
   * @return 对象的共享指针
   */
  template<typename CONNMANGER, typename... ARGS>
  std::shared_ptr<CONNMANGER> CreateConnManager(ARGS&& ... args) {
    static_assert(std::is_base_of<ConnManagerIf, CONNMANGER>::value,
                  "connManager must be base of bamboo::net::ConnManagerIf");
    BB_ASSERT(connManager_ == nullptr);

    auto ptr = std::make_shared<CONNMANGER>(std::forward<ARGS>(args)...);
    connManager_ = std::dynamic_pointer_cast<ConnManagerIf>(ptr);
    return ptr;
  }

  /**
   * 设置管理类
   *
   * @note 一个监听助手只能拥有一个管理类，因此管理类只能通过 CreateConnManager 或 SetConnManager 两者中一个设置
   *
   * @param mgr 管理类
   */
  virtual void SetConnManager(ConnManagerPtr& mgr) final;

  /// 获取链接管理类
  virtual ConnManagerPtr GetConnManager() final;

 protected:
  /// 构造函数，只能被server类创建
  AcceptorIf(boost::asio::io_context& io);

  ConnManagerPtr connManager_;
  std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
  boost::asio::io_context& io_;
};

using AcceptorPtr = std::shared_ptr<AcceptorIf>;

}
}
