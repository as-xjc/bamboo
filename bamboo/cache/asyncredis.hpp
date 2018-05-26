#pragma once

#include <boost/utility/string_view.hpp>
#include <bamboo/cache/asyncredisif.hpp>

namespace bamboo {
namespace cache {

/**
 * redis 异步客户端
 */
class AsyncRedis : public AsyncRedisIf {
 public:
  using AsyncRedisIf::AsyncRedisIf;
  virtual ~AsyncRedis();

  /// 异步命令回调处理函数
  using CommandCallback = std::function<void(redisReply*)>;

  /// 异步执行命令，设置回调函数
  void Command(CommandCallback cb, const char *format, ...);

  /// 异步执行命令，无回调
  void Command(const char *format, ...);

  /// 发布信息
  void Publish(boost::string_view channel, boost::string_view message);

 protected:
  /// redis 异步回调处理函数
  void AsyncCommandCallback(redisReply* reply, void* data) override;

 private:
  uint32_t genId();

  uint32_t id_{0};
  std::map<uint32_t, CommandCallback> commands_;
};

}
}

