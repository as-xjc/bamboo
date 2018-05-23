#pragma once

#include <bamboo/define.hpp>

#include <bamboo/protocol/protocolif.hpp>
#include <bamboo/net/socket.hpp>

namespace bamboo {
namespace net {

/**
 * 链接管理类基类
 */
class ConnManagerIf {
 public:
  /// 默认的构造函数
  ConnManagerIf();

  /// 默认的析构函数
  virtual ~ConnManagerIf();

  /**
   * 新链接建立的处理函数
   * @param socket 新链接
   * @return socket对象
   */
  virtual SocketPtr OnConnect(boost::asio::ip::tcp::socket&& socket) = 0;

  /**
   * 链接删除的处理
   * @param socket 链接Id
   */
  virtual void OnDelete(uint32_t socket) = 0;

  /**
   * 往socket发送数据
   * @param socket 要发送的链接id
   * @param data 待发送的数据
   * @param size 数据长度
   */
  virtual void WriteData(uint32_t socket, const char* data, std::size_t size) = 0;

  /// 链接数据可读的回调函数类型
  using ReadHandler = std::function<std::size_t(SocketPtr, const char*, std::size_t)>;

  /// 设置数据可读的回调函数
  virtual void SetReadHandler(ReadHandler&& handle) final;

  /// 获取数据可读的回调函数
  virtual ReadHandler& GetReadHandler() final;

  /**
   * 创建协议类
   *
   * @note 只能拥有一个可读回调函数，因此只能通过 CreateProtocol 或 SetReadHandler 两者中一个设置
   *
   * @tparam PROTOCOL 协议类，必须派生于 bamboo::protocol::ProtocolIf
   * @tparam ARGS 参数类型
   * @param args 构造函数参数
   * @return 协议对象的共享指针
   */
  template<typename PROTOCOL, typename... ARGS>
  std::shared_ptr<PROTOCOL> CreateProtocol(ARGS&& ... args) {
    static_assert(std::is_base_of<bamboo::protocol::ProtocolIf, PROTOCOL>::value,
                  "protocol must be base of bamboo::protocol::ProtocolIf");
    BB_ASSERT(!protocol_);
    BB_ASSERT(!reader_);

    auto ptr = std::make_shared<PROTOCOL>(std::forward<ARGS>(args)...);
    protocol_ = std::dynamic_pointer_cast<bamboo::protocol::ProtocolIf>(ptr);
    reader_ = [this](bamboo::net::SocketPtr so, const char* data, std::size_t size) -> std::size_t {
      return protocol_->ReceiveData(so, data, size);
    };
    return ptr;
  }

  /// 获取协议对象
  virtual bamboo::protocol::ProtocolPtr GetProtocol() final;

  /// 新链接建立的回调处理函数类型
  using ConnectHandler = std::function<void(SocketPtr)>;

  /// 设置新链接建立的回调处理函数
  virtual void SetConnectHandler(ConnectHandler&& handle) final;

  /// 获取新链接建立的回调处理函数
  virtual ConnectHandler& GetConnectHandler() final;

  /// 链接删除的回调处理函数
  using DeleteHandler = std::function<void(SocketPtr)>;

  /// 设置链接删除的回调函数
  virtual void SetDeleteHandler(DeleteHandler&& handle) final;

  /// 获取链接删除的回调函数
  virtual DeleteHandler& GetDeleteHandler() final;

 private:
  ReadHandler reader_;
  DeleteHandler deleter_;
  ConnectHandler connector_;
  bamboo::protocol::ProtocolPtr protocol_;
};

using ConnManagerPtr = std::shared_ptr<ConnManagerIf>;

}
}