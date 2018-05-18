#include "bamboo/cache/redis.hpp"

#include <bamboo/log/log.hpp>

namespace bamboo {
namespace cache {

Redis::Redis() : redis_(nullptr, nullptr) {}
Redis::~Redis() {}

bool Redis::Connect(const char* address, uint16_t port) {
  std::unique_ptr<redisContext, decltype(&redisFree)> ptr(redisConnect(address, port), &redisFree);
  if (!ptr) {
    BB_ERROR_LOG("redis connect null");
    return false;
  }

  if (ptr->err) {
    BB_ERROR_LOG("redis connect error:%s", ptr->errstr);
    return false;
  }

  redis_ = std::move(ptr);
  return true;
}

std::shared_ptr<redisReply> Redis::Command(const char* format, ...) {
  if (!redis_) return nullptr;
  va_list ap;
  void* reply = nullptr;
  va_start(ap, format);
  reply = redisvCommand(redis_.get(), format, ap);
  va_end(ap);
  return std::shared_ptr<redisReply>(reinterpret_cast<redisReply*>(reply), &freeReplyObject);
}

bool Redis::Ping() {
  if (!redis_) return false;

  auto reply = Command("PING");
  if (reply) {
    if (reply->type == REDIS_REPLY_ERROR) {
      BB_ERROR_LOG("ping redis error:%s", reply->str);
      return false;
    } else {
      return true;
    }
  }

  auto ret = redisReconnect(redis_.get());
  if (ret == REDIS_OK) {
    BB_INFO_LOG("re-connection redis ok");
    return true;
  } else {
    BB_ERROR_LOG("re-connection redis fail");
    return false;
  }
}

Redis::operator bool() const {
  return redis_ != nullptr;
}

bool Redis::Auth(boost::string_view view) {
  if (!redis_) return false;

  auto reply = Command("AUTH %b", view.data(), view.size());
  return reply && reply->type == REDIS_REPLY_STATUS && std::strcmp(reply->str, "OK") == 0;
}

}
}