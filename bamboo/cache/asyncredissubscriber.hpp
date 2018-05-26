#pragma once

#include <vector>
#include <boost/utility/string_view.hpp>
#include "asyncredisif.hpp"

namespace bamboo {
namespace cache {

/**
 * 异步 redis 订阅客户端
 */
class AsyncRedisSubscriber : public AsyncRedisIf {
 public:
  using AsyncRedisIf::AsyncRedisIf;
  virtual ~AsyncRedisSubscriber();

  /// 订阅回调函数类型
  using SubscriberCallback = std::function<void(std::string, std::string)>;

  /// 订阅频道
  virtual void Subscribe(boost::string_view channel, SubscriberCallback cb);

  /// 订阅多个频道
  virtual void Subscribe(const std::vector<std::string>& channels, SubscriberCallback cb);

  /// 退订所有频道
  virtual void Unsubscribe();

  /// 退订频道
  virtual void Unsubscribe(const std::vector<std::string>& channels);

  /// 订阅模式
  virtual void PSubscribe(boost::string_view channel, SubscriberCallback cb);

  /// 订阅多个模式
  virtual void PSubscribe(const std::vector<std::string>& channels, SubscriberCallback cb);

  /// 退订所有模式
  virtual void PUnsubscribe();

  /// 退订模式
  virtual void PUnsubscribe(const std::vector<std::string>& channels);

 protected:
  /// redis 异步回调处理函数
  void AsyncCommandCallback(redisReply* reply, void* data) override;

 private:
  std::map<std::string, SubscriberCallback> subscribes_;
  std::map<std::string, SubscriberCallback> psubscribes_;
};

}
}