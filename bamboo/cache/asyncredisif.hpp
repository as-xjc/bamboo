#pragma once

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace bamboo {
namespace cache {

/**
 * 异步redis客户端基类
 */
class AsyncRedisIf {
 public:
  AsyncRedisIf(boost::asio::io_context& io);
  virtual ~AsyncRedisIf();

  /// 设置redis地址
  virtual void SetAddress(const char* address, std::uint16_t port) final;

  /// 连接到redis
  virtual bool Connect() final;

  /// 连接回调函数
  using ConnectCallback = std::function<void(int)>;

  /// 设置连接回调函数
  virtual void SetConnectCallback(ConnectCallback cb) final;

  /// 连接断开回调函数
  using DisconnectCallback = std::function<void(int)>;

  /// 设置连接断开回调函数
  virtual void SetDisconnectCallback(DisconnectCallback cb) final;

  /// 判断是否连接中
  virtual bool IsConnected() final;

  /// 主动断开连接
  virtual void Disconnect() final;

 protected:
  /// 异步执行命令函数
  virtual void AsyncCommand(void* data, const char* format, ...) final;
  virtual void AsyncVCommand(void* data, const char* format, va_list ap) final;

  /// 异步回调处理函数
  virtual void AsyncCommandCallback(redisReply* reply, void* data) = 0;

 private:
  /// 启动读事件
  virtual void waitRead() final;

  /// 启动写事件
  virtual void waitWrite() final;

  /// redis 异步相关的回调函数
  static void RedisConnectCallback(const struct redisAsyncContext*, int status);
  static void RedisDisconnectCallback(const struct redisAsyncContext*, int status);
  static void RedisAddRead(void* privdata);
  static void RedisDelRead(void* privdata);
  static void RedisAddWrite(void* privdata);
  static void RedisDelWrite(void* privdata);
  static void RedisCleanup(void* privdata);

  /// redis 异步回调处理的总入口
  static void CommandHandle(struct redisAsyncContext*, void* r, void* data);

  boost::asio::ip::tcp::socket socket_;
  bool isRead_{true};
  bool isReading_{false};
  bool isWrite_{true};
  bool isWriting_{false};

  redisAsyncContext* context_{nullptr};
  ConnectCallback connectCallback_;
  DisconnectCallback disconnectCallback_;

  std::string ip_;
  uint16_t port_{0};
};

}
}