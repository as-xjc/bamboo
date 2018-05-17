#pragma once

#include <bamboo/define.hpp>

#include <bamboo/protocol/messageif.hpp>

namespace bamboo {
namespace net {

/**
 * socket基类
 */
class SocketIf : public boost::asio::ip::tcp::socket {
 public:

  /// 默认的构造函数
  explicit SocketIf(boost::asio::ip::tcp::socket&& socket);

  /// 默认的析构函数
  virtual ~SocketIf();

  /// 继承基类的构造函数
  using boost::asio::ip::tcp::socket::basic_stream_socket;

  /// 设置id
  virtual void SetId(uint32_t id) final;

  /// 获取id
  virtual uint32_t GetId() final ;

  /// 数据可读处理函数
  virtual void ReadData() = 0;

  /**
   * 发送数据
   * @param data 待发送数据
   * @param size 长度
   */
  virtual void WriteData(const char* data, std::size_t size) = 0;

  /**
   * 发送数据
   * @param buffer 待发送数据对象
   */
  virtual void WriteData(std::unique_ptr<std::string>&& buffer) = 0;

  /**
   * 发送数据
   * @param message 待发送消息
   */
  virtual void WriteData(bamboo::protocol::MessageIf* message) = 0;

  /// 关闭处理
  virtual void Close() = 0;

  /// 数据可读回调函数类型
  using ReadHandler = std::function<std::size_t(const char*, std::size_t)>;

  /// 设置数据可读的回调函数
  virtual void SetReadHandler(ReadHandler&& handle) final;

  /// 获取数据可读回调函数
  virtual ReadHandler& GetReadHandler() final;

  /// socket关闭回调函数类型
  using CloseHandler = std::function<void()>;

  /// 设置关闭回调函数
  virtual void SetCloseHandler(CloseHandler&& handle) final;

  /// 获取关闭回调函数
  virtual CloseHandler& GetCloseHandler() final;

 private:
  uint32_t id_{0};
  ReadHandler reader_;
  CloseHandler closer_;
};

using SocketPtr = std::shared_ptr<SocketIf>;

}
}
