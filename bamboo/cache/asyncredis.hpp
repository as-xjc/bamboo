#pragma once

#include <boost/utility/string_view.hpp>
#include <bamboo/cache/asyncredisif.hpp>

namespace bamboo {
namespace cache {

class AsyncRedis : public AsyncRedisIf {
 public:
  using AsyncRedisIf::AsyncRedisIf;
  virtual ~AsyncRedis();

  using CommandCallback = std::function<void(redisReply*)>;
  void Command(CommandCallback cb, const char *format, ...);
  void Command(const char *format, ...);
  void Publish(boost::string_view channel, boost::string_view message);

 protected:
  void AsyncCommandCallback(redisReply* reply, void* data) override;

 private:
  uint32_t genId();

  uint32_t id_{0};
  std::map<uint32_t, CommandCallback> commands_;
};

}
}

