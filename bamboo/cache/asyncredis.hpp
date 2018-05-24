#pragma once

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace bamboo {
namespace cache {

class AsyncRedis final {
 public:
  AsyncRedis(boost::asio::io_context& io);
  virtual ~AsyncRedis();

  void SetAddress(const char* address, std::uint16_t port);
  bool Connect();

  using ConnectCallback = std::function<void(int)>;
  void SetConnectCallback(ConnectCallback cb);

  bool IsConnected();
  void Disconnect();

  using DisconnectCallback = std::function<void(int)>;
  void SetDisconnectCallback(DisconnectCallback cb);

  using CommandCallback = std::function<void(redisReply*)>;
  void Command(CommandCallback cb, const char *format, ...);
  void Command(const char *format, ...);

 private:
  uint32_t genId();
  void waitRead();
  void waitWrite();
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
  bool isWrite_{true};

  redisAsyncContext* context_{nullptr};

  ConnectCallback connectCallback_;
  DisconnectCallback disconnectCallback_;
  uint32_t id_{0};
  std::map<uint32_t, CommandCallback> commands_;

  std::string ip_;
  uint16_t port_{0};
};

}
}

