#include <bamboo/cache/asyncredissubscriber.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <bamboo/log/log.hpp>

namespace bamboo {
namespace cache {

AsyncRedisSubscriber::~AsyncRedisSubscriber() {}

void AsyncRedisSubscriber::Subscribe(boost::string_view channel, AsyncRedisSubscriber::SubscriberCallback cb) {
  AsyncCommand(nullptr, "SUBSCRIBE %s", channel.data());
  subscribes_[channel.data()] = std::move(cb);
}

void AsyncRedisSubscriber::Subscribe(const std::vector<std::string>& channels, AsyncRedisSubscriber::SubscriberCallback cb) {
  if (channels.empty()) return;

  std::string c = boost::algorithm::join(channels, " ");
  AsyncCommand(nullptr, "SUBSCRIBE %s", c.c_str());

  for (auto& channel : channels) {
    subscribes_[channel] = cb;
  }
}

void AsyncRedisSubscriber::PSubscribe(boost::string_view channel, AsyncRedisSubscriber::SubscriberCallback cb) {
  AsyncCommand(nullptr, "PSUBSCRIBE %s", channel.data());
  psubscribes_[channel.data()] = std::move(cb);
}

void AsyncRedisSubscriber::PSubscribe(const std::vector<std::string>& channels, AsyncRedisSubscriber::SubscriberCallback cb) {
  if (channels.empty()) return;

  std::string c = boost::algorithm::join(channels, " ");
  AsyncCommand(nullptr, "PSUBSCRIBE %s", c.c_str());

  for (auto& channel : channels) {
    psubscribes_[channel] = cb;
  }
}

void AsyncRedisSubscriber::PUnsubscribe() {
  AsyncCommand(nullptr, "PUNSUBSCRIBE");
}

void AsyncRedisSubscriber::PUnsubscribe(const std::vector<std::string>& channels) {
  std::string c = boost::algorithm::join(channels, " ");
  AsyncCommand(nullptr, "PUNSUBSCRIBE %s", c.c_str());
}

void AsyncRedisSubscriber::Unsubscribe() {
  AsyncCommand(nullptr, "UNSUBSCRIBE");
}

void AsyncRedisSubscriber::Unsubscribe(const std::vector<std::string>& channels) {
  std::string c = boost::algorithm::join(channels, " ");
  AsyncCommand(nullptr, "UNSUBSCRIBE %s", c.c_str());
}

void AsyncRedisSubscriber::AsyncCommandCallback(redisReply* reply, void* data) {
  if (reply->type == REDIS_REPLY_ERROR) {
    BB_ERROR_LOG("Subscriber async callback error:%s", reply->str);
    return;
  }

  if (reply->type != REDIS_REPLY_ARRAY) return;
  if (reply->elements < 1) return;

  boost::string_view cmd = reply->element[0]->str;
  if (cmd == "subscribe") {
    // 判断订阅是否成功
  } else if (cmd == "unsubscribe") {
    if (reply->elements < 3) return;

    boost::string_view channels(reply->element[1]->str, reply->element[1]->len);
    std::vector<std::string> v;
    boost::algorithm::split(v, channels, boost::algorithm::is_any_of(" "));
    for (auto& channel : v) {
      subscribes_.erase(channel);
    }
  } else if (cmd == "message") {
    if (reply->elements < 3) return;

    boost::string_view channel(reply->element[1]->str, reply->element[1]->len);
    boost::string_view message(reply->element[2]->str, reply->element[2]->len);
    auto it = subscribes_.find(channel.data());
    if (it != subscribes_.end()) {
      if (it->second) it->second(channel.to_string(), message.to_string());
    }
  } else if (cmd == "psubscribe") {
    // 判断订阅是否成功
  } else if (cmd == "punsubscribe") {
    if (reply->elements < 3) return;

    boost::string_view channel(reply->element[1]->str, reply->element[1]->len);
    psubscribes_.erase(channel.data());
  } else if (cmd == "pmessage") {
    if (reply->elements < 4) return;

    boost::string_view channel(reply->element[2]->str, reply->element[2]->len);
    boost::string_view message(reply->element[3]->str, reply->element[3]->len);
    auto it = psubscribes_.find(reply->element[1]->str);
    if (it != psubscribes_.end()) {
      if (it->second) it->second(channel.to_string(), message.to_string());
    }
  }
}

}
}