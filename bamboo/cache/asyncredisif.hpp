#pragma once

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace bamboo {
namespace cache {

class AsyncRedisIf {
 public:
  AsyncRedisIf(boost::asio::io_context& io);
  virtual ~AsyncRedisIf();

  virtual void SetAddress(const char* address, std::uint16_t port) final;
  virtual bool Connect() final;

  using ConnectCallback = std::function<void(int)>;
  virtual void SetConnectCallback(ConnectCallback cb) final;

  using DisconnectCallback = std::function<void(int)>;
  virtual void SetDisconnectCallback(DisconnectCallback cb) final;

  virtual bool IsConnected() final;
  virtual void Disconnect() final;

 protected:
  virtual void AsyncCommand(void* data, const char* format, ...) final;
  virtual void AsyncVCommand(void* data, const char* format, va_list ap) final;

  virtual void AsyncCommandCallback(redisReply* reply, void* data) = 0;

 private:
  virtual void waitRead() final;
  virtual void waitWrite() final;

  static void RedisConnectCallback(const struct redisAsyncContext*, int status);
  static void RedisDisconnectCallback(const struct redisAsyncContext*, int status);
  static void RedisAddRead(void* privdata);
  static void RedisDelRead(void* privdata);
  static void RedisAddWrite(void* privdata);
  static void RedisDelWrite(void* privdata);
  static void RedisCleanup(void* privdata);
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