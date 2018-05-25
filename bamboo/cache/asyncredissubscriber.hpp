#pragma once

#include <vector>
#include <boost/utility/string_view.hpp>
#include "asyncredisif.hpp"

namespace bamboo {
namespace cache {

class AsyncRedisSubscriber : public AsyncRedisIf {
 public:
  using AsyncRedisIf::AsyncRedisIf;
  virtual ~AsyncRedisSubscriber();

  using SubscriberCallback = std::function<void(std::string, std::string)>;

  virtual void Subscribe(boost::string_view channel, SubscriberCallback cb);
  virtual void Subscribe(const std::vector<std::string>& channels, SubscriberCallback cb);
  virtual void Unsubscribe();
  virtual void Unsubscribe(const std::vector<std::string>& channels);

  virtual void PSubscribe(boost::string_view channel, SubscriberCallback cb);
  virtual void PSubscribe(const std::vector<std::string>& channels, SubscriberCallback cb);
  virtual void PUnsubscribe();
  virtual void PUnsubscribe(const std::vector<std::string>& channels);

 protected:
  void AsyncCommandCallback(redisReply* reply, void* data) override;

 private:
  std::map<std::string, SubscriberCallback> subscribes_;
  std::map<std::string, SubscriberCallback> psubscribes_;
};

}
}