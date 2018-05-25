#include "bamboo/cache/asyncredis.hpp"

#include "bamboo/log/log.hpp"

namespace bamboo {
namespace cache {

AsyncRedis::~AsyncRedis() {}

uint32_t AsyncRedis::genId() {
  ++id_;
  if (id_ == 0) ++id_;

  return id_;
}

void AsyncRedis::Command(AsyncRedis::CommandCallback cb, const char* format, ...) {
  if (!IsConnected()) return;

  uint32_t id = genId();
  commands_[id] = std::move(cb);

  auto data = reinterpret_cast<void*>(id);
  va_list ap;
  va_start(ap, format);
  AsyncVCommand(data, format, ap);
  va_end(ap);
}

void AsyncRedis::Command(const char* format, ...) {
  if (!IsConnected()) return;

  va_list ap;
  va_start(ap, format);
  AsyncVCommand(nullptr, format, ap);
  va_end(ap);
}

void AsyncRedis::AsyncCommandCallback(redisReply* reply, void* data) {
  if (data == nullptr) return;

  auto id = static_cast<uint32_t>(reinterpret_cast<std::ptrdiff_t>(data));

  auto it = commands_.find(id);
  if (it != commands_.end()) {
    if (it->second) it->second(reply);

    commands_.erase(it);
  }
}

void AsyncRedis::Publish(boost::string_view channel, boost::string_view message) {
  AsyncCommand(nullptr, "PUBLISH %b %b", channel.data(), channel.size(), message.data(), message.size());
}

}
}